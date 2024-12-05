#include "http_config.h"
#include "http_opts.h"
#include "httpclient.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_fota.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct
{  
    unsigned int download_delta_size;
    unsigned short download_unit_index;
    unsigned short download_unit_num;
}download_info_struct;


/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/	
#define DOWNLOAD_UNIT_BYTES    1024     //HTTP包数据大小


/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
static char *g_sever_url = NULL;
static download_info_struct g_download_info = {0};
osThreadId_t g_http_fota = NULL;


/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
int ota_http_settings_init(httpclient_t *client, httpclient_data_t *client_data)
{
    int ret = -1;
    if ((client != NULL) && (client_data != NULL)) {
        ret = 0;
		
	    client_data->header_buf = (char *)xy_malloc(HTTPCLIENT_HEADER_SIZE);
        client_data->header_buf_len = HTTPCLIENT_HEADER_SIZE;

        client_data->response_buf = (char *)xy_malloc(HTTPCLIENT_RESP_SIZE);
        client_data->response_buf_len = HTTPCLIENT_RESP_SIZE;
    }
    return ret;
}

int ota_http_settings_deinit(httpclient_t *client, httpclient_data_t *client_data)
{
    int ret = -1;
    if ((client != NULL) && (client_data != NULL)) {
        ret = 0;

		xy_free(client_data->header_buf);
		xy_free(client_data->response_buf);

		httpclient_clse(client);
    }

    return ret;
}

int ota_http_request_send(httpclient_t *client, char *url, int method, httpclient_data_t *client_data)
{
    int ret = -1;
    if (client == NULL || url == NULL || client_data == NULL) {
        xy_printf(0, XYAPP, WARN_LOG, "http recv fuc input parameter err");
    } else {
		ret = httpclient_conn(client, url);
		if (ret < 0) {
			goto exit;
		}
		
		ret = httpclient_send(client, url, method, client_data);
		if (ret < 0) {
			goto exit;
		}
    }
exit:
    return ret;
}

int ota_http_recv_data(httpclient_t *client, httpclient_data_t *client_data)
{
   int ret = -1;
   if (client == NULL || client_data == NULL) {
        xy_printf(0, XYAPP, WARN_LOG, "http recv fuc input parameter err");
    } else {
        ret = httpclient_recv(client, client_data);
        if ((ret == HTTP_ERECV) || (ret == HTTP_ETIMEOUT)) {
            xy_printf(0, XYAPP, WARN_LOG, "recv ret:%d", ret);
        }
    }
    return ret;
}

void ota_get_http_header(char *out_buf, int len)
{
    if (out_buf == NULL) {
        xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]get_http_header:out buffer should not null!");
        return;
    }

    xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]download index: %d, total: %d.", g_download_info.download_unit_index, g_download_info.download_unit_num);
    snprintf((char *)out_buf, len, "Range: bytes=%d-%d\r\n", 
            g_download_info.download_unit_index * DOWNLOAD_UNIT_BYTES,
            (g_download_info.download_unit_index + 1) * DOWNLOAD_UNIT_BYTES - 1);
}

/*获取差分包大小*/
int ota_get_xyDelta_size(const char *url)
{
	int ret = -1;
	int delta_size = 0;
	int val_pos = 0, val_len = 0;
	int ota_file_size = 0;
	char send_header[64]          = {0};
	httpclient_t client           = {0};
    httpclient_data_t client_data = {0};

	ret = ota_http_settings_init(&client, &client_data);
	if (ret < 0) {
		goto exit;
	}

	strncpy(send_header, "Connection: close\r\n", sizeof(send_header));
	client.header = send_header;
	
	ret = ota_http_request_send(&client, url, HTTP_HEAD, &client_data);
	if (ret < 0) {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]request fail ret:%d", ret);
		goto exit;
	}

	do {
		ret = ota_http_recv_data(&client, &client_data);
		if (ret < 0) {
			if (ret == HTTP_ETIMEOUT)
			{	
				xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]recv ret:%d", ret);
				continue;
			}	
		}

		int response_code = httpclient_get_response_code(&client);
		if (response_code != 200)
		{
			xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]recv data error, response_code:%d", response_code);
			ret = -1;
			break;
		}
		
		if (0 == httpclient_get_response_header_value(client_data.header_buf, "Content-Length", (int *)&val_pos, (int *)&val_len)) {
			sscanf(client_data.header_buf + val_pos, "%d", &ota_file_size);
		 	xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]file size:%d", ota_file_size);
		 	ret = ota_file_size;
			break;
		}		
	}while (ret == HTTP_EAGAIN || ret == HTTP_ETIMEOUT);
	
exit:
	ota_http_settings_deinit(&client, &client_data);
    return ret;
}

/*下载差分包数据*/
int ota_get_fota_data(const char *url, char *outbuf)
{
	int ret = -1;
	int total_len = 0;
	char send_header[128]         = {0};
	httpclient_t client           = {0};
    httpclient_data_t client_data = {0};

	ret = ota_http_settings_init(&client, &client_data);
	if (ret < 0) {
		goto exit;
	}

	ota_get_http_header(send_header, 128);
	snprintf(send_header + strlen(send_header), 128 - strlen(send_header),  "Connection: close\r\n", sizeof(send_header));
	client.header = send_header;
	
	ret = ota_http_request_send(&client, url, HTTP_GET, &client_data);
	if (ret < 0) {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]request fail ret:%d", ret);
		goto exit;
	}
	
	do {
		ret = ota_http_recv_data(&client, &client_data);
		if (ret < 0) {
			if (ret == HTTP_ETIMEOUT)
			{	
				xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]recv ret:%d", ret);
				continue;
			}	
		}

		if (client_data.content_block_len) {
			memcpy(outbuf + total_len, client_data.response_buf, client_data.content_block_len);  
			total_len += client_data.content_block_len;
		}

	}while (ret == HTTP_EAGAIN || ret == HTTP_ETIMEOUT);

	ret = total_len;
	
exit:
	ota_http_settings_deinit(&client, &client_data);
    return ret;
}


void ota_http_fota_proc(const char *url)
{
    int recv_len = 0;
    unsigned int total_len = 0;
    char *data_buf = (char*)xy_malloc(DOWNLOAD_UNIT_BYTES);
    char *rsp_cmd = (char*)xy_malloc(40);

    //获取差分包信息
    g_download_info.download_delta_size = ota_get_xyDelta_size(url);
    if (g_download_info.download_delta_size > 0) {
        if (g_download_info.download_delta_size % DOWNLOAD_UNIT_BYTES)
            g_download_info.download_unit_num = (g_download_info.download_delta_size/DOWNLOAD_UNIT_BYTES) + 1;
        else
            g_download_info.download_unit_num = (g_download_info.download_delta_size/DOWNLOAD_UNIT_BYTES);
    } else {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]get delta size fail");
		goto error;
	}

	OTA_upgrade_init();
	
    //获取差分包
    for(; g_download_info.download_unit_index < g_download_info.download_unit_num; g_download_info.download_unit_index++) 
	{
        recv_len = ota_get_fota_data(url, data_buf); 
        if (recv_len > 0) {
            if (OTA_save_one_packet(data_buf, recv_len) != XY_OK) {
            	xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]OTA_save_one_packet fail");
                goto error;
            }
            
            total_len += recv_len;
            xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]get delta data %d %d", recv_len, total_len);
        } else {
			xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]get delta data fail %d", recv_len);
			goto error;
		}
    }

    if(total_len != g_download_info.download_delta_size) {
        xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]get delta size error ");
        goto error;
    }

    snprintf(rsp_cmd, 40, "+HTTPFOTA:DOWNLOAD SUCCESS");
	send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
	xy_free(rsp_cmd);
	xy_free(data_buf);
	xy_free(g_sever_url);
	g_sever_url = NULL;
	g_http_fota = NULL;
	osThreadExit();
	
error:
	snprintf(rsp_cmd, 40, "+HTTPFOTA:DOWNLOAD FAILED");
	send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
	xy_free(rsp_cmd);
	xy_free(data_buf);
	xy_free(g_sever_url);
	g_sever_url = NULL;
	g_http_fota = NULL;
	osThreadExit();
}


//AT+HTTPFOTA=<cmd>[,<data>]
int at_http_fota(char *at_buf, char **prsp_cmd)
{
    if(g_req_type == AT_CMD_REQ)
    {
        int cmd = -1;
        char *data = xy_malloc(strlen(at_buf));

        if(at_parse_param("%d(1-4),%s", at_buf, &cmd, data) != AT_OK)
        {
        	xy_free(data);
            return  (ATERR_PARAM_INVALID);
        }

        switch (cmd)
        {
	        case 1://设置服务器的URL
	        {
				if (strlen(data) < 0) {
	            	xy_free(data);
	                return  (ATERR_PARAM_INVALID);
	            }
	            if (g_sever_url != NULL) {
	                xy_free(g_sever_url);
	                g_sever_url = NULL;
	            }
	            g_sever_url = xy_malloc(strlen(data) + 1);
	            strcpy(g_sever_url, data);
	            break;
	        }
	        case 2: //下载差分包
	        {								
	    		if (g_sever_url == NULL) {
					xy_free(data);
					return  (ATERR_NOT_ALLOWED);
				}
				
				if (g_http_fota != NULL)
				{
					xy_free(data);
					return  (ATERR_NOT_ALLOWED);
				}
				
                osThreadAttr_t attr = {0};
                attr.name = "http_fota_task";
                attr.priority = osPriorityNormal1;
				attr.stack_size = osStackShared;
                g_http_fota = osThreadNew((osThreadFunc_t)ota_http_fota_proc, g_sever_url, &attr);
	            break;
	        }
	        case 3://校验升级
	        {
	            if (OTA_delta_check()){
	            	xy_free(data);
	            	return  (ATERR_NOT_ALLOWED);
	            }

	            xy_printf(0, XYAPP, WARN_LOG, "[HTTP_FOTA]update start!");
	            OTA_upgrade_start();
	            break;
	        }
	        case 4://查询升级结果，查询以后，自动清除
	        {
	            *prsp_cmd = xy_malloc(40);
	            if(OTA_get_upgrade_result() == XY_OK) {
	            	
	                snprintf(*prsp_cmd, 40, "+HTTPFOTA:UPDATE SUCCESS");
	            }
	            else {
	                snprintf(*prsp_cmd, 40, "+HTTPFOTA:UPDATE FAILED");
	            }
	            break;
	        }
	        default:
	        	xy_free(data);
            	return  (ATERR_NOT_ALLOWED);
        }
        xy_free(data);
    }
#if (AT_CUT!=1)
    else if (g_req_type == AT_CMD_TEST)
    {
        *prsp_cmd = xy_malloc(40);
        snprintf(*prsp_cmd, 40, "+HTTPFOTA:(1-4)");
    }
#endif
    else {
		 return  (ATERR_NOT_ALLOWED);
	}

    return AT_END;
}


