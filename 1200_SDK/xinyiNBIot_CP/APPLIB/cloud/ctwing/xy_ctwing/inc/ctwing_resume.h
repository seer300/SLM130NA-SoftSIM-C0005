#pragma once
#include "lwip/ip_addr.h"
#include "xy_rtc_api.h"

/**
 * @brief 创建SDK恢复流程结束信号量
 * @note  
 */
void xy_ctlw_create_sdk_resume_sem();


/**
 * @brief 释放SDK恢复流程结束信号量
 * 
 */
void xy_ctlw_release_sdk_resume_sem();

/**
 * @brief 获取SDK恢复流程结束信号量
 * 
 */
void xy_ctlw_acquire_sdk_resume_sem();

/**
 * @brief 删除SDK恢复流程结束信号量
 */
void xy_ctlw_delete_sdk_resume_sem();


/**
 * @brief 检查session会话文件是否存在
 * 
 * @return true 存在
 * @return false 不存在
 */
bool xy_ctlw_check_if_session_file_exist(void);

/**
 * @brief CTwing业务恢复接口
 * @note 目前使用场景：
 * 1、用于主动update的RTC深睡唤醒 2、下行数据触发的深睡唤醒 3、本地FOTA升级后的上报恢复
 */
bool xy_ctlw_resume(void);


/**
 * @brief 主动update RTC发起的业务恢复任务
 * 
 */
/**若唤醒原因为深睡唤醒,仅唤醒后第一次执行callback需要考虑业务线程的恢复,线程退出时句柄不置NULL*/
void xy_ctlw_auto_update_rtc_resume_task(void);



