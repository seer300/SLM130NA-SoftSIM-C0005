#include "core_cm3.h"
#include "xinyi2100.h"
#include "fast_recovery.h"
#include "hw_types.h"
#include "hw_prcm.h"
#include "nvic.h"
#include "prcm.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "sys_proc.h"
#include "hw_gpio.h"
#include "xy_flash.h"
#include "tick.h"
#include "soc_control.h"
#include "module_runtime_dbg.h"
#include "adc_adapt.h"
#include "adc_adapt.h"
#include "sys_rc32k.h"
#include "cache.h"
#include "xy_printf.h"
#include "adc.h"

uint32_t sysclk_ctrl = 0;
//ioldo1调节检测VAT的门限电压配置
uint8_t coreprcm_ctrl = 0;
volatile uint32_t g_fast_startup_flag = AP_WAKEUP_NORMAL;

m3_fast_recover_reg_t g_m3_fast_recover_reg = {0};
nvic_scene_t g_nvic_scene = {0};
gpio_scene_t gpio_scene = {0};

	
// 在 save_scene_and_wfi 中调用
extern volatile uint32_t g_LUT_clean_period_min;

extern void PRCM_RetmemPwrManage(void);
void save_scene_hook_before_wfi(void)
{
	Debug_Runtime_Add("(In save_scene_hook_before_wfi) NVIC save start");
	// 保存NVIC部分寄存器

	// g_nvic_scene.CTRL = SysTick->CTRL;
	// g_nvic_scene.LOAD = SysTick->LOAD;
	g_nvic_scene.ISER[0] = NVIC->ISER[0];
	g_nvic_scene.ISER[1] = NVIC->ISER[1];

	//外部中断仅仅保存CLKTIM、UTC、LPUART、LPTIM、QSPI、WAKEUP、GPIO的中断优先级
	g_nvic_scene.IP[0] = NVIC->IP[CLKTIM_IRQn];
	g_nvic_scene.IP[1] = NVIC->IP[UTC_IRQn];
	g_nvic_scene.IP[2] = NVIC->IP[LPUART_IRQn];
	g_nvic_scene.IP[3] = NVIC->IP[LPTIM_IRQn];
	g_nvic_scene.IP[4] = NVIC->IP[QSPI_IRQn];
	g_nvic_scene.IP[5] = NVIC->IP[WAKEUP_IRQn];
	g_nvic_scene.IP[6] = NVIC->IP[GPIO_IRQn];
	g_nvic_scene.IP[7] = NVIC->IP[MCNT_IRQn];

	g_nvic_scene.VTOR = SCB->VTOR;
	g_nvic_scene.AIRCR = (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) | 0x05FA;
	g_nvic_scene.CCR = SCB->CCR;

	//内部中断仅仅保存MemoryManagement、BusFault、UsageFault、SVCall、DebugMonitor、PendSV、SysTick的中断优先级
	g_nvic_scene.SHP[0] = SCB->SHP[0];//MemoryManagement
	g_nvic_scene.SHP[1] = SCB->SHP[1];//BusFault	
	g_nvic_scene.SHP[2] = SCB->SHP[2];//UsageFault
	g_nvic_scene.SHP[3] = SCB->SHP[7];//SVCall
	g_nvic_scene.SHP[4] = SCB->SHP[8];//DebugMonitor
	g_nvic_scene.SHP[5] = SCB->SHP[10];//PendSV
	g_nvic_scene.SHP[6] = SCB->SHP[11];//SysTick

	Debug_Runtime_Add("NVIC save stop, sysclk&gpio save start");
	sysclk_ctrl = COREPRCM->SYSCLK_CTRL;

	//保存低32位IO配置
    gpio_scene.DOUT0     = GPIO->DOUT[0];  
	gpio_scene.CTRL0     = GPIO->MODE[0];
	gpio_scene.PULLUP0   = GPIO->PULL_UP[0];
	gpio_scene.PULLDOWN0 = GPIO->PULL_DOWN[0];
	gpio_scene.INPUTEN0  = GPIO->INPUT_EN[0];
	gpio_scene.OUTPUTEN0 = GPIO->OUTPUT_EN[0];

	//保存高32位IO配置
    gpio_scene.DOUT1     = GPIO->DOUT[1];  
	gpio_scene.CTRL1     = GPIO->MODE[1];
	gpio_scene.PULLUP1   = GPIO->PULL_UP[1];
	gpio_scene.PULLDOWN1 = GPIO->PULL_DOWN[1];
	gpio_scene.INPUTEN1  = GPIO->INPUT_EN[1];
	gpio_scene.OUTPUTEN1 = GPIO->OUTPUT_EN[1];
	
	//gpio_scene.CFGSEL0         = GPIO->CTL[0];
	// gpio_scene.PAD_SEL2        = GPIO->PERI[2];
	// gpio_scene.PAD_SEL3        = GPIO->PERI[3];
	// gpio_scene.PERI_IN_SEL2    = GPIO->PERILIN[GPIO_AP_SWCLKTCK];
	// gpio_scene.PERI_IN_SEL3    = GPIO->PERILIN[GPIO_AP_SWDIOTMS];
	//gpio_scene.PERI_IN_SEL_EN1 = GPIO->PERILIN_EN[1];
#if (LLPM_VER != 1)
    //表计不判断是否为3v，节省时间。即便为1.8v执行括号内部代码，也没关系
	if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)
#endif    
	{
		//保存ioldo1调节检测VAT的门限电压配置
		coreprcm_ctrl = HWREGB(COREPRCM_ADIF_BASE + 0x02);
	}
	
	//表计的retmem大小模式和retmem电源配置模式固定。不调用这个接口来节省工作时间。
	//其他的项目，会在进深睡前根据需要打开的retmem大小，选择最合适的pmu配置	
	PRCM_RetmemPwrManage();

	AONPRCM->UTC_PCLKEN_CTRL = 0;
	//while(AONPRCM->UTC_PCLKEN_CTRL != 0);

#if (LLPM_VER != 1)
    PRCM_SramRetentionEnable(SH_SRAM8K|AP_SRAM0|AP_SRAM1);
#else
    //AONPRCM->SMEM_SLPCTRL |=  AP_SRAM0_RET_ENA_Msk | AP_SRAM1_RET_ENA_Msk | SH_SRAM8K_RET_ENA_Msk;
    HWREGB((uint32_t)&AONPRCM->SMEM_SLPCTRL) = AP_SRAM0_RET_ENA_Msk | AP_SRAM1_RET_ENA_Msk | SH_SRAM8K_RET_ENA_Msk;
#endif 	

//对于极低功耗的opencpu，AON LUT不为空即不会产生rc中断，出于功耗、耗时考虑无需执行以下步骤
#if (LLPM_VER != 1)  
	if((HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K) && ( g_LUT_clean_period_min == 0))
	{
		//NVIC_IntUnregister(RC32K_IRQn);  //去注册后，calib_int不再产生NVIC RC32K_pending，睡眠唤醒后校准
		*((volatile uint32_t *)0xe000e284) |= (0x1 << (RC32K_IRQn - 32));   //清除RC32K_pending，规避AP睡眠失败导致的异常唤醒
	}
#endif

	Debug_Runtime_Add("WFI start");

	Debug_Runtime_Add("WFI start2");

    if( HWREG(BAK_MEM_OTP_SOC_ID_BASE) == 0 )  // old
    {
        //配置唤醒时hrc电源选择vprogen
        AONPRCM->AONCLK_CTRL &= ~0x200;
    }


    __WFI();		
}

void restore_scene_hook_after_wfi(void)
{
	// 恢复NVIC部分寄存器
	Debug_Runtime_Add("restore_scene_hook_after_wfi start");
	// SysTick->LOAD = g_nvic_scene.LOAD;
	// SysTick->CTRL = g_nvic_scene.CTRL;
	// SysTick->VAL = 0;
	NVIC->ISER[0] = g_nvic_scene.ISER[0];
	NVIC->ISER[1] = g_nvic_scene.ISER[1];

	//外部中断仅仅恢复CLKTIM、UTC、LPUART、LPTIM、QSPI、WAKEUP、GPIO的中断优先级
	NVIC->IP[CLKTIM_IRQn] = g_nvic_scene.IP[0];
	NVIC->IP[UTC_IRQn] 	  = g_nvic_scene.IP[1];
	NVIC->IP[LPUART_IRQn] = g_nvic_scene.IP[2];
	NVIC->IP[LPTIM_IRQn]  = g_nvic_scene.IP[3];
	NVIC->IP[QSPI_IRQn]   = g_nvic_scene.IP[4];
	NVIC->IP[WAKEUP_IRQn] = g_nvic_scene.IP[5];
	NVIC->IP[GPIO_IRQn]   = g_nvic_scene.IP[6];
	NVIC->IP[MCNT_IRQn]   = g_nvic_scene.IP[7];

	SCB->VTOR = g_nvic_scene.VTOR;
	SCB->AIRCR = g_nvic_scene.AIRCR;
	SCB->CCR = g_nvic_scene.CCR;
	
	//内部中断仅仅恢复MemoryManagement、BusFault、UsageFault、SVCall、DebugMonitor、PendSV、SysTick的中断优先级
	SCB->SHP[0]  = g_nvic_scene.SHP[0];//MemoryManagement
	SCB->SHP[1]  = g_nvic_scene.SHP[1];//BusFault
	SCB->SHP[2]  = g_nvic_scene.SHP[2];//UsageFault
	SCB->SHP[7]  = g_nvic_scene.SHP[3];//SVCall
	SCB->SHP[8]  = g_nvic_scene.SHP[4];//DebugMonitor
	SCB->SHP[10] = g_nvic_scene.SHP[5];//PendSV
	SCB->SHP[11] = g_nvic_scene.SHP[6];//SysTick

	Debug_Runtime_Add("restore_scene_hook_after_wfi stop");
}

void restore_gpio_scene_after_wfi(void)
{
	// if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)
	// {
	// 	//恢复ioldo1调节检测VAT的门限电压配置
	// 	HWREGB(COREPRCM_ADIF_BASE + 0x02) = coreprcm_ctrl;
	// }
	//GPIO->PCFT[0]                  |= 0xFFFFFFFF;
	// GPIO->PERI[2] 					= gpio_scene.PAD_SEL2;
	// GPIO->PERI[3] 					= gpio_scene.PAD_SEL3;
	// GPIO->PERILIN[GPIO_AP_SWCLKTCK] = gpio_scene.PERI_IN_SEL2;
	// GPIO->PERILIN[GPIO_AP_SWDIOTMS] = gpio_scene.PERI_IN_SEL3;
	//GPIO->PERILIN_EN[1] 			= gpio_scene.PERI_IN_SEL_EN1;
	//GPIO->CTL[0] 					= gpio_scene.CFGSEL0;

	//恢复低32位IO配置
    GPIO->DOUT[0]       = gpio_scene.DOUT0 ; 
	GPIO->MODE[0] 		= gpio_scene.CTRL0;
	GPIO->INPUT_EN[0]	= gpio_scene.INPUTEN0;
	GPIO->OUTPUT_EN[0]	= gpio_scene.OUTPUTEN0;
	GPIO->PULL_UP[0] 	= gpio_scene.PULLUP0;
	GPIO->PULL_DOWN[0] 	= gpio_scene.PULLDOWN0;

	//恢复高32位IO配置
    GPIO->DOUT[1]       = gpio_scene.DOUT1 ; 
	GPIO->MODE[1] 		= gpio_scene.CTRL1;
	GPIO->INPUT_EN[1]	= gpio_scene.INPUTEN1;
	GPIO->OUTPUT_EN[1]	= gpio_scene.OUTPUTEN1;
	GPIO->PULL_UP[1] 	= gpio_scene.PULLUP1;
	GPIO->PULL_DOWN[1] 	= gpio_scene.PULLDOWN1;
}

extern uint32_t Wakeup_Deepsleep_Upstate_Get(void);
extern void Time_Non_Lp_Init(void);
extern volatile int g_Flash_Have_Init;

/**
 * @brief 快速回复用户初始化回调函数注册接口
 */
pFunType_void p_User_Init_FastRecovery = NULL;
void FastRecov_Init_Hook_Regist(pFunType_void p_fun)
{
	p_User_Init_FastRecovery = p_fun;
	mark_dyn_addr(&p_User_Init_FastRecovery);
}

extern void at_uart_wakup_init();
/*由于快速恢复RAM不下电，进而需要对有些全局变量进行重新初始，且不建议使用static变量*/
void Reset_info_By_Fast(void)
{	
#if AT_LPUART==1
	at_uart_wakup_init();
#endif

	g_gpio13_2V = 0;  //清除GPIO13参考电压设置
	
    xy_print_uart_Init(XY_LOGVIEW_AP_RATE);

	xy_printf("GetAPClockFreq=%d GetlsioFreq=%d\r\n",GetAPClockFreq(),GetlsioFreq());

    // 等待lsio时钟稳定
    while (PRCM_LSioclkSrcGet() != LS_IOCLK_SRC_HRC){}

	/*由于初始化外设需要一定耗时，建议客户尽量在流程中确定需要使用某外设时才进行初始化，以节省功耗。*/
    if(p_User_Init_FastRecovery != NULL)
    {
        p_User_Init_FastRecovery();
    }
}

extern void xy_Flash_FastRecovery_Init(void);
/*目前仅有表计类OPENCPU才会执行快速恢复*/
void FastSystemInit(void)
{

    if( HWREG(BAK_MEM_OTP_SOC_ID_BASE) == 0 )  // old
    {
        // 解决open版本低温唤醒死机问题，由于配置之间需要延时，
        // 但是为了不影响OPEN版本快速唤醒的时间将整个配置分成3个步骤
        // 步骤1
        //由于sidolp唤醒之后就是1.05v，所以不用设置sidolp电压
        //设置force_hrc ldovpro pu和force_hrc ldodcdc_pu为1,force hrc ldo sel为0
        AONPRCM->PWRCTL_TEST6 = (AONPRCM->PWRCTL_TEST6 & 0xE0) | 0x06;
        //设置hrc bypass_mode为1
        AONPRCM->PWRCTL_TEST6 |= 0x01;
    }

	/*此处未更新子状态，是因为OPENCPU深睡唤醒源可能多种并行触发，无法识别准确*/
	HWREGB(BAK_MEM_AP_UP_REASON) = WAKEUP_DSLEEP;
	
	//flash在睡眠时断电，要恢复成未初始化状态
	g_Flash_Have_Init = 0;

	Debug_Runtime_Add("(In FastSystemInit)xy_Flash_FastRecovery_Init start");
	
	xy_Flash_FastRecovery_Init(); 

	HWREGB(0x40004073) |= 0x1;		//打开BBPLL_locklost计数功能

    if( HWREG(BAK_MEM_OTP_SOC_ID_BASE) == 0 )  // old
    {
        // 解决open版本低温唤醒死机问题，由于配置之间需要延时，
        // 但是为了不影响OPEN版本快速唤醒的时间将整个配置分成3个步骤
        // 步骤2，步骤2与步骤1之间至少需要20us延时
        //设置force_hrc_ldo_sel为1
        AONPRCM->PWRCTL_TEST6 |= 0x08;
        //设置hrc_auto_pwrsel为1
        AONPRCM->AONCLK_CTRL |= 0x200;
    }


	Debug_Runtime_Add("xy_Flash_FastRecovery_Init stop Time_Non_Lp_Init start");
	Time_Non_Lp_Init();
	Debug_MODULE_Runtime_Add("g_db_Lp_Init");

	Debug_Runtime_Add("Time_Non_Lp_Init stop Fast_SystemClockSrc_Select start");
	Fast_SystemClockSrc_Select();
	Debug_MODULE_Runtime_Add("g_db_SystemClockSrc_Select");

#if (LLPM_VER != 1)
	PRCM_PowerOnRetldo(); //retldo 需要稳定时间，因此必须在初始化阶段开启（表计必然使用sido，因此无需开启retldo）
    //AONPRCM->CORE_PWR_CTRL1 |= 0x04;//sido_vnormal_adjena
	//AONPRCM->CORE_PWR_CTRL0 = 0x64;//sidolp 1.2v,standby/deep 1.0v。B0不用再设置
    //AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
#endif

	Debug_Runtime_Add("Fast_SystemClockSrc_Select stop PRCM_IOLDO1_Disable_PSM_Byp start");

#if (LLPM_VER != 1)
    //表计不判断是否为3v，节省时间。即便为1.8v执行括号内部代码，也没关系
	if(PRCM_IOLDO1_IsActiveFlag_3V() != 0)
#endif    
	{
		//在restore_gpio_scene_after_wfi恢复调节监测VBAT的门限电压
		//睡之前aon_psmioldo1_byp_en可能为1，Deepsleep唤醒后强制清0；
		//PRCM_IOLDO1_Disable_PSM_Byp();
		HWREGB(COREPRCM_ADIF_BASE + 0x02) = coreprcm_ctrl;
	}

	Debug_Runtime_Add("PRCM_IOLDO1_Disable_PSM_Byp stop soc_delay_control start");
	soc_delay_control();
	Debug_Runtime_Add("soc_delay_control stop PRCM_ApIntWakeupEnable start");

	PRCM_ApIntWakeupEnable();	 // AP wakeup enable
	Debug_MODULE_Runtime_Add("g_db_ApIntWakeupEnable");

	Debug_Runtime_Add("PRCM_ApIntWakeupEnable stop GPIO recover start");
	//恢复GPIO前调用AGPIO_GPIO0_7_Update函数
	restore_gpio_scene_after_wfi();
	Debug_MODULE_Runtime_Add("g_db_gpio_scene_after_wfi");
	
    AGPIO_GPIO0_7_Update();
	Debug_MODULE_Runtime_Add("g_db_GPIO0_7_Update");

	Debug_Runtime_Add("GPIO recover stop CacheInit start");

	/*开启FLASH或CP RAM的cache功能，以提高FLASH运行速度*/
#if (CACHE_ENABLE_SET == 1)
#if (DYN_LOAD == 1)
		/*动态启CP核时，再进行FLASH的cache使能*/
		CacheInit_Ext(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, SHARE_RAM0_BASE, SHARE_RAM1_BASE-1);
#else
		CacheInit_Ext(AP_CACHE_BASE, CACHE_CCR0_WRITE_THROUGH, ARM_FLASH_BASE_ADDR, HWREG(BAK_MEM_FOTA_FLASH_BASE) - 1);
#endif	
#endif

    if( HWREG(BAK_MEM_OTP_SOC_ID_BASE) == 0 )  // old
    {
        // 解决open版本低温唤醒死机问题，由于配置之间需要延时，
        // 但是为了不影响OPEN版本快速唤醒的时间将整个配置分成3个步骤
        // 步骤3，步骤3与步骤2之间至少需要10us延时
        //设置hrc bypass_mode为0
        AONPRCM->PWRCTL_TEST6 &= ~0x01;
        //force_hrc ldovpro pu和force_hrc ldodcdc_pu为0
        AONPRCM->PWRCTL_TEST6 &= ~0x06;
    }

	Debug_Runtime_Add("CacheInit stop ADC_recover_init start");
	ADC_recover_init();
	Debug_Runtime_Add("ADC_recover_init stop Reset_info_By_Fast start");
	/*由于快速恢复RAM不下电，进而需要对有些全局变量或部分外设进行重新初始*/
	Reset_info_By_Fast();
}

