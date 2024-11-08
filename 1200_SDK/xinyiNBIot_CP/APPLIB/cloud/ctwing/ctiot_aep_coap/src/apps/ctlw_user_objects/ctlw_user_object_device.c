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

/*
 * This object is single instance only, and is mandatory to all LWM2M device as it describe the object such as its
 * manufacturer, model, etc...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//linux system call
//#include <unistd.h>//read,write,ctlw_usleep
//#include <time.h>

#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2mclient.h"
#include "ctlw_lwm2m_sdk.h"

//extern int reboot;

#define PRV_MANUFACTURER "Open Mobile Alliance"
#define PRV_MODEL_NUMBER "Lightweight M2M Client"
#define PRV_SERIAL_NUMBER "345000123"
#define PRV_FIRMWARE_VERSION "1.0"
#define PRV_POWER_SOURCE_1 1
#define PRV_POWER_SOURCE_2 5
#define PRV_POWER_VOLTAGE_1 3800
#define PRV_POWER_VOLTAGE_2 5000
#define PRV_POWER_CURRENT_1 125
#define PRV_POWER_CURRENT_2 900
#define PRV_BATTERY_LEVEL 100
#define PRV_MEMORY_FREE 15
#define PRV_ERROR_CODE 0
#define PRV_TIME_ZONE "Europe/Berlin"
#define PRV_BINDING_MODE "U"

#define PRV_OFFSET_MAXLEN 7 //+HH:MM\0 at max
#define PRV_TLV_BUFFER_SIZE 128

// Resource Id's:
#define RES_O_MANUFACTURER 0
#define RES_O_MODEL_NUMBER 1
#define RES_O_SERIAL_NUMBER 2
#define RES_O_FIRMWARE_VERSION 3
#define RES_M_REBOOT 4
#define RES_O_FACTORY_RESET 5
#define RES_O_AVL_POWER_SOURCES 6
#define RES_O_POWER_SOURCE_VOLTAGE 7
#define RES_O_POWER_SOURCE_CURRENT 8
#define RES_O_BATTERY_LEVEL 9
#define RES_O_MEMORY_FREE 10
#define RES_M_ERROR_CODE 11
#define RES_O_RESET_ERROR_CODE 12
#define RES_O_CURRENT_TIME 13
#define RES_O_UTC_OFFSET 14
#define RES_O_TIMEZONE 15
#define RES_M_BINDING_MODES 16
// since TS 20141126-C:
#define RES_O_DEVICE_TYPE 17
#define RES_O_HARDWARE_VERSION 18
#define RES_O_SOFTWARE_VERSION 19
#define RES_O_BATTERY_STATUS 20
#define RES_O_MEMORY_TOTAL 21

typedef struct
{
    int64_t free_memory;
    int64_t error;
    int64_t time;
    uint8_t battery_level;
    char time_offset[PRV_OFFSET_MAXLEN];
} device_data_t;

/**************************************************************************/
typedef struct
{
    int64_t currentTime;
    char utcOffset[7];
    char timeZone[128];
} deviceWritePara;

deviceWritePara deviceWriteParameter = {1367491215, "+08:00", "Beijing/China"};

static int batteryLeft = 100;
static time_t next_change_time = 0;

static uint8_t update_battery()
{
    //fprintf(stderr, "update battery ...\n");
    time_t tv_sec;

    tv_sec = ctlw_lwm2m_gettime();

    if (next_change_time < tv_sec)
    {
        batteryLeft--;
        ctlw_resource_value_changed("/3/0/9");
        next_change_time = tv_sec + 20;
    }
    if (batteryLeft < 1)
        batteryLeft = 0;
    return batteryLeft;
}
static void *device_read(uint8_t resourceID)
{
    switch (resourceID)
    {
    case 0: /*Manufacturer*/
    {
        char *manufacturer = "china telecom shanghai research institute";
        return ctlw_lwm2m_strdup(manufacturer);
    }
    case 1: /*Model Number*/
    {
        char *modelNum = "lwm2m client";
        return ctlw_lwm2m_strdup(modelNum);
    }
    case 2: /*Serial Number*/
    {
        char *serialNum = "20180705";
        return ctlw_lwm2m_strdup(serialNum);
    }
    case 3: /*Firmware Version*/
    {
        char *firmwareVersion = "1.0";
        return ctlw_lwm2m_strdup(firmwareVersion);
    }
    case 6: /*Available Power Sources */
    {
        int powerSources[2] = {1, 5};
        int *ptrPowerSources = malloc(sizeof(powerSources));
        *ptrPowerSources = powerSources[0];
        *(ptrPowerSources + 1) = powerSources[1];
        return ptrPowerSources;
    }
    case 7: /*Power Source Voltage */
    {
        int powerSourcesVoltage[2] = {3000, 6000};
        int *ptrPowerSourcesVoltage = malloc(sizeof(powerSourcesVoltage));
        ptrPowerSourcesVoltage[0] = powerSourcesVoltage[0];
        ptrPowerSourcesVoltage[1] = powerSourcesVoltage[1];
        return ptrPowerSourcesVoltage;
    }
    case 8: /*Power Source Current */
    {
        int powerSourcesCurrent[2] = {299, 899};
        int *ptrPowerSourcesCurrent = malloc(sizeof(powerSourcesCurrent));
        ptrPowerSourcesCurrent[0] = powerSourcesCurrent[0];
        ptrPowerSourcesCurrent[1] = powerSourcesCurrent[1];
        return ptrPowerSourcesCurrent;
    }
    case 9: /* Battery Level */
    {
        uint8_t batteryLevel = update_battery();
        uint8_t *ptrBatteryLevel = malloc(sizeof(batteryLevel));
        *ptrBatteryLevel = batteryLevel;
        return ptrBatteryLevel;
    }
    case 10: /*Memory Free*/
    {
        int memoryFree = 16;
        int *ptrMemoryFree = malloc(sizeof(memoryFree));
        *ptrMemoryFree = memoryFree;
        return ptrMemoryFree;
    }
    case 11: /*Error Code */
    {
        int errorCode[2] = {1, 3};
        int *ptrErrorCode = malloc(sizeof(errorCode));
        ptrErrorCode[0] = errorCode[0];
        ptrErrorCode[1] = errorCode[1];
        return ptrErrorCode;
    }
    case 13: /* Current Time*/
        return &(deviceWriteParameter.currentTime);
    case 14: /*UTC Offset */
        return deviceWriteParameter.utcOffset;
    case 15: /*Timezone */
        return deviceWriteParameter.timeZone;
    case 16: /*Supported Binding and Modes */
    {
        char *bindingMode = "U";
        return ctlw_lwm2m_strdup(bindingMode);
    }
    case 17: /* Device Type */
    {
        char *deviceType = "Device Type";
        return ctlw_lwm2m_strdup(deviceType);
    }
    case 18: /*Hardware Version */
    {
        char *hardwareVersion = "Hardware Version";
        return ctlw_lwm2m_strdup(hardwareVersion);
    }
    case 19: /*Software Version*/
    {
        char *softwareVersion = "Software Version";
        return ctlw_lwm2m_strdup(softwareVersion);
    }
    case 20: /* Battery Status */
    {
        //0：正常,1：充电,2：充电完成,3：故障,4：电量不足,5：未安装,6：信息未知
        int batteryStatus = 0;
        int *ptrBatteryStatus = malloc(sizeof(batteryStatus));
        *ptrBatteryStatus = batteryStatus;
        return ptrBatteryStatus;
    }
    case 21: /*Memory Total */
    {
        int memoryTotal = 128;
        int *ptrMemoryTotal = malloc(sizeof(memoryTotal));
        *ptrMemoryTotal = memoryTotal;
        return ptrMemoryTotal;
    }
    default:
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"object3 no this resource\n");
        break;
    }
    return NULL;
}

static uint8_t prv_set_value(lwm2m_data_t *dataP,
                             device_data_t *devDataP)
{
    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id)
    {
    case RES_O_MANUFACTURER:
    {
        char *manufacturer;
        manufacturer = (char *)device_read(0);
        ctlw_lwm2m_data_encode_string(manufacturer, dataP);
        ctlw_lwm2m_free(manufacturer);
        return COAP_205_CONTENT;
    }
    case RES_O_MODEL_NUMBER:
    {
        char *modelNumber;
        modelNumber = (char *)device_read(1);
        ctlw_lwm2m_data_encode_string(modelNumber, dataP);
        ctlw_lwm2m_free(modelNumber);
        return COAP_205_CONTENT;
    }

    case RES_O_SERIAL_NUMBER:
    {
        char *serialNumber;
        serialNumber = (char *)device_read(2);
        ctlw_lwm2m_data_encode_string(serialNumber, dataP);
        ctlw_lwm2m_free(serialNumber);
        return COAP_205_CONTENT;
    }

    case RES_O_FIRMWARE_VERSION:
    {
        char *firmwareVersion;
        firmwareVersion = (char *)device_read(3);
        ctlw_lwm2m_data_encode_string(firmwareVersion, dataP);
        ctlw_lwm2m_free(firmwareVersion);
        return COAP_205_CONTENT;
    }

    case RES_M_REBOOT:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_FACTORY_RESET:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_AVL_POWER_SOURCES:
    {
        lwm2m_data_t *subTlvP;
        int *powerSources;
        int powerSourcesNum, i;
        powerSources = (int *)device_read(6);
        powerSourcesNum = sizeof(powerSources) / sizeof(int);

        subTlvP = ctlw_lwm2m_data_new(powerSourcesNum);

        for (i = 0; i < powerSourcesNum; i++)
        {
            subTlvP[i].id = i;
            ctlw_lwm2m_data_encode_int(powerSources[i], subTlvP + i);
        }
        /*
	        subTlvP[0].id = 0;
	        ctlw_lwm2m_data_encode_int(PRV_POWER_SOURCE_1, subTlvP);
	        subTlvP[1].id = 1;
	        ctlw_lwm2m_data_encode_int(PRV_POWER_SOURCE_2, subTlvP + 1);
	*/
        ctlw_lwm2m_data_encode_instances(subTlvP, powerSourcesNum, dataP);
        ctlw_lwm2m_free(powerSources);
        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_VOLTAGE:
    {
        lwm2m_data_t *subTlvP;
        int *powerSourceVoltage;
        int powerSourceVoltageNum, i;
        powerSourceVoltage = (int *)device_read(7);
        powerSourceVoltageNum = sizeof(powerSourceVoltage) / sizeof(int);

        subTlvP = ctlw_lwm2m_data_new(powerSourceVoltageNum);

        for (i = 0; i < powerSourceVoltageNum; i++)
        {
            subTlvP[i].id = i;
            ctlw_lwm2m_data_encode_int(powerSourceVoltage[i], subTlvP + i);
        }

        ctlw_lwm2m_data_encode_instances(subTlvP, powerSourceVoltageNum, dataP);
        ctlw_lwm2m_free(powerSourceVoltage);
        return COAP_205_CONTENT;
    }

    case RES_O_POWER_SOURCE_CURRENT:
    {
        lwm2m_data_t *subTlvP;
        int *powerSourceCurrent;
        int powerSourceCurrentNum, i;
        powerSourceCurrent = (int *)device_read(8);
        powerSourceCurrentNum = sizeof(powerSourceCurrent) / sizeof(int);

        subTlvP = ctlw_lwm2m_data_new(powerSourceCurrentNum);

        for (i = 0; i < powerSourceCurrentNum; i++)
        {
            subTlvP[i].id = i;
            ctlw_lwm2m_data_encode_int(powerSourceCurrent[i], subTlvP + i);
        }

        ctlw_lwm2m_data_encode_instances(subTlvP, powerSourceCurrentNum, dataP);
        ctlw_lwm2m_free(powerSourceCurrent);
        return COAP_205_CONTENT;
    }

    case RES_O_BATTERY_LEVEL:
    {
        uint8_t *batteryLevel;
        batteryLevel = (uint8_t *)device_read(9);
        ctlw_lwm2m_data_encode_int(*batteryLevel, dataP);
        //ctlw_lwm2m_data_encode_int(devDataP->battery_level, dataP);
        ctlw_lwm2m_free(batteryLevel);
        return COAP_205_CONTENT;
    }

    case RES_O_MEMORY_FREE:
    {
        int *memoryFree;
        memoryFree = (int *)device_read(10);
        ctlw_lwm2m_data_encode_int(*memoryFree, dataP);
        ctlw_lwm2m_free(memoryFree);
        return COAP_205_CONTENT;
    }

    case RES_M_ERROR_CODE:
    {
        lwm2m_data_t *subTlvP;
        int *errorCode;
        int errorCodeNum, i;
        errorCode = (int *)device_read(11);
        errorCodeNum = sizeof(errorCode) / sizeof(int);

        subTlvP = ctlw_lwm2m_data_new(errorCodeNum);

        for (i = 0; i < errorCodeNum; i++)
        {
            subTlvP[i].id = i;
            ctlw_lwm2m_data_encode_int(errorCode[i], subTlvP + i);
        }

        ctlw_lwm2m_data_encode_instances(subTlvP, errorCodeNum, dataP);
        ctlw_lwm2m_free(errorCode);
        return COAP_205_CONTENT;
    }

    case RES_O_RESET_ERROR_CODE:
        return COAP_405_METHOD_NOT_ALLOWED;

    case RES_O_CURRENT_TIME:
    {
        int64_t *timeCurrent;
        timeCurrent = (int64_t *)device_read(13);
        ctlw_lwm2m_data_encode_int(ctlw_lwm2m_gettime() + *timeCurrent, dataP);
        return COAP_205_CONTENT;
    }

    case RES_O_UTC_OFFSET:
    {
        char *utcOffset;
        utcOffset = (char *)device_read(14);
        ctlw_lwm2m_data_encode_string(utcOffset, dataP);
        return COAP_205_CONTENT;
    }

    case RES_O_TIMEZONE:
    {
        char *oTimeZone;
        oTimeZone = (char *)device_read(15);
        ctlw_lwm2m_data_encode_string(oTimeZone, dataP);
        return COAP_205_CONTENT;
    }

    case RES_M_BINDING_MODES:
    {
        char *mBindingMode;
        mBindingMode = (char *)device_read(16);
        ctlw_lwm2m_data_encode_string(mBindingMode, dataP);
        ctlw_lwm2m_free(mBindingMode);
        return COAP_205_CONTENT;
    }

    case RES_O_DEVICE_TYPE:
    {
        char *deviceType;
        deviceType = (char *)device_read(17);
        ctlw_lwm2m_data_encode_string(deviceType, dataP);
        ctlw_lwm2m_free(deviceType);
        return COAP_205_CONTENT;
    }

    case RES_O_HARDWARE_VERSION:
    {
        char *hardwareVersion;
        hardwareVersion = (char *)device_read(18);
        ctlw_lwm2m_data_encode_string(hardwareVersion, dataP);
        ctlw_lwm2m_free(hardwareVersion);
        return COAP_205_CONTENT;
    }

    case RES_O_SOFTWARE_VERSION:
    {
        char *softwareVersion;
        softwareVersion = (char *)device_read(19);
        ctlw_lwm2m_data_encode_string(softwareVersion, dataP);
        ctlw_lwm2m_free(softwareVersion);
        return COAP_205_CONTENT;
    }

    case RES_O_BATTERY_STATUS:
    {
        int *batteryStatus;
        batteryStatus = (int *)device_read(20);
        ctlw_lwm2m_data_encode_int(*batteryStatus, dataP);
        ctlw_lwm2m_free(batteryStatus);
        return COAP_205_CONTENT;
    }

    case RES_O_MEMORY_TOTAL:
    {
        int *memoryTotal;
        memoryTotal = (int *)device_read(21);
        ctlw_lwm2m_data_encode_int(*memoryTotal, dataP);
        ctlw_lwm2m_free(memoryTotal);
        return COAP_205_CONTENT;
    }

    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_device_read(uint16_t instanceId,
                               int *numDataP,
                               lwm2m_data_t **dataArrayP,
                               ctlw_lwm2m_object_t *objectP)
{
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
            //RES_O_MANUFACTURER,
            //RES_O_MODEL_NUMBER,
            //RES_O_SERIAL_NUMBER,
            RES_O_FIRMWARE_VERSION,
            //E: RES_M_REBOOT,
            //E: RES_O_FACTORY_RESET,
            //RES_O_AVL_POWER_SOURCES,
            //RES_O_POWER_SOURCE_VOLTAGE,
            //RES_O_POWER_SOURCE_CURRENT,
            //RES_O_BATTERY_LEVEL,
            //RES_O_MEMORY_FREE,
            //RES_M_ERROR_CODE,
            //E: RES_O_RESET_ERROR_CODE,
            //RES_O_CURRENT_TIME,
            //RES_O_UTC_OFFSET,
            //RES_O_TIMEZONE,
            //RES_M_BINDING_MODES,
            //RES_O_DEVICE_TYPE,
            //RES_O_HARDWARE_VERSION,
            //RES_O_SOFTWARE_VERSION,
            //RES_O_BATTERY_STATUS,
            //RES_O_MEMORY_TOTAL
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
        result = prv_set_value((*dataArrayP) + i, (device_data_t *)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

void ctlw_display_user_device_object(ctlw_lwm2m_object_t *object)
{
#ifdef WITH_LOGS
    device_data_t *data = (device_data_t *)object->userData;
    ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"  /%u: Device object:\r\n", object->objID);
    if (NULL != data)
    {
        ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"    time: %lld, time_offset: %s\r\n",
                     (long long)data->time, data->time_offset);
    }
#endif
}

ctlw_lwm2m_object_t *ctlw_get_user_object_device(void)
{
    /*
     * The ctlw_get_object_device function create the object itself and return a pointer to the structure that represent it.
     */
    ctlw_lwm2m_object_t *deviceObj;

    deviceObj = (ctlw_lwm2m_object_t *)ctlw_lwm2m_malloc(sizeof(ctlw_lwm2m_object_t));

    if (NULL != deviceObj)
    {
        memset(deviceObj, 0, sizeof(ctlw_lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3 is the standard ID for the mandatory object "Object device".
         */
        deviceObj->objID = LWM2M_DEVICE_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        deviceObj->instanceList = (lwm2m_list_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != deviceObj->instanceList)
        {
            memset(deviceObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            ctlw_lwm2m_free(deviceObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        deviceObj->readFunc = prv_device_read;

        deviceObj->discoverFunc = NULL;
        deviceObj->writeFunc = NULL;
        deviceObj->executeFunc = NULL;

        deviceObj->userData = ctlw_lwm2m_malloc(sizeof(device_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != deviceObj->userData)
        {
            ((device_data_t *)deviceObj->userData)->battery_level = PRV_BATTERY_LEVEL;
            ((device_data_t *)deviceObj->userData)->free_memory = PRV_MEMORY_FREE;
            ((device_data_t *)deviceObj->userData)->error = PRV_ERROR_CODE;
            ((device_data_t *)deviceObj->userData)->time = 1367491215;
            strcpy(((device_data_t *)deviceObj->userData)->time_offset, "+01:00");
        }
        else
        {
            ctlw_lwm2m_free(deviceObj->instanceList);
            ctlw_lwm2m_free(deviceObj);
            deviceObj = NULL;
        }
    }

    return deviceObj;
}

void ctlw_free_user_object_device(ctlw_lwm2m_object_t *objectP)
{
    if (NULL != objectP->userData)
    {
        ctlw_lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList)
    {
        ctlw_lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }

    ctlw_lwm2m_free(objectP);
}

uint8_t ctlw_user_device_change(lwm2m_data_t *dataArray,
                      ctlw_lwm2m_object_t *objectP)
{
    uint8_t result;

    switch (dataArray->id)
    {
    case RES_O_BATTERY_LEVEL:
    {
        int64_t value;
        if (1 == ctlw_lwm2m_data_decode_int(dataArray, &value))
        {
            if ((0 <= value) && (100 >= value))
            {
                ((device_data_t *)(objectP->userData))->battery_level = value;
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
    }
    break;
    case RES_M_ERROR_CODE:
        if (1 == ctlw_lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->error))
        {
            result = COAP_204_CHANGED;
        }
        else
        {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_MEMORY_FREE:
        if (1 == ctlw_lwm2m_data_decode_int(dataArray, &((device_data_t *)(objectP->userData))->free_memory))
        {
            result = COAP_204_CHANGED;
        }
        else
        {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    default:
        result = COAP_405_METHOD_NOT_ALLOWED;
        break;
    }

    return result;
}
