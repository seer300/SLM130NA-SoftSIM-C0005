#pragma once

#include "xy_utils.h"
#include "httpclient.h"

typedef enum {
	ESCAPE_MECHANISM = 0, 	// 字符串输入输出
	HEX_CHARACTER = 1	  	// 16进制输入输出
} http_encode_method_e;

typedef enum {
	SEVER_CERT = 1,   		// 服务器证书
	CLIENT_CERT = 2,  		// 客户端证书
	CLIENT_PK = 3,	  		// 客户端pk
	PRINT_MODE = 8	  		// 更改打印模式
} http_cfg_type_e;

typedef enum {
	HTTPSTAT_CLOSED,
	HTTPSTAT_CONNECTING,
	HTTPSTAT_CONNECTED,
	HTTPSTAT_RECEDATA,
}httpstatus_e;

typedef struct http_context_reference_s {
	uint8_t http_id;         
	uint8_t print_mode;      // 1 in hex mode, 0 is in string mode
	uint8_t quit;
	uint8_t padding;
	httpstatus_e status;     // 用于判断是否命令执行状态
	char *host;              // 记录创建客户端时输入的host
	char *url;               // 用于http请求存储临时url
	httpclient_t* client;     
	httpclient_data_t *client_data;
	osThreadId_t  http_send_thread;
	osThreadId_t  http_recv_thread;
} http_context_reference_t;

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_http_create(char *at_buf, char **rsp_cmd);
int at_http_cfg(char *at_buf, char **rsp_cmd);
int at_http_header(char *at_buf, char **rsp_cmd);
int at_http_content(char *at_buf, char **rsp_cmd);
int at_http_send(char *at_buf, char **rsp_cmd);
int at_http_close(char *at_buf, char **rsp_cmd);
int at_http_fota(char *at_buf, char **prsp_cmd);


