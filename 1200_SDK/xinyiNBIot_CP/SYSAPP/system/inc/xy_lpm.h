#pragma once

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/


typedef enum
{
	LPM_WFI          = 1 << 0,     
    LPM_STANDBY      = 1 << 1,      
    LPM_DEEPSLEEP    = 1 << 2,      
	LPM_ALL = LPM_STANDBY | LPM_DEEPSLEEP, 
} Lock_Type_E;





/**
 * @brief 创建工作锁，最多可创建 32 把。
 * @param lock_name [IN]  锁名，字符串
 * @return  锁句柄，正常值为0-31。若返回-1表示创建失败
 * @warning 若重复创建，返回相同的句柄
 */
int8_t create_sleep_lock(char* lock_name);

/**
 * @brief 锁住某种睡眠，不允许进入该睡眠。
 * @param lockfd [IN]  
 * @param sleep_type [IN]  睡眠等级位图，可组合
 */
void sleep_lock(int8_t lockfd, Lock_Type_E sleep_type);

/**
 * @brief 释放某种睡眠锁。仅当该等级休眠的所有锁都被释放时，才允许系统进入该休眠。
 * @param lockfd [IN]  
 * @param sleep_type [IN]  睡眠等级位图，可组合
 */
void sleep_unlock(int8_t lockfd, Lock_Type_E sleep_type);

/**
 * @brief 释放某种等级睡眠的所有锁。
 * @param sleep_type [IN]  
 */
void clear_sleep_lock(Lock_Type_E sleep_type);

/**
 * @brief 删除某睡眠锁。
 * @param lockfd [IN]
 */
void delete_sleep_lock(int8_t lockfd);

/**
 * @brief 判断是否存在某种睡眠锁,当入参为组合睡眠锁时，只要有其中一种睡眠被锁住便返回1
 * @param lockfd [IN]
 * @param sleep_type [IN]
 * @return	-1 未创建
 * 			0 无锁
 * 			1 有锁
 */
int8_t get_lock_stat(int8_t lockfd, Lock_Type_E sleep_type);


/**
 * @brief 获取某种睡眠锁的数量
 * @param sleep_type [IN]
 */
uint8_t get_sleep_lock_num(Lock_Type_E sleep_type);

/**
 * @brief 判断某种等级睡眠是否被锁住
 * @param sleep_type [IN]
 */
uint8_t is_sleep_locked(Lock_Type_E sleep_type);



/**
 * @brief 网络业务延迟锁，用于解决RTC唤醒场景下，业务状态机存在osDelay操作导致快速进入深睡无法执行业务流程的问题
 * @param  timeout_ms[IN] 延迟锁超时时间，单位毫秒.0表示无限长，即始终持有锁
 * @return XY_ERR表示延迟锁申请失败，XY_OK表示成功
 * @note 超时时间不宜过大,小于1秒为佳
 */
int app_delay_lock(uint32_t timeout_ms);

void app_delay_unlock();



