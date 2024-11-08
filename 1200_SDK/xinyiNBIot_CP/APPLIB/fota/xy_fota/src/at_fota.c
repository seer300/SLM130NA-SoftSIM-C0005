
#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_fota.h"



static int g_packet_sn = 0;

static uint8_t chk_sum(const void *data, uint32_t length)
{
	uint8_t ret = 0;
	int   val = 0;
	unsigned char *tmp_ptr = (unsigned char *)data;
	unsigned char *tail;

	xy_assert(((int)data%4) == 0);
	while(length>0)
	{
		val ^= *tmp_ptr;
		tmp_ptr++;
		length--;
	}
	ret ^= *((unsigned char *)&val);
	ret ^= *((unsigned char *)&val+1);
	ret ^= *((unsigned char *)&val+2);
	ret ^= *((unsigned char *)&val+3);

	tail = (unsigned char *)tmp_ptr;
	while((uint32_t)tail < (int)data+length)
	{
		ret ^= *tail;
		tail++;
	}
	return ret;
}


//下载差分包,下载一个包段，包段是FOTA的连续段，段可以是任意长度（小于512字节），但是必须按顺序提供
int at_fota_downing(char *at_buf)
{
    int len = -1;
    int sn = -1;
    uint8_t crc_sum = 0;
    char *crc_data = xy_malloc(3);
    char *save_data = NULL;
    char *src_data = xy_malloc(strlen(at_buf));

    
    if (at_parse_param(",%d,%d,%s,%s", at_buf, &sn,&len,src_data,crc_data) != AT_OK)
        goto ERR_PROC;

    if (sn != g_packet_sn)
        goto ERR_PROC;
        
    if (len > 512 || len <= 0 || strlen(src_data) != len * 2)
        goto ERR_PROC;
    
    save_data = xy_malloc(len);
    if (hexstr2bytes(src_data, len * 2, save_data, len) == -1)
        goto ERR_PROC;
    
    if (hexstr2bytes(crc_data, 2, (char *)(&crc_sum), 1) == -1)
        goto ERR_PROC;

    uint8_t sum = chk_sum(save_data, len);
    if(sum != crc_sum)
        goto ERR_PROC;
      
    if(OTA_save_one_packet(save_data, len) != XY_OK)
        goto ERR_PROC;

    g_packet_sn++;

    if(save_data)
        xy_free(save_data);
    if(src_data)
        xy_free(src_data);
    if(crc_data)
        xy_free(crc_data);

    return 0;

ERR_PROC:
    if(save_data)
        xy_free(save_data);
    if(src_data)
        xy_free(src_data);
    if(crc_data)
        xy_free(crc_data);

    return -1;
}


/*AT+NFWUPD=<cmd>[,<sn>,<len>,<data>,<crc>]  本地OTA差分包的推送与升级控制流程*/
int at_NFWUPD_req(char *at_buf, char **prsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int cmd = -1;

        if (at_parse_param("%d", at_buf, &cmd) != AT_OK)
        {
            return  (ATERR_PARAM_INVALID);
        }

        switch(cmd)
		{
	        case 0: //擦除flash并进行初始化
	        {
				OTA_upgrade_init();
        		g_packet_sn = 0;
	            break;
	        }
	        case 1: //下载差分包
	            if (at_fota_downing(at_buf))
	                return  (ATERR_NOT_ALLOWED);
	            break;
	        case 2: //校验差分包
	        {
        	    *prsp_cmd = xy_malloc(50);
				
				if (g_packet_sn == 0)
				{
				    snprintf(*prsp_cmd, 50, "+NFWUPD:PACKAGE_NOT_DOWNLOADED");
					return AT_END; 
				}
				    
				if (OTA_delta_check())
				{
			  		snprintf(*prsp_cmd, 50, "+NFWUPD:PACKAGE_VALIDATION_FAILURE");
					return AT_END; 
				}
	
				snprintf(*prsp_cmd, 50, "+NFWUPD:PACKAGE_VALIDATION_SUCCESS");
	
	            break;
	        }
	        case 3:
	        {
	            if (OTA_get_state() != XY_FOTA_DOWNLOADED) 
	                return  (ATERR_NOT_ALLOWED);

	            *prsp_cmd = xy_malloc(40);
	            snprintf(*prsp_cmd, 40, "+NFWUPD:xyDelta");
	            break;
	        }
	        case 4:
	        {
	            if (OTA_get_state() != XY_FOTA_DOWNLOADED)
	                return  (ATERR_NOT_ALLOWED);
				
	            *prsp_cmd = xy_malloc(40);
	            snprintf(*prsp_cmd, 40, "+NFWUPD:V1.0");
	             break;
	        }
	        case 5: //启动升级
	        {
			    if (OTA_upgrade_start() != XY_OK)
			    	return  (ATERR_NOT_ALLOWED);
				
	            break;
	        }
	        default:
	            return  (ATERR_NOT_ALLOWED);
		}

    }
#if (AT_CUT!=1)
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(40);
        snprintf(*prsp_cmd, 40, "+NFWUPD:(0-5)");
    }
#endif
    else
        return  (ATERR_NOT_ALLOWED);

    return AT_END;
}

/*AT+FOTACTR=<cmd>  终端用户的OTA控制*/
int at_FOTACTR_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int cmd = -1;

        if (at_parse_param("%d(0-1)", at_buf, &cmd) != AT_OK)
        {
            return  (ATERR_PARAM_INVALID);
        }
		OTA_set_permit(cmd);
	}
	/*AT+FOTACTR?*/
	else if (g_req_type == AT_CMD_QUERY)
	{
		if (OTA_is_doing())
		{
			*prsp_cmd = xy_malloc(40);
	 		snprintf(*prsp_cmd, 40, "+FOTACTR:1,1");
		}
		else
		{
			*prsp_cmd = xy_malloc(40);
	 		snprintf(*prsp_cmd, 40, "+FOTACTR:0,%d",OTA_get_permit());
		}
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(40);
        snprintf(*prsp_cmd, 40, "+FOTACTR:(0-1)");
    }
#endif
    else
        return  (ATERR_NOT_ALLOWED);
	
    return AT_OK;
}


 
