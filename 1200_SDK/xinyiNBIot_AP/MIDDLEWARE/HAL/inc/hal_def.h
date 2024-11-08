#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "system.h"
#include "common.h"

extern void DisablePrimask(void);
extern void EnablePrimask(void);

/**
 * @brief  状态枚举定义
 */
typedef enum
{
    HAL_OK = 0x00U,      //正常、成功
    HAL_ERROR = 0x01U,   //错误
    HAL_BUSY = 0x02U,    //忙
    HAL_TIMEOUT = 0x03U, //超时
    HAL_LOST = 0x04U     //丢失
} HAL_StatusTypeDef;

/**
 * @brief 设备锁枚举定义
 */
typedef enum
{
    HAL_UNLOCKED = 0x00U,  //设备未上锁
    HAL_LOCKED = 0x01U     //设备处于上锁状态
} HAL_LockTypeDef;

/**
 * @brief 标志或中断状态枚举定义
 */
typedef enum
{
    HAL_RESET = 0,       // 未置位
    HAL_SET = !HAL_RESET // 已置位
} HAL_FlagStatus, HAL_ITStatus;

/**
 * @brief 使能、失能状态枚举定义
 */
typedef enum
{
  HAL_DISABLE = 0,           //失能、禁能
  HAL_ENABLE = !HAL_DISABLE  //使能
} HAL_FunctionalState;

/**
 * @brief  成功、失败状态枚举定义
 */
typedef enum
{
  HAL_SUCCESS = 0U,        //成功
  HAL_FAIL = !HAL_SUCCESS  //失败
} HAL_ErrorStatus;

/*!< 置位某一位*/
#define SET_BIT(REG, BIT) ((REG) |= (BIT))

/*!< 清零某一位*/
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

/*!< 读取某一位*/
#define READ_BIT(REG, BIT) ((REG) & (BIT))

/*!< 清零入参值*/
#define CLEAR_REG(REG) ((REG) = (0x0))

/*!< 给入参赋值*/
#define WRITE_REG(REG, VAL) ((REG) = (VAL))

/*!< 读取入参值*/
#define READ_REG(REG) ((REG))

/*!< 最大延迟时间*/
#define HAL_MAX_DELAY (0xFFFFFFFFU)

/*!< 防止编译器报错*/
#define UNUSED_ARG(X) (void)X

/**
  * @brief  设备上锁函数
  * @param  __HANDLE__：外设控制结构体
  */
#define __HAL_LOCK(__HANDLE__)             \
do{                                        \
    DisablePrimask();                      \
    if((__HANDLE__)->Lock == HAL_LOCKED)   \
    {                                      \
        EnablePrimask();                   \
        return HAL_BUSY;                   \
    }                                      \
    else                                   \
    {                                      \
        (__HANDLE__)->Lock = HAL_LOCKED;   \
    }                                      \
    EnablePrimask();                       \
}while (0U)                            

/**
  * @brief  设备解锁函数
  * @param  __HANDLE__：外设控制结构体
  */
#define __HAL_UNLOCK(__HANDLE__)        \
do{                                     \
    (__HANDLE__)->Lock = HAL_UNLOCKED;  \
}while (0U)

/**
  * @brief  设备上锁函数（全双工设备使用）
  * @param  __HANDLE__：外设控制结构体
  */
#define __HAL_LOCK_RX(__HANDLE__)           \
do{                                         \
    DisablePrimask();                       \
    if((__HANDLE__)->RxLock == HAL_LOCKED)  \
    {                                       \
        EnablePrimask();                    \
        return HAL_BUSY;                    \
    }                                       \
    else                                    \
    {                                       \
        (__HANDLE__)->RxLock = HAL_LOCKED;  \
    }                                       \
    EnablePrimask();                        \
}while (0U)                            

/**
  * @brief  设备解锁函数（全双工设备使用）
  * @param  __HANDLE__：外设控制结构体
  */
#define __HAL_UNLOCK_RX(__HANDLE__)       \
do{                                       \
    (__HANDLE__)->RxLock = HAL_UNLOCKED;  \
}while (0U)

/**
  * @brief 传入值单位为ms。最大不可超过12000ms。
  * @param	ms 具体的延时时间，单位为毫秒.
  * @note  utc_cnt_delay接口为低精度延迟接口，粒度为30us。
  */
#define HAL_Delay(ms)   delay_func_us((ms) * 1000)

/**
 * @brief 用户调用的高精度延时接口，传入值单位为us，使用时必须保证不被中断打断
 * @brief 工作原理：AP通过执行cpu cycle固定的汇编循环语句达到延时效果.
 * @warning 传入参数不得超过12s
 * @warning 接口必须大于最小延时才能正常工作，低于最小延时按照最小延时处理。最小延迟时间计算公式为：3000000/AP的时钟频率*60；
 * 几个经典场景的延时如下：
 * AP时钟 HRC 4分频时，最小延迟不得低于40us；
 * AP时钟 PLL 10分频时，最小延迟不得低于5us
 * @note  utc_cnt_delay接口为低精度延迟接口，粒度为30us。
 */
#define HAL_Delay_US(us)    delay_func_us(us)
