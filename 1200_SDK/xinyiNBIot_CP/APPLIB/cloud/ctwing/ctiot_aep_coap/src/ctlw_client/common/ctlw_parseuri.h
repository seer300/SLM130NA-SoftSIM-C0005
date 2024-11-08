#ifndef _CTLW_PARSE_URI_H
#define _CTLW_PARSE_URI_H

#include <stdbool.h>

typedef struct
{
    char protocol[6];
    char *address;
    int port;
    char *uri;
} CTIOT_URI;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

    bool ctiot_parse_url(char *url, CTIOT_URI *uri);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_CTLW_PARSE_URI_H
