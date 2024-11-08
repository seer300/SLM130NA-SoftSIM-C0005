/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Julien Vermillard, Sierra Wireless
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

/*
 *  Resources:
 *
 *          Name         | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Short ID             |  0 |     R      |  Single   |    Yes    | Integer | 1-65535 |       |
 *  Lifetime             |  1 |    R/W     |  Single   |    Yes    | Integer |         |   s   |
 *  Default Min Period   |  2 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Default Max Period   |  3 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Disable              |  4 |     E      |  Single   |    No     |         |         |       |
 *  Disable Timeout      |  5 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Notification Storing |  6 |    R/W     |  Single   |    Yes    | Boolean |         |       |
 *  Binding              |  7 |    R/W     |  Single   |    Yes    | String  |         |       |
 *  Registration Update  |  8 |     E      |  Single   |    Yes    |         |         |       |
 *
 */

#include "ctlw_liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_NV_data.h"
#ifdef PLATFORM_XINYI
#include "ctwing_util.h"
#endif
typedef struct _server_instance_
{
    struct _server_instance_ *next; // matches lwm2m_list_t::next
    uint16_t instanceId;            // matches lwm2m_list_t::id
    uint16_t shortServerId;
    uint32_t lifetime;
    uint32_t defaultMinPeriod;
    uint32_t defaultMaxPeriod;
    uint32_t disableTimeout;
	int64_t lastBootstrapped;
    bool storing;
    char binding[4];
} server_instance_t;


#ifdef PLATFORM_XINYI //适配SDK允许Ctwing平台操作binding_mode,检测binding_mode是否合法
bool xy_ctlw_server_check_binding_valid(const char *binding, size_t size)
{
    if (binding != NULL && (strncmp(binding, "U", size) == 0
            || strncmp(binding, "UQ", size) == 0
            || strncmp(binding, "S", size) == 0
            || strncmp(binding, "SQ", size) == 0
            || strncmp(binding, "US", size) == 0
            || strncmp(binding, "UQS", size) == 0))
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif

//int ctlw_updateRegistration(lwm2m_context_t * contextP,lwm2m_server_t * server);
static uint8_t prv_get_value(lwm2m_data_t *dataP,
                             server_instance_t *targetP)
{
    switch (dataP->id)
    {
    case LWM2M_SERVER_SHORT_ID_ID:
        ctlw_lwm2m_data_encode_int(targetP->shortServerId, dataP);
        return COAP_205_CONTENT;

    case LWM2M_SERVER_LIFETIME_ID:
        ctlw_lwm2m_data_encode_int(targetP->lifetime, dataP);
        return COAP_205_CONTENT;
    case LWM2M_SERVER_STORING_ID:
        ctlw_lwm2m_data_encode_bool(targetP->storing, dataP);
        return COAP_205_CONTENT;
    case LWM2M_SERVER_BINDING_ID:
        ctlw_lwm2m_data_encode_string(targetP->binding, dataP);
        return COAP_205_CONTENT;
    case LWM2M_SERVER_UPDATE_ID:
        return COAP_405_METHOD_NOT_ALLOWED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_server_read(uint16_t instanceId,
                               int *numDataP,
                               lwm2m_data_t **dataArrayP,
                               ctlw_lwm2m_object_t *objectP)
{
    server_instance_t *targetP;
    uint8_t result;
    int i;

    targetP = (server_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;
    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
            LWM2M_SERVER_SHORT_ID_ID,
            LWM2M_SERVER_LIFETIME_ID,
            LWM2M_SERVER_STORING_ID,
            LWM2M_SERVER_BINDING_ID
            };
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

    i = 0;
    do
    {
        result = prv_get_value((*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_server_discover(uint16_t instanceId,
                                   int *numDataP,
                                   lwm2m_data_t **dataArrayP,
                                   ctlw_lwm2m_object_t *objectP)
{
	 server_instance_t * targetP;
    uint8_t result;
    int i;

    result = COAP_205_CONTENT;

    targetP = (server_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        uint16_t resList[] = {
            LWM2M_SERVER_SHORT_ID_ID,
            LWM2M_SERVER_LIFETIME_ID,
            LWM2M_SERVER_STORING_ID,
            LWM2M_SERVER_BINDING_ID,
            LWM2M_SERVER_UPDATE_ID,
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = ctlw_lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
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
            case LWM2M_SERVER_SHORT_ID_ID:
            case LWM2M_SERVER_LIFETIME_ID:
            case LWM2M_SERVER_STORING_ID:
            case LWM2M_SERVER_BINDING_ID:
            case LWM2M_SERVER_UPDATE_ID:
                break;

            default:
                result = COAP_404_NOT_FOUND;
                break;
            }
        }
    }

    return result;
}

static uint8_t prv_server_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t *dataArray,
                                ctlw_lwm2m_object_t *objectP)
{
	ctiot_context_t *pContext = ctiot_get_context();

    server_instance_t *targetP;
    int i;
    uint8_t result;

    targetP = (server_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
    {
        return COAP_404_NOT_FOUND;
    }

    i = 0;
    do
    {
        switch (dataArray[i].id)
        {

        case LWM2M_SERVER_SHORT_ID_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;
            /*
            {
                uint32_t value = targetP->shortServerId;
                result = prv_set_int_value(dataArray + i, &value);
                if (COAP_204_CHANGED == result)
                {
                    if (0 < value && 0xFFFF >= value)
                    {
                        targetP->shortServerId = value;
                    }
                    else
                    {
                        result = COAP_406_NOT_ACCEPTABLE;
                    }
                }
            }
            break;
            */

        case LWM2M_SERVER_LIFETIME_ID:
        {
			result = COAP_401_UNAUTHORIZED;
            break;
        }
        case LWM2M_SERVER_STORING_ID:
        {
			result = COAP_401_UNAUTHORIZED;
            break;
        }
        case LWM2M_SERVER_BINDING_ID:
        {
#ifdef PLATFORM_XINYI
            if ((dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE)
                && xy_ctlw_server_check_binding_valid((char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length))
            {
                if(xy_ctlw_set_binding_by_iot_plat((char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length))
                {
                    memset(targetP->binding, 0, sizeof(targetP->binding));
                    strncpy(targetP->binding, (char *)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                    result = COAP_204_CHANGED;
                }
                else
                {
                    result = COAP_400_BAD_REQUEST;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
#else
            result = COAP_401_UNAUTHORIZED;
#endif         
            break;
        }
        case LWM2M_SERVER_UPDATE_ID:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;
		default:
            return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_server_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t *buffer,
                                  int length,
                                  ctlw_lwm2m_object_t *objectP)

{
    server_instance_t *targetP;
    ctiot_context_t *pContext = ctiot_get_context();
    targetP = (server_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;
    switch (resourceId)
    {
    case LWM2M_SERVER_SHORT_ID_ID:
    case LWM2M_SERVER_LIFETIME_ID:
    case LWM2M_SERVER_STORING_ID:
    case LWM2M_SERVER_BINDING_ID:
        return COAP_405_METHOD_NOT_ALLOWED;
    case LWM2M_SERVER_UPDATE_ID:
    {
        ctiot_publish_sdk_notification("/19/0/0", CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LWEVENT, 1, 0, 0, NULL);
        return COAP_204_CHANGED;
    }
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_server_delete(uint16_t id,
                                 ctlw_lwm2m_object_t *objectP)
{
    objectP->instanceList = ctlw_lwm2m_list_find(objectP->instanceList, id);
    if (NULL == objectP->instanceList)
        return COAP_404_NOT_FOUND;
    return COAP_401_UNAUTHORIZED;
}

static uint8_t prv_server_create(uint16_t instanceId,
                                 int numData,
                                 lwm2m_data_t *dataArray,
                                 ctlw_lwm2m_object_t *objectP)
{
	ctiot_context_t *pContext = ctiot_get_context();

    return COAP_401_UNAUTHORIZED;
}

void ctlw_copy_server_object(ctlw_lwm2m_object_t *objectDest, ctlw_lwm2m_object_t *objectSrc)
{
    memcpy(objectDest, objectSrc, sizeof(ctlw_lwm2m_object_t));
    objectDest->instanceList = NULL;
    objectDest->userData = NULL;
    server_instance_t *instanceSrc = (server_instance_t *)objectSrc->instanceList;
    server_instance_t *previousInstanceDest = NULL;
    while (instanceSrc != NULL)
    {
        server_instance_t *instanceDest = (server_instance_t *)ctlw_lwm2m_malloc(sizeof(server_instance_t));
        if (NULL == instanceDest)
        {
            return;
        }
        memcpy(instanceDest, instanceSrc, sizeof(server_instance_t));
        // not sure it's necessary:
        strcpy(instanceDest->binding, instanceSrc->binding);
        instanceSrc = (server_instance_t *)instanceSrc->next;
        if (previousInstanceDest == NULL)
        {
            objectDest->instanceList = (lwm2m_list_t *)instanceDest;
        }
        else
        {
            previousInstanceDest->next = instanceDest;
        }
        previousInstanceDest = instanceDest;
    }
}

void ctlw_display_server_object(ctlw_lwm2m_object_t *object)
{
#ifdef WITH_LOGS
    ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "  /%u: Server object, instances:\r\n", object->objID);
    server_instance_t *serverInstance = (server_instance_t *)object->instanceList;
    while (serverInstance != NULL)
    {
        ctiot_log_info(LOG_SEND_RECV_MODULE, LOG_OTHER_CLASS, "    /%u/%u: instanceId: %u, shortServerId: %u, lifetime: %u, storing: %s, binding: %s\r\n",
                     object->objID, serverInstance->instanceId,
                     serverInstance->instanceId, serverInstance->shortServerId, serverInstance->lifetime,
                     serverInstance->storing ? "true" : "false", serverInstance->binding);
        serverInstance = (server_instance_t *)serverInstance->next;
    }
#endif
}

ctlw_lwm2m_object_t *ctlw_get_server_object(int serverId,
                                  const char *binding,
                                  int lifetime,
                                  bool storing)
{
    ctlw_lwm2m_object_t *serverObj;

    serverObj = (ctlw_lwm2m_object_t *)ctlw_lwm2m_malloc(sizeof(ctlw_lwm2m_object_t));

    if (NULL != serverObj)
    {
        server_instance_t *serverInstance;

        memset(serverObj, 0, sizeof(ctlw_lwm2m_object_t));

        serverObj->objID = LWM2M_SERVER_OBJECT_ID;

        // Manually create an hardcoded server
        serverInstance = (server_instance_t *)ctlw_lwm2m_malloc(sizeof(server_instance_t));
        if (NULL == serverInstance)
        {
            ctlw_lwm2m_free(serverObj);
            return NULL;
        }

        memset(serverInstance, 0, sizeof(server_instance_t));
        serverInstance->instanceId = 0;
        serverInstance->shortServerId = serverId;
        serverInstance->lifetime = lifetime;
        serverInstance->storing = storing;
        memcpy(serverInstance->binding, binding, strlen(binding) + 1);
		serverInstance->lastBootstrapped = (int64_t)ctlw_lwm2m_gettime();
        serverObj->instanceList = LWM2M_LIST_ADD(serverObj->instanceList, serverInstance);

        serverObj->readFunc = prv_server_read;
        serverObj->discoverFunc = prv_server_discover;
        serverObj->writeFunc = prv_server_write;
        serverObj->createFunc = prv_server_create;
        serverObj->deleteFunc = prv_server_delete;
        serverObj->executeFunc = prv_server_execute;
    }

    return serverObj;
}

void ctlw_clean_server_object(ctlw_lwm2m_object_t *object)
{
    while (object->instanceList != NULL)
    {
        server_instance_t *serverInstance = (server_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        ctlw_lwm2m_free(serverInstance);
    }
}
