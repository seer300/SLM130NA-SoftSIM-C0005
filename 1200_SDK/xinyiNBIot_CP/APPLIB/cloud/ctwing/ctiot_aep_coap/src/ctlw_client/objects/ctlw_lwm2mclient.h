/*******************************************************************************
 *
 * Copyright (c) 2014 Bosch Software Innovations GmbH, Germany.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *
 *******************************************************************************/
/*
 * lwm2mclient.h
 *
 *  General functions of lwm2m test client.
 *
 *  Created on: 22.01.2015
 *  Author: Achim Kraus
 *  Copyright (c) 2015 Bosch Software Innovations GmbH, Germany. All rights reserved.
 */

#ifndef _CTLW_LWM2MCLIENT_H_
#define _CTLW_LWM2MCLIENT_H_


#include "ctlw_config.h"

#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2m_sdk.h"

/*
 * object_device.c
 */
ctlw_lwm2m_object_t *ctlw_get_object_device(void);
void ctlw_free_object_device(ctlw_lwm2m_object_t *objectP);
uint8_t ctlw_device_change(lwm2m_data_t *dataArray, ctlw_lwm2m_object_t *objectP);
void ctlw_display_device_object(ctlw_lwm2m_object_t *objectP);
/*
 * object_firmware.c
 */

#ifdef WITH_FOTA
ctlw_lwm2m_object_t *ctlw_get_object_firmware(void);
void ctlw_free_object_firmware(ctlw_lwm2m_object_t *objectP);
void ctlw_display_firmware_object(ctlw_lwm2m_object_t *objectP);
#endif
/*
 * object_location.c
 */
ctlw_lwm2m_object_t *ctlw_get_object_location(void);
void ctlw_free_object_location(ctlw_lwm2m_object_t *object);
void ctlw_display_location_object(ctlw_lwm2m_object_t *objectP);

/*
 * object_server.c
 */
ctlw_lwm2m_object_t *ctlw_get_server_object(int serverId, const char *binding, int lifetime, bool storing);
void ctlw_clean_server_object(ctlw_lwm2m_object_t *object);
void ctlw_display_server_object(ctlw_lwm2m_object_t *objectP);
void ctlw_copy_server_object(ctlw_lwm2m_object_t *objectDest, ctlw_lwm2m_object_t *objectSrc);

/*
 * object_connectivity_moni.c
 */
ctlw_lwm2m_object_t *ctlw_get_object_conn_m(void);
void ctlw_free_object_conn_m(ctlw_lwm2m_object_t *objectP);
uint8_t ctlw_connectivity_moni_change(lwm2m_data_t *dataArray, ctlw_lwm2m_object_t *objectP);

/*
 * object_connectivity_stat.c
 */
extern ctlw_lwm2m_object_t *ctlw_get_object_conn_s(void);
void ctlw_free_object_conn_s(ctlw_lwm2m_object_t *objectP);
extern void ctlw_conn_s_updateTxStatistic(ctlw_lwm2m_object_t *objectP, uint16_t txDataByte, bool smsBased);
extern void ctlw_conn_s_updateRxStatistic(ctlw_lwm2m_object_t *objectP, uint16_t rxDataByte, bool smsBased);

/*
 * object_access_control.c
 */
ctlw_lwm2m_object_t *ctlw_acc_ctrl_create_object(void);
void ctlw_acl_ctrl_free_object(ctlw_lwm2m_object_t *objectP);
bool ctlw_acc_ctrl_obj_add_inst(ctlw_lwm2m_object_t *accCtrlObjP, uint16_t instId,
                           uint16_t acObjectId, uint16_t acObjInstId, uint16_t acOwner);
bool ctlw_acc_ctrl_oi_add_ac_val(ctlw_lwm2m_object_t *accCtrlObjP, uint16_t instId,
                            uint16_t aclResId, uint16_t acValue);

/*
 * object_security.c
 */
ctlw_lwm2m_object_t *ctlw_get_security_object(int serverId, const char *serverUri, char *bsPskId, char *psk, uint16_t pskLen, bool isBootstrap);
void ctlw_clean_security_object(ctlw_lwm2m_object_t *objectP);
char *ctlw_get_server_uri(ctlw_lwm2m_object_t *objectP, uint16_t secObjInstID);
void ctlw_display_security_object(ctlw_lwm2m_object_t *objectP);
void ctlw_copy_security_object(ctlw_lwm2m_object_t *objectDest, ctlw_lwm2m_object_t *objectSrc);

/*
 *  object_data_report.c
*/
ctlw_lwm2m_object_t *ctlw_get_data_report_object(void);
void ctlw_free_data_report_object(ctlw_lwm2m_object_t *object);

/*
 *	object_custom.c
 */
uint8_t ctlw_find_use_targe(char data, char *targe);
char ctlw_t_strtok(char *in, char *targe, char **args, uint8_t *argc);

#endif /* _CTLW_LWM2MCLIENT_H_ */
