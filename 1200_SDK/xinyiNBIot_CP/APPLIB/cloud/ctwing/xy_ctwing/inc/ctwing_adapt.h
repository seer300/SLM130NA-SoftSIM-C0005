#pragma once
#include "xy_utils.h"
#include "lwip/ip_addr.h"
#include "net_app_resume.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctwing_util.h"



/**
 * @brief 从文件系统中获取config和session文件
 * 
 * @return void* 
 */
void *xy_ctlw_get_sdk_nv(void);

/**
 * @brief Session及config内容保存至文件系统，
 * @note  适配电信SDK ctchip_flush_nv接口
 */
void xy_ctlw_flush_sdk_nv(ctlw_file_type_e type);

void xy_update_SDK_NV(void);

/**
 * @brief CTwing SDK模块启动入口
 * @warning  使用CTwing SDK模块任何功能前必须调用此接口
 * @note     内部流程仅会执行一次
 * @return int32_t
 */
int32_t xy_ctlw_module_entry();

/**
 * @brief 
 * 适配电信SDK xy_ctlw_ctiot_get_context 函数
 * 内存优化,动态申请替代原SDK静态分配
 * @return ctiot_context_t* 
 */
ctiot_context_t * xy_ctlw_ctiot_get_context();

uint32_t xy_ctlw_gettime();

void xy_ctlw_usleep(uint32_t usec);

int xy_ctlw_thread_create(osThreadId_t *thread, const osThreadAttr_t *attr, void *(*start_routine)(void *), void *arg);

int xy_ctlw_thread_exit();

int xy_ctlw_thread_cancel(osThreadId_t thread_id);

int xy_ctlw_mutex_init(osMutexId_t *mutex);

int xy_ctlw_mutex_lock(osMutexId_t *mutex);

int xy_ctlw_mutex_unlock(osMutexId_t *mutex);

int xy_ctlw_mutex_destroy(osMutexId_t *mutex);

uint16_t xy_ctlw_random(void);

int xy_ctlw_cmd_ioctl(ctlw_cmd_e cmd, char *arg, int len);

uint16_t xy_ctlw_get_iptype();

uint16_t xy_ctlw_get_local_ip(char *localIP, int addrFamily);

void xy_ctlw_asyn_notify(char *at_str);

system_boot_reason_e xy_ctlw_get_system_boot_reason(void);

uint16_t xy_ctlw_sync_cstate(void);

uint16_t xy_ctlw_get_imsi_info(uint8_t *imsi,uint16_t maxImsiLen);

uint16_t xy_ctlw_get_imei_info(uint8_t *imei,uint16_t maxImeiLen);

uint8_t xy_ctlw_get_psm_mode(void);

bool xy_ctlw_is_net_in_oos(void);

uint16_t xy_ctlw_get_iccid_info(uint8_t* iccid,uint16_t maxIccidLen);

uint16_t xy_ctlw_get_apn_info(uint8_t* apn,uint16_t maxApnLen);

uint16_t xy_ctlw_get_module_info(uint8_t* sv,uint16_t maxSvLen,uint8_t* chip,uint16_t maxChipLen,uint8_t* module,uint16_t maxModuleLen);

uint16_t xy_ctlw_get_rsrp(int32_t* rsrp);

uint16_t xy_ctlw_get_wireless_signal_info(uint8_t *rsrp, uint16_t maxRsrpLen, uint8_t *sinr, uint16_t maxSinrLen, uint8_t *txPower, uint16_t maxTxPowerLen, uint8_t *cellID, uint16_t maxCellIDLen);

uint16_t xy_ctlw_get_cell_id(uint32_t* cellID);

void xy_ctlw_event_ip_status_init(void);

void xy_ctlw_event_ip_status_destroy(void);

void xy_ctlw_auto_update_rtc_callback();

void xy_ctlw_start_auto_update_rtc(uint32_t timeMs);

void xy_ctlw_stop_auto_update_rtc(void);

