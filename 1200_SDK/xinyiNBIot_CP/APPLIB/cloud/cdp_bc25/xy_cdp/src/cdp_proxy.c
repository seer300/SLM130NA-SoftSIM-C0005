
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#if TELECOM_VER
#include "xy_at_api.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "at_cdp.h"
#include "xy_cdp_api.h"
#include "net_app_resume.h"
#include "ps_netif_api.h"
#include "cdp_backup.h"
#include "cloud_proxy.h"

#define CDP_PROXY_TIMEOUT  96//处理超时时间
extern downstream_info_t *downstream_info;


typedef enum
{
	CDP_TYPE_SEND =0,
	CDP_TYPE_DEREG ,
	CDP_TYPE_UPDATE ,
}cdp_proxy_datatype_e;

//AT+XYCONFIG=1,221.229.214.202
proxy_config_callback cdpProxyConfigProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	int ret = XY_OK;

	cdp_module_init();

	if (req_type == AT_CMD_REQ)
	{
		uint8_t* serverIP =  xy_malloc(strlen(paramList));
		int serverPort = 5683;
		int lifetime = 86400;

	    if (at_parse_param(",%s,%d,%d", paramList,serverIP, &serverPort, &lifetime) != AT_OK)
	    {
	    	xy_printf(0,XYAPP, WARN_LOG, "[CDPDEMO]Err:  cdpProxyConfig failed");
	    	ret = XY_ERR;
	    }

		if( cdp_cloud_setting(serverIP, serverPort))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[CDPDEMO]Err: cdp_cloud_setting failed");
			ret =  XY_ERR;
		}
		
		if(cdp_register(lifetime, CDP_PROXY_TIMEOUT))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[CDPDEMO]Err: register cdp failed");
			ret =  XY_ERR;
		}
		
		xy_free(serverIP);        		
	}
	else if(req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(32);
		if(is_cdp_running())
		{
			sprintf(*prsp_cmd, "+XYCONFIG:success");
		}
		else
			sprintf(*prsp_cmd, "+XYCONFIG:fail");
	}
	else
		xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY]err req_type ");

	return ret;
}

//AT+XYRECV
//AT+XYRECV?
proxy_recv_callback cdpProxyRecvProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	cdp_module_init();	

	if (req_type == AT_CMD_ACTIVE)
	{
		int data_len = 0;
		char *data = xy_malloc(60 + data_len*2);
		*prsp_cmd = xy_malloc(40);
	
		data = (char *)get_message_via_lwm2m((int *)&data_len);
		if(data_len == 0)
		{
			sprintf(*prsp_cmd, "+XYRECV:0");
		}
		else
		{
			sprintf(*prsp_cmd,"+XYRECV:%d,", data_len);
			bytes2hexstr(data, data_len, *prsp_cmd + strlen(*prsp_cmd), data_len * 2+1);
		}
		if(data)
			xy_free(data);
	}
	else if(req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);
		if(downstream_info == NULL)
			sprintf(*prsp_cmd, "\r\nXYBUF=0,XYRCV=0,XYDROP=0");
		else
			sprintf(*prsp_cmd, "\r\nXYBUF=%d,XYRCV=%d,XYDROP=%d", 
		    get_downstream_message_buffered_num(), get_downstream_message_received_num(), get_downstream_message_dropped_num());
	}
	else
		xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY]err req_type ");

    return XY_OK;
}

//AT+XYSEND=0,3,112233 发送数据
//AT+XYSEND=1 去注册
//AT+XYSEND=2 更新
proxy_send_callback cdpProxySendProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd)
{
	int ret = XY_OK;

	cdp_module_init();

	if (req_type == AT_CMD_REQ)
	{
		int dataType = -1;
		int dataLen = -1;
		char* data = xy_malloc(strlen(paramList));
	
		if (at_parse_param("%d,%d,%s", paramList,&dataType, &dataLen, data) != AT_OK || dataType == -1 ||(dataLen!=-1 && data== NULL))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY] parse error\r\n");
			ret = XY_ERR;
		}

		switch(dataType)
		{
			case CDP_TYPE_SEND:
				xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY]Send");
				char* trans_data = xy_malloc(dataLen);
			
				if(strlen(data)!=dataLen * 2)
				{
					ret = XY_ERR;
				}
				
				if (hexstr2bytes(data, dataLen * 2, trans_data, dataLen) == XY_ERR)
				{
					ret = XY_ERR;
				}
				ret = cdp_send_syn(trans_data,dataLen,cdp_CON);
				if(ret != XY_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "cdp send err\r\n");
				}
				xy_free(trans_data);
				break;
			case CDP_TYPE_DEREG:
				ret = cdp_deregister(CDP_PROXY_TIMEOUT);
				if(ret != XY_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "cdp deregister err\r\n");
				}
				break;
			case CDP_TYPE_UPDATE:
				ret = cdp_lifetime_update(CDP_PROXY_TIMEOUT);
				if(ret != XY_OK)
				{
					xy_printf(0,XYAPP, WARN_LOG, "cdp update err\r\n");
				}
				break;
			default:
				break;
		}
		xy_free(data);
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[CDP_PROXY]err req_type ");
		ret = XY_ERR;
	}
		
	return ret;
}

#endif
