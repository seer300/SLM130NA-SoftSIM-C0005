/*****************************************************************************************************************************
 * @brief  一些基础系统级的API接口，包括中断相关、时钟相关、内存申请等
 *****************************************************************************************************************************/

#pragma once

#include "system.h"
#include "sys_mem.h"
#include "xy_lpm.h"
#include "xy_memmap.h"
#include "xy_timer.h"
#include "xy_printf.h"
#include "xy_at_api.h"
#include "hw_gpio.h"
#include "xy_cp.h"
#include "dyn_load.h"

/* @brief 软重启的细分类型，由xy_Soft_Reset调用设置 */
typedef enum
{
    SOFT_RB_BY_NRB,             //  AT+NRB重启
    SOFT_RB_BY_RESET,           //  AT+RESET软重启，恢复出厂设置，类似重新烧录版本
    SOFT_RB_BY_FOTA,            //  FOTA软重启
    SOFT_RB_BY_LOW_VOL,         //  low vBAT软重启，暂未使用
    SOFT_RB_BY_AP_USER,         //  AP用户调用xy_Soft_Reset触发
    SOFT_RB_BY_CP_USER,         //  CP用户调用xy_Soft_Reset触发
} Soft_Reset_Type;

/* @brief 芯片重启细分原因 */
typedef enum
{
    PIN_RESET,                  //  按键复位，整个芯片复位
    SOC_RESET,                  //  CP核看门狗 + xy_Soc_Reset + assert异常断言
    WDT_RESET,                  //  AP_WDT/UTC_WDT复位
    LVD_RESET,                  //  LVD复位
    SVD_RESET,                  //  SVD复位
    DFTGLB_RESET,               //  DFTGLB复位
} Global_Reset_Type;

/* @brief 不建议客户使用！深睡唤醒细分原因，目前仅供模组形态调试使用！ */
/* @warning 由于可能存在多个唤醒源并发场景，进而造成唤醒原因不准，不建议客户使用细分原因 */
typedef enum
{
    AT_WAKUP,        //AT的LPUART唤醒
    UTC_WAKUP,       //UTC唤醒,仅CP核使用
} Wake_Deepsleep_Type;

/***************************************************************************************************************************** 
 *@brief  系统开机原因，从SOC寄存器中读取.
 *@warning  SOFT_RESET、WAKEUP_DSLEEP上电情况下，retention memory内存保持不变，由软件进行差异化使用
 *****************************************************************************************************************************/
typedef enum
{
    POWER_ON = 1,            //  正常断电后上电，OPENCPU形态的stop_CP复用该状态，子原因为1
    GLOBAL_RESET,            //  断言等软件异常的全局复位，子类型参看Global_Reset_Type
    SOFT_RESET,              //  通过NRB/xy_Soft_Reset软复位，子类型参看Soft_Reset_Type
    WAKEUP_DSLEEP,           //  深睡唤醒，子类型参看Wake_Deepsleep_Type
} Boot_Reason_Type;

//#define xy_zalloc(ulSize)       xy_zalloc_r(ulSize)

/****************************************** 芯翼内存申请接口 ******************************************/
#define xy_malloc(ulSize)       xy_malloc_r(ulSize)  //if malloc fail,will assert
#define xy_malloc2(ulSize)      xy_malloc_r2(ulSize) //if malloc fail,will return NULL 
#define xy_free(mem)            xy_free_r(mem)

/*
 *@brief  指示当前系统是否运行在中断服务程序中，包括pendSv中断 
*/
#define IS_IRQ_MODE()         (__get_IPSR() != 0U)

/*
 *@brief  指示当前系统是否已关中断 
*/
#define IS_IRQ_MASKED()       ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U))

/****************************************** 芯翼断言封装接口 ******************************************/
extern char* g_assert_file;
extern uint32_t g_assert_line;
extern char* g_cp_assert_file;
extern uint32_t g_cp_assert_line;

#if AT_LPUART
#define xy_assert(a)     \
	  do                     \
	  {                      \
	    if (!(a))            \
	    {                    \
		  g_assert_file = (char*)__FILE__, g_assert_line = (uint32_t) __LINE__;\
	      sys_assert_proc(); \
	    }                    \
	  } while (0)
#else
#define xy_assert(a)     \
	  do                     \
	  {                      \
	    if (!(a))            \
	    {                    \
	      sys_assert_proc(); \
	    }                    \
	  } while (0)
#endif


/*该断言主要用于低级错误检查(如入参错误)，以排查出用户编程时未判断API返回值造成的异常*/
#if XY_DUMP
#define debug_assert(a)  xy_assert(a)
#else
#define debug_assert(a)    
#endif

/******************************************************************************************************************************
 * @brief  获取芯片的上电原因，具体参看Boot_Reason_Type
 *****************************************************************************************************************************/
#define Get_Boot_Reason() HWREGB(BAK_MEM_AP_UP_REASON)

/******************************************************************************************************************************
 * @brief   获取基于芯片上电原因的细分子原因，如Global_Reset_Type、Soft_Reset_Type、Wake_Deepsleep_Type
 * @warning 不建议用户区分深睡唤醒的子原因！由于可能存在多个唤醒源并发场景，进而造成WAKEUP_DSLEEP子原因不准
 *****************************************************************************************************************************/
#define Get_Boot_Sub_Reason()  HWREG(BAK_MEM_AP_UP_SUBREASON)

/**
 * @brief 指示芯片是否从深睡模式唤醒
 */
bool Is_WakeUp_From_Dsleep();

/******************************************************************************************************************************
 * @brief   动态获取CPU主频时钟，由于时钟精度，会有一定误差。AP核默认为PLL分频时钟36.8M，表计等特殊产品可配置为13M。
 * @warning 由于启动CP核后必须使用PLL时钟，进而会造成AP核部分外设会被切换频率，具体参阅《引脚和平台外设使用说明》
 *****************************************************************************************************************************/
uint32_t GetAPClockFreq(void);

/**
 * @brief   关闭高于0的中断，0优先级中断包括：watchdog、SVD
 * @warning 内部有计数器，需要与EnableInterrupt配对使用。
 */
void DisableInterrupt(void);

/**
 * @brief   开启中断，与DisableInterrupt配对使用
 * @warning 内部有计数器，需要与DisableInterrupt配对使用。
 */
void EnableInterrupt(void);


/******************************************************************************************************************************
 * @brief   开启中断
 * @warning 内部有计数器，需要与DisablePrimask配对使用。
 *****************************************************************************************************************************/
void EnablePrimask(void);

/******************************************************************************************************************************
 * @brief   关闭所有中断，除了NMI不可屏蔽中断
 * @warning 内部有计数器，需要与EnablePrimask配对使用。
 *****************************************************************************************************************************/
void DisablePrimask(void);

/******************************************************************************************************************************
* @brief   执行软重启，由业务软件调用，aon区域和retention memory内存区域供电保持，flash内容不会被损坏
*****************************************************************************************************************************/
void xy_Soft_Reset(Soft_Reset_Type soft_reset_type);

/******************************************************************************************************************************
 * @brief  芯片级复位，所有外设均会断电再上电,等效于硬重启PIN_RESET，flash内容可能被损坏
 *****************************************************************************************************************************/
void xy_Soc_Reset();

/******************************************************************************************************************************
 * @brief  供用户注册的回调函数。该回调函数供AP核执行软或硬复位之前调用，以紧急执行用户的私有行为，不建议执行FLASH操作
 * @param  回调的入参为重启的类型，其中0XFF表示SoC硬复位，其他正值参看Soft_Reset_Type
 * @note   该回调在xy_Soft_Reset或xy_Soc_Reset或FOTA重启之前执行，进而只能处理实时的事务，如写E2、写GPIO等，无法再响应中断等调度行为。
 * @warning 由于断言等异常复位也会执行该回调，进而不建议客户在回调中执行flash擦写等动作。如必须执行FLASH，建议识别入参的复位类型后进行细分处理
 *****************************************************************************************************************************/
void Pre_Reset_Hook_Regist(pFunType_u8 pfun);

/******************************************************************************************************************************
 * @brief 在main入口处调用,以重配置AP核JLINK的PIN脚。例如AP_Jlink_Reset(GPIO_PAD_NUM_9,GPIO_PAD_NUM_10)
 * @attention   GPIO8(AP_SWDCLK)、GPIO13(AP_SWDIO)为硬件默认的AP_SWD接口，软件可不做任何配置即可使用。
 *****************************************************************************************************************************/
void AP_Jlink_Reset(GPIO_PadTypeDef clk_pad,GPIO_PadTypeDef io_pad);
void CP_Jlink_Reset(GPIO_PadTypeDef clk_pad, GPIO_PadTypeDef io_pad);

/******************************************************************************************************************************
 * @brief  使用RC32K时钟的opencpu用户需周期性调用本接口以触发RC32K校准，目的是提高RC32精度.
 *         为使RC性能最优，建议每隔半小时调用一次！芯翼建议opencpu用户利用自身的周期性事件伴随调用，无须新增专用的定时事件！
 *         Eg. 假设opencpu系统的采样定时器周期为1min，则每采样30次时调用一次本接口即可，
 *         模组用户无须关心！使用xtal 32k的用户也无须关心！
 *****************************************************************************************************************************/
void Set_RC32K_Cali_Event(void);
