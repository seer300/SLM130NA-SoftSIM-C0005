/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef HTTP_CLIENT_WRAPPER_H
#define HTTP_CLIENT_WRAPPER_H

#include "xy_utils.h"

#define HTTP_CLIENT_DEBUG 0

#define http_malloc(ulSize)   		xy_malloc(ulSize)
#define http_free(mem)				xy_free(mem)

#if HTTP_CLIENT_DEBUG
#define http_info(fmt, ...)   do { user_printf("[HTTP][%s %d] "fmt" \r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)
#define http_err(fmt, ...)    do { user_printf("[HTTP][%s %d] "fmt" \r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)
#define http_debug(fmt, ...)  do { user_printf("[HTTP][%s %d] "fmt" \r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)
#else
#define http_info(fmt, ...)
#define http_err(fmt, ...)    do { user_printf("[HTTP][%s %d] "fmt" \r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__); } while (0)
#define http_debug(fmt, ...)
#endif

#ifndef MIN
#define MIN(x,y) (((x)<(y))?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif

int http_tcp_conn_wrapper(httpclient_t *client, const char *host);
int http_tcp_close_wrapper(httpclient_t *client);
int http_tcp_send_wrapper(httpclient_t *client, const char *data, int length);
int http_tcp_recv_wrapper(httpclient_t *client, char *buf, int buflen, int timeout_ms, int *p_read_len);

#if WITH_MBEDTLS_SUPPORT
#if CONFIG_HTTP_SECURE
int http_ssl_conn_wrapper(httpclient_t *client, const char *host);
int http_ssl_close_wrapper(httpclient_t *client);
int http_ssl_send_wrapper(httpclient_t *client, const char *data, size_t length);
int http_ssl_recv_wrapper(httpclient_t *client, char *buf, int buflen, int timeout_ms, int *p_read_len);
#endif
#endif

#endif  /* HTTP_CLIENT_WRAPPER_H */
