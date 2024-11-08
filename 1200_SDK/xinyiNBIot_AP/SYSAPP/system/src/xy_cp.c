#include "prcm.h"
#include "xy_system.h"
#include "sys_ipc.h"
#include "sys_rc32k.h"
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
#include "hw_gpio.h"
#include "sys_mem.h"
#include "xinyi2100.h"
#include "dyn_load.h"
#include "at_uart.h"
#include "err_process.h"

/**
 * @brief  ICM_CHANGE_CP_STATE消息的子动作ID
 */
typedef enum
{
	CP_STOP = 0, /*!< 通知CP核停止运行*/
	CP_RAI,		 /*!< 通知CP核执行RAI流程，以加速回落idle态*/
	CP_CFUN,
	CP_MAX,
} ICM_CP_MsgID;

extern void Ap_HclkDivSet(uint32_t ClockDiv);

/*压缩读寄存器耗时，变量判断*/
#if(LLPM_VER == 1)
extern volatile uint8_t g_cpcore_is_alive;
#endif

bool CP_Is_Alive(void)
{
#if(LLPM_VER == 1)
    return (g_cpcore_is_alive) ? true : false;
#else
    return (HWREGB(AON_RST_CTRL) & 0x80) ? true : false;
#endif
}
__FLASH_FUNC void CP_Jlink_Config(void)
{
	GPIO_InitTypeDef GPIO_Handle = {0};

	uint8_t swd_clk_pin = READ_FAC_NV(uint8_t,swd_swclk_pin);
	uint8_t swd_io_pin = READ_FAC_NV(uint8_t,swd_swdio_pin);

	if((swd_clk_pin != 0xFF)&&(swd_io_pin != 0xFF))
	{
		GPIO_AllocateRemove(GPIO_CP_SWCLKTCK);
		GPIO_AllocateRemove(GPIO_CP_SWDIOTMS);

		GPIO_Handle.Pin = swd_clk_pin;
		GPIO_Handle.PinRemap = GPIO_CP_SWCLKTCK;
		GPIO_Handle.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&GPIO_Handle, NORMAL_SPEED);
		GPIO_InputPeriSelect(swd_clk_pin,GPIO_CP_SWCLKTCK);
		GPIO_InputPeriSelectCmd(GPIO_CP_SWCLKTCK,ENABLE);

		GPIO_Handle.Pin = swd_io_pin;
		GPIO_Handle.PinRemap = GPIO_CP_SWDIOTMS;
		GPIO_Handle.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&GPIO_Handle, NORMAL_SPEED);
		GPIO_InputPeriSelect(swd_io_pin,GPIO_CP_SWDIOTMS);
		GPIO_InputPeriSelectCmd(GPIO_CP_SWDIOTMS,ENABLE);
	}
}

extern volatile int g_Flash_Have_Init;
extern void wait_cp_flash_write_done(void);
extern void Hw_Init_By_LoadCP(void);
__FLASH_FUNC bool Boot_CP(uint32_t wait_ms)
{
	uint32_t start_tick = 0;

#if XY_DUMP
    if(IS_IRQ_MODE())
    {
        xy_assert(0);
    }
#endif
	xy_assert(wait_ms != 0);

    clear_event(EVENT_BOOT_CP);
    if (CP_Is_Alive() == true)
    {
        return 1;
    }

    Debug_MODULE_Runtime_Add("g_db_boot_cp_start");
	
	xy_logview_switch_cp();

	clear_at_info();

    //快速恢复时未开dma时钟
    xy_Flash_Init();

    //power on cpcore
    //PWRCFG_Mode_On(CPCORE_PWR_IN_DEEPSLEEP);

    //非动态加载版本，bootcp时才开启cp sram
	PRCM_SramPowerModeSet(CP_SRAM0|CP_SRAM1, SRAM_POWER_MODE_OFF_DSLEEP);

    IPC_Init();

    // CP 独占UTC资源用作深睡唤醒，为避免双核配置寄存器冲突，由主控核统一规划
	PRCM_AonWkupSourceEnable(AON_WKUP_SOURCE_UTC);
    PRCM_CpIntWakeupEnable();

#if (LLPM_VER == 1)
    //屏蔽CP唤醒源，唤醒时进入mcu mode
    HWREG(AONPRCM_BASE + 0x64) = 0x01;//cp仅关心utc唤醒
    // PRCM_CpIntWakeupEnable();
    // PRCM_ApCpIntWkupTrigger();//退出mcu模式
    PRCM_ForceCPOff_Disable();
    delay_func_us(100);
#else
    //模组或者非表计opencpu，cp关注的唤醒源
    HWREG(AONPRCM_BASE + 0x64) = 0x01;//cp仅关心utc唤醒
#endif

    Ap_HclkDivSet(10);
  
    if(SYSCLK_SRC_PLL != PRCM_SysclkSrcGet())
    {
        Debug_MODULE_Runtime_Add("g_db_boot_pll_lock_start");

        SIDO_NORMAL_INIT();
	    START_PLL();
        //等待PLL锁定
        while (!BBPLL_Lock_Status_Get());
        //切时钟过程关中断
        DisablePrimask();
        Sys_Clk_Src_Select(SYSCLK_SRC_PLL);
        EnablePrimask();

        Debug_MODULE_Runtime_Add("g_db_boot_pll_lock_end");
    }
    //解决启动CP时候的ioldo纹波问题
    if(g_nv_ioldo1_mode)
    {
        PRCM_IOLDO1_ModeCtl(IOLDO1_NORMAL_Enable);
    }
    else
    {
        PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);
    }

    // bug12672 解决simvcc打开，ioldo2电压被下拉500mv
    PRCM_IOLDO2_ModeCtl(IOLDO2_NORMAL_Enable);

    PRCM_ClockEnable(CORE_CKG_CTL_MCNT_EN);
    PRCM_ClockEnable(CORE_CKG_CTL_SEMA_EN);

    ADC_TsensorPowerEN(); 
	//OTP_Read();
	/*表计类功耗极致产品，单核时使用的是低频RC时钟，一旦启动CP核必须切换为高频的PLL时钟，进而需要重新初始化外设，具体参阅《低功耗开发指南》*/
    Hw_Init_By_LoadCP();

    /*仅AP输出明文log,CP核不输出任何LOG*/
	if(HWREGB(BAK_MEM_AP_LOG) == 7)
	{
		xy_print_uart_Init(XY_LOGVIEW_AP_RATE);//涉及主频切换，此时CSP3_UART必须重新初始化一次
	}

    CP_Jlink_Config();
    
    // Enable CP Memory Remap
    AON_CP_Memory_Remap_Enable();

	/*CP核读取使用的状态机，需要在此初始化*/
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_DEFAULT_STATUS;
	HWREG(BAK_MEM_FLASH_NOTICE) = 0;
	HWREGB(BAK_MEM_ATUART_STATE) = 0;
	HWREGB(BAK_MEM_CP_DO_DUMP_FLAG) = 0;
	HWREGB(BAK_MEM_DUMP_LOGVIEW_FLAG) = 0;
	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;
	HWREGB(BAK_MEM_AP_STOP_CP_REQ) = 0;   //防止stop_CP未成功导致无法睡眠
    //LLPM VER开启CP电源后需要重新给BB下电上电，否则CP可能卡死
    COREPRCM->PWRCTL_CFG |= 0x20;
    delay_func_us(10);
	// poweron BB subsystem,stop cp时会power off BB来reset bb&dfe
	COREPRCM->PWRCTL_CFG &= ~0x20;

#if DYN_LOAD
    Dyn_Switch(2);
#endif

	/*OPENCPU形态用户自行设置UTC全局看门狗*/
#if (MODULE_VER != 0)
	/*监控CP核异常。模组形态CP核会喂狗UTC看门狗，确保永不深睡场景不会触发*/
    UTC_WDT_Init(CP_WATCHDOG_TIME);
#endif

    // 规避大功率发射IO口电压波动问题
    Gpio_LeakCurrentEnable();

    // Release CP Core
    Release_CP_Core();

    Debug_MODULE_Runtime_Add("g_db_boot_cp_end");

	start_tick = Get_Tick();
	//等待cp核开启调度，若CP核无法在30s内起调度，则加载CP异常，返回失败
	while(HWREGB(BAK_MEM_BOOT_CP_SYNC) != CP_IN_WORK)
	{
		/*若中断里起CP，或人为关中断后起CP，若CP侧此时有写flash动作，由于无法响应ICM_FLASHWRT_NOTICE消息，在此处处理*/
		if(IS_IRQ_MASKED() || IS_IRQ_MODE())
			wait_cp_flash_write_done();
		
        if (Check_Ms_Timeout(start_tick, wait_ms))
        {              
            g_errno = XY_ERR_CP_BOOT;
            
#if  (XY_DUMP)
            xy_assert(0);
#endif
            return 0;
        }
	}
	
	return 1;
}


bool notify_and_wait_CP_off(uint32_t wait_ms)
{
	int cp_ctl = CP_STOP;
	IPC_Message pMsg = {ICM_CHANGE_CP_STATE, (void *)&cp_ctl, sizeof(cp_ctl)};
	uint32_t start_tick = 0;

	if(IPC_WriteMessage(&pMsg) < 0)
	{
#if XY_DUMP
		while(1);
#endif
		return 0;
	}

	start_tick = Get_Tick();
	
	/*确认CP核是否已卡死，若卡死则尽快返回失败，由AP核执行强制停CP核行为*/
	while (HWREGB(BAK_MEM_AP_STOP_CP_REQ) != 1)
	{
		if(HWREGB(BAK_MEM_AP_STOP_CP_REQ) == 2)
			break;
		/*CP核锁中断时长不能超过1秒*/
		if (Check_Ms_Timeout(start_tick, wait_ms)) 
		{
#if XY_DUMP
			while(1);
#endif
			g_errno = XY_ERR_CP_DEAD;
			return 0;
		}
	}
	
	/*超时等待CP核执行关RF等CFUN=5行为，直至进入WFI*/
	while (HWREGB(BAK_MEM_AP_STOP_CP_REQ) != 2)
	{
		if ( Check_Ms_Timeout(start_tick, wait_ms))
		{
#if XY_DUMP
			while(1);
#endif
			g_errno = XY_ERR_CP_DEAD;
			return 0;
		}
	}
	return 1;
}


uint32_t force_stop_CP_handshake(uint32_t wait_ms)
{
    uint32_t ret = 1;
	uint32_t start_tick;

	HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) = FORCE_STOP_CP_REQ;

    //触发IPC中断
	PRCM_ApCpIntWkupTrigger();

	start_tick = Get_Tick();
	
	/*确认CP核是否已卡死，若卡死则尽快返回失败，由AP核执行强制停CP核行为*/
	while (HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) != FORCE_STOP_CP_ACK)
	{
		/*等待CP进入wakeup中断响应*/
		if (Check_Ms_Timeout(start_tick, wait_ms)) 
		{
#if XY_DUMP
			while(1);
#endif
            ret = 0;
			break;
		}
	}
    //恢复默认值
	HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) = FORCE_STOP_CP_NONE;

    return ret;
}

void flash_off_and_on()
{
    qspi_wait_idle();
    Flash_Poweroff();
    utc_cnt_delay(200);
    Flash_Poweron_Delay();
}

void reset_cp_dma(void)
{
    //cp log dma
    DMAPeriphReqDis(DMA_CHANNEL_1);
    DMAChannelMuxDisable(DMA_CHANNEL_1);
    DMAChannelTransferStop(DMA_CHANNEL_1);
    //cp rf/aes dma
    DMAPeriphReqDis(DMA_CHANNEL_2);
    DMAChannelMuxDisable(DMA_CHANNEL_2);
    DMAChannelTransferStop(DMA_CHANNEL_2);
    //cp aes dma
    DMAPeriphReqDis(DMA_CHANNEL_3);
    DMAChannelMuxDisable(DMA_CHANNEL_3);
    DMAChannelTransferStop(DMA_CHANNEL_3);
    //cp mem2mem dma
    // DMAPeriphReqDis(DMA_CHANNEL_6);
    // DMAChannelMuxDisable(DMA_CHANNEL_6);
    // DMAChannelTransferStop(DMA_CHANNEL_6);
}

void Disable_UTC_Alarm()
{
	UTC_AlarmDisable(UTC_ALARM_ALL);
	UTC_AlarmCntCheckDisable();
    NVIC_ClearPendingIRQ(UTC_IRQn);//清除utc_pending
    *((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) = RTC_NEXT_OFFSET_INVAILD;   //设alarm超时时间为无效值，起CP后由CP更新为有效值
}
extern void Hw_Init_By_StopCP(void);
void SwitchSysclkInStopCP(void)
{
    #if(LLPM_VER == 1)
    {
        //低功耗应用，为降低功耗，StopCP后切换回HRC时钟
        Sys_Clk_Src_Select(SYS_CLK_SRC);
        Xtal_Pll_Ctl(SYS_CLK_SRC,LSIO_CLK_SRC);
        if(AP_HCLK_DIV == 1)
        {
            PRCM_SlowfreqDivDisable();
        }
        else
        {
            Ap_HclkDivSet(AP_HCLK_DIV);
            PRCM_SlowfreqDivEnable();
        }

        //切换sido mode
        SIDO_LPMODE();
		
        AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
	    AONPRCM->CORE_PWR_CTRL0 = 0xB4;//sidolp 1.05v,deepsleep 1.0v
        AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0

        PRCM_ForceCPOff_Enable();//进mcu 模式

        Hw_Init_By_StopCP();
    }
    #endif
}

/*wait_ms为0，表示强制复位CP核；非零表示执行CP软关机流程的超时时间*/
int Stop_CP(uint32_t wait_ms)
{
	return Stop_CP2(wait_ms,0);
}

/*wait_ms为0，表示强制复位CP核；reset_ap_freq指示是否恢复AP单核的主频，即13M*/
int Stop_CP2(uint32_t wait_ms,uint8_t reset_ap_freq)
{
	int ret = 0;

    if (CP_Is_Alive() == false)
    {  
        clear_event(EVENT_BOOT_CP);
        return 1;
    }

    xy_open_log(0); //cp启动时，csp log是dma方式，手动写fifo不保证没问题, 暂时禁掉打印，重新初始化完成后打开

	HWREGB(BAK_MEM_AP_STOP_CP_REQ) = 0x5A;

	/*通知CP核执行软件的复位与NV保存,CP核可能异常无应答，后续会继续给CP核下电*/
	if(wait_ms != 0)
	{
		ret = notify_and_wait_CP_off(wait_ms);
        DisablePrimask();
        //CP停止执行指令
        Hold_CP_Core();
        SEMA_ResetAll();
        EnablePrimask();
	}
    else
    {
        DisablePrimask();
        rf_trx_close();//先关闭耗电大头，金卡在抠电池场景避免耗电
        //要避免AP stop CP握手与CP写flash挂起AP的冲突

        uint32_t ret, ckg_l_bak;
        ret = force_stop_CP_handshake(6);  //握手最多等待6ms
        if (ret == 0)
        {
            ckg_l_bak = COREPRCM->CKG_CTRL_L;
        }
        //CP停止执行指令
        Hold_CP_Core();
        if (ret == 0)
        {
            COREPRCM->CKG_CTRL_L = ckg_l_bak;
        }

        PRCM_CpApIntWkupClear();//清除可能存在的cp_ap_wkup_int,防止被CP的写flash req卡住
        //避免CP异常时占据信号量
	    SEMA_ResetAll();
        EnablePrimask();
    }
    
    // 配置GPIO33、34、37为输入下拉，取消漏电配置
    Gpio_LeakCurrentDisable();

#if (LLPM_VER == 1)
    //屏蔽CP唤醒源，唤醒时进入mcu mode
    HWREG(AONPRCM_BASE + 0x64) = 0x00;
#endif

    if(reset_ap_freq == 0)
	{
        extern void Lpm_Msg_NvWrite(void);
        Lpm_Msg_NvWrite();
    }
    
	/*CP核停止运行流程异常，或者强行停CP核，在此处进行CP相关硬件资源的强行关闭*/
	if(ret == 0)
	{
		//flash掉电上电,重新初始化
		if(DMAChannelGetStartStatus(DMA_CHANNEL_0) == 1 )
		{
        	while(DMAChannelTransferRemainCNT(DMA_CHANNEL_0));//flash dma chn
		}


        //flash_off_and_on();

        //reset_cp_dma();

		//关闭RF相关寄存器配置，否则再次启动CP可能有异常
	   	rf_trx_close();
		// stop BB subsystem,reset bb&dfe
		COREPRCM->PWRCTL_CFG |= 0x20;
        //delay_func_us(100);

        Simvcc_Poweroff();
	}

	/*强行停CP核，设置为上电模式，CP核下次工作必执行attach，但小区信息等非易变NV仍有效*/
	HWREGB(BAK_MEM_CP_UP_REASON) = POWER_ON; 
    HWREG(BAK_MEM_CP_UP_SUBREASON) = 1;


    ADC_TsensorPowerDIS();

    //reset aes/crc
    // COREPRCM->RST_CTRL |= 0x4200;
    // prcm_delay(10);
    // COREPRCM->RST_CTRL &= ~0x4200;

	// stop BB subsystem
	//COREPRCM->PWRCTL_CFG |= 0x20;
    
	// stop CP subsystem
	//AONPRCM->PWRCTL_TEST7 |= 0x04;//cp domain iso
	//prcm_delay(10);//don't delete
	//AONPRCM->PWRCTL_TEST7 |= 0x08;//cp domain gate

    //COREPRCM->RST_CTRL &= ~0x1000;//rst dfe
    
    Disable_UTC_Alarm();   //停CP前去使能RTC定时器，CP侧所有的RTC定时器都失效
    
    // CP 独占UTC资源用作深睡唤醒，为避免双核配置寄存器冲突，由主控核统一规划
	PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_UTC);
    PRCM_CpIntWakeupDisable();

    //bootcp时才开启cp sram
	PRCM_SramPowerModeSet(CP_SRAM0|CP_SRAM1, SRAM_POWER_MODE_FORCE_OFF);


    //PWRCFG_Mode_Off(CPCORE_PWR_IN_DEEPSLEEP);

    if(reset_ap_freq == 1)
	{
        SwitchSysclkInStopCP();
        
        /*仅AP输出明文log,CP核不输出任何LOG*/
        if(HWREGB(BAK_MEM_AP_LOG) == 7)
        {
            xy_print_uart_Init(XY_LOGVIEW_AP_RATE);//涉及主频切换，此时CSP3_UART必须重新初始化一次
        }
	}
    
    clear_event(EVENT_BOOT_CP);
	// stop CP后，忽略CP向AP发送的核间中断请求。
	// 此处只清除核间状态标记位，不清除wakeup 中断的pending，是为了防止将用户的wakuep事件误删除！
    PRCM_CpApIntWkupClear();
	
	/*起CP时，初始化在CP核；停CP后必须AP重新初始化下*/
	xy_logview_switch_ap();
	xy_open_log(1);

	HWREGB(BAK_MEM_AP_STOP_CP_REQ) = 0;
	
	clear_at_info();

	return 1;
}


__FLASH_FUNC int Send_Rai(void)
{
    int cp_ctl = CP_RAI;
    IPC_Message pMsg = {ICM_CHANGE_CP_STATE, (void *)&cp_ctl,sizeof(cp_ctl)};
	xy_printf("Send_Rai!");
    if (CP_Is_Alive() == false)
    {
        return 1;
    }

	if(IPC_WriteMessage(&pMsg) < 0)
	{
        xy_printf("Send_Rai failed!");
        return 0;
    }
	else
		return 1;
}

__FLASH_FUNC int Send_Cfun(int val)
{
	uint8_t  cp_ctl[4];
	IPC_Message pMsg = {ICM_CHANGE_CP_STATE, (void *)&cp_ctl,sizeof(cp_ctl)};

	cp_ctl[0] = CP_CFUN;
	cp_ctl[1] = val;
	
	xy_printf("Send_Cfun!");
	
	if (CP_Is_Alive() == false)
	{
		return 1;
	}

	if(IPC_WriteMessage(&pMsg) < 0)
	{
		xy_printf("Send_Rai failed!");
		return 0;
	}
	else
		return 1;
}


__FLASH_FUNC void xy_CP_Reboot()
{
    Stop_CP(0);
    Boot_CP(WAIT_CP_BOOT_MS);
}


/*仅用于OPENCPU场景。涉及CP核总体硬件的操作，只能放在AP核此处来执行。只能在at_uart_write或Send_AT_Req中调用，且当前不支持回复"\r\nOK\r\n"。若需要回复，需要定制下*/
__FLASH_FUNC int CP_Special_AT_Proc(void *data_addr)
{
	UNUSED_ARG(data_addr);
	
#if MODULE_VER==0
    if(strncmp(data_addr,"AT+NRB",strlen("AT+NRB"))==0 || strncmp(data_addr,"AT+RESET",strlen("AT+RESET"))==0)
    {
        xy_CP_Reboot();
        return 1;
    }
	else if(strncmp(data_addr,"AT+BOOTCP=1",strlen("AT+BOOTCP=1"))==0)
    {
        Boot_CP(WAIT_CP_BOOT_MS);
        return 1;
    }
	else if(strncmp(data_addr,"AT+BOOTCP=0",strlen("AT+BOOTCP=0"))==0)
    {
        Stop_CP(0);
        return 1;
    }
#endif
    return 0;
}

