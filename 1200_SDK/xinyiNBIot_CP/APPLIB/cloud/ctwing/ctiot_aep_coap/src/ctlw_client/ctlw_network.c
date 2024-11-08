#include <string.h>
#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_lwm2mclient.h"

#include "ctlw_connection.h"


void *ctlw_lwm2m_connect_server(uint16_t secObjInstID,
                           void *userData)
{
    ctiot_client_data_t *dataP;
    char *uri;
    char *host;
    char *port;
    connection_t *newConnP = NULL;

    dataP = (ctiot_client_data_t *)userData;

    uri = ctlw_get_server_uri(dataP->securityObjP, secObjInstID);
    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"uri:%s\r\n", uri);
    if (uri == NULL)
        return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
    {
        host = uri + strlen("coaps://");
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://")))
    {
        host = uri + strlen("coap://");
    }
    else
    {
        goto exit;
    }
    port = strrchr(host, ':');
    if (port == NULL)
        goto exit;
    // remove brackets
    if (host[0] == '[')
    {
        host++;
        if (*(port - 1) == ']')
        {
            *(port - 1) = 0;
        }
        else
            goto exit;
    }
    // split strings
    *port = 0;
    port++;

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"Opening connection to server at %s:%s\r\n", host, port);

    newConnP = ctlw_connection_create(dataP->connList, dataP->sock, host, port, dataP->addressFamily);

    if (newConnP == NULL)
    {
    	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_SOCKET_CLASS,"Connection creation failed.\n");
    }
    else
    {
    	connection_t *tmpConnP=dataP->connList;
		while(tmpConnP!=NULL)
		{
			if(tmpConnP==newConnP)
			{
				break;
			}
			tmpConnP=tmpConnP->next;
		}
		if(tmpConnP==NULL)//不是老的connP,挂新的connP
        	dataP->connList = newConnP;
    }

exit:
    ctlw_lwm2m_free(uri);
    return (void *)newConnP;
}

void ctlw_lwm2m_close_connection(void *sessionH,
                            void *userData)
{
    ctiot_client_data_t *app_data;
    connection_t *targetP;


    app_data = (ctiot_client_data_t *)userData;

    targetP = (connection_t *)sessionH;


    if (targetP == app_data->connList)
    {
        app_data->connList = targetP->next;
        ctlw_lwm2m_free(targetP);
    }
    else
    {
        connection_t *parentP;


        parentP = app_data->connList;
        while (parentP != NULL && parentP->next != targetP)
        {
            parentP = parentP->next;
        }
        if (parentP != NULL)
        {
            parentP->next = targetP->next;
            ctlw_lwm2m_free(targetP);
        }
    }
}
