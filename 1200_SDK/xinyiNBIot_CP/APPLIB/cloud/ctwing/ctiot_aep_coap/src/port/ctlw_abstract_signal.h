/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#ifndef __ABSTRACT_SIGNAL_H
#define __ABSTRACT_SIGNAL_H

#include "ctlw_liblwm2m.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif
	typedef void (*PEventCallback)(void);
    typedef struct
    {
        uint16_t epsFlag:1;//0-未订阅，1-已订阅
        uint16_t ipFlag:1;//0-未订阅，1-已订阅
        uint16_t psmStatusFlag:1;//0-未订阅，1-已订阅
        uint16_t serverIPType:4;
		uint16_t sdkIPStatus:4;
        uint16_t chipIpType:5;
		uint16_t psmStatus;
		PEventCallback ipCallback;
		PEventCallback psmCallback;
    }abstract_signal_s;

	typedef enum{
		STATUS_NO_PSMING,
		STATUS_PSMING,
		STATUS_NET_OOS,
	}psm_status_e;
	typedef enum{
		SERVER_IP_TYPE_NONE,
		SERVER_IP_TYPE_V4ONLY,
		SERVER_IP_TYPE_V6ONLY,
		SERVER_IP_TYPE_V4V6,
	}server_ip_type_e;
	typedef enum{
		CUR_SOCKET_TYPE_NONE,
		CUR_SOCKET_TYPE_V4,
		CUR_SOCKET_TYPE_V6,
	}current_socket_type_e;
	typedef enum{
		CHIP_IP_TYPE_FALSE,
		CHIP_IP_TYPE_V4ONLY,
		CHIP_IP_TYPE_V6ONLY,
		CHIP_IP_TYPE_V4V6,
		CHIP_IP_TYPE_V6ONLY_V6PREPARING,
		CHIP_IP_TYPE_V4V6_V6PREPARING,
	}chip_ip_type_e;
	typedef enum{
		SDK_IP_STATUS_FALSE,
		SDK_IP_STATUS_TRUE_V4,
		SDK_IP_STATUS_TRUE_V6,
		SDK_IP_STATUS_DISABLE,
		SDK_IP_STATUS_V6PREPARING,
	}sdk_ip_status_e;
	/*
		向芯片订阅ip状态变化通知
	*/
    uint16_t ctiot_signal_init(void);
	void ctiot_signal_destroy(void);
    uint16_t ctiot_signal_subscribe_ip_event(PEventCallback pEventCallback,server_ip_type_e serverIPType);
    uint16_t ctiot_signal_cancel_ip_event(void);
    uint16_t ctiot_signal_emit_ip_event(chip_ip_type_e chipIPType);
	uint16_t ctiot_get_ip_status(chip_ip_type_e chipIPType);
    uint16_t ctiot_signal_cancel_all(void);
    uint8_t ctiot_signal_is_ip_event_subscribed(void);
	void ctiot_signal_set_chip_ip_type(uint16_t iptype);
	uint16_t ctiot_signal_get_chip_ip_type(void);
	server_ip_type_e ctiot_signal_get_server_ip_type(void);
	sdk_ip_status_e ctiot_signal_get_ip_status(void);
	uint16_t ctiot_signal_get_psm_status(void);
	uint16_t ctiot_signal_set_psm_status(uint16_t status);
	uint16_t ctiot_signal_subscribe_psm_status(PEventCallback pEventCallback);
	uint16_t ctiot_signal_cancel_psm_status(void);
	uint16_t ctiot_signal_emit_psm_status(uint16_t status);
	uint16_t ctiot_signal_is_psm_status_subscribed(void);
#ifdef __cplusplus
}
#endif
#endif //__ABSTRACT_SIGNAL_H
