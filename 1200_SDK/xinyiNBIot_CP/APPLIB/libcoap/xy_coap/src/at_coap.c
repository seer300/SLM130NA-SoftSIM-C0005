/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <coap.h>
#include "lwip/sockets.h"
#include "xy_utils.h"
#include "at_com.h"

#include "ps_netif_api.h"
#include "xy_at_api.h"
#include "xy_coap_api.h"

uint8_t g_coap_serverip[20] = {0};
extern bool xy_coap_messgae_process_can_quit(coap_context_t *ctx);
extern void xy_coap_clear_option();
extern coap_context_t *xy_coap_client_context_init(void);
extern int xy_coap_create_socket(coap_context_t  *ctx,char *ipaddr,unsigned short port);

typedef struct CoapOptionData
{
    int optNum;
    unsigned char *optValue;
} CoapOptionData;

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
coapClientConf coapClientCfg = {0};

osThreadId_t g_coaprecvpacket_handle = NULL;
int g_stop_recv_packet_flag = 0;

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/

/*******************************************************************************************
 Function    : coap_recv_packet_task
 Description : receive COAP packet function task
 Input       : void
 Return      : void
 *******************************************************************************************/
void coap_recv_packet_task()
{
    xy_printf(0,XYAPP, WARN_LOG,"[COAP] recv packet thread begin\n");
    //wait PDP active
    if(!xy_tcpip_is_ok())
        xy_assert(0);

    xy_coap_asy_receive(coapClientCfg.coap_client,&g_stop_recv_packet_flag);

	g_coaprecvpacket_handle = NULL;
	osThreadExit();

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] recv packet thread end\n");
}

/*******************************************************************************************
 Function    : coap_task_create
 Description : create recv COAP packet task
 Input       : void
 Return      : void
 *******************************************************************************************/
void coap_task_create()
{
    osThreadAttr_t task_attr = {0};

    if (g_coaprecvpacket_handle != NULL)
        return;
	
	task_attr.name = "coap_recv_packet";
	task_attr.priority = osPriorityNormal1;
	task_attr.stack_size = osStackShared;
    g_coaprecvpacket_handle = osThreadNew((osThreadFunc_t)(coap_recv_packet_task), NULL, &task_attr);
}
/*****************************************************************************
 Function    : at_COAPCREATE_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPCREATE=<server>,<port>
 *****************************************************************************/
int at_COAPCREATE_req(char *at_buf, char **prsp_cmd)
{
    int port = 0;
    char * remote_ip    = xy_malloc(strlen(at_buf));
	memset(remote_ip, 0, strlen(at_buf));
	int ret = AT_END;

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CREATE BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
    	
        ret =  (ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!xy_tcpip_is_ok()) {
        ret = (ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

    if (at_parse_param("%s(),%d(1-65535)", at_buf, remote_ip,&port) != AT_OK || coapClientCfg.coap_client != NULL)
    {
        ret = (ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    coapClientCfg.coap_client = xy_coap_client_create(remote_ip,port);

    if (coapClientCfg.coap_client == NULL ) /*create failed*/
    {
        ret =  (ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    coap_task_create();
    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CREATE END\n");

ERR_PROC:
    if(remote_ip)
        xy_free(remote_ip);
    return ret;
}

/*****************************************************************************
 Function    : at_COAPDEL_req
 Description : delete COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPDEL
 *****************************************************************************/
int at_COAPDEL_req(char *at_buf, char **prsp_cmd)
{
    int  time = 0;

    (void) at_buf;

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] DEL BEGIN\n");

    if(g_req_type != AT_CMD_ACTIVE)
    {
        return  (ATERR_PARAM_INVALID);
    }

	if(coapClientCfg.coap_client == NULL)
    {
        goto ERR_PROC;
    }

    if(!xy_coap_messgae_process_can_quit(coapClientCfg.coap_client))
    {
        return  (ATERR_NOT_ALLOWED);
    }

    g_stop_recv_packet_flag = 1;//stop recv thread
    while(g_coaprecvpacket_handle != NULL)
    {
        time +=200;
        osDelay(200);
        if (time > 2000)
        {
            return  (ATERR_NOT_ALLOWED);
        }

    }
    xy_coap_clear_option();
    xy_coap_client_delete(coapClientCfg.coap_client);
    coapClientCfg.coap_client = NULL;
    g_stop_recv_packet_flag = 0;
	memset(g_coap_serverip,0,20);
    xy_printf(0,XYAPP, WARN_LOG,"[COAP] DEL END\n");
ERR_PROC:

    return AT_END;
}

/*****************************************************************************
 Function    : at_COAPHEAD_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPHEAD=<msgid>,<tkl>,<token>
 *****************************************************************************/
int at_COAPHEAD_req(char *at_buf, char **prsp_cmd)
{
    int  msgid = -1;
    int  tkl   = -1;
    char * token = NULL;
    token = xy_malloc(strlen(at_buf));
	memset(token, 0, strlen(at_buf));

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG HEAD BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
    	xy_free(token);
        return  (ATERR_PARAM_INVALID);
    }

    if (!xy_tcpip_is_ok()) {
		xy_free(token);
        return  (ATERR_NOT_NET_CONNECT);
    }

    if (at_parse_param("%d,%d,%s", at_buf, &msgid, &tkl, token) != AT_OK  ||msgid < 0 || msgid > 65535||tkl < 0 || tkl > 8 ||strlen(token) != (size_t)tkl)
    {
    	xy_free(token);
        return  (ATERR_PARAM_INVALID);
    }

    if (coapClientCfg.coap_client == NULL )
    {
    	xy_free(token);
        return  (ATERR_NOT_ALLOWED);
    }

    xy_config_coap_head(msgid,token,tkl);

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG HEAD END\n");

    xy_free(token);
    return AT_END;
}

int commaCount(char* buf)
{
    int count = 0;
    while(*buf != '\0')
    {
        if(*buf == ',')
            count++;

        buf++;
    }

    return count;
}

/*****************************************************************************
 Function    : at_COAPOPTION_req
 Description : config COAP options
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPOPTION=<opt_count>,<opt_name>,<opt_value>
 *****************************************************************************/
int at_COAPOPTION_req(char *at_buf, char **prsp_cmd)
{
	int ret = AT_END;
    int  i;
    int  opt_count = 0;
    char *form = NULL;
    //char* test = ",%d,%c";
    char *format = NULL;
    // void *p[] = {&opt_count};

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG OPTION BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {

        // if (at_parse_param_2("%d", at_buf, p) != AT_OK || opt_count <=0 || opt_count > 10 || commaCount(at_buf) != 2*opt_count)
        if (at_parse_param("%d(1-10)", at_buf, &opt_count) != AT_OK || commaCount(at_buf) != 2*opt_count)
        {
            return  (ATERR_PARAM_INVALID);
        }

		if (coapClientCfg.coap_client == NULL )
        {
            return  (ATERR_NOT_ALLOWED);
        }

        format = xy_malloc(opt_count*strlen(",%d,%s,"));
		memset(format, 0x00, opt_count*strlen(",%d,%s,"));
        xy_printf(0,XYAPP, WARN_LOG,"[COAP] opt_count = %d\n",opt_count);
        CoapOptionData option[opt_count];
        memset (&option,0,sizeof(CoapOptionData)*opt_count);
        form = format;
        for(i=0;i <2*opt_count;i+=2)
        {
            xy_printf(0,XYAPP, WARN_LOG,"[COAP] i = %d",i);
            option[i/2].optValue = xy_malloc(strlen(at_buf));

            strcat(form, ",%d,%s");
            if (at_parse_param(format, at_buf, &option[i / 2].optNum, option[i / 2].optValue) != AT_OK)
            {
                ret = (ATERR_PARAM_INVALID);
                goto ERR_PROC;
            }

            form[strlen(form) - 1] = '\0';
            form[strlen(form) - 1] = '\0';
            form[strlen(form) - 1] = '\0';
            form[strlen(form) - 1] = '\0';
            form[strlen(form) - 1] = ',';     
        }

        xy_coap_clear_option();

        for(i=0;i <opt_count;i++)
        {
            if(xy_config_coap_option(option[i].optNum,option[i].optValue) != XY_OK)
            {
                ret = (ATERR_PARAM_INVALID);
                goto ERR_PROC;
            }
        }

        xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG OPTION END\n");

ERR_PROC:
        for(i=0;i <opt_count;i++)
        {
            if(option[i].optValue)
                xy_free(option[i].optValue);
        }
        if(format)
            xy_free(format);

    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST) {
        *prsp_cmd = xy_malloc(80);
        snprintf(*prsp_cmd, 80, "<opt_count>,<opt_name>,\"<opt_value>\"[,...]");
    }
#endif
    else {
        return  (ATERR_PARAM_INVALID);
    }

    return ret;
}

/*****************************************************************************
 Function    : at_COAPSEND_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +COAPSEND=<type>,<method>,<data_len>,<data>
 *****************************************************************************/
int at_COAPSEND_req(char *at_buf, char **prsp_cmd)
{
	int ret = AT_END;
    int  data_len = 0;
    char *tans_data = NULL;
    char *method = NULL;
    char *type   = NULL;
    char *data   = NULL;

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] SEND BEGIN\n");

    if(g_req_type != AT_CMD_REQ)
    {
        ret = (ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (!xy_tcpip_is_ok()) {
        ret = (ATERR_NOT_NET_CONNECT);
        goto ERR_PROC;
    }

	method = xy_malloc(strlen(at_buf));
    type   = xy_malloc(strlen(at_buf));
	memset(type, 0, strlen(at_buf));
	memset(method, 0, strlen(at_buf));
	
    data = xy_malloc2(strlen(at_buf));
	if(data == NULL)
	{
		ret = (ATERR_NO_MEM);
		goto ERR_PROC;
	}
	memset(data, 0, strlen(at_buf));
	
    if (at_parse_param("%s,%s,%d,%s", at_buf, type,method,&data_len,data) != AT_OK ||strlen(method) > 6||strlen(type) > 3|| data_len > 1000 || data_len < 0 || strlen(data) != (size_t)(data_len * 2))
    {
        ret = (ATERR_PARAM_INVALID);
        goto ERR_PROC;
    }

    if (coapClientCfg.coap_client == NULL )
    {
        ret = (ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    if(data_len != 0)
    {
        tans_data = xy_malloc(data_len + 1);
        if (hexstr2bytes(data, data_len * 2, tans_data, data_len) == -1)
        {
            ret = (ATERR_PARAM_INVALID);
            goto ERR_PROC;
        }
        tans_data[data_len]= '\0';
        xy_printf(0,XYAPP, WARN_LOG,"[COAP] tans_data = %d,%s\n",tans_data,tans_data);
    }

    if (0 != xy_coap_asy_send(coapClientCfg.coap_client, type,method, tans_data,data_len))
    {
        ret = (ATERR_NOT_ALLOWED);
        goto ERR_PROC;
    }

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] SEND END\n");

ERR_PROC:
    if(method)
        xy_free(method);
    if(type)
        xy_free(type);
    if(data)
        xy_free(data);
    if(tans_data)
        xy_free(tans_data);
    return ret;
}

/*****************************************************************************
 Function    : at_QCOAPCFG_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPCFG="Showra"[,<Showra>]
               +QCOAPCFG="Showrspopt"[,<Showrspopt>]
 *****************************************************************************/
int at_QCOAPCFG_req(char *at_buf, char **prsp_cmd)
{
    int showValue = -1;
    char* showStr = xy_malloc(strlen(at_buf));
	memset(showStr, 0, strlen(at_buf));

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG BEGIN\n");

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%s,%d", at_buf, showStr,&showValue) != AT_OK  || !strcmp(showStr,""))
            goto PRAR_ERR_PROC;
		
        if(at_strcasecmp(showStr,"Showra"))
        {
            if(showValue == -1)
            {
                *prsp_cmd = xy_malloc(50);
                snprintf(*prsp_cmd,50, "\"Showra\",%d",coapClientCfg.is_show_ipport);
            }
            else if(showValue ==0 || showValue ==1 )
                coapClientCfg.is_show_ipport = showValue;
            else
                goto PRAR_ERR_PROC;

        }
        else if(at_strcasecmp(showStr,"Showrspopt"))
        {
            if(showValue == -1)
            {
                *prsp_cmd = xy_malloc(50);
                snprintf(*prsp_cmd,50, "\"Showrspopt\",%d",coapClientCfg.is_show_opt);
            }
            else if(showValue ==0 || showValue ==1 )
                coapClientCfg.is_show_opt = showValue;
            else
                goto PRAR_ERR_PROC;
        }
        else
            goto PRAR_ERR_PROC;

    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG AT_CMD_QUERY\n");
    }
#if (AT_CUT!=1)
    else if(g_req_type == AT_CMD_TEST)
    {
        xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG AT_CMD_TEST\n");
    }
#endif
    else
        goto PRAR_ERR_PROC;

    if(showStr)
        xy_free(showStr);
    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG END\n");
    return AT_END;

PRAR_ERR_PROC:
    if(showStr)
        xy_free(showStr);
    return  (ATERR_PARAM_INVALID);
}


/*****************************************************************************
 Function    : at_QCOAPADDRES_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPADDRES=<length>,"<resource>"
 *****************************************************************************/
int at_QCOAPADDRES_req(char *at_buf, char **prsp_cmd)
{
    uint8_t length   = -1;
    char* resource = xy_malloc(strlen(at_buf));
    coap_resource_t *coap_resource = NULL;
	memset(resource, 0, strlen(at_buf));

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG RES BEGIN\n");

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%1d,%s", at_buf, &length,resource) != AT_OK || length > 128 ||strlen(resource) != length)
	    {
	    	xy_free(resource);
	        return  (ATERR_PARAM_INVALID);
	    }

		if (coapClientCfg.coap_client == NULL )
	    {
	    	xy_free(resource);
	        return  (ATERR_NOT_ALLOWED);
	    }
		
	    coap_resource = coap_resource_init(coap_make_str_const(resource), 0);
	    coap_add_attr(coap_resource, coap_make_str_const(resource), coap_make_str_const(resource), 0);
	    coap_add_resource(coapClientCfg.coap_client, coap_resource);

	    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG RES END\n");
		
    }
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_malloc(60);
		snprintf(*prsp_cmd, 60, "<1-128>,\"<resource>\"");
	}
#endif
	else 
	{
		xy_free(resource);
		return  (ATERR_PARAM_INVALID);
	}
	
    xy_free(resource);
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPDATASTATUS_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : AT+QCOAPDATASTATUS?
 *****************************************************************************/
int at_QCOAPDATASTATUS_req(char *at_buf, char **prsp_cmd)
{
    uint8_t send_status = 0;

    if(g_req_type != AT_CMD_QUERY)
    {
        return  (ATERR_PARAM_INVALID);
    }

	*prsp_cmd = xy_malloc(50);
    if (coapClientCfg.coap_client == NULL )
    {
        snprintf(*prsp_cmd,50, "%d",send_status);
		return AT_END;
    }

    xy_coap_client_get_data_status(coapClientCfg.coap_client,&send_status);
    snprintf(*prsp_cmd,50, "%d",send_status);

    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPCREATE_req
 Description : create COAP client
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPCREATE=<local_port>
 *****************************************************************************/
int at_QCOAPCREATE_req(char *at_buf, char **prsp_cmd)
{
	
	uint16_t localPort = 0;

    if(g_req_type == AT_CMD_REQ)
    {
        if (at_parse_param("%2d(1-65535)", at_buf, &localPort) != AT_OK || coapClientCfg.coap_client != NULL)
	    {
	        return  (ATERR_PARAM_INVALID);
	    }
	    coapClientCfg.local_port = localPort;
	    coapClientCfg.coap_client = xy_coap_client_context_init();
    }
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST) 
	{
		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "<1-65535>");
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
	    
    return AT_END;
}

/*****************************************************************************
 Function    : at_QCOAPHEAD_req
 Description : config COAP head
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPHEAD=<mode>[,[<msgid>][,<tkl>,<token>]]
 *****************************************************************************/
int at_QCOAPHEAD_req(char *at_buf, char **prsp_cmd)
{
	int ret = AT_END;
    uint8_t mode = -1;
    int  msgid = -1;
    int  tkl   = -1;
	int  parse_num = 0;
    char * token = xy_malloc(strlen(at_buf));
    char * tokenStr = NULL;
	memset(token, 0, strlen(at_buf));

    xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG HEAD BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {

        parse_num = commaCount(at_buf)+1;
		if (at_parse_param("%1d(1-5)", at_buf, &mode) != AT_OK)
		{
			ret = (ATERR_PARAM_INVALID);
			goto ERR_PROC;
		}
		
		if (coapClientCfg.coap_client == NULL )
        {
            ret = (ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }
		
	   switch(mode)
	   {
		   case 1:
			   //Generate message ID and token values randomly.
				if(parse_num > 1) {
					ret = (ATERR_PARAM_INVALID);
			   		goto ERR_PROC;
				}
			   msgid = 0;
			   tkl = 0;
			   break;
		   case 2:
		   	   //Generate message ID randomly; configure token values.
		       if(at_parse_param(",%d(1-8),%s",at_buf,&tkl, token)!= AT_OK || strlen(token) != tkl*2 || parse_num > 3)
			   {
				   ret = (ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   msgid = 0;
			   break;
		   case 3:
			   //Configure message ID ; token value is not needed.
			  if(at_parse_param(",%d(0-65535)",at_buf,&msgid)!= AT_OK || parse_num > 2)
			   {
				   ret = (ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   tkl = 8;
			   break;
		   case 4:
			   //Configure message ID; generate token values randomly.
			   if(at_parse_param(",%d(0-65535)",at_buf,&msgid)!= AT_OK || parse_num > 2)
			   {
				   ret = (ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   tkl = 0;
			   break;
		   case 5:
			   //Configure message ID and token values.
		 	   if(at_parse_param(",%d(0-65535),%d(1-8),%s", at_buf, &msgid, &tkl, token) != AT_OK || strlen(token) != tkl*2)
			   {
				   ret = (ATERR_PARAM_INVALID);
				   goto ERR_PROC;
			   }
			   break;
	   }
	   if(tkl != 0 && strlen(token) > 0)
	   {
		   tokenStr = xy_malloc(tkl);
		   hexstr2bytes(token, tkl * 2, tokenStr, tkl);
	   }
	
	   xy_config_coap_head(msgid,tokenStr,tkl);
	
	   xy_printf(0,XYAPP, WARN_LOG,"[COAP] CONFIG HEAD END\n");

	}
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_malloc(70);
		snprintf(*prsp_cmd, 70, "<mode>[,[<msgid>][,<tkl>,<token>]]");

	}
#endif
    else {

		ret = (ATERR_PARAM_INVALID);
    }

   
ERR_PROC:
    if(token)
        xy_free(token);
    if(tokenStr)
        xy_free(tokenStr);
    return ret;
}

/*****************************************************************************
 Function    : at_QCOAPSEND_req
 Description : send COAP messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +QCOAPSEND=<type>,<method/rspcode>,<ip_addr>,<port>,<data_len>,<data>
 *****************************************************************************/
int at_QCOAPSEND_req(char *at_buf, char **prsp_cmd)
{
	int ret = AT_END;
    uint8_t type = -1;
    int method = -1;
    uint16_t port = -1;
    int  data_len = 0;
    char *data = NULL;
    char *remote_ip = NULL;

	
    xy_printf(0,XYAPP, WARN_LOG,"[COAP] SEND BEGIN\n");

    if(g_req_type == AT_CMD_REQ) {
		
		if (!xy_tcpip_is_ok()) {
			   ret = (ATERR_NOT_NET_CONNECT);
			   goto ERR_PROC;
		}
		
	   data = xy_malloc2(strlen(at_buf));
       if(data == NULL)
	   {
	       ret = (ATERR_NO_MEM);
		   goto ERR_PROC;
	   }
	   memset(data, 0x00, strlen(at_buf));
	   remote_ip = xy_malloc(80);
	   if (at_parse_param("%1d(0-3),%d,%47s(),%2d(1-65535),%l[1-1024],%h", at_buf, &type, &method, remote_ip, &port, &data_len, data) != AT_OK || (INADDR_NONE == inet_addr(remote_ip))
	   		|| method > 505) {

		   ret = (ATERR_PARAM_INVALID);
		   goto ERR_PROC;
	   }
	
       if (coapClientCfg.coap_client == NULL )
        {
            ret = (ATERR_NOT_ALLOWED);
            goto ERR_PROC;
        }


	   if(strlen(g_coap_serverip) > 0 && strncmp(remote_ip, g_coap_serverip, strlen(remote_ip)))
	   	{
	   		ret = (ATERR_NOT_ALLOWED);
            goto ERR_PROC;
	   	}

	   strncpy(g_coap_serverip, remote_ip, strlen(remote_ip));

		if(g_coaprecvpacket_handle == NULL) {
		    memcpy(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip));
			if(xy_coap_create_socket(coapClientCfg.coap_client,remote_ip,port) != XY_OK)
			{
			   ret = (ATERR_NOT_NET_CONNECT);
			   goto ERR_PROC;
			}
			coap_task_create();
		}
		else if(memcmp(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip)) != 0)
		{
		    xy_coap_clear_option();
		    coap_session_release( coapClientCfg.coap_client->sessions );
            memcpy(coapClientCfg.server_ip,remote_ip,sizeof(coapClientCfg.server_ip));
            if(xy_coap_create_socket(coapClientCfg.coap_client,remote_ip,port) != XY_OK)
			{
			   ret = (ATERR_NOT_NET_CONNECT);
			   goto ERR_PROC;
			}
		}

	
	   if (0 != xy_coap_pkt_send_ec(coapClientCfg.coap_client, type,method, data,data_len,0))
        {
           ret = (ATERR_NOT_ALLOWED);
           goto ERR_PROC;
        }
	
	   xy_printf(0,XYAPP, WARN_LOG,"[COAP] SEND END\n");
	}
#if (AT_CUT!=1)
	else if(g_req_type == AT_CMD_TEST) {

		*prsp_cmd = xy_malloc(90);
		snprintf(*prsp_cmd, 90, "<type>,<method/rspcode>,<ip_addr>,<port>[,<length>,<data>]");
	}
#endif
	else {
		ret = (ATERR_PARAM_INVALID);
    }
   
ERR_PROC:
    if(remote_ip)
        xy_free(remote_ip);
    if(data)
        xy_free(data);
    return ret;
}

static uint16_t  s_coap_inited = 0;

void at_coap_init(void)
{
    if(!s_coap_inited)
    {
        s_coap_inited = 1;
    }
    return;
}
