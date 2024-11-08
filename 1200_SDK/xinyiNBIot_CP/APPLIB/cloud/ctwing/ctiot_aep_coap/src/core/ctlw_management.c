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
 *    domedambrosio - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/
/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/



#include "ctlw_config.h"

#include "ctlw_internals.h"
#include <stdio.h>

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_sdk_internals.h"



#ifdef LWM2M_CLIENT_MODE
static int prv_readAttributes(ctlw_multi_option_t * query,
                              lwm2m_attributes_t * attrP)
{
    int64_t intValue;
    double floatValue;

    memset(attrP, 0, sizeof(lwm2m_attributes_t));

    while (query != NULL)
    {
        if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_MIN_PERIOD_STR, ATTR_MIN_PERIOD_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MIN_PERIOD)) return -1;
            if (query->len == ATTR_MIN_PERIOD_LEN) return -1;

            if (1 != ctlw_utils_textToInt(query->data + ATTR_MIN_PERIOD_LEN, query->len - ATTR_MIN_PERIOD_LEN, &intValue)) return -1;
            if (intValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_MIN_PERIOD;
            attrP->minPeriod = intValue;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_MIN_PERIOD_STR, ATTR_MIN_PERIOD_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MIN_PERIOD)) return -1;
            if (query->len != ATTR_MIN_PERIOD_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_MIN_PERIOD;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_MAX_PERIOD_STR, ATTR_MAX_PERIOD_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MAX_PERIOD)) return -1;
            if (query->len == ATTR_MAX_PERIOD_LEN) return -1;

            if (1 != ctlw_utils_textToInt(query->data + ATTR_MAX_PERIOD_LEN, query->len - ATTR_MAX_PERIOD_LEN, &intValue)) return -1;
            if (intValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_MAX_PERIOD;
            attrP->maxPeriod = intValue;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_MAX_PERIOD_STR, ATTR_MAX_PERIOD_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_MAX_PERIOD)) return -1;
            if (query->len != ATTR_MAX_PERIOD_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_MAX_PERIOD;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_GREATER_THAN_STR, ATTR_GREATER_THAN_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_GREATER_THAN)) return -1;
            if (query->len == ATTR_GREATER_THAN_LEN) return -1;

            if (1 != ctlw_utils_textToFloat(query->data + ATTR_GREATER_THAN_LEN, query->len - ATTR_GREATER_THAN_LEN, &floatValue)) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_GREATER_THAN;
            attrP->greaterThan = floatValue;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_GREATER_THAN_STR, ATTR_GREATER_THAN_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_GREATER_THAN)) return -1;
            if (query->len != ATTR_GREATER_THAN_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_GREATER_THAN;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_LESS_THAN_STR, ATTR_LESS_THAN_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_LESS_THAN)) return -1;
            if (query->len == ATTR_LESS_THAN_LEN) return -1;

            if (1 != ctlw_utils_textToFloat(query->data + ATTR_LESS_THAN_LEN, query->len - ATTR_LESS_THAN_LEN, &floatValue)) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_LESS_THAN;
            attrP->lessThan = floatValue;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_LESS_THAN_STR, ATTR_LESS_THAN_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_LESS_THAN)) return -1;
            if (query->len != ATTR_LESS_THAN_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_LESS_THAN;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_STEP_STR, ATTR_STEP_LEN) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_STEP)) return -1;
            if (query->len == ATTR_STEP_LEN) return -1;

            if (1 != ctlw_utils_textToFloat(query->data + ATTR_STEP_LEN, query->len - ATTR_STEP_LEN, &floatValue)) return -1;
            if (floatValue < 0) return -1;

            attrP->toSet |= LWM2M_ATTR_FLAG_STEP;
            attrP->step = floatValue;
        }
        else if (ctlw_lwm2m_strncmp((char *)query->data, ATTR_STEP_STR, ATTR_STEP_LEN - 1) == 0)
        {
            if (0 != ((attrP->toSet | attrP->toClear) & LWM2M_ATTR_FLAG_STEP)) return -1;
            if (query->len != ATTR_STEP_LEN - 1) return -1;

            attrP->toClear |= LWM2M_ATTR_FLAG_STEP;
        }
        else return -1;

        query = query->next;
    }

    return 0;
}

//----delete----
char* ctlw_getAttributesStr(ctlw_multi_option_t * query)
{
	ctlw_multi_option_t* ptr = query;
	char* attrStr = ctlw_lwm2m_malloc(256);
	if(!attrStr)
	{
		return NULL;
	}
	memset(attrStr, 0x00, 256);
	ptr = query;
	uint8_t len = 0;
	while(ptr)
	{
		memcpy(attrStr + len, ptr->data, ptr->len);
		len+= ptr->len;
		ptr = ptr->next;
		if(!ptr)
		{
			attrStr[len] = '\0';
			len += 1;
		}
		else
		{
			attrStr[len] = ';';
			len += 1;
		}
	}
	return attrStr;
}
//--------

//----add----
static uint8_t ctlw_intercept_obj_operation(lwm2m_uri_t * uriP, ctlw_coap_packet_t* message, ctiot_operate_type_e type, lwm2m_media_type_t format);
//--------
uint8_t ctlw_dm_handleRequest(lwm2m_context_t * contextP,
                         lwm2m_uri_t * uriP,
                         lwm2m_server_t * serverP,
                         ctlw_coap_packet_t * message,
                         ctlw_coap_packet_t * response)
{
    uint8_t result;
    lwm2m_media_type_t format;

    LOG_ARG("Code: %02X, server status: %s", message->code, STR_STATUS(serverP->status));
    LOG_URI(uriP);

    if (IS_OPTION(message, COAP_OPTION_CONTENT_TYPE))
    {
        format = ctlw_utils_convertMediaType(message->content_type);
    }
    else
    {
        format = LWM2M_CONTENT_TLV;
    }
    if (uriP->objectId == LWM2M_SECURITY_OBJECT_ID)
    {
        return COAP_404_NOT_FOUND;
    }

    if (serverP->status != STATE_REGISTERED
        && serverP->status != STATE_REG_UPDATE_NEEDED
        && serverP->status != STATE_REG_FULL_UPDATE_NEEDED
        && serverP->status != STATE_REG_UPDATE_PENDING)
    {
        return COAP_IGNORE;
    }

    // TODO: check ACL

    switch (message->code)
    {
    case COAP_GET:
        {
            uint8_t * buffer = NULL;
            size_t length = 0;
            int res;

            if (IS_OPTION(message, COAP_OPTION_OBSERVE))
            {
                lwm2m_data_t * dataP = NULL;
                int size = 0;
				//----add----
				result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_OBSERVE, format);
				//--------
				if(result == COAP_205_CONTENT)
				{
					result = ctlw_object_readData(contextP, uriP, &size, &dataP);
				}
				else
				{
					LOG_ARG("a result:%d\r\n",result);
				}
                if (COAP_205_CONTENT == result)
                {
                    result = ctlw_observe_handleRequest(contextP, uriP, serverP, size, dataP, message, response);

                    if (COAP_205_CONTENT == result)
                    {
                        if (IS_OPTION(message, COAP_OPTION_ACCEPT))
                        {
                            format = ctlw_utils_convertMediaType((ctlw_coap_content_type_t)message->accept[0]);
                        }
                        else
                        {
                            format = LWM2M_CONTENT_TLV;
                        }
						//----add----
                        if(uriP->objectId == 19)
                        {

	                        format = LWM2M_CONTENT_OPAQUE;
                        }
						//----------

						res = ctlw_lwm2m_data_serialize(uriP, size, dataP, &format, &buffer);
	                    if (res < 0)
	                    {
	            	        result = COAP_500_INTERNAL_SERVER_ERROR;
	                    }
	                    else
	                    {
	                        length = (size_t)res;
                            LOG_ARG("Observe Request[/%d/%d/%d]: %.*s\n", uriP->objectId, uriP->instanceId, uriP->resourceId, length, buffer);
						}
                    }
                }
				ctlw_lwm2m_data_free(size, dataP);
            }
            else if (IS_OPTION(message, COAP_OPTION_ACCEPT)
                  && message->accept_num == 1
                  && message->accept[0] == APPLICATION_LINK_FORMAT)
            {
                format = LWM2M_CONTENT_LINK;
				//----add-----
				result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_DISCOVER, format);
				if(result == COAP_205_CONTENT)
				//--------
                	result = ctlw_object_discover(contextP, uriP, serverP, &buffer, &length);
            }
            else
            {
                if (IS_OPTION(message, COAP_OPTION_ACCEPT))
                {
                    format = ctlw_utils_convertMediaType((ctlw_coap_content_type_t)message->accept[0]);
                }
				//----add----
				result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_READ, format);
				if(result == COAP_205_CONTENT)
				//----------
                	result = ctlw_object_read(contextP, uriP, &format, &buffer, &length);
            }

            if (COAP_205_CONTENT == result)
            {
                ctlw_coap_set_header_content_type(response, format);
                ctlw_coap_set_payload(response, buffer, length);
            }
            else
            {
                ctlw_lwm2m_free(buffer);
            }
        }
        break;

    case COAP_POST:
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
				//----add-----
                result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_CREATE, format);
				if(result == COAP_205_CONTENT)
				//-----------
               		result = ctlw_object_create(contextP, uriP, format, message->payload, message->payload_len);
                if (result == COAP_201_CREATED)
                {
                    //longest uri is /65535/65535 = 12 + 1 (null) chars
                    char location_path[13] = "";
                    //instanceId expected
                    if ((uriP->flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }

                    if (sprintf(location_path, "/%d/%d", uriP->objectId, uriP->instanceId) < 0)
                    {
                        result = COAP_500_INTERNAL_SERVER_ERROR;
                        break;
                    }
                    ctlw_coap_set_header_location_path(response, location_path);

                    ctlw_lwm2m_update_registration(contextP, 0, true);
                }
            }
            else if (!LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
				//----add----
                result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_WRITE, format);
				if(result == COAP_205_CONTENT)
				//----------
                	result = ctlw_object_write(contextP, uriP, format, message->payload, message->payload_len);

            }
            else
            {
				//----add----
                result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_EXECUTE, format);
				if(result == COAP_205_CONTENT)
				//----------
                	result = ctlw_object_execute(contextP, uriP, message->payload, message->payload_len);
            }
        }
        break;

    case COAP_PUT:
        {
            if (IS_OPTION(message, COAP_OPTION_URI_QUERY))
            {
                lwm2m_attributes_t attr;
                if (0 != prv_readAttributes(message->uri_query, &attr))
                {
                    result = COAP_400_BAD_REQUEST;
                }
                else
                {
					//----add----
					result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_WRITE, LWM2M_CONTENT_LINK);
					if(result == COAP_205_CONTENT)
					{
					//--------
	                    result = ctlw_observe_setParameters(contextP, uriP, serverP, &attr);
					}
                }
            }
            else if (LWM2M_URI_IS_SET_INSTANCE(uriP))
            {
				//----add----
                result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_WRITE, format);
				if(result == COAP_205_CONTENT)
				{
				//----------
#ifdef PLATFORM_XINYI
                    ctiot_context_t *pContext = ctiot_get_context();
                    if(pContext->abstractCloudFlag == 1)
                    {
                        pContext->abstractDownMsgId = message->mid;//抽象云AT,记录下行数据messageId
                    }
#endif
	                result = ctlw_object_write(contextP, uriP, format, message->payload, message->payload_len);
				}
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
          }

        break;

    case COAP_DELETE:
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP) || LWM2M_URI_IS_SET_RESOURCE(uriP))
            {
                result = COAP_400_BAD_REQUEST;
            }
            else
            {
				//----add----
                result = ctlw_intercept_obj_operation(uriP, message, OPERATE_TYPE_DELETE, format);
				if(result == COAP_205_CONTENT)
				{
				//----------
               	 	result = ctlw_object_delete(contextP, uriP);
				}
                if (result == COAP_202_DELETED)
                {
                    ctlw_lwm2m_update_registration(contextP, 0, true);
                }
            }
        }
        break;

    default:
        result = COAP_400_BAD_REQUEST;
        break;
    }
//----add-----
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	ctiot_context_t* pContext=ctiot_get_context();
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
	{
		if(message->payload != NULL)
		{
			ctlw_lwm2m_free(message->payload);
			message->payload = NULL;
			message->payload_len = 0;
		}
	}
#endif
//----------
    return result;
}

//----add----
static uint8_t ctlw_intercept_obj_operation(lwm2m_uri_t * uriP, ctlw_coap_packet_t* message, ctiot_operate_type_e type, lwm2m_media_type_t format)
{
	uint8_t result = COAP_205_CONTENT;
	ctiot_context_t* pContext=ctiot_get_context();
	if( uriP->objectId == 19 && (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId == 1)
		&& (LWM2M_URI_IS_SET_RESOURCE(uriP) && uriP->resourceId == 0) && type == OPERATE_TYPE_WRITE && !(IS_OPTION(message, COAP_OPTION_URI_QUERY)))
	{
		if(format != LWM2M_CONTENT_OPAQUE)
	    {
	         return COAP_415_UNSUPPORTED_CONTENT_FORMAT;
	    }
	}

#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
		if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
		{
			if(message->payload != NULL)
			{
				uint8_t *newPayload = ctlw_lwm2m_malloc(1024); // 明文数据长度不能大于1024Byte，否则解密失败。
				int32_t newPayloadLen = 0;

				int32_t encrtpyResult = Sec_dev_Decrypt(message->payload, message->payload_len, newPayload, 1024,  &newPayloadLen);
				if(encrtpyResult == 0)
				{
					message->payload = newPayload;
					message->payload_len = newPayloadLen;
				}
				else
				{
					message->payload = NULL;
					message->payload_len = 0;

					char uriStr[20] = {0};
					int32_t len = ctlw_uri_toString(uriP, uriStr, 20, NULL);
					if(len < 0)
					{
						return COAP_IGNORE;
					}
					//uriStr[len - 1] = '\0'; // 去除最后一个'/'
					//ctiot_publish_sdk_notification(uriStr, CTIOT_APP_NOTIFICATION,CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LWEVENT, 3, 0, 0, NULL);
					return COAP_403_FORBIDDEN;
				}
			}
		}
#endif

	if(type == OPERATE_TYPE_WRITE && IS_OPTION(message, COAP_OPTION_URI_QUERY) && message->code == COAP_PUT)
	{
		result = COAP_401_UNAUTHORIZED;
	}
	return result;
}
//--------
#endif

