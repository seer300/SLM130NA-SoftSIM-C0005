/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "xy_ps_api.h"
#include "xy_utils.h"
#include "xy_system.h"
#include "lwip/errno.h"
#include "xy_at_api.h"
#include "ctwing_adapt.h"
#endif


#include "ctlw_liblwm2m.h"
#include "ctlw_internals.h"
#include "ctlw_platform.h"
#include "ctlw_abstract_signal.h"
#include "ctlw_heap.h"
#include "ctlw_abstract_os.h"
#include "ctlw_sdk_internals.h"
#include "ctlw_NV_data.h"

#ifdef PLATFORM_LINUX
//使用 ifconf结构体和ioctl函数时需要用到该头文件
#include <net/if.h>
#include <sys/ioctl.h>
//消息队列需要用到的头
#include <sys/ipc.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
//使用ifaddrs结构体时需要用到该头文件
#include <ifaddrs.h>
#include "chip_info.h"
#endif

#ifdef PLATFORM_XYZ
#include "ps_lib_api.h"
#include "at_util.h"
#endif



#ifdef PLATFORM_XYZ
static uint8_t ctiotInitSlpHandler = 0xff; //预初始投票句柄
static uint8_t ctiotSlpHandler = 0xff; //收发线投票句柄
static uint8_t ctiotIPEventSlpHandler = 0xff; //IP事件投票句柄
#endif

#ifdef PLATFORM_LINUX
static uint8_t ctiotInitSlpHandler= 1;  //预初始投票句柄
static uint8_t ctiotSlpHandler = 0;//收发线投票句柄
static uint8_t ctiotIPEventSlpHandler = 2;  //IP事件投票句柄
#endif

#ifdef PLATFORM_XINYI
static uint8_t ctiotSlpHandler = 0;
static uint8_t ctiotInitSlpHandler= 1;
static uint8_t ctiotIPEventSlpHandler = 2;
static uint8_t ctiotBsHandler = 3;
static uint8_t ctiotInitAppSlpHandler = 4;
#endif

void ctchip_asyn_notify(char *at_str)
{
#ifdef PLATFORM_LINUX
	ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"\r\n%s\r\n", at_str);
#endif
#ifdef PLATFORM_XYZ
	atcURC(AT_CHAN_DEFAULT, (CHAR*)at_str);
#endif
#ifdef PLATFORM_XINYI
	xy_ctlw_asyn_notify(at_str);
#endif
}

uint16_t ctchip_init_ip_event_slp_handler(void)
{
#ifdef PLATFORM_XYZ
	if(slpManApplyPlatVoteHandle("CTIOT_IP", &ctiotIPEventSlpHandler) == 0)
	{
		return CTIOT_NB_SUCCESS;
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}

#endif
#ifdef PLATFORM_LINUX
	ctiotIPEventSlpHandler = 2;
	return CTIOT_NB_SUCCESS;
#endif
#ifdef PLATFORM_XINYI
	ctiotIPEventSlpHandler = 2;
	return CTIOT_NB_SUCCESS;
#endif
}
uint16_t ctchip_init_send_recv_slp_handler(void)
{
#ifdef PLATFORM_XYZ
	if(slpManApplyPlatVoteHandle("CTIOT_SEND_RECV", &ctiotSlpHandler) == 0)
	{
		return CTIOT_NB_SUCCESS;
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}

#endif
#ifdef PLATFORM_LINUX
	ctiotSlpHandler = 0;
	return CTIOT_NB_SUCCESS;
#endif
#ifdef PLATFORM_XINYI
	ctiotSlpHandler = 0;
	return CTIOT_NB_SUCCESS;
#endif
}

uint16_t ctchip_init_initialization_slp_handler(void)
{
#ifdef PLATFORM_XYZ
	if(slpManApplyPlatVoteHandle("CTIOT_INIT", &ctiotInitSlpHandler) == 0)
	{
		return CTIOT_NB_SUCCESS;
	}
	else
	{
		return CTIOT_OTHER_ERROR;
	}
#endif
#ifdef PLATFORM_LINUX
	ctiotInitSlpHandler = 1;
	return CTIOT_NB_SUCCESS;
#endif

#ifdef PLATFORM_XINYI
	ctiotInitSlpHandler = 1;
	return CTIOT_NB_SUCCESS;
#endif
}
uint8_t ctchip_get_initialization_slp_handler(void)
{
	return ctiotInitSlpHandler;
}

uint8_t ctchip_get_send_recv_slp_handler(void)
{
	return ctiotSlpHandler;
}

uint8_t ctchip_get_ip_event_slp_handler(void)
{
	return ctiotIPEventSlpHandler;
}

 static	void prv_print_slpState(void)
{
	uint8_t counter0 = 0, counter1 = 0, counter2 = 0;
#ifdef PLATFORM_XYZ
	slpManSlpState_t pstate;
	if (RET_TRUE == slpManCheckVoteState(ctiotSlpHandler, &pstate, &counter0))
	{
	}
	if (RET_TRUE == slpManCheckVoteState(ctiotIPEventSlpHandler, &pstate, &counter1))
	{
	}
	if (RET_TRUE == slpManCheckVoteState(ctiotInitSlpHandler, &pstate, &counter2))
	{
	}
#endif
#ifdef PLATFORM_LINUX
	print_vote_status();
#endif
}

system_boot_reason_e ctchip_get_system_boot_reason(void)
{
	system_boot_reason_e result = CTIOT_UNKNOWN_STATE;
#ifdef PLATFORM_XYZ
	slpManWakeSrc_e state = slpManGetWakeupSrc();
	if (state == WAKEUP_FROM_POR)
	{
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"wake up by reboot");
		result = CTIOT_ACTIVE_REBOOT; //上电引起
	}
	else if (state == WAKEUP_FROM_RTC)
	{
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"wake up by timer");
		result = CTIOT_ACTIVE_WAKEUP; //内部定时器引起
	}
	else if (state == WAKEUP_FROM_PAD)
	{
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"wake up by power level");
		result = CTIOT_ACTIVE_WAKEUP; //MCU拉引脚电平引起
	}
#endif
#ifdef PLATFORM_LINUX
	uint8_t bootupCause;
	bootupCause = appGetStartReason();
	switch (bootupCause)
	{
	case 0:
	{
		result = CTIOT_ACTIVE_REBOOT;
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_ACTIVE_STATE,value=%d\r\n",result);
		break;
	}
	case 1:
	{
		result = CTIOT_ACTIVE_WAKEUP;
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_HIB_STATE_MCU,value=%d\r\n",result);
		break;
	}
	default:
	{
		result = CTIOT_ACTIVE_WAKEUP;
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_HIB_STATE_OTH,value=%d\r\n",result);
		break;
	}
	}
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_system_boot_reason();
#endif
	return result;
}

void ctchip_init_app_slp_handler(char* name,uint8_t* handler)
{
#ifdef PLATFORM_XYZ
		slpManApplyPlatVoteHandle(name, handler);
#endif
#ifdef PLATFORM_XINYI
	ctiotInitAppSlpHandler = 1;
#endif

}

uint8_t ctchip_disable_sleepmode(uint8_t slpHandler)
{
	uint8_t result = 0;
#ifdef PLATFORM_XYZ
	uint8_t counter = 0;
	slpManSlpState_t pstate;
	if (RET_TRUE == slpManCheckVoteState(slpHandler, &pstate, &counter))
	{
		if (counter <= 0)
		{
			prv_print_slpState();
			result = slpManPlatVoteDisableSleep(slpHandler, SLP_SLP2_STATE);
			prv_print_slpState();
		}
	}
#endif
#ifdef PLATFORM_LINUX
	if(linux_get_vote_status(slpHandler)!=1)
	{
		ctiot_log_info(LOG_COMMON_MODULE,LOG_VOTE_CLASS,"chip support sleep...\r\n");
		ctiot_log_info(LOG_COMMON_MODULE,LOG_VOTE_CLASS,"sdk busy:%d\r\n",slpHandler);
		vote_for_busy(slpHandler);
	}
#endif
	return result;
}

uint8_t ctchip_enable_sleepmode(uint8_t slpHandler)
{
	uint8_t result= 0;
#ifdef PLATFORM_XYZ
	uint8_t counter = 0;
	slpManSlpState_t pstate;
	uint8_t testcounter;

	if (RET_TRUE == slpManCheckVoteState(slpHandler, &pstate, &counter))
	{
		testcounter = counter;
		if (counter > 0)
		{
			prv_print_slpState();
		}
		for (; counter > 0; counter--)
		{
			result = slpManPlatVoteEnableSleep(slpHandler, SLP_SLP2_STATE);
			if(result != 0)
				break;
		}
		if (testcounter > 0)
		{
			prv_print_slpState();
		}
	}

#endif
#ifdef PLATFORM_LINUX
	if(linux_get_vote_status(slpHandler)!=0)
	{
		ctiot_log_info(LOG_COMMON_MODULE,LOG_VOTE_CLASS,"chip support sleep...\r\n");
		ctiot_log_info(LOG_COMMON_MODULE,LOG_VOTE_CLASS,"sdk free:%d\r\n",slpHandler);
		vote_for_sleep(slpHandler);
	}
#endif
	return result;
}

uint16_t ctchip_sync_cstate(void)
{
	uint16_t wirelessStatus = 0;
#ifdef PLATFORM_XYZ
	//result = appGetCeregStateSync(&wirelessStatus); //新版本启用
	extern void CcmRegGetCmiPsCeregCnfInfo(CmiPsCeregModeEnum reportMode,CmiPsGetCeregCnf* pGetCeregCnf);
	CmiPsCeregModeEnum reportMode = CMI_PS_CEREG_LOC_PSM_INFO_EMM_CAUSE;//5
	CmiPsGetCeregCnf pGetCeregCnf;
	CcmRegGetCmiPsCeregCnfInfo(reportMode,&pGetCeregCnf);
	wirelessStatus = pGetCeregCnf.state;	
#endif
#ifdef PLATFORM_LINUX
	appGetCeregStateSync(&wirelessStatus);
#endif
#ifdef PLATFORM_XINYI
	wirelessStatus = xy_ctlw_sync_cstate();
#endif
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"chip network state sync:%u\r\n",wirelessStatus);
	return wirelessStatus;
}

uint16_t ctchip_get_imsi_info(uint8_t *imsi,uint16_t maxImsiLen)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	if (imsi == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	memset(imsi,0,maxImsiLen);
#ifdef PLATFORM_XYZ
	if (appGetImsiNumSync((CHAR *)imsi) != NBStatusSuccess)
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:get imsi failed\r\n");
		//处理imsi获取错误
		goto exit;
	}
	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:imsi=%s\r\n", imsi);
	}
#endif
#ifdef PLATFORM_LINUX
	if (appGetImsiNumSync((CHAR *)imsi) != NBStatusSuccess)
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:get imsi failed\r\n");
		//处理imsi获取错误
		goto exit;
	}
	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:imsi=%s\r\n", imsi);
	}
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_imsi_info(imsi, maxImsiLen);
#endif
	
exit:
	return result;
}

uint16_t ctchip_get_imei_info(uint8_t* imei,uint16_t maxImeiLen)
{
	uint16_t result = CTIOT_NB_SUCCESS;
	if (imei == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	memset(imei,0,maxImeiLen);
#ifdef PLATFORM_XYZ
	if (appGetImeiNumSync((CHAR *)imei) != NBStatusSuccess)
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:get imei failed\r\n");
		//处理imei获取错误
		goto exit;
	}
	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imei_info:imei=%s\r\n", imei);
	}
#endif
#ifdef PLATFORM_LINUX
	if (appGetImeiNumSync(imei) != NBStatusSuccess)
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imsi_info:get imei failed\r\n");
		//处理imei获取错误
		goto exit;
	}
	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_imei_info:imei=%s\r\n", imei);
	}
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_imei_info(imei, maxImeiLen);
#endif
exit:
	return result;
}
void ctchip_stop_sleep_timer(void)
{
#ifdef PLATFORM_LINUX
	alarm(0);
#endif

#ifdef PLATFORM_XINYI
	xy_ctlw_stop_auto_update_rtc();
#endif
}
void ctchip_start_sleep_timer(uint32_t timeMs)
{
#ifdef PLATFORM_LINUX
	alarm(timeMs/1000);
#endif

#ifdef PLATFORM_XINYI
	xy_ctlw_start_auto_update_rtc(timeMs/1000);
#endif
}

int32_t ctchip_get_sock_errno(int32_t sock)
{
#ifdef PLATFORM_XYZ
	return sock_get_errno(sock);
#endif
#ifdef PLATFORM_LINUX
	return errno;
#endif
#ifdef PLATFORM_XINYI
	return errno;
#endif
}

uint8_t ctchip_get_psm_mode(void)
{
	uint8_t psmMode = 0;
#ifdef PLATFORM_XYZ
	CmsRetId result = appGetPSMModeSync(&psmMode);
	if(result != CMS_RET_SUCC)
	{
	    psmMode = 0;
	}
#endif
#ifdef PLATFORM_LINUX
	psmMode = 0;
#endif
#ifdef PLATFORM_XINYI
	psmMode = xy_ctlw_get_psm_mode();
#endif
	return psmMode;
}

bool ctchip_is_net_in_oos(void)
{
	bool result = false;
#ifdef PLATFORM_XYZ
	NmAtiSyncRet netInfo= { 0 };
	appGetNetInfoSync(0 /*pContext->chipInfo.cCellID*/, &netInfo);
	if(netInfo.body.netInfoRet.netifInfo.netStatus == NM_NETIF_OOS)
	{
		result = true;
	}
#endif
#ifdef PLATFORM_LINUX
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_is_net_in_oos();
#endif
	return result;
}

uint16_t ctchip_get_session_ip(uint8_t stReason, int32_t addressFamily,uint8_t* sesIpAddr)

{
	uint16_t result = CTIOT_NB_SUCCESS;
#ifdef PLATFORM_XYZ
	if (stReason == CTIOT_ACTIVE_WAKEUP)
	{
		 return ctlw_get_local_ip((char *)sesIpAddr, addressFamily);
	}
#endif
#ifdef PLATFORM_LINUX
	if (stReason == CTIOT_ACTIVE_WAKEUP)
	{
		return ctlw_get_local_ip((char *)sesIpAddr, addressFamily);
	}
#endif
#ifdef PLATFORM_XINYI
	if (stReason == CTIOT_ACTIVE_WAKEUP)
	{
		uint16_t ipResult = ctlw_get_local_ip((char *)sesIpAddr, addressFamily);
		if (ipResult == CTIOT_NB_SUCCESS)
		{
			return CTIOT_NB_SUCCESS;
		}
		else
		{
			return NULL;
		}
	}
	else if (stReason == CTIOT_ACTIVE_REBOOT)
	{
		return NULL;
	}
#endif
	return result;
}

//此接口未实现，需厂商实现
uint16_t ctchip_write_session_ip(uint8_t* sessionIP)//如果失败返回CTIOT_OTHER_ERROR
{
	uint16_t result = CTIOT_NB_SUCCESS;
	if(sessionIP!=NULL)
		ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctchip_write_session_ip:write session ip,%s",sessionIP);
	else
		ctiot_log_info(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctchip_write_session_ip:write session ip,NULL");
	return result;
}

uint16_t ctchip_get_iccid_info(uint8_t* iccid,uint16_t maxIccidLen)//iccid空间需上层分配
{
	uint16_t result = CTIOT_NB_SUCCESS;
	if(iccid == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	memset(iccid,0,maxIccidLen);
	//test 需从芯片获取
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_iccid_info(iccid, maxIccidLen);
#endif
exit:
	return result;
}

uint16_t ctchip_get_apn_info(uint8_t* apn,uint16_t maxApnLen)//apn空间需上层分配
{
	uint16_t result = CTIOT_NB_SUCCESS;
	if(apn == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	memset(apn,0,maxApnLen);
	//test 需从芯片获取
#ifdef PLATFORM_XYZ
	sprintf((char *)apn,"%s","Psm0.eDRX0.ctnb");
#endif
#ifdef PLATFORM_LINUX
	sprintf((char *)apn,"%s","Psm0.eDRX0.ctnb");
#endif
#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_apn_info(apn, maxApnLen);
#endif

exit:
	return result;
}

uint16_t ctchip_get_module_info(uint8_t* sv,uint16_t maxSvLen,uint8_t* chip,uint16_t maxChipLen,uint8_t* module,uint16_t maxModuleLen)//sv、chip、module空间需上层分配
{
	uint16_t result = CTIOT_NB_SUCCESS;
#ifdef PLATFORM_XYZ
	char *versioninfo = NULL;
#endif
	if(sv == NULL || chip == NULL || module == NULL)
	{
		result = CTIOT_OTHER_ERROR;
		goto exit;
	}
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctchip_get_module_info:clear info");
	memset(sv,0,maxSvLen);
	memset(chip,0,maxChipLen);
	memset(module,0,maxModuleLen);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"begin get info");
	//test 需从芯片获取
#ifdef PLATFORM_XYZ
	versioninfo = (char *)appGetNBVersionInfo();
	//-- SDK Version: XYZ2.1_SW_V001.000.xxx -- -- EVB Version: XYZ2.1_HW_V1.0 --
	if (versioninfo != NULL)
	{
		//处理版本信息，包括软件版本号字符串信息，包含了芯片版本，以及 SDK 版本等信息
		char *cSDKVersionStartPos = strstr(versioninfo, "SDK Version: ") + strlen("SDK Version: ");
		int32_t i = 0;
		while (*(cSDKVersionStartPos + i) != ' ')
		{
			sv[i] = *(cSDKVersionStartPos + i);
			i++;
		}
		i = 0;
		char *cChipVersionStartPos = strstr(versioninfo, "EVB Version: ") + strlen("EVB Version: ");
		while (*(cChipVersionStartPos + i) != ' ')
		{
			module[i] = *(cChipVersionStartPos + i);
			i++;
		}
	}
	sprintf((char *)chip,"%s","XYZ2.1");
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctiot_get_chip_info:sv:%s\tcFirmwareVersion:%s\tchip:%s\r\n", sv, module,chip);
#endif
#ifdef PLATFORM_LINUX
	char *versioninfo = NULL;
	versioninfo = (char *)appGetNBVersionInfo();
	//-- SDK Version: XYZ2.1_SW_V001.000.xxx -- -- EVB Version: XYZ2.1_HW_V1.0 --
	if (versioninfo != NULL)
	{
		//处理版本信息，包括软件版本号字符串信息，包含了芯片版本，以及 SDK 版本等信息
		char *cSDKVersionStartPos = strstr(versioninfo, "SDK Version: ") + strlen("SDK Version: ");
		int32_t i = 0;
		while (*(cSDKVersionStartPos + i) != ' ')
		{
			sv[i] = *(cSDKVersionStartPos + i);
			i++;
		}
		i = 0;
		char *cChipVersionStartPos = strstr(versioninfo, "EVB Version: ") + strlen("EVB Version: ");
		while (*(cChipVersionStartPos + i) != ' ')
		{
			module[i] = *(cChipVersionStartPos + i);
			i++;
		}
	}
	sprintf((char *)chip,"%s","XYZ2.1.1");
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctiot_get_chip_info:sv:%s\tcFirmwareVersion:%s\tchip:%s\r\n", sv, module,chip);
#endif

#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_module_info(sv, maxSvLen, chip, maxChipLen, module, maxModuleLen);
#endif
exit:
	return result;
}

uint16_t ctchip_get_wireless_signal_info(uint8_t *rsrp, uint16_t maxRsrpLen, uint8_t *sinr, uint16_t maxSinrLen, uint8_t *txPower, uint16_t maxTxPowerLen, uint8_t *cellID, uint16_t maxCellIDLen)
{
	//如果芯片取不到数据，就提供缺省值，rsrp = "0"，sinr="999",cell_id="000000",txPower = "999"
	uint16_t result = CTIOT_NB_SUCCESS;
#ifdef PLATFORM_LINUX_DEMO
	memset(rsrp,0,maxRsrpLen);
	memset(sinr,0,maxSinrLen);
	memset(txPower,0,maxTxPowerLen);
	memset(cellID,0,maxCellIDLen);
	//test,需从芯片获取
	//*rsrp = -120;
	//*sinr = 10;
	//*txPower = 30;
	uint32_t intCellID;
	int32_t intRsrp;
	if(ctchip_get_rsrp(&intRsrp)==0)
	{
		sprintf((char *)rsrp,"%d",intRsrp);
	}
	else
	{
		sprintf((char *)rsrp,"%d",-120);
	}
	sprintf((char *)sinr,"%d",10);
	sprintf((char *)txPower,"%d",30);
	if(ctchip_get_cell_id(&intCellID)==0)
	{
		sprintf((char *)cellID,"%d",intCellID);
	}
	else
	{
		sprintf((char *)cellID,"%s","ABCD_EDDJ");
	}
#endif

#ifdef PLATFORM_XINYI
	result = xy_ctlw_get_wireless_signal_info(rsrp, maxRsrpLen, sinr, maxSinrLen, txPower, maxTxPowerLen, cellID, maxCellIDLen);
#endif
	return result;

}

uint16_t ctchip_get_cell_id(uint32_t* cellID)
{
	int32_t result = 0;
#ifdef PLATFORM_XYZ
	uint16_t tac;
	if(appGetLocationInfoSync(&tac, (UINT32 *)cellID)!=0)
	{
		return 1;
	}
    ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS, "cellid=%u\r\n",*cellID);
#endif
#ifdef PLATFORM_LINUX
	result = xy_ctlw_get_cell_id(cellID);
#endif
	return result;
}
uint16_t ctchip_get_rsrp(int32_t* rsrp)
{
	int32_t result = 0;
#ifdef PLATFORM_XYZ
	*rsrp = -120;//待芯片厂商提供方法
#endif
#ifdef PLATFORM_LINUX
	result = xy_ctlw_get_rsrp(rsrp);
#endif
	return result;

}
uint8_t * ctchip_get_nv(void)
{
#ifdef PLATFORM_XYZ
	return slpManGetUsrNVMem();
#endif
#ifdef PLATFORM_LINUX
	return linux_get_NV();
#endif
#ifdef PLATFORM_XINYI
    return xy_ctlw_get_sdk_nv();
#endif
}

uint16_t ctchip_update_nv(void)
{
#ifdef PLATFORM_XYZ
	slpManUpdateUserNVMem();
	return 0;
#endif
#ifdef PLATFORM_LINUX
	linux_update_NV();
#endif
#ifdef PLATFORM_XINYI
	xy_update_SDK_NV();
	return 0;
#endif
}

#ifdef PLATFORM_XINYI
uint16_t ctchip_flush_nv(ctlw_file_type_e type)
#else
uint16_t ctchip_flush_nv(void)
#endif
{
	ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"flush memory to File System\r\n");
#ifdef PLATFORM_XYZ
	slpManFlushUsrNVMem();
	return 0;
#endif
#ifdef PLATFORM_LINUX
	linux_flush_NV();
	return 0;
#endif
#ifdef PLATFORM_LINUX
	linux_flush_NV();
#endif
#ifdef PLATFORM_XINYI
	xy_ctlw_flush_sdk_nv(type);
	return 0;
#endif
}

#ifdef PLATFORM_XYZ
int32_t ctchip_event_callback(urcID_t eventID, void *param, uint32_t paramLen)
{
	ctiot_context_t *pContext = ctiot_get_context();

	switch (eventID)
	{
	case NB_URC_ID_PS_NETINFO://网络连接状态变化事件
	{
		NmAtiNetifInfo * netif = (NmAtiNetifInfo *)param;
		ctiot_signal_set_chip_ip_type(netif->ipType);
		if(ctiot_signal_get_psm_status() == STATUS_NET_OOS && netif->netStatus != NM_NETIF_OOS)
		{
			ctiot_signal_emit_psm_status(STATUS_NO_PSMING);
		}
		if (netif->netStatus == NM_NETIF_ACTIVATED || netif->netStatus == NM_NETIF_ACTIVATED_INFO_CHNAGED)
		{
  			if(netif->ipType == NM_NET_TYPE_IPV4 ||netif->ipType == NM_NET_TYPE_IPV6 ||netif->ipType ==NM_NET_TYPE_IPV4_IPV6preparing)
  			{
  				ctiot_signal_emit_ip_event((chip_ip_type_e)netif->ipType);
  			}
			else if(netif->ipType == NM_NET_TYPE_IPV4V6)
			{
				if(!(netif->netStatus == NM_NETIF_ACTIVATED_INFO_CHNAGED &&ctiot_signal_get_server_ip_type()==SERVER_IP_TYPE_V4V6) )
				{
					ctiot_signal_emit_ip_event((chip_ip_type_e)netif->ipType);
				}
			}
		}
		else
		{
			if(netif->netStatus == NM_NETIF_OOS && ctiot_signal_is_psm_status_subscribed())
			{
				ctiot_signal_emit_psm_status(STATUS_NET_OOS);
			}
			ctiot_signal_set_chip_ip_type(0);
			ctiot_signal_emit_ip_event((chip_ip_type_e)0);
		}
		break;
	}
	case NB_URC_ID_MM_PSM_STATE_CHANGED:
	{
		ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"NB_URC_ID_MM_PSM_STATE_CHANGED  comming\r\n");
		uint32_t* psmState=(uint32_t *)param;
		uint16_t status =(uint16_t) (*psmState);
		ctiot_signal_emit_psm_status(status);
		break;
	}
	default:
		break;
	}
	return 0;
}

#endif

int8_t ctchip_event_ip_status_init(void)
{
#ifdef PLATFORM_XYZ
	NBStatus_t registerStatus = registerPSEventCallback(NB_GROUP_ALL_MASK, (psEventCallback_t)ctchip_event_callback);
	if (registerStatus != NBStatusSuccess)
	{
		return -1;
	}
#endif
#ifdef PLATFORM_LINUX
#endif
#ifdef PLATFORM_XINYI
	xy_ctlw_event_ip_status_init();
#endif
	return 0;
}

void ctchip_event_ip_status_destroy(void)
{
#ifdef PLATFORM_XYZ
	deregisterPSEventCallback((psEventCallback_t)ctchip_event_callback);
#endif
#ifdef PLATFORM_XINYI
	xy_ctlw_event_ip_status_destroy();
#endif
}

int8_t ctchip_event_psm_status_init(void)
{
#ifdef PLATFORM_XYZ
	return 0;
#endif
#ifdef PLATFORM_LINUX
	return 0;
#endif
#ifdef PLATFORM_XINYI
	
	return 0;
#endif
}

void ctchip_event_psm_status_destroy(void)
{
#ifdef PLATFORM_XYZ
#endif
#ifdef PLATFORM_LINUX
#endif
#ifdef PLATFORM_XINYI
#endif
}
#if CTIOT_TIMER_AUTO_UPDATE == 1
void timer_callback(uint8_t id)
{
#ifdef PLATFORM_XINYI
	xy_ctlw_auto_update_rtc_callback();
#else
	ctiot_vote_recv_send_busy();
	#ifdef PLATFORM_LINUX
	ctiot_set_auto_update_flag(1);
	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"timer callback, autoupdate\r\n");
	#endif
	ctiot_start_auto_update_timer();

#endif
	
}
void ctchip_register_timer_callback(timer_callback_func timerCB)
{
	#ifdef PLATFORM_LINUX
	signal(SIGALRM,timerCB);
	#endif
}
#endif
