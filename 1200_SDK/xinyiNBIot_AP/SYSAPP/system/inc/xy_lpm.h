#pragma once

/*****************************************************************************************************************************
 * @brief   低功耗相关接口
 ****************************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "mcu_adapt.h"

/**
 * @brief  用户睡眠锁，用户不得随意修改值范围(DEEPSLEEP共24个；standby4个；wfi4个);必须保证锁的排他性使用，因为内部未做累加计数
*/
typedef enum
{
	DSLEEP_BASE = 0,
	XY_NATSPEED_LOCK=DSLEEP_BASE, /* AT+NATSPEED专用 */
	DSLEEP_QSCLK,           /* BC25深睡锁,由出厂NV和AT命令控制 */

	USER_DSLEEP_LOCK1,      /* 用户自行改宏名 */
	USER_DSLEEP_LOCK2,      /* 用户自行改宏名 */
	USER_DSLEEP_LOCK3,      /* 用户自行改宏名 */
	DSLEEP_END=16,

    STANDBY_BASE = DSLEEP_END,
    STANDBY_DEFAULT=STANDBY_BASE,
    STANDBY_AT_RATE_LOCK,   /* at串口波特率锁，超过9600建议上锁 */
	USER_STANDBY_LOCK1,     /* 用户自行改宏名 */
	USER_STANDBY_LOCK2,     /* 用户自行改宏名 */
	STANDBY_END=28,
	
	WFI_BASE = STANDBY_END,
    WFI_DEFAULT=WFI_BASE,  /*启动CP核后进行云通信期间，不能进入WFI，否则会造成不读AT缓存造成处理延误*/
    USER_WFI_LOCK1,       /* 用户自行改宏名 */
	USER_WFI_LOCK2,       /* 用户自行改宏名 */
	USER_WFI_LOCK3,       /* 用户自行改宏名 */
	WFI_END=32,
} LPM_LOCK_TypeDef;
	

/**
 * @brief  CPU低功耗相关的状态机
*/
typedef enum
{
    LPM_ACTIVE = 0,
    LPM_DSLEEP,
    LPM_STANDBY,
    LPM_WFI,
} LPM_MODE;


/*
 * @brief 睡眠锁申请接口，即禁止进入某等级低功耗状态
 * @param lpm_lock_type 锁类型，具体参见@ref LPM_LOCK_TypeDef
 * @note  对于深睡锁LPM_DSLEEP，一般是在唤醒中断中申请，main函数中处理完相关事务后释放
 * @warning  必须保证锁的排他性使用，因为内部未做累加计数；不建议客户使用
*/
void LPM_LOCK(LPM_LOCK_TypeDef lpm_lock_type);

/*
 * @brief 睡眠锁释放接口，即允许芯片进入某等级低功耗状态；不建议客户使用
*/
void LPM_UNLOCK(LPM_LOCK_TypeDef lpm_lock_type);

/*
 * @brief 某睡眠锁是否被持有，若被持有，则表明有相应的事务正在等待执行；不建议客户使用
 * @param lpm_lock_type 锁类型，具体参见@ref LPM_LOCK_TypeDef
*/
bool LPM_LOCK_EXIST(LPM_LOCK_TypeDef lpm_lock_type);

/*
 * @brief 释放AP核所有的睡眠锁；不建议客户使用
 * @note  供用户容错使用，调用该接口仅能保证AP核可进入深睡，CP核还需用户单独执行(如调用Stop_CP)
 */
void LPM_UNLOCK_ALL();

/*
 * @brief   用户在做完相应事务后，调用该接口，指定进入某种睡眠等级。内部会根据入参进行睡眠尝试，若睡眠条件不满足则会自动降级睡眠等级
 * @param   sleepmode: 睡眠等级，@ref LPM_MODE；一般选择LPM_DSLEEP
 *          LPM_DSLEEP模式下,除retention memory部分持续供电外其余部分均掉电，可被UTC、外部PIN唤醒。
 *          LPM_STANDBY模式下，所有外设时钟停止，CPU和内部寄存器状态会保持。可被UTC、外部PIN、GPI产生的中断唤醒。
 *          LPM_WFI模式下所有中断均可唤醒。
*/
bool Enter_LowPower_Mode(LPM_MODE sleepmode);

/**
 * @brief 深睡前HOOK函数注册接口，此时关中断，可以进行FLASH的擦写动作
 * @warning 若用户开启了BAN_WRITE_FLASH宏，则只能在此处执行FLASH擦写，其他地方执行会造成断言
 */
void Into_Dslp_Hook_Regist(pFunType_void pfun);

/*
 * @brief 快速回复用户初始化回调函数注册接口，因为RAM不下电，进而某些关键全局变量需重新赋初始值
 * @note  AON区域的外设(WKUP_EN,GPIO_WKP,UTC,LPTIMER,LPUART,CLKTICK)深睡时不会下电，进而唤醒后无需重新初始化。
 * @warning  非AON区域的外设(CSP1,CSP2,CSP3,TIMER2,ADC,UART2,I2C1,I2C2,SPI)必须按需启停，即XXX_Init-->传输-->XXX_Deinit，不得轻易在此函数中初始化！
 * @warning  RAM不下电，进而慎用static静态全局，以防止唤醒后逻辑出错
 * @note  若用户需要排查快速恢复唤醒后是否有不合理的运行flash代码，可以在该函数里调用Flash_mpu_Lock()来禁止运行flash代码，一旦发生则会断言！
*/
void FastRecov_Init_Hook_Regist(pFunType_void p_fun);


/*
 * @brief  不建议客户使用！时钟源不为PLL，在加载启动CP后，系统的主频会切换到PLL时钟，进而需要通过该函数重新初始化外设。
 * @warning  目前，主要为SPI、CSP2、CSP3三路硬件会受主频切换影响，建议客户不要使用。
 * @note  以下几种外设起CP时主频不会变化：UART2，CSP1，TIMER2，ADC，I2C1，I2C2，UTC，LPTIMER，LPUART，CLKTICK
*/
void BootCp_Init_Hook_Regist(pFunType_void pfun);



/*
 * @brief  不建议客户使用！时钟源不为PLL，在加载启动CP后，系统的主频会切换到PLL时钟；停CP后为降低功耗会切换HRC时钟，进而需要通过该函数重新初始化外设。
 * @warning  目前，主要为SPI、CSP2、CSP3三路硬件会受主频切换影响，建议客户不要使用。
 * @note  以下几种外设起CP时主频不会变化：UART2，CSP1，TIMER2，ADC，I2C1，I2C2，UTC，LPTIMER，LPUART，CLKTICK
*/
void StopCp_Init_Hook_Regist(pFunType_void pfun);





