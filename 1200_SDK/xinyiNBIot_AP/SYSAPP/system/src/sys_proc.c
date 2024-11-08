/*
 * @file  sys_proc.c
 * @brief 操作CP相关接口以及wakeup相关接口，包括wakeup配置、wakeup上电原因查询等
 * @warning
 */
#include "hw_types.h"
#include "prcm.h"
#include "xy_system.h"
#include "sys_ipc.h"
#include "sys_proc.h"
#include "xy_cp.h"
#include "utc.h"
#include "dma.h"
#include "tick.h"
#include "xy_flash.h"
#include "rf_drv.h"
#include "sema.h"
#include "common.h"
#include "hal_lpuart.h"
#include "xy_at_api.h"
#include "flash_vendor.h"
#include "xy_event.h"
#include "module_runtime_dbg.h"
#include "sys_rc32k.h"
#include "nvic.h"
#include "at_uart.h"
#include "adc.h"

extern HAL_LPUART_HandleTypeDef g_at_lpuart;

/*压缩读寄存器耗时，变量判断*/
#if(LLPM_VER == 1)
volatile uint8_t g_cpcore_is_alive = 0;
#endif

/*因为读AON寄存器慢，进而在Reset_Handler入口处读取，供后续多函数全局读取使用。对应WAKEUP_STATUS_LPMTMR_WKUP_Msk。*/
volatile uint32_t wakeup_info;

/**
 * @brief 启CP时用户硬件初始化注册接口
 */
pFunType_void p_User_HwInitbyLoadCp = NULL;
__FLASH_FUNC void BootCp_Init_Hook_Regist(pFunType_void pfun)
{
	p_User_HwInitbyLoadCp = pfun;
	mark_dyn_addr(&p_User_HwInitbyLoadCp);
}

/**
 * @brief 软硬复位，用户注册的回调
 */
pFunType_u8 p_User_Reset_Proc = NULL;
__FLASH_FUNC void Pre_Reset_Hook_Regist(pFunType_u8 pfun)
{
    p_User_Reset_Proc = pfun;
	mark_dyn_addr(&p_User_Reset_Proc);
}


/*主要为SPI、CSP2、CSP3三路硬件会受主频切换影响,慎用！*/
__FLASH_FUNC void Hw_Init_By_LoadCP(void)
{
	if(p_User_HwInitbyLoadCp != NULL)
    {
        p_User_HwInitbyLoadCp();
    }
}

/**
 * @brief 停CP时用户硬件初始化注册接口
 */
pFunType_void p_User_HwInitbyStopCp = NULL;
__FLASH_FUNC void StopCp_Init_Hook_Regist(pFunType_void pfun)
{
	p_User_HwInitbyStopCp = pfun;
	mark_dyn_addr(&p_User_HwInitbyStopCp);
}

/*主要为SPI、CSP2、CSP3三路硬件会受主频切换影响,慎用！*/
void Hw_Init_By_StopCP(void)
{
	if(p_User_HwInitbyStopCp != NULL)
    {
        p_User_HwInitbyStopCp();
    }
}

void Hold_CP_Core()
{
    AON_CP_Core_Hold();
#if(LLPM_VER == 1)
    g_cpcore_is_alive = 0;
#endif
}

void Release_CP_Core()
{
   // Release CP Core
    AON_CP_Core_Release();
#if(LLPM_VER == 1)
    g_cpcore_is_alive = 1;
#endif
}

/*用于软件复位失败的异常监控，需要考虑二级boot执行FOTA升级的时长*/
#define SOFTRESET_WDT_TIMEOUT   (8*60)   


/*CP核软重启在pendsv中执行该函数，此处未检查是否有AT输出，因为CP核已经做了延迟，保证重要URC能够发送完*/
void do_soft_reset(Soft_Reset_Type soft_reset_type)
{
    if(p_User_Reset_Proc != NULL)
    {
        p_User_Reset_Proc(soft_reset_type);
    }
	
#if (AT_LPUART == 1)
	at_uart_wait_send_done();     // 等待LPUART数据发送完成，若数据一直发送不完则2秒后自动退出
	HAL_LPUART_DeInit(&g_at_lpuart);   // 失能LPUART，关LPUART时钟，确保重启后可重配AT通道
#endif

	/*防止复位期间异常，开启UTC看门狗*/
    UTC_WDT_Init(SOFTRESET_WDT_TIMEOUT);

    Disable_UTC_Alarm(); //软重启前去使能RTC定时器，CP侧所有的RTC定时器都失效
    
	HWREG(BAK_MEM_AP_UP_SUBREASON) = soft_reset_type;

#if (MODULE_VER == 0x0)	// OPENCPU形态，支持快速恢复
	HWREGB(AON_AONGPREG0) &= (~0x43);
#endif
	//规避NRB后boot otp读错卡死问题
	PRCM_SlowfreqDivEnable();
    // bug8248,系统时钟选择hrc
	Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
    
	HWREGB(AON_SYSCLK_CTRL2) = 0x03; // pll&xtal force off
	//切换sido mode
	SIDO_LPMODE();

#if (LLPM_VER == 1)
    //PRCM_CpIntWakeupEnable();
    //PRCM_ApCpIntWkupTrigger();//如果复位时vddc正处于mcu模式，先通过发核间中断方式退出mcu模式；ap boot可以正常切换sido normal状态
    PRCM_ForceCPOff_Disable();//退出mcu模式
#endif

    // 只有在DMA时钟打开的情况下，才去查看DMA状态寄存器，等待DMA传输完成
    if (COREPRCM->CKG_CTRL_L & (uint32_t)CORE_CKG_CTL_DMAC_EN)
    {
        //bug8993 fota流程reset后会卡1级boot。重启前开关flashvcc，待机观察问题是否还会复现.该接口需要在ram上
        if(DMAChannelGetStartStatus(DMA_CHANNEL_0) == 1 )
        {
            while(DMAChannelTransferRemainCNT(DMA_CHANNEL_0));//flash dma chn
        }
    }

    extern void flash_off_and_on();
    flash_off_and_on();

    // 防止软复位后，IOLDO模式变化导致电压出现波动
    PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);
    PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_Enable);

#if XY_DUMP  //类似bug9523问题，调试代码，怀疑AON_RST_CTRL设置未生效
	if(soft_reset_type == SOFT_RB_BY_RESET)
		xy_assert(!(HWREGB(AON_RST_CTRL) & 0x80));
#endif
    PRCM_SramPowerModeSet(CP_SRAM0|CP_SRAM1, SRAM_POWER_MODE_FORCE_ON);

#if (LLPM_VER == 1)
    AONPRCM->SYSCLK_CTRL &= ~0x1100;//表计模式force stop 32k as sysclk,加快唤醒，睡眠失败/唤醒后需清除，否则切换时钟时可能有问题
#endif

	// 软重启，AON区和RET MEM保持为软重启前状态
	HWREGB(COREPRCM_BASE + 0x06) = 0x01;
    while(1);
}

__FLASH_FUNC void xy_Soft_Reset(Soft_Reset_Type soft_reset_type)
{
	//若utc中断中调用boot_cp接口此处需添加
    //若此时cp处于睡眠唤醒阶段，由于失能utc_alarm会导致CP卡死在等待phytmr run的接口里（XY_DEBUG宏为1时，NRB偶现双核卡死；为0时可以强制reset CP)
	//Disable_UTC_Alarm();

	Stop_CP(0);

	DisablePrimask(); //防止中断，强行复位会有异常

	do_soft_reset(soft_reset_type);
}



__OPENCPU_FUNC unsigned long Global_Reset_Upstate_Get(void)
{
    uint32_t Global_Reset_stat = 0;
    volatile uint8_t up_stat = AONPRCM->UP_STATUS;

    if (up_stat & AON_UP_STAT_EXTPIN_Msk)
    {
        Global_Reset_stat = PIN_RESET;
    }
    else if (up_stat & AON_UP_STAT_LVDRST_Msk)
    {
        Global_Reset_stat = LVD_RESET;
    }
    else if (up_stat & AON_UP_STAT_AONWDT_Msk)
    {
        Global_Reset_stat = WDT_RESET;
    }
    else if (up_stat & AON_UP_STAT_SOFTRST_Msk)
    {
        Global_Reset_stat = SOC_RESET;
    }
    else if (up_stat & AON_UP_STAT_DFTGLB_Msk)
    {
        Global_Reset_stat = DFTGLB_RESET;
    }
    else
        Global_Reset_stat = SVD_RESET;

    return Global_Reset_stat;
}

__OPENCPU_FUNC uint32_t Wakeup_Deepsleep_Upstate_Get(void)
{
    uint32_t Wakeup_Deepsleep_stat = 0;

    // wakeup from Deepsleep
    if (wakeup_info & WAKEUP_STATUS_LPUART_WKUP_Msk)
    {
        Wakeup_Deepsleep_stat |= (1 << AT_WAKUP);
    }
    if (wakeup_info & WAKEUP_STATUS_UTC_WKUP_Msk)
    {
        Wakeup_Deepsleep_stat |= (1 << UTC_WAKUP);
    }
    
    return Wakeup_Deepsleep_stat;
}

bool Is_WakeUp_From_Dsleep()
{
	return  (Get_Boot_Reason() == WAKEUP_DSLEEP);
}

__OPENCPU_FUNC unsigned long Sys_Upstate_Get(void)
{
    volatile uint8_t up_stat;

    up_stat = AONPRCM->UP_STATUS;

    wakeup_info = AONPRCM->WAKEUP_STATUS;

    // 放在这里的原因是，软重启后UP_STATUS寄存器标志位可能和其他上电原因相同造成误判，故在这里先进行判断
    if(HWREGB(COREPRCM_BASE + 0x07) & 0x01)
    {   
        HWREGB(COREPRCM_BASE + 0x07) = 0x01;
        return SOFT_RESET;
    }
    else if (wakeup_info & WAKEUP_STATUS_DSLEEP_WKUP_Msk)
    {
        return WAKEUP_DSLEEP;
    }    
    else if (up_stat & AON_UP_STAT_POR_Msk)
    {
        return POWER_ON;
    }
    else
    {
        return GLOBAL_RESET;
    }
}


/*该函数用于系统初始化时第一时间获取SoC上电原因，仅允许调用一次！后续要获取，直接调用Get_Boot_Reason即可*/
__OPENCPU_FUNC Boot_Reason_Type get_sys_up_stat(void)
{
    Boot_Reason_Type up_reason = Sys_Upstate_Get();

	if(POWER_ON == up_reason)
    {
        HWREG(BAK_MEM_AP_UP_SUBREASON) = 0; // POWER_ON没有子上电原因
    }
    else if (GLOBAL_RESET == up_reason)
    {
        HWREG(BAK_MEM_AP_UP_SUBREASON) = Global_Reset_Upstate_Get();
    }
	/*理论上即使模组也可能存在同时多中断源触发，如RTC与LPUART*/
    else if (WAKEUP_DSLEEP == up_reason)
    {
        HWREG(BAK_MEM_AP_UP_SUBREASON) = Wakeup_Deepsleep_Upstate_Get();
    }
	//软重启时子上电原因本来就保存在BAK_MEM_AP_UP_SUBREASON中，无需再次获取

    HWREGB(BAK_MEM_AP_UP_REASON) = up_reason;

    return up_reason;
}

extern int g_local_fota;
//处理cp核的复位请求，目前所有软件复位动作全由ap执行
__OPENCPU_FUNC void Proc_Cp_Reset_Req(uint32_t *reset_reason)
{
	/*本地FOTA升级，即将复位之前，清除FOTA指示，以防止Fota_Reboot_Init中启动CP核*/
	if(g_local_fota == 1)
		HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 0;

	DisablePrimask();
	
	Hold_CP_Core();

	do_soft_reset(*reset_reason);
}


// 整个芯片复位，功能相当于PIN_RESET按键复位
void xy_Soc_Reset()
{
    if(p_User_Reset_Proc != NULL)
    {
        p_User_Reset_Proc(0XFF);
    }
		
    DisablePrimask();

#if (AT_LPUART == 1)
    // 等待LPUART数据发送完成，若数据一直发送不完则2秒后自动退出
    at_uart_wait_send_done();
#endif

    PRCM_SocReset();

    while(1);
}


