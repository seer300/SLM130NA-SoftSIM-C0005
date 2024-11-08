#include "xy_lpm.h"
#include "xy_timer.h"
#include "flash_vendor.h"
#include "utc.h"
#include "prcm.h"
#include "fast_recovery.h"
#include "hw_tick.h"
#include "xy_memmap.h"
#include "sys_clk.h"
#include "xy_timer.h"
#include "xy_system.h"
#include "sys_proc.h"
#include "at_uart.h"
#include "sys_ipc.h"
#include "xy_event.h"
#include "common.h"
#include "hw_types.h"
#include "uart.h"
#include "xy_list.h"
#include "xy_flash.h"
#include "xy_cp.h"
#include "tick.h"
#include "soc_control.h"
#include "dyn_load.h"
#include "nvic.h"
#include "xy_gpio_rcr.h"
#include "sys_rc32k.h"
#include "adc.h"
#include "rc32k.h"
#include "adc_adapt.h"

#define STANDBY_SLEEPTIME_THREHOLD   15
#define WFI_SLEEPTIME_THREHOLD       15
#define DEEPSLEEP_SLEEPTIME_THREHOLD 100
#define PRCM_LPMODE_LOG_BASE         UART2_BASE
#define TICK_COUNTER_MAX             ((uint32_t)0xFFFFFFFF)    //TICK_COUNTER寄存器计数的最大值

extern volatile uint32_t g_rc32k_cali_flag;
typedef enum
{
    SLEEP_PERMITT = 0,
    SLEEP_NOT_ALLOW,
} LPM_SLEEP_TYPE;

typedef enum
{
	STANDBY_FLASH_STATUS_NORMAL = 0,
    STANDBY_FLASH_STATUS_DEEP_POWER_DOWN,
    STANDBY_FLASH_STATUS_FLASH_VCC_OFF,
	STANDBY_FLASH_STATUS_VDDIO_OFF,
} STANDBY_FLASH_STATUS_ENUM;

//锁位图，DEEPSLEEP共16个；standby和wfi各8个
uint32_t lpm_lock = 0;

// 该全局变量控制LPUART在芯片低功耗模式下的供电状态，以及LPUART是否为低功耗模式的唤醒源。
uint8_t g_lpuart_used = 0; // 1：LPUART在芯片低功耗模式下供电打开，且为低功耗模式唤醒源；0：LPUART在芯片低功耗模式下供电关闭，且不为低功耗模式唤醒源
uint8_t g_lptim_used = 0;



/**
 * @brief 深睡前HOOK函数注册接口,此时关中断，可以进行FLASH的擦写动作
 */
pFunType_void p_Before_DeepsleepHooK = NULL;
void Into_Dslp_Hook_Regist(pFunType_void pfun)
{
    p_Before_DeepsleepHooK = pfun;
	mark_dyn_addr(&p_Before_DeepsleepHooK);
}

void Before_Deepsleep_Hook(void)
{
    if(p_Before_DeepsleepHooK != NULL)
    {
        p_Before_DeepsleepHooK();
    }
}

void LPM_LOCK(LPM_LOCK_TypeDef lpm_lock_type)
{
    DisablePrimask();
	
    lpm_lock = lpm_lock | (1U << lpm_lock_type);
	
	if (lpm_lock_type < DSLEEP_END)
		HWREGB(BAK_MEM_AP_LOCK_TYPE) |= 1;
	else if (lpm_lock_type < STANDBY_END)
		HWREGB(BAK_MEM_AP_LOCK_TYPE) |= 2;
	else if (lpm_lock_type < WFI_END)
		HWREGB(BAK_MEM_AP_LOCK_TYPE) |= 4;

	if (CP_Is_Alive() == true)
		PRCM_ApCpIntWkupTrigger();
	
	EnablePrimask();
}

void LPM_UNLOCK(LPM_LOCK_TypeDef lpm_lock_type)
{
    DisablePrimask();
	
    lpm_lock = lpm_lock & (~(1U << lpm_lock_type));
	
	if ((lpm_lock & (((uint64_t)1 << DSLEEP_END) - ((uint64_t)1 << DSLEEP_BASE))) == 0)
	{
		HWREGB(BAK_MEM_AP_LOCK_TYPE) &= (0XFF-1);
	}

	if ((lpm_lock & (((uint64_t)1 << STANDBY_END) - ((uint64_t)1 << STANDBY_BASE))) == 0)
	{
		HWREGB(BAK_MEM_AP_LOCK_TYPE) &= (0XFF-2);
	}

	if ((lpm_lock & (((uint64_t)1 << WFI_END) - ((uint64_t)1 << WFI_BASE))) == 0)
	{
		HWREGB(BAK_MEM_AP_LOCK_TYPE) &= (0XFF-4);
	}

	if (CP_Is_Alive() == true)
		PRCM_ApCpIntWkupTrigger();
	
	EnablePrimask();

}

bool LPM_LOCK_EXIST(LPM_LOCK_TypeDef lpm_lock_type)
{
	if(lpm_lock & (1U << lpm_lock_type))
		return 1;
    else
		return 0;
}

void LPM_UNLOCK_ALL()
{
    lpm_lock = 0;
}

#define LPM_NV_WRITE_NUM ( 5 )
typedef struct
{
	uint8_t write_num;
	struct
	{
		uint32_t dest_addr;
		uint32_t src_addr;
		uint32_t size;
	}buff[LPM_NV_WRITE_NUM];
}lpm_nv_write_t;

/*CP核睡眠期间不能写flash，进而将NV的保存放在AP核执行.OPENCPU形态只会低概率写出厂NV和非易变NV*/
void Lpm_Msg_NvWrite(void)
{
	volatile lpm_nv_write_t* lpm_nv_write = (lpm_nv_write_t*)BAK_MEM_LPM_NV_WRITE_BUFF;

	for(uint8_t i = 0; i < lpm_nv_write->write_num; i++)
	{
		xy_assert(lpm_nv_write->buff[i].src_addr != 0);

		/*出厂NV中，只有RF自校准NV改变才写FLASH*/

#if (MODULE_VER == 0 && BAN_WRITE_FLASH == 1)
        if(lpm_nv_write->buff[i].dest_addr == NV_FLASH_FACTORY_BASE)
            xy_ftl_write(lpm_nv_write->buff[i].dest_addr,(void *)lpm_nv_write->buff[i].src_addr,lpm_nv_write->buff[i].size);
        else
#endif
        {
            xy_Flash_Write(lpm_nv_write->buff[i].dest_addr, (void *)lpm_nv_write->buff[i].src_addr, lpm_nv_write->buff[i].size);
#if XY_DEBUG
            if(lpm_nv_write->buff[i].dest_addr == FS_FLASH_BASE)
                Send_AT_to_Ext("+DBGINFO:Save Cloud CFG\r\n");
#endif
        }

#if XY_DEBUG
		char *rsp = xy_malloc(64);
		sprintf(rsp,"+DBGINFO:AP DEEPSLEEP WRITE NV %lX %lX %ld!\r\n", lpm_nv_write->buff[i].dest_addr, lpm_nv_write->buff[i].src_addr, lpm_nv_write->buff[i].size);
		Send_AT_to_Ext(rsp);
		xy_free(rsp);
#endif
	}
}
// 将存进RTC链表里的64位寄存器拼接值，转换成相对1970/1/1的毫秒偏移量
uint64_t Transform_Mum64_To_Ms(uint64_t rtc_cal_time_cnt_num64)
{
	uint8_t ulAMPM;
	uint8_t ulHour;
	uint8_t ulMin;
	uint8_t ulSec;
	uint8_t ulMinSec;

	uint32_t rtc_info_cal;
	uint32_t rtc_info_timer;
	uint32_t rtc_info_cnt;

	uint64_t ap_rtc_msec;

	RTC_TimeTypeDef rtc_info;

	if ((rtc_cal_time_cnt_num64 != 0) && (rtc_cal_time_cnt_num64 != RTC_ALARM_INVALID))
	{
		// 计算年月日
		rtc_info_cal = (uint32_t)(rtc_cal_time_cnt_num64 >> 41);
		rtc_info.tm_year = (rtc_info_cal >> 11) & 0xFF;
		rtc_info.tm_year = 2000 + (rtc_info.tm_year >> 4) * 10 + (rtc_info.tm_year & 0x0F);
		rtc_info.tm_mon  = (rtc_info_cal >> 6) & 0x1F;
		rtc_info.tm_mon  = (rtc_info.tm_mon >> 4) * 10 + (rtc_info.tm_mon & 0x0F);
		rtc_info.tm_mday = rtc_info_cal & 0x3F;
		rtc_info.tm_mday = (rtc_info.tm_mday >> 4) * 10 + (rtc_info.tm_mday & 0x0F);

		// 计算时/分/秒/毫秒
		rtc_info_timer = (uint32_t)(rtc_cal_time_cnt_num64 >> 9);
		UTC_TimerGet(&ulAMPM, &ulHour, &ulMin, &ulSec, &ulMinSec, rtc_info_timer);
		rtc_info.tm_hour = (uint32_t)(ulAMPM * 12 + ulHour);
		rtc_info.tm_min  = ulMin;
		rtc_info.tm_sec  = ulSec;

		// 计算CNT
		rtc_info_cnt = (uint32_t)(rtc_cal_time_cnt_num64 & 0x1FF);
		rtc_info.tm_msec = (uint32_t)((ulMinSec * 10UL) + (rtc_info_cnt / 32UL));

		// 转换为ms
		ap_rtc_msec = xy_mktime(&rtc_info);
	}
	else
	{
		ap_rtc_msec = rtc_cal_time_cnt_num64;
	}

	return ap_rtc_msec;
}

int RTC_Alarm_Timeout_Check(void)     //BUG8986 规避代码：若距离最近超时的RTC事件不足1ms，则退出睡眠
{
	uint32_t tick_cal_current; 

	if(*((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) == RTC_NEXT_OFFSET_INVAILD)  //剩余毫秒偏移量不是有效值可略过rtc超时的判断
	{
		return SLEEP_PERMITT;
	}

	tick_cal_current = (uint32_t)Tick_CounterGet();

	//对TICK_COUNTER寄存器全F后跳变的计算处理
	if(tick_cal_current >= HWREG(BAK_MEM_TICK_CAL_BASE))
	{
		if((uint64_t)(tick_cal_current - HWREG(BAK_MEM_TICK_CAL_BASE) + 5) > *((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET))
		{
			return SLEEP_NOT_ALLOW;
		}
	}
	else
	{
		if((uint64_t)(TICK_COUNTER_MAX + tick_cal_current - HWREG(BAK_MEM_TICK_CAL_BASE) + 2) > *((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET))
		{
			return SLEEP_NOT_ALLOW;
		}
	}

	return SLEEP_PERMITT;
}

/**
 * @brief config retldo or sido according to the retmem configuration
 * 
 */
void PRCM_RetmemPwrManage(void)
{
#if !LLPM_VER  //表计在初始化阶段配置，减少重复配置
	uint8_t is_8K_on=0,num_64k_on=0;
	uint32_t SMEM_SLPCTRL_reg;

	//根据实际打开的retmem配置,决策使用sido或者retldo供电,来达到最优的功耗
	SMEM_SLPCTRL_reg = AONPRCM->SMEM_SLPCTRL;
	
	if( (SMEM_SLPCTRL_reg & SH_SRAM8K_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << SH_SRAM8K_SLPCTL_Pos))
		is_8K_on = 1;

    if( (SMEM_SLPCTRL_reg & AP_SRAM0_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << AP_SRAM0_SLPCTL_Pos))
        num_64k_on++;

    if( (SMEM_SLPCTRL_reg & AP_SRAM1_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << AP_SRAM1_SLPCTL_Pos))
        num_64k_on++;

    if( (SMEM_SLPCTRL_reg & CP_SRAM0_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << CP_SRAM0_SLPCTL_Pos))
        num_64k_on++;
	
    if( (SMEM_SLPCTRL_reg & CP_SRAM1_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << CP_SRAM1_SLPCTL_Pos))
        num_64k_on++;

    if( (SMEM_SLPCTRL_reg & SH_SRAM64K_SLPCTL_Msk) == (SRAM_POWER_MODE_FORCE_ON << SH_SRAM64K_SLPCTL_Pos))
        num_64k_on++;    

    if(num_64k_on >= 2)
    {
        //sido supply
        AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
        AONPRCM->CORE_PWR_CTRL0 = 0xB4;//sidolp 1.05v,standby/deep 1.0v
        AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
        AONPRCM->AONPWR_CTRL |= 0x8800;//sido is on in deepsleep;sel sido lpmode as vddret source
	    PRCM_PowerOffRetldo();

    }
    else if(num_64k_on | is_8K_on)
    {
        //retldo supply
        //retldo 需要稳定时间，已经在初始化阶段开启，无需重复配置
        PRCM_PowerOnRetldo();
        AONPRCM->AONPWR_CTRL &= ~0x8800;       
    }    
    else
    {
        //both retldo and sido off
        AONPRCM->AONPWR_CTRL &= ~0x8800;//
	    PRCM_PowerOffRetldo();
    }
#endif
}
extern uint32_t g_event_bitmap;
extern bool Is_ZeroCopy_Buf_Freed();
extern volatile uint8_t g_LUT_cleanup_flag;
int DeepSleep_Admittance_Check(void)
{
#if (AT_LPUART == 1)
    /*main主函数中有待输出的字符串或者AT命令是否未处理，若未处理则退出睡眠*/
    if(at_uart_idle() == false)
		return SLEEP_NOT_ALLOW;
#endif        
	if((lpm_lock & (((uint64_t)1<<DSLEEP_END) - ((uint64_t)1<<DSLEEP_BASE))) != 0)
		return SLEEP_NOT_ALLOW;

	if(g_event_bitmap != 0)
		return SLEEP_NOT_ALLOW;

	if(g_rc32k_cali_flag != 0 || g_LUT_cleanup_flag != 0)
		return SLEEP_NOT_ALLOW;

	if((((HWREG(0xe000e100) & HWREG(0xe000e200)) >> WAKEUP_IRQn) & 1UL) != 0)   //若已产生wakeup中断，立即退出
        return SLEEP_NOT_ALLOW;
		
	return SLEEP_PERMITT;
}

extern int Timer_Reset_ForDslp(int Limit);
void DeepSleep_Context_Save(void)
{
#if (MODULE_VER != 0x0)	
	RC32K_CALIB_Dis();
#endif

	//lptim和lptim的唤醒使能只能在深睡生效，在active时若设置生效，则会触发多余的wakeup中断
	uint16_t WkupSrc = AON_WKUP_SOURCE_APTICK;
	
	if(g_lpuart_used)
		WkupSrc |= AON_WKUP_SOURCE_LPUART1;
	if(g_lptim_used)
		WkupSrc |= AON_WKUP_SOURCE_LPTMR;

	PRCM_AonWkupSourceEnable(WkupSrc);
	Debug_Runtime_Add("WkupSrc stop Timer_Reset_ForDslp start");

	/*深睡时，仅保证深睡保持的超时ID继续有效，不保持的不更新寄存器*/
	Timer_Reset_ForDslp(TIMER_LP_END);
}

extern uint16_t g_pri_nesting; //中断嵌套变量
extern volatile uint32_t g_LUT_clean_period_min;
/*深睡期间写flash耗时过久，会造成LPUART的FIFO 32字节溢出*/
void do_flash_write_Dslp()
{
/*写flash期间，临时修改NVIC状态，保证LPUART可以正常接收*/
#if (AT_LPUART && MODULE_VER)
	//保存systick_int_mask位、64位NVIC中断使能状态、PRIMASK位
	uint32_t systick_int_mask = SysTick->CTRL & SysTick_CTRL_TICKINT_Msk;
	uint32_t nvic_enable_bak0 = NVIC->ISER[0];
	uint32_t nvic_enable_bak1 = NVIC->ISER[1];
	uint32_t primask_bak = __get_PRIMASK();
	uint16_t nest_mask;

	//禁能systick中断、63位NVIC中断，只使能LPUART中断
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICER[1] = 0xFFFFFFFF;
	NVIC->ISER[0] = 1 << LPUART_IRQn;

	nest_mask = g_pri_nesting;
	g_pri_nesting = 0;

	//开总中断
	__set_PRIMASK(0);
#endif

#if XY_DEBUG
	Send_AT_to_Ext("+SOC DEEPSLEEP\r\n");
#endif

	Lpm_Msg_NvWrite();

#if (MODULE_VER != 0x0)	
	if(g_LUT_clean_period_min == 0)
	{
		// 不支持休眠期间偷偷唤醒，使用golden ref完全拟合
		Fill_Aon_LUT_With_Golden_Ref();
	}
	else
	{
		// 支持休眠期间偷偷唤醒，使用线性局部拟合
		Fill_Aon_LUT_With_Linear_Interpolation();
	}

	// AP单核写flash：保存32k校准信息至flash中
	Save_32kCaliTbl_To_Flash();

	//模组形态，睡眠前保存深睡保持定时器全局及ap time链表至flash
	extern void Save_ApTime_To_Flash();
	Save_ApTime_To_Flash();
#endif

/*写完flash，恢复NVIC状态*/
#if (AT_LPUART && MODULE_VER)
	//恢复总中断状态
	__set_PRIMASK(primask_bak);
	g_pri_nesting = nest_mask;


	//恢复systick_int_mask位、64位NVIC中断使能状态
	SysTick->CTRL |= systick_int_mask;
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICER[1] = 0xFFFFFFFF;
	NVIC->ISER[0] = nvic_enable_bak0;
	NVIC->ISER[1] = nvic_enable_bak1;
#endif

}

uint8_t g_sysclk_sel_before_sleep = 0;//进入sleep前时钟选择，若是pll睡眠后恢复
void DeepSleep_Power_Manage(void)
{


	//如果启动了CP核，进行CP的去reset，并恢复用户默认时钟配置
  	if (CP_Is_Alive() == true)
    {
		// reset CP
		Hold_CP_Core();

#if (LLPM_VER == 1)
        //屏蔽CP唤醒源，唤醒时进入mcu mode
        HWREG(AONPRCM_BASE + 0x64) = 0;
#endif

        // 配置GPIO34、37、38为输入下拉，取消漏电配置
        Gpio_LeakCurrentDisable();

		/*若CP核深睡期间被打断，则放弃上次深睡的NV保存，待再次进入深睡后再保存NV*/
		if((CP_IS_DEEPSLEEP() == true))
			do_flash_write_Dslp();

		/*模组形态CP核负责喂狗，深睡之前需去初始化，否则会造成深睡复位异常*/
#if (MODULE_VER != 0)
		UTC_WDT_Deinit();
#endif
	}

	Before_Deepsleep_Hook();

#if DYN_LOAD
	Dyn_Switch(1);
#endif
	
//若之前起过CP，这里需要将始终源切换为HRC，以便将SIDO变为更低功耗的LPMODE模式。
//这里表计配置完成后为加快速度没有查询当前MCU的状态；非表计流程阻塞等待MCU时钟源等切换完毕
	g_sysclk_sel_before_sleep = (AONPRCM->SYSCLK_CTRL & SYSCLK_SRC_SEL_Msk)>>SYSCLK_SRC_SEL_Pos;

	if( g_sysclk_sel_before_sleep ==  SYSCLK_SRC_PLL)
	{
		//当前系统时钟选择pll，则切换为非pll，并切换sido lp
		//如果编译配置sysclk为pll，则强制切换为hrc；编译配置sysclk非pll，则按照编译配置
		if(SYS_CLK_SRC == SYSCLK_SRC_PLL)
		{
			PRCM_SlowfreqDivDisable();

            #if (MODULE_VER != 0x0)	
                if(AONPRCM->AONGPREG2 & 0x10)
                {
                    Sys_Clk_Src_Select(SYSCLK_SRC_XTAL);
                    //xtal on hrc off when wkup
                    Xtal_Pll_Ctl(SYSCLK_SRC_XTAL,LSIO_CLK_SRC);
                }
                else
                {
                    Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
                    //xtal off hrc on when wkup 
                    Xtal_Pll_Ctl(SYSCLK_SRC_HRC,LSIO_CLK_SRC);
                }
            #else
                Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
                Xtal_Pll_Ctl(SYSCLK_SRC_HRC,LSIO_CLK_SRC);
            #endif
		}
		else
		{
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
		}
		//切换sido mode
		SIDO_LPMODE();
        
        #if(LLPM_VER == 1)
        AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
	    AONPRCM->CORE_PWR_CTRL0 = 0xB4;//sidolp 1.05v,deepsleep 1.0v
        AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0

        PRCM_ForceCPOff_Enable();//进mcu 模式
        #endif
	}	
}

extern uint8_t PRCM_IsOn_Retldo(void);
void normal_deepsleep_wfi(int lpuart_keep)
{
	/*深睡时关闭LPUART唤醒功能*/
	if(lpuart_keep == 0)
	{
		PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);

		PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_LPUART1);
	}

	// bit 7,   AONGPREG0_CP_FAST_BOOT
    // bit 6,   AONGPREG0_CORE_LOAD_FLAG_SECONDARY_BOOT
    // bit 5-3, External Flash Delay Config
    // bit 2,   AONGPREG0_FORCE_XTAL_AFTER_RST
    // bit 1,   AONGPREG0_CORE_LOAD_FLAG_ARM
    // bit 0,   AONGPREG0_CORE_LOAD_FLAG_DSP
    HWREGB(AON_AONGPREG0)  = (0x02 << 3);  //delay in bootrom,0x02,2.3ms @26M;0x01,4.6ms @26M
    AONPRCM->FLASH_VCC_IO_CTRL = (AONPRCM->FLASH_VCC_IO_CTRL & ~0x03) | 0x01;//flash vcc not off in standby,off in deepsleep
	//FlashEnterLPMode();
    PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_Enable);//模组睡眠自动关闭ioldo2，以及flashvcc，更低功耗

	PRCM_LPUA_PWR_Ctl(LPUA_DEEPSLEEP_MODE_OFF); //off in deepsleep
	
	/*8k retension mem受CP核的BAK_MEM_OFF_RETMEM_FLAG控制*/
#if BAN_WRITE_FLASH	
	PRCM_SramPowerModeSet(CP_SRAM0 | CP_SRAM1, SRAM_POWER_MODE_OFF_DSLEEP); 
#else
	PRCM_SramPowerModeSet(CP_SRAM0 | CP_SRAM1 | SH_SRAM64K, SRAM_POWER_MODE_OFF_DSLEEP); 
#endif
	PRCM_SramPowerModeSet(AP_SRAM0 | AP_SRAM1, SRAM_POWER_MODE_OFF_DSLEEP);

    // 由于模组形态 8Kretmem在深睡时可能打开，所以此处在深睡前打开8kretmem的rentention功能，降低功耗
	PRCM_SramRetentionEnable(SH_SRAM8K);
	PRCM_RetmemPwrManage();//该接口只能在所有retmem配置完电源模式后调用
	
	// 规避唤醒后utc停止计数长达20ms的bug
	AONPRCM->UTC_PCLKEN_CTRL = 0;
	while(AONPRCM->UTC_PCLKEN_CTRL != 0);

	if( g_LUT_clean_period_min == 0)
	{
		if(HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
		{
			*((volatile uint32_t *)0xe000e284) |= (0x1 << (RC32K_IRQn - 32));   //清除RC32K_pending，规避AP睡眠失败导致的异常唤醒
		}
	}

    HWREGB(0x40000059) = 0x7; // itune = 7
    
    // ecobit深睡前需提前800us打开
    HWREGB(0x40000800) |= (0x01 << 6);
    utc_cnt_delay(26);

#if (AT_LPUART == 1)
    if(at_uart_idle())
#endif
    {
            AONPRCM->AONGPREG2 = (AONPRCM->AONGPREG2 & 0x1F) | (PRCM_LPUARTclkDivGet() << 5);  //AONGPREG2的bit5-7用于记录lpuart分频
        PRCM_LPUA1_ClkSet(AON_HRC_DIV16);

        // bug15477 new深睡前配置aon_cntl_85和aon_cnt_77为1
        if (HWREG(BAK_MEM_OTP_SOC_ID_BASE) >= 0x1)
        {
            while ((HWREGB(0x40000809) & 0x20) == 0)
            {
                HWREGB(0x40000809) |= 0x20; // aon_cnt_77
            }
        }

            __WFI();
    }

    // 深睡失败恢复lpuart分频系数
    if (AONPRCM->AONGPREG2 & 0xE0)
    {
        PRCM_LPUA1_ClkSet(AONPRCM->AONGPREG2 >> 5);
        AONPRCM->AONGPREG2 &= 0x1F;
    }

    // 深睡失败清除ecobit
    HWREGB(0x40000800) &= ~(0x01 << 6);

	SCB->SCR &= ~0x04;	
}

void fast_deepsleep_wfi()
{
	//AP快速恢复场景，RAM不掉电，唤醒后跳过一级boot，flash可断电
	HWREGB(AON_AONGPREG0) = 0x43;
	HWREG(DEEPSLEEP_WAKEUP_AP_JUMP_ADDR) = SRAM_BASE;

#if (MODULE_VER == 0 && LLPM_VER == 0)
    //pll模式的opencpu配置和flashvcc以及ioldo2睡眠状态。表计已在初始化配置
    AONPRCM->FLASH_VCC_IO_CTRL &= ~0x03;//flash vcc not off automatic in deepsleep&standby
#endif

    PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_PSM_Enable);

    FlashEnterLPMode();

	//lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。初始化时配置power
	// if(g_lpuart_used || g_lptim_used)
	// {
	// 	PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //force on in deesleep
	// }

	//PRCM_SramPowerModeSet(AP_SRAM0| AP_SRAM1, SRAM_POWER_MODE_FORCE_ON);	//快速恢复初始化阶段已经配置AP_SRAM，此处注释掉给表计节省工作时长
	//PRCM_SramPowerModeSet(CP_SRAM0 | CP_SRAM1 | SH_SRAM64K, SRAM_POWER_MODE_OFF_DSLEEP);	// CP的电由boot_CP配置

	g_fast_startup_flag = AP_WAKEUP_FASTRECOVERY_BEFORE;

	Debug_Runtime_Add("(In fast_deepsleep_wfi)FlashEnterLPMode stop stop save_scene_and_wfi start");
	save_scene_and_wfi();
}

void Deepsleep_Entry(void)
{
#if(MODULE_VER != 0x0)
	if(RTC_Alarm_Timeout_Check() == SLEEP_NOT_ALLOW)  //BUG8986规避代码：若距离最近超时的RTC事件不足1ms，则退出deepsleep;单核睡眠不判断
	{
		return;
	}
#endif

#if(DEBUG_MODULE_RUN_TIME == 1)
	// 保存AP睡眠前时刻点
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x18) = Tick_CounterGet();
#endif
	HWREG(RAM_NV_VOLATILE_LPM_START + 0x2c) = Tick_CounterGet() - HWREG(RAM_NV_VOLATILE_LPM_START + 0x2c);  //保存于易变NV中的睡眠信息:ap_cp_delta_time

	// force idle	/* lpm enable */
	COREPRCM->LPM_CTRL |= 0x1f0001;

	/* Set SLEEPDEEP bit of Cortex System Control Register */
	SCB->SCR |= 0x04;

#if (LLPM_VER == 1)// opencpu、表计 为节省时间，减少判断，将大多数初始化放在lpm_init中
	HWREGB((uint32_t)&COREPRCM->LPM_AP_CTRL) = LOW_POWER_MODE_DSLEEP;

	// 普通模式ioldo in LPMODE，睡眠模式时默认关闭ioldo,相关代码已经在lpm_init中实现以节省时间和功耗
    //2023/12/11改动可能有normal mode的切换和响应电压值调整需要调用
   	PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_PSM_Enable);

	//如果ioldo1_byp_flag置1，深睡前需要置aon_psmioldo1_byp_en = 1,Deepsleep唤醒后需要清0；
	// if(PRCM_IOLDO1_BypFlag_Get() == 1)	
	// {
	// 	PRCM_IOLDO1_Enable_PSM_Byp();
	// }
#else
    PRCM_LPM_ApReqLpMode(LOW_POWER_MODE_DSLEEP);
	
	PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_PSM_Enable);  // 普通模式ioldo in LPMODE，睡眠模式时in PSM mode

	//表计不判断是否为3v，节省时间。即便为1.8v执行括号内部代码，也没关系
	// if(PRCM_IOLDO1_IsActiveFlag_3V() != 0) 
	// {
	// 	if(PRCM_IOLDO1_BypFlag_Get() == 1)
	// 	{
	// 		PRCM_IOLDO1_Enable_PSM_Byp();
	// 	}
	// } 

	//表计不支持8K深睡下电，此处不做判断，节省时间。
	if (HWREGB(BAK_MEM_OFF_RETMEM_FLAG) == 1)
	{
		// 设置retmem区域为深睡掉电模式，唤醒后初始化阶段会再设置为FORCE ON模式
		PRCM_SramPowerModeSet(SH_SRAM8K, SRAM_POWER_MODE_OFF_DSLEEP);
	}
	else
	{
		PRCM_SramPowerModeSet(SH_SRAM8K, SRAM_POWER_MODE_FORCE_ON);
	}   
#endif
	Debug_Runtime_Add("(In Deepsleep_Entry) fast_deepsleep_wfi start");
#if (MODULE_VER == 0x0)			// opencpu,执行快速恢复
{
	fast_deepsleep_wfi();
}
#else		//模组，不执行快速恢复
{
    #if (AT_WAKEUP_SUPPORT == 1)  // 宏配置支持lpuart深睡唤醒，通过出厂NV参数off_wakeupRXD可关闭
    {
        extern uint8_t g_off_wakeupRXD;
        normal_deepsleep_wfi(!g_off_wakeupRXD);
    }
    #else
    {
        normal_deepsleep_wfi(0);
    }
    #endif
}
#endif
}

void DeepSleep_Power_Recover(void)
{

	// 非快速恢复时运行至此，表明深睡失败，因此主动恢复电源配置
	// 快速恢复模式的电源配置在reset_handler完成，此处无需额外操作
	if(g_fast_startup_flag != AP_WAKEUP_FASTRECOVERY_AFTER)
	{
		HWREGB(AON_AONGPREG0) = 0x42;	// 恢复为wdt reset方式
		//深睡被打断时，直接关闭lptim和lpuart的深睡维持寄存器电源。
		//PRCM_LPUA_PWR_Ctl(LPUA_DEEPSLEEP_MODE_OFF);

		AONPRCM->UTC_PCLKEN_CTRL = 1;

#if (MODULE_VER == 0x0)
		PRCM_SramRetentionDisable(SH_SRAM8K|AP_SRAM0|AP_SRAM1);
#else
		PRCM_SramRetentionDisable(SH_SRAM8K);
#endif

    //2023/12/13修改：LLPM_VER也会在睡眠前重复设置ioldo状态；模组版本睡眠失败会启动cp，需要按要求设置normal mode，opencpu直接设置lpmode
    //normal切换内部有utc超时机制，UTC_PCLKEN_CTRL需要设置
#if (MODULE_VER != 0x0)    
        if(g_nv_ioldo1_mode)
        {
            PRCM_IOLDO1_ModeCtl(IOLDO1_NORMAL_Enable);
        }
        else
        {
            PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);
        }
#else
        PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);    
#endif

//表计在睡眠前后不会修改sidolp的电压，因此不用在这里恢复
#if (LLPM_VER != 1)

        PRCM_PowerOnRetldo(); 
#endif	
		//时钟和sido恢复为睡眠前状态，只有sysclk为pll会在睡眠前被修改
		if(g_sysclk_sel_before_sleep == SYSCLK_SRC_PLL)
		{
            #if (LLPM_VER == 1)
            AONPRCM->SYSCLK_CTRL &= ~0x1100;//表计模式force stop 32k as sysclk,加快唤醒，睡眠失败/唤醒后需清除，否则切换时钟时可能有问题
            #endif 
			PRCM_ForceCPOff_Disable();
			delay_func_us(100);    
			SIDO_NORMAL();
            if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
            {
                HWREGB(0x40000059) = 0x7;   // itune = 7
                HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
            }
			/*pll 已经设置过，Coreprcm配置未丢失，*/
			HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
            while (!(COREPRCM->ANAXTALRDY & 0x01)); //xtal ready
            HWREGB(0x40000059) = 0x4;   // itune = 4

            HWREGB(COREPRCM_ADIF_BASE + 0x58) = 0x5c; //enable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
            HWREGB(AON_SYSCLK_CTRL2) = 0;//disable:xtal,pll power on

			//模组模式下或SYS_CLK_SRC不为PLL但后续启动CP导切换到PLL，频率恢复为启动CP时的状态：pll 10分频；否则恢复至配置文件频率

			#if (MODULE_VER != 0x0)
				Ap_HclkDivSet(10);
			#else
				if(SYS_CLK_SRC != SYSCLK_SRC_PLL )
				{
					Ap_HclkDivSet(10);
				}
				else
				{
					Ap_HclkDivSet(AP_HCLK_DIV);
				}
			#endif		

			Sys_Clk_Src_Select(SYSCLK_SRC_PLL);
		}

		FlashExitLPMode();


		// 模组形态深睡失败，重新加载CP
#if (MODULE_VER != 0x0)

		/*模组形态深睡失败，只能是AT唤醒，此处不考虑从寄存器层面识别；否则太多理论并行冲突场景了。*/
		HWREG(BAK_MEM_CP_UP_SUBREASON) |= (1 << AT_WAKUP);

		// poweron BB subsystem,stop cp时会power off BB来reset bb&dfe
		COREPRCM->PWRCTL_CFG &= ~0x20;
		// poweron CP subsystem
		//AONPRCM->PWRCTL_TEST7 &= ~0x08;//cp domain gate=0
		//prcm_delay(10);//don't delete
		//AONPRCM->PWRCTL_TEST7 &= ~0x04;//cp domain iso=0

		/*深睡恢复后，初始化看门狗，由CP核喂狗*/
		UTC_WDT_Init(CP_WATCHDOG_TIME);

        // 解决simvcc打开，ioldo2电压被下拉500mv
        PRCM_IOLDO2_ModeCtl(IOLDO2_NORMAL_Enable);

		Release_CP_Core();
        HWREGB(BAK_MEM_BOOT_CP_SYNC) = CP_DEFAULT_STATUS;

		extern void Set_Restore_LUT_Flag(uint8_t flag);
		Set_Restore_LUT_Flag(1);
#endif

		/*深睡失败，需要保证非深睡保持的超时ID继续有效，如软看门狗超时*/
		Timer_Reset_ForDslp(TIMER_NON_LP_END);


// #if (LLPM_VER != 1)
//     //表计不判断是否为3v，节省时间。即便为1.8v执行括号内部代码，也没关系
// 		if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)
// #endif        
// 		{
// 			//睡眠失败aon_psmioldo1_byp_en强制清0；
// 			PRCM_IOLDO1_Disable_PSM_Byp();
// 		}
	}
}

void DeepSleep_Context_Recover(void)
{
#if (MODULE_VER != 0x0)
	RC32K_CALIB_En();
#endif

	//lptim和lptim的唤醒使能只能在深睡生效，在active时若设置生效，则会触发多余的wakeup中断，active时关闭其唤醒能力
	// 失能AP核外设产生wakeup中断的能力
	PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_LPUART1 | AON_WKUP_SOURCE_APTICK | AON_WKUP_SOURCE_LPTMR);
}



#if(MODULE_VER != 0)     //OpenCpu形态不支持standby，为压缩内存，此处不编译

int StandBy_Admittance_Check(void)
{
	if ((lpm_lock & (((uint64_t)1<<STANDBY_END) - ((uint64_t)1<<STANDBY_BASE))) != 0)
		return SLEEP_NOT_ALLOW;

#if (AT_LPUART == 1)
    /*main主函数中有待输出的字符串或者AT命令是否未处理，若未处理则退出睡眠*/
    if(at_uart_idle() == false)
	{
        return SLEEP_NOT_ALLOW;
	}
#endif

	if(g_rc32k_cali_flag != 0 || g_LUT_cleanup_flag != 0) 
	{
		return SLEEP_NOT_ALLOW;
	}

	return SLEEP_PERMITT;

}

void StandBy_Context_Save(void)
{
	PRCM_AonWkupSourceEnable(AON_WKUP_SOURCE_LPUART1 | AON_WKUP_SOURCE_APTICK);
}

uint8_t g_tsensor_power_before_standby = 0;//记录进入standby前tsensor使用使能状态，睡眠后恢复
static uint8_t g_standby_lpldo_delay_cnt;
void StandBy_Power_Manage(void)
{
    // 配置GPIO33、34、37为输入下拉，取消漏电配置
    Gpio_LeakCurrentDisable();

    if(g_ADCVref == ADC_VREF_VALUE_2P2V)    //VREF =2.2V
    {
          HWREGB(0x40004851) = 0;
    }
	if(HWREGB(0x40004852) & 0X8 )
	{	
		ADC_TsensorPowerDIS();
		g_tsensor_power_before_standby = 1;
	}
    //若存在调用adc等接口，trxbg force on的场景，此处关闭trxbg
    ADC_Trxbg_InstantClose();

	//启动cp且cp仍在睡眠中；cp启动则flash必然是开启的
	if(CP_IS_STANDBY() == true)
	{
		HWREGB(BAK_MEM_LPM_FLASH_STATUS) = STANDBY_FLASH_STATUS_DEEP_POWER_DOWN;
		g_sysclk_sel_before_sleep = 0xff;
		//二次判断，防止cp唤醒后执行flash
		if(CP_IS_STANDBY() == true)
		{
			g_sysclk_sel_before_sleep = (AONPRCM->SYSCLK_CTRL & SYSCLK_SRC_SEL_Msk)>>SYSCLK_SRC_SEL_Pos;
			if( g_sysclk_sel_before_sleep ==  SYSCLK_SRC_PLL)
			{
				//当前系统时钟选择pll，则切换为非pll，并切换sido lp
				PRCM_SlowfreqDivDisable();
				Sys_Clk_Src_Select(SYSCLK_SRC_HRC);
				HWREGB(AON_SYSCLK_CTRL2) = 0x02; // pll force off
                HWREGB(COREPRCM_ADIF_BASE + 0x58) &= 0xE3; //disable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
				HWREGB(0x40000059) = 0x7;   // itune = 7 for xtal on after standby
				//切换sido mode
				SIDO_LPMODE();
			}

            // ecobit进低功耗前需提前800us打开，大约26个utc clk
            HWREGB(0x40000800) |= (0x01 << 6);
            if (AONPRCM->CORE_PWR_CTRL3 & IOLDO1_NORMAL_CTL_Msk)
            {
                // 当ioldo1从normal切到lp需要20个utc clk所以只需再延时6个utc clk
                g_standby_lpldo_delay_cnt = 26-20;
            }
            else
            {
                g_standby_lpldo_delay_cnt = 26;
            }
            
			//flash 进入deep power down模式，ioldo为psm,保留standby期间使用GPIO的功能
			FlashEnterLPMode();
			//standby时psmmode，需要压力测试
			PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_PSM_Enable);
            AONPRCM->FLASH_VCC_IO_CTRL = (AONPRCM->FLASH_VCC_IO_CTRL & ~0x03) | 0x01;//flash vcc not off in standby,off in deepsleep
            PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_PSM_Enable);
			// if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)
			// {
			// 	//如果ioldo1_byp_flag置1，深睡前需要置aon_psmioldo1_byp_en = 1,Deepsleep唤醒后需要清0；
			// 	if(PRCM_IOLDO1_BypFlag_Get() == 1)
			// 	{
			// 		PRCM_IOLDO1_Enable_PSM_Byp();
			// 	}
			// }
		}
		else
		{
			HWREGB(BAK_MEM_LPM_FLASH_STATUS) = STANDBY_FLASH_STATUS_NORMAL;
		}
	}
	else
	{
		//单核或cp已不在wfi状态时，暂且配置standby时lpmode
        PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_PSM_Enable);
        AONPRCM->FLASH_VCC_IO_CTRL = (AONPRCM->FLASH_VCC_IO_CTRL & ~0x03) | 0x01;//flash vcc not off in standby,off in deepsleep
        PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_PSM_Enable);
	}
		
	// force idle
	COREPRCM->LPM_CTRL |= 0x1f0001;

    //使用standby0.9v，vddaon lpldo供电
    AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
    AONPRCM->CORE_PWR_CTRL0 = 0x63;//sidolp 1.2v,standby 0.9v	
    AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
}

void StandBy_Entry(void)
{
	if(RTC_Alarm_Timeout_Check() == SLEEP_NOT_ALLOW)   //BUG8986 规避代码：若距离最近超时的RTC事件不足1ms，则退出standby
	{
		return;
	}

    // ecobit进低功耗前需提前800us打开，此处加延时
    utc_cnt_delay(g_standby_lpldo_delay_cnt);

#if (AT_LPUART == 1)
    if( !at_uart_idle() )
    {
        return;
    }
#endif
#if(DEBUG_MODULE_RUN_TIME == 1)
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x10) = *(uint32_t *)0x40001008;
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x14) = UTC_ClkCntGet(0);
#endif
	
	//此处再次判断CP状态，防止CP已退出wfi，此时需要AP立即恢复sido及pll，否则CP可能在等待PLL卡死
	if((CP_IS_STANDBY() == true) || (CP_Is_Alive() == false))
	{
		PRCM_LPM_ApReqLpMode(LOW_POWER_MODE_STANDBY);

        AONPRCM->AONGPREG2 = (AONPRCM->AONGPREG2 & 0x1F) | (PRCM_LPUARTclkDivGet() << 5);  //AONGPREG2的bit5-7用于记录lpuart分频
        PRCM_LPUA1_ClkSet(AON_HRC_DIV16);

        AONPRCM->AONPWR_CTRL &= ~0x1000;    //vddaon_pwrsel_standby = 0,select lpldo 1.2v
		/* Set SLEEPDEEP bit of Cortex System Control Register */
		SCB->SCR |= 0x04;

		__WFI();

		SCB->SCR &= ~0x04;

        // 退出standby恢复lpuart分频系数
        if (AONPRCM->AONGPREG2 & 0xE0)
        {
            PRCM_LPUA1_ClkSet(AONPRCM->AONGPREG2 >> 5);
            AONPRCM->AONGPREG2 &= 0x1F;
        }
        
		PRCM_LPM_ApReqLpMode(LOW_POWER_MODE_NORMAL);
	}
	
    // 退出standby清除ecobit
    HWREGB(0x40000800) &= ~(0x01 << 6);
}

void StandBy_Power_Recover(void)
{
    // 规避大功率发射IO口电压波动问题
    Gpio_LeakCurrentEnable();
    
    if(g_ADCVref == ADC_VREF_VALUE_2P2V)    //VREF =2.2V
    {
		HWREGB(0x40004851) = 0x02;
    }
	if( g_tsensor_power_before_standby)
	{
		ADC_TsensorPowerEN();
		g_tsensor_power_before_standby = 0;
	}

	//cp启动的情况下必然选择pll。cp未启动可能选择pll
	if(HWREGB(BAK_MEM_LPM_FLASH_STATUS) != STANDBY_FLASH_STATUS_NORMAL)
	{
		if(g_sysclk_sel_before_sleep ==  SYSCLK_SRC_PLL)
		{
			//当前系统时钟选择pll，则切换为非pll，并切换sido lp
			SIDO_NORMAL();
			//PRCM_SlowfreqDivDisable();
            if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
            {
                HWREGB(0x40000059) = 0x7;   // itune = 7
                HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
            }

            HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
            while (!(COREPRCM->ANAXTALRDY & 0x01)); //xtal ready
            HWREGB(0x40000059) = 0x4;   // itune = 4

            HWREGB(COREPRCM_ADIF_BASE + 0x58) = 0x5c; //enable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
            HWREGB(AON_SYSCLK_CTRL2) = 0;//disable:xtal,pll power on
		}

		//在等待pll的过程中，执行flash的恢复配置工作
		if(HWREGB(BAK_MEM_LPM_FLASH_STATUS) == STANDBY_FLASH_STATUS_DEEP_POWER_DOWN)
		{
			FlashExitLPMode();
		}
		else if(HWREGB(BAK_MEM_LPM_FLASH_STATUS) == STANDBY_FLASH_STATUS_FLASH_VCC_OFF)
		{
			PRCM_FLASH_VCC_On();
			//flash手册上要求是2ms；此处delay不会延长standby恢复时长，在pll恢复的时间内执行
			utc_cnt_delay(64);//utc delay 1.8ms左右,最多增加延时到2ms<pll lock，否则会拉长standby唤醒时间
			//无需初始化flash，配置未丢失，flash的protect是非易失
		}
		else if(HWREGB(BAK_MEM_LPM_FLASH_STATUS) == STANDBY_FLASH_STATUS_VDDIO_OFF)
		{
			utc_cnt_delay(64);//utc delay 1.8ms左右,最多增加延时到2ms<pll lock，否则会拉长standby唤醒时间
			//无需初始化flash，配置未丢失，flash的protect是非易失
		}

		if(g_sysclk_sel_before_sleep ==  SYSCLK_SRC_PLL)
			Sys_Clk_Src_Select(SYSCLK_SRC_PLL);

		HWREGB(BAK_MEM_LPM_FLASH_STATUS) = STANDBY_FLASH_STATUS_NORMAL;
	}

    //设置lpmode，可能会影响LLPM_VER版本深睡时psmioldo的开启，先注释掉
    //2023/12/11修改：LLPM_VER也会在睡眠前重复设置ioldo状态，此处配置不影响
    //此处可以不加模组宏区分，因为opencpu目前不进standby
    if(g_nv_ioldo1_mode && (CP_Is_Alive() == true))
    {
        PRCM_IOLDO1_ModeCtl(IOLDO1_NORMAL_Enable);
    }
    else
    {
        PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);
    }

    if (CP_Is_Alive() == true)
    {
        // 解决simvcc打开，ioldo2电压被下拉500mv
        PRCM_IOLDO2_ModeCtl(IOLDO2_NORMAL_Enable);
    }
    else
    {
        PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_Enable);
    }

    // if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)      
	// {
    //     //睡眠失败或退出时aon_psmioldo1_byp_en强制清0；
    //     PRCM_IOLDO1_Disable_PSM_Byp();
	// }

}

void StandBy_Context_Recover(void)
{
#if (MODULE_VER && AT_WAKEUP_SUPPORT)
	// 唤醒原因的获取必须放在清除lpuart使能之前，否则获取失败！
	if ((AONPRCM->WAKEUP_STATUS) & (WAKEUP_STATUS_LPUART_WKUP_Msk))
	{
#if AT_LPUART
		/*STANDBY唤醒*/
		g_wakup_at = 1;
		/*外部AT唤醒STANDBY，波特率高于9600，延迟若干秒开启STANDBY*/
		at_uart_standby_ctl();
#endif		
	}
#endif

	PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_LPUART1 | AON_WKUP_SOURCE_APTICK);
	// AONPRCM->AONPWR_CTRL &= ~0x04;//AP standby pd
}

#endif



__OPENCPU_FUNC int WFI_Admittance_Check(void)
{
	/*接收到的AT命令会挂链表，在main主函数中遍历执行，期间可能进入WFI，造成不及时处理*/
#if MODULE_VER
	if(is_event_set(EVENT_AT_URC))
#else  /*OPENCPU场景，AP核不进WFI功耗无特别增加，为了方便实现，此处关闭*/
	if(CP_Is_Alive())
#endif
		return SLEEP_NOT_ALLOW;

#if (AT_LPUART == 1)
	/*main主函数中有待输出的字符串或者AT命令是否未处理，若未处理则退出睡眠*/
	if(at_uart_idle() == false)
	    return SLEEP_NOT_ALLOW;
#endif


	if((lpm_lock & (((uint64_t)1<<WFI_END) - ((uint64_t)1<<WFI_BASE))) != 0)
		return SLEEP_NOT_ALLOW;

	if(g_rc32k_cali_flag != 0)
		return SLEEP_NOT_ALLOW;

	return SLEEP_PERMITT;
}

void WFI_Entry(void)
{
	__WFI();
}

// 根据CP核睡眠模式，AP裁决睡眠模式
LPM_MODE LpmSleepEstimate(LPM_MODE sleepmode)
{
	if (CP_Is_Alive() == false)
    {
		return sleepmode;
    }

	if(sleepmode == LPM_DSLEEP)
	{
		if(CP_IS_DEEPSLEEP())
		{
			sleepmode = LPM_DSLEEP;
		}
		else
		{

			sleepmode = LPM_STANDBY;
		}
	}

	if(sleepmode == LPM_STANDBY)
	{
#if(MODULE_VER != 0)  
		if(CP_IS_STANDBY())
		{
			sleepmode = LPM_STANDBY;
		}
		else
		{
			sleepmode = LPM_WFI;
		}
#else
		sleepmode = LPM_WFI;   //opencpu形态不支持standby，直接降级为wfi
#endif
	}

	delay_func_us(10);   //CP进入睡眠后会trigger AP，此处delay一段时间，保证锁中断前能处理完该核间中断

	return sleepmode;
}

extern volatile uint32_t g_fast_startup_flag;

bool Enter_LowPower_Mode(LPM_MODE sleepmode)
{
	LPM_MODE degraded_sleepmode;
	//xy_printf("before HAL_LPM_EnterDeepSleep %u\r\n", (unsigned int)lpm_lock);
	Debug_Runtime_Add("(In Enter_LowPower_Mode)LpmSleepEstimate start");

	//获取当前CP睡眠状态，据此断定AP当前实际可进入的睡眠模式
	degraded_sleepmode = LpmSleepEstimate(sleepmode);

	Debug_Runtime_Add("LpmSleepEstimate stop RC32K_CALIB_WakeUp_Dis start");

	if (degraded_sleepmode == LPM_DSLEEP)
	{
		if(g_LUT_clean_period_min == 0)
		{
			RC32K_CALIB_WakeUp_Dis();  // 关闭rc cali中断使能位，避免异常唤醒！
		}
		else
		{
			RC32K_CALIB_WakeUp_En();
		}

		DisablePrimask();
		Debug_Runtime_Add("RC32K_CALIB_WakeUp config stop DeepSleep_Admittance_Check start");

		if (DeepSleep_Admittance_Check() != SLEEP_PERMITT)
		{
			EnablePrimask();
			return 0;
		}
		Debug_Runtime_Add("DeepSleep_Admittance_Check stop DeepSleep_Context_Save start");

		DeepSleep_Context_Save();

		Debug_Runtime_Add("DeepSleep_Context_Save stop DeepSleep_Power_Manage start");
		if((CP_Is_Alive() == false) || (CP_IS_DEEPSLEEP() == true))
		{
			DeepSleep_Power_Manage();

			Debug_Runtime_Add("DeepSleep_Power_Manage stop Deepsleep_Entry start");

            Deepsleep_Entry();

			Debug_Runtime_Add("Deepsleep_Entry stop DeepSleep_Power_Recover start");

			DeepSleep_Power_Recover();
		}
		Debug_Runtime_Add("DeepSleep_Power_Recover stop DeepSleep_Context_Recover start");
		DeepSleep_Context_Recover();
		Debug_Runtime_Add("DeepSleep_Context_Recover stop");
	}
#if(MODULE_VER != 0)     //OpenCpu形态不支持standby，为压缩内存，此处不编译
	else if (degraded_sleepmode == LPM_STANDBY)
	{
		RC32K_CALIB_WakeUp_En();

		DisablePrimask();

		if (StandBy_Admittance_Check() != SLEEP_PERMITT)
		{
			EnablePrimask();
			return 0;
		}

		StandBy_Context_Save();

		if((CP_Is_Alive() == false) || (CP_IS_STANDBY() == true))
		{
			StandBy_Power_Manage();

			StandBy_Entry();

			StandBy_Power_Recover();
		}

		StandBy_Context_Recover();

	}
#endif
	else if (degraded_sleepmode == LPM_WFI)
	{
		DisablePrimask();

		if (WFI_Admittance_Check() != SLEEP_PERMITT)
		{
			EnablePrimask();
			return 0;
		}

		WFI_Entry();
	}


	EnablePrimask();
	if(g_fast_startup_flag == AP_WAKEUP_FASTRECOVERY_AFTER)
	{
		// 从深睡中快速恢复，返回1
		g_fast_startup_flag = AP_WAKEUP_NORMAL;			
		return 1;
	}
	else
	{
		// 深睡失败，返回0
		// 对Standby\WFI而言，返回值无意义
		g_fast_startup_flag = AP_WAKEUP_NORMAL;
		return 0;
	}
}
