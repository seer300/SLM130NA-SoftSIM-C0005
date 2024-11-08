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
 *    Axel Lorente - Please refer to git log
 *    Achim Kraus, Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Ville Skyttä - Please refer to git log
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

#include "ctlw_user_objects.h"
#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2mclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PRV_TLV_BUFFER_SIZE 64

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
    struct _prv_instance_ * next;   // matches lwm2m_list_t::next
    uint16_t shortID;               // matches lwm2m_list_t::id
    float  minMeasuredValue;//5601
    float  maxMeasuredValue;//5602
    float  minRangeValue;//5603
    float  maxRangeValue;//5604
    float  sensorValue;//5700
	uint16_t sensorUnits;//5701
	uint16_t testVal;
} prv_instance_t;

static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        lwm2m_data_t ** dataArrayP,
                        ctlw_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int i;

    targetP = (prv_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;
	printf("----------------prv_read-------------------------\r\n");
    if (*numDataP == 0)
    {
        *dataArrayP = ctlw_lwm2m_data_new(6);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 6;
        (*dataArrayP)[0].id = 5601;
		(*dataArrayP)[1].id = 5602;
		(*dataArrayP)[2].id = 5603;
		(*dataArrayP)[3].id = 5604;
        (*dataArrayP)[4].id = 5700;
		(*dataArrayP)[5].id = 5701;
    }

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case 5601:
			printf("resource 5601\r\n");
            ctlw_lwm2m_data_encode_float(targetP->minMeasuredValue, *dataArrayP + i);
            break;
		case 5602:
			printf("resource 5602\r\n");
            ctlw_lwm2m_data_encode_float(targetP->maxMeasuredValue, *dataArrayP + i);
            break;
		case 5603:
			printf("resource 5603\r\n");
            ctlw_lwm2m_data_encode_float(targetP->minRangeValue, *dataArrayP + i);
            break;
		case 5604:
			printf("resource 5604\r\n");
            ctlw_lwm2m_data_encode_float(targetP->maxRangeValue, *dataArrayP + i);
            break;
		case 5605:
			return COAP_405_METHOD_NOT_ALLOWED;
        case 5700:
			printf("resource 5700\r\n");
            ctlw_lwm2m_data_encode_float(targetP->sensorValue, *dataArrayP + i);
            break;
		case 5701:
			printf("resource 5700\r\n");
            ctlw_lwm2m_data_encode_int(targetP->sensorUnits, *dataArrayP + i);
            break;
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_discover(uint16_t instanceId,
                            int * numDataP,
                            lwm2m_data_t ** dataArrayP,
                            ctlw_lwm2m_object_t * objectP)
{
    int i;

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *dataArrayP = ctlw_lwm2m_data_new(7);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 7;
        (*dataArrayP)[0].id = 5601;
        (*dataArrayP)[1].id = 5602;
        (*dataArrayP)[2].id = 5603;
		(*dataArrayP)[3].id = 5604;
        (*dataArrayP)[4].id = 5605;
		(*dataArrayP)[5].id = 5700;
		(*dataArrayP)[6].id = 5701;
    }
    else
    {
        for (i = 0; i < *numDataP; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case 5601:
            case 5602:
            case 5603:
			case 5604:
            case 5605:
			case 5700:
			case 5701:
                break;
            default:
                return COAP_404_NOT_FOUND;
            }
        }
    }

    return COAP_205_CONTENT;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         lwm2m_data_t * dataArray,
                         ctlw_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int i;

    targetP = (prv_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
        case 5601:
        case 5602:
		case 5603:
        case 5604:
		case 5605:
        case 5701:
            return COAP_405_METHOD_NOT_ALLOWED;
		case 5700:
		{
			double value = 0.0f;

            if (1 != ctlw_lwm2m_data_decode_float(dataArray + i, &value))
            {
                return COAP_400_BAD_REQUEST;
            }
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "value:%lf",value);
			targetP->sensorValue = (float)value;
			ctiot_log_debug(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "targetP->sensorValue:%f",targetP->sensorValue);
			break;
		}
        default:
            return COAP_404_NOT_FOUND;
        }
    }

    return COAP_204_CHANGED;
}

static uint8_t prv_delete(uint16_t id,
                          ctlw_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;

    objectP->instanceList = ctlw_lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    ctlw_lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          lwm2m_data_t * dataArray,
                          ctlw_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    uint8_t result;


    targetP = (prv_instance_t *)ctlw_lwm2m_malloc(sizeof(prv_instance_t));
    if (NULL == targetP) return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(instanceId, numData, dataArray, objectP);

    if (result != COAP_204_CHANGED)
    {
        (void)prv_delete(instanceId, objectP);
    }
    else
    {
        result = COAP_201_CREATED;
    }

    return result;
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t * buffer,
                        int length,
                        ctlw_lwm2m_object_t * objectP)
{

	prv_instance_t * targetP;

    targetP = (prv_instance_t *)ctlw_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case 5601:
	case 5602:
	case 5603:
	case 5604:
	case 5700:
	case 5701:
		return COAP_405_METHOD_NOT_ALLOWED;
	case 5605:
		targetP->maxMeasuredValue=100.0f;
		targetP->minMeasuredValue= -100.0f;
		return COAP_204_CHANGED;
    default:
        return COAP_404_NOT_FOUND;
    }
}

void ctlw_display_test_object(ctlw_lwm2m_object_t * object)
{
#ifdef WITH_LOGS
    fprintf(stdout, "  /%u: Test object, instances:\r\n", object->objID);
    prv_instance_t * instance = (prv_instance_t *)object->instanceList;
    while (instance != NULL)
    {
        fprintf(stdout, "    /%u/%u: shortId: %u\r\n",
                object->objID, instance->shortID,
                instance->shortID);
        instance = (prv_instance_t *)instance->next;
    }
#endif
}

ctlw_lwm2m_object_t * ctlw_get_test_object(void)
{
    ctlw_lwm2m_object_t * testObj;
    testObj = (ctlw_lwm2m_object_t *)ctlw_lwm2m_malloc(sizeof(ctlw_lwm2m_object_t));

    if (NULL != testObj)
    {
        int i;
        prv_instance_t * targetP;

        memset(testObj, 0, sizeof(ctlw_lwm2m_object_t));

        testObj->objID = CTLW_TEST_OBJECT_ID;
        for (i=0 ; i < 3 ; i++)
        {
            targetP = (prv_instance_t *)ctlw_lwm2m_malloc(sizeof(prv_instance_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(prv_instance_t));
            targetP->shortID = 10 + i;
            targetP->minMeasuredValue = -10.0f;
			targetP->maxMeasuredValue = 50.0f;
			targetP->minRangeValue = 30.0f;
			targetP->maxRangeValue = 50.0f;
			targetP ->sensorUnits = 3;
			targetP ->sensorValue = 10.0f + (float)i;
            testObj->instanceList = LWM2M_LIST_ADD(testObj->instanceList, targetP);
        }
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        testObj->readFunc = prv_read;
        testObj->discoverFunc = prv_discover;
        testObj->writeFunc = prv_write;
        testObj->executeFunc = prv_exec;
        testObj->createFunc = prv_create;
        testObj->deleteFunc = prv_delete;
    }

    return testObj;
}

void ctlw_free_test_object(ctlw_lwm2m_object_t * object)
{
    LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        ctlw_lwm2m_free(object->userData);
        object->userData = NULL;
    }
    ctlw_lwm2m_free(object);
}

