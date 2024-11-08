#pragma once

/*******************************************************************************
 *				              Include header files							  *
 ******************************************************************************/
#include "cmsis_os2.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define AT_RSP_OK         			"\r\nOK\r\n"
#define AT_CMD_PREFIX 				30
#define AT_RECV_MAX_LENGTH  		4000
#define AT_RETRANS_MAX_NUM 			40  //底板MCU和用户同时发送冲突时采用的重传次数，以取代原先8007的报错
#define AT_RETRANS_DELAY_TIME 		50  //单位:ms

/* at ctl thread */
#define AT_CTRL_THREAD_NAME         "at_ctl"
#define AT_CTRL_THREAD_STACKSIZE 	osStackShared
#define AT_CTRL_THREAD_PRIO 		osPriorityAboveNormal	

/* at proxy thread */
#define AT_PROXY_THREAD_NAME        "at_proxy"
#define AT_PROXY_THREAD_STACKSIZE 	osStackShared
#define AT_PROXY_THREAD_PRIO 		AT_CTRL_THREAD_PRIO	

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
enum AT_MSG_ID
{
	AT_MSG_RCV_STR_FROM_FARPS,	//farps send  at cmd to  at_ctl
	AT_MSG_RCV_STR_FROM_NEARPS, //ps(cp/user rsp) send at cmd to at_ctl
};

typedef enum AT_SRC_FD_T
{
	AT_XY_FD_MIN = 0,
	AT_FD_INVAIL = AT_XY_FD_MIN,

	/*  nearps fd */
	NEARPS_CP_FD,			//3gpp ps
	NEARPS_USER_FD,			//仅供at_proxy中平台扩展AT命令异步处理方式使用，即AT_ASYN

	/*  farps fd  */
	FARPS_BLE_FD, 			//蓝牙
	FARPS_AP_ASYNC,  		//AP核本地的异步AT命令机制使用，即Send_AT_Req与Get_AT_Rsp异步模式，或者纯AT虚拟通道模式at_uart_write
	FARPS_AP_SYNC,		    //AT_Send_And_Get_Rsp应答结果AT字符串，不包含URC，要求中间结果必须与OK作为一条命令发送过来
	FARPS_AP_EXT,           //外部MCU通过AT物理串口进行的AT命令发送
	FARPS_LOG_FD,  			//log
	AT_XY_FD_MAX = 0x8,

	/*  user fd  */
	FARPS_USER_MIN,
	FARPS_USER_PROXY = FARPS_USER_MIN, //仅用于at_send_wait_rsp接口，普通的平台扩展AT命令无需使用该ID
	FARPS_USER_MAX = 0x10,
} AT_SRC_FD;

/*与Boot_Reason_Type及子原因非一一对应。该宏值仅用于调试*/
typedef enum
{
	REBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET,  //正常断电上电动作,含OPENCPU的stop_CP场景
	REBOOT_CAUSE_SECURITY_RESET_PIN,           //外部通过RESET-PIN执行的芯片硬复位
	REBOOT_CAUSE_APPLICATION_AT,               //AT+NRB触发的软重启
	REBOOT_CAUSE_APPLICATION_RTC,              //RTC触发的深睡唤醒
	REBOOT_CAUSE_SECURITY_EXTERNAL_PIN,        //外部LPUART唤醒，通常为AT命令唤醒
	REBOOT_CAUSE_SECURITY_WATCHDOG,            //AP_WDT/UTC_WDT复位，不包括CP核的看门狗复位
	REBOOT_CAUSE_SECURITY_FOTA_UPGRADE,        //FOTA升级重启
	REBOOT_CAUSE_APPLICATION_SYSRESETREQ,      //AT+RESET触发的软重启
	REBOOT_CAUSE_SECURITY_GLOBAL_RESET,        //CP核看门狗 + xy_Soc_Reset + ASSERT异常断言 + SVD
	REBOOT_CAUSE_SECURITY_SOFT_RESET,          //用户调用xy_Soft_Reset触发的软重启
	REBOOT_CAUSE_SECURITY_WAKEUP_DSLEEP,       //除了RTC和LPUART之外的其他深睡唤醒，如WAKUP-PIN唤醒等
	REBOOT_CAUSE_SECURITY_RESET_UNKNOWN,       //未知的异常上电，一般为硬件故障
} AT_REBOOT_CAUSE_E;


enum AT_PROXY_MSG_ID
{
	AT_PROXY_MSG_CMD_PROC,
};
