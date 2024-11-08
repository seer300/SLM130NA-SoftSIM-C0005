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
 *    Julien Vermillard - initial implementation
 *    Fabien Fleutot - Please refer to git log
 *    David Navarro, Intel Corporation - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Gregory Lemercier - Please refer to git log
 *
 *******************************************************************************/

/*
 * This object is single instance only, and provide firmware upgrade functionality.
 * Object ID is 5.
 */

/*
 * resources:
 * 0 package                   write
 * 1 package url               write
 * 2 update                    exec
 * 3 state                     read
 * 5 update result             read
 * 6 package name              read
 * 7 package version           read
 * 8 update protocol support   read
 * 9 update delivery method    read
 */

#include "ctlw_config.h"

#ifdef WITH_FOTA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ctlw_liblwm2m.h"
#include "ctlw_fota.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctwing_util.h"
#include "xy_fota.h"
#include "ctlw_NV_data.h"

// ---- private object "Firmware" specific defines ----
// Resource Id's:
#define RES_M_PACKAGE 0
#define RES_M_PACKAGE_URI 1
#define RES_M_UPDATE 2
#define RES_M_STATE 3
#define RES_M_UPDATE_RESULT 5
#define RES_O_PKG_NAME 6
#define RES_O_PKG_VERSION 7
#define RES_O_UPDATE_PROTOCOL 8
#define RES_M_UPDATE_METHOD 9

#define LWM2M_FIRMWARE_PROTOCOL_NUM 4
#define LWM2M_FIRMWARE_PROTOCOL_NULL ((uint8_t)-1)

typedef struct
{
	uint8_t state;
	uint8_t result;
	uint16_t shortID;
	char pkg_name[256];
	char pkg_version[256];
	uint8_t protocol_support[LWM2M_FIRMWARE_PROTOCOL_NUM];
	uint8_t delivery_method;
} firmware_data_t;

extern ctiotFotaManage fotaStateManage;

/***********************************************************************/
typedef enum
{
	FIRMWARE_INITIAL,
	FIRMWARE_UPDATE_SUCCESS,
	FIRMWARE_NOT_ENOUGH_MEMORY,
	FIRMWARE_NOT_ENOUGH_RAM,
	FIRMWARE_CONNECT_INTERRUPT,
	FIRMWARE_PACKET_CHECK_FAIL,
	FIRMWARE_PACKET_TYPE_NOT_SUPPORT,
	FIRMWARE_INVAILD_URI,
	FIRMWARE_UPDATE_FAIL,
	FIRMWARE_PROTOCOL_NOT_SUPPORT
} firmwareUpdateResult;

firmwareWritePara firmwareWriteParameter = {0, 0};
thread_handle_t fota_tid;

static void *firmware_read(uint8_t resourceID)
{
	switch (resourceID)
	{
	case 1:
		if (firmwareWriteParameter.packageUri == NULL)
			return NULL;
		else
			return firmwareWriteParameter.packageUri;

	case 3:
	{
		//0:idle(下载前或升级完成后)1: Downloading(正在下载)2: Downloaded(已下载)3: Updating(正在升级)
		int state = fotaStateManage.fotaState;
		int *ptrState = malloc(sizeof(state));
		*ptrState = state;
		return ptrState;
	}

	case 5:
	{
		int updateResult = fotaStateManage.fotaResult;
		int *ptrUpdateResult = malloc(sizeof(updateResult));
		*ptrUpdateResult = updateResult;
		return ptrUpdateResult;
	}

	case 6:
	{
		char *packetName = "lwm2m packet1";
		return ctlw_lwm2m_strdup(packetName);
	}
	case 7:
	{
		char *packetVersion = CTIOT_SDK_VERSION;
		return ctlw_lwm2m_strdup(packetVersion);
	}

	case 8: /*Multiple*/
	{
		//0:coap 1: coaps 2: http1.1 3: https1.1
		int ptotocolVersion[2] = {0, 1};
		int *ptrPtotocolVersion = malloc(sizeof(ptotocolVersion));
		ptrPtotocolVersion[0] = ptotocolVersion[0];
		ptrPtotocolVersion[1] = ptotocolVersion[1];
		return ptrPtotocolVersion;
	}

	case 9:
	{
		//0:pull only 1: push only 2:both
		int deliveryMethod = 0;
		int *ptrDeliveryMethod = malloc(sizeof(deliveryMethod));
		*ptrDeliveryMethod = deliveryMethod;
		return ptrDeliveryMethod;
	}

	default:
		printf("firmware read resourceID error\n");
		break;
	}
	return NULL;
}

static uint8_t firmware_write(uint8_t resourceID, firmwareWritePara firmwareWriteData)
{
	switch (resourceID)
	{
	case 0:
	{
		printf("firmwareWriteData.packageLength is %d\n", firmwareWriteData.packageLength);
		if (firmwareWriteParameter.package == NULL)
		{
			firmwareWriteParameter.package = malloc(firmwareWriteData.packageLength + 1);
			if (firmwareWriteParameter.package == NULL)
			{
				perror("no enough memory!");
			}

			memset(firmwareWriteParameter.package, 0, firmwareWriteData.packageLength + 1);
			strcpy(firmwareWriteParameter.package, firmwareWriteData.package);
			printf("firmwareWriteParameter.package is %s\n", firmwareWriteParameter.package);
		}
		else
		{
			free(firmwareWriteParameter.package);
			firmwareWriteParameter.package = malloc(firmwareWriteData.packageLength + 1);
			if (firmwareWriteParameter.package == NULL)
			{
				perror("no enough memory!");
			}

			memset(firmwareWriteParameter.package, 0, firmwareWriteData.packageLength + 1);
			strcpy(firmwareWriteParameter.package, firmwareWriteData.package);
			printf("firmwareWriteParameter.package is %s\n", firmwareWriteParameter.package);
		}
	}
	break;
	case 1:
	{
		printf("firmwareWriteData.packageUriLength is %d\n", firmwareWriteData.packageUriLength);
		if (firmwareWriteParameter.packageUri == NULL)
		{
			firmwareWriteParameter.packageUri = malloc(firmwareWriteData.packageUriLength + 1);
			if (firmwareWriteParameter.packageUri == NULL)
			{
				perror("no enough memory!");
			}
			memset(firmwareWriteParameter.packageUri, 0, firmwareWriteData.packageUriLength + 1);
			strcpy(firmwareWriteParameter.packageUri, firmwareWriteData.packageUri);
			printf("firmwareWriteParameter.packageUri is %s\n", firmwareWriteParameter.packageUri);
		}
		else
		{
			free(firmwareWriteParameter.packageUri);
			firmwareWriteParameter.packageUri = malloc(firmwareWriteData.packageUriLength + 1);
			if (firmwareWriteParameter.packageUri == NULL)
			{
				perror("no enough memory!");
			}
			memset(firmwareWriteParameter.packageUri, 0, firmwareWriteData.packageUriLength + 1);
			strcpy(firmwareWriteParameter.packageUri, firmwareWriteData.packageUri);
			printf("firmwareWriteParameter.packageUri is %s\n", firmwareWriteParameter.packageUri);
		}
	}
	break;
	default:
		printf("firmware write resourceID error\n");
		break;
	}
	return 0;
}

extern bool g_ctlw_fota_is_no_done;
static void *exec_fota(void *arg)
{
	int res = 1;
	ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"begine fota exec...\r\n");
	ctlw_usleep(10 * 1000 * 1000);

    //FOTA升级记录保存
    ctiot_context_t* pContext = ctiot_get_context();
    
	g_ctlw_fota_is_no_done = true;
	pContext->fotaFlag = 1;
    
	c2f_encode_params(pContext);
	
	c2f_encode_context(pContext,true);

    //重启开始FOTA升级
	OTA_upgrade_start();
    
	thread_exit(NULL);
	return NULL;
}

static uint8_t firmware_execute(uint8_t resourceID)
{
	switch (resourceID)
	{
	case 2:
		{
			ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"firmware updating ...\n");
			int err = 0;

			osThreadAttr_t thread_attr = {0};
			thread_attr.name	   = "ctlw_fota_exec";
			thread_attr.priority   = osPriorityNormal1;
			thread_attr.stack_size = osStackShared;

			err = thread_create(&fota_tid, &thread_attr, exec_fota, NULL);
			if (err != 0)
			{
				ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"can't create thread: %d\n", err);
			}
		}
		break;
	default:
		ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"firmware execute resourceID error\n");
		break;
	}
	return 0;
}

/***********************************************************************/

static uint8_t prv_firmware_read(uint16_t instanceId,
								 int *numDataP,
								 lwm2m_data_t **dataArrayP,
								 ctlw_lwm2m_object_t *objectP)
{
	int i;
	uint8_t result;
	firmware_data_t *data = (firmware_data_t *)(objectP->userData);
	// this is a single instance object
	if (instanceId != 0)
	{
		return COAP_404_NOT_FOUND;
	}
	// is the server asking for the full object ?
	if (*numDataP == 0)
	{
		*dataArrayP = ctlw_lwm2m_data_new(6);
		if (*dataArrayP == NULL)
			return COAP_500_INTERNAL_SERVER_ERROR;
		*numDataP = 6;
		(*dataArrayP)[0].id = 3;
		(*dataArrayP)[1].id = 5;
		(*dataArrayP)[2].id = 6;
		(*dataArrayP)[3].id = 7;
		(*dataArrayP)[4].id = 8;
		(*dataArrayP)[5].id = 9;
	}

	i = 0;
	do
	{
		switch ((*dataArrayP)[i].id)
		{
		case RES_M_PACKAGE:
		case RES_M_UPDATE:
			result = COAP_405_METHOD_NOT_ALLOWED;
			break;

		case RES_M_PACKAGE_URI:
		{
			char *packetUri;
			packetUri = (char *)firmware_read(1);
			ctlw_lwm2m_data_encode_string(packetUri, *dataArrayP + i);
			free(packetUri);
			result = COAP_205_CONTENT;
			break;
		}

		case RES_M_STATE:
		{
			// firmware update state (int)
			int *updateState;
			updateState = (int *)firmware_read(3);
			ctlw_lwm2m_data_encode_int(*updateState, *dataArrayP + i);
			//ctlw_lwm2m_data_encode_int(fotaStateManagement.fotaState, *dataArrayP + i);
			free(updateState);
			result = COAP_205_CONTENT;
			break;
		}

		case RES_M_UPDATE_RESULT:
		{
			int *updateResult;
			updateResult = (int *)firmware_read(5);
			ctlw_lwm2m_data_encode_int(*updateResult, *dataArrayP + i);
			//ctlw_lwm2m_data_encode_int(fotaStateManagement.fotaResult, *dataArrayP + i);
#ifdef PLATFORM_XINYI
			xy_ctlw_fota_notify(fotaStateManage.fotaState, FOTA_RESULT_OVER);
			ctiot_context_t * pContext = ctiot_get_context();
			c2f_encode_params(pContext);//将fotaflag标志位的清除写入config文件中
#endif
			free(updateResult);
			result = COAP_205_CONTENT;
			break;
		}

		case RES_O_PKG_NAME:
		{
			char *packetName;
			packetName = (char *)firmware_read(6);
			ctlw_lwm2m_data_encode_string(packetName, *dataArrayP + i);
			free(packetName);
			result = COAP_205_CONTENT;
			break;
		}

		case RES_O_PKG_VERSION:
		{
			char *packetVersion;
			packetVersion = (char *)firmware_read(7);
			ctlw_lwm2m_data_encode_string(packetVersion, *dataArrayP + i);
			free(packetVersion);
			result = COAP_205_CONTENT;
			break;
		}

		case RES_O_UPDATE_PROTOCOL:
		{
			int *updateProtocol;
			int ri;
			int num = 0;
			lwm2m_data_t *subTlvP = NULL;

			updateProtocol = (int *)firmware_read(8);
			num = sizeof(updateProtocol) / sizeof(int);
			if (num)
			{
				subTlvP = ctlw_lwm2m_data_new(num);
				for (ri = 0; ri < num; ri++)
				{
					subTlvP[ri].id = ri;
					ctlw_lwm2m_data_encode_int(updateProtocol[ri], subTlvP + ri);
				}
			}
			else
			{
				/* If no protocol is provided, use CoAP as default (per spec) */
				num = 1;
				subTlvP = ctlw_lwm2m_data_new(num);
				subTlvP[0].id = 0;
				ctlw_lwm2m_data_encode_int(0, subTlvP);
			}
			free(updateProtocol);

			ctlw_lwm2m_data_encode_instances(subTlvP, num, *dataArrayP + i);
			result = COAP_205_CONTENT;
			break;

		}

		case RES_M_UPDATE_METHOD:
		{
			int *deliveryMethod;
			deliveryMethod = (int *)firmware_read(9);
			ctlw_lwm2m_data_encode_int(*deliveryMethod, *dataArrayP + i);
			free(deliveryMethod);
			result = COAP_205_CONTENT;
			break;
		}

		default:
			result = COAP_404_NOT_FOUND;
		}

		i++;
	} while (i < *numDataP && result == COAP_205_CONTENT);

	return result;
}

static uint8_t prv_firmware_write(uint16_t instanceId,
								  int numData,
								  lwm2m_data_t *dataArray,
								  ctlw_lwm2m_object_t *objectP)
{
	int i;
	uint8_t result;
	firmware_data_t * data = (firmware_data_t*)(objectP->userData);
	firmwareWritePara writeData = {0};

	// this is a single instance object
	if (instanceId != 0)
	{
		return COAP_404_NOT_FOUND;
	}

	i = 0;

	do
	{
		switch (dataArray[i].id)
		{
		case RES_M_PACKAGE:
			// inline firmware binary
			writeData.packageLength = dataArray->value.asBuffer.length;
			writeData.package = ctlw_lwm2m_malloc(writeData.packageLength);
			strcpy(writeData.package, (char *)dataArray->value.asBuffer.buffer);
			result = firmware_write(0, writeData);
			ctlw_lwm2m_free(writeData.package);
			result = COAP_204_CHANGED;
			break;

		case RES_M_PACKAGE_URI:
			
			if(fotaStateManage.fotaState == FOTA_STATE_DOWNLOADING)
			{
				result = COAP_204_CHANGED;
				break;
			}
			// URL for download the firmware
			fotaStateManage.fotaState = FOTA_STATE_DOWNLOADING;
			fotaStateManage.fotaResult = FOTA_RESULT_INIT;
			fotaStateManage.notifyUpdatingMsgId = 0;
			ctiot_fota_state_changed();
			writeData.packageUriLength = dataArray->value.asBuffer.length;
			writeData.packageUri = ctlw_lwm2m_malloc(writeData.packageUriLength + 1);
			memset(writeData.packageUri, 0, writeData.packageUriLength + 1);
			strncpy(writeData.packageUri, (char *)dataArray->value.asBuffer.buffer, writeData.packageUriLength);
			result = firmware_write(1, writeData);
			ctlw_lwm2m_free(writeData.packageUri);
			result = COAP_204_CHANGED;
			break;

		default:
			result = COAP_405_METHOD_NOT_ALLOWED;
		}

		i++;
	} while (i < numData && result == COAP_204_CHANGED);

	return result;
}

static uint8_t prv_firmware_execute(uint16_t instanceId,
									uint16_t resourceId,
									uint8_t *buffer,
									int length,
									ctlw_lwm2m_object_t *objectP)
{
	firmware_data_t * data = (firmware_data_t*)(objectP->userData);

	// this is a single instance object
	if (instanceId != 0)
	{
		return COAP_404_NOT_FOUND;
	}

	if (length != 0)
		return COAP_400_BAD_REQUEST;

	// for execute callback, resId is always set.
	switch (resourceId)
	{
	case RES_M_UPDATE:
		if (fotaStateManage.fotaState != FOTA_STATE_DOWNLOADED)
		{
			printf("firmware package not ready\n");
			return COAP_400_BAD_REQUEST;
		}
#ifndef PLATFORM_XINYI
		fprintf(stdout, "\n\t FIRMWARE UPDATE\r\n\n");
#endif
		fotaStateManage.fotaState = FOTA_STATE_UPDATING;
		fotaStateManage.fotaResult = FOTA_RESULT_INIT;
		ctiot_fota_state_changed();
		firmware_execute(2);
        return COAP_204_CHANGED;
	default:
		return COAP_405_METHOD_NOT_ALLOWED;
	}
}

static uint8_t prv_firmware_discover(uint16_t instanceId,
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

	result = COAP_205_CONTENT;

	// is the server asking for the full object ?
	if (*numDataP == 0)
	{
		uint16_t resList[] = {
			RES_M_PACKAGE,
			RES_M_PACKAGE_URI,
			RES_M_UPDATE,
			RES_M_STATE,
			RES_M_UPDATE_RESULT,
			RES_O_PKG_NAME,
			RES_O_PKG_VERSION,
			RES_O_UPDATE_PROTOCOL,
			RES_M_UPDATE_METHOD};
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
			case RES_M_PACKAGE:
			case RES_M_PACKAGE_URI:
			case RES_M_UPDATE:
			case RES_M_STATE:
			case RES_M_UPDATE_RESULT:
			case RES_O_PKG_NAME:
			case RES_O_PKG_VERSION:
			case RES_O_UPDATE_PROTOCOL:
			case RES_M_UPDATE_METHOD:
				break;
			default:
				result = COAP_404_NOT_FOUND;
			}
		}
	}

	return result;
}

static uint8_t prv_firmware_delete(uint16_t id,
								   ctlw_lwm2m_object_t *objectP)
{
	firmware_data_t *targetP;

	objectP->instanceList = ctlw_lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
	if (NULL == targetP)
		return COAP_404_NOT_FOUND;

	ctlw_lwm2m_free(targetP);

	return COAP_202_DELETED;
}

static uint8_t prv_firmware_create(uint16_t instanceId,
								   int numData,
								   lwm2m_data_t *dataArray,
								   ctlw_lwm2m_object_t *objectP)
{
	firmware_data_t *targetP;
	uint8_t result;

	targetP = (firmware_data_t *)ctlw_lwm2m_malloc(sizeof(firmware_data_t));
	if (NULL == targetP)
		return COAP_500_INTERNAL_SERVER_ERROR;
	memset(targetP, 0, sizeof(firmware_data_t));

	targetP->shortID = instanceId;
	objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

	result = prv_firmware_write(instanceId, numData, dataArray, objectP);

	if (result != COAP_204_CHANGED)
	{
		(void)prv_firmware_delete(instanceId, objectP);
	}
	else
	{
		result = COAP_201_CREATED;
	}

	return result;
}

void ctlw_display_firmware_object(ctlw_lwm2m_object_t *object)
{
	firmware_data_t *data = (firmware_data_t *)object->userData;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"  /%u: Firmware object:\r\n", object->objID);
	if (NULL != data)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"    state: %u, result: %u\r\n", data->state,
					 data->result);
	}
}

ctlw_lwm2m_object_t *ctlw_get_object_firmware(void)
{
	/*
     * The ctlw_get_object_firmware function create the object itself and return a pointer to the structure that represent it.
     */
	ctlw_lwm2m_object_t *firmwareObj;
	firmwareObj = (ctlw_lwm2m_object_t *)ctlw_lwm2m_malloc(sizeof(ctlw_lwm2m_object_t));
	if (NULL != firmwareObj)
	{
		memset(firmwareObj, 0, sizeof(ctlw_lwm2m_object_t));

		/*
         * It assigns its unique ID
         * The 5 is the standard ID for the optional object "Object firmware".
         */
		firmwareObj->objID = LWM2M_FIRMWARE_UPDATE_OBJECT_ID;

		/*
         * and its unique instance
         *
         */
		//ctiot_context_t *pContext = ctiot_get_context();
		//if (pContext->fotaFlag == 1)
		//{
			firmwareObj->instanceList = (lwm2m_list_t *)ctlw_lwm2m_malloc(sizeof(lwm2m_list_t));
			if (NULL != firmwareObj->instanceList)
			{
				memset(firmwareObj->instanceList, 0, sizeof(lwm2m_list_t));
			}
			else
			{
				ctlw_lwm2m_free(firmwareObj);
				return NULL;
			}
		//}

		/*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
		firmwareObj->createFunc = prv_firmware_create;
		firmwareObj->deleteFunc = prv_firmware_delete;
		firmwareObj->readFunc = prv_firmware_read;
		firmwareObj->writeFunc = prv_firmware_write;
		firmwareObj->executeFunc = prv_firmware_execute;
		firmwareObj->discoverFunc = prv_firmware_discover;
		firmwareObj->userData = ctlw_lwm2m_malloc(sizeof(firmware_data_t));
		/*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
		if (NULL != firmwareObj->userData)
		{
			firmware_data_t *data = (firmware_data_t *)(firmwareObj->userData);

			data->state = FOTA_STATE_IDIL;
			data->result = FOTA_RESULT_INIT;
			strcpy(data->pkg_name, "lwm2mclient");
			strcpy(data->pkg_version, "1.0");

			/* Only support CoAP based protocols */
			data->protocol_support[0] = 0;
			data->protocol_support[1] = 1;
			data->protocol_support[2] = LWM2M_FIRMWARE_PROTOCOL_NULL;
			data->protocol_support[3] = LWM2M_FIRMWARE_PROTOCOL_NULL;

			/* Only support push method */
			data->delivery_method = 1;
		}
		else
		{
			ctlw_lwm2m_free(firmwareObj);
			firmwareObj = NULL;
		}
	}
    lwm2m_printf("ctlw_get_object_firmware\r\n");
	return firmwareObj;
}

void ctlw_free_object_firmware(ctlw_lwm2m_object_t *objectP)
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

#endif
