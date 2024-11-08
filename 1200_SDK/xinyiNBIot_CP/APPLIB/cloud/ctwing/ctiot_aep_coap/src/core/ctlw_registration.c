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
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Manuel Sangoi - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin - Please refer to git log
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_sdk_internals.h"
#include "ctlw_abstract_signal.h"
#define MAX_LOCATION_LENGTH 50      // strlen("/rd/65534") + 1



//----add----
//extern CTIOT_AEP_INIT initInfo;
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
//----------

#ifdef LWM2M_CLIENT_MODE
int ctlw_prv_getRegistrationQueryLength(lwm2m_context_t * contextP,
                                          lwm2m_server_t * server)
{
    int index;
    int res;
    uint8_t buffer[21];

    index = strlen(QUERY_STARTER QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);
    index += strlen(contextP->endpointName);

    if (NULL != contextP->msisdn)
    {
        index += strlen(QUERY_DELIMITER QUERY_SMS);
        index += strlen(contextP->msisdn);
    }

    switch (server->binding)
    {
    case BINDING_U:
        index += strlen("&b=U");
        break;
    case BINDING_UQ:
        index += strlen("&b=UQ");
        break;
    case BINDING_S:
        index += strlen("&b=S");
        break;
    case BINDING_SQ:
        index += strlen("&b=SQ");
        break;
    case BINDING_US:
        index += strlen("&b=US");
        break;
    case BINDING_UQS:
        index += strlen("&b=UQS");
        break;
    default:
        return 0;
    }

    if (0 != server->lifetime)
    {
        index += strlen(QUERY_DELIMITER QUERY_LIFETIME);
        res = ctlw_utils_intToText(server->lifetime, buffer, sizeof(buffer));
        if (res == 0) return 0;
        index += res;
    }

    return index + 1;
}

int ctlw_prv_getRegistrationQuery(lwm2m_context_t * contextP,
                                    lwm2m_server_t * server,
                                    char * buffer,
                                    size_t length)
{
    int index;
    int res;

    index = ctlw_utils_stringCopy(buffer, length, QUERY_STARTER QUERY_VERSION_FULL QUERY_DELIMITER QUERY_NAME);
    if (index < 0) return 0;
    res = ctlw_utils_stringCopy(buffer + index, length - index, contextP->endpointName);
    if (res < 0) return 0;
    index += res;

    if (NULL != contextP->msisdn)
    {
        res = ctlw_utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_SMS);
        if (res < 0) return 0;
        index += res;
        res = ctlw_utils_stringCopy(buffer + index, length - index, contextP->msisdn);
        if (res < 0) return 0;
        index += res;
    }

    switch (server->binding)
    {
    case BINDING_U:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=U");
        break;
    case BINDING_UQ:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=UQ");
        break;
    case BINDING_S:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=S");
        break;
    case BINDING_SQ:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=SQ");
        break;
    case BINDING_US:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=US");
        break;
    case BINDING_UQS:
        res = ctlw_utils_stringCopy(buffer + index, length - index, "&b=UQS");
        break;
    default:
        res = -1;
    }
    if (res < 0) return 0;
    index += res;

    if (0 != server->lifetime)
    {
        res = ctlw_utils_stringCopy(buffer + index, length - index, QUERY_DELIMITER QUERY_LIFETIME);
        if (res < 0) return 0;
        index += res;
        res = ctlw_utils_intToText(server->lifetime, (uint8_t *)(buffer + index), length - index);
        if (res == 0) return 0;
        index += res;
    }

    if(index < (int)length)
    {
        buffer[index++] = '\0';
    }
    else
    {
        return 0;
    }

    return index;
}


static void prv_handleRegistrationReply(lwm2m_transaction_t *transacP,
                                        void *message)
{
	//----modified----
    ctlw_coap_packet_t *packet = (ctlw_coap_packet_t *)message;
    lwm2m_server_t *targetP = (lwm2m_server_t *)(transacP->userData);

    ctiot_context_t *pContext = ctiot_get_context();
	ctlw_coap_packet_t *transMsg = transacP->message;

	if(transMsg != NULL && transMsg->payload != NULL)
	{
		ctlw_lwm2m_free(transMsg->payload);
	}

    if (packet != NULL)
        ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply,code=%d\r\n", packet->code);

    if ((pContext->sessionStatus == UE_LOGINING) && (targetP->status == STATE_REG_PENDING))
    {
        time_t tv_sec = ctlw_lwm2m_gettime();
        targetP->registration = tv_sec;

        if (packet != NULL && packet->code == COAP_201_CREATED)
        {
            targetP->status = STATE_REGISTERED;
            if (NULL != targetP->location)
            {
                ctlw_lwm2m_free(targetP->location);
            }
            targetP->location = ctlw_coap_get_multi_option_as_string(packet->location_path);
            if (ctiot_location_path_validation(targetP->location) == CTIOT_OTHER_ERROR)
            {
                if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
                {
                	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"reg location path error\r\n");
                    ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_LOC_PATH_ERROR);
                }
                return;
            }
            LOG("Registration successful");

            if (ctiot_trace_ip_by_bindmode() == CTIOT_ONIP_RECONNECT)
            {
                ctiot_signal_subscribe_ip_event(ctiot_ip_event_async, (server_ip_type_e)pContext->serverIPType);
            }
			if(pContext->clientWorkMode == UQ_WORK_MODE && pContext->connectionType == MODE_DTLS)
			{
				if(ctiot_signal_subscribe_psm_status(NULL) != CTIOT_NB_SUCCESS)
				{
					ctiot_exit_on_error(pContext);
					return ;
				}
			}
			ctiot_publish_sdk_notification(NULL,CTIOT_SYSTEM_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_REG, 0, 0, 0, NULL);
			pContext->bootFlag = BOOT_LOCAL_BOOTUP;
            if(ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGINED))
           	{
           		ctiot_change_client_status(CTIOT_STATE_REG_PENDING,CTIOT_STATE_READY);
            }
            
#ifndef PLATFORM_XINYI
			ctiot_update_sdataflash_needed();
#else
            ctiot_update_sdataflash_needed(false);
#endif
            #if CTIOT_TIMER_AUTO_UPDATE == 1
            ctiot_start_auto_update_timer();
            #endif
        }
        else if (packet == NULL)
        {
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply:reg fail CTIOT_TIME_OUT_ERROR\r\n");
            targetP->status = STATE_REG_FAILED;

            if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
            {
                ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_TIME_OUT_ERROR);
            }
        }
        else if (packet->code == 0x83 /*COAP_403*/)
        {
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply:reg fail CTIOT_FORBID_ERROR\r\n");
            if(g_softap_fac_nv->off_debug == 0)
                xy_assert(0);
            targetP->status = STATE_REG_FAILED;

			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
            {
                ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_FORBID_ERROR);
            }
        }
        else if (packet->code == 0x80 /*COAP_400*/)
        {
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply:reg fail CTIOT_EPNAME_ERROR\r\n");
            if(g_softap_fac_nv->off_debug == 0)
                xy_assert(0);
            targetP->status = STATE_REG_FAILED;

			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
            {
                ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_EPNAME_ERROR);
            }
        }
        else if (packet->code == 0x8C /*COAP_412*/)
        {
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply:reg fail CTIOT_PRECON_ERROR\r\n");
            targetP->status = STATE_REG_FAILED;

			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
            {
                ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_PRECON_ERROR);
            }
        }
        else
        {
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleRegistrationReply:reg fail CTIOT_PLAT_OTH_ERROR\r\n");
            targetP->status = STATE_REG_FAILED;

			if (ctiot_update_session_status( NULL,pContext->sessionStatus, UE_LOGIN_OUTING))
            {
                ctiot_set_release_flag(RELEASE_MODE_LOGIN_FAIL_2, CTIOT_PLAT_OTH_ERROR);
            }
        }
    }
	//--------
}

// send the registration for a single server
static uint8_t prv_register(lwm2m_context_t * contextP,
                            lwm2m_server_t * server)
{
     char * query;
    int query_length;
    uint8_t * payload;
    int payload_length;
    lwm2m_transaction_t * transaction;
	//----add----
	ctiot_context_t* pContext=ctiot_get_context();
	//----------

    payload_length = ctlw_object_getRegisterPayloadBufferLength(contextP);
    if(payload_length == 0) return COAP_500_INTERNAL_SERVER_ERROR;
    payload = ctlw_lwm2m_malloc(payload_length);
    if(!payload) return COAP_500_INTERNAL_SERVER_ERROR;
    payload_length = ctlw_object_getRegisterPayload(contextP, payload, payload_length);
    if(payload_length == 0)
    {
        ctlw_lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
//----add----
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	if(pContext->connectionType == MODE_ENCRYPTED_PAYLOAD)
	{
		uint8_t *newPayload = ctlw_lwm2m_malloc(payload_length + 244); // 由于加密后数据长度会大于明文长度，所以密文数据缓冲区的长度要在明文数据长度基础上添加244byte的缓冲区长度
		int32_t newPayloadLen = 0;
		int32_t encrtpyResult = Sec_dev_Encrypt(payload, payload_length, newPayload, payload_length + 244,  &newPayloadLen);
		ctlw_lwm2m_free(payload);
		if(encrtpyResult == 0)
		{
			payload = newPayload;
			payload_length = newPayloadLen;
		}
		else
		{
			return COAP_500_INTERNAL_SERVER_ERROR;
		}
	}
#endif

	int extendlen=0;
	//if(pContext->regParaMode == PARAM_CT_ENHANCED_MODE)
	//{
    	extendlen = ctiot_coap_extend_query_len();
	//}
//--------
	query_length = ctlw_prv_getRegistrationQueryLength(contextP, server);
    if(query_length == 0)
    {
        ctlw_lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    query = ctlw_lwm2m_malloc(query_length+extendlen);
    if(!query)
    {
        ctlw_lwm2m_free(payload);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if(ctlw_prv_getRegistrationQuery(contextP, server, query, query_length) != query_length)
    {
        ctlw_lwm2m_free(payload);
        ctlw_lwm2m_free(query);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
//----add----
	//if(pContext->regParaMode == PARAM_CT_ENHANCED_MODE)
	if(extendlen > 0)
	{
	    char* extend_query= ctiot_coap_extend_query(extendlen);

	    if(extend_query == NULL)
	    {
	        ctlw_lwm2m_free(payload);
	        ctlw_lwm2m_free(query);
	        return COAP_500_INTERNAL_SERVER_ERROR;
	    }

	    sprintf(query,"%s%s",query,extend_query);
	    if(extend_query!=NULL)
	    {
	        ctlw_lwm2m_free(extend_query);
	    }
	}
//---------
    if (server->sessionH == NULL)
    {
        server->sessionH = ctlw_lwm2m_connect_server(server->secObjInstID, contextP->userData);
    }

    if (NULL == server->sessionH)
    {
        ctlw_lwm2m_free(payload);
        ctlw_lwm2m_free(query);
        return COAP_503_SERVICE_UNAVAILABLE;
    }

    transaction = ctlw_transaction_new(server->sessionH, COAP_POST, NULL, NULL, contextP->nextMID++, 4, NULL);
    if (transaction == NULL)
    {
        ctlw_lwm2m_free(payload);
        ctlw_lwm2m_free(query);
//----add----
//COAP_503_SERVICE_UNAVAILABLE 改为 COAP_500_INTERNAL_SERVER_ERROR
        return COAP_500_INTERNAL_SERVER_ERROR;
//---------
    }

    ctlw_coap_set_header_uri_path(transaction->message, "/"URI_REGISTRATION_SEGMENT);
    ctlw_coap_set_header_uri_query(transaction->message, query);
    ctlw_coap_set_header_content_type(transaction->message, LWM2M_CONTENT_LINK);
    ctlw_coap_set_payload(transaction->message, payload, payload_length);

    transaction->callback = prv_handleRegistrationReply;
    transaction->userData = (void *) server;

    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);
	//----modified----
	ctlw_lwm2m_free(query);
	//--------
    server->status = STATE_REG_PENDING;

    return COAP_NO_ERROR;
}

void ctlw_prv_handleRegistrationUpdateReply(lwm2m_transaction_t * transacP,
                                              void * message)
{
    ctlw_coap_packet_t * packet = (ctlw_coap_packet_t *)message;
    lwm2m_server_t * targetP = (lwm2m_server_t *)(transacP->userData);

    if (targetP->status == STATE_REG_UPDATE_PENDING)
    {
        time_t tv_sec = ctlw_lwm2m_gettime();
		//----modified----
        targetP->registration = tv_sec;
		//------------

        if (packet != NULL && packet->code == COAP_204_CHANGED)
        {
            targetP->status = STATE_REGISTERED;
            LOG("Registration update successful");
        }
        else
        {
            targetP->status = STATE_REG_FAILED;
            LOG("Registration update failed");
        }
    }
}

static int prv_updateRegistration(lwm2m_context_t * contextP,
                                  lwm2m_server_t * server,
                                  bool withObjects)
{
    lwm2m_transaction_t * transaction;
    uint8_t * payload = NULL;
    int payload_length;

    transaction = ctlw_transaction_new(server->sessionH, COAP_POST, NULL, NULL, ctlw_lwm2m_get_next_mid(contextP)/*contextP->nextMID++*/, 4, NULL);
    if (transaction == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    ctlw_coap_set_header_uri_path(transaction->message, server->location);

    if (withObjects == true)
    {
        payload_length = ctlw_object_getRegisterPayloadBufferLength(contextP);
        if(payload_length == 0)
        {
            ctlw_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload = ctlw_lwm2m_malloc(payload_length);
        if(!payload)
        {
            ctlw_transaction_free(transaction);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }

        payload_length = ctlw_object_getRegisterPayload(contextP, payload, payload_length);
        if(payload_length == 0)
        {
            ctlw_transaction_free(transaction);
            ctlw_lwm2m_free(payload);
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        ctlw_coap_set_payload(transaction->message, payload, payload_length);
    }

    transaction->callback = ctlw_prv_handleRegistrationUpdateReply;
    transaction->userData = (void *) server;
	//----add----
    transaction->buffer=NULL;
    transaction->buffer_len=0;
	//----------
    contextP->transactionList = (lwm2m_transaction_t *)LWM2M_LIST_ADD(contextP->transactionList, transaction);

	//----modified----
    server->status = STATE_REG_UPDATE_PENDING;
	//-------------
    return COAP_NO_ERROR;
}

// update the registration of a given server

int ctlw_lwm2m_update_registration(lwm2m_context_t * contextP,
                              uint16_t shortServerID,
                              bool withObjects)
{
    lwm2m_server_t * targetP;
    uint8_t result;

    LOG_ARG("State: %s, shortServerID: %d", STR_STATE(contextP->state), shortServerID);

	//---add----
	//---------
    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    if (targetP == NULL)
    {
        if (ctlw_object_getServers(contextP, false) == -1)
        {
            LOG("No server found");
            return COAP_404_NOT_FOUND;
        }
    }
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
        if (shortServerID != 0)
        {
            if (targetP->shortID == shortServerID)
            {
                // found the server, trigger the update transaction
                if (targetP->status == STATE_REGISTERED
                 || targetP->status == STATE_REG_UPDATE_PENDING)
                {
                    if (withObjects == true)
                    {
                        targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    else
                    {
                        targetP->status = STATE_REG_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else if ((targetP->status == STATE_REG_FULL_UPDATE_NEEDED)
                      || (targetP->status == STATE_REG_UPDATE_NEEDED))
                {
                    // if REG (FULL) UPDATE is already set, returns COAP_NO_ERROR
                    if (withObjects == true)
                    {
                        targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                    }
                    return COAP_NO_ERROR;
                }
                else
                {
                    return COAP_400_BAD_REQUEST;
                }
            }
        }
        else
        {
            if (targetP->status == STATE_REGISTERED
             || targetP->status == STATE_REG_UPDATE_PENDING)
            {
                if (withObjects == true)
                {
                    targetP->status = STATE_REG_FULL_UPDATE_NEEDED;
                }
                else
                {
                    targetP->status = STATE_REG_UPDATE_NEEDED;
                }
            }
        }
        targetP = targetP->next;
    }

    if (shortServerID != 0
     && targetP == NULL)
    {
        // no server found
        result = COAP_404_NOT_FOUND;
    }

    return result;
}
uint8_t ctlw_registration_start(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
    uint8_t result;

    LOG_ARG("State: %s", STR_STATE(contextP->state));

    result = COAP_NO_ERROR;

    targetP = contextP->serverList;
    while (targetP != NULL && result == COAP_NO_ERROR)
    {
		//----modified----
        result = prv_register(contextP, targetP);
        targetP = targetP->next;
		//-------------
    }

    return result;
}


/*
 * Returns STATE_REG_PENDING if at least one registration is still pending
 * Returns STATE_REGISTERED if no registration is pending and there is at least one server the client is registered to
 * Returns STATE_REG_FAILED if all registration failed.
 */
lwm2m_status_t ctlw_registration_getStatus(lwm2m_context_t * contextP)
{
    lwm2m_server_t * targetP;
	//----modified----
    lwm2m_status_t reg_status = STATE_REG_FAILED;
    if( contextP == NULL )
		return reg_status;

    LOG_ARG("State: %s", STR_STATE(contextP->state));
    targetP = contextP->serverList;
	//------------
    while (targetP != NULL)
    {
        LOG_ARG("targetP->status: %s", STR_STATUS(targetP->status));
        switch (targetP->status)
        {
            case STATE_REGISTERED:
            case STATE_REG_UPDATE_NEEDED:
            case STATE_REG_FULL_UPDATE_NEEDED:
            case STATE_REG_UPDATE_PENDING:
                if (reg_status == STATE_REG_FAILED)
                {
                    reg_status = STATE_REGISTERED;
                }
                break;
			case STATE_DEREG_FAILED:
				reg_status = STATE_DEREG_FAILED;
				break;

            case STATE_REG_PENDING:
                reg_status = STATE_REG_PENDING;
                break;

            case STATE_REG_FAILED:
				reg_status = STATE_REG_FAILED;
				break;
            case STATE_DEREG_PENDING:
				reg_status = STATE_DEREG_PENDING;
				break;
            case STATE_DEREGISTERED:
				reg_status = STATE_DEREGISTERED;
				break;
            default:
                break;
        }
        LOG_ARG("reg_status: %s", STR_STATUS(reg_status));

        targetP = targetP->next;
    }

    return reg_status;
}

static void prv_handleDeregistrationReply(lwm2m_transaction_t * transacP,
                                          void * message)
{
    lwm2m_server_t * targetP;
	//----add----
	LOG("prv_handleDeregistrationReply...\r\n");
	//---------
    targetP = (lwm2m_server_t *)(transacP->userData);

    if (NULL != targetP)
    {
		//----modified----
    	ctlw_coap_packet_t * packet = (ctlw_coap_packet_t *)message;
		if (packet != NULL && packet->code == COAP_202_DELETED)
        {
			targetP->status = STATE_DEREGISTERED;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleDeregistrationReply:targetP->status = STATE_DEREGISTERED\r\n");
        }
		else
		{
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"prv_handleDeregistrationReply:targetP->status = STATE_DEREG_FAILED\r\n");
			targetP->status = STATE_DEREG_FAILED;
		}
		//-----------
    }
}

void ctlw_registration_deregister(lwm2m_context_t * contextP,
                             lwm2m_server_t * serverP)
{
	ctlw_coap_packet_t message[1];
	//----add----
	uint8_t       temp_token[COAP_TOKEN_LEN];
    time_t        tv_sec = ctlw_lwm2m_gettime();
	uint16_t      mID    = ctlw_lwm2m_get_next_mid(contextP);//contextP->nextMID++;
	//--------

    LOG_ARG("State: %s, serverP->status: %s", STR_STATE(contextP->state), STR_STATUS(serverP->status));

    if (serverP->status == STATE_DEREGISTERED
     || serverP->status == STATE_REG_PENDING
     || serverP->status == STATE_DEREG_PENDING
     || serverP->status == STATE_REG_FAILED
     || serverP->location == NULL)
    {
        return;
    }

	//----modified----
	ctlw_coap_init_message(message, COAP_TYPE_NON, COAP_DELETE, mID);
	ctlw_coap_set_header_uri_path(message,serverP->location);

	temp_token[0] = mID;
    temp_token[1] = mID >> 8;
    temp_token[2] = tv_sec;
    temp_token[3] = tv_sec >> 8;
    temp_token[4] = tv_sec >> 16;
    temp_token[5] = tv_sec >> 24;
    ctlw_coap_set_header_token(message, temp_token, 4);
	ctlw_message_send(contextP, message, serverP->sessionH);
	//------------
}

//----add----
void ctlw_registration_deregister_by_con(lwm2m_context_t * contextP,
							  lwm2m_server_t * serverP)
{
	lwm2m_transaction_t * transaction;
	LOG("ctlw_registration_deregister_by_con\r\n");
	LOG_ARG("State: %s, serverP->status: %s", STR_STATE(contextP->state), STR_STATUS(serverP->status));

	if (serverP->status == STATE_DEREGISTERED
		|| serverP->status == STATE_REG_PENDING
		|| serverP->status == STATE_DEREG_PENDING
		|| serverP->status == STATE_REG_FAILED
		|| serverP->location == NULL)
	{
		serverP->status = STATE_DEREG_FAILED;
		return;
	}
	transaction = ctlw_transaction_new(serverP->sessionH, COAP_DELETE, NULL, NULL, contextP->nextMID++, 4, NULL);
	if (transaction == NULL)
	{
		serverP->status = STATE_DEREG_FAILED;
		return;
	}
	ctlw_coap_set_header_uri_path(transaction->message, serverP->location);

	transaction->callback = prv_handleDeregistrationReply;
	transaction->userData = (void *) serverP;

	if (ctlw_transaction_send(contextP, transaction) == 0)
	{
		serverP->status = STATE_DEREG_PENDING;
		ctlw_transaction_add(contextP,transaction);
	}
	else
	{
		serverP->status = STATE_DEREG_FAILED;
		ctlw_transaction_free(transaction);
	}
	LOG("ctlw_registration_deregister_by_con send end...\r\n");
}
//-----------------

#endif

// for each server update the registration if needed
// for each client check if the registration expired


void ctlw_registration_step(lwm2m_context_t * contextP,
                       time_t currentTime,
                       time_t * timeoutP)
{
#ifdef LWM2M_CLIENT_MODE
    lwm2m_server_t * targetP = contextP->serverList;
	//----delete----
	ctiot_context_t* pContext=ctiot_get_context();
	//--------
    LOG_ARG("State: %s", STR_STATE(contextP->state));
    targetP = contextP->serverList;
    while (targetP != NULL)
    {
        switch (targetP->status)
        {
        case STATE_REGISTERED:
        {
			//----modified----
			//----------
        }
        break;

        case STATE_REG_UPDATE_NEEDED:
		  	prv_updateRegistration(contextP, targetP, false);
			break;

        case STATE_REG_FULL_UPDATE_NEEDED:
            prv_updateRegistration(contextP, targetP, true);
            break;

        case STATE_REG_FAILED:
            if (targetP->sessionH != NULL)
            {
                ctlw_lwm2m_close_connection(targetP->sessionH, contextP->userData);
                targetP->sessionH = NULL;
            }
            break;

        default:
            break;
        }
        targetP = targetP->next;
    }

#endif

}

//----add----
int ctlw_updateRegistration(lwm2m_context_t * contextP,lwm2m_server_t * server)
{
    return prv_updateRegistration(contextP,server,false);
}
//----------
