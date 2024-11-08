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

#include <stdio.h>
#include "ctlw_internals.h"
#include "ctlw_platform.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_sdk_internals.h"




extern int32_t ctiot_observe_mutex_init(void);
extern int32_t ctiot_observe_mutex_lock(void);
extern int32_t ctiot_observe_mutex_unlock(void);
extern int32_t ctiot_observe_mutex_destroy(void);

#ifdef LWM2M_CLIENT_MODE
static lwm2m_observed_t * prv_findObserved(lwm2m_context_t * contextP,
                                           lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    targetP = contextP->observedList;
    while (targetP != NULL
        && (targetP->uri.objectId != uriP->objectId
         || targetP->uri.flag != uriP->flag
         || (LWM2M_URI_IS_SET_INSTANCE(uriP) && targetP->uri.instanceId != uriP->instanceId)
         || (LWM2M_URI_IS_SET_RESOURCE(uriP) && targetP->uri.resourceId != uriP->resourceId)))
    {
        targetP = targetP->next;
    }
    return targetP;
}

static void prv_unlinkObserved(lwm2m_context_t * contextP,
                               lwm2m_observed_t * observedP)
{
    if (contextP->observedList == observedP)
    {
        contextP->observedList = contextP->observedList->next;
    }
    else
    {
        lwm2m_observed_t * parentP;

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

static lwm2m_watcher_t * prv_findWatcher(lwm2m_observed_t * observedP,
                                         lwm2m_server_t * serverP)
{
    lwm2m_watcher_t * targetP;

    targetP = observedP->watcherList;
    while (targetP != NULL
        && targetP->server != serverP)
    {
        targetP = targetP->next;
    }

    return targetP;
}

static lwm2m_watcher_t * prv_getWatcher(lwm2m_context_t * contextP,
                                        lwm2m_uri_t * uriP,
                                        lwm2m_server_t * serverP)
{
    lwm2m_observed_t * observedP;
    bool allocatedObserver;
    lwm2m_watcher_t * watcherP;

    allocatedObserver = false;

    observedP = prv_findObserved(contextP, uriP);
    if (observedP == NULL)
    {
        observedP = (lwm2m_observed_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_observed_t));
        if (observedP == NULL) return NULL;
        allocatedObserver = true;
        memset(observedP, 0, sizeof(lwm2m_observed_t));
        memcpy(&(observedP->uri), uriP, sizeof(lwm2m_uri_t));
        observedP->next = contextP->observedList;
        contextP->observedList = observedP;
    }

    watcherP = prv_findWatcher(observedP, serverP);
    if (watcherP == NULL)
    {
        watcherP = (lwm2m_watcher_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_watcher_t));
        if (watcherP == NULL)
        {
            if (allocatedObserver == true)
            {
                ctlw_lwm2m_free(observedP);
            }
            return NULL;
        }
        memset(watcherP, 0, sizeof(lwm2m_watcher_t));
        watcherP->active = false;
        watcherP->server = serverP;
        watcherP->next = observedP->watcherList;
        observedP->watcherList = watcherP;
    }

    return watcherP;
}
//----add----
static void prv_obj_observe_notify(lwm2m_uri_t * uriP, uint32_t count)
{
	ctiot_context_t* pContext = ctiot_get_context();

	char uriStr[20] = {0};

	if(ctiot_uri_to_string(uriP, (uint8_t *)uriStr, 20, NULL) != -1)
	{
		if(count == 0)
		{
			ctiot_publish_sdk_notification(uriStr, CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LWEVENT, 0, 0, 0, NULL);
		}
		else
		{
			ctiot_publish_sdk_notification(uriStr, CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_LWEVENT, 0, 1, 0, NULL);
		}
	}
}
//-----------

uint8_t ctlw_observe_handleRequest(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              int size,
                              lwm2m_data_t * dataP,
                              ctlw_coap_packet_t * message,
                              ctlw_coap_packet_t * response)
{
    lwm2m_observed_t * observedP;
    lwm2m_watcher_t * watcherP;
    uint32_t count;
	//----add----
	ctiot_context_t* pTmpContext=ctiot_get_context();
	//---------
    LOG_ARG("Code: %02X, server status: %s", message->code, STR_STATUS(serverP->status));
    LOG_URI(uriP);

    ctlw_coap_get_header_observe(message, &count);
	//----add----
    bool writeDataflag = false;
	bool changed=false;

#ifdef PLATFORM_XINYI
    bool needNotify = false;//observe 上报放到写session文件到flash之后，解决bug9688上报后立刻NRB,flash未写结束，跨会话reboot模式失败
#endif
	//----------
    switch (count)
    {
    case 0:
        if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;
        if (message->token_len == 0) return COAP_400_BAD_REQUEST;
		//----add----
		ctiot_observe_mutex_lock();
		//----------
        watcherP = prv_getWatcher(contextP, uriP, serverP);
        if (watcherP == NULL)
		{
			//----add----
			ctiot_observe_mutex_unlock();
			//----------
			return COAP_500_INTERNAL_SERVER_ERROR;
        }
		//----add----
		if( watcherP->active == false)
		{
#ifdef PLATFORM_XINYI
			writeDataflag = true;
            needNotify = true;//observe 上报放到写session文件到flash之后，解决bug9688上报后立刻NRB,flash未写结束，跨会话reboot模式失败
#else
            prv_obj_observe_notify(uriP, count);
			writeDataflag = true;
#endif
		}

		if(watcherP->tokenLen == message->token_len)
		{
			if(memcmp(watcherP->token,message->token,watcherP->tokenLen)!=0)
			{
				changed = true;
			}
		}
		else
		{
			changed = true;
		}
		//----------

        watcherP->tokenLen = message->token_len;
        memcpy(watcherP->token, message->token, message->token_len);
        watcherP->active = true;

        watcherP->lastTime = ctlw_lwm2m_gettime();
        watcherP->lastMid = response->mid;
        if (IS_OPTION(message, COAP_OPTION_ACCEPT))
        {
            watcherP->format = ctlw_utils_convertMediaType((ctlw_coap_content_type_t)message->accept[0]);
        }
        else
        {
            watcherP->format = LWM2M_CONTENT_TLV;
        }
        if (LWM2M_URI_IS_SET_RESOURCE(uriP))
        {
			//----add----
        	if(uriP->objectId!=19)
        	{
			//---------
	            switch (dataP->type)
	            {
	            case LWM2M_TYPE_INTEGER:
	                if (1 != ctlw_lwm2m_data_decode_int(dataP, &(watcherP->lastValue.asInteger))) return COAP_500_INTERNAL_SERVER_ERROR;
	                break;
	            case LWM2M_TYPE_FLOAT:
	                if (1 != ctlw_lwm2m_data_decode_float(dataP, &(watcherP->lastValue.asFloat))) return COAP_500_INTERNAL_SERVER_ERROR;
	                break;
	            default:
	                break;
	            }
        	}
        }
        ctlw_coap_set_header_observe(response, watcherP->counter++);
		//----add----

		if(writeDataflag || changed)
		{
#ifdef PLATFORM_XINYI
            ctiot_update_sdataflash_needed(uriP->objectId == 19?true:false);
#else
		    ctiot_update_sdataflash_needed();
#endif
		}

#ifdef PLATFORM_XINYI
        if(needNotify == true)
        {
            prv_obj_observe_notify(uriP, count);//observe 上报放到写session文件到flash之后，解决bug9688上报后立刻NRB,flash未写结束，跨会话reboot模式失败
        }
#endif

		ctiot_observe_mutex_unlock();
		//----------
        return COAP_205_CONTENT;

    case 1:
        // cancellation
        observedP = prv_findObserved(contextP, uriP);
        if (observedP)
        {
            watcherP = prv_findWatcher(observedP, serverP);
            if (watcherP)
            {
                ctlw_observe_cancel(contextP, watcherP->lastMid, serverP->sessionH);
				//----add----
				prv_obj_observe_notify(uriP, count);
#ifdef PLATFORM_XINYI
                ctiot_update_sdataflash_needed(uriP->objectId == 19?true:false);
#else
				ctiot_update_sdataflash_needed();
#endif
				//----------
            }

        }
        return COAP_205_CONTENT;

    default:
        return COAP_400_BAD_REQUEST;
    }
}

void ctlw_observe_cancel(lwm2m_context_t * contextP,
                    uint16_t mid,
                    void * fromSessionH)
{
    lwm2m_observed_t * observedP;

    LOG_ARG("mid: %d", mid);
	//----add----
	ctiot_observe_mutex_lock();
	//---------
    for (observedP = contextP->observedList;
         observedP != NULL;
         observedP = observedP->next)
    {
        lwm2m_watcher_t * targetP = NULL;

        if (observedP->watcherList->lastMid == mid
         && ctlw_lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
        {
            targetP = observedP->watcherList;
            observedP->watcherList = observedP->watcherList->next;
        }
        else
        {
            lwm2m_watcher_t * parentP;

            parentP = observedP->watcherList;
            while (parentP->next != NULL
                && (parentP->next->lastMid != mid
                 || !ctlw_lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
            {
                parentP = parentP->next;
            }
            if (parentP->next != NULL)
            {
                targetP = parentP->next;
                parentP->next = parentP->next->next;
            }
        }
        if (targetP != NULL)
        {

            if (targetP->parameters != NULL) ctlw_lwm2m_free(targetP->parameters);
            ctlw_lwm2m_free(targetP);

            if (observedP->watcherList == NULL)
            {
                prv_unlinkObserved(contextP, observedP);
                ctlw_lwm2m_free(observedP);
            }
			//----add----
			ctiot_observe_mutex_unlock();
			//----------
            return;
        }
    }
	ctiot_observe_mutex_unlock();
}
//----add----
void ctlw_observe_cancel_on_token(lwm2m_context_t * contextP,
                    uint8_t* token,
                    void * fromSessionH)
{
	lwm2m_observed_t * observedP;

	ctiot_observe_mutex_lock();
    for (observedP = contextP->observedList;
         observedP != NULL;
         observedP = observedP->next)
    {
    	lwm2m_watcher_t * targetP = NULL;

        if (memcmp(observedP->watcherList->token,token,observedP->watcherList->tokenLen)==0
         && ctlw_lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
        {
            targetP = observedP->watcherList;
            observedP->watcherList = observedP->watcherList->next;
        }
        else
        {
            lwm2m_watcher_t * parentP;

            parentP = observedP->watcherList;
            while (parentP->next != NULL
                && (memcmp(parentP->next->token ,token,parentP->next->tokenLen)==0
                 || !ctlw_lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
            {
                parentP = parentP->next;
            }
            if (parentP->next != NULL)
            {
                targetP = parentP->next;
                parentP->next = parentP->next->next;
            }
        }
        if (targetP != NULL)
        {
			if(observedP != NULL )
			{
				ctiot_context_t * pContext = ctiot_get_context();
#ifdef PLATFORM_XINYI
                ctiot_update_sdataflash_needed(true);
#else
                ctiot_update_sdataflash_needed();
#endif
				
			}

            if (targetP->parameters != NULL) ctlw_lwm2m_free(targetP->parameters);
            ctlw_lwm2m_free(targetP);

            if (observedP->watcherList == NULL)
            {
                prv_unlinkObserved(contextP, observedP);
                ctlw_lwm2m_free(observedP);
            }
			ctiot_observe_mutex_unlock();
            return;
        }
    }
	ctiot_observe_mutex_unlock();
}

bool ctlw_observe_cancel_non_reset(lwm2m_context_t * contextP,
					uint16_t mid,
					void * fromSessionH)
{
	bool ret = false;
	lwm2m_observed_t * observedP;
	ctiot_context_t* pContext = ctiot_get_context();
	ctiot_observe_mutex_lock();
	for (observedP = contextP->observedList;
		 observedP != NULL;
		 observedP = observedP->next)
	{
		lwm2m_watcher_t * targetP = NULL;
		lwm2m_watcher_t * parentP = NULL;

		if (observedP->watcherList->lastMid == mid
		 && ctlw_lwm2m_session_is_equal(observedP->watcherList->server->sessionH, fromSessionH, contextP->userData))
		{
			targetP = observedP->watcherList;
			//observedP->watcherList = observedP->watcherList->next;
		}
		else
		{
			parentP = observedP->watcherList;
			while (parentP->next != NULL
				&& (parentP->next->lastMid != mid
				 || !ctlw_lwm2m_session_is_equal(parentP->next->server->sessionH, fromSessionH, contextP->userData)))
			{
				parentP = parentP->next;
			}
			if (parentP->next != NULL)
			{
				targetP = parentP->next;
				//parentP->next = parentP->next->next;
			}
		}
		if (targetP != NULL)
		{
			ret = true;
			if(observedP != NULL )
			{
				lwm2m_uri_t *uriP = &(observedP->uri);
				//19/0/0 做特殊处理，不cancel
				if(uriP->objectId == 19 && (LWM2M_URI_IS_SET_INSTANCE(uriP) && uriP->instanceId == 0)&& (LWM2M_URI_IS_SET_RESOURCE(uriP) && uriP->resourceId == 0) )
				{
					if(pContext)
					{
						ctiot_change_msg_status(pContext->upMsgList, mid, CTIOT_RST_ERROR);
					}
				}
				else
				{
					//从observe关系中摘链
					if (parentP != NULL)
					{
						parentP->next = parentP->next->next;
					}
					else
					{
						observedP->watcherList = observedP->watcherList->next;
					}

					if (targetP->parameters != NULL) ctlw_lwm2m_free(targetP->parameters);
					ctlw_lwm2m_free(targetP);

					if (observedP->watcherList == NULL)
					{
						prv_unlinkObserved(contextP, observedP);
						ctlw_lwm2m_free(observedP);
					}

					if(pContext)
					{
						ctiot_change_msg_status(pContext->upMsgList, mid, CTIOT_RST_ERROR);
					}
				}
				char uriStr[20] = {0}; //最大uri "/65535/65535/65535"
				ctiot_uri_to_string(uriP, (uint8_t *)uriStr, 20, NULL);
				ctiot_publish_sdk_notification(uriStr, CTIOT_APP_NOTIFICATION, CTIOT_NOTIFY_ASYNC_NOTICE, CTIOT_NOTIFY_SUBTYPE_SEND, CTIOT_RST_ERROR, mid, 0, NULL);
			}
			break;
		}
	}
	ctiot_observe_mutex_unlock();
	return ret;
}
//---------
void ctlw_observe_clear(lwm2m_context_t * contextP,
                   lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * observedP;

    LOG_URI(uriP);
	//----add----
	ctiot_observe_mutex_lock();
	//--------
    observedP = contextP->observedList;
    while(observedP != NULL)
    {
        if (observedP->uri.objectId == uriP->objectId
            && (LWM2M_URI_IS_SET_INSTANCE(uriP) == false
                || observedP->uri.instanceId == uriP->instanceId))
        {
            lwm2m_observed_t * nextP;
            lwm2m_watcher_t * watcherP;

            nextP = observedP->next;

            for (watcherP = observedP->watcherList; watcherP != NULL; watcherP = watcherP->next)
            {
                if (watcherP->parameters != NULL) ctlw_lwm2m_free(watcherP->parameters);
            }
            LWM2M_LIST_FREE(observedP->watcherList);

            prv_unlinkObserved(contextP, observedP);
            ctlw_lwm2m_free(observedP);

            observedP = nextP;
        }
        else
        {
            observedP = observedP->next;
        }
    }
	//----add----
	ctiot_observe_mutex_unlock();
	//----------
}

uint8_t ctlw_observe_setParameters(lwm2m_context_t * contextP,
                              lwm2m_uri_t * uriP,
                              lwm2m_server_t * serverP,
                              lwm2m_attributes_t * attrP)
{
    uint8_t result;
    lwm2m_watcher_t * watcherP;

    LOG_URI(uriP);
    LOG_ARG("toSet: %08X, toClear: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
            attrP->toSet, attrP->toClear, attrP->minPeriod, attrP->maxPeriod, attrP->greaterThan, attrP->lessThan, attrP->step);
	//----add----
	ctiot_context_t* pTmpContext=ctiot_get_context();
	//----------

    if (!LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)) return COAP_400_BAD_REQUEST;

    result = ctlw_object_checkReadable(contextP, uriP, attrP);
    if (COAP_205_CONTENT != result) return result;

    watcherP = prv_getWatcher(contextP, uriP, serverP);
    if (watcherP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;

    // Check rule “lt” value + 2*”stp” values < “gt” value
    if ((((attrP->toSet | (watcherP->parameters?watcherP->parameters->toSet:0)) & ~attrP->toClear) & ATTR_FLAG_NUMERIC) == ATTR_FLAG_NUMERIC)
    {
        float gt;
        float lt;
        float stp;

        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN))
        {
            gt = attrP->greaterThan;
        }
        else
        {
            gt = watcherP->parameters->greaterThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN))
        {
            lt = attrP->lessThan;
        }
        else
        {
            lt = watcherP->parameters->lessThan;
        }
        if (0 != (attrP->toSet & LWM2M_ATTR_FLAG_STEP))
        {
            stp = attrP->step;
        }
        else
        {
            stp = watcherP->parameters->step;
        }

        //if (lt + (2 * stp) >= gt) return COAP_400_BAD_REQUEST;
        if (gt + (2 * stp) >= lt) return COAP_400_BAD_REQUEST;//----modified----
    }

    if (watcherP->parameters == NULL)
    {
        if (attrP->toSet != 0)
        {
            watcherP->parameters = (lwm2m_attributes_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_attributes_t));
            if (watcherP->parameters == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
            memcpy(watcherP->parameters, attrP, sizeof(lwm2m_attributes_t));
        }
    }
    else
    {
        watcherP->parameters->toSet &= ~attrP->toClear;
        if (attrP->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD)
        {
            watcherP->parameters->minPeriod = attrP->minPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD)
        {
            watcherP->parameters->maxPeriod = attrP->maxPeriod;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_GREATER_THAN)
        {
            watcherP->parameters->greaterThan = attrP->greaterThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_LESS_THAN)
        {
            watcherP->parameters->lessThan = attrP->lessThan;
        }
        if (attrP->toSet & LWM2M_ATTR_FLAG_STEP)
        {
            watcherP->parameters->step = attrP->step;
        }
    }

    LOG_ARG("Final toSet: %08X, minPeriod: %d, maxPeriod: %d, greaterThan: %f, lessThan: %f, step: %f",
            watcherP->parameters->toSet, watcherP->parameters->minPeriod, watcherP->parameters->maxPeriod, watcherP->parameters->greaterThan, watcherP->parameters->lessThan, watcherP->parameters->step);

	//----add----
	if(uriP->objectId==19 && LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(uriP)&& uriP->instanceId == 0 && uriP->resourceId == 0)
	{
#ifdef PLATFORM_XINYI
		ctiot_update_sdataflash_needed(true);
#else
        ctiot_update_sdataflash_needed();
#endif
	}
	//----------
    return COAP_204_CHANGED;
}

lwm2m_observed_t * ctlw_observe_findByUri(lwm2m_context_t * contextP,
                                     lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_URI(uriP);
	//----add----
	ctiot_observe_mutex_lock();
	//----------
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if ((!LWM2M_URI_IS_SET_INSTANCE(uriP) && !LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)))
             || (LWM2M_URI_IS_SET_INSTANCE(uriP) && LWM2M_URI_IS_SET_INSTANCE(&(targetP->uri)) && (uriP->instanceId == targetP->uri.instanceId)))
             {
                 if ((!LWM2M_URI_IS_SET_RESOURCE(uriP) && !LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)))
                     || (LWM2M_URI_IS_SET_RESOURCE(uriP) && LWM2M_URI_IS_SET_RESOURCE(&(targetP->uri)) && (uriP->resourceId == targetP->uri.resourceId)))
                 {
                     LOG_ARG("Found one with%s observers.", targetP->watcherList ? "" : " no");
                     LOG_URI(&(targetP->uri));
					 //----add----
					 ctiot_observe_mutex_unlock();
					 //--------
                     return targetP;
                 }
             }
        }
        targetP = targetP->next;
    }
	//----add----
	ctiot_observe_mutex_unlock()
	//---------
    LOG("Found nothing");
    return NULL;
}

void ctlw_lwm2m_resource_value_changed(lwm2m_context_t * contextP,
                                  lwm2m_uri_t * uriP)
{
    lwm2m_observed_t * targetP;

    LOG_URI(uriP);
	//----add----
	ctiot_observe_mutex_lock();
	//---------
    targetP = contextP->observedList;
    while (targetP != NULL)
    {
        if (targetP->uri.objectId == uriP->objectId)
        {
            if (!LWM2M_URI_IS_SET_INSTANCE(uriP)
             || (targetP->uri.flag & LWM2M_URI_FLAG_INSTANCE_ID) == 0
             || uriP->instanceId == targetP->uri.instanceId)
            {
                if (!LWM2M_URI_IS_SET_RESOURCE(uriP)
                 || (targetP->uri.flag & LWM2M_URI_FLAG_RESOURCE_ID) == 0
                 || uriP->resourceId == targetP->uri.resourceId)
                {
                    lwm2m_watcher_t * watcherP;

                    LOG("Found an observation");
                    LOG_URI(&(targetP->uri));

                    for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
                    {
                        if (watcherP->active == true)
                        {
                            LOG("Tagging a watcher");
                            watcherP->update = true;
                        }
                    }
                }
            }
        }
        targetP = targetP->next;
    }
	//----add----
	ctiot_observe_mutex_unlock();
	//---------
}

void ctlw_observe_step(lwm2m_context_t * contextP,
                  time_t currentTime,
                  time_t * timeoutP)
{
    lwm2m_observed_t * targetP;

    LOG("Entering");
	//----add----
	ctiot_observe_mutex_lock();
	//----------
    for (targetP = contextP->observedList ; targetP != NULL ; targetP = targetP->next)
    {
        lwm2m_watcher_t * watcherP;
        uint8_t * buffer = NULL;
        size_t length = 0;
        lwm2m_data_t * dataP = NULL;
        int size = 0;
        double floatValue = 0;
        int64_t integerValue = 0;
        bool storeValue = false;
        ctlw_coap_packet_t message[1];
        time_t interval;

        LOG_URI(&(targetP->uri));
        if (LWM2M_URI_IS_SET_RESOURCE(&targetP->uri))
        {
            lwm2m_uri_t *uriP = &targetP->uri;
			//----add----
            if(uriP->objectId == 19 || (uriP->objectId == 3 && !(uriP->instanceId == 0 && uriP->resourceId == 3)))
			{
				continue;
			}
			//-----------
            else if (COAP_205_CONTENT != ctlw_object_readData(contextP, &targetP->uri, &size, &dataP))
			{
                continue;
			}
            switch (dataP->type)
            {
            case LWM2M_TYPE_INTEGER:
                if (1 != ctlw_lwm2m_data_decode_int(dataP, &integerValue))
                {
                    ctlw_lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            case LWM2M_TYPE_FLOAT:
                if (1 != ctlw_lwm2m_data_decode_float(dataP, &floatValue))
                {
                    ctlw_lwm2m_data_free(size, dataP);
                    continue;
                }
                storeValue = true;
                break;
            default:
                break;
            }
        }
        for (watcherP = targetP->watcherList ; watcherP != NULL ; watcherP = watcherP->next)
        {
            if (watcherP->active == true)
            {
                bool notify = false;
                if (watcherP->update == true)
                {
                    // value changed, should we notify the server ?

                    if (watcherP->parameters == NULL || watcherP->parameters->toSet == 0)
                    {
                        // no conditions
                        notify = true;
                        LOG("Notify with no conditions");
                        LOG_URI(&(targetP->uri));
                    }


                    if (notify == false
                     && watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & ATTR_FLAG_NUMERIC) != 0)
                    {

                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_LESS_THAN) != 0)
                        {
                            LOG("Checking lower threshold");
                            // Did we cross the lower threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue <= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->lessThan)
                                 || (integerValue >= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->lessThan))
                                {
                                    LOG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue <= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->lessThan)
                                 || (floatValue >= watcherP->parameters->lessThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->lessThan))
                                {
                                    LOG("Notify on lower threshold crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_GREATER_THAN) != 0)
                        {
                            LOG("Checking upper threshold");
                            // Did we cross the upper threshold ?
                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                                if ((integerValue <= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger > watcherP->parameters->greaterThan)
                                 || (integerValue >= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asInteger < watcherP->parameters->greaterThan))
                                {
                                    LOG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            case LWM2M_TYPE_FLOAT:
                                if ((floatValue <= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat > watcherP->parameters->greaterThan)
                                 || (floatValue >= watcherP->parameters->greaterThan
                                  && watcherP->lastValue.asFloat < watcherP->parameters->greaterThan))
                                {
                                    LOG("Notify on lower upper crossing");
                                    notify = true;
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        if ((watcherP->parameters->toSet & LWM2M_ATTR_FLAG_STEP) != 0)
                        {
                            LOG("Checking step");

                            switch (dataP->type)
                            {
                            case LWM2M_TYPE_INTEGER:
                            {
                                int64_t diff;

                                diff = integerValue - watcherP->lastValue.asInteger;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            case LWM2M_TYPE_FLOAT:
                            {
                                double diff;

                                diff = floatValue - watcherP->lastValue.asFloat;
                                if ((diff < 0 && (0 - diff) >= watcherP->parameters->step)
                                 || (diff >= 0 && diff >= watcherP->parameters->step))
                                {
                                    LOG("Notify on step condition");
                                    notify = true;
                                }
                            }
                                break;
                            default:
                                break;
                            }
                        }
                    }

                    if (watcherP->parameters != NULL
                     && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MIN_PERIOD) != 0)
                    {
                        LOG_ARG("Checking minimal period (%d s)", watcherP->parameters->minPeriod);

                        if (watcherP->lastTime + watcherP->parameters->minPeriod > currentTime)
                        {
                            // Minimum Period did not elapse yet
                            interval = watcherP->lastTime + watcherP->parameters->minPeriod - currentTime;
                            if (*timeoutP > interval) *timeoutP = interval;
                            notify = false;
                        }
                        else
                        {
                            LOG("Notify on minimal period");
                            notify = true;
                        }
                    }
               }

                // Is the Maximum Period reached ?
                if (notify == false
                 && watcherP->parameters != NULL
                 && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    LOG_ARG("Checking maximal period (%d s)", watcherP->parameters->maxPeriod);

                    if (watcherP->lastTime + watcherP->parameters->maxPeriod <= currentTime)
                    {
                        LOG("Notify on maximal period");
                        notify = true;
                    }
                }

                if (notify == true)
                {
                    if (buffer == NULL)
                    {
                        if (dataP != NULL)
                        {
                            int res;

                            res = ctlw_lwm2m_data_serialize(&targetP->uri, size, dataP, &(watcherP->format), &buffer);
                            if (res < 0)
                            {
                                break;
                            }
                            else
                            {
                                length = (size_t)res;
                            }

                        }
                        else
                        {
                            if (COAP_205_CONTENT != ctlw_object_read(contextP, &targetP->uri, &(watcherP->format), &buffer, &length))
                            {
                                buffer = NULL;
                                break;
                            }
                        }
                        ctlw_coap_init_message(message, COAP_TYPE_NON, COAP_205_CONTENT, 0);
                        ctlw_coap_set_header_content_type(message, watcherP->format);
                        ctlw_coap_set_payload(message, buffer, length);
                    }
                    watcherP->lastTime = currentTime;
                    watcherP->lastMid = ctlw_lwm2m_get_next_mid(contextP);/*contextP->nextMID++*/;
                    message->mid = watcherP->lastMid;
                    ctlw_coap_set_header_token(message, watcherP->token, watcherP->tokenLen);
                    ctlw_coap_set_header_observe(message, watcherP->counter++);
                    ctlw_message_send(contextP, message, watcherP->server->sessionH);
                    watcherP->update = false;
                }

                // Store this value
                if (notify == true && storeValue == true)
                {
                    switch (dataP->type)
                    {
                    case LWM2M_TYPE_INTEGER:
                        watcherP->lastValue.asInteger = integerValue;
                        break;
                    case LWM2M_TYPE_FLOAT:
                        watcherP->lastValue.asFloat = floatValue;
                        break;
                    default:
                        break;
                    }
                }

                if (watcherP->parameters != NULL && (watcherP->parameters->toSet & LWM2M_ATTR_FLAG_MAX_PERIOD) != 0)
                {
                    // update timers
                    interval = watcherP->lastTime + watcherP->parameters->maxPeriod - currentTime;
                    if (*timeoutP > interval) *timeoutP = interval;
                }
            }
        }
        if (dataP != NULL) ctlw_lwm2m_data_free(size, dataP);
        if (buffer != NULL) ctlw_lwm2m_free(buffer);
    }
	//----add----
	ctiot_observe_mutex_unlock();
	//----------
}

#endif

