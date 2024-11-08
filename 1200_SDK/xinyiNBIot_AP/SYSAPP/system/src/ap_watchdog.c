#include "ap_watchdog.h"

extern uint8_t g_wdt_enable;
extern void UTC_WDT_Func(void);
volatile uint32_t g_UTC_RST_FUNC_addr; //存放UTC_WDT_Func()函数地址的实体，存在ram上

/**
 * @brief AP看门狗初始化函数
 * @param WorkMode AP看门狗工作模式。详情参考 @ref AP_WDT_WorkModeTypeDef.
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如客户使用硬看门狗，main入口和p_User_Init_FastRecovery函数指针所指向函数中都必须调用该API
 * @warning 产品调试阶段若发生不合理的硬看门狗重启，可以设置为AP_WDT_WORKMODE_INT模式，在中断函数中排查触发的根源。
 *          该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
void AP_WDT_Init(AP_WDT_WorkModeTypeDef WorkMode, uint32_t sec)
{
    if(g_wdt_enable == 0)
    {
		return;
    }

	PRCM_ClockEnable(CORE_CKG_CTL_AP_WDT_EN);

#if (XY_DEBUG == 0)
    if (WorkMode == AP_WDT_WORKMODE_RESET)
    {
		//AP看门狗跳转至一级boot执行UTC看门狗执行函数的地址，这里赋值为代码起始位置的地址
		g_UTC_RST_FUNC_addr = (uint32_t)UTC_WDT_Func;

		//一级boot从POWERUP_AP_JUMP_ADDR获取得到UTC看门狗执行函数的地址，如同从arm bin的起始地址获取到reset handler的方式
        HWREG(POWERUP_AP_JUMP_ADDR) = ((uint32_t)&g_UTC_RST_FUNC_addr) - 4;

        //AP看门狗RESET模式记录
        HWREGB(AON_AONGPREG0) = 0x42;

        //配置AP看门狗为RESET模式
		AP_WDT->CON &= ~(WDT_CTL_WATCHDOG_EN | WDT_CTL_INT_EN);
		AP_WDT->CON |= (WDT_CTL_WATCHDOG_EN | WDT_CTL_RST_EN);
        WDT_ReloadSet(AP_WDT, XY_UTC_CLK * sec);
	}
	else
#endif
	//DEBUG模式一定是中断模式，以方便问题定位
	{
		(void)WorkMode;
        
        //配置AP看门狗为中断模式
        AP_WDT->CON &= ~(WDT_CTL_WATCHDOG_EN | WDT_CTL_RST_EN);
        WDT_ReadClearInt(AP_WDT);
        AP_WDT->CON |= (WDT_CTL_WATCHDOG_EN | WDT_CTL_INT_EN);
        WDT_ReloadSet(AP_WDT, XY_UTC_CLK * sec);

        //配置中断优先级，并使能中断
		NVIC_SetPriority(WDT_IRQn, 0);
		NVIC_EnableIRQ(WDT_IRQn);
	}
}

/**
 * @brief 用于秒级的周期性喂狗，通常在main主函数的while循环里执行喂狗操作。耗时几us
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如果用户按照工作容许的最大时长来设置，则无需调用该接口进行喂狗，一旦超时则表明未能正常进入芯片深睡，触发看门狗异常。
 * @warning 该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
__RAM_FUNC void AP_WDT_Refresh(uint32_t sec)
{
	if(g_wdt_enable == 0)
    {
        return;
    }
	DisablePrimask();
    WDT_ReloadSet(AP_WDT, XY_UTC_CLK * sec);
    EnablePrimask();
}

/**
 * @brief 使能AP看门狗
 */
void AP_WDT_Enable(void)
{
	if(g_wdt_enable == 0)
    {
		return;
    }
	WDT_Enable(AP_WDT);
}

/**
 * @brief 禁能AP看门狗
 */
void AP_WDT_Disable(void)
{
	WDT_Disable(AP_WDT);
}

/**
 * @brief AP看门狗中断模式(AP_WDT_WORKMODE_INT)回调函数注册，用户可根据需求重新定义，例如进行产品级容错等。
 * @warning 硬看门狗异常，不保证系统一定能正常处理中断回调！进而建议客户使用UTC全局看门狗来二次监控
 */
pFunType_void p_WDT_Int_func = NULL;
void AP_WDT_Int_Reg(pFunType_void p_fun)
{
	p_WDT_Int_func = p_fun;
	mark_dyn_addr(&p_WDT_Int_func);
}
