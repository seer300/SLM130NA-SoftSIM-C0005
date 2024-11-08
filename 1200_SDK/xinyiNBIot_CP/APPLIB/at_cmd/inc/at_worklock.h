#pragma once

/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_utils.h"

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/


int at_WORKLOCK_req(char *at_buf, char **prsp_cmd);



/**
 * @brief 收到AT命令时，锁DEEPSLEEP/STANDBY睡眠一段时间。通过出厂NV参数deepsleep_delay配置延迟时长
 * @return XY_OK：延迟锁激活成功，XY_ERR:激活失败
 * @note  非唤醒开机及AT唤醒，都会调用该接口。
 */
int at_delaylock_act();


/**
 * @brief 去激活AT命令延迟锁
 */
int at_delaylock_deact();

/**
 * @brief 通过QSCLK命令来控制睡眠等级
 */
void set_qsclk_lock(uint8_t qsclk_mode);

/**
 * @brief worklock全局资源初始化
 */
void sleep_lock_init();


