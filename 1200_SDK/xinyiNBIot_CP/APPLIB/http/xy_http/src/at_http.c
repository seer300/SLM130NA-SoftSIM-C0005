#include "at_http.h"
#include "http_config.h"
#include "http_opts.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "at_socket_api.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/	
#define HTTP_CONTEXT_REF_NUM   1
/*由于证书达到数K大小，为了避免大的堆内存申请，保存到FOTA备份区*/
#if HARDCODE_USER_CERT_PK
#define HARDCODE_USER_CERT_PK_BASE (RUNINFO_DEBUG_ADDR-0x3000)
#endif

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
http_context_reference_t *g_http_context_refs[HTTP_CONTEXT_REF_NUM] = {0};


/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/
HTTPC_RESULT http_cert_pk_clear(http_context_reference_t *http_context_ref);

void http_err_report(http_context_reference_t* http_context_ref, HTTPC_RESULT errorCode)
{	
	char *urc_buf = xy_malloc(40);
	
	if (errorCode == HTTP_ECONN) {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP] connection error occurred, ret:%d", errorCode);
		snprintf(urc_buf, 40, "+HTTPERR:%d,%d", http_context_ref->http_id, errorCode);
		
	} 
	else if (errorCode == HTTP_ECLSD) {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP] connection closed by http server, ret:%d", errorCode);
		snprintf(urc_buf, 40, "+HTTPERR:%d,%d", http_context_ref->http_id, errorCode);
	}
	else {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP] other error, ret:%d", errorCode);
		snprintf(urc_buf, 40, "+HTTPERR:%d,%d", http_context_ref->http_id, errorCode);
	}
	
	send_urc_to_ext(urc_buf, strlen(urc_buf));
	xy_free(urc_buf);
}

void http_recv_report(http_context_reference_t* http_context_ref, int flag)
{
	char *urc_buf = NULL;
	char *temp_buf = NULL;
	int header_len = 0, content_len = 0;

	header_len = strlen(http_context_ref->client_data->header_buf);
	content_len = http_context_ref->client_data->content_block_len;

	if (header_len) {
		if (http_context_ref->print_mode == HEX_CHARACTER) {
			temp_buf = xy_malloc2(header_len * 2 + 1);
			if (temp_buf == NULL)
			{
				return;
			}
			bytes2hexstr(http_context_ref->client_data->header_buf, header_len, temp_buf, (header_len * 2 + 1));
			urc_buf = xy_malloc2(40 + header_len * 2);
			if (urc_buf == NULL)
			{
				xy_free(temp_buf);
				return;
			}
			snprintf(urc_buf, 40 + header_len * 2, "\r\n+HTTPNMIH:%d,%d,%d\r\n%s\r\n", http_context_ref->http_id, http_context_ref->client->response_code, header_len, temp_buf);
			send_urc_to_ext_NoCache(urc_buf, strlen(urc_buf));
			xy_free(temp_buf);
			xy_free(urc_buf);
		} else {
			urc_buf = xy_malloc2(40 + header_len);
			if (urc_buf == NULL)
			{
				return;
			}

			snprintf(urc_buf, 40 + header_len, "\r\n+HTTPNMIH:%d,%d,%d\r\n%s\r\n", http_context_ref->http_id, http_context_ref->client->response_code, header_len, http_context_ref->client_data->header_buf);			
			send_urc_to_ext_NoCache(urc_buf, strlen(urc_buf));
			xy_free(urc_buf);
		}
	}

	if (content_len) {
		 if (http_context_ref->print_mode == HEX_CHARACTER) {
			temp_buf = xy_malloc2(content_len * 2 + 1);
			if (temp_buf == NULL)
			{
				return;
			}
			bytes2hexstr(http_context_ref->client_data->response_buf, content_len, temp_buf, (content_len * 2 + 1));
			urc_buf = xy_malloc2(40 + content_len * 2);
			if (urc_buf == NULL)
			{
				xy_free(temp_buf);
				return;
			}
			snprintf(urc_buf, 40 + content_len * 2, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n%s\r\n", http_context_ref->http_id, flag, 
		                              http_context_ref->client_data->response_content_len, http_context_ref->client_data->content_block_len, temp_buf);
			send_urc_to_ext_NoCache(urc_buf, strlen(urc_buf));
			xy_free(temp_buf);
			xy_free(urc_buf);
		} else {
			int size = 0;
		
			urc_buf = xy_malloc2(40 + content_len);
			if (urc_buf == NULL)
			{
				return;
			}
			snprintf(urc_buf, 40 + content_len, "\r\n+HTTPNMIC:%d,%d,%d,%d\r\n", http_context_ref->http_id, flag, 
		                              http_context_ref->client_data->response_content_len,http_context_ref->client_data->content_block_len); 

			size += strlen(urc_buf);
            memcpy(urc_buf + size, http_context_ref->client_data->response_buf, content_len);
            size += content_len;
            memcpy(urc_buf + size, "\r\n", 2);
            size += 2;			
			send_urc_to_ext_NoCache(urc_buf, size);
			xy_free(urc_buf);
		}	
	}
}

void http_send_report(http_context_reference_t* http_context_ref, HTTPC_RESULT errorCode)
{	
	(void)http_context_ref;
	
	char *urc_buf = xy_malloc(24);
	
	if (errorCode == HTTP_SUCCESS) {
		snprintf(urc_buf, 24, "+REQUESTSUCCESS");
	} 
	else {
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP] http send report, ret:%d", errorCode);
		snprintf(urc_buf, 24, "+BADREQUEST");
	}
	
	send_urc_to_ext(urc_buf, strlen(urc_buf));
	xy_free(urc_buf);
}

HTTPC_RESULT get_free_http_context_ref(int *http_id)
{
	int i;
	HTTPC_RESULT ret = HTTP_SUCCESS;
	http_context_reference_t* http_context_ref = NULL;

	for (i = 0; i < HTTP_CONTEXT_REF_NUM; i++) {
		if (g_http_context_refs[i] == NULL) {
			break;
		}
	}
	if (i == HTTP_CONTEXT_REF_NUM) {
		return HTTP_EUNKOWN;
	}

	httpclient_t* client = xy_malloc(sizeof(httpclient_t));
	memset(client, 0, sizeof(httpclient_t));
	client->socket = -1;

	httpclient_data_t* client_data = xy_malloc(sizeof(httpclient_data_t));
	memset(client_data, 0, sizeof(httpclient_data_t));
	client_data->header_buf = xy_malloc2 (HTTPCLIENT_HEADER_SIZE);
	if (client_data->header_buf == NULL)
	{
		xy_free(client);
		xy_free(client_data);
		return HTTP_ENOBUFS;
	}
	client_data->response_buf = xy_malloc2 (HTTPCLIENT_RESP_SIZE);
	if (client_data->response_buf == NULL)
	{
		xy_free(client);
		xy_free(client_data->header_buf);
		xy_free(client_data);
		return HTTP_ENOBUFS;
	}
	client_data->header_buf_len = HTTPCLIENT_HEADER_SIZE;
    client_data->response_buf_len = HTTPCLIENT_RESP_SIZE;

    http_context_ref = xy_malloc(sizeof(http_context_reference_t));
	memset(http_context_ref, 0, sizeof(http_context_reference_t));
	http_context_ref->client = client;
	http_context_ref->client_data = client_data;
	http_context_ref->http_id = i;
	g_http_context_refs[i] = http_context_ref;

	*http_id = i;
	
	return HTTP_SUCCESS;
}

HTTPC_RESULT http_context_clear(http_context_reference_t *http_context_ref)
{
	if (http_context_ref == NULL) {
		return HTTP_EARG;
	}
	
	if (http_context_ref->client != NULL) {
		http_cert_pk_clear(http_context_ref);
		if (http_context_ref->client->header != NULL) {
			xy_free(http_context_ref->client->header);
			http_context_ref->client->header = NULL;
		}
		if (http_context_ref->client->auth_user != NULL) {
			xy_free(http_context_ref->client->auth_user);
			http_context_ref->client->auth_user = NULL;
		}
		if (http_context_ref->client->auth_password != NULL) {
			xy_free(http_context_ref->client->auth_password);
			http_context_ref->client->auth_password = NULL;
		}
		xy_free(http_context_ref->client);
		http_context_ref->client = NULL;
	}
	if (http_context_ref->client_data != NULL) {
		if (http_context_ref->client_data->header_buf != NULL) {
			xy_free(http_context_ref->client_data->header_buf);
			http_context_ref->client_data->header_buf = NULL;
		}
		if (http_context_ref->client_data->response_buf != NULL) {
			xy_free(http_context_ref->client_data->response_buf);
			http_context_ref->client_data->response_buf = NULL;
		}
		if (http_context_ref->client_data->post_buf) {
			xy_free(http_context_ref->client_data->post_buf);
			http_context_ref->client_data->post_buf = NULL;
		}
		if (http_context_ref->client_data->redirect_url) {
			xy_free(http_context_ref->client_data->redirect_url);
			http_context_ref->client_data->redirect_url = NULL;
			http_context_ref->client_data->is_redirected = 0;
		}
		xy_free(http_context_ref->client_data);
		http_context_ref->client_data = NULL;
	}

	if (http_context_ref->host != NULL) {
		xy_free(http_context_ref->host);
		http_context_ref->host = NULL;
	}
	if (http_context_ref->url != NULL) {
		xy_free(http_context_ref->url);
		http_context_ref->url = NULL;
	}
	xy_free(http_context_ref);
	
	return  HTTP_SUCCESS;
}


HTTPC_RESULT http_header_content_clear(http_context_reference_t *http_context_ref)
{
	if (http_context_ref == NULL) {
		return HTTP_EARG;
	}
	if (http_context_ref->client != NULL) {
		if (http_context_ref->client->header) {
			xy_free(http_context_ref->client->header);
			http_context_ref->client->header = NULL;
		}
	}

	if (http_context_ref->client_data != NULL) {
		if (http_context_ref->client_data->post_buf) {
			xy_free(http_context_ref->client_data->post_buf);
			http_context_ref->client_data->post_buf = NULL;
			http_context_ref->client_data->post_buf_len = 0;
		}
	}

	return  HTTP_SUCCESS;
}

HTTPC_RESULT http_cert_pk_clear(http_context_reference_t *http_context_ref)
{
	if (http_context_ref == NULL) {
		return HTTP_EARG;
	}
#if !HARDCODE_USER_CERT_PK
#if WITH_MBEDTLS_SUPPORT
	if (http_context_ref->client != NULL) {
		if (http_context_ref->client->server_cert) {
			xy_free(http_context_ref->client->server_cert);
			http_context_ref->client->server_cert = NULL;
		}
		if (http_context_ref->client->client_cert) {
			xy_free(http_context_ref->client->client_cert);
			http_context_ref->client->client_cert = NULL;
		}
		if (http_context_ref->client->client_pk) {
			xy_free(http_context_ref->client->client_pk);
			http_context_ref->client->client_pk = NULL;
		}
	}
#endif
#endif
	return  HTTP_SUCCESS;
}

HTTPC_RESULT http_client_cfg(int http_id, int type, char *data, int encode_method)
{
	char *temp_data = NULL;
	int len = -1;
	http_context_reference_t *http_context_ref = NULL;
	
	http_context_ref = g_http_context_refs[http_id];

	if (http_context_ref == NULL)
		goto failed;

	if (type != SEVER_CERT && type != CLIENT_CERT && type != CLIENT_PK && type != PRINT_MODE)
		goto failed;

	if ((data != NULL) && (strcmp(data, "") != 0)) {
		/* 编码模式 0， 直接取值*/
		if (encode_method == 0) {
			len = strlen(data);
			temp_data = xy_malloc2(len + 1);
			if (temp_data == NULL) {
				goto failed;
			}
			strcpy(temp_data, data);
		}
		/* 编码模式 1， 16进制数转化为ascii码*/
		else if(encode_method == 1) {
			len = strlen(data) / 2;
			temp_data = xy_malloc2(len + 1);
			if (temp_data == NULL) {
				goto failed;
			}
			if (hexstr2bytes(data, len * 2, temp_data, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		/*根据不同的type传参，决定实例的不同配置请求*/
		/* 按不同的请求，先删除上次保存的实例，然后将data传参填入实例结构体中。 */
		switch (type) {
#if WITH_MBEDTLS_SUPPORT
			case SEVER_CERT: // 服务器cert证书
			{
#if !HARDCODE_USER_CERT_PK
				if (http_context_ref->client->server_cert != NULL) {
					xy_free(http_context_ref->client->server_cert);
				}

				http_context_ref->client->server_cert = temp_data;
				http_context_ref->client->server_cert_len = strlen(temp_data) + 1;
#else
				len = strlen(temp_data) + 1;
				xy_Flash_Write(HARDCODE_USER_CERT_PK_BASE, temp_data, len);
				http_context_ref->client->server_cert = HARDCODE_USER_CERT_PK_BASE;
				http_context_ref->client->server_cert_len = len;
				xy_free(temp_data);
#endif
				break;
			}
			case CLIENT_CERT: // 客户端cert证书
			{
#if !HARDCODE_USER_CERT_PK
				if (http_context_ref->client->client_cert != NULL) {
					xy_free(http_context_ref->client->client_cert);
				}
				http_context_ref->client->client_cert = temp_data;
				http_context_ref->client->client_cert_len = strlen(temp_data) + 1;
#else
				len = strlen(temp_data) + 1;
				xy_Flash_Write(HARDCODE_USER_CERT_PK_BASE+0x1000, temp_data, len);
				http_context_ref->client->client_cert = HARDCODE_USER_CERT_PK_BASE+0x1000;
				http_context_ref->client->client_cert_len = len;
				xy_free(temp_data);
#endif
				break;
			}
			case CLIENT_PK: // 客户端 private key
			{
#if !HARDCODE_USER_CERT_PK
				if (http_context_ref->client->client_pk != NULL) {
					xy_free(http_context_ref->client->client_pk);
				}
				http_context_ref->client->client_pk = temp_data;
				http_context_ref->client->client_pk_len = strlen(temp_data) + 1;
#else
				len = strlen(temp_data) + 1;
				xy_Flash_Write(HARDCODE_USER_CERT_PK_BASE+0x2000, temp_data, len);
				http_context_ref->client->client_pk = HARDCODE_USER_CERT_PK_BASE+0x2000;
				http_context_ref->client->client_pk_len = len;
				xy_free(temp_data);
#endif
				break;
			}
#endif
			case PRINT_MODE:// 打印模式配置： 1 代表用16进制模式打印收到的字符， 0 代表字符串模式
			{
				http_context_ref->print_mode = (int)strtol(temp_data,NULL,10);		
				xy_free(temp_data);
				break;
			}
			default:
				goto failed;
		}
	}
	return HTTP_SUCCESS;

failed:
	if (temp_data != NULL) {
		xy_free(temp_data);
	}
	return HTTP_EUNKOWN;
}

HTTPC_RESULT http_header_cfg(int http_id, char *header, int encode_method)
{
	http_context_reference_t *http_context_ref = NULL;
	char *temp_header_content = NULL;
	char *temp = NULL;
	int len;

	http_context_ref = g_http_context_refs[http_id];

	if (http_context_ref == NULL)
		goto failed;

	/* 若header参数非空，根据encode_method编码方式解析读取到的字符串*/
	if ((header != NULL) && (strcmp(header, "") != 0)) {
		if (encode_method == 0) {
			len = strlen(header);
			temp_header_content = xy_malloc2(len + 1);
			if (temp_header_content == NULL) {
				goto failed;
			}
			strcpy(temp_header_content, header);
		}
		else if(encode_method == 1) {
			len = strlen(header) / 2;
			temp_header_content = xy_malloc2(len + 1);
			if (temp_header_content == NULL) {
				goto failed;
			}
			memset(temp_header_content, 0, len + 1);

			if (hexstr2bytes(header, len * 2, temp_header_content, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		/* 当输入的 header 字段非空时，新输入的 header 将添加在上一次 header 输入之后。 */
		if (http_context_ref->client->header == NULL) {
			http_context_ref->client->header = temp_header_content;
		} else {
			temp = xy_malloc2(strlen(http_context_ref->client->header) + strlen(temp_header_content) + 1);
			if (temp == NULL)
			{
				goto failed;
			}
			strcpy(temp, http_context_ref->client->header);
			xy_free(http_context_ref->client->header);
			strcpy(temp + strlen(http_context_ref->client->header), temp_header_content);
			http_context_ref->client->header = temp;
			xy_free(temp_header_content);
		}
	}
	/* 当输入的 header 字段为空时，将清空此前已输入 header */
	else {
		if(http_context_ref->client->header != NULL)
			xy_free(http_context_ref->client->header);
		http_context_ref->client->header = NULL;
	}
	
	return HTTP_SUCCESS;

failed:
	if (temp_header_content != NULL) {
		xy_free(temp_header_content);
	}
	return HTTP_EUNKOWN;
}

HTTPC_RESULT http_content_cfg(int http_id, char *content, int encode_method)
{
	http_context_reference_t *http_context_ref = NULL;
	char *temp_content = NULL;
	char *temp = NULL;
	int len;

	http_context_ref = g_http_context_refs[http_id];

	if (http_context_ref == NULL)
		goto failed;
	if(is_Uplink_FlowCtl_Open())
		goto failed;

	if ((content != NULL) && (strcmp(content, "") != 0)) {
		if (encode_method == 0) {
			len = strlen(content);
			temp_content = xy_malloc2(len + 1);
			if (temp_content == NULL) {
				goto failed;
			}
			strcpy(temp_content, content);
		}
		else if(encode_method == 1) {
			len = strlen(content) / 2;
			temp_content = xy_malloc2(len + 1);
			if (temp_content == NULL) {
				goto failed;
			}
			memset(temp_content, 0, len + 1);

			if (hexstr2bytes(content, len * 2, temp_content, len) < 0) {
				goto failed;
			}
		} else {
			goto failed;
		}

		if (http_context_ref->client_data->post_buf == NULL) {
			http_context_ref->client_data->post_buf = temp_content;
			http_context_ref->client_data->post_buf_len = strlen(temp_content);
		} else {
			temp = xy_malloc2(strlen(http_context_ref->client_data->post_buf) + strlen(temp_content) + 1);
			if (temp == NULL)
			{
				goto failed;
			}
			strcpy(temp, http_context_ref->client_data->post_buf);
			xy_free(http_context_ref->client_data->post_buf);
			strcpy(temp + strlen(http_context_ref->client_data->post_buf), temp_content);
			http_context_ref->client_data->post_buf = temp;
			http_context_ref->client_data->post_buf_len = strlen(temp);
			xy_free(temp_content);
		}
	}
	else {
		if(http_context_ref->client_data->post_buf != NULL)
			xy_free(http_context_ref->client_data->post_buf);
		http_context_ref->client_data->post_buf = NULL;
	}
	return HTTP_SUCCESS;

failed:
	if (temp_content != NULL) {
		xy_free(temp_content);
	}
	return HTTP_EUNKOWN;
}

HTTPC_RESULT http_url_is_vaild(http_context_reference_t *http_context_ref, char *url)
{
    char *scheme_ptr = (char *) url;
	char defaultport[4] = {0};
    char *host_ptr = NULL;
    size_t host_len = 0;
    char *port_ptr;
    char *path_ptr;
	char *temp_url;
	int temp_url_len;

    if (http_context_ref == NULL || url == NULL) {
        return HTTP_EPARSE;
    }
	
	if (!strncmp(url, "http", strlen("http"))) {
		strcpy(defaultport, "80");
	} 
	else if (!strncmp(url, "https", strlen("https"))) {
		strcpy(defaultport, "443");
	}
	else {
		return HTTP_EPARSE; /* URL is invalid */
	}
	
    host_ptr = (char *) strstr(url, "://");
    if (host_ptr == NULL) {
        return HTTP_EPARSE; /* URL is invalid */
    }

    host_ptr += 3;

    port_ptr = strchr(host_ptr, ':');
    if ( port_ptr != NULL) {
		uint16_t port;
        host_len = port_ptr - host_ptr;
        port_ptr++;
	    if (sscanf(port_ptr, "%hu", &port) != 1) {
            return HTTP_EPARSE;
        }
    }
	
    path_ptr = strchr(host_ptr, '/');
    if(path_ptr != NULL) {
        temp_url_len = path_ptr - scheme_ptr;
    } 
	else {
		temp_url_len = strlen(scheme_ptr);
	}

	temp_url = xy_malloc(temp_url_len + 1);
	memcpy(temp_url, scheme_ptr, temp_url_len);
	temp_url[temp_url_len] = '\0';
	http_context_ref->host = temp_url;
	
    return HTTP_SUCCESS;
}

HTTPC_RESULT http_url_concat(http_context_reference_t *http_context_ref, char *path)
{
	char *url = NULL;
	int urllen = -1, pathlen = -1;

	if (http_context_ref == NULL || path == NULL) {
		 return HTTP_EPARSE;
	}

	if (http_context_ref->url != NULL) {
		xy_free(http_context_ref->url);
	}
	urllen = strlen(http_context_ref->host);
	pathlen = strlen(path);
	url = xy_malloc(urllen + pathlen + 1);
	memset(url, 0, urllen + pathlen + 1);
	memcpy(url, http_context_ref->host, urllen);
	strcat(url, path);
	http_context_ref->url = url;

	return HTTP_SUCCESS;
}

void http_recv_thread(void* argument)
{
	int ret = HTTP_SUCCESS;
	http_context_reference_t *http_context_ref = NULL;

	http_context_ref = (http_context_reference_t*)argument;

	while(true)
	{	
		if (http_context_ref->quit == 1)
		{
			goto exit;
		}
		
		memset(http_context_ref->client_data->header_buf, 0, http_context_ref->client_data->header_buf_len);
		memset(http_context_ref->client_data->response_buf, 0, http_context_ref->client_data->response_buf_len);
		
		ret = httpclient_recv(http_context_ref->client, http_context_ref->client_data);
		xy_printf(0, XYAPP, WARN_LOG, "[HTTP] content_block_len:%d, response_code:%d, ret:%d", http_context_ref->client_data->content_block_len, http_context_ref->client->response_code, ret);

		if (ret == HTTP_SUCCESS) {
			http_recv_report(http_context_ref, 0);
		} 
		else if (ret == HTTP_EAGAIN) {
			http_recv_report(http_context_ref, 1);
		} 
		else if (ret == HTTP_ETIMEOUT){
			continue; //we assume select timeouted here
		}
		else { 
			http_err_report(http_context_ref, ret);
			break;
		}
	}

exit:
	if (http_context_ref->status != HTTPSTAT_CLOSED) {
		httpclient_clse(http_context_ref->client);
		http_context_ref->status = HTTPSTAT_CLOSED;
	}
	 
	http_context_ref->http_recv_thread = NULL;
	osThreadExit();
}

void http_send_thread(void* argument)
{
	int ret = HTTP_SUCCESS;
	http_context_reference_t* http_context_ref = NULL;

	http_context_ref = (http_context_reference_t*)argument;
			
	if (http_context_ref->status == HTTPSTAT_CLOSED) {
		http_context_ref->status = HTTPSTAT_CONNECTING;
		ret = httpclient_conn(http_context_ref->client, http_context_ref->url);
	    http_cert_pk_clear(http_context_ref); //由于server_cert, client_cert与client_pk是进行动态申请内存，且较大，所以需要连接结束以后，及时释放相关内存；
		if (ret != HTTP_SUCCESS) {
			http_send_report(http_context_ref, ret);
			call_socket_delay(SOCK_CANCEL_CHECK,0,0,0);
			httpclient_clse(http_context_ref->client);
			http_context_ref->status = HTTPSTAT_CLOSED;
			goto exit;
		}
		http_context_ref->status = HTTPSTAT_CONNECTED;
	}
	
	ret = httpclient_send(http_context_ref->client, http_context_ref->url, http_context_ref->client->method, http_context_ref->client_data);
	if (ret != HTTP_SUCCESS) {
		http_send_report(http_context_ref, ret);
		call_socket_delay(SOCK_CANCEL_CHECK,0,0,0);
		httpclient_clse(http_context_ref->client);
		http_context_ref->status = HTTPSTAT_CLOSED;
		goto exit;
	}

	http_send_report(http_context_ref, ret);
	call_socket_delay(SOCK_CANCEL_CHECK,0,0,0);

	http_context_ref->status = HTTPSTAT_RECEDATA;
	
	if (http_context_ref->http_recv_thread == NULL) {
		osThreadAttr_t task_attr = {0};
		task_attr.name = "http_recv_thread";
		task_attr.priority = osPriorityNormal1;
		task_attr.stack_size = osStackShared;
		http_context_ref->http_recv_thread = osThreadNew((osThreadFunc_t)(http_recv_thread), http_context_ref, &task_attr);
	}
	
exit:
	http_header_content_clear(http_context_ref);
	http_context_ref->http_send_thread = NULL;
	osThreadExit();
}


//AT+HTTPCREATE=<host>[,<username>,<password>]
int at_http_create(char *at_buf, char **rsp_cmd)
{
	int ret = HTTP_SUCCESS;
	int http_id = -1;
	int parsed_num = -1;
	int auth_user_len  = 0;
	int auth_passwd_len = 0;
	char *url = xy_malloc(strlen(at_buf));
	char *auth_user = xy_malloc(strlen(at_buf));
	char *auth_passwd = xy_malloc(strlen(at_buf));
	http_context_reference_t *http_context_ref = NULL;

	memset(auth_user, 0, strlen(at_buf));
	memset(auth_passwd, 0, strlen(at_buf));

	*rsp_cmd = xy_malloc(24);
			
	if (at_parse_param_escape("%s,%s,%s", at_buf, &parsed_num, url, auth_user, auth_passwd) != AT_OK) {
		goto create_error;
	}

	if (parsed_num <= 0 || parsed_num > 3) {
		goto create_error;
	}

	auth_user_len = strlen(auth_user);
	auth_passwd_len = strlen(auth_passwd);
	if ((auth_user_len != 0 && auth_passwd_len == 0) || (auth_user_len == 0 && auth_passwd_len != 0)) {
		goto create_error;
	}
		
	if (get_free_http_context_ref(&http_id) != HTTP_SUCCESS) {
		goto create_error;
	}

	http_context_ref = g_http_context_refs[http_id];

	if (http_url_is_vaild(http_context_ref, url) != HTTP_SUCCESS) {
		goto create_error;
	}

	if (auth_user_len != 0 && auth_passwd_len != 0) {
		if (httpclient_basic_auth(http_context_ref->client, auth_user, auth_passwd) != HTTP_SUCCESS) {
			goto create_error;
		}
	}
	else {
		xy_free(auth_user);
		xy_free(auth_passwd);
	}
	
	snprintf(*rsp_cmd, 24, "+HTTPCREATE:%d", http_id);
	
	xy_free(url);
	return AT_END;

create_error:
	snprintf(*rsp_cmd, 24, "\r\nERROR\r\n");
	xy_free(url);
	xy_free(auth_user);
	xy_free(auth_passwd);
	http_context_clear(http_context_ref);
	g_http_context_refs[http_id] = NULL;
	return AT_END;
}

//AT+HTTPCFG=<id>,<type>,<value>[more,[encode>]]
int at_http_cfg(char *at_buf, char **rsp_cmd)
{
	int ret = HTTP_SUCCESS;
	int parsed_num = -1;
	int type = -1;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;
	char *data = xy_malloc(strlen(at_buf)); //可能含转义字符

	if (at_parse_param_escape("%d(0-),%d,%s,%d", at_buf, &parsed_num, &http_id, &type, data, &encode_method) != AT_OK) {
		goto cfg_error;
	}

	if (parsed_num < 2 || parsed_num > 4 || http_id >= HTTP_CONTEXT_REF_NUM) {
		goto cfg_error;
	}

#if WITH_MBEDTLS_SUPPORT
	if (type != SEVER_CERT && type != CLIENT_CERT && type != CLIENT_PK && type != PRINT_MODE)
		goto cfg_error;
#else
	if (type != PRINT_MODE)
		goto cfg_error;
#endif

	if (parsed_num == 4) {
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER)
		{
			goto cfg_error;
		}
	}

	ret = http_client_cfg(http_id, type, data, encode_method);
	if (ret != HTTP_SUCCESS){
		goto cfg_error;
	}

	xy_free(data);
	return AT_END;

cfg_error:
	*rsp_cmd = xy_malloc(12);
	snprintf(*rsp_cmd, 12, "\r\nERROR\r\n");
	xy_free(data);
	return AT_END;
}

//AT+HTTPHEADER=<id>[,<header>[,encode_method>]]
int at_http_header(char *at_buf, char **rsp_cmd)
{
	int ret = HTTP_SUCCESS;
	int parsed_num = -1;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;
	char *header = xy_malloc(strlen(at_buf));  //可能含转义字符
	http_context_reference_t *http_context_ref = NULL;

	memset(header, 0, strlen(at_buf));
	
	if (at_parse_param_escape("%d(0-),%s,%d", at_buf, &parsed_num, &http_id, header, &encode_method) != AT_OK) {					
		goto header_error;
	}

	if (parsed_num < 1 || parsed_num > 3 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto header_error;
	}

	if (parsed_num == 3) {
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER) {
			goto header_error;
		}
	}

	http_context_ref = g_http_context_refs[http_id];
	if (http_context_ref == NULL){
		goto header_error;
	}
	
	if (parsed_num == 1) {
		if (http_context_ref->client->header != NULL) {
			int header_len = 0;

			header_len = strlen(http_context_ref->client->header);
			*rsp_cmd = xy_malloc(40 + header_len);
			snprintf(*rsp_cmd, 40 + header_len, "\r\n+HTTPHEADER:%d,%d\r\n%s\r\nOK\r\n", http_id, header_len,
				http_context_ref->client->header);
		}
	} else {
		ret = http_header_cfg(http_id, header, encode_method);
		if(ret != HTTP_SUCCESS){
			goto header_error;
		}
	}
	
	xy_free(header);
	return AT_END;
	
header_error:
	xy_free(header);
	*rsp_cmd = xy_malloc(12);
	snprintf(*rsp_cmd, 12, "\r\nERROR\r\n");
	return AT_END;
}

//AT+HTTPCONTENT=<id>[,<content>[,encode_method>]]
int at_http_content(char *at_buf, char **rsp_cmd)
{
	int ret = HTTP_SUCCESS;
	int parsed_num = -1;
	int http_id = -1;
	int encode_method = ESCAPE_MECHANISM;
	char *content = xy_malloc(strlen(at_buf));  //可能含转义字符
	http_context_reference_t *http_context_ref = NULL;

	memset(content, 0, strlen(at_buf));
	
	if (at_parse_param_escape("%d(0-),%s,%d", at_buf, &parsed_num, &http_id, content, &encode_method) != AT_OK) {
		goto content_error;
	}

	if (parsed_num < 1 || parsed_num > 3 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto content_error;
	}

	if (parsed_num == 3)
	{
		if(encode_method != ESCAPE_MECHANISM && encode_method != HEX_CHARACTER)
		{
			goto content_error;
		}
	}

	http_context_ref = g_http_context_refs[http_id];
	if (http_context_ref == NULL){
		goto content_error;
	}

	if (parsed_num == 1){
		if (http_context_ref->client_data->post_buf != NULL) {
			int content_len = 0;
			
			content_len = strlen(http_context_ref->client_data->post_buf);
		    *rsp_cmd = xy_malloc(40 + content_len);
			snprintf(*rsp_cmd, 40 + content_len,  "\r\n+HTTPCONTENT:%d,%d\r\n%s\r\n\r\nOK\r\n", http_id, content_len,
					http_context_ref->client_data->post_buf);
		}
	} else {
		ret = http_content_cfg(http_id, content, encode_method);
		if(ret != HTTP_SUCCESS){
			goto content_error;
		}
	}				

	xy_free(content);
	return AT_END;

content_error:
	xy_free(content);
	*rsp_cmd = xy_malloc(12);
	snprintf(*rsp_cmd, 12, "\r\nERROR\r\n");
	return AT_END;
}

//AT+HTTPSEND=<id>,<method>,<path>
int at_http_send(char *at_buf, char **rsp_cmd)
{
	int ret = HTTP_SUCCESS;
	int parsed_num = -1;
	int http_id = -1;
	int method = -1;
	char *path= xy_malloc(strlen(at_buf));
	http_context_reference_t *http_context_ref = NULL;
	osThreadAttr_t task_attr = {0};

	if (at_parse_param_escape("%d(0-),%d,%s",at_buf, &parsed_num, &http_id, &method, path) != AT_OK) {
		goto send_error;
	}

	if (parsed_num != 3 || http_id >= HTTP_CONTEXT_REF_NUM) {
		goto send_error;
	}

	if (method != HTTP_GET && method != HTTP_POST && method != HTTP_PUT && method != HTTP_DELETE && method != HTTP_HEAD){
		goto send_error;
	}

	http_context_ref = g_http_context_refs[http_id];
	if (http_context_ref == NULL) {
		goto send_error;
	} 

	if (http_context_ref->http_send_thread != NULL) {
		goto send_error;
	}
	
	http_context_ref->client->method = method;
	http_url_concat(http_context_ref, path);
	
	call_socket_delay(SOCK_HTTP_CTL_TIMEOUT, http_id,SOCKT_TIMEOUT_10S,0);

	task_attr.name = "http_send_thread";
	task_attr.priority = osPriorityNormal1;
	task_attr.stack_size = osStackShared;
	http_context_ref->http_send_thread = osThreadNew((osThreadFunc_t)(http_send_thread), http_context_ref, &task_attr);
	
	xy_free(path);
	return AT_END;

send_error:
	*rsp_cmd = xy_malloc(12);
	snprintf(*rsp_cmd, 12, "\r\nERROR\r\n");
	xy_free(path);
	return AT_END;
}

extern void http_ctl_timeout_callback(int socket_id)
{
	http_context_reference_t *http_context_ref = NULL;
	
	http_context_ref = g_http_context_refs[socket_id];
	if (http_context_ref == NULL) {
		return;
	} 

	if (http_context_ref->client == NULL) {
		return;
	}

	close(http_context_ref->client->socket);

}

//AT+HTTPCLOSE=<id>
int at_http_close(char *at_buf, char **rsp_cmd)
{
	int http_id = -1;
	int parsed_num = -1;
	http_context_reference_t *http_context_ref = NULL;
	
	if (at_parse_param_escape("%d(0-)",at_buf, &parsed_num, &http_id) != AT_OK) {
		goto close_error;
	}

	if(parsed_num != 1 || http_id >= HTTP_CONTEXT_REF_NUM){
		goto close_error;
	}

	http_context_ref = g_http_context_refs[http_id];
	if (http_context_ref == NULL) {
		goto close_error;
	}

	if (http_context_ref->status == HTTPSTAT_CONNECTING) {
		goto close_error; //tcp 连接过程中不允许关闭，否则会导致lwip异常;
	}
	
	if (http_context_ref->status == HTTPSTAT_CONNECTED || http_context_ref->status == HTTPSTAT_RECEDATA) {
		/* 置标志位后,在 http_recv_thread 中完成关闭socket操作 */
		http_context_ref->quit = 1;
		xy_printf(0,XYAPP, WARN_LOG, "[at_http_close]http id(%d) quit flag set 1!!!", http_id);
	}	
	
	while (http_context_ref->http_send_thread != NULL || http_context_ref->http_recv_thread != NULL)
	{
		osDelay(100);
	}
	http_context_clear(http_context_ref);
	g_http_context_refs[http_id] = NULL;

	return AT_END;

close_error:
	*rsp_cmd = xy_malloc(12);
	snprintf(*rsp_cmd, 12, "\r\nERROR\r\n");
	return AT_END;
}

