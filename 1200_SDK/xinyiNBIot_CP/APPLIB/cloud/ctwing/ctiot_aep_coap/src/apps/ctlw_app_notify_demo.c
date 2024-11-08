#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_app_notify_demo.h"
#include "ctlw_platform.h"
static float testAppVal1 = 1.0f;

void ctlw_app_notify_demo_1(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)

{
	if(uri) ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw_app_notify_demo_1, uri: %s", uri);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw_app_notify_demo_1,notifyType=%u,subType=%u,value=%u\r\n",notifyType,subType,value);
	if (subType == CTIOT_NOTIFY_SUBTYPE_LSTATUS && value >= 2)
	{
		return;
	}
	switch (notifyType)
	{
		case CTIOT_NOTIFY_ASYNC_NOTICE:
		{
			switch (subType)
			{
				case CTIOT_NOTIFY_SUBTYPE_REG:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_REG,result=%u\r\n",value);
					if(value == 0)
					{
						ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"app can start here\r\n");
					}
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LWEVENT:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LWEVENT,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_UPDATE:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_UPDATE,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DEREG:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DEREG,result=%u\r\n",value);
					ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"app can stop here\r\n");
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SEND:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SEND,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LSTATUS:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LSTATUS,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DTLS_HS:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DTLS_HS,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE\r\n");
					ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"app can start here\r\n");
					break;
				}
				default:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"unknown subtype\r\n");
					break;
				}
			}
			break;
		}
		case CTIOT_SYS_API_ERROR:
		{
			ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"send error,value = %u",value);
			break;
		}
		case CTIOT_NOTIFY_RECV_DATA:
		{
			if(subType == 0)
			{
				//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:value:%u,reservedData:%s\r\n",value,reservedData);
				testAppVal1 = value;
			}
			else if(subType == 1)
			{
				ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:NULL\r\n");
			}
			break;
		}
		default:
		{
			ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"unknown notifyType\r\n");
			break;
		}
	}
}


void ctlw_app_notify_demo_2(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData)

{
	if(uri) ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw_app_notify_demo_2, uri: %s", uri);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctlw_app_notify_demo_2,notifyType=%u,subType=%u,value=%u\r\n",notifyType,subType,value);
	if (subType == CTIOT_NOTIFY_SUBTYPE_LSTATUS && value >= 2)
	{
		return;
	}
	switch (notifyType)
	{
		case CTIOT_NOTIFY_ASYNC_NOTICE:
		{
			switch (subType)
			{
				case CTIOT_NOTIFY_SUBTYPE_REG:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_REG,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LWEVENT:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LWEVENT,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_UPDATE:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_UPDATE,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DEREG:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DEREG,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SEND:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SEND,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_LSTATUS:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_LSTATUS,result=%u,data1=%u\r\n",value,data1);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_DTLS_HS:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_DTLS_HS,result=%u\r\n",value);
					break;
				}
				case CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE\r\n");
					break;
				}
				default:
				{
					ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"unknown subtype\r\n");
					break;
				}
			}
			break;
		}
		case CTIOT_SYS_API_ERROR:
		{
			ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"send error,value = %u",value);
			break;
		}
		case CTIOT_NOTIFY_RECV_DATA:
		{
			if(subType == 0)
			{
				//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:value:%u,reservedData:%s\r\n",value,reservedData);
				testAppVal1 = value;
			}
			else if(subType == 1)
			{
				ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"CTIOT_NOTIFY_RECV_DATA:NULL\r\n");
			}
			break;
		}
		default:
		{
			ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"unknown notifyType\r\n");
			break;
		}
	}
}
