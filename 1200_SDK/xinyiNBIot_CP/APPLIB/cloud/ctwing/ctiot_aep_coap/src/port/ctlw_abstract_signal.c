/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#include "ctlw_abstract_signal.h"
#include "ctlw_abstract_os.h"
#include "ctlw_liblwm2m.h"

static abstract_signal_s signalItem = { 0 };

uint16_t ctiot_signal_init(void)
{
    ctiot_log_debug(LOG_INIT_MODULE,LOG_IP_CLASS,"initilizing eps/ip event...\r\n");
    uint16_t result = CTIOT_NB_SUCCESS;
	if(ctchip_event_ip_status_init()==-1)
	{
		return CTIOT_SYS_API_ERROR;
	}
	signalItem.chipIpType = ctlw_get_ip_type();
	return result;
}

void ctiot_signal_destroy(void)
{
	ctchip_event_ip_status_destroy();
}

uint16_t ctiot_signal_subscribe_ip_event(PEventCallback pEventCallback,server_ip_type_e serverIPType)
{
    ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"subscribe ip change event...\r\n");
    signalItem.ipFlag = 1;
	signalItem.ipCallback = pEventCallback;
	signalItem.serverIPType = serverIPType;
	ctiot_signal_emit_ip_event((chip_ip_type_e)signalItem.chipIpType);
    return CTIOT_NB_SUCCESS;
}
uint16_t ctiot_signal_cancel_ip_event(void)
{
    ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"cancel ip change event...\r\n");
    signalItem.ipFlag = 0;
	signalItem.ipCallback = NULL;
    return CTIOT_NB_SUCCESS;
}
sdk_ip_status_e ctiot_signal_get_ip_status(void)
{
	return (sdk_ip_status_e)signalItem.sdkIPStatus;
}

uint16_t ctiot_get_ip_status(chip_ip_type_e chipIPType)
{
	switch(signalItem.serverIPType)
	{
		case SERVER_IP_TYPE_V4ONLY:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V4ONLY:
				case CHIP_IP_TYPE_V4V6:
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_TRUE_V4;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY:
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_DISABLE;
					break;
				}
				default:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		case SERVER_IP_TYPE_V6ONLY:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V6ONLY:
				case CHIP_IP_TYPE_V4V6:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_TRUE_V6;
					break;
				}
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_V6PREPARING;
					break;
				}
				case CHIP_IP_TYPE_V4ONLY:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_DISABLE;
					break;
				}
				default:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		case SERVER_IP_TYPE_V4V6:
		{
			switch(chipIPType)
			{
				case CHIP_IP_TYPE_V4ONLY:
				case CHIP_IP_TYPE_V4V6:
				case CHIP_IP_TYPE_V4V6_V6PREPARING:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_TRUE_V4;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_TRUE_V6;
					break;
				}
				case CHIP_IP_TYPE_V6ONLY_V6PREPARING:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_V6PREPARING;
					break;
				}
				default:
				{
					signalItem.sdkIPStatus = SDK_IP_STATUS_FALSE;
					break;
				}
			}
			break;
		}
		default:
			signalItem.sdkIPStatus = SDK_IP_STATUS_FALSE;
			break;
	}
	return signalItem.sdkIPStatus;
}

server_ip_type_e ctiot_signal_get_server_ip_type(void)
{
	return (server_ip_type_e)signalItem.serverIPType;
}

uint16_t ctiot_signal_emit_ip_event(chip_ip_type_e chipIPType)
{
	if(ctiot_signal_is_ip_event_subscribed())//如果sdk订阅ip地址变化事件，才向sdk发送变化通知(目前适配层ip变化事件到来,无论什么情况,都会调用此方法)
	{
		ctiot_get_ip_status(chipIPType);
		ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ip event is comming,ip status =%u...\r\n",signalItem.sdkIPStatus);
		if(signalItem.ipCallback != NULL)
		{
			if(signalItem.sdkIPStatus == SDK_IP_STATUS_TRUE_V4 || signalItem.sdkIPStatus == SDK_IP_STATUS_TRUE_V6 || signalItem.sdkIPStatus == SDK_IP_STATUS_DISABLE)
			{
				signalItem.ipCallback();
			}
		}
	}
    return CTIOT_NB_SUCCESS;
}

void ctiot_signal_set_chip_ip_type(uint16_t iptype)
{
	signalItem.chipIpType = iptype;
}

uint16_t ctiot_signal_get_chip_ip_type(void)
{
	return signalItem.chipIpType;
}

uint16_t ctiot_signal_cancel_all(void)
{
	if(ctiot_signal_is_ip_event_subscribed())
	{
    	ctiot_signal_cancel_ip_event();
	}
	if(ctiot_signal_is_psm_status_subscribed())
	{
		ctiot_signal_cancel_psm_status();
	}
    return CTIOT_NB_SUCCESS;
}

uint8_t ctiot_signal_is_ip_event_subscribed(void)
{
    return signalItem.ipFlag;
}


uint16_t ctiot_signal_get_psm_status(void)
{
	return signalItem.psmStatus;
}

uint16_t ctiot_signal_set_psm_status(uint16_t status)
{
	uint16_t result = CTIOT_OTHER_ERROR;
    if((status == STATUS_NO_PSMING || status  == STATUS_PSMING || status == STATUS_NET_OOS) && status != signalItem.psmStatus)
    {
    	signalItem.psmStatus = status;
		result = CTIOT_NB_SUCCESS;
    }
    ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctiot_signal_set_psm_status:current psm status:%u\r\n",signalItem.psmStatus);
	return result;
}

uint16_t ctiot_signal_subscribe_psm_status(PEventCallback pEventCallback)
{
	//若芯片有独立的psm通知事件，需要在此处订阅芯片的psm事件，并返回订阅结果
	ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctiot signal subscribe psm status\r\n");
	if(signalItem.psmStatusFlag == 1)
	{
		return CTIOT_NB_SUCCESS;
	}
	if(ctchip_event_psm_status_init()==-1)
	{
		return CTIOT_SYS_API_ERROR;
	}
	if(ctchip_is_net_in_oos())
	{
		signalItem.psmStatus = STATUS_NET_OOS;
	}
	else
	{
		if(ctchip_get_psm_mode()==STATUS_PSMING)
		{
			signalItem.psmStatus = STATUS_PSMING;
		}
		else
		{
			signalItem.psmStatus = STATUS_NO_PSMING;
		}
	}
	signalItem.psmCallback = pEventCallback;
	signalItem.psmStatusFlag = 1;
	return CTIOT_NB_SUCCESS;
}

uint16_t ctiot_signal_cancel_psm_status(void)
{
    ctchip_event_psm_status_destroy();
	ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"cancel psm status event...\r\n");
    signalItem.psmStatusFlag = 0;
	signalItem.psmCallback = NULL;
    return CTIOT_NB_SUCCESS;
}

uint16_t ctiot_signal_emit_psm_status(uint16_t status)
{
	if(ctiot_signal_is_psm_status_subscribed()==1)
	{
		ctiot_signal_set_psm_status(status);

		if(signalItem.psmCallback != NULL && status == 0)
		{
			//ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"psm status event is comming,stauts=%u...\r\n",status);

			signalItem.psmCallback();
		}
	}
	return CTIOT_NB_SUCCESS;
}

uint16_t ctiot_signal_is_psm_status_subscribed(void)
{
	ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctiot_signal_is_psm_status_subscribed:%u\r\n",signalItem.psmStatusFlag);
	return signalItem.psmStatusFlag;
}

