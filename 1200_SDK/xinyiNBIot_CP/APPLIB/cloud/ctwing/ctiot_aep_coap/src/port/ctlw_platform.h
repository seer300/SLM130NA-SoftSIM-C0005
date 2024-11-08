/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

#ifndef _CTLW_PLATFORM_H_
#define _CTLW_PLATFORM_H_

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_liblwm2m.h"

#define CT_MODULE_CAPBILITY "11000000000000" //模组能力
/*CT_MODULE_CAPBILITY 固定长度14,默认全零
字符串位		能力			是否具备
						是	否
1		电信SDK	 	    1	0
2		定制AT指令集			1	0
3		软件安全模组			1	0
4		硬件安全模组			1	0
5		卫星定位			1	0
6		网络定位			1	0
7		其他定位			1	0
8		…		
*/

#ifdef PLATFORM_XINYI
#include "ctwing_adapt.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif
	//****************************************************************
	//! @brief ctchip_init_initialization_slp_handler:预处理投票句柄初始化
	//! @param 无参数
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_init_initialization_slp_handler(void);

	//****************************************************************
	//! @brief ctchip_init_ip_event_slp_handler:IP事件投票句柄初始化
	//! @param 无参数
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_init_ip_event_slp_handler(void);

	//****************************************************************
	//! @brief ctchip_init_send_recv_slp_handler:发送接收线程投票句柄初始化
	//! @param 无参数
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_init_send_recv_slp_handler(void);

	//****************************************************************
	//! @brief ctchip_init_app_slp_handler:APP投票句柄初始化
	//! @param 无参数
	//! @retval 无返回值
	//****************************************************************
	void ctchip_init_app_slp_handler(char *name, uint8_t *handler);
	
	//获取预处理投票句柄
	uint8_t ctchip_get_initialization_slp_handler(void);
	//获取发送接收线程投票句柄
	uint8_t ctchip_get_send_recv_slp_handler(void);
	//获取IP事件投票句柄
	uint8_t ctchip_get_ip_event_slp_handler(void);

	//****************************************************************
	//! @brief ctchip_disable_sleepmode:芯片投忙票
	//! @param uint8_t slpHandler 投票句柄
	//! @retval 0为成功，其它为失败
	//****************************************************************
	uint8_t ctchip_disable_sleepmode(uint8_t slpHandler);
	
	//****************************************************************
	//! @brief ctchip_enable_sleepmode:芯片投闲票
	//! @param uint8_t slpHandler 投票句柄
	//! @retval 0为成功，其它为失败
	//****************************************************************
	uint8_t ctchip_enable_sleepmode(uint8_t slpHandler);


	//测试用，设置内部唤醒时钟，测试内部时钟唤醒场景
	void ctchip_start_sleep_timer(uint32_t timeMS);
	void ctchip_stop_sleep_timer(void);

	//****************************************************************
	//! @brief  ctchip_asyn_notify:异步通知消息
	//! @param  char *str 通知的数据
	//! @retval 无返回值
	//****************************************************************
	void ctchip_asyn_notify(char *str);

	//****************************************************************
	//! @brief  ctchip_get_sock_errno:查询socket的errno
	//! @param  int sock socket句柄
	//! @retval int32_t 返回错误码
	int32_t ctchip_get_sock_errno(int32_t sock);

	//****************************************************************
	//! @brief  ctchip_get_psm_mode:查询当前是否处于psm模式
	//! @param  无参数
	//! @retval uint16_t 返回值参考psm_status_e
	//****************************************************************
	uint8_t ctchip_get_psm_mode(void);

	//****************************************************************
	//! @brief  ctchip_is_net_in_oos:查询当前是否处于OOS状态
	//! @param  无参数
	//! @retval uint16_t OOS状态返回tue，否则返回false
	//****************************************************************
	bool ctchip_is_net_in_oos(void);

	//****************************************************************
	//! @brief  ctchip_get_session_ip:查询芯片会话ip
	//! @param  uint8_t stReason 启动原因
	//! @param  int addressFamily ip地址类型
	//! @param  uint8_t* sesIpAddr 出参,会话ip地址
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_session_ip(uint8_t stReason, int32_t addressFamily,uint8_t* sesIpAddr);

	//****************************************************************
	//! @brief  ctchip_write_session_ip:写芯片会话IP
	//! @param  sessionIP 会话IP
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_write_session_ip(uint8_t *sessionIP);

	//****************************************************************
	//! @brief  ctchip_get_system_boot_reason:查询芯片启动原因
	//! @param  void 无参数
	//! @retval system_boot_reason_e 参考system_boot_reason_e
	//****************************************************************
	system_boot_reason_e ctchip_get_system_boot_reason(void);

	//****************************************************************
	//! @brief  ctchip_sync_cstate:查询eps状态
	//! @param  void 无参数
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_sync_cstate(void);

	//****************************************************************
	//! @brief  ctchip_get_wireless_signal_info:查询无线信号信息，包括rsrp、sinr、txpower、cellid
	//! @param  uint8_t *rsrp 无线信号信息rsrp
	//! @param  uint16_t maxRsrpLen rsrp数据长度
	//! @param  uint8_t *sinr 无线信号信息sinr
	//! @param  uint16_t maxSinrLen sinr数据长度
	//! @param  uint8_t *txPower 无线信号信息txpower
	//! @param  uint16_t maxTxPowerLen txpower数据长度
	//! @param  uint8_t *cellID 无线信号信息cellid
	//! @param  uint16_t maxCellIDLen cellid数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_wireless_signal_info(uint8_t *rsrp, uint16_t maxRsrpLen, uint8_t *sinr, uint16_t maxSinrLen, uint8_t *txPower, uint16_t maxTxPowerLen, uint8_t *cellID, uint16_t maxCellIDLen);

	//****************************************************************
	//! @brief  ctchip_get_module_info:查询模组软件版本，芯片型号及模组型号
	//! @param  uint8_t *sv 模组软件版本
	//! @param  uint16_t maxSvLen 模组软件版本数据长度
	//! @param  uint8_t *chip 芯片型号
	//! @param  uint16_t maxChipLen 芯片型号数据长度
	//! @param  uint8_t *module 模组型号
	//! @param  uint16_t maxModuleLen 模组型号数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_module_info(uint8_t *sv, uint16_t maxSvLen, uint8_t *chip, uint16_t maxChipLen, uint8_t *module, uint16_t maxModuleLen);

	//****************************************************************
	//! @brief  ctchip_get_imei_info:查询芯片imei信息
	//! @param  uint8_t *imei imei数据
	//! @param  uint16_t maxImeiLen imei数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_imei_info(uint8_t *imei, uint16_t maxImeiLen);

	//****************************************************************
	//! @brief  ctchip_get_imsi_info:查询芯片imsi信息
	//! @param  uint8_t *imsi imsi数据
	//! @param  uint16_t maxImsiLen imsi数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_imsi_info(uint8_t *imsi, uint16_t maxImsiLen);

	//****************************************************************
	//! @brief  ctchip_get_iccid_info:查询芯片iccid
	//! @param  uint8_t *iccid iccid数据
	//! @param  uint16_t maxIccidLen iccid数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_iccid_info(uint8_t *iccid, uint16_t maxIccidLen);

	//****************************************************************
	//! @brief  ctchip_get_apn_info:查询芯片apn
	//! @param  uint8_t *apn apn数据
	//! @param  uint16_t maxApnLen apn数据长度
	//! @retval uint16_t 成功返回0，失败返回错误码
	//****************************************************************
	uint16_t ctchip_get_apn_info(uint8_t *apn, uint16_t maxApnLen);

	//****************************************************************
	//! @brief  ctchip_get_nv:获取数据区首地址
	//! @param void 无参数
	//! @retval uint8_t * 数据区首地址
	//****************************************************************
	uint8_t *ctchip_get_nv(void);

	//****************************************************************
	//! @brief  ctchip_updata_nv:托管写入数据区接口
	//! @param void 无参数
	//! @retval uint16_t 成功返回0,失败返回1
	//****************************************************************
	uint16_t ctchip_update_nv(void);

	//****************************************************************
	//! @brief  ctchip_flush_nv:立即写入数据区接口
	//! @param void 无参数
	//! @retval uint16_t 成功返回0,失败返回1
	//****************************************************************
	#ifdef PLATFORM_XINYI
	uint16_t ctchip_flush_nv(ctlw_file_type_e type);
	#else
	uint16_t ctchip_flush_nv(void);
	#endif

	//****************************************************************
	//! @brief  ctchip_event_ip_status_init:ip事件通知初始化接口
	//! @param  void 无参数
	//! @retval int8_t；成功返回0，失败返回-1
	//****************************************************************
	int8_t ctchip_event_ip_status_init(void);

	//****************************************************************
	//! @brief	ctchip_event_ip_status_destroy:取消ip事件通知接口
	//! @param void 无参数
	//! @retval void 无返回值
	//****************************************************************
	void ctchip_event_ip_status_destroy(void);

	//****************************************************************
	//! @brief  ctchip_event_psm_status_init:psm状态事件通知初始化接口
	//! @param  void 无参数
	//! @retval int8_t；成功返回0，失败返回-1
	//****************************************************************
	int8_t ctchip_event_psm_status_init(void);

	//*****************************************************************
	//! @brief  ctchip_event_psm_status_destroy:取消psm状态事件通知接口
	//! @param void 无参数
	//! @retval void 无返回值
	//*****************************************************************
	void ctchip_event_psm_status_destroy(void);

#ifdef PLATFORM_XYZ
	//*****************************************************************
	//! @brief  ctchip_event_callback:芯片事件通知接口
	//! @param  urcID_t eventID 事件类型
	//! @param  void *param 通知内容
	//! @param  uint32_t paramLen 通知内容长度
	//! @retval int32_t；成功返回0，失败返回-1
	//*****************************************************************
	int32_t ctchip_event_callback(urcID_t eventID, void *param, uint32_t paramLen);
#endif
	//*****************************************************************
	//! @brief  ctchip_get_cell_id:查询cellID接口
	//! @param  int32_t* cellID,返回cellID值
	//! @retval uint16_t;成功返回0,失败返回1
	//*****************************************************************
	uint16_t ctchip_get_cell_id(uint32_t* cellID);

	//*****************************************************************
	//! @brief  ctchip_get_rsrp:查询rsrp接口
	//! @param  int32_t* rsrp,返回rsrp值
	//! @retval uint16_t;成功返回0,失败返回1
	//*****************************************************************
	uint16_t ctchip_get_rsrp(int32_t* rsrp);
	
	//定时任务回调方法定义
	typedef void (*timer_callback_func)(uint8_t id);
	//*****************************************************************
	//! @brief  ctchip_register_timer_callback:注册定时回调处理方法
	//! @param  timer_callback_func timerCB,定时回调处理方法
	//! @retval void:无
	//*****************************************************************
	void ctchip_register_timer_callback(timer_callback_func timerCB);

#ifdef __cplusplus
}
#endif

#endif //_CTLW_PLATFORM_H_
