
/************************************************************************

            (c) Copyright 2018 by 中国电信上海研究院. All rights reserved.

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ctlw_lwm2m_sdk.h"

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The ctlw_lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ *next; // matches lwm2m_list_t::next
    uint16_t shortID;            // matches lwm2m_list_t::id
    uint8_t test;
    double dec;
} prv_instance_t;

static void ctlw_downMsgList_add(ctiot_down_msg_list *newNode)
{
	ctiot_context_t* pContext=ctiot_get_context();
	if(pContext->downMsgList == NULL)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw_downMsgList_add:downMsgList not initialized\r\n");
		return;
	}
	if(pContext->downMsgList->max_msg_num == pContext->downMsgList->msg_count)
	{
		ctiot_down_msg_list *node = (ctiot_down_msg_list *)ctiot_coap_queue_get(pContext->downMsgList);
		if(node != NULL)
		{
			ctlw_lwm2m_free(node->payload);
			ctlw_lwm2m_free(node);
		}
	}

	if(ctiot_coap_queue_add(pContext->downMsgList, newNode) != CTIOT_NB_SUCCESS)
	{
		ctlw_lwm2m_free(newNode);
		ctlw_lwm2m_free(newNode->payload);
	}
}

static uint8_t prv_read(uint16_t instanceId,
                        int *numDataP,
                        lwm2m_data_t **dataArrayP,
                        ctlw_lwm2m_object_t *objectP)
{
	int i;
	uint8_t result;

	if (instanceId != 0 && instanceId != 1 && instanceId != 2)
	{
		return COAP_404_NOT_FOUND;
	}

	if (*numDataP == 0)
	{
		return COAP_401_UNAUTHORIZED;
	}

	i = 0;
	do
	{
		switch ((*dataArrayP)[i].id)
		{
		case 0:
			{
				if(instanceId == 0)
				{
					ctlw_lwm2m_data_encode_opaque("CTWing\0", strlen("CTWing\0"), &(*dataArrayP)[i]);
					result = COAP_205_CONTENT;
				}
				else if(instanceId == 2)
				{
					result = COAP_404_NOT_FOUND;
				}
				else
				{
					result = COAP_401_UNAUTHORIZED;
				}
				break;
			}

		default:
			result = COAP_404_NOT_FOUND;
		}
		i++;
	} while (i < *numDataP && result == COAP_205_CONTENT);

	return result;

}

static uint8_t prv_discover(uint16_t instanceId,
                            int *numDataP,
                            lwm2m_data_t **dataArrayP,
                            ctlw_lwm2m_object_t *objectP)
{
    uint8_t result;
    int i;

    result = COAP_205_CONTENT;

    if (*numDataP == 0)
    {
        uint16_t resList[] = {0};
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = ctlw_lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    else
    {
        for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case 0:
                break;
            default:
                result = COAP_404_NOT_FOUND;
            }
        }
    }
	return result;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t *dataArray,
                         ctlw_lwm2m_object_t *objectP)
{
	if(instanceId == 0 ||instanceId == 1)
	{
		int i = 0;
	    do
	    {
			switch (dataArray[i].id){
				case 0:
					{
						if(instanceId == 1)
						{
							if(dataArray[i].value.asBuffer.length > MAX_RECV_DATA_LEN)
							{
								return COAP_400_BAD_REQUEST;
							}

							char* payload_buf = ctiot_at_encode(dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
							ctiot_context_t* pContext=ctiot_get_context();
							if(pContext->recvDataMode == RECV_DATA_MODE_0)
							{
								
#ifdef PLATFORM_XINYI
								ctiot_sdk_notification(CTIOT_NOTIFY_RECV_DATA, CTIOT_NOTIFY_RECV_MSG, 0, 0, 0, payload_buf);
								if(pContext->abstractCloudFlag == 1)
								{
									ctlw_proxy_cloud_buf_add(payload_buf);//云抽象AT功能,单独维护下行数据链表
								}
								else
								{
									ctlw_lwm2m_free(payload_buf);
								}
#else
								ctiot_sdk_notification(CTIOT_NOTIFY_RECV_DATA, CTIOT_NOTIFY_RECV_MSG, 0, 0, 0, payload_buf);
								ctlw_lwm2m_free(payload_buf);
#endif
							}
							else if(pContext->recvDataMode == RECV_DATA_MODE_1 ||pContext->recvDataMode == RECV_DATA_MODE_2 )
							{
								ctiot_down_msg_list* node = ctlw_lwm2m_malloc(sizeof(ctiot_down_msg_list));
								memset(node, 0, sizeof(ctiot_down_msg_list));
								//node->msgId = message->mid;
								node->msgStatus = 0;//type=0
								node->payload = (uint8_t *)payload_buf;
								node->recvtime = ctlw_lwm2m_gettime();

								ctlw_downMsgList_add(node);
								if(pContext->recvDataMode == RECV_DATA_MODE_1)
								{
									ctiot_sdk_notification(CTIOT_NOTIFY_RECV_DATA, CTIOT_NOTIFY_NULL_MSG, 0, 0, 0, NULL);
								}
							}
							return COAP_204_CHANGED;
						}
						else
						{
							return COAP_401_UNAUTHORIZED;
						}
					}
				default:
					return COAP_404_NOT_FOUND;
			}
		} while (i < numData);
	}
    return COAP_404_NOT_FOUND;
}

static uint8_t prv_delete(uint16_t id,
                          ctlw_lwm2m_object_t *objectP)
{
    ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_delete COAP_401_UNAUTHORIZED\r\n");
    return COAP_401_UNAUTHORIZED;
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          lwm2m_data_t *dataArray,
                          ctlw_lwm2m_object_t *objectP)
{
   ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_create COAP_401_UNAUTHORIZED\r\n");
    return COAP_401_UNAUTHORIZED;
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t *buffer,
                        int length,
                        ctlw_lwm2m_object_t *objectP)
{
    switch (resourceId)
    {
    case 0:
		ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_exec COAP_405_METHOD_NOT_ALLOWED\r\n");
		return COAP_405_METHOD_NOT_ALLOWED;
    default:
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_exec COAP_404_NOT_FOUND\r\n");
        return COAP_404_NOT_FOUND;
    }
}

ctlw_lwm2m_object_t *ctlw_get_data_report_object(void)
{
    ctlw_lwm2m_object_t *dataReportObject;

    dataReportObject = (ctlw_lwm2m_object_t *)ctlw_lwm2m_malloc(sizeof(ctlw_lwm2m_object_t));

    if (NULL != dataReportObject)
    {
        int i;
        lwm2m_list_t *targetP;

        memset(dataReportObject, 0, sizeof(ctlw_lwm2m_object_t));

        dataReportObject->objID = DATA_REPORT_OBJECT;
		for (i = 0; i < 2; i++)
		{

            targetP = (lwm2m_list_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_list_t));
            if (NULL == targetP)
                goto exit;
            memset(targetP, 0, sizeof(lwm2m_list_t));
            targetP->id = i;
            dataReportObject->instanceList = LWM2M_LIST_ADD(dataReportObject->instanceList, targetP);
        }

        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        dataReportObject->readFunc = prv_read;
        dataReportObject->discoverFunc = prv_discover;
        dataReportObject->writeFunc = prv_write;
        dataReportObject->executeFunc = prv_exec;
        dataReportObject->createFunc = prv_create;
        dataReportObject->deleteFunc = prv_delete;
    }

    return dataReportObject;
exit:
    LWM2M_LIST_FREE(dataReportObject->instanceList);
    ctlw_lwm2m_free(dataReportObject);
    return NULL;
}

void ctlw_free_data_report_object(ctlw_lwm2m_object_t *object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        ctlw_lwm2m_free(object->userData);
        object->userData = NULL;
    }
    ctlw_lwm2m_free(object);
}
