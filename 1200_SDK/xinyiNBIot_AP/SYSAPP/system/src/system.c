/*
 * @file  system.c
 * @brief 一些基础系统级的API接口
 * @warning
 */
#include <string.h>
#include "tick.h"
#include "mcnt.h"
#include "prcm.h"
#include "utc.h"
#include "sys_ipc.h"
#include "watchdog.h"
#include "qspi_flash.h"
#include "flash_vendor.h"
#include "factory_nv.h"
#include "fast_recovery.h"
#include "xy_memmap.h"
#include "sys_clk.h"
#include "xy_lpm.h"
#include "xy_system.h"
#include "gpio.h"
#include "rc32k.h"
#include "sys_rc32k.h"
#include "nvic.h"
#include "adc.h"
#include "sys_clk.h"
#include "xy_flash.h"
#include "hw_gpio.h"
#include "hal_gpio.h"
#include "xy_cp.h"
#include "common.h"
#include "sys_proc.h"
#include "soc_control.h"
#include "runtime_dbg.h"
#include "dyn_load.h"
#include "module_runtime_dbg.h"
#include "adc_adapt.h"
#include "cache.h"
#include "ap_watchdog.h"
#include "mpu.h"
#include "utc_watchdog.h"
#include "xy_ftl.h"
#include "at_uart.h"
#include "mpu_protect.h"

// hrc 26M时钟上电测量值，深睡唤醒需要保持住，以免多次测量
#define FLASH_26MHRCCALI_TBL_LEN    4
// 用于判断是否需要更新校准值
#define FLASH_26MHRCCALI_CHECK_LEN  8

uint16_t g_pri_nesting = 1;
uint16_t g_base_nesting = 0;

uint8_t g_cldo_set_vol = 0;
uint8_t g_nv_ioldo1_mode = 1;
uint8_t g_off_wakeupRXD = 0;	//对应出厂NV参数off_wakeupRXD，是否关闭串口唤醒功能
/*AP核触发的死机置0，CP核触发的死机置1*/
int g_dump_core = -1;

m3_common_reg_t m3_reg = {0};
m3_fault_reg_t m3_fault = {0};
m3_int_push_reg_t *m3_push_reg = NULL;
char* g_assert_file = NULL;
uint32_t g_assert_line = 0;
char* g_cp_assert_file = NULL;
uint32_t g_cp_assert_line = 0;
extern void IPC_Init_WAKEUP_DSLEEP(void);
uint8_t REG_Bus_Field_Set(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long ulValue);
uint32_t xy_chksum(const void *dataptr, int32_t len);
extern void init_32k_clk(void);

void EnablePrimask(void)
{
	if (g_pri_nesting)
		g_pri_nesting--;
	if (g_pri_nesting == 0)
		__enable_irq();
}

void DisablePrimask(void)
{
	__disable_irq();
	g_pri_nesting++;
}

void EnableInterrupt(void)
{
	if (g_base_nesting)
		g_base_nesting--;
	if (g_base_nesting == 0)
		__set_BASEPRI(0);
}

void DisableInterrupt(void)
{
	__set_BASEPRI(1 << 5);
	g_base_nesting++;
}

extern int Time_Init(void);
__FLASH_FUNC int clk_tick_init(void)
{
	Time_Init();

	if (Get_Boot_Reason() != WAKEUP_DSLEEP)
	{
		Tick_APIntDisable(TICK_INT_AP_PERIOD_Msk | TICK_INT_AP_COMPARE_Msk | TICK_INT_AP_OVERFLOW_Msk);
 		Tick_PrescaleSet(TICK_PRESCALE_DIV32);

		Tick_TimerEnable();
	}

	NVIC_SetPriority(CLKTIM_IRQn, 2);
	IntEnable(INT_TICK);

	return 1;
}

/*表计类执行快速恢复，进而深睡唤醒不会运行此函数*/
__FLASH_FUNC void measure_hrc_clk(void)
{
	uint32_t hrc_clk = 0;
	READ_CALI_PARAM(hrc_clk,&hrc_clk);

	// 上电、异常掉电进行频率表校验，26Mhrc校准值存入8字节，通过比较前后4字节来判断是否进行重新校准
	if ((uint32_t)hrc_clk < 23000000 || (uint32_t)hrc_clk > 27000000)
	{
		PRCM_ClockEnable(CORE_CKG_CTL_MCNT_EN);

		// 因为MCNT校准低速时钟需要系统时钟切换至更高的速度，所以切换当前系统时钟至XTAL26M，用于校准HRC26M频率
		uint32_t clockSource = PRCM_SysclkSrcGet();
		if (SYSCLK_SRC_XTAL != clockSource)
		{
            if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
            {
                HWREGB(0x40000059) = 0x7;   // itune = 7
                HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
            }

            HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
            while (!(COREPRCM->ANAXTALRDY & 0x01)); //xtal ready
			Sys_Clk_Src_Select(SYSCLK_SRC_XTAL); 
		}

		MCNT_SetClkSrc(1);

		NVIC_DisableIRQ(MCNT_IRQn);
		NVIC_ClearPendingIRQ(MCNT_IRQn);

		MCNT_Stop();
		MCNT_SetCNT32k(XY_XTAL_CLK/5/8); // about 200ms
		MCNT_Start();

		while (0 == NVIC_GetPendingIRQ(MCNT_IRQn));

		// 保存到出厂NV
		// NV:rc32k_freq or misc_freq中存放的是rc频率的十倍，表示晶振误差为1ppm。
		hrc_clk = (uint64_t)GetSysClockFreq() * (XY_XTAL_CLK/5/8) * 10 / (MCNT_GetMCNT() - 1);

		PRCM_ClockDisable(CORE_CKG_CTL_MCNT_EN);

		g_hrc_clock = (uint32_t)((hrc_clk + 5) / 10 * 8);

		WRITE_CALI_PARAM(hrc_clk,&g_hrc_clock);
	}
	else
		g_hrc_clock = (uint32_t)hrc_clk;
}

/*GPIO8(AP_SWDCLK)、GPIO13(AP_SWDIO)为硬件默认的AP_SWD接口，软件可不做任何配置即可使用 */
void AP_Jlink_Reset(GPIO_PadTypeDef clk_pad,GPIO_PadTypeDef io_pad)
{
	GPIO_InitTypeDef gpio_init = {0};
	
    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

	GPIO_AllocateRemove(GPIO_AP_SWCLKTCK);
	GPIO_AllocateRemove(GPIO_AP_SWDIOTMS);

	gpio_init.Pin = clk_pad;
	gpio_init.PinRemap = GPIO_AP_SWCLKTCK;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&gpio_init);

	gpio_init.Pin = io_pad;
	gpio_init.PinRemap = GPIO_AP_SWDIOTMS;
    gpio_init.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&gpio_init);
}


void CP_Jlink_Reset(GPIO_PadTypeDef clk_pad,GPIO_PadTypeDef io_pad)
{
	GPIO_InitTypeDef GPIO_Handle = {0};

    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

	GPIO_AllocateRemove(GPIO_CP_SWCLKTCK);
	GPIO_AllocateRemove(GPIO_CP_SWDIOTMS);

	GPIO_Handle.Pin = clk_pad;
	GPIO_Handle.PinRemap = GPIO_CP_SWCLKTCK;
	GPIO_Handle.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&GPIO_Handle);

	GPIO_Handle.Pin = io_pad;
	GPIO_Handle.PinRemap = GPIO_CP_SWDIOTMS;
	GPIO_Handle.Mode = GPIO_MODE_HW_PER;
	HAL_GPIO_Init(&GPIO_Handle);
}

//配置所有gpio input pulldown，除了ap_swd:gpio8/13
__FLASH_FUNC void gpio_lpm_init(void)
{
	//gpio mode
    GPIO->MODE[0] &= ~0xFFFFDEFF;
    GPIO->MODE[1] &= ~0xFFFFFFFF;

    //register control
    GPIO->CTL[0]  &= ~0xFFFFDEFF;
    GPIO->CTL[1]  &= ~0xFFFFFFFF;

    //enable input and disable output
    GPIO->INPUT_EN[0]  |= 0xFFFFDEFF;
    GPIO->INPUT_EN[1]  |= 0xFFFFFFFF;
    GPIO->OUTPUT_EN[0] |= 0xFFFFDEFF;
    GPIO->OUTPUT_EN[1] |= 0xFFFFFFFF;

    //enable pulldown and disable pullup
    GPIO->PULL_UP[0]   |= 0xFFFFDEFF;
    GPIO->PULL_UP[1]   |= 0xFFFFFFFF;
    GPIO->PULL_DOWN[0] &= ~0xFFFFDEFF;
    GPIO->PULL_DOWN[1] &= ~0xFFFFFFFF;
}

extern uint32_t _Boot_Load_Addr;
extern uint32_t _Boot_Exec_Addr;
extern uint32_t _Boot_Length;


/*************************************** WKUP_EN引脚的复位时序 **************************************
一、当resetctl NV值大于等于0且小于等于11时，wkup_en引脚为下拉低电平状态，外部按键也需要保证为默认下拉状态。
	例如，当resetctl NV值为0时，则有以下唤醒、复位时序：
	唤醒时序：当按键高电平时长大于160ms小于5.12s，且产生下降沿后触发唤醒；
	复位时序：当按键高电平时长大于5.12s，且产生下降沿后触发复位。
二、当resetctl NV值大于等于12且小于等于23时，wkup_en引脚为上拉高电平状态，外部按键也需要保证为默认上拉状态。
	例如，当resetctl NV值为12时，则有以下唤醒、复位时序：
	唤醒时序：当按键低电平时长大于160ms小于5.12s，且产生上升沿后触发唤醒；
	复位时序：当按键低电平时长大于5.12s，且产生上升沿后触发复位。
***************************************************************************************************/
__FLASH_FUNC void wkup_en_init(void)
{
	uint8_t rstwkp_pw = 0;
	WakeupEn_InitTypeDef wkup_en = {0};

	// 获取resetctl
	rstwkp_pw = READ_FAC_NV(uint8_t, resetctl);

	// 配置唤醒电平
	if ( rstwkp_pw <=11 ) //wkup_en引脚既有复位功能又有唤醒功能，设置其默认低电平状态，唤醒电平为高电平，下降沿触发
	{
		wkup_en.mode = WAKEUP_AND_RESET;
		wkup_en.pull = PIN_PULLDOWN;
		wkup_en.wkup_polarity = POLARITY_HIGH;
		wkup_en.wkup_edge = FALLING;
	}
	else if ( rstwkp_pw >=12 && rstwkp_pw <=23 ) //wkup_en引脚既有复位功能又有唤醒功能，设置其默认高电平状态，唤醒电平为低电平，上升沿触发
	{
		wkup_en.mode = WAKEUP_AND_RESET;
		wkup_en.pull = PIN_PULLUP;
		wkup_en.wkup_polarity = POLARITY_LOW;
		wkup_en.wkup_edge = RISING;

		rstwkp_pw -= 12;
	}
	else
	{
		xy_assert(0);
	}

	// 配置唤醒脉宽
	switch(rstwkp_pw)
	{
		case 1:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_320ms; break; }
		case 2:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_1p28s; break; }
		case 3:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_2p56s; break; }
		case 4:  { wkup_en.wakeup_time = PULSE_10ms; wkup_en.reset_time = PLUSE_5p12s; break; }
		case 5:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_320ms; break; }
		case 6:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_1p28s; break; }
		case 7:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_2p56s; break; }
		case 8:  { wkup_en.wakeup_time = PULSE_20ms; wkup_en.reset_time = PLUSE_5p12s; break; }
		case 9:  { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_320ms; break; }
		case 10: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_1p28s; break; }
		case 11: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_2p56s; break; } 
		case 0: { wkup_en.wakeup_time = PULSE_160ms; wkup_en.reset_time = PLUSE_5p12s; break; } //NV默认值
		default:;
	}

	// 初始化wkup_en引脚
	WakeupEn_Init(&wkup_en);
}

__FLASH_FUNC void lpm_init(void)
{
	PRCM_SetWkupAonDly(15,15,52,10,15); //ncorerst_dly, sidordy_dly, hrc_dly, xtalrdy_dly, plllock_dly  
	
	// SVD config  (降低睡眠功耗，全局上电后做一次即可！！!)；A1应该不再需要，先注释掉
	// SVD->CFG = (SVD->CFG & SVD_FILT_CFG_ResetMsk) | SVD_MODE_NO_FILTER;
    // SVD->CFG &= SVD_FILT_CFG_ResetMsk;
	
	//清除WAKEUP_STATUS寄存器里LPUART唤醒原因标志位
	PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_LPUART1);
	
	/* wkup_en pin config */	
	wkup_en_init();

	/* wakeup setting */
	AONPRCM->AON_WAKUP_ENA = 1;	 // globle wakeup enable single of low_power mode
	PRCM_ApIntWakeupEnable();	 // AP wakeup enable
	
	NVIC_SetPriority(WAKEUP_IRQn, 1);
	IntEnable(INT_WAKEUP);

	NVIC_SetPriority(UTC_IRQn, 2);
	IntEnable(INT_UTC);


#if (MODULE_VER == 0x0) 	// OPENCPU形态，支持快速恢复
    PRCM_SramPowerModeSet(SH_SRAM8K|AP_SRAM0|AP_SRAM1, SRAM_POWER_MODE_FORCE_ON);
	
    HWREG(DEEPSLEEP_WAKEUP_AP_JUMP_ADDR) = SRAM_BASE;
#else
    PRCM_SramPowerModeSet(SH_SRAM8K,SRAM_POWER_MODE_FORCE_ON);
#endif

    PRCM_SetRetldoVol();    //retldo 0.9v
	AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
//表计为节省功耗，降低sido的电压,在初始化阶段配置一次，深睡唤醒依然生效
#if(LLPM_VER == 1)
    //AONPRCM->AONPWR_CTRL |= 0x100;//ds_exit_mcu_en = 1
    HWREG(AONPRCM_BASE + 0x64) = 0;//peri wakeup cpen = 0
	AONPRCM->CORE_PWR_CTRL0 = 0xB4;//sidolp 1.05v,deepsleep 1.0v
	//表计一定用快速恢复,且sido供电,关闭retldo
	AONPRCM->AONPWR_CTRL |= 0xC800;//sido is on in deepsleep;sel sido lpmode as vddret source;sel sido lpmode as vddaon source
	PRCM_PowerOffRetldo();
    //表计全程维持此配置，避免进入deepsleep时每次配置耗时
    PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_PSM_Enable);

    AONPRCM->FLASH_VCC_IO_CTRL &= ~0x03;//flash vcc not off automatic in deepsleep&standby

    PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_PSM_Enable);

    PRCM_ForceCPOff_Enable();

    AONPRCM->AONGPREG2 |= 0x04; // new修复 Bug 12277 - opencpu深睡唤醒lsioclk切换为hrc较慢

#else
	PRCM_PowerOnRetldo(); //retldo 需要稳定时间，因此必须在初始化阶段开启（表计必然使用sido，因此无需开启retldo）
	AONPRCM->CORE_PWR_CTRL0 = 0x64;//sidolp 1.2v,standby 1.0v	

    PRCM_IOLDO1_ModeCtl(IOLDO1_LPMODE_Enable);

    PRCM_IOLDO2_ModeCtl(IOLDO2_LPMODE_Enable);
#endif
    AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0

	/*CP核禁止写FLASH，则出厂/非易变NV保存在64K内存上，深睡维持供电*/
#if (MODULE_VER==0)
	PRCM_SramPowerModeSet(SH_SRAM64K, SRAM_POWER_MODE_FORCE_ON); 
#endif

#if DYN_LOAD
    //opencpu动态加载版本默认占用CP SRAM或者shareram保持常开，且深睡保持， 根据动态库代码段大小选择shareram或者CP SRAM
	PRCM_SramPowerModeSet(SH_SRAM64K, SRAM_POWER_MODE_FORCE_ON);
#endif
	PRCM_SramPowerModeSet(CP_SRAM0|CP_SRAM1, SRAM_POWER_MODE_FORCE_OFF);
	//power off peri2
	//COREPRCM->PWRCTL_CFG |= 0x02; //起CP及SPI等外设使用时需要peri2
	//表计类power down flash
	//Flash_Poweroff();

	//gpio_lpm_init();

#if (MODULE_VER != 0)
	extern volatile uint32_t g_LUT_clean_period_min;
	RC32K_cleanup_event_set(g_LUT_clean_period_min);
#endif
}

/**
 * @brief  配置GPIO52、53、54为推挽输出低
 */
void Gpio_52_54_Out_Low(void)
{
    DisablePrimask();
    
    GPIO->MODE[1] &=      ~( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    GPIO->INPUT_EN[1] &=  ~( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    GPIO->OUTPUT_EN[1] &= ~( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    GPIO->PULL_UP[1] |=    ( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    GPIO->PULL_DOWN[1] |=  ( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    GPIO->DOUT[1] &=      ~( (1<<(52-32)) | (1<<(53-32)) | (1<<(54-32)));
    EnablePrimask();
}

extern void VPortDefineHeapRegions(void);
extern void rtc_init_intern(void);
extern void xy_ftl_init();

/*非快速恢复的系统上电执行初始化*/
__FLASH_FUNC void Sdk_Hardware_Init(void)
{
    AONPRCM->BOOTMODE =0x80;//force flash boot	

    HWREGB(0x40000801) = ((HWREGB(0x40000801) & (~0x18)) | 0x08);   //Set UVLO = 1,PDR_V=1.71v 

#if (MODULE_VER != 0)
    // bug15477 new模组形态上电配置aon_cntl_85为1
    if (HWREG(BAK_MEM_OTP_SOC_ID_BASE) >= 0x1)
    {
        HWREGB(0x4000080A) |= 0x20; // aon_cnt_85
    }
#endif

    AONPRCM->AONCLK_CTRL &= ~0x01;//dis hrc auto clock detection

#if (MODULE_VER == 0)
    //设置hrc的ldo为1.04v
    HWREGB(0x40000811) = (HWREGB(0x40000811) & (~0xf)) | 0x4;
#endif

    HWREGB(0x40000800) &= ~0x70;   //ecobit clear，lpldo 1.2V

	get_adc_caliv();

	g_cldo_set_vol = READ_FAC_NV(uint8_t,cldo_set_vol);  //get nv->cldo_set_vol

	if(CP_Is_Alive() == 0)
	{
		// 解决一级BOOT DMA使用bug
		// HWREGB(0x40050004) = 0;
		// HWREGB(0x40050204) = 0;
		// DMAChannelTransferStop(DMA_CHANNEL_1);
	}

	VPortDefineHeapRegions();

	xy_ftl_init();
	
	measure_hrc_clk();

	//不能与measure_hrc_clk调换先后位置，init_32k_clk使用到ADC接口，入参的mclk(lsio)在measure_hrc_clk函数中获取；
	init_32k_clk();

	System_Clock_CFG();

	clk_tick_init();

	rtc_init_intern();

	NVIC_SetVectorTable(g_pfnVectors);
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_3);

	NVIC_SetPriority(PendSV_IRQn, 7);

    lpm_init();

#if (MODULE_VER != 0) 
    // 配置GPIO52、53、54推完输出0
    Gpio_52_54_Out_Low();
#endif


	AGPIO_GPIO0_7_Update();

}

//查询flash是否处于deep-powerdown-mode的变量,默认上电/重启/非快速恢复唤醒后。flash处于正常状态
static uint8_t s_flash_lpmode = 0;

uint8_t is_FlashLPMode(void)
{
	return s_flash_lpmode;
}

void FlashEnterLPMode(void)
{
    if(s_flash_lpmode == 0)
    {
        //PRCM_ClockEnable(CORE_CKG_CTL_QSPI_EN);
        FLASH_LowPowerEnter();
        //need tDp > 3us
        //HWREGB(AON_AONGPREG1) |= 0x80;//warm reset in bootrom
        //HWREGB(AON_AONGPREG2) |= 0x03;//delay in bootrom,tRST 69us@XTAL26M
        HWREG(AON_AONGPREG0)    |= 0x038000;//delay in bootrom,tRST 69us@XTAL26M,warm reset in bootrom

        s_flash_lpmode = 1;

        //AONPRCM->AONPWR_CTRL |= 0x100000;//qspi_pullupena = 1，避免深睡后的电流鼓包
        HWREGB(AONPRCM_BASE + 0x32) = 0x10;//qspi_pullupena = 1，避免深睡后的电流鼓包
        //PRCM_ClockDisable(CORE_CKG_CTL_QSPI_EN);//这里如果关闭了qspi clk，需要考虑没睡下去或者继续使用flash时打开clk
    }
}

void FlashExitLPMode(void)
{
    if(s_flash_lpmode != 0)
    {
        //PRCM_ClockEnable(CORE_CKG_CTL_QSPI_EN);
        FLASH_LowPowerExit();
    	utc_cnt_delay(2);//tRES1 > 30us
        s_flash_lpmode = 0;
    }
    //这里没有放在上面if()中，是考虑非快速恢复的深睡唤醒，s_flash_lpmode初始值为0，但aon中可能是上拉，影响flash通信的功耗
    //AONPRCM->AONPWR_CTRL &= ~0x100000;//qspi_pullupena = 0
    HWREGB(AONPRCM_BASE + 0x32) = 0;//qspi_pullupena = 0
}

void FlashExitLPMode_NoDelay(void)
{
    if(s_flash_lpmode != 0)
    {
        //PRCM_ClockEnable(CORE_CKG_CTL_QSPI_EN);
        FLASH_LowPowerExit();
    	//utc_cnt_delay(1);//tRES1 > 30us
        s_flash_lpmode = 0;
    }
    //这里没有放在上面if()中，是考虑非快速恢复的深睡唤醒，s_flash_lpmode初始值为0，但aon中可能是上拉，影响flash通信的功耗
    //AONPRCM->AONPWR_CTRL &= ~0x100000;//qspi_pullupena = 0
    HWREGB(AONPRCM_BASE + 0x32) = 0;//qspi_pullupena = 0
}

/**
  * @brief 仅在深睡时sido不开启的场景下，切换aonclk
  * @return None
  */
__RAM_FUNC void Switch_VDDAON_32K(void)
{

    if(AONPRCM->LPUA1_CLKFLAG != LPUART1_CLKSRC_32K)
    {
        //若lpuart选择hrc，则切换成utc，并且记录hrc选择，唤醒后重配选择hrc
        AONPRCM->AONGPREG2 = (AONPRCM->AONGPREG2 & 0x1F) | ((AONPRCM->LPUA1_CTRL & 0x70)<<1);//AONGPREG2的高3bit记录lpuart时钟选择
        AONPRCM->LPUA1_CTRL = (AONPRCM->LPUA1_CTRL & ~0x70) | (AON_UTC_CLK << 4);  
        while(AONPRCM->LPUA1_CLKFLAG != LPUART1_CLKSRC_32K);
    }
#if(MODULE_VER != 0)   
    //aonsys select 32k，模组版本保持原来的顺序不变。open将切换时机调整靠后的地方加快睡眠速度
    AONPRCM->AONCLK_CTRL &= ~0x02;
    while( (AONPRCM->AONCLK_FLAG & 0x10));
#endif    

    //目前非模组一定执行快速恢复，wfi前都可以关闭hrc，此处关闭hrc
    if((AONPRCM->AONGPREG2 & 0x10) || (MODULE_VER == 0))
    {

    #if(MODULE_VER != 0)
        //new otp，模组，此时sysclk应该由前面的睡眠流程保证是xtal
        HWREGB(0x40000059) = 0x7;   // itune = 7
        HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
        while (!(COREPRCM->ANAXTALRDY & 0x01));     //xtal ready
        AONPRCM->SYSCLK_CTRL = (AONPRCM->SYSCLK_CTRL & ~SYSCLK_SRC_SEL_Msk) | (SYSCLK_SRC_XTAL << SYSCLK_SRC_SEL_Pos);        //sysclk xtal
        while( ((COREPRCM->SYSCLK_FLAG & CORESYS_CLK_FLAG_Msk) >> CORESYS_CLK_FLAG_Pos) != 0x04);           //syclk flag xtal
    #else
        HWREGB(AON_SYSCLK_CTRL2) |= 0x3;   //force xtal off,force pll off，前面的流程已经force off
        HWREGB(COREPRCM_ADIF_BASE + 0x58) &= 0xE3; //disable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
        //仅opencpu，执行快速恢复，且sido1p4 off会执行此分支，此时xtal pll为关闭状态
        PRCM_SetWkupAonDly(2,8,52,2,15); //此处主要为了缩减唤醒后的xtal delay，aonclk utc下xtal ready较慢

        // 1. 先force_xtal_pwron为0，然后xtal_pwron_byp为1。force_xtal_clkout_ena为0，xtal_clkout_ena_byp为1. 
        // 2. force_xtal_rdy和xtal_rdy_byp同时为1，fpll_off为1
        // 3. fxtal_off为0，然后等待一个utc时钟
        // 4. 设置系统时钟为xtal
        COREPRCM->XTALPLL_TEST0 = COREPRCM->XTALPLL_TEST0 & 0xFC;  //force_xtal_pwron为0，xtal_pwron_byp为1
        COREPRCM->XTALPLL_TEST0 |= 0x01;
        COREPRCM->XTALPLL_TEST0 = (COREPRCM->XTALPLL_TEST0 & 0x3F) | 0x40;  //force_xtal_clkout_ena为0，xtal_clkout_ena_byp为1

        COREPRCM->XTALPLL_TEST2 |= 0x0C;    //force_xtal_rdy和xtal_rdy_byp同时为1
        
        if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
        {
            HWREGB(0x40000059) = 0x7;   // itune = 7
            HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
        }
        HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
        utc_cnt_delay(1);   //从xtal on到可以进深睡用的是aonsysclk，需要半个到1个utcclk，如果不delay，切换aonsys 32k后，这个时钟变慢了

        AONPRCM->SYSCLK_CTRL = (AONPRCM->SYSCLK_CTRL & ~SYSCLK_SRC_SEL_Msk) | (SYSCLK_SRC_XTAL << SYSCLK_SRC_SEL_Pos);        //sysclk xtal，但实际没开xtal并不会切换到xtal

        //aonsys select 32k
        AONPRCM->AONCLK_CTRL &= ~0x02;
        while( (AONPRCM->AONCLK_FLAG & 0x10));
    #endif 

        AONPRCM->AONCLK_CTRL |= 0x100;      //hrc pd normal

    #if(MODULE_VER != 0)
        delay_func_init();
    #endif 

    }
    
   
}

/**
  * @brief 睡眠失败/唤醒后将aon切换回HRC
  * @return None
  */
__RAM_FUNC void Switch_VDDAON_HRC(void)
{
    //extern void delay_func_us(float tick_us);

    //目前非模组一定执行快速恢复，wfi前都可以关闭hrc，此处需要打开hrc
    if((AONPRCM->AONGPREG2 & 0x10) || (MODULE_VER == 0))
    {
        //new otp,hrc recovery
        AONPRCM->AONCLK_CTRL &= ~0x100;     //hrc on
        while( !(AONPRCM->AONCLK_FLAG & 0x40));     //hrc ready 

        #if (MODULE_VER == 0x0)	
            //aonsys select hrc，可以加快下方的时钟切换等待
            AONPRCM->AONCLK_CTRL |= 0x02;
            while( !(AONPRCM->AONCLK_FLAG & 0x10));
            //仅opencpu，执行快速恢复，且sido1p4 off会执行此分支，此时xtal为开启状态
            AONPRCM->SYSCLK_CTRL = (AONPRCM->SYSCLK_CTRL & ~SYSCLK_SRC_SEL_Msk) | (SYSCLK_SRC_HRC << SYSCLK_SRC_SEL_Pos);        //sysclk hrc
            while( ((COREPRCM->SYSCLK_FLAG & CORESYS_CLK_FLAG_Msk) >> CORESYS_CLK_FLAG_Pos) != 0x02);           //sysclk flag hrc
            

            // 如果没有睡下去：
            // 1. 设置fxtal_off为1,
            // 2. 取消所有的force_xtal_pwron，force_xtal_clkout_ena和xtal_rdy_byp
            // 3. 再设置fxtal_off为0，启动xtal
            //opencpu xtal off，inlcude opencpu using pll
            HWREGB(AON_SYSCLK_CTRL2) |= 0x01;   //xtal off
            COREPRCM->XTALPLL_TEST0 &= 0x3C;
            COREPRCM->XTALPLL_TEST2 &= 0xF3;

            PRCM_SetWkupAonDly(2,8,52,10,15); //此处主要为了调整xtal delay在aonclk hrc下的值
        #endif
    }

    if(AONPRCM->AONGPREG2 & 0xE0)   //唤醒回来查询lpuart时钟选择备份，将lpuart选择重置为hrc
    {
        AONPRCM->LPUA1_CTRL = (AONPRCM->LPUA1_CTRL & ~0x70) | ((AONPRCM->AONGPREG2 & 0xE0)>>1);  
        AONPRCM->AONGPREG2 &= 0x1F;
    }
    //aonsys select hrc
    AONPRCM->AONCLK_CTRL |= 0x02;
    while( !(AONPRCM->AONCLK_FLAG & 0x10));
}


/*受watchdog_enable控制，用于开关UTC和AP看门狗*/
uint8_t g_wdt_enable = 0;

/*受off_debug和watchdog_enable共同控制，当为0时死机保持现场*/
uint8_t g_off_debug = 1;
uint8_t g_dump_mem_into_flash=0;

/*由于读flash耗时过久，统一在此初始化阶段读取，全局方式访问*/
__OPENCPU_FUNC void fac_nv_param_read()
{
	g_off_wakeupRXD = READ_FAC_NV(uint8_t,off_wakeupRXD);
	g_dump_mem_into_flash = READ_FAC_NV(uint8_t, dump_mem_into_flash);
#if (XY_DUMP)
	g_wdt_enable = 0;
	g_off_debug = 0;
    // 由于二级BOOT初始化了UTC看门狗，所以这里要去初始化
    UTC_WDT_Deinit();
#else
	g_wdt_enable = READ_FAC_NV(uint8_t,watchdog_enable);
	g_off_debug = READ_FAC_NV(uint8_t, off_debug);

	/*看门狗关闭时，断言也不重启，以保持现场*/
	if(g_wdt_enable == 0)
	{
		g_off_debug = 0;
		UTC_WDT_Deinit();
	}
	else
	{
		UTC_WDT_Init(2*60);
	}
#endif
	/*根据off_debug的NV来设置CP核的debug模式及断言保持能力*/
	if(READ_FAC_NV(uint8_t, off_debug) == 0)
		HWREGB(BAK_MEM_XY_DUMP) = 1;
	else
		HWREGB(BAK_MEM_XY_DUMP) = 0;
}

extern void at_uart_wakup_init();
extern void BakupMemInit(void);
extern void Fota_Reboot_Init(void);
/*除了快速恢复的深睡唤醒场景外，其他场景皆会调用*/
__OPENCPU_FUNC void SystemInit(void)
{
#if DYN_LOAD
	set_got_r9();
#endif
	//将delay初始化函数放在SystemInit开始，保证后续可以使用delay函数
	delay_func_init();
	HWREGB(0x40004073) |= 0x1;		//打开BBPLL_locklost计数功能

	// 默认flash上电
	xy_Flash_Init();

	HWREGB(0x400080a4)=0xff;
	HWREGB(0x400080a5)=0x1f;

	fac_nv_param_read();
	BakupMemInit();

#if (MODULE_VER != 0)
    //低电压时候，深睡前切换vddaon可能发生复位，会选择sido打开vddaon sido供电
    //需要在BakupMemInit()调用后再清除配置
    AONPRCM->AONPWR_CTRL &= ~0xC800;//clear:sido is on in deepsleep;sel sido lpmode as vddaon source
#endif   

	/*开启FLASH或CP RAM的cache功能，以提高FLASH运行速度*/
#if (CACHE_ENABLE_SET == 1)
#if (DYN_LOAD == 1)
		/*动态启CP核时，再进行FLASH的cache使能*/
		CacheInit_Ext(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, SHARE_RAM0_BASE, SHARE_RAM1_BASE-1);
#else
		CacheInit_Ext(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, ARM_FLASH_BASE_ADDR, HWREG(BAK_MEM_FOTA_FLASH_BASE) - 1);
#endif	
#endif

	Sdk_Hardware_Init();
	xy_print_uart_Init(XY_LOGVIEW_AP_RATE);
	
	extern void save_up_dbg_into(int reason,int subreason);
	save_up_dbg_into(Get_Boot_Reason(),Get_Boot_Sub_Reason());


#if AT_LPUART==1
	at_uart_wakup_init();
	at_uart_init();
#endif

	/*寄存器内容对应AONPRCM_TypeDef中的前8字节内容*/
	xy_printf("+DBGINFO:AP BootR %d,SubR %d;CP BootR %d,SubR %d,AON REG:%x %x\n",Get_Boot_Reason(),  \
		Get_Boot_Sub_Reason(),HWREGB(BAK_MEM_CP_UP_REASON),HWREG(BAK_MEM_CP_UP_SUBREASON),HWREG(0x40000000), HWREG(0x40000008));

	EnablePrimask();

#if (MODULE_VER==0)
	Fota_Reboot_Init();
#endif

}

/*在 Reset_Handler 的第一行被调，内部判断是否是快速启动，执行快速恢复*/
void first_excute_in_reset_handler(void)
{
	Debug_Runtime_Init();
	Debug_MODULE_Runtime_Init();
	Debug_Runtime_Add("first_excute_in_reset_handler start");

	//短时间深睡唤醒，为保证电容不进行反复充放电，需要在睡眠时开启sido，这时才需要关闭retldo，当前版本为加快启动速度先注释掉
	//PRCM_PowerOffRetldo();
//表计和非表计工程开启的外设是不同的
#if (LLPM_VER == 1)
	//打开外设gpio、lpua1、lptmr、aonprcm、tick、utc时钟
	//Core_PRCM_Clock_Enable0(0x8005C020);
	//关闭外设sha、dmac、uart2、spi时钟
	//Core_PRCM_Clock_Disable0(0x40002006);
	//打开外设gpio、lpua1、lptmr、aonprcm、tick、utc、qspi时钟
	COREPRCM->CKG_CTRL_L = CORE_CKG_CTL_GPIO_EN | CORE_CKG_CTL_LPUART_EN | CORE_CKG_CTL_LPTMR_EN | \
		CORE_CKG_CTL_AONPRCM_EN | CORE_CKG_CTL_TICK_EN | CORE_CKG_CTL_UTC_EN | CORE_CKG_CTL_QSPI_EN;

	COREPRCM->CKG_CTRL_H &= ~0x01;//disable force_sm_clkon,for power saving
	//COREPRCM->CORE_GPR1 |= 0x80;//关闭外设qspi时钟
#else
	//打开外设tick、sha、bb、dfe、gpio、phytmr、lpua1、aonprcm、sema、aes、utc、qspi时钟
	PRCM_ClockEnable(0xF006D828);
	//关闭外设spi时钟
	PRCM_ClockDisable(0x2);
	COREPRCM->CKG_CTRL_H &= ~0x01;//disable force_sm_clkon,for power saving
#endif

	Debug_Runtime_Add("COREPRCM->CKG_CTRL config stop configUTC start ");
	COREPRCM->WAKUP_STAT = 0xC000;//clear ap_nrst_flag & cp_nrst_flag,for wdt reset's use

	//关波验证用，需要时再放开，当前可以去除以加快唤醒时长
	//while((COREPRCM->CKG_CTRL_L&0x20)!=0x20);

    //压缩耗时，只读一次aonprcm寄存器
    wakeup_info = AONPRCM->WAKEUP_STATUS;
	// 深睡唤醒后，第一时间配置utc，避免utc读数不准
	//if((wakeup_info & 0x10) || (AONPRCM->UTC_PCLKEN_CTRL == 0))
	{
		AONPRCM->UTC_PCLKEN_CTRL = 1;
		//while(AONPRCM->UTC_PCLKEN_CTRL == 0);	
	}

	Debug_MODULE_Runtime_Add("g_db_ap_reset_handler");
#if (DEBUG_MODULE_RUN_TIME == 1)
	//bug8686调试代码，用于深睡提前量的debug，后续稳定会删除
	HWREG(BAK_MEM_RUN_TIME_STATISTICS) = *(uint32_t *)0x40001008; //UTC_TIMER
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x04) = UTC_ClkCntGet(0); //UTC_CNT
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x08) = *(uint32_t *)0x40001010; //UTC_ALARM_TIMER
	HWREG(BAK_MEM_RUN_TIME_STATISTICS+0x0c) = UTC_ClkCntConvert(UTC_AlarmCntGet()); //UTC_ALARM_CLK_CNT
#endif
  
#if (MPU_EN == 1)
	Mpu_Protect_Init(); //耗时200us
#endif

	Debug_MODULE_Runtime_Add("g_db_Mpu_Protect");

	Debug_Runtime_Add("configUTC stop restore_msp start ");
	//执行快速恢复，从WFI下一句继续执行
	if ((wakeup_info & 0x10) && (g_fast_startup_flag == AP_WAKEUP_FASTRECOVERY_BEFORE) && ((HWREGB(AON_AONGPREG0) & 0x43) == 0x43))
	{
		g_fast_startup_flag = AP_WAKEUP_FASTRECOVERY_AFTER;
		
#if DYN_LOAD
		set_got_r9();
#endif
		restore_msp();
		Debug_MODULE_Runtime_Add("g_db_restore_msp");

		Debug_Runtime_Add("restore_msp stop FastSystemInit start");

		FastSystemInit();
		Debug_MODULE_Runtime_Add("g_db_FastSystemInit");
		
		Debug_MODULE_Runtime_Add("g_db_FastRecovery");

		Debug_Runtime_Add("FastSystemInit stop restore_scene_and_running start");
	
		// 必须在当前函数的最后调用，执行下面的函数时，当前函数不会返回，会直接跳转到深睡前的地方执行
		restore_scene_and_running();
	}

}


#if defined(__CC_ARM)
/******************************************************************************
 * @brief  This function dump current register to memory, use trace to analyze
 * @param  None
 * @retval None
 *****************************************************************************/
__asm void DumpRegister_from_Normal(void)
{
    IMPORT m3_reg
    /* push R4, use R4 to load variable address */
    PUSH{R4}
    /* load variable address to R4, save R0-R3 */
    LDR R4, = m3_reg
    STMIA R4 !, {R0 - R3}
    /* move variable address to R0, pop R4 to restore sp, then save R4-R12 */
    MOV R0, R4
    POP{R4} 
    STMIA R0 !, {R4 - R12}
    /* save SP, LR, PC */
    MOV R1, SP
    MOV R2, LR
    MOV R3, PC
    STMIA R0 !, {R1 - R3}
    /* save xPSR, MSP, PSP, PRIMASK, BASEPRI, FAULTMASK, CONTROL */
    MRS R1, xPSR
    MRS R2, MSP
    MRS R3, PSP
    MRS R4, PRIMASK
    MRS R5, BASEPRI
    MRS R6, FAULTMASK
    MRS R7, CONTROL
    STMIA R0 !, {R1 - R7}
    /* use R12 to load variable address */
    LDR R12, = m3_reg
    /* recovery R0-R11, and store R12 to recovery R12 */
    LDMIA R12 !, {R0 - R11} 
    LDR R12, [R12] 
    BX LR 
    ALIGN
}

/******************************************************************************
 * @brief  This function handles Hard Fault exception.
 * @param  stack_sp: the SP before trigger HardFault, MSP or PSP
 * @retval None
 *****************************************************************************/
__asm void DumpRegister_from_Fault(uint32_t *stack_sp)
{
    /* save R4-R11 */
    LDR R1, = m3_reg // R1 is the address to save common register
    ADD R1, R1, #16 // R1 is the address of m3_reg.R4
    STMIA R1 !, {R4 - R11} // now R1 is the address of m3_reg.R12
    /* save r0-r3 in stack */
    LDR R1, = m3_reg // reload R1, R1 is the address of m3_reg.R0
    MOV R2, R0 // SP move to R2
    LDMIA R2 !, {R3 - R6} // load R0-R3 from stack
    STMIA R1 !, {R3 - R6} // save R0-R3
    /* save R12, SP, LR, PC, xPSR in stack */
    LDR R1, = m3_reg // reload R1, R1 is the address of m3_reg.R0
    ADD R1, R1, #48 // R1 is the address of m3_reg.R12
    ADD R2, R0, #16 // R2 is the stack address of R12
    LDMIA R2 !, {R3, R5 - R7} // load R12, LR, PC, xPSR from stack
    MOV R4, R0 // SP move to R4
    STMIA R1 !, {R3 - R7} // now R1 is the address of m3_reg.MSP
    /* save MSP, PSP, PRIMASK, BASEPRI, FAULTMASK, CONTROL */
    LDR R1, = m3_reg // reload R1, R1 is the address of m3_reg.R0
    ADD R1, R1, #68 // R1 is the address of m3_reg.MSP
    MRS R2, MSP // move all the special register to common register
    MRS R3, PSP
    MRS R4, PRIMASK
    MRS R5, BASEPRI
    MRS R6, FAULTMASK
    MRS R7, CONTROL
    STMIA R1 !, {R2 - R7} // now R1 is the address of m3_reg.CONTROL + 4
    BX LR
}

__asm void HardFault_Handler(void)
{
    IMPORT Dump_Handler_in_Fault
    PRESERVE8
    TST LR,#4 
    ITE EQ
    MRSEQ R0, MSP
    MRSNE R0, PSP
    B Dump_Handler_in_Fault
    ALIGN
}

#elif defined(__GNUC__)
void __attribute__((naked)) DumpRegister_from_Normal(void)
{
	__asm__ __volatile__(
		/* push r4, use r4 to load variable address */
		"	push   {r4}							\n"
		/* load variable address to r4, save r0-r3 */
		"	ldr    r4, =m3_reg					\n"
		"	stmia  r4!, {r0-r3}					\n"
		/* move variable address to r0, pop r4 to restore sp, then save r4-r12 */
		"	mov    r0, r4						\n"
		"	pop    {r4}							\n"
		"	stmia  r0!, {r4-r12}				\n"
		/* save sp, lr, pc */
		"	mov    r1, sp						\n"
		"	mov    r2, lr						\n"
		"	mov    r3, pc						\n"
		"	stmia  r0!, {r1-r3}					\n"
		/* save xpsr, msp, psp, primask, basepri, faultmask, control */
		"	mrs    r1, xpsr						\n"
		"	mrs    r2, msp						\n"
		"	mrs    r3, psp						\n"
		"	mrs    r4, primask					\n"
		"	mrs    r5, basepri					\n"
		"	mrs    r6, faultmask				\n"
		"	mrs    r7, control					\n"
		"	stmia  r0!, {r1-r7}					\n"
		/* use r12 to load variable address */
		"	ldr    r12, =m3_reg					\n"
		/* recovery r0-r11, and store r12 to recovery r12 */
		"	ldmia  r12!, {r0-r11}				\n"
		"	ldr    r12, [r12]					\n"
		"	bx     lr							\n"
		"	.align 4							\n");
}

 void __attribute__((naked)) HardFault_Handler(void)
{
	__asm__ __volatile__(
		"	tst    lr, #4						\n" // test current sp is msp or psp
		"	ite    eq							\n"
		"	mrseq  r0, msp						\n"
		"	mrsne  r0, psp						\n" // current sp may as a parameter of Register_Dump_from_Interrupt
		"	b      Dump_Handler_in_Fault		\n"
		"	.align 4							\n");
}
#endif

enum{
	cp_dumplogvi_status_idle,     //告知ap核准备导dump状态
	cp_dumplogvi_status_start,    //告知ap核处于导dump状态
	cp_dumplogvi_status_reset,    //告知ap核处于等待ap核进行复位状态
};

enum{
	ap_dumpflash_status_start,	  //告知cp核暂时不允许导dump
	ap_dumpflash_status_idle,     //告知cp核可以准备导dump状态
	ap_dumpflash_status_reqreset, //告知cp核准备复位
};

typedef struct{
	uint8_t ap_dumpflash_status:4;  //默认ap_dumpflash_status_start状态
	uint8_t cp_dumplogvi_status:4;	//默认cp_dumplogvi_status_idle状态
}dump_sync_t;

/*release模式下，供dump到flash和logview导出的时长，超过该时长后硬卡门狗将自动复位*/
#define DUMP_WDT_TIME   30   //sec

/*死机时复位还是保持住现场，由出厂NV参数off_debug决定*/
 void proc_cp_abnormal(void)
{
	volatile dump_sync_t* dump_sync = (dump_sync_t*)BAK_MEM_DUMP_LOGVIEW_FLAG;
	__attribute__((unused)) uint32_t *cp_assert_info = (uint32_t *)HWREG(BAK_MEM_CP_ASSERT_INFO);

	g_dump_core = 1;

	/*防止之前有调用Flash_mpu_Lock禁止FLASH读写，此处开启容许FLASH读写*/
#if (MPU_EN == 1)
	MPU_SubregionCmd(MPU_REGION_NUMBER_5, MPU_SUB_REGION_ALL, DISABLE); 
#endif

#if AT_LPUART
	char dump_str[50] = {0};
	g_cp_assert_file = (char *)cp_assert_info[0];
	g_cp_assert_line = cp_assert_info[1];
	snprintf(dump_str,sizeof(dump_str)-1,"+ASSERT:CP,%s,%lu\r\n",g_cp_assert_file,g_cp_assert_line);
	at_uart_write_fifo(dump_str,strlen(dump_str));
#endif

	/*release模式*/
	if(g_off_debug)
	{
#if MODULE_VER
		__disable_fault_irq(); 
		
		AP_WDT_Init(AP_WDT_WORKMODE_RESET, DUMP_WDT_TIME);

		dump_sync->ap_dumpflash_status = ap_dumpflash_status_start;

		dump_into_to_flash();

		//通知logview导dump
		dump_sync->ap_dumpflash_status = ap_dumpflash_status_idle;
		while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_start);
		//等待cp核向logview输出完dump信息后再复位
		dump_sync->ap_dumpflash_status = ap_dumpflash_status_reqreset;
		while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_reset);

		xy_Soc_Reset();
#else
		//通知logview导dump
		dump_sync->ap_dumpflash_status = ap_dumpflash_status_idle;
		while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_start);
		/*RELEASE模式CP核死机，不得私自重启，交由用户自行容错,也不得操作硬看门狗*/
		g_errno = XY_ERR_CP_DEAD;
#endif
	}
	else
	{
		__disable_fault_irq(); 

		WDT_Disable(AP_WDT);
		UTC_WDT_Deinit();

		dump_sync->ap_dumpflash_status = ap_dumpflash_status_start;

		dump_into_to_flash();

		//通知logview导dump
		dump_sync->ap_dumpflash_status = ap_dumpflash_status_idle;
		while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_start);


		while(1);
	}
}



/*死机时复位还是保持住现场，由出厂NV参数off_debug决定*/
 void proc_ap_abnormal(void)
{
	g_dump_core = 0;
	volatile dump_sync_t* dump_sync = (dump_sync_t*)BAK_MEM_DUMP_LOGVIEW_FLAG;

	__disable_fault_irq(); //关闭所有中断使能，包括hard_fault，防止后续流程造成hardfault

	/*防止之前有调用Flash_mpu_Lock禁止FLASH读写，此处开启容许FLASH读写*/
#if (MPU_EN == 1)
	MPU_SubregionCmd(MPU_REGION_NUMBER_5, MPU_SUB_REGION_ALL, DISABLE); 
#endif

#if AT_LPUART	
	char dump_str[50] = {0};
	/*输出死机信息*/
	snprintf(dump_str,sizeof(dump_str) - 1,"+ASSERT:AP,%s,%lu\r\n",g_assert_file,g_assert_line);
	at_uart_write_fifo(dump_str,strlen(dump_str));
#endif

	if(g_off_debug == 1)
	{
		AP_WDT_Init(AP_WDT_WORKMODE_RESET, DUMP_WDT_TIME);
	}
	else
	{
		WDT_Disable(AP_WDT);
		UTC_WDT_Deinit();
	}

	/* AP死机且CP alive时，通知CP核执行logview的dump输出*/
	if(g_dump_core == 0 && CP_Is_Alive() == true)
	{
		//open场景下cp死机后ap又死机.
		if(dump_sync->cp_dumplogvi_status == cp_dumplogvi_status_start)
		{
			dump_sync->ap_dumpflash_status = ap_dumpflash_status_start;
			while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_idle);
		}
		else
		{
			//通知logview导dump
			dump_sync->ap_dumpflash_status = ap_dumpflash_status_start;
		}

		HWREGB(BAK_MEM_AP_DO_DUMP_FLAG) = 1;
		HWREG(COREPRCM_BASE + 0x20) |= 0x20;//触发IPC中断
	}

	dump_into_to_flash();

	if(CP_Is_Alive() == true)
	{
		//通知logview导dump
		dump_sync->ap_dumpflash_status = ap_dumpflash_status_idle;
		while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_start);
		if(g_off_debug == 1)
		{
			//等待cp核向logview输出完dump信息后再复位
			dump_sync->ap_dumpflash_status = ap_dumpflash_status_reqreset;
			while(dump_sync->cp_dumplogvi_status != cp_dumplogvi_status_reset);
		}
	}

	if(g_off_debug == 1)
	{
		xy_Soc_Reset();
	}
	else
	{
		while(1);
	}
}

 void sys_assert_proc(void)
{
	DisablePrimask();

	DumpRegister_from_Normal();

	proc_ap_abnormal();

	while (1);
}


 void Dump_Handler_in_Fault(uint32_t *stack_sp)
{
	DumpRegister_from_Normal();

	m3_push_reg = (m3_int_push_reg_t *)stack_sp;

	uint32_t reg = SCB->CFSR;

	m3_fault.MFSR = (reg >> 0) & 0xFF;
	m3_fault.BFSR = (reg >> 8) & 0xFF;
	m3_fault.UFSR = (uint16_t)((reg >> 16) & 0xFFFF);
	m3_fault.HFSR = SCB->HFSR;
	m3_fault.DFSR = SCB->DFSR;
	m3_fault.MMAR = SCB->MMFAR;
	m3_fault.BFAR = SCB->BFAR;
	m3_fault.AFAR = SCB->AFSR;

#if AT_LPUART
	g_assert_file = __FILE__;
	g_assert_line = __LINE__;
#endif
	proc_ap_abnormal();
	while (1)
		;
}

__FLASH_FUNC void Disable_All_WDT(void)
{
    AP_WDT_Disable();
	UTC_WDT_Deinit();
}
