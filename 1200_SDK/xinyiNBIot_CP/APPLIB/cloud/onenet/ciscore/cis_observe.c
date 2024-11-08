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
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Baijie & Longrong, China Mobile - Please refer to git log
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

#include "xy_utils.h"
#include "at_onenet.h"
#include "cis_internals.h"
#include "cis_log.h"
#include "std_object/std_object.h"
#include "xy_cis_api.h"

extern onenet_context_reference_t onenet_context_refs[CIS_REF_MAX_NUM];

typedef struct 
{
    unsigned short ackID;
    void* context;
} notify_userdata_t;


#if CIS_ENABLE_CMIOT_OTA
extern uint8_t std_poweruplog_ota_notify(st_context_t * contextP,cis_oid_t * objectid,uint16_t mid);
uint16_t cmiot_ota_observe_mid = 0;
#endif


static st_observed_t * prv_findObserved(st_context_t * contextP,st_uri_t * uriP);
static void prv_unlinkObserved(st_context_t * contextP,st_observed_t * observedP);
st_observed_t * prv_getObserved(st_context_t * contextP,st_uri_t * uriP);



coap_status_t observe_asynAckNodata(st_context_t * context,st_request_t* request,cis_coapret_t result, uint8_t raiflag)
{
    st_observed_t* observed;
    st_uri_t* uriP = &request->uri;
    coap_packet_t packet[1];

    if (request->tokenLen == 0)
    {
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    coap_init_message(packet, COAP_TYPE_ACK, result, (uint16_t)request->mid);
    coap_set_header_token(packet, request->token, request->tokenLen);

    if(request->type == CALLBACK_TYPE_OBSERVE)
    {
        if (!CIS_URI_IS_SET_INSTANCE(uriP) && CIS_URI_IS_SET_RESOURCE(uriP)) 
        {
            return COAP_400_BAD_REQUEST;
        }

        if(result == COAP_205_CONTENT)
        {
            observed = prv_getObserved(context, uriP);
            if (observed == NULL) 
            {
                return COAP_500_INTERNAL_SERVER_ERROR;
            }

            observed->actived = true;
            observed->msgid = request->mid;
            observed->tokenLen = request->tokenLen;
            cis_memcpy(observed->token, request->token, request->tokenLen);
            observed->lastTime = utils_gettime_s();
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
                observed->accept_type = request->accept_type;
                coap_set_header_content_type(packet, observed->accept_type == -1 ? LWM2M_CONTENT_TLV_NEW : observed->accept_type);
            }
            coap_set_header_observe(packet, observed->counter++);
        }
    }
    else if(request->type == CALLBACK_TYPE_OBSERVE_CANCEL)
    {
        observed = prv_findObserved(context, uriP);
        if (observed == NULL) 
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
        observe_cancel(context, MESSAGEID_INVALID, uriP);
    }
    else if(request->type == CALLBACK_TYPE_OBSERVE_PARAMS)
    {
        observed = prv_findObserved(context, uriP);
        if (observed == NULL) 
        {
            return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }
    else{
        return COAP_400_BAD_REQUEST;
    }

    core_callbackEvent(context, CIS_EVENT_COMMON_STATUS_READY, NULL);	//xy add for bak session by observe

    //onenet_bak_obsvInfo(context);
    packet_send(context, packet,raiflag);

    return COAP_NO_ERROR;
}
extern void utils_send_timer_del();
extern osSemaphoreId_t g_cis_notify_sem;
static void prv_handleNotifyReply(st_transaction_t * transacP, void * message)
{
    coap_packet_t * packet = (coap_packet_t *)message;
    notify_userdata_t *notify_userdata = (notify_userdata_t*)transacP->userData;
    if(notify_userdata == NULL){
        LOGE("ERROR:prv_handleNotifyReply failed,notify_userdata is NULL");
        return;
    }


	//Get ack response, give the notify sem
	if(g_cis_notify_sem != NULL)
		osSemaphoreRelease(g_cis_notify_sem);

    if (packet == NULL)
    {
        core_callbackEvent(notify_userdata->context,CIS_EVENT_NOTIFY_FAILED,(void *)notify_userdata->ackID);
    }
    else
    {
        if (packet->type == COAP_TYPE_ACK && (packet->code == 0 ||packet->code == CIS_COAP_204_CHANGED))
            core_callbackEvent((void *)notify_userdata->context,CIS_EVENT_NOTIFY_SUCCESS,(void *)notify_userdata->ackID);
        else
            core_callbackEvent((void *)notify_userdata->context,CIS_EVENT_NOTIFY_FAILED,(void *)notify_userdata->ackID);
    }
    cis_free(notify_userdata);
}

coap_status_t observe_asynReport(st_context_t * context,const st_observed_t* observe,st_notify_t* notify,uint8_t raiflag)
{
    st_observed_t* target;
    coap_packet_t packet[1];
    coap_status_t ret = COAP_NO_ERROR;

    et_media_type_t formatP;
    uint8_t* bufferP;
    int32_t length;

    st_transaction_t * transaction = NULL;
    notify_userdata_t *notify_userdata = NULL;
    
    st_data_t* data = observe->reportData;
    uint16_t count = observe->reportDataCount;
    char *at_str = NULL;

    if(count < 0 || data == NULL || observe == NULL || notify == NULL){
        return COAP_400_BAD_REQUEST;
    }

    for (target = context->observedList ; target != NULL ; target = target->next)
    {
        if (target->msgid ==  observe->msgid)
            break;
    }

    if(target == NULL){
        return COAP_404_NOT_FOUND;
    }

    if(CIS_URI_IS_SET_INSTANCE(&target->uri) && CIS_URI_IS_SET_RESOURCE(&target->uri))
    {
        if(count > 1)
        {
            return COAP_400_BAD_REQUEST;
        }else
            if(!uri_exist(&target->uri,&observe->uri))
            {
                return COAP_400_BAD_REQUEST;
            }
    }


    if (data != NULL)
    {
        if (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_YANFEI)
            formatP = observe->accept_type == -1 ? LWM2M_CONTENT_TLV_NEW : observe->accept_type;
        else
            formatP = target->format;
        length = data_serialize(&target->uri, count, data, &formatP, &bufferP);
        if (length <= -1) 
        {
            LOGE("ERROR:observe report data serialize failed.");
            return COAP_500_INTERNAL_SERVER_ERROR;
        }else if(length == 0)
        {
            LOGW("WARNING:observe report serialize length == 0.");
        }
    }else
    {
        LOGD("object_asyn_ack_withdata data is null.");
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (notify->ackID == 0)
    {
        if(context->cloud_platform == CLOUD_PLATFORM_ANDLINK || (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_ANDLINK))
        {
            coap_init_message(packet, COAP_TYPE_NON, COAP_POST,context->nextMid);
            coap_set_header_content_type(packet, LWM2M_CONTENT_JSON_NEW);
            char endpointname_tmp1[40]="ep=";
            strcat(endpointname_tmp1, context->endpointName);
            coap_set_header_uri_path(packet, "dp");
            coap_set_header_uri_query(packet, endpointname_tmp1);
        }
        else
        {
            coap_init_message(packet, COAP_TYPE_NON, COAP_205_CONTENT,0);
            coap_set_header_content_type(packet, formatP);
            coap_set_header_observe(packet, target->counter++);
        }
        coap_set_payload(packet, bufferP, length);
        target->lastTime = utils_gettime_s();
        target->lastMid = context->nextMid++;
        packet->mid = target->lastMid;
        coap_set_header_token(packet, target->token, target->tokenLen);
        ret = packet_send(context, packet, raiflag);
        if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
#if LWM2M_COMMON_VER
            at_str = xy_malloc(32);
            if(notify->report_type)
                snprintf(at_str, 32, "+QLASENDDATA: %d,%d", convert_coap_code_to_commond_result_code(ret),target->lastMid);
            else
                snprintf(at_str, 32, "+QLANOTIFY: %d", convert_coap_code_to_commond_result_code(ret));

            send_urc_to_ext(at_str, strlen(at_str));
            xy_free(at_str);
#endif
        }
    }
    else
    {
        if(context->cloud_platform == CLOUD_PLATFORM_ANDLINK || (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_ANDLINK))
            transaction = transaction_new(COAP_TYPE_CON, COAP_POST, NULL, NULL, context->nextMid, target->tokenLen, target->token);
        else
            transaction = transaction_new(COAP_TYPE_CON, COAP_205_CONTENT, NULL, NULL, context->nextMid, target->tokenLen, target->token);
        if (transaction == NULL) 
        {
            LOGD("observe_asynReport transaction_new failed.");
            ret = COAP_500_INTERNAL_SERVER_ERROR;
            goto out;

        }
        /*
#if CIS_ENABLE_AUTH
        if(context->authCode!=NULL)
            coap_set_header_auth_code( transaction->message, context->authCode );
#endif*/
        if(context->cloud_platform == CLOUD_PLATFORM_ANDLINK || (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_ANDLINK))
        {
            coap_set_header_content_type(transaction->message, LWM2M_CONTENT_JSON_NEW);
        }
        else
        {
            coap_set_header_content_type(transaction->message, formatP);
        }
        coap_set_payload(transaction->message, bufferP, length);
        target->lastTime = utils_gettime_s();
        target->lastMid = context->nextMid++;
        //packet->mid = target->lastMid;
        coap_set_header_token(transaction->message, target->token, target->tokenLen);
        if(context->cloud_platform == CLOUD_PLATFORM_ANDLINK || (context->cloud_platform == CLOUD_PLATFORM_COMMON && context->platform_common_type == CLOUD_PLATFORM_COMMON_ANDLINK))
        {
            char endpointname_tmp[40]="ep=";
            strcat(endpointname_tmp, context->endpointName);
            coap_set_header_uri_path(transaction->message, "dp");
            coap_set_header_uri_query(transaction->message, endpointname_tmp);
        }
        else
            coap_set_header_observe(transaction->message, target->counter++);

        transaction->callback = prv_handleNotifyReply;
        notify_userdata = (notify_userdata_t *)cis_malloc(sizeof(notify_userdata_t));
        memset(notify_userdata, 0, sizeof(notify_userdata_t));
        if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
        {
            notify_userdata->ackID = target->lastMid;
        }
        else
        {
            notify_userdata->ackID = notify->ackID;
        }
        notify_userdata->context = (void *) context;
        transaction->userData = (void *) notify_userdata;
        transaction->server = context->server;
        context->transactionList = (st_transaction_t *)CIS_LIST_ADD(context->transactionList, transaction);

        if (transaction_send(context, transaction, raiflag))
        {
            ret = COAP_NO_ERROR;
            if (context->cloud_platform == CLOUD_PLATFORM_COMMON)
            {
#if LWM2M_COMMON_VER
                at_str = xy_malloc(64);
                if(notify->report_type)
                    snprintf(at_str, 64, "+QLASENDDATA: %d,%d", convert_coap_code_to_commond_result_code(ret),target->lastMid);
                else
                    snprintf(at_str, 64, "+QLAURC: \"report\",%d\r\n\r\n+QLANOTIFY: %d", target->lastMid, convert_coap_code_to_commond_result_code(ret));
                send_urc_to_ext(at_str, strlen(at_str));
                xy_free(at_str);
#endif
            }
            goto out;
        }
    }


/*
    coap_init_message(packet, COAP_TYPE_ACK, COAP_205_CONTENT,0);
    coap_set_header_content_type(packet, formatP);
    coap_set_payload(packet, bufferP, length);
    target->lastTime = utils_gettime_s();
    target->lastMid = context->nextMid++;
    packet->mid = target->lastMid;
    coap_set_header_token(packet, target->token, target->tokenLen);
    coap_set_header_observe(packet, target->counter++);
    ret = packet_send(context, packet);
    */
out:
    cis_free(bufferP);

    return ret;
}


void observe_removeAll(st_context_t* contextP)
{
    while (NULL != contextP->observedList)
    {
        st_observed_t * targetP;

        targetP = contextP->observedList;
        contextP->observedList = contextP->observedList->next;

        if( targetP->parameters != NULL)
        {
            cis_free(targetP->parameters);
        }

        if(targetP->reportData != NULL)
        {
            data_free(targetP->reportDataCount,targetP->reportData);
            targetP->reportData = NULL;
            targetP->reportDataCount = 0;
        }

        cis_free(targetP);
    }
}



coap_status_t observe_handleRequest(st_context_t * contextP,st_uri_t * uriP,
	coap_packet_t * message,coap_packet_t * response)
{
    uint32_t flag;
    bool isObserveFlag;
    bool isObserveRepeat;
    coap_status_t result;
    cis_mid_t observeMid;
    st_object_t * targetObject;

    (void) response;

    if (!CIS_URI_IS_SET_INSTANCE(uriP) && CIS_URI_IS_SET_RESOURCE(uriP)) 
    {
        return COAP_400_BAD_REQUEST;
    }
    if (message->token_len == 0) 
    {
        return COAP_400_BAD_REQUEST;
    }

    targetObject = (st_object_t *)CIS_LIST_FIND(contextP->objectList, uriP->objectId);
    if (NULL == targetObject)
    {
        return COAP_404_NOT_FOUND;
    }
    if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON)
    {
    }
    else
    {
        if (std_object_isStdObject(uriP->objectId))
        {
#if CIS_ENABLE_CMIOT_OTA
            if (uriP->objectId != CIS_POWERUPLOG_OBJECT_ID && uriP->objectId != CIS_CMDHDEFECVALUES_OBJECT_ID)
            {
                coap_packet_t packet[1];
                coap_init_message(packet, COAP_TYPE_ACK, COAP_405_METHOD_NOT_ALLOWED, (uint16_t)message->mid);
                coap_set_header_token(packet, message->token, message->token_len);
                packet_send(contextP, packet, RAI_NULL);
                return COAP_IGNORE;
            }
#else
            coap_packet_t packet[1];
            coap_init_message(packet, COAP_TYPE_ACK, COAP_405_METHOD_NOT_ALLOWED, (uint16_t)message->mid);
            coap_set_header_token(packet, message->token, message->token_len);
            packet_send(contextP, packet, RAI_NULL);
            return COAP_IGNORE;
#endif
        }
    }
    coap_get_header_observe(message, &flag);
    isObserveFlag = (flag == 0 ? true:false);

    if(isObserveFlag)
    {
        isObserveRepeat = packet_asynFindObserveRequest(contextP,message->mid,&observeMid);
		if (isObserveRepeat)
		{
			LOGD("repeat observe request 0x%x", observeMid);
			xy_printf(0,XYAPP, WARN_LOG, "repeat observe request 0x%x", observeMid);
		}
		else
		{
			observeMid = ++contextP->nextObserveNum;
			observeMid = (observeMid << 16) | message->mid;
			LOGD("new observe request 0x%x", observeMid);
		}
 #if CIS_ENABLE_CMIOT_OTA
		if((uriP->objectId == CIS_POWERUPLOG_OBJECT_ID||uriP->objectId == CIS_CMDHDEFECVALUES_OBJECT_ID)&&(uriP->instanceId == 0))
		{
			st_request_t* request;
			request = (st_request_t *)cis_malloc(sizeof(st_request_t));
			request->format = utils_convertMediaType(message->content_type);
			request->mid = message->mid;

			cis_memcpy(request->token,message->token,message->token_len);
			request->tokenLen = message->token_len;
			request->type = CALLBACK_TYPE_OBSERVE;
			request->uri = *uriP;
			result = observe_asynAckNodata(contextP,request,CIS_COAP_205_CONTENT, RAI_NULL);

			cis_free(request);
			result = COAP_IGNORE;
			if(uriP->objectId == CIS_CMDHDEFECVALUES_OBJECT_ID)
				cmiot_ota_observe_mid = message->mid;
			else
				std_poweruplog_ota_notify(contextP,&uriP->objectId,message->mid);
			
			return result;
		}
#endif       

	    if(g_cis_downstream_cb == NULL)
	    {
#if LWM2M_COMMON_VER
            if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->is_auto_ack == 1)
            {
                st_request_t *request;
                request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                request->format = utils_convertMediaType(message->content_type);
                request->mid = message->mid;

                cis_memcpy(request->token, message->token, message->token_len);
                request->tokenLen = message->token_len;
                request->type = CALLBACK_TYPE_OBSERVE;
                request->uri = *uriP;
                request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                cis_free(request);
                result = COAP_IGNORE;
            }
            else
#endif
            {
#if LWM2M_COMMON_VER
                if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
                {
                    st_request_t *request;
                    request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                    request->format = utils_convertMediaType(message->content_type);
                    request->mid = message->mid;

                    cis_memcpy(request->token, message->token, message->token_len);
                    request->tokenLen = message->token_len;
                    request->type = CALLBACK_TYPE_OBSERVE;
                    request->uri = *uriP;
                    request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                    result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                    cis_free(request);
                }
                else
#endif
                    packet_asynPushRequest(contextP, message, CALLBACK_TYPE_OBSERVE, observeMid);
                if(contextP->callback.onObserve(contextP, uriP, true, observeMid) != COAP_206_CONFORM)
                {
                    st_request_t *request_del = NULL;
                    packet_asynRemoveRequest(contextP, observeMid, &request_del);
                }
                result = COAP_IGNORE;
            }
        }
        else
        {
            if (!isObserveRepeat)
            {
#if LWM2M_COMMON_VER
                if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->is_auto_ack == 1)
                {
                    st_request_t *request;
                    request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                    request->format = utils_convertMediaType(message->content_type);
                    request->mid = message->mid;

                    cis_memcpy(request->token, message->token, message->token_len);
                    request->tokenLen = message->token_len;
                    request->type = CALLBACK_TYPE_OBSERVE;
                    request->uri = *uriP;
                    request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                    result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                    cis_free(request);
                    result = COAP_IGNORE;
                }
                else
#endif
                {
#if LWM2M_COMMON_VER
                    if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
                    {
                        st_request_t *request;
                        request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                        request->format = utils_convertMediaType(message->content_type);
                        request->mid = message->mid;

                        cis_memcpy(request->token, message->token, message->token_len);
                        request->tokenLen = message->token_len;
                        request->type = CALLBACK_TYPE_OBSERVE;
                        request->uri = *uriP;
                        request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                        result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                        cis_free(request);
                    }
                    else
#endif
                    {
                        packet_asynPushRequest(contextP, message, CALLBACK_TYPE_OBSERVE, observeMid);
                        g_cis_downstream_cb(CALLBACK_TYPE_OBSERVE, contextP, uriP, message->mid, -1,
                                            cis_data_type_undefine, NULL, 0);
                    }
                }
            }
            result = COAP_IGNORE;
	    }

    }
    else if(!isObserveFlag)
    {
#if CIS_ENABLE_CMIOT_OTA
		if((uriP->objectId == CIS_POWERUPLOG_OBJECT_ID||uriP->objectId == CIS_CMDHDEFECVALUES_OBJECT_ID)&&(uriP->instanceId == 0))
		{
			st_request_t* request;
			request = (st_request_t *)cis_malloc(sizeof(st_request_t));
			request->format = utils_convertMediaType(message->content_type);
			request->mid = message->mid;
		
			cis_memcpy(request->token,message->token,message->token_len);
			request->tokenLen = message->token_len;
			request->type = CALLBACK_TYPE_OBSERVE_CANCEL;
			request->uri = *uriP;
			result = observe_asynAckNodata(contextP,request,CIS_COAP_205_CONTENT, RAI_NULL);
		
			cis_free(request);
			result = COAP_IGNORE;
			return result;
		}
#endif
        st_observed_t *observedP;
        observedP = prv_findObserved(contextP, uriP);
        if (observedP == NULL)
        {
            return COAP_404_NOT_FOUND;
        }

        if(g_cis_downstream_cb == NULL)
        {
#if LWM2M_COMMON_VER
            if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->is_auto_ack == 1)
            {
                st_request_t *request;
                request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                request->format = utils_convertMediaType(message->content_type);
                request->mid = message->mid;

                cis_memcpy(request->token, message->token, message->token_len);
                request->tokenLen = message->token_len;
                request->type = CALLBACK_TYPE_OBSERVE_CANCEL;
                request->uri = *uriP;
                request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                cis_free(request);
                result = COAP_IGNORE;
            }
            else
#endif
            {
#if LWM2M_COMMON_VER
                if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
                {
                    st_request_t *request;
                    request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                    request->format = utils_convertMediaType(message->content_type);
                    request->mid = message->mid;

                    cis_memcpy(request->token, message->token, message->token_len);
                    request->tokenLen = message->token_len;
                    request->type = CALLBACK_TYPE_OBSERVE_CANCEL;
                    request->uri = *uriP;
                    request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                    result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                    cis_free(request);
                }
                else
#endif
                    packet_asynPushRequest(contextP, message, CALLBACK_TYPE_OBSERVE_CANCEL, message->mid);

                if(contextP->callback.onObserve(contextP, uriP, false, message->mid)!= COAP_206_CONFORM)
                {
                    st_request_t *request_del = NULL;
                    packet_asynRemoveRequest(contextP, message->mid, &request_del);
                }
                result = COAP_IGNORE;
            }
        }
        else
        {
#if LWM2M_COMMON_VER
            if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->is_auto_ack == 1)
            {
                st_request_t *request;
                request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                request->format = utils_convertMediaType(message->content_type);
                request->mid = message->mid;

                cis_memcpy(request->token, message->token, message->token_len);
                request->tokenLen = message->token_len;
                request->type = CALLBACK_TYPE_OBSERVE_CANCEL;
                request->uri = *uriP;
                request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                cis_free(request);
                result = COAP_IGNORE;
            }
            else
#endif
            {
#if LWM2M_COMMON_VER
                if (contextP->cloud_platform == CLOUD_PLATFORM_COMMON && g_lwm2m_common_config_data->access_mode == 1)
                {
                    st_request_t *request;
                    request = (st_request_t *)cis_malloc(sizeof(st_request_t));
                    request->format = utils_convertMediaType(message->content_type);
                    request->mid = message->mid;

                    cis_memcpy(request->token, message->token, message->token_len);
                    request->tokenLen = message->token_len;
                    request->type = CALLBACK_TYPE_OBSERVE_CANCEL;
                    request->uri = *uriP;
                    request->accept_type = message->accept_num > 0 ? message->accept[0] : -1;
                    result = observe_asynAckNodata(contextP, request, CIS_COAP_205_CONTENT, RAI_NULL);

                    cis_free(request);
                }
                else
#endif
                {
                    packet_asynPushRequest(contextP, message, CALLBACK_TYPE_OBSERVE_CANCEL, message->mid);
                    g_cis_downstream_cb(CALLBACK_TYPE_OBSERVE_CANCEL, contextP, uriP, message->mid, -1,
                                        cis_data_type_undefine, NULL, 0);
                }
            }
            result = COAP_IGNORE;
        }
    }else{
        return COAP_400_BAD_REQUEST;
    }

    return result;
}


//NOTE: if mid==LWM2M_MAX_ID then find observed_obj by uriP and remove it from list
//     else find observed_obj which lastMid=mid and remove it from list
void observe_cancel(st_context_t * contextP,
                    uint16_t mid, 
                    st_uri_t* uriP)
{
	st_observed_t * observedP;

	LOGD("observe_cancel mid: %d", mid);

	if( mid == MESSAGEID_INVALID)
	{
		observedP = prv_findObserved(contextP, uriP);
		if( observedP != NULL)
		{
			prv_unlinkObserved(contextP, observedP);
            if(observedP->reportData != NULL)
            {
                data_free(observedP->reportDataCount,observedP->reportData);
                observedP->reportData = NULL;
                observedP->reportDataCount = 0;
            }
			cis_free(observedP);
		}
		return;
	}


	for (observedP = contextP->observedList;
		observedP != NULL;
		observedP = observedP->next)
	{
		if ( observedP->lastMid == mid)
		{
			prv_unlinkObserved(contextP, observedP);
            if(observedP->reportData != NULL)
            {
                data_free(observedP->reportDataCount,observedP->reportData);
                observedP->reportData = NULL;
                observedP->reportDataCount = 0;
            }
			cis_free(observedP);
			return;
		}
	}

	return;
}

coap_status_t observe_setParameters(st_context_t * contextP,
	st_uri_t * uriP,st_observe_attr_t * attrP,coap_packet_t* message)
{
	cis_coapret_t result;

	LOGD("toSet: %08X, toClear: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
		attrP->toSet, attrP->toClear, attrP->minPeriod, attrP->maxPeriod, attrP->greaterThan, attrP->lessThan, attrP->step);
    CIS_LOG_URI("observe set",uriP);

	if (!CIS_URI_IS_SET_INSTANCE(uriP) && CIS_URI_IS_SET_RESOURCE(uriP)) 
	{
		return COAP_400_BAD_REQUEST;
	}
    
	st_observed_t *targetP = prv_getObserved(contextP, uriP);
	if (targetP == NULL) 
	{
		return COAP_500_INTERNAL_SERVER_ERROR;
	}
    
	// Check rule “lt” value + 2*”stp” values < “gt” value
	if ((((attrP->toSet | (targetP->parameters?targetP->parameters->toSet:0)) & ~attrP->toClear) & ATTR_FLAG_NUMERIC) == ATTR_FLAG_NUMERIC)
	{
		float gt;
		float lt;
		float stp;

		if (0 != (attrP->toSet & ATTR_FLAG_GREATER_THAN))
		{
			gt = (float)attrP->greaterThan;
		}
		else
		{
			gt = (float)targetP->parameters->greaterThan;
		}
		if (0 != (attrP->toSet & ATTR_FLAG_LESS_THAN))
		{
			lt = (float)attrP->lessThan;
		}
		else
		{
			lt = (float)targetP->parameters->lessThan;
		}
		if (0 != (attrP->toSet & ATTR_FLAG_STEP))
		{
			stp = (float)attrP->step;
		}
		else
		{
			stp = (float)targetP->parameters->step;
		}

		if (lt + (2 * stp) >= gt) 
		{
			return COAP_400_BAD_REQUEST;
		}
	}

	if (targetP->parameters == NULL)
	{
		if (attrP->toSet != 0)
		{
			targetP->parameters = (st_observe_attr_t *)cis_malloc(sizeof(st_observe_attr_t));
			if (targetP->parameters == NULL) 
			{
				return COAP_500_INTERNAL_SERVER_ERROR;
			}
			cis_memcpy(targetP->parameters, attrP, sizeof(st_observe_attr_t));
		}
	}
	else
	{
		targetP->parameters->toSet &= ~attrP->toClear;
		if (attrP->toSet & ATTR_FLAG_MIN_PERIOD)
		{
			targetP->parameters->minPeriod = attrP->minPeriod;
		}
		if (attrP->toSet & ATTR_FLAG_MAX_PERIOD)
		{
			targetP->parameters->maxPeriod = attrP->maxPeriod;
		}
		if (attrP->toSet & ATTR_FLAG_GREATER_THAN)
		{
			targetP->parameters->greaterThan = attrP->greaterThan;
		}
		if (attrP->toSet & ATTR_FLAG_LESS_THAN)
		{
			targetP->parameters->lessThan = attrP->lessThan;
		}
		if (attrP->toSet & ATTR_FLAG_STEP)
		{
			targetP->parameters->step = attrP->step;
		}
	}

	LOGD("Final toSet: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
		targetP->parameters->toSet, targetP->parameters->minPeriod, targetP->parameters->maxPeriod, targetP->parameters->greaterThan, targetP->parameters->lessThan, targetP->parameters->step);

    if(contextP->callback.onSetParams == NULL){
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if(g_cis_downstream_cb == NULL)
    {
        result = contextP->callback.onSetParams(contextP,uriP,*attrP,message->mid);
        if (result == COAP_206_CONFORM)
        {
            packet_asynPushRequest(contextP,message,CALLBACK_TYPE_OBSERVE_PARAMS,message->mid);
            result = COAP_IGNORE;
        }
    }
    else
    {
        packet_asynPushRequest(contextP,message,CALLBACK_TYPE_OBSERVE_PARAMS,message->mid);
        g_cis_downstream_cb(CALLBACK_TYPE_OBSERVE_PARAMS, contextP, uriP, message->mid, -1,
			cis_data_type_undefine, (char*)message->uri_query->data, strlen((char*)message->uri_query->data));	
        result = COAP_IGNORE;
    }

	return result;
}

st_observed_t * observe_findByUri(st_context_t * contextP,
	st_uri_t * uriP)
{
	return prv_findObserved( contextP, uriP);
}

st_observed_t * observe_findByMsgid(st_context_t * contextP, cis_mid_t mid)
{
    st_observed_t * targetP;
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->msgid == mid)
        {
            return targetP;
        }
        targetP = targetP->next;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
//private
static st_observed_t * prv_findObserved(st_context_t * contextP,st_uri_t * uriP)
{
    st_observed_t * targetP;

    //
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if ((!CIS_URI_IS_SET_INSTANCE(uriP) && !CIS_URI_IS_SET_INSTANCE(&(targetP->uri)))
                || (CIS_URI_IS_SET_INSTANCE(uriP) && CIS_URI_IS_SET_INSTANCE(&(targetP->uri)) && (uriP->instanceId == targetP->uri.instanceId)))
            {
                if((CIS_URI_IS_SET_RESOURCE(uriP) && CIS_URI_IS_SET_RESOURCE(&(targetP->uri))) && (uriP->resourceId == targetP->uri.resourceId)
                    || (!CIS_URI_IS_SET_RESOURCE(uriP) && !CIS_URI_IS_SET_RESOURCE(&(targetP->uri))))
                {
                    return targetP;
                }
            }
        }
        targetP = targetP->next;
    }

    //LOGD("Found nothing");
    return NULL;
}

static void prv_unlinkObserved(st_context_t * contextP,st_observed_t * observedP)
{
    LOGD("observe unlink mid:0x%x",observedP->msgid);
    CIS_LOG_URI("observe unlink",&observedP->uri);
    
    if (contextP->observedList == observedP)
    {
        contextP->observedList = contextP->observedList->next;
    }
    else
    {
        st_observed_t * parentP;

        parentP = contextP->observedList;
        while (parentP->next != NULL
            && parentP->next != observedP)
        {
            parentP = parentP->next;
        }
        if (parentP->next != NULL)
        {
            parentP->next = parentP->next->next;
        }
    }
}


st_observed_t * prv_getObserved(st_context_t * contextP,st_uri_t * uriP)
{
    st_observed_t * observedP;

    observedP = prv_findObserved(contextP, uriP);
    if (observedP == NULL)
    {
        observedP = (st_observed_t *)cis_malloc(sizeof(st_observed_t));
        if (observedP == NULL) 
        {
            return NULL;
        }
        cis_memset(observedP, 0, sizeof(st_observed_t));
        cis_memcpy(&(observedP->uri), uriP, sizeof(st_uri_t));
        observedP->actived = false;
        observedP->format = LWM2M_CONTENT_TEXT;
        contextP->observedList = (st_observed_t *)CIS_LIST_ADD(contextP->observedList,observedP);

        CIS_LOG_URI("observe new",uriP);
    }
    return observedP;
}
