#include "prcm.h"
#include "rf_drv.h"
#include "dma.h"
#include "gpio.h"
#include "utc.h"
#include "hal_def.h"
#include "sema.h"
/**
  * @brief 关掉vddio_out1
  * @return None
  */
void VDDIO1_Poweroff(void)
{
    AONPRCM->PWRMUX1_TRIG = 0;
    AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x07) | 0x04;//pmux1 off
    AONPRCM->PWRMUX1_TRIG = 1;
}

/**
  * @brief 打开vddio_out1
  * @return None
  */
void VDDIO1_Poweron(void)
{
    AONPRCM->PWRMUX1_TRIG = 0;
    AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x07) | 0x02;//pmux1 select ioldo1
    AONPRCM->PWRMUX1_TRIG = 1;
}

void prcm_delay(unsigned long uldelay)
{
    volatile unsigned long i;
    
    for(i = 0; i < uldelay; i++)
    {
    }
}
//=====================================================================
//======================== BBPLL Functions ============================
//=====================================================================
void BBPLL_Cal_On(void)
{
	COREPRCM->BBPLL_CALCTL |= (1 << 1);
}

void BBPLL_Cal_Off(void)
{
	COREPRCM->BBPLL_CALCTL &= ~(1 << 1);
}

void BBPLL_DivN_Set(uint32_t ulDivN)
{
	/*bbpll divN*/
	HWREGH(COREPRCM_ADIF_BASE + 0x30) = ulDivN; // 28
}

unsigned int BPLL_DivN_Get(void)
{
	return (HWREGH(COREPRCM_ADIF_BASE + 0x30) & 0x3FF);
}

void BBPLL_FracN_Set(uint32_t ulFracN)
{
	HWREG(COREPRCM_ADIF_BASE + 0x34) = ulFracN;
}

unsigned int BBPLL_FracN_Get(void)
{
	return HWREG(COREPRCM_ADIF_BASE + 0x34);
}

inline uint8_t BBPLL_Lock_Status_Get(void)
{
	// return ((HWREGB(COREPRCM_BASE + 0x0D) >> 2) & 0x1);
	return ((COREPRCM->ANAXTALRDY >> 4) & 0x1);
}
uint32_t get_pll_divn(void)
{
	volatile uint32_t pll_divn_cur;
	pll_divn_cur = (((uint32_t)HWREGH(COREPRCM_ADIF_BASE + 0x30))*0xffffff) + (HWREG(COREPRCM_ADIF_BASE + 0x34) & 0xffffff);
	return pll_divn_cur;
}

void BBPLL_Freq_Set(uint32_t ulFreqHz)
{
	uint32_t ulDivN = 0;
	uint32_t ulFracN = 0;
	uint32_t tmpFracN = 0;
	uint32_t xtalCLK = 26000000;
	uint32_t bbpllCLK = ulFreqHz;

	BBPLL_Cal_Off();

	ulDivN = (bbpllCLK * 2) / xtalCLK;
	tmpFracN = (bbpllCLK * 2) % xtalCLK;
	ulFracN = (uint32_t)((tmpFracN / (xtalCLK * 1.0)) * 0xFFFFFF);

	BBPLL_DivN_Set(ulDivN);
	BBPLL_FracN_Set(ulFracN);

	HWREGB(COREPRCM_ADIF_BASE + 0x29) |= (1 << 1); //integer mode for PFD
	HWREGB(COREPRCM_ADIF_BASE + 0x2B) |= (1 << 6); //reduce bbpll vco bias filter R value

	/*bbpll nfvld */
	HWREGB(COREPRCM_ADIF_BASE + 0x32) = 0;
	HWREGB(COREPRCM_ADIF_BASE + 0x32) = 1;//bbpll_nfVld

	BBPLL_Cal_On();
}
void START_PLL(void)
{
	if (BBPLL_Lock_Status_Get())
		return;

    if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
    {
        HWREGB(0x40000059) = 0x7;   // itune = 7
        HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
    }
    HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
	while (!(COREPRCM->ANAXTALRDY & 0x01));    //xtal ready

    HWREGB(0x40000059) = 0x4;   // itune = 4    
	HWREGB(COREPRCM_ADIF_BASE + 0x58) = 0x5c; //enable clk to bbpll,rfpll,enable clk to rfpll and bbpll buf
	HWREGB(AON_SYSCLK_CTRL2) = 0;//disable:xtal,pll power on
    
	BBPLL_Freq_Set(XY_BBPLL_CLK);
}

extern uint8_t g_cldo_set_vol;
void SIDO_NORMAL_INIT(void)
{
    // uint32_t peri1_clkdiv;

    // //bug8724
    // //切换sysclk=hrc，配合pclkdiv=4，如此可以保证lsioclk至少2倍于pclk1，sido_clk_div能够被正确配置
    // PRCM_SysclkSrcSelect(SYSCLK_SRC_HRC);
	// while (SYSCLK_SRC_HRC != PRCM_SysclkSrcGet());

    // peri1_clkdiv = COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_Msk;
    // COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_ResetMsk) | PERI1_PCLK_Div4;
    // while( (COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_Msk) != PERI1_PCLK_Div4);

    if( ((COREPRCM->SYSCLK_FLAG & 0x700) == 0x200) && (AONPRCM->AONCLK_CTRL & (0x07<<24)))
    {
        if( (AONPRCM->AONCLK_CTRL & (0x07<<24)) == (1<<24))
        {
            //lsioclk==hrc，且hrc是2分频,sido_div配置为2.5分频
            COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SIDO_CLK_DIV_ResetMsk) | (1<<SIDO_CLK_DIV_Pos);
        }
        else
        {
            //lsioclk==hrc，且hrc是4分频,sido_div配置为1分频
            COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SIDO_CLK_DIV_ResetMsk) | (0xF<<SIDO_CLK_DIV_Pos);
        }
    }
    else
    {
        //lsioclk==xtal,或者hrc不分频，sido_div配置5
        if(0 == g_cldo_set_vol)
        {//正常版本
            COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SIDO_CLK_DIV_ResetMsk) | (5<<SIDO_CLK_DIV_Pos);
        }
        else
        {//入网入库版本，rf杂散问题
            COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SIDO_CLK_DIV_ResetMsk) | (7<<SIDO_CLK_DIV_Pos);
        }
    }
    //COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_ResetMsk) | peri1_clkdiv;

    //降低sido1p8对1p4的影响
	HWREGB(0x40004817) = ((HWREGB(0x40004817) & (~0x1e)) | 0x06);

    AONPRCM->PWRCTL_TEST1 |= 0x10;//force_cldo_rdy
    AONPRCM->PWRCTL_TEST0 |= 0x04;//FORCE SIDO_1P4_RDY

    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | (0x30|0x0E);//sido1p4 1.2v in normal mode,enable adjust sido,sido_tune_in_normal_ena
    AONPRCM->CORE_PWR_CTRL0 = 0x64;//sidolp 1.2v,standby/deep 1.0v
    utc_cnt_delay(2);
    if(2 == g_cldo_set_vol)
    {
        AONPRCM->CORE_PWR_CTRL2 = (AONPRCM->CORE_PWR_CTRL2 & 0xF0)|0x04;//coreldo 1.0v
    }
    else if(1 == g_cldo_set_vol)
    {
        AONPRCM->CORE_PWR_CTRL2 = (AONPRCM->CORE_PWR_CTRL2 & 0xF0)|0x05;//coreldo 1.1v
    }
    else
    {
        AONPRCM->CORE_PWR_CTRL2 = (AONPRCM->CORE_PWR_CTRL2 & 0xF0)|0x06;//coreldo 1.2v
    }
    //HWREGB(COREPRCM_ADIF_BASE + 0x01) = (HWREGB(COREPRCM_ADIF_BASE + 0x01) & 0xF0) | 0x09;//i_cldo_vadj_fine<3:0>
    //HWREGB(AONPRCM_ADIF_BASE + 0x06) = (HWREGB(AONPRCM_ADIF_BASE + 0x06) & 0x83) | (0x1A<<2);//HBPG 11010: 1.033

    AONPRCM->CORE_PWR_CTRL1 |= 0x01;//sido_normal_ena
    while( !(AONPRCM->AONSYS_FLAG & 0x02));//sido normal ready
    utc_cnt_delay(3);
	HWREGB(0x40004809) &= ~0x80;//i_sido_dummyload_dis,0 to enable

    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | 0x40;//sido1p4 1.3v in normal mode,enable adjust sido
    utc_cnt_delay(3);
    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | 0x50;//sido1p4 1.4v in normal mode,enable adjust sido
    utc_cnt_delay(3);
    AONPRCM->CORE_PWR_CTRL1 &= ~0x02;//sido_tune_in_normal_ena=0
    AONPRCM->PWRCTL_TEST1 &= ~0x10;//force_cldo_rdy=0
    AONPRCM->PWRCTL_TEST0 &= ~0x04;//FORCE SIDO_1P4_RDY=0

    AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
}
void SIDO_NORMAL(void)
{
    while ( (PRCM_LSioclkSrcGet() != LS_IOCLK_SRC_HRC) && (PRCM_LSioclkSrcGet() != LS_IOCLK_SRC_XTAL));

    AONPRCM->PWRCTL_TEST1 |= 0x10;//force_cldo_rdy
    AONPRCM->PWRCTL_TEST0 |= 0x04;//FORCE SIDO_1P4_RDY

    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | (0x30|0x0E);//sido1p4 1.2v in normal mode,enable adjust sido,sido_tune_in_normal_ena
    AONPRCM->CORE_PWR_CTRL0 = 0x64;//sidolp 1.2v,standby/deep 1.0v
    utc_cnt_delay(2);

    AONPRCM->CORE_PWR_CTRL1 |= 0x01;//sido_normal_ena
    while( !(AONPRCM->AONSYS_FLAG & 0x02));//sido normal ready
    utc_cnt_delay(3);

    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | 0x40;//sido1p4 1.3v in normal mode,enable adjust sido
    utc_cnt_delay(3);
    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | 0x50;//sido1p4 1.4v in normal mode,enable adjust sido
    utc_cnt_delay(3);
    AONPRCM->CORE_PWR_CTRL1 &= ~0x02;//sido_tune_in_normal_ena=0
    AONPRCM->PWRCTL_TEST1 &= ~0x10;//force_cldo_rdy=0
    AONPRCM->PWRCTL_TEST0 &= ~0x04;//FORCE SIDO_1P4_RDY=0
    
    AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
}
void SIDO_LPMODE(void)
{
    AONPRCM->PWRCTL_TEST1 |= 0x10;//force_cldo_rdy
    AONPRCM->PWRCTL_TEST0 |= 0x04;//FORCE SIDO_1P4_RDY

    AONPRCM->CORE_PWR_CTRL1 = (AONPRCM->CORE_PWR_CTRL1 & 0x0F) | (0x30|0x0E);//sido1p4 1.2v in normal mode,enable adjust sido,sido_tune_in_normal_ena
    utc_cnt_delay(3);

    //HWREGB(0x40004818) &= 0x10;//sido1p8 disable
	AONPRCM->CORE_PWR_CTRL1 &= ~0x01;//sido_lp
    while( !(AONPRCM->AONSYS_FLAG & 0x01));//sido lp ready
    utc_cnt_delay(3);
	
	AONPRCM->CORE_PWR_CTRL1 &= ~0x04;//sido_vnormal_adjena=0
    AONPRCM->CORE_PWR_CTRL1 &= ~0x02;//sido_tune_in_normal_ena=0
    AONPRCM->PWRCTL_TEST1 &= ~0x10;//force_cldo_rdy=0
    AONPRCM->PWRCTL_TEST0 &= ~0x04;//FORCE SIDO_1P4_RDY=0
}
//=====================================================================
//======================== AON PRCM Functions =========================
//=====================================================================
// 0 means: LOAD_IMAGE
// 1 means: NOT_LOAD_IMAGE
unsigned char AON_CP_Load_Flag_Get(void)
{
    volatile uint8_t core_load_flag = 0;
    
    core_load_flag = HWREGB(AON_AONGPREG0);
    
    if(core_load_flag & AONGPREG0_CORE_LOAD_FLAG_CP)
    {
        return NOT_LOAD_IMAGE;
    }
    
    return LOAD_IMAGE;
}


unsigned char AON_AP_Load_Flag_Get(void)
{
    volatile uint8_t core_load_flag = 0;
    
    core_load_flag = HWREGB(AON_AONGPREG0);
    
    if(core_load_flag & AONGPREG0_CORE_LOAD_FLAG_AP)
    {
        return NOT_LOAD_IMAGE;
    }
    
    return LOAD_IMAGE;
}


unsigned char AON_SECONDARY_Load_Flag_Get(void)
{
    volatile uint8_t core_load_flag = 0;
    
    core_load_flag = HWREGB(AON_AONGPREG0);
    
    if(core_load_flag & AONGPREG0_CORE_LOAD_FLAG_SEC)
    {
        return NOT_LOAD_IMAGE;
    }
    
    return LOAD_IMAGE;
}


void AON_CP_Load_Flag_Set(uint8_t flag)
{
    if(LOAD_IMAGE == flag)
    {
        HWREGB(AON_AONGPREG0) &= (uint8_t)(~AONGPREG0_CORE_LOAD_FLAG_CP);
    }
    else
    {
        HWREGB(AON_AONGPREG0) |= (uint8_t)AONGPREG0_CORE_LOAD_FLAG_CP;
    }
}


void AON_AP_Load_Flag_Set(uint8_t flag)
{
    if(LOAD_IMAGE == flag)
    {
        HWREGB(AON_AONGPREG0) &= (uint8_t)(~AONGPREG0_CORE_LOAD_FLAG_AP);
    }
    else
    {
        HWREGB(AON_AONGPREG0) |= (uint8_t)AONGPREG0_CORE_LOAD_FLAG_AP;
    }
}


void AON_SECONDARY_Load_Flag_Set(uint8_t flag)
{
    if(LOAD_IMAGE == flag)
    {
        HWREGB(AON_AONGPREG0) &= (uint8_t)(~AONGPREG0_CORE_LOAD_FLAG_SEC);
    }
    else
    {
        HWREGB(AON_AONGPREG0) |= (uint8_t)AONGPREG0_CORE_LOAD_FLAG_SEC;
    }
}


void AON_Flash_Delay_Get(void)
{
    volatile uint8_t flash_delay_config = 0;
    
    flash_delay_config = ((HWREGB(AON_AONGPREG0) >> 3) & 0x07);
    
    if(0x00 == flash_delay_config)
    {
        prcm_delay(30000);      // 4.68 ms by FPGA NoBB Version Crystal Clock (XTAL 38.4M)
    }
    else if(0x01 == flash_delay_config)
    {
        prcm_delay(20000);
    }
    else if(0x02 == flash_delay_config)
    {
        prcm_delay(10000);      // 1.56 ms by FPGA NoBB Version Crystal Clock (XTAL 38.4M)
    }
    else if(0x03 == flash_delay_config)
    {
        prcm_delay(5000);
    }
    else if(0x04 == flash_delay_config)
    {
        prcm_delay(2500);
    }
    else if(0x05 == flash_delay_config)
    {
        prcm_delay(1000);
    }
    else if(0x06 == flash_delay_config)
    {
        prcm_delay(200);
    }
    else if(0x07 == flash_delay_config)
    {
        
    }
}


inline void AON_Flash_Delay_Set(uint8_t value)
{
    HWREGB(AON_AONGPREG0) = ((HWREGB(AON_AONGPREG0) & (uint8_t)(~AONGPREG0_FLASH_DELAY)) | ((value & 0x7) << 3));
}

inline unsigned char AON_BOOTMODE_GET(void)
{
    return ((HWREGB(AON_BOOTMODE)) & 0x07);
}

inline void AON_CP_Memory_Remap_Enable(void)
{
    HWREGB(AON_CP_REMAP) |= 0x01;
}

inline void AON_CP_Memory_Remap_Disable(void)
{
    HWREGB(AON_CP_REMAP) &= 0xFE;
}

inline void AON_CP_Core_Release(void)
{
    HWREGB(AON_RST_CTRL) |= 0x80;
}

inline void AON_CP_Core_Hold(void)
{
    if(AONPRCM->RST_CTRL & 0x80)
    {
        HWREGB(AON_RST_CTRL) &= 0x7F;
        while(!(COREPRCM->WAKUP_STAT & 0x8000));//wait nrst flag
        COREPRCM->WAKUP_STAT = 0x8000;//clear cp_nrst_flag,for wdt reset's use
    }
}

inline unsigned long AON_UP_STATUS_Get(void)
{
    return (((HWREGB(AON_WKUP_STAT0) & 0x30) << 3) | (HWREGB(AON_UP_STAT) & 0x7F));
}

inline void AON_System_Clock_Select(unsigned char ucClkSrc)
{
    HWREGB(AON_SYSCLK_CTRL0) = (HWREGB(AON_SYSCLK_CTRL0) & 0xFC) | ucClkSrc;
}


//=====================================================================
//======================== Core PRCM Functions ========================
//=====================================================================
inline unsigned long Core_PRCM_CHIP_VER_GET(void)
{
    return (HWREG(CORE_CHIP_VER));
}

/**
  * @brief  Registers an interrupt handler for CPU Wakeup interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the CPU Wakeup interrupt occurs.
  * @retval None
  */
void CPUWakeupIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_WAKEUP, g_pRAMVectors, pfnHandler);

    IntEnable(INT_WAKEUP);
}

/**
  * @brief  Unregisters an interrupt handler for the CPU Wakeup interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void CPUWakeupIntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_WAKEUP);

    IntUnregister(INT_WAKEUP, g_pRAMVectors);
}

/**
  * @brief  AP wakeup interrupt enable
  * @retval None
  */
inline void ApIntWakeupEnable(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_WAKEUP_AP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x2E) |= 0x02;
}

/**
  * @brief  AP wakeup interrupt disable
  * @retval None
  */
inline void ApIntWakeupDisable(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_WAKEUP_AP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x2E) &= ~0x02;
}

/**
  * @brief  CP wakeup interrupt enable
  * @retval None
  */
inline void CpIntWakeupEnable(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_WAKEUP_CP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x32) |= 0x02;
}

/**
  * @brief  CP wakeup interrupt disable
  * @retval None
  */
inline void CpIntWakeupDisable(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_WAKEUP_CP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x32) &= ~0x02;
}

/**
  * @brief  AP trigger CP wakeup interrupt
  * @retval None
  */
inline void AP_CP_int_wakeup(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_AP_TRIGGER_CP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x2F) |= 0x01;
}

/**
  * @brief  AP trigger CP wakeup interrupt
  * @retval None
  */
inline void AP_CP_int_clear(void)
{
    //HWREG(CORE_WAKEUP) &= ~PRCM_AP_TRIGGER_CP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x2F) &= ~0x01;
}

/**
  * @brief  CP trigger AP wakeup interrupt
  * @retval None
  */
inline void CP_AP_int_wakeup(void)
{
    //HWREG(CORE_WAKEUP) |= PRCM_CP_TRIGGER_AP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x33) |= 0x01;
}

/**
  * @brief  CP trigger AP wakeup interrupt
  * @retval None
  */
inline void CP_AP_int_clear(void)
{
    //HWREG(CORE_WAKEUP) &= ~PRCM_CP_TRIGGER_AP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x33) &= ~0x01;
}




inline unsigned char PRCM_BootmodeGet(void)
{
    return (AONPRCM->BOOTMODE & 0x07);
}

inline void PRCM_CP_MemoryRemapEnable(void)
{
    AONPRCM->CP_REMAP |= 0x01;
}

inline void PRCM_CP_MemoryRemapDisable(void)
{
    AONPRCM->CP_REMAP &= 0xFE;
}

inline void PRCM_CP_CoreRelease(void)
{
    AONPRCM->RST_CTRL |= 0x80;
}

inline void PRCM_CP_CoreHold(void)
{
    AONPRCM->RST_CTRL &= 0x7F;
}


inline void PRCM_SocReset(void)
{
    AONPRCM->RST_CTRL |= 0x08;
}

uint32_t PRCM_UpStatusGet(void)
{
    uint8_t tempreg_upstat;
    uint32_t tempreg_wkupstat;

    tempreg_upstat = AONPRCM->UP_STATUS;
    tempreg_wkupstat = AONPRCM->WAKEUP_STATUS;

    if(tempreg_wkupstat & WAKEUP_STATUS_DSLEEP_WKUP_Msk)
        return UP_STAT_DSLEEP_WKUP;
    else if(tempreg_wkupstat & WAKEUP_STATUS_STANDBY_WKUP_Msk)
        return UP_STAT_STANDBY_WKUP;
    else if(tempreg_upstat & AON_UP_STAT_EXTPIN_Msk)
        return UP_STAT_EXTPINT_RST;
    else if(tempreg_upstat & AON_UP_STAT_LVDRST_Msk)
        return UP_STAT_LVD_RST;
    else if(tempreg_upstat & AON_UP_STAT_AONWDT_Msk)
        return UP_STAT_UTCWDT_RST;
    else if(tempreg_upstat & AON_UP_STAT_SOFTRST_Msk)
        return UP_STAT_SOFT_RST;
    else if(tempreg_upstat & AON_UP_STAT_DFTGLB_Msk)
        return UP_STAT_DFTGLB_RST;
    else if(tempreg_upstat & AON_UP_STAT_SVDRST_Msk)
        return UP_STAT_SVD_RST;
    else if(tempreg_upstat & AON_UP_STAT_POR_Msk)
        return UP_STAT_POR;
    else
        while(1);         

}

/**
 * @brief :select a clock source for sysclk
 * 
 * @param ClkSrc: a value of SYSCLK_SRC_TypeDef 
 */
inline void PRCM_SysclkSrcSelect(uint32_t ClkSrc)
{
    AONPRCM->SYSCLK_CTRL = (AONPRCM->SYSCLK_CTRL & ~SYSCLK_SRC_SEL_Msk) | (ClkSrc << SYSCLK_SRC_SEL_Pos);
}

/**
 * @brief: get the clock source of sysclk
 * 
 * @return: LS_IOCLK_SRC_TypeDef
 */
uint32_t PRCM_SysclkSrcGet(void)
{
    volatile uint8_t clksrc;
    clksrc = (COREPRCM->SYSCLK_FLAG & CORESYS_CLK_FLAG_Msk) >> CORESYS_CLK_FLAG_Pos;
    if(clksrc == 0x08)
        return SYSCLK_SRC_PLL;
    else if(clksrc == 0x04)
        return SYSCLK_SRC_XTAL;
    else if(clksrc == 0x02)
        return SYSCLK_SRC_HRC;
    else if(clksrc == 0x01)
        return SYSCLK_SRC_32K;        

    return  SYSCLK_SRC_32K;
}

/**
 * @brief :select a clock source for lsioclk
 * 
 * @param ClkSrc :a value of LS_IOCLK_SRC_TypeDef 
 */
inline void PRCM_LSioclkSrcSelect(uint32_t ClkSrc)
{
    AONPRCM->SYSCLK_CTRL = (AONPRCM->SYSCLK_CTRL & ~LS_IOCLK_SRC_SEL_Msk) | (ClkSrc << LS_IOCLK_SRC_SEL_Pos);
}

/**
 * @brief get the clock source of lsioclk
 * 
 * @return LS_IOCLK_SRC_TypeDef 
 */
uint32_t PRCM_LSioclkSrcGet(void)
{
    uint8_t clksrc;
    clksrc = (COREPRCM->SYSCLK_FLAG & LS_IOCLK_FLAG_Msk) >> LS_IOCLK_FLAG_Pos;

    if(clksrc == 0x04)
        return LS_IOCLK_SRC_XTAL;
    else if(clksrc == 0x02)
        return LS_IOCLK_SRC_HRC;
	else if(clksrc == 0x01)
		return LS_IOCLK_SRC_32K;      
		
		
	return LS_IOCLK_SRC_32K;  
}
/**
 * @brief :get peri1pclk dividing ratio.
 * 
 */
inline uint32_t Peri1_PclkDivGet(void)
{
	return ((COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_Msk) >> PERI1_PCLK_DIV_Pos) + 1;
}
/**
 * @brief :get peri2pclk dividing ratio.
 * 
 */
inline uint32_t Peri2_PclkDivGet(void)
{
	return ((COREPRCM->SYSCLK_CTRL & PERI2_PCLK_DIV_Msk) >> PERI2_PCLK_DIV_Pos) + 1;
}


/**
 * @brief Enable clock divider when system clock is not pll.
 * 
 */
inline void PRCM_SlowfreqDivEnable(void)
{
    AONPRCM->SYSCLK_CTRL |= SLOWFREQ_DIV_ENA_Msk;
}
/**
 * @brief Disable clock divider when system clock is not pll.
 * 
 */
inline void PRCM_SlowfreqDivDisable(void)
{
    AONPRCM->SYSCLK_CTRL &= ~SLOWFREQ_DIV_ENA_Msk;
}
/**
 * @brief Configures the ap hclk.
 * 
 * @param AphclkDiv :specifies the aphclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg AP_HCLK_Div4
 *     @arg AP_HCLK_Divx
 *     @arg AP_HCLK_Div19 
 */
inline void PRCM_AphclkDivSet(uint32_t AphclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & AP_HCLK_DIV_ResetMsk) | AphclkDiv;
}

/**
 * @brief Configures the cp hclk.
 * 
 * @param CphclkDiv :specifies the cphclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg CP_HCLK_Div3
 *     @arg CP_HCLK_Divx
 *     @arg CP_HCLK_Div16 
 */
inline void PRCM_CphclkDivSet(uint32_t CphclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & CP_HCLK_DIV_ResetMsk) | CphclkDiv;
}

/**
 * @brief Configures the system hclk.
 * 
 * @param SyshclkDiv :specifies the syshclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg SYS_HCLK_Div4
 *     @arg SYS_HCLK_Divx
 *     @arg SYS_HCLK_Div19 
 */
inline void PRCM_SyshclkDivSet(uint32_t SyshclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SYS_HCLK_DIV_ResetMsk) | SyshclkDiv;
}

/**
 * @brief Configures the peripheral1 pclk.
 * 
 * @param PeripclkDiv :specifies the peri1pclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg PERI1_PCLK_Div1
 *     @arg PERI1_PCLK_Div2
 *     @arg PERI1_PCLK_Div3 
 *     @arg PERI1_PCLK_Div4
 */
inline void PRCM_Peri1pclkDivSet(uint32_t PeripclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & PERI1_PCLK_DIV_ResetMsk) | PeripclkDiv;
}
/**
 * @brief Configures the peripheral2 pclk.
 * 
 * @param PeripclkDiv :specifies the peri2pclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg PERI2_PCLK_Div1
 *     @arg PERI2_PCLK_Div2
 *     @arg PERI2_PCLK_Div3 
 *     @arg PERI2_PCLK_Div4
 */
inline void PRCM_Peri2pclkDivSet(uint32_t PeripclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & PERI2_PCLK_DIV_ResetMsk) | PeripclkDiv;
}

/**
 * @brief Configures the sspi mclk.
 * 
 * @param SSPIclkDiv :specifies the sspi mclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg SSPI_CLK_Div3
 *     @arg SSPI_CLK_Divx
 *     @arg SSPI_CLK_Div16
 */
inline void PRCM_SSPIclkDivSet(uint32_t SSPIclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & SSPI_CLK_DIV_ResetMsk) | SSPIclkDiv;
}

/**
 * @brief Configures the qspi refclk.
 * 
 * @param QSPIrefclkDiv :specifies the qspi refclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg QSPI_REFCLK_Div2
 *     @arg QSPI_REFCLK_Div3
 *     @arg QSPI_REFCLK_Div4
 *     @arg QSPI_REFCLK_Div5
 */
inline void PRCM_QSPIrefclkDivSet(uint32_t QSPIrefclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & QSPI_REFCLK_DIV_ResetMsk) | QSPIrefclkDiv;
}

/**
 * @brief Configures the trng pclk.
 * 
 * @param TRNGpclkDiv :specifies the trng pclk dividing ratio.
 *   This parameter can be one of the following values:
 *     @arg TRNG_PCLK_Div1
 *     @arg TRNG_PCLK_Div2
 *     @arg TRNG_PCLK_Div3
 *     @arg TRNG_PCLK_Div4
 */
inline void PRCM_TRNGpclkDivSet(uint32_t TRNGpclkDiv)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & TRNG_PCLK_DIV_ResetMsk) | TRNGpclkDiv;
}

/**
 * @brief Configures the I2C1 refclk.
 * 
 * @param I2C1refclk :specifies the source of I2C1 refclk.
 *   This parameter can be one of the following values:
 *     @arg I2C1_REFCLK_SEL_Peri1pclk
 *     @arg I2C1_REFCLK_SEL_LSioclk
 */
inline void PRCM_I2C1refclkSet(uint32_t I2C1refclk)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & I2C1_REFCLK_SEL_ResetMsk) | I2C1refclk;
}
/**
 * @brief Configures the I2C2 refclk.
 * 
 * @param I2C1refclk :specifies the source of I2C2 refclk.
 *   This parameter can be one of the following values:
 *     @arg I2C2_REFCLK_SEL_Peri2pclk
 *     @arg I2C2_REFCLK_SEL_LSioclk
 */
inline void PRCM_I2C2refclkSet(uint32_t I2C2refclk)
{
    COREPRCM->SYSCLK_CTRL = (COREPRCM->SYSCLK_CTRL & I2C2_REFCLK_SEL_ResetMsk) | I2C2refclk;
}
/**
 * @brief Configures the LPUA1 refclk.
 * 
 * @param clksrcdiv: specifies the source of LPUA1 refclk.
 *  This parameter can be one of the following values:
 *   @arg AON_HRC_DIV1
 *   @arg AON_HRC_DIV2
 *   @arg AON_HRC_DIV4
 *   @arg AON_HRC_DIV8
 *   @arg AON_HRC_DIV16
 *   @arg AON_UTC_CLK
 */
void PRCM_LPUA1_ClkSet(uint8_t clksrcdiv)
{
    AONPRCM->LPUA1_CTRL = (AONPRCM->LPUA1_CTRL & ~0x70) | (clksrcdiv << 4);
}
/**
 * @brief Configures the LPUA1 refclk.
 * 
 * @return 0:LPUART1_CLKSRC_32K, 1:LPUART1_CLKSRC_AON_HRC
 */
inline static uint8_t PRCM_LPUA1_ClkSrc_Get(void)
{
    return (AONPRCM->LPUA1_CLKFLAG);
}


/**
 * @brief  按照波特率选择LPUA时钟源.
 * @param  baudrate 
 */
inline void PRCM_LPUA1_ClkSet_by_BaudRate(uint32_t baudrate)
{
    // 用32k需要深睡force lpuart power on
    //if (baudrate <= 9600)
    //{
    //    PRCM_LPUA1_ClkSet(AON_UTC_CLK); 
    //    while(PRCM_LPUA1_ClkSrc_Get() != LPUART1_CLKSRC_32K);
    //}
    //else
    {
        //RC26MCLKDIV=1时，此处设置LPUART的HRC分频系数为2，则LPUART参考时钟为13M， 以支持波特率：2400~921600
        //RC26MCLKDIV=2时，此处设置LPUART的HRC分频系数为2，则LPUART参考时钟为6.5M， 以支持波特率：1200~921600
        //RC26MCLKDIV=4时，此处设置LPUART的HRC分频系数为2，则LPUART参考时钟为3.25M，以支持波特率：1200~921600
        if (baudrate <= 9600) PRCM_LPUA1_ClkSet(AON_HRC_DIV16);
        else PRCM_LPUA1_ClkSet(AON_HRC_DIV2); 

        while(PRCM_LPUA1_ClkSrc_Get() != LPUART1_CLKSRC_AON_HRC);
    }
}

/**
 * @brief LPUART的引脚选择
 * @param gpiosel 
 *   @arg AON_LPUART_PAD_DISABLE     : LPUART引脚关闭，LPUART无法使用，此时GPIO3/4/5/6可自由使用
 *   @arg AON_LPUART_PAD_RXD_WKUPRST : GPIO3/WKPRST分别为LPUART的TXD/RXD
 *   @arg AON_LPUART_PAD_RXD_GPIO4   : GPIO3/4分别为LPUART的TXD/RXD
 *   @arg AON_LPUART_PAD_HARDFLOWCTL : GPIO3/4/5/6分别为LPUART的TXD/RXD/CTS/RTS
 */
inline void PRCM_LPUA1_PadSel(uint8_t padsel)
{
    AONPRCM->LPUA1_CTRL = (AONPRCM->LPUA1_CTRL  & ~0x03) | (padsel);
}


static void PRCM_ClockEnable_Original(uint64_t ModuleClk)
{
    if(ModuleClk & (uint64_t)0xFFFFFFFF)
        COREPRCM->CKG_CTRL_L |= (uint32_t)ModuleClk;

    if(ModuleClk & (uint64_t)0xFFFFFFFF00000000)
        COREPRCM->CKG_CTRL_H |= (uint32_t)(ModuleClk >> 32);    
}

void PRCM_ClockEnable(uint64_t ModuleClk)
{
    uint32_t primask_bak = __get_PRIMASK();
    uint8_t cp_alive;

    __set_PRIMASK(1);

    cp_alive = AONPRCM->RST_CTRL & 0x80;

    if(cp_alive)
    {
        //流程上确保CP启动前sema时钟已经打开
        do{
			SEMA_RequestNonBlocking(SEMA_SLAVE_CKG, SEMA_SEMA_DMAC_NO_REQ,SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
		}while(SEMA_MASTER_AP != SEMA_MasterGet(SEMA_SLAVE_CKG));
    }

    PRCM_ClockEnable_Original(ModuleClk);

    if(cp_alive)
    {
        SEMA_Release(SEMA_SLAVE_CKG, SEMA_MASK_NULL);
    }

    __set_PRIMASK(primask_bak);

    if(ModuleClk & (uint64_t)0xFFFFFFFF)
    {
        while((COREPRCM->CKG_CTRL_L & (uint32_t)ModuleClk) != (uint32_t)ModuleClk);   
    }

    if(ModuleClk & (uint64_t)0xFFFFFFFF00000000)
    {
        while((COREPRCM->CKG_CTRL_H & (uint32_t)(ModuleClk >> 32)) != (uint32_t)(ModuleClk >> 32));  
    }

}

static void PRCM_ClockDisable_Original(uint64_t ModuleClk)
{
    volatile uint32_t dummy;

    //bug10180
    if(ModuleClk & CORE_CKG_CTL_CSP1_EN)
        dummy = CSP1->RESERVED0[0];
    if(ModuleClk & CORE_CKG_CTL_TRNG_EN)
        dummy = TRNG->RESERVED3;    
    if(ModuleClk & CORE_CKG_CTL_TMR2_EN)
        dummy = TMR2->RESERVED0;  

    UNUSED_ARG(dummy);

    if(ModuleClk & (uint64_t)0xFFFFFFFF)
        COREPRCM->CKG_CTRL_L &= (uint32_t)(~ModuleClk);

    if(ModuleClk & (uint64_t)0xFFFFFFFF00000000)
        COREPRCM->CKG_CTRL_H &= (uint32_t)(~(ModuleClk >> 32));  
}

void PRCM_ClockDisable(uint64_t ModuleClk)
{
    uint32_t primask_bak = __get_PRIMASK();
    uint8_t cp_alive;

    __set_PRIMASK(1);

    cp_alive = AONPRCM->RST_CTRL & 0x80;

    if(cp_alive)
    {
        //流程上确保CP启动前sema时钟已经打开
        do{
			SEMA_RequestNonBlocking(SEMA_SLAVE_CKG, SEMA_SEMA_DMAC_NO_REQ,SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
		}while(SEMA_MASTER_AP != SEMA_MasterGet(SEMA_SLAVE_CKG));
    }

    PRCM_ClockDisable_Original(ModuleClk);

    if(cp_alive)
    {
        SEMA_Release(SEMA_SLAVE_CKG, SEMA_MASK_NULL);
    }

    __set_PRIMASK(primask_bak);
}
inline uint32_t PRCM_ChipVerGet(void)
{
    return COREPRCM->CHIP_VER;
}

/**
  * @brief  AP wakeup interrupt enable
  * @retval None
  */
inline void PRCM_ApIntWakeupEnable(void)
{
    //COREPRCM->WAKUP_ENA |= PRCM_WAKEUP_AP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x2E) |= 0x02;
}

/**
  * @brief  AP wakeup interrupt disable
  * @retval None
  */
inline void PRCM_ApIntWakeupDisable(void)
{
    //COREPRCM->WAKUP_ENA &= ~PRCM_WAKEUP_AP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x2E) &= ~0x02;
}

/**
  * @brief  CP wakeup interrupt enable
  * @retval None
  */
inline void PRCM_CpIntWakeupEnable(void)
{
    //COREPRCM->WAKUP_ENA |= PRCM_WAKEUP_CP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x32) |= 0x02;
}

/**
  * @brief  CP wakeup interrupt disable
  * @retval None
  */
inline void PRCM_CpIntWakeupDisable(void)
{
    //COREPRCM->WAKUP_ENA &= ~PRCM_WAKEUP_CP_INT_EN_Msk;
    HWREGB(COREPRCM_BASE + 0x32) &= ~0x02;
}

/**
  * @brief  AP trigger CP wakeup interrupt
  * @retval None
  */
inline void PRCM_ApCpIntWkupTrigger(void)
{
    //COREPRCM->WAKUP_ENA |= PRCM_AP_TRIGGER_CP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x2F) |= 0x01;
}

/**
  * @brief  get AP trigger CP wakeup interrupt
  * @retval uint32_t
  */
inline uint32_t PRCM_ApCpIntWkupGet(void)
{
    return (HWREGB(COREPRCM_BASE + 0x2F) & 0x01);
}

/**
  * @brief  AP trigger CP wakeup interrupt
  * @retval None
  */
inline void PRCM_ApCpIntWkupClear(void)
{
    //COREPRCM->WAKUP_ENA &= ~PRCM_AP_TRIGGER_CP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x2F) &= ~0x01;
}

/**
  * @brief  CP trigger AP wakeup interrupt
  * @retval None
  */
inline void PRCM_CpApIntWkupTrigger(void)
{
    //COREPRCM->WAKUP_ENA |= PRCM_CP_TRIGGER_AP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x33) |= 0x01;
}

/**
  * @brief  get CP trigger AP wakeup interrupt
  * @retval uint32_t
  */
inline uint32_t PRCM_CpApIntWkupGet(void)
{
    return (HWREGB(COREPRCM_BASE + 0x33) & 0x01);
}

/**
  * @brief  CP trigger AP wakeup interrupt
  * @retval None
  */
inline void PRCM_CpApIntWkupClear(void)
{
    //COREPRCM->WAKUP_ENA &= ~PRCM_CP_TRIGGER_AP_INT_Msk;
    HWREGB(COREPRCM_BASE + 0x33) &= ~0x01;
}	
/**
 * @brief enable aon-wakeup-int source
 * 
 * @param WkupSrc: selects aon-wakeup-source
 *    This parameter can be one of the following values:
 *      @arg AON_WKUP_SOURCE_EXTPINT        :external pin wakeup int
 *      @arg AON_WKUP_SOURCE_UTC            :UTC wakeup int
 *      @arg AON_WKUP_SOURCE_LPUART1
 *      @arg AON_WKUP_SOURCE_SVD
 *      @arg AON_WKUP_SOURCE_LPTMR
 *      @arg AON_WKUP_SOURCE_LPTMR_ASYNC
 *      @arg AON_WKUP_SOURCE_KEYSCAN
 *      @arg AON_WKUP_SOURCE_TMR2
 *      @arg AON_WKUP_SOURCE_APTICK
 *      @arg AON_WKUP_SOURCE_CPTICK
 * 
 */
void PRCM_AonWkupSourceEnable(uint16_t WkupSrc)
{
    if((WkupSrc & AON_WKUP_SOURCE_EXTPINT) && (WkupSrc != AON_WKUP_SOURCE_EXTPINT))
    {
        AONPRCM->RSTWKP_CFG |= 0x01;
        AONPRCM->WAKUP_CTRL |= (WkupSrc & 0x7fff);
    }
    else if(WkupSrc == AON_WKUP_SOURCE_EXTPINT)
    {
        AONPRCM->RSTWKP_CFG |= 0x01;
    }
    else 
    {
        AONPRCM->WAKUP_CTRL |= WkupSrc;
    }

    if(WkupSrc & AON_WKUP_SOURCE_LPUART1)
    {
        AONPRCM->LPUA1_CTRL |= 0x08;//g_at_lpuart async ena
    }
}
/**
 * @brief enable aon-wakeup-int source
 * 
 * @param WkupSrc: selects aon-wakeup-source
 *    This parameter can be one of the following values:
 *      @arg AON_WKUP_SOURCE_EXTPINT        :external pin wakeup int
 *      @arg AON_WKUP_SOURCE_UTC            :UTC wakeup int
 *      @arg AON_WKUP_SOURCE_LPUART1
 *      @arg AON_WKUP_SOURCE_SVD
 *      @arg AON_WKUP_SOURCE_LPTMR
 *      @arg AON_WKUP_SOURCE_LPTMR_ASYNC
 *      @arg AON_WKUP_SOURCE_KEYSCAN
 *      @arg AON_WKUP_SOURCE_TMR2
 *      @arg AON_WKUP_SOURCE_APTICK
 *      @arg AON_WKUP_SOURCE_CPTICK
 */
void PRCM_AonWkupSourceDisable(uint16_t WkupSrc)
{
    if((WkupSrc & AON_WKUP_SOURCE_EXTPINT) && (WkupSrc != AON_WKUP_SOURCE_EXTPINT))
    {
        AONPRCM->RSTWKP_CFG &= ~0x01;
        AONPRCM->WAKUP_CTRL &= ~(WkupSrc & 0x7fff);
    }
    else if(WkupSrc == AON_WKUP_SOURCE_EXTPINT)
    {
        AONPRCM->RSTWKP_CFG &= ~0x01;
    }
    else 
    {
        AONPRCM->WAKUP_CTRL &= ~WkupSrc;
    }

    if(WkupSrc & AON_WKUP_SOURCE_LPUART1)
    {
        AONPRCM->LPUA1_CTRL &= ~0x0C;//g_at_lpuart async ena=0 && g_at_lpuart sync ena=0
    }
}

/**
 * @brief 
 * 
 * @param Sramx: sram bitmap，can be combined by | operator,such as AP_SRAM0_BANK0|SH_SRAM8K
 * @param SramPowerMode: can be SRAM_POWER_MODE_OFF_DSLEEP,SRAM_POWER_MODE_FORCE_ON,SRAM_POWER_MODE_FORCE_OFF
 */
void PRCM_SramPowerModeSet(uint32_t Sramx,uint32_t SramPowerMode)
{
    uint8_t apsram0_bank0_mode,apsram0_bank1_mode,apsram1_bank0_mode,apsram1_bank1_mode;

    if(Sramx & AP_SRAM0_BANK0)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM0_BANK0_SLPCTL_Msk) | (SramPowerMode << AP_SRAM0_BANK0_SLPCTL_Pos);
    }
    if(Sramx & AP_SRAM0_BANK1)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM0_BANK1_SLPCTL_Msk) | (SramPowerMode << AP_SRAM0_BANK1_SLPCTL_Pos);
    }
    if(Sramx & AP_SRAM1_BANK0)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM1_BANK0_SLPCTL_Msk) | (SramPowerMode << AP_SRAM1_BANK0_SLPCTL_Pos);
    }
    if(Sramx & AP_SRAM1_BANK1)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM1_BANK1_SLPCTL_Msk) | (SramPowerMode << AP_SRAM1_BANK1_SLPCTL_Pos);
    }

    if(Sramx & CP_SRAM0)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~CP_SRAM0_SLPCTL_Msk) | (SramPowerMode << CP_SRAM0_SLPCTL_Pos);
    }
    if(Sramx & CP_SRAM1)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~CP_SRAM1_SLPCTL_Msk) | (SramPowerMode << CP_SRAM1_SLPCTL_Pos);
    }

    if(Sramx & SH_SRAM64K)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~SH_SRAM64K_SLPCTL_Msk) | (SramPowerMode << SH_SRAM64K_SLPCTL_Pos);
    }
    if(Sramx & SH_SRAM8K)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~SH_SRAM8K_SLPCTL_Msk) | (SramPowerMode << SH_SRAM8K_SLPCTL_Pos);
    }

    
    apsram0_bank0_mode = (AONPRCM->SMEM_SLPCTRL & AP_SRAM0_BANK0_SLPCTL_Msk) >> AP_SRAM0_BANK0_SLPCTL_Pos;
    apsram0_bank1_mode = (AONPRCM->SMEM_SLPCTRL & AP_SRAM0_BANK1_SLPCTL_Msk) >> AP_SRAM0_BANK1_SLPCTL_Pos;
    apsram1_bank0_mode = (AONPRCM->SMEM_SLPCTRL & AP_SRAM1_BANK0_SLPCTL_Msk) >> AP_SRAM1_BANK0_SLPCTL_Pos;
    apsram1_bank1_mode = (AONPRCM->SMEM_SLPCTRL & AP_SRAM1_BANK1_SLPCTL_Msk) >> AP_SRAM1_BANK1_SLPCTL_Pos;
    /* special deal for APsram0*/
    if(apsram0_bank0_mode == SRAM_POWER_MODE_FORCE_ON || apsram0_bank1_mode == SRAM_POWER_MODE_FORCE_ON)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM0_SLPCTL_Msk) | (SRAM_POWER_MODE_FORCE_ON << AP_SRAM0_SLPCTL_Pos);
    }
    else if(apsram0_bank0_mode == SRAM_POWER_MODE_FORCE_OFF && apsram0_bank1_mode == SRAM_POWER_MODE_FORCE_OFF)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM0_SLPCTL_Msk) | (SRAM_POWER_MODE_FORCE_OFF << AP_SRAM0_SLPCTL_Pos);
    }
    else
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM0_SLPCTL_Msk) | (SRAM_POWER_MODE_OFF_DSLEEP << AP_SRAM0_SLPCTL_Pos);    
    }
    /* special deal for APsram1*/
    if(apsram1_bank0_mode == SRAM_POWER_MODE_FORCE_ON || apsram1_bank1_mode == SRAM_POWER_MODE_FORCE_ON)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM1_SLPCTL_Msk) | (SRAM_POWER_MODE_FORCE_ON << AP_SRAM1_SLPCTL_Pos);
    }
    else if(apsram1_bank0_mode == SRAM_POWER_MODE_FORCE_OFF && apsram1_bank1_mode == SRAM_POWER_MODE_FORCE_OFF)
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM1_SLPCTL_Msk) | (SRAM_POWER_MODE_FORCE_OFF << AP_SRAM1_SLPCTL_Pos);
    }
    else
    {
        AONPRCM->SMEM_SLPCTRL = (AONPRCM->SMEM_SLPCTRL & ~AP_SRAM1_SLPCTL_Msk) | (SRAM_POWER_MODE_OFF_DSLEEP << AP_SRAM1_SLPCTL_Pos);    
    }
}


/**
 * @brief enable the specific sram enter into retention mode
 * 
 * @param : sram bitmap，can be combined by | operator,such as AP_SRAM0|AP_SRAM1|SH_SRAM8K 
 */
void PRCM_SramRetentionEnable(uint32_t Sramx)
{
    if(Sramx & AP_SRAM0)
    {
        AONPRCM->SMEM_SLPCTRL |=  AP_SRAM0_RET_ENA_Msk;
    }
    if(Sramx & AP_SRAM1)
    {
        AONPRCM->SMEM_SLPCTRL |=  AP_SRAM1_RET_ENA_Msk;
    }
    if(Sramx & SH_SRAM8K)
    {
        AONPRCM->SMEM_SLPCTRL |=  SH_SRAM8K_RET_ENA_Msk;
    }
}

/**
 * @brief diable the specific sram enter into retention mode
 * 
 * @param : sram bitmap，can be combined by | operator,such as AP_SRAM0|AP_SRAM1|SH_SRAM8K 
 */
void PRCM_SramRetentionDisable(uint32_t Sramx)
{
    if(Sramx & AP_SRAM0)
    {
        AONPRCM->SMEM_SLPCTRL &=  AP_SRAM0_RET_ENA_ResetMsk;
    }
    if(Sramx & AP_SRAM1)
    {
        AONPRCM->SMEM_SLPCTRL &=  AP_SRAM1_RET_ENA_ResetMsk;
    }
    if(Sramx & SH_SRAM8K)
    {
        AONPRCM->SMEM_SLPCTRL &=  SH_SRAM8K_RET_ENA_ResetMsk;
    }
}

void PRCM_SlectXtal32k(void)
{
    if( !(AONPRCM->AONCLK_FLAG & 0x02))   //not use XTAL 32k
    {
        AONPRCM->AONCLK_CTRL |= 0x400;   //power up xtal 32k
        while( !(AONPRCM->AONCLK_FLAG & 0x100));// wait for xtal 32K ready
        AONPRCM->AONCLK_CTRL |= 0x04;     //select xtal 32k 
        while(!(AONPRCM->AONCLK_FLAG & 0x02)); //wait for using xtal 32K flag
        //AONPRCM->AONCLK_CTRL |= 0x800;   //rc32 pd deepsleep
        //AONPRCM->PWRCTL_TEST6 = (AONPRCM->PWRCTL_TEST6 & 0x9F)|0x20;//rc32k_pu_byp
    }
}


void PRCM_SlectRc32k(void)
{
    if(!(AONPRCM->AONCLK_FLAG & 0x01))   // not use rc32k
    {
    	//power up rc32k
		AONPRCM->PWRCTL_TEST6 &= ~0x20;  // set rc32k_pu_byp = 0
		while (((AONPRCM->AONCLK_FLAG & 0x80) != 0x80));   // wait rc32k_rdy == 1, worst 3ms

		// select rc32k
		AONPRCM->AONCLK_CTRL &= ~0x04;  // set aon_xtal32k_sel = 0
		while ((AONPRCM->AONCLK_FLAG & 0x01) != 0x01); // check utc_rc32k_flag == 1

		// power off xtal32k
		AONPRCM->AONCLK_CTRL &= ~0x400; // set aon_xtal32k_pu = 0
    }
}

/**
 * @brief Power up xtal32k
 * @param None
 * @return None
 */
void Prcm_PowerUpXtal32k(void)
{
	AONPRCM->AONCLK_CTRL |= 0x400; // set  aon_xtal32k_pu = 1
}

/**
 * @brief Power off xtal32k
 * @param None
 * @return None
 */
void Prcm_PowerOffXtal32k(void)
{
	AONPRCM->AONCLK_CTRL &= ~0x400; // set  aon_xtal32k_pu = 0		
    AONPRCM->AONCLK_CTRL &= ~0x04;
    while(!(AONPRCM->AONCLK_FLAG & 0x01));
}

/**
 * @brief Power up rc32k
 * @param None
 * @return None
 */
void Prcm_PowerUpRc32k(void)
{
	AONPRCM->PWRCTL_TEST6 &= ~0x20; // set rc32k_pu_byp = 0	  
}

/**
 * @brief Power off rc32k
 * @param None
 * @return None
 */
void Prcm_PowerOffRc32k(void)
{
    AONPRCM->AONCLK_CTRL |= 0x800;//rc32 pd deepsleep
    HWREGB(0x40000030) &= ~0xc0; //rc32k_crtl off sleep
}


/**
 * @brief Get the ready status of xtal32k
 * @note In the worst case, it takes 500ms for this flag to be set.
 * @param None
 * @return The ready status
 *       0x01: xtal32k is ready
 *       0x00: xtal32k is not ready
 */
uint8_t Prcm_GetXtal32kRdy(void)
{
	return ((AONPRCM->AONCLK_FLAG & 0x100) ? 1 : 0);
}

/**
 * @brief Get the ready status of rc32k
 * @param None
 * @return The ready status
 *       0x01: rc32k is ready
 *       0x00: rcc32k is not ready
 */
uint8_t Prcm_GetRc32kRdy(void)
{
	return ((AONPRCM->AONCLK_FLAG & 0x80) ? 1 : 0);
}

/**
 * @brief Select xtal32k as utc32k clock source
 * @note This function must be called after the return value of Prcm_GetXtal32kRdy() is 0x01.
 * @param None
 * @return void
 */
void Prcm_Xtal32kAsUtcClk(void)
{
	// utc32k_clk select xtal32k
	AONPRCM->AONCLK_CTRL |= 0x04;  // set  aon_xtal32k_sel = 1
}

/**
 * @brief Select rc32k as utc32k clock source
 * @param None
 * @return None
 */
void Prcm_Rc32kAsUtcClk(void)
{
	// utc32k_clk select rc32k
	AONPRCM->AONCLK_CTRL &= ~0x04;  // set  aon_xtal32k_sel = 0
}

/**
 * @brief Check whether utc32k_clk source flag is active or inactive
 * @param ClkSrcFlag This parameter can be one of the following values:
 *        @arg UTC_CLKSRC_RC32K
*         @arg UTC_CLKSRC_XTAL32K
 * @return The ready status
 *       1: active
 *       0: inactive
 */
uint8_t Prcm_IsActive_UtcClkFlag(uint8_t ClkSrcFlag)
{
	return ((AONPRCM->AONCLK_FLAG & ClkSrcFlag) ? 1 : 0);  // check utc_xxx_flag == 1. about 120us
}

void PRCM_SetRetldoVol(void)
{
    // retldo 0.9V
    HWREGB(0x40000801) = (HWREGB(0x40000801) & (~0x07)) | 0x03;
}
void PRCM_PowerOnRetldo(void)
{
    // retldo 0.9V
    //HWREGB(0x40000801) = (HWREGB(0x40000801) & (~0x07)) | 0x03;
    AONPRCM->AONPWR_CTRL |= (uint32_t)0x200;
}
void PRCM_PowerOffRetldo(void)
{
    AONPRCM->AONPWR_CTRL &= ~(uint32_t)0x200;
}

uint8_t PRCM_IsOn_Retldo(void)
{
    return (AONPRCM->AONPWR_CTRL & (uint32_t)0x200) ? 1 : 0;
}

void PRCM_SetWkupAonDly(uint8_t ncorerst_dly,uint8_t sidordy_dly,uint8_t hrc_dly,uint8_t xtalrdy_dly,uint8_t plllock_dly)
{
    AONPRCM->AON_DLY_CTRL_L = (AONPRCM->AON_DLY_CTRL_L & 0xFFFF0FF0) | (sidordy_dly<<12) | ncorerst_dly;
    AONPRCM->AON_DLY_CTRL_H = (AONPRCM->AON_DLY_CTRL_H & 0x0FFF0F00) | (plllock_dly<<28) | (xtalrdy_dly<<12) | hrc_dly;
}
/**
 * @brief use lpm to control low-power mode
 * 
 */
inline void PRCM_LPM_LpmCtrlEnable(void)
{
    COREPRCM->LPM_CTRL |= 0x01;
}
/**
 * @brief not use lpm to control low-power mode
 * 
 */
inline void PRCM_LPM_LpmCtrlDisable(void)
{
    COREPRCM->LPM_CTRL &= ~0x01;
}
/**
 * @brief When two cores both request into low-power mode, but also has peripherals are active, 
 * when this scenario occur, wait idle_tm_cnt assert interrupt
 * 
 */
inline void PRCM_LPM_PeriIdleTimeEnable(void)
{
    COREPRCM->LPM_CTRL |= 0x10;
}
/**
 * @brief not enable idle_tm_cnt
 * 
 */
inline void PRCM_LPM_PeriIdleTimeDisable(void)
{
    COREPRCM->LPM_CTRL &= ~0x10;
}
/**
 * @brief set peripheral idle time when enter into lowpower mode
 * 
 */
inline void PRCM_LPM_PeriIdleTimeSet(uint8_t IdleTimeCnt)
{
    COREPRCM->LPM_CTRL = (COREPRCM->LPM_CTRL & (uint32_t)0xFFFF00FF) | ((uint32_t)IdleTimeCnt << 8);
}
/**
 * @brief write 1 to clear idle-time-int
 * 
 */
inline void PRCM_LPM_IdleTimeIntClear(void)
{
    COREPRCM->LPM_CTRL = COREPRCM->LPM_CTRL |= 0x1000000;
}
/**
 * @brief write 1 to force idle
 * 
 */
inline void PRCM_LPM_ForceIdle(void)
{
    COREPRCM->LPM_CTRL = COREPRCM->LPM_CTRL |= 0x0f0000;
}
/**
 * @brief ap request lowpower mode
 * 
 * @param LowPowerMode : specifies the chip's low power mode
 *   This parameter can be one of the following values:
 *     @arg LOW_POWER_MODE_NORMAL: not enter into lowpower mode
 *     @arg LOW_POWER_MODE_DSLEEP: chip enters into deepsleep mode
 *     @arg LOW_POWER_MODE_STANDBY: chip enters into standby mode
 */
inline void PRCM_LPM_ApReqLpMode(uint32_t LowPowerMode)
{
    COREPRCM->LPM_AP_CTRL = (COREPRCM->LPM_AP_CTRL & 0xFFFFFFFC) | LowPowerMode;
}
/**
 * @brief cp request lowpower mode
 * 
 * @param LowPowerMode :should be LOW_POWER_MODE_TypeDef-data
 */
inline void PRCM_LPM_CpReqLpMode(uint32_t LowPowerMode)
{
    COREPRCM->LPM_CP_CTRL = (COREPRCM->LPM_CP_CTRL & 0xFFFFFFFC) | LowPowerMode;
}
/**
 * @brief config EXPIN Wkup Config properties
 * 
 *
 * @param WakeupPolarity specifies the polarity to wakeup
 *   This parameter can be one of the following values:
 *     @arg EXPIN_POL_CFG_HIGH_LEVEL
 *     @arg EXPIN_POL_CFG_LOW_LEVEL
 * @param WakeupEdge specifies the edge to wakeup
 *   This parameter can be one of the following values:
 *     @arg EXPIN_WKUP_CFG_FALLING
 *     @arg EXPIN_WKUP_CFG_ASYNC
 *     @arg EXPIN_WKUP_CFG_RISING
 *     @arg EXPIN_WKUP_CFG_BOTH
 * @param WakeupPulse specifies the pulse to wakeup
 *   This parameter can be one of the following values:
 *     @arg EXPIN_WKP_PLS_CFG_32US
 *     @arg EXPIN_WKP_PLS_CFG_10MS  
 *     @arg EXPIN_WKP_PLS_CFG_20MS
 *     @arg EXPIN_WKP_PLS_CFG_160MS
 
 @param ResetPulse specifies the pulse to reset
 *   This parameter can be one of the following values:
 *     @arg EXPIN_RST_PLS_CFG_20MS_OR_320MS	 
 *     @arg EXPIN_RST_PLS_CFG_1_28S 
 *     @arg EXPIN_RST_PLS_CFG_2_56S 
 *     @arg EXPIN_RST_PLS_CFG_5_12S 
 */
void EXPIN_WkupResetConfig(uint8_t WakeupPolarity,uint8_t WakeupEndge,uint8_t WakeupPluse,uint32_t ResetPulse)
{   
	AONPRCM->RSTWKP_CFG &= ~(0xFE);  //clear configuration
	
	AONPRCM->RSTWKP_CFG |= ((WakeupPolarity<<EXPIN_POLCFG_Pos)&EXPIN_POLCFG_Msk)|((WakeupEndge<<EXPIN_WKP_CFG_Pos)& EXPIN_WKP_CFG_Msk)\
	|((WakeupPluse<<EXPIN_WKP_PLS_CFG_Pos)&EXPIN_WKP_PLS_CFG_Msk)|((ResetPulse<<EXPIN_RST_PLS_CFG_Pos)&EXPIN_RST_PLS_CFG_Msk);
}


void EXPIN_WkupResetEnable(void)
{
    AONPRCM->RST_CTRL1 |= 0x01;
    AONPRCM->RSTWKP_CFG |= 0x01;
}

void EXPIN_ResetEnable(void)
{
    AONPRCM->RST_CTRL1 |= 0x01;
    AONPRCM->RSTWKP_CFG &= ~0x01;
}

void EXPIN_WkupEnable(void)
{
	AONPRCM->RSTWKP_CFG |= 0x01;
    AONPRCM->RST_CTRL1 &= ~0x01;
    // AONPRCM->AON_WAKUP_ENA = 1;//aon wakeup en
}

inline uint8_t EXPIN_Read(void)
{
	return (AONPRCM->RSTWKP_DATA &0x01);
}
/**
@param  EXPIN pull mode set
 *   This parameter can be one of the following values:
 *   @arg EXPIN_FLOAT  
 *   @arg EXPIN_PULL_UP
 *   @arg EXPIN_PULL_DOWN  
*/
void EXPIN_PullSet(uint8_t pull_mode)
{
	AONPRCM->RSTWKP_CTRL &= ~EXPIN_PULL_MSK;  //clear RSTWKP_CTRL before configuration
	AONPRCM->RSTWKP_CTRL |= pull_mode & EXPIN_PULL_MSK;
}


/**
 * @brief config AGPI WKUP properties
 * 
 * @param AGPIx 
 *   This parameter can be one of the following values:
 *     @arg AGPIO0
 *     @arg AGPIO1
 *     @arg AGPIO2
 * @param WakeupPolarity specifies the polarity to wakeup
 *   This parameter can be one of the following values:
 *     @arg AGPI_POL_CFG_HIGH_LEVEL
 *     @arg AGPI_POL_CFG_LOW_LEVEL
 * @param WakeupEdge specifies the edge to wakeup
 *   This parameter can be one of the following values:
 *     @arg AGPI_WKUP_CFG_RISING
 *     @arg AGPI_WKUP_CFG_ASYNC
 *     @arg AGPI_WKUP_CFG_FALLING
 *     @arg AGPI_WKUP_CFG_BOTH
 * @param WakeupPulse specifies the pulse to wakeup
 *   This parameter can be one of the following values:
 *     @arg AGPI_PLS_CFG_3_UTC_CLK
 *     @arg AGPI_PLS_CFG_5_UTC_CLK
 *     @arg AGPI_PLS_CFG_7_UTC_CLK
 *     @arg AGPI_PLS_CFG_35_UTC_CLK
 */
void AGPI_WakeupConfig(uint8_t AGPIx,uint32_t WakeupPolarity,uint32_t WakeupEdge,uint32_t WakeupPulse)
{
    uint32_t agpiwkup_ctrl;
    uint32_t agpiwkup_shifter;

    agpiwkup_ctrl = AONPRCM->AGPIWKUP_CTRL;
    if(AGPIx == AGPIO0)
    {
        agpiwkup_ctrl = agpiwkup_ctrl & AGPI0_POL_CFG_ENA_ResetMsk & AGPI0_WKUP_CFG_ENA_ResetMsk & AGPI0_PLS_CFG_ENA_ResetMsk;
    }
    else if(AGPIx == AGPIO1)
    {
        agpiwkup_ctrl = agpiwkup_ctrl & AGPI1_POL_CFG_ENA_ResetMsk & AGPI1_WKUP_CFG_ENA_ResetMsk & AGPI1_PLS_CFG_ENA_ResetMsk;
    }
    else if(AGPIx == AGPIO2)
    {
        agpiwkup_ctrl = agpiwkup_ctrl & AGPI2_POL_CFG_ENA_ResetMsk & AGPI2_WKUP_CFG_ENA_ResetMsk & AGPI2_PLS_CFG_ENA_ResetMsk;
    }
    agpiwkup_shifter = (uint32_t)AGPIx<<3;
    agpiwkup_ctrl |= (WakeupPolarity<<agpiwkup_shifter) | (WakeupEdge<<agpiwkup_shifter) | (WakeupPulse<<agpiwkup_shifter);
    AONPRCM->AGPIWKUP_CTRL = agpiwkup_ctrl;
}

void AGPI_WakeupEnable(uint8_t AGPIx)
{
    uint32_t agpiwkup_shifter;

    agpiwkup_shifter = (uint32_t)AGPIx<<3;
    AONPRCM->AGPIWKUP_CTRL |= 1<<agpiwkup_shifter;
    // AONPRCM->AON_WAKUP_ENA = 1;//aon wakeup en
}

void AGPI_WakeupDisable(uint8_t AGPIx)
{
    uint32_t agpiwkup_shifter;

    agpiwkup_shifter = (uint32_t)AGPIx<<3;
    AONPRCM->AGPIWKUP_CTRL &= ~(1<<agpiwkup_shifter);
}

inline void AGPIO_InputEnable(uint8_t AGPIOx)
{
    AGPIO->CTRL1 |= 1<<AGPIOx;
}
inline void AGPIO_InputDisable(uint8_t AGPIOx)
{
    AGPIO->CTRL1 &= ~(1<<AGPIOx);
}

inline void AGPIO_OutputEnable(uint8_t AGPIOx)
{
    AGPIO->CTRL1 &= ~(1<<(AGPIOx+4));
}
inline void AGPIO_OutputDisable(uint8_t AGPIOx)
{
    AGPIO->CTRL1 |= 1<<(AGPIOx+4);
}

static uint8_t AGPIO_Read(uint8_t AGPIOx)
{
    uint8_t read_value;
    read_value = AGPIO->CTRL0 & (1<<AGPIOx);
    return read_value;
}

static uint8_t AGPIO_Output_Read(uint8_t AGPIOx)
{
    uint8_t read_value;
    read_value = AGPIO->CTRL3 & (1<<AGPIOx);
    return read_value;
}

uint8_t AGPIO_ReadPin(uint8_t AGPIOx)
{
    //输入使能：低4位对应bit位1
    //输出使能：高4位对应bit为0
    uint8_t output_state, input_state;
    output_state = AGPIO->CTRL1 & (1 << (AGPIOx + 4));
    input_state = AGPIO->CTRL1 & (1 << AGPIOx);

    //输入使能且输出失能，则为输入模式
    if( input_state && output_state )
    {
        return (AGPIO_Read(AGPIOx) >> AGPIOx);
    }
    else
    {
        return (AGPIO_Output_Read(AGPIOx) >> AGPIOx);
    }
}

void AGPIO_TogglePin(uint8_t AGPIO_Pin)
{
    if (AGPIO_ReadPin(AGPIO_Pin) == 1)
    {
        AGPIO_Clear(AGPIO_Pin);
    }
    else
    {
        AGPIO_Set(AGPIO_Pin);
    }
}

inline void AGPIO_PullupEnable(uint8_t AGPIOx)
{
    AGPIO->CTRL2 &= ~(1<<AGPIOx);
}
inline void AGPIO_PullupDisable(uint8_t AGPIOx)
{
    AGPIO->CTRL2 |= 1<<AGPIOx;
}

inline void AGPIO_PulldownEnable(uint8_t AGPIOx)
{
    AGPIO->CTRL2 &= ~(1<<(AGPIOx+4));
}
inline void AGPIO_PulldownDisable(uint8_t AGPIOx)
{
    AGPIO->CTRL2 |= 1<<(AGPIOx+4);
}

inline void AGPIO_OpendrainEnable(uint8_t AGPIOx)
{
    AGPIO->CTRL3 &= ~(1<<(AGPIOx+4));
}
inline void AGPIO_OpendrainDisable(uint8_t AGPIOx)
{
    AGPIO->CTRL3 |= 1<<(AGPIOx+4);
}

inline void AGPIO_Set(uint8_t AGPIOx)
{
    AGPIO->CTRL3 |= 1<<(AGPIOx);
}
inline void AGPIO_Clear(uint8_t AGPIOx)
{
    AGPIO->CTRL3 &= ~(1<<(AGPIOx));
}

inline void AGPIO_GPIO0_7_RetentionEnable(void)
{
    AGPIO->CTRL4 = 0;
}

inline void AGPIO_GPIO0_7_RetentionDisable(void)
{
    AGPIO->CTRL4 = 1;
}

inline void AGPIO_GPIO0_7_Update(void)
{
    AGPIO->CTRL5 = 1;
}
//GPI WKUP Init
void GPI_WkupInit(uint8_t GPIx)
{
	GPIO_InitTypeDef GPIO_InitStu = {0};
	if(GPIx<3)
	{
		AONPRCM->BOOTMODE =0x80;  //force flash mode
	}
    else if(GPIx==3 || GPIx==4)
	{
		AONPRCM -> LPUA1_CTRL &= ~0x03; //PAD3 and PAD4 need disable LPUART
	}
	else 
	{
	}
	//set GPIx INPUT mode
	GPIO_InitStu.Pin      = GPIx;
	GPIO_InitStu.Pull     = GPIO_PULL_DOWN;
	GPIO_InitStu.Mode     = GPIO_MODE_INPUT;
    GPIO_Init(&GPIO_InitStu, NORMAL_SPEED);

    //Open IOLDO1
	HWREGB(0x40000803) &= ~0x20;//bug7553 i_aon_ioldo1_vopd_en
	AONPRCM->CORE_PWR_CTRL3 |= 0x02;//ioldo1 psm mode
	
}

/**
* @brief config GPI WKUP properties
 * 
 * @param GPIx 
 *   This parameter can be one of the following values:
 *     @arg GPIO0
 *     @arg GPIO1
 *     @arg GPIO2
 *     @arg GPIO3
 *     @arg GPIO4
 *     @arg GPIO5
 *     @arg GPIO6
 *     @arg GPIO7
 *
 * @param AsyncEnable specifies the edge to wakeup
 *   This parameter can be one of the following values:
 *     @arg DISABLE
 *     @arg ENABLE
 *    
 * @param WakeupPolarity specifies the polarity to wakeup
 *   This parameter can be one of the following values:
 *     @arg GPI_POL_CFG_HIGH_LEVEL
 *     @arg GPI_POL_CFG_LOW_LEVEL
 
 * @param PulseRemap use AGPIx to wakeup,AGPIx_wkup_ena should be 1
 *   This PulseRemap can be one of the following values:
 *     @arg DISABLE
 *     @arg ENABLE
 *    
*/
void GPI_WakeupConfig(uint8_t GPIx,uint32_t AsyncEnable,uint32_t WakeupPolarity,uint32_t PulseRemap)
{
	uint32_t gpiwkup_ctr;
	uint32_t gpiwkup_ctrNew=0;
    uint32_t gpiwkup_shifter;

    gpiwkup_ctr = AONPRCM->GPIWKUP_CTRL;
	
	if(GPIx <5)
	{
		gpiwkup_ctrNew = (AsyncEnable << GPI_WKUP_ASYNC_ENA_Pos)|(WakeupPolarity << GPI_POL_CFG_Pos);
	}
	else if(GPIx >= 5 && GPIx <= 7 )
	{
		gpiwkup_ctrNew = (AsyncEnable << GPI_WKUP_ASYNC_ENA_Pos)|(WakeupPolarity << GPI_POL_CFG_Pos) |(PulseRemap << GPI_FILTER_REMAP_Pos);
	
	}
	else 
	{
	}
	
	gpiwkup_shifter = GPIx <<2 ;//*4,整体移动4bits	
	gpiwkup_ctr &= ~(0x0000000F << gpiwkup_shifter); //clear data
	gpiwkup_ctr |= gpiwkup_ctrNew << gpiwkup_shifter;
	AONPRCM->GPIWKUP_CTRL = gpiwkup_ctr;
}

void GPI_WakeupEnable(uint8_t GPIx)
{
	uint32_t gpiwkup_ctrNew=0;
    uint32_t gpiwkup_shifter;

	gpiwkup_shifter = GPIx<<2;//*4,整体移动4bits	
	gpiwkup_ctrNew = GPI_WKUP_ENA_Msk << gpiwkup_shifter;
	
	AONPRCM->GPIWKUP_CTRL |= gpiwkup_ctrNew;
    // AONPRCM->AON_WAKUP_ENA = 1;//aon wakeup en
}

void GPI_WakeupDisable(uint8_t GPIx)
{
	uint32_t gpiwkup_ctrNew=0;
    uint32_t gpiwkup_shifter;

	gpiwkup_shifter = GPIx<<2;//*4,整体移动4bits	
	gpiwkup_ctrNew = GPI_WKUP_ENA_Msk << gpiwkup_shifter;
	
	AONPRCM->GPIWKUP_CTRL &= ~gpiwkup_ctrNew;
}


/**
 * @brief enable svd detect.
 * 
 */
inline void SVD_Start(void)
{
    SVD->CFG |= SVD_MODE_NO_FILTER;
}
/**
 * @brief disable svd detect.
 * 
 */
inline void SVD_Stop(void)
{
    SVD->CFG &= SVD_FILT_CFG_ResetMsk;
}
/**
 * @brief Config SVD parameters.
 * 
 * @param SVD_FilterMode :svd filter mode.
 *   This parameter can be one of the following values:
 *     @arg SVD_MODE_DISABLE: disable svd
 *     @arg SVD_MODE_NO_FILTER: enable svd without filter
 *     @arg SVD_MODE_FILTER_HRC_2p5us: filter unit 2.5us for hrc_clk. If use utc_clk, it's 1 utc_clk
 *     @arg SVD_MODE_FILTER_UTC_30us: filter unit 2.5us for hrc_clk. If use utc_clk, it's 1 utc_clk
 *     @arg SVD_MODE_FILTER_HRC_10us: filter unit 10us for hrc_clk. If use utc_clk, it's 1 utc_clk
 * @param FilterNum :the length of sample filter.
 *   This parameter can be one of the following values:
 *     @arg SVD_FILTER_NUM_0
 *     @arg SVD_FILTER_NUM_x
 *     @arg SVD_FILTER_NUM_126
 * @param PeriodUnit :Select delay unit for period.
 *   This parameter can be one of the following values:
 *     @arg SVD_PRDUNIT_SEL_HRC10us
 *     @arg SVD_PRDUNIT_SEL_RC32K5ms
 *     @arg SVD_PRDUNIT_SEL_XTAK32K7p5ms
 * @param PeriodNUm :period to start svd for next measure.
 *   This parameter can be one of the following values:
 *     @arg SVD_PERIOD_NUM_0
 *     @arg SVD_PERIOD_NUM_x
 *     @arg SVD_PERIOD_NUM_2048
 * @param SampleNum :Number of sample times to finish svd.
 *   This parameter can be one of the following values:
 *     @arg SVD_SAMPLE_NUM_1
 *     @arg SVD_SAMPLE_NUM_x
 *     @arg SVD_SAMPLE_NUM_15
 * @param WarmDelay :svd enable wram delay control.
 *   This parameter can be one of the following values:
 *     @arg SVD_WARM_DLY_0
 *     @arg SVD_WARM_DLY_20us
 *     @arg SVD_WARM_DLY_40us
 *     @arg SVD_WARM_DLY_80us
 */
void SVD_Config(uint32_t SVD_FilterMode,uint32_t FilterNum,uint32_t PeriodUnit,uint32_t PeriodNUm,uint32_t SampleNum,uint32_t WarmDelay)
{
    uint32_t svd_config;
    svd_config = SVD->CFG;
    svd_config = (svd_config & SVD_FILT_CFG_ResetMsk) | SVD_FilterMode;
    svd_config = (svd_config & SVD_FILT_NUM_ResetMsk) | FilterNum;
    svd_config = (svd_config & SVD_PRDUNIT_SEL_ResetMsk) | PeriodUnit;
    svd_config = (svd_config & SVD_PRD_CFG_ResetMsk) | PeriodNUm;
    svd_config = (svd_config & SVD_SAMPLE_NUM_ResetMsk) | SampleNum;
    svd_config = (svd_config & SVD_WARM_DLY_ResetMsk) | WarmDelay;
    SVD->CFG = svd_config;
}

/**
 * @brief Gnerate interrupt or reset when voltage below minimum threshold.
 * 
 * @param GenIntOrReset : interrupt or reset
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN_CFG_GEN_INT
 *     @arg SVD_THDMIN_CFG_GEN_RST
 */
inline void SVD_THDMIN_GenerateConfig(uint32_t GenIntOrReset)
{
    SVD->CFG = (SVD->CFG & SVD_THDMIN_CFG_ResetMsk) | GenIntOrReset;
}

/**
 * @brief :Disable power down high current consumers(BBsystem,PLL,CPCore) when voltage below minimum threshold.
 * 
 */
inline void SVD_THDMIN_HCPD_Disable(void)
{
    SVD->CFG = (SVD->CFG & SVD_HCPD_CTL_ResetMsk) | SVD_HCPD_CTL_Disable;
}

/**
 * @brief :Enable power down high current consumers(BBsystem,PLL,CPCore) when voltage below minimum threshold.
 * 
 */
inline void SVD_THDMIN_HCPD_Enable(void)
{
    SVD->CFG = (SVD->CFG & SVD_HCPD_CTL_ResetMsk) | SVD_HCPD_CTL_Enable;
}

/**
 * @brief :Disable enter into deepsleep  when voltage below threshold1.
 * 
 */
inline void SVD_THD1_DeepsleepDisable(void)
{
    SVD->CTRL = (SVD->CTRL & SVD_THD1_DPSLP_CTL_ResetMsk) | SVD_THD1_DPSLP_CTL_Disable;
}

/**
 * @brief :Enable enter into deepsleep  when voltage below threshold1.
 * 
 */
inline void SVD_THD1_DeepsleepEnable(void)
{
    SVD->CTRL = (SVD->CTRL & SVD_THD1_DPSLP_CTL_ResetMsk) | SVD_THD1_DPSLP_CTL_Enable;
}

/**
 * @brief Configure the SVD threshold register.
 * 
 * @param SVD_THDx :Specifies the thd register.
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3
 * @param Voltage_mv :Specifies the trigger voltage,it should be 640mv to 1260mv
 */
void SVD_THD_VoltageSet(uint8_t SVD_THDx,uint16_t Voltage_mv)
{
    uint32_t svd_vol_resetmsk,vsel;

    if(Voltage_mv<SVD_THD_VSEL_MIN || Voltage_mv>SVD_THD_VSEL_MAX)
    {
        return;
    }
    else
    {
        vsel = (Voltage_mv - SVD_THD_VSEL_MIN)/SVD_THD_VSEL_STEP;
    }

    svd_vol_resetmsk = ~(0x1F << ((uint32_t)SVD_THDx * 8));  
    vsel = ((uint32_t)vsel) << ((uint32_t)SVD_THDx * 8);
    SVD->THRES_CTRL = (SVD->THRES_CTRL & svd_vol_resetmsk) | vsel;
}

/**
 * @brief Configure the SVD thd voltage selection register.
 * 
 * @param SVD_THDx :Specifies the thd register.
 *   This parameter can be one of the following values:
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3
 * @param VoltageSource 
 *   This parameter can be one of the following values:
 *     @arg SVD_THD_SRC_SEL_SVDPIN
 *     @arg SVD_THD_SRC_SEL_VBATP
 * @note SVD_THDMIN only supports SVDPIN
 */
void SVD_THD_SourceSel(uint8_t SVD_THDx,uint16_t VoltageSource)
{
    uint16_t svd_thd_src_resetmsk,source;

    svd_thd_src_resetmsk = ~(0x1 << ((uint16_t)SVD_THDx));  
    source = VoltageSource << ((uint16_t)SVD_THDx);
    SVD->CTRL = (SVD->CTRL & svd_thd_src_resetmsk) | source;
}

/**
 * @brief Reads the specified THD interrupt status.
 * 
 * @param SVD_THDx 
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3 
 * @return uint8_t, the int status of the specified THD
 */
uint8_t SVD_IntStatusGetBit(uint8_t SVD_THDx)
{
    if((SVD->INT_STAT & (1<<SVD_THDx)) != (uint8_t)RESET)
    {
        return (uint8_t)SET;
    }
    else
    {
        return (uint8_t)RESET;
    }
}

/**
 * @brief Clears the specified THD interrupt status.
 * 
 * @param SVD_THDx 
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3 
 * @return uint8_t, the int status of the specified THD
 */
inline void SVD_IntStatusClearBit(uint8_t SVD_THDx)
{
    SVD->INT_STAT = 1<<SVD_THDx;
}

/**
 * @brief Clears the THD interrupt status.
 * 
 */
inline void SVD_IntStatusClear(void)
{
    SVD->INT_STAT = 0x0F;
}

/**
 * @brief Reads the SVD INT_STAT register.
 * @return uint8_t, the int status of SVD
 */
inline uint8_t SVD_IntStatusGet(void)
{
    return SVD->INT_STAT;
}

/**
 * @brief Enable the specified THD's interrupt.
 * 
 * @param SVD_THDx 
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3 
 */
inline void SVD_IntEnable(uint8_t SVD_THDx)
{
    SVD->INT_CTRL |= 1<<SVD_THDx;
}

/**
 * @brief Disable the specified THD's interrupt.
 * 
 * @param SVD_THDx 
 *   This parameter can be one of the following values:
 *     @arg SVD_THDMIN
 *     @arg SVD_THD1
 *     @arg SVD_THD2
 *     @arg SVD_THD3 
 */
inline void SVD_IntDisable(uint8_t SVD_THDx)
{
    SVD->INT_CTRL &= ~(1<<SVD_THDx);
}

///**
// * @brief Reads the  SM_Active_Status register.
// * @return uint8_t, the status of SM_Active
// */
//uint8_t SM_Active_StatusGet(void)
//{
//    return (AONPRCM->ISO7816_VDD_CTL) & 0x01;
//}

/**
 * @brief Config PWRMUX power source selection in normalmode.
 * 
 * @param Pwr_Mux 
 *   This parameter can be one of the following values:
 *     @arg PWRMUX0
 *     @arg PWRMUX1
 *     @arg PWRMUX2
 * @param Pwr_Sel 
 *   This parameter can be one of the following values:
 *     @arg SIDO_SEL
 *     @arg IOLDO1_SEL
 *     @arg IOLDO2_SEL
 */
void PWRMUX_CFG_Sel(uint8_t Pwr_Mux,uint8_t Pwr_Sel)
{
	switch (Pwr_Mux) 
	{
		case PWRMUX0:
		{
            if(Pwr_Sel == SIDO_SEL)
            {
                AONPRCM->PWRMUX0_TRIG = 0;
			    AONPRCM->PWRMUX0_CFG = (AONPRCM->PWRMUX0_CFG & ~0x03)|0x01;//select sido
                AONPRCM->PWRMUX0_TRIG = 1;
            }
            else if(Pwr_Sel == IOLDO1_SEL)
            {
                AONPRCM->PWRMUX0_TRIG = 0;
                AONPRCM->PWRMUX0_CFG = (AONPRCM->PWRMUX0_CFG & ~0x03)|0x02;//select ioldo1
                AONPRCM->PWRMUX0_TRIG = 1;
            }
			break;
		}
		case PWRMUX1:
		{
            if(Pwr_Sel == SIDO_SEL)
            {   AONPRCM->PWRMUX1_TRIG = 0;
               	AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x03)|0x01;//select sido 
                AONPRCM->PWRMUX1_TRIG = 1;
            }
            else if(Pwr_Sel == IOLDO1_SEL)
            {
                AONPRCM->PWRMUX1_TRIG = 0;
                AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x03)|0x02;//select ioldo1
                AONPRCM->PWRMUX1_TRIG = 1;
            }
            else
            {
                AONPRCM->PWRMUX1_TRIG = 0;
                AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x03)|0x03;//select ioldo2
                AONPRCM->PWRMUX1_TRIG = 1;
            }
			break;
		}
		case PWRMUX2:
		{
			if(Pwr_Sel == SIDO_SEL)
            {
                AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x03)|0x01;//select sido
                AONPRCM->PWRMUX2_TRIG = 1;
            }
            else if(Pwr_Sel == IOLDO1_SEL)
            {
                AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x03)|0x02;//select ioldo1
                AONPRCM->PWRMUX2_TRIG = 1;
            }
            else
            {
                AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x03)|0x03;//select ioldo2
                AONPRCM->PWRMUX2_TRIG = 1;
            }
			break;
		}
		default:
		break;
	}
}
/**
 * @brief Config PWRMUX off.
 * 
 * @param Pwr_Mux 
 *   This parameter can be one of the following values:
 *     @arg PWRMUX0
 *     @arg PWRMUX1
 *     @arg PWRMUX2
 */
void PWRMUX_CFG_Off(uint8_t Pwr_Mux)
{
	switch (Pwr_Mux) 
	{
		case PWRMUX0:
		{

			AONPRCM->PWRMUX0_TRIG = 0;
            AONPRCM->PWRMUX0_CFG = (AONPRCM->PWRMUX0_CFG & ~0x03)|0x04;
            AONPRCM->PWRMUX0_TRIG = 1;
			break;
		}
		case PWRMUX1:
		{
			AONPRCM->PWRMUX1_TRIG = 0;
            AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & ~0x03)|0x04;
            AONPRCM->PWRMUX1_TRIG = 1;
			break;
		}
		case PWRMUX2:
		{
			AONPRCM->PWRMUX2_TRIG = 0;
            AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x03)|0x04;
            AONPRCM->PWRMUX2_TRIG = 1;
			break;
		}
		default:
		break;
	}
}
/**
 * @brief Lpua pwr control .
 * 
 * @param Lpua_Pwr_Mode 
 *   This parameter can be one of the following values:
 *     @arg LPUA_DEEPSLEEP_MODE_OFF
 *     @arg LPUA_ANY_MODE_ON
 *     @arg LPUA_ANY_MODE_OFF
 */
void PRCM_LPUA_PWR_Ctl(uint8_t Lpua_Pwr_Mode)
{
	if(Lpua_Pwr_Mode == LPUA_DEEPSLEEP_MODE_OFF) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LPUA_PWR_MODE_CTL_ResetMsk) | (LPUA_DEEPSLEEP_MODE_OFF << LPUA_PWR_MODE_CTL_Pos);
	else if(Lpua_Pwr_Mode == LPUA_ANY_MODE_ON) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LPUA_PWR_MODE_CTL_ResetMsk) | (LPUA_ANY_MODE_ON << LPUA_PWR_MODE_CTL_Pos);
	else	AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LPUA_PWR_MODE_CTL_ResetMsk) | (LPUA_ANY_MODE_OFF << LPUA_PWR_MODE_CTL_Pos);
}
/**
 * @brief Lcdc pwr control .
 * 
 * @param Lcdc_pwr_Mode 
 *   This parameter can be one of the following values:
 *     @arg LCDC_SLEEP_MODE_OFF
 *     @arg LCDC_ANY_MODE_ON
 *     @arg LCDC_ANY_MODE_OFF
 */
void PRCM_LCDC_PWR_Ctl(uint8_t Lcdc_Pwr_Mode)
{
	if(Lcdc_Pwr_Mode == LCDC_SLEEP_MODE_OFF) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LCDC_PWR_MODE_CTL_ResetMsk) | (LCDC_SLEEP_MODE_OFF << LCDC_PWR_MODE_CTL_Pos);
	else if(Lcdc_Pwr_Mode == LCDC_ANY_MODE_ON) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LCDC_PWR_MODE_CTL_ResetMsk) | (LCDC_ANY_MODE_ON << LCDC_PWR_MODE_CTL_Pos);
	else	AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & LCDC_PWR_MODE_CTL_ResetMsk) | (LCDC_ANY_MODE_OFF << LCDC_PWR_MODE_CTL_Pos);
}
/**
 * @brief RC32K_CTRL pwr control .
 * 
 * @param RC32k_Ctrl_Pwr_Mode 
 *   This parameter can be one of the following values:
 *     @arg RC32K_CTRL_SLEEP_MODE_OFF
 *     @arg RC32K_CTRL_ANY_MODE_ON
 *     @arg RC32K_CTRL_ANY_MODE_OFF
 */
void PRCM_RC32K_CTRL_PWR_Ctl(uint8_t RC32k_Ctrl_Pwr_Mode)
{
	if(RC32k_Ctrl_Pwr_Mode == RC32K_CTRL_SLEEP_MODE_OFF) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & RC32K_CTRL_PWR_MODE_CTL_ResetMsk) | (RC32K_CTRL_SLEEP_MODE_OFF << RC32K_CTRL_PWR_MODE_CTL_Pos);
	else if(RC32k_Ctrl_Pwr_Mode == RC32K_CTRL_ANY_MODE_ON) 
		AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & RC32K_CTRL_PWR_MODE_CTL_ResetMsk) | (RC32K_CTRL_ANY_MODE_ON << RC32K_CTRL_PWR_MODE_CTL_Pos);
	else	AONPRCM->AONPWR_CTRL = (AONPRCM->AONPWR_CTRL & RC32K_CTRL_PWR_MODE_CTL_ResetMsk) | (RC32K_CTRL_ANY_MODE_OFF << RC32K_CTRL_PWR_MODE_CTL_Pos);
}
/**
 * @brief Ioldo1/2 mode cfg.
 * 
 * @param WORK_MODE 
 *   This parameter can be one of the following values:
 *     @arg NOR_MODE
 *     @arg LP_MODE
 *     @arg PD_MODE
 * @param IOLDO_ModeCtl 
 *   This parameter can be one of the following values:
 *   NOR_MODE：
 *     @arg IOLDO1_LPMODE_Enable
 *     @arg IOLDO1_NORMAL_Enable
 *     @arg IOLDO2_LPMODE_Enable
 *     @arg IOLDO2_NORMAL_Enable
 *   LP_MODE：
 *     @arg IOLDO1_LPMODE_LP_Enable
 *     @arg IOLDO1_LPMODE_PSM_Enable
 *     @arg IOLDO2_LPMODE_LP_Enable
 *     @arg IOLDO2_LPMODE_PSM_Enable
 *   PD_MODE：
 *     @arg IOLDO1_PD_ANY_Enable
 *     @arg IOLDO2_PD_ANY_Enable
 */
void PRCM_IOLDO_ModeCtl(uint8_t WORK_MODE, uint16_t IOLDO_ModeCtl)
{
	if(WORK_MODE == NOR_MODE)
	{
		if(IOLDO_ModeCtl & IOLDO1_LPMODE_Enable)  
			AONPRCM->CORE_PWR_CTRL3 &=  0xF0;
		else if(IOLDO_ModeCtl & IOLDO1_NORMAL_Enable)  
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_NORMAL_CTL_Msk;   

		if(IOLDO_ModeCtl & IOLDO2_LPMODE_Enable) 
			AONPRCM->CORE_PWR_CTRL3 &=  0x0F;    
		else if(IOLDO_ModeCtl & IOLDO2_NORMAL_Enable)  
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0x0F) | IOLDO2_NORMAL_CTL_Msk;
	}
	else if(WORK_MODE == LP_MODE)
	{ 
		if(IOLDO_ModeCtl & IOLDO1_LPMODE_LP_Enable)
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_LPMODE_LP_CTL_Msk;
		else if(IOLDO_ModeCtl & IOLDO1_LPMODE_PSM_Enable)
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_LPMODE_PSM_CTL_Msk;

		if(IOLDO_ModeCtl & IOLDO2_LPMODE_LP_Enable) 
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0x0F) | IOLDO2_LPMODE_LP_CTL_Msk;
		else if(IOLDO_ModeCtl & IOLDO2_LPMODE_PSM_Enable) 
			AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0x0F) | IOLDO2_LPMODE_PSM_CTL_Msk;
	}
	else
	{
		if(IOLDO_ModeCtl & IOLDO1_PD_ANY_Enable)
				AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_PD_ANY_CTL_Msk;
		if(IOLDO_ModeCtl & IOLDO2_PD_ANY_Enable)  
				AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0x0F) | IOLDO2_PD_ANY_CTL_Msk;   
	}
}

//timeout_cnt 必须小于319
uint32_t PRCM_Check_Utccnt_Timeout(uint32_t start_cnt, uint32_t timeout_cnt)
{
    uint32_t new_cnt = UTC_ClkCntConvert(UTC->CLK_CNT);

    // 无溢出情况：新tick值大于等于旧tick值
	if(new_cnt >= start_cnt)
	{
		if(new_cnt - start_cnt >= timeout_cnt)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	// 溢出情况：新tick值小于旧tick值
	else
	{
		if(320 - start_cnt + new_cnt >= timeout_cnt)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}

/**
 * @brief Ioldo1 mode cfg.
 * 
 * @param IOLDO_ModeCtl 
 *   This parameter can be one of the following values:
 *     @arg IOLDO1_LPMODE_Enable
 *     @arg IOLDO1_NORMAL_Enable
 *     @arg IOLDO1_LPMODE_LP_Enable
 *     @arg IOLDO1_LPMODE_PSM_Enable
 *     @arg IOLDO1_PD_ANY_Enable
 */
void PRCM_IOLDO1_ModeCtl(uint16_t IOLDO_ModeCtl)
{
    //extern void delay_func_us(float tick_us);
    //实测lp-normal rdy需要100us，normal-lp 有时5us，长的发现40
    uint8_t core_pwr_ctrl3 = AONPRCM->CORE_PWR_CTRL3;

    if(IOLDO_ModeCtl & IOLDO1_LPMODE_Enable)  
    {
        //HWREGB(0X40000803) |= (1 << 5);  // 不打开psmioldo时，置1
        HWREGB(0X40000803) = (HWREGB(0X40000803) & 0xE0) | 0x1D | (1 << 5);    //lpmode 3/1.8v
        //AONPRCM->CORE_PWR_CTRL3 &=  0xF0;
        AONPRCM->CORE_PWR_CTRL3 =  core_pwr_ctrl3 & 0xF0;
        //while( (AONPRCM->AONSYS_FLAG & 0x100));
    }
    else if(IOLDO_ModeCtl & IOLDO1_NORMAL_Enable)
    {
        //HWREGB(0X40000803) |= (1 << 5);  // 不打开psmioldo时，置1
        //AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_NORMAL_CTL_Msk; 
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0xF0) | IOLDO1_NORMAL_CTL_Msk; 
        uint32_t start_cnt = UTC_ClkCntConvert(UTC->CLK_CNT);
        while( ((AONPRCM->AONSYS_FLAG & 0x100)==0)  &&  (PRCM_Check_Utccnt_Timeout(start_cnt,5) == 0));
        //delay_func_us(300);
        HWREGB(0X40000803) = (HWREGB(0X40000803) & 0xE0) | 0x06 | (1 << 5);    //normal 3.3/1.98v actual 3/1.8v
    }
    else if(IOLDO_ModeCtl & IOLDO1_LPMODE_LP_Enable)
    {
        //HWREGB(0X40000803) |= (1 << 5);  // 不打开psmioldo时，置1
        HWREGB(0X40000803) = (HWREGB(0X40000803) & 0xE0) | 0x1D | (1 << 5);    //lpmode 3/1.8v        
        //AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_LPMODE_LP_CTL_Msk;
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0xF0) | IOLDO1_LPMODE_LP_CTL_Msk;
        //while( (AONPRCM->AONSYS_FLAG & 0x100));
    }
    else if(IOLDO_ModeCtl & IOLDO1_LPMODE_PSM_Enable)
    {
        //HWREGB(0X40000803) &= ~(1 << 5);  // bug 7553
        HWREGB(0X40000803) = ((HWREGB(0X40000803) & 0xE0) | 0x1D)  & (~(1 << 5)) ;    //lpmode 3/1.8v        
        //AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_LPMODE_PSM_CTL_Msk;
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0xF0) | IOLDO1_LPMODE_PSM_CTL_Msk;
        //while( (AONPRCM->AONSYS_FLAG & 0x100)); 
    }
    else
    {
        //HWREGB(0X40000803) |= (1 << 5);  // 不打开psmioldo时，置1
        HWREGB(0X40000803) = (HWREGB(0X40000803) & 0xE0) | 0x1D | (1 << 5);    //lpmode 3/1.8v
        //AONPRCM->CORE_PWR_CTRL3 = (AONPRCM->CORE_PWR_CTRL3 & 0xF0) | IOLDO1_PD_ANY_CTL_Msk;
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0xF0) | IOLDO1_PD_ANY_CTL_Msk;
    }

    //normal >> lp
    if((core_pwr_ctrl3 & IOLDO1_NORMAL_CTL_Msk)  &&  (!(IOLDO_ModeCtl & IOLDO1_NORMAL_Enable)))
    {
        //while( (AONPRCM->AONSYS_FLAG & 0x100));
        utc_cnt_delay(4);   //等待normal lp切换完成，不然可能会影响AT的收发，状态异常
        utc_cnt_delay(16);  //等待normal>>lp稳定。底板上测试，会下降100mv然后500us内回升。进standby切换psmioldo时也会下降，这两个下降给分开
    }
}

/**
 * @brief Ioldo2 mode cfg.
 * 
 * @param IOLDO_ModeCtl 
 *   This parameter can be one of the following values:
 *     @arg IOLDO2_LPMODE_Enable
 *     @arg IOLDO2_NORMAL_Enable
 *     @arg IOLDO2_LPMODE_LP_Enable
 *     @arg IOLDO2_LPMODE_PSM_Enable
 *     @arg IOLDO2_PD_ANY_Enable
 */
void PRCM_IOLDO2_ModeCtl(uint16_t IOLDO_ModeCtl)
{
    //实测lp-normal rdy需要100us，normal-lp 有时5us，长的发现40
    uint8_t core_pwr_ctrl3 = AONPRCM->CORE_PWR_CTRL3;

    if(IOLDO_ModeCtl & IOLDO2_LPMODE_Enable)  
    {
        HWREGH(0X40000804) = (HWREGH(0X40000804) & 0xE0FF) | (0x1D << 8) | (1 << 5);    //lpmode 3/1.8v
        AONPRCM->CORE_PWR_CTRL3 =  core_pwr_ctrl3 & 0x0F;
    }
    else if(IOLDO_ModeCtl & IOLDO2_NORMAL_Enable)
    {
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0x0F) | IOLDO2_NORMAL_CTL_Msk; 
        uint32_t start_cnt = UTC_ClkCntConvert(UTC->CLK_CNT);
        while( ((AONPRCM->AONSYS_FLAG & 0x200)==0)  &&  (PRCM_Check_Utccnt_Timeout(start_cnt,5) == 0));

        HWREGH(0X40000804) = (HWREGH(0X40000804) & 0xE0FF) | (0x0 << 8) | (1 << 5);    //normal 3.1/1.86v actual 3/1.8v
    }
    else if(IOLDO_ModeCtl & IOLDO2_LPMODE_LP_Enable)
    {
        HWREGH(0X40000804) = (HWREGH(0X40000804) & 0xE0FF) | (0x1D << 8) | (1 << 5);    //lpmode 3/1.8v
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0x0F) | IOLDO2_LPMODE_LP_CTL_Msk;
    }
    else if(IOLDO_ModeCtl & IOLDO2_LPMODE_PSM_Enable)
    {
        HWREGH(0X40000804) = ((HWREGH(0X40000804) & 0xE0FF) | (0x1D << 8))  & (~(1 << 5)) ;    //lpmode 3/1.8v        
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0x0F) | IOLDO2_LPMODE_PSM_CTL_Msk;
    }
    else
    {
        HWREGH(0X40000804) = (HWREGH(0X40000804) & 0xE0FF) | (0x1D << 8) | (1 << 5);    //lpmode 3/1.8v
        AONPRCM->CORE_PWR_CTRL3 = (core_pwr_ctrl3 & 0x0F) | IOLDO2_PD_ANY_CTL_Msk;
    }

    //normal >> lp
    if((core_pwr_ctrl3 & IOLDO2_NORMAL_CTL_Msk)  &&  (!(IOLDO_ModeCtl & IOLDO2_NORMAL_Enable)))
    {
        utc_cnt_delay(4);   //等待normal lp切换完成，不然可能会影响AT的收发，状态异常
    }
}

/**
 * @brief Pwr cfg.
 * 
 * @param PWR_ModeCtl 
 *   This parameter can be one of the following values:
       
 *     @arg PERI1_PWR
 *     @arg PERI2_PWR
 *     @arg APCORE_PWR_IN_DEEPSLEEP
 *     @arg CPCORE_PWR_IN_DEEPSLEEP
 *     @arg MSYS_PWR_IN_DEEPSLEEP
 *     @arg BBSYS_PWR
 */
void PWRCFG_Mode_On(uint32_t PWR_ModeCtl)
{
    COREPRCM->PWRCTL_CFG &= ~PWR_ModeCtl;
}
/**
 * @brief Pwr cfg.
 * 
 * @param PWR_ModeCtl 
 *   This parameter can be one of the following values:
       
 *     @arg PERI1_PWR
 *     @arg PERI2_PWR
 *     @arg APCORE_PWR_IN_DEEPSLEEP
 *     @arg CPCORE_PWR_IN_DEEPSLEEP
 *     @arg MSYS_PWR_IN_DEEPSLEEP
 *     @arg BBSYS_PWR
 */
void PWRCFG_Mode_Off(uint32_t PWR_ModeCtl)
{
	COREPRCM->PWRCTL_CFG |= PWR_ModeCtl;
}
/**
 * @brief Flash pwr cfg.
 * 
 */
void PRCM_FLASH_VCC_Off(void)
{
    AONPRCM->FLASHPMUX_TRIG = 0;
    AONPRCM->FLASHPMUX_CFG = (AONPRCM->FLASHPMUX_CFG & ~0x07) | 0x04;//flashmux off
    AONPRCM->FLASHPMUX_TRIG = 1;
}
void PRCM_FLASH_VCC_On(void)
{
    AONPRCM->FLASHPMUX_TRIG = 0;
    AONPRCM->FLASHPMUX_CFG = (AONPRCM->FLASHPMUX_CFG & ~0x07) | 0x02;//flashmux ioldo2
    AONPRCM->FLASHPMUX_TRIG = 1;
}

void PRCM_IOLDO1_3V(void)
{
    AONPRCM->RST_CTRL1 |= 0x80;//ioldo1 3v,for A1 chip
}
void PRCM_IOLDO1_1p8V(void)
{
    AONPRCM->RST_CTRL1 &= ~0x80;//ioldo1 1.8v,for A1 chip
}
/**
  * @brief  获取ioldo1电压是否为3V
  * @return Register status
  */
inline uint8_t PRCM_IOLDO1_IsActiveFlag_3V(void)
{
    return (AONPRCM->RST_CTRL1 & 0x80);
}
/**
  * @brief 调节监测VBAT的门限电压
  * @return None
  */
inline void PRCM_IOLDO1_VBAT_DET_REF_Set(void)
{
    HWREGB(0x40004802) |= (HWREGB(0x40004802) & (~0x03)) | 0x02; //i_ioldo1_vbat_det_ref_sel  
}
/**
  * @brief 使能ioldo自动bypass
  * @return None
  */
inline void PRCM_IOLDO1_Enable_AutoByp(void)
{
    HWREGB(0x40000803) |= 0x40; //i_ioldo1_autobyp_en  
}	
/**
  * @brief  获取ioldo1_byp_flag
  * @return This parameter can be one of the following values
  *   @arg  IOLDO1_Bypss_Flag	
      @arg  IOLDO1_Bypss_NoFlag	
  */
inline uint8_t PRCM_IOLDO1_BypFlag_Get(void)
{
    return (HWREGB(0x40000821) & 0x01); //i_ioldo1_autobyp_en  
}	
/**
  * @brief 使能ioldo PSM bypass
  * @return None
  */
inline void PRCM_IOLDO1_Enable_PSM_Byp(void)
{
	HWREGB(0x40000820) = (HWREGB(0x40000820) & (~0x10)) | 0x10;
}
/**
  * @brief 失能ioldo PSM bypass
  * @return None
  */
inline void PRCM_IOLDO1_Disable_PSM_Byp(void)
{
	HWREGB(0x40000820) &= (~0x10);
}

/**
  * @brief 关掉simvcc
  * @return None
  */
void Simvcc_Poweroff(void)
{
    HWREGB(ISO7816_BASE + 0x00) |= 0x04;//iso7816 reset
    AONPRCM->ISO7816_VDD_CTL = 0;//software ctl
    //ioldo2 off
    //AONPRCM->CORE_PWR_CTRL3 &= 0x0F;
    //AONPRCM->PWRCTL_TEST3 = (AONPRCM->PWRCTL_TEST3 & 0xF0) | 0x01;
    //utc_cnt_delay(3);
    // AONPRCM->PWRMUX1_TRIG = 0;
    // AONPRCM->PWRMUX1_CFG = (AONPRCM->PWRMUX1_CFG & 0xcf) | 0x20;
    // AONPRCM->PWRMUX1_BYP &= ~0x02; 
    // AONPRCM->PWRMUX1_TRIG = 1;

    AONPRCM->PWRMUX2_TRIG = 0;
    AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x07) | 0x04;//pmux2 off
    AONPRCM->PWRMUX2_TRIG = 1;
}



/**
  * @brief CP下电
  * @return None
  */
void PRCM_ForceCPOff_Enable(void)
{
    AONPRCM->PWRCTL_TEST7 |= 0x04;//cp domain iso
	prcm_delay(10);//don't delete
	AONPRCM->PWRCTL_TEST7 |= 0x08;//cp domain gate
}

/**
  * @brief CP上电
  * @return None
  */
void PRCM_ForceCPOff_Disable(void)
{
    AONPRCM->PWRCTL_TEST7 &= ~0x08;//cp domain gate=0
	prcm_delay(10);//don't delete
	AONPRCM->PWRCTL_TEST7 &= ~0x04;//cp domain iso=0
    prcm_delay(10);
}

void LPLDO_Load_On(void)
{
    /*在vddaon选择非LPLDO时，在LPLDO的负载上接一个约100uA的负载，当切换到LPLDO时，关闭该负载。需要在切换mux之前0.8ms以上，将aon cntl<6>置1，在唤醒后
    要即时把这个信号置为0，避免漏电*/
    HWREGB(0x40000800) |= 0x40;//aon_cntl[6]=1,LPLDO 100uA
}

void LPLDO_Load_Off(void)
{
    /*在vddaon选择非LPLDO时，在LPLDO的负载上接一个约100uA的负载，当切换到LPLDO时，关闭该负载。需要在切换mux之前0.8ms以上，将aon cntl<6>置1，在唤醒后 
    要即时把这个信号置为0，避免漏电*/
    HWREGB(0x40000800) &= ~0x40;//aon_cntl[6]=1,LPLDO 100uA
}
