#ifndef __XINYI2100_H_
#define __XINYI2100_H_

#ifdef __cplusplus
 extern "C" {
#endif



/**
 * @brief Configuration of the Cortex-M3 Processor and Core Peripherals
 */
#define __MPU_PRESENT             1 /*!< xinyi devices provide an MPU */
#define __NVIC_PRIO_BITS          3 /*!< xinyi uses 3 Bits for the Priority Levels    */
#define __Vendor_SysTickConfig    0 /*!< Set to 1 if different SysTick Config is used */

#define	NVIC_INT_NEST_DISABLE     1 /*!< Set to 1 disable int nest */


/**
 * @brief xinyi Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */
typedef enum IRQn
{
/******  Cortex-M3 Processor Exceptions Numbers ***************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2  Non Maskable Interrupt                            */
  HardFault_IRQn              = -13,    /*!< 3  Cortex-M3 Hard Fault Interrupt                    */
  MemoryManagement_IRQn       = -12,    /*!< 4  Cortex-M3 Memory Management Interrupt             */
  BusFault_IRQn               = -11,    /*!< 5  Cortex-M3 Bus Fault Interrupt                     */
  UsageFault_IRQn             = -10,    /*!< 6  Cortex-M3 Usage Fault Interrupt                   */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M3 SV Call Interrupt                       */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M3 Debug Monitor Interrupt                 */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M3 Pend SV Interrupt                       */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M3 System Tick Interrupt                   */
/******  xinyi specific Interrupt Numbers *********************************************************/

  CLKTIM_IRQn                 = 0,      /*!< 16 xinyi CLK timer Interrupt                         */
  CSP1_IRQn                   = 1,      /*!< 17 xinyi CSP1 Interrupt                              */
  I2C1_IRQn                   = 2,      /*!< 18 xinyi I2C1 Interrupt                              */
  SPI1_IRQn                   = 3,      /*!< 19 xinyi SPI1 Interrupt                              */
  TIM1_IRQn                   = 4,      /*!< 20 xinyi TIM1 Interrupt                              */
  UART2_IRQn                  = 5,      /*!< 21 xinyi UART1 Interrupt                             */
  UTC_IRQn                    = 6,      /*!< 22 xinyi UTC Interrupt                               */
  LPUART_IRQn                 = 7,      /*!< 23 xinyi LpUART Interrupt                            */
  LPTIM_IRQn                  = 8,      /*!< 24 xinyi LpTimer Interrupt                           */
  MCNT_IRQn                   = 9,      /*!< 25 xinyi MEASURECNT Interrupt                        */
  TIM2_IRQn                   = 10,     /*!< 26 xinyi TIM2 Interrupt                              */
  CSP2_IRQn                   = 11,     /*!< 27 xinyi CSP2 Interrupt                              */
  LPMODE_IRQn                 = 12,     /*!< 28 xinyi break lpmode Interrupt                      */
  HRC_MISS_IRQn               = 13,     /*!< 29 xinyi hrc miss Interrupt                          */
  ROOTCLK_XTAL_IRQn           = 14,     /*!< 30 xinyi ROOTCLK XTAL Interrupt                      */
  ROOTCLK_PLL_IRQn            = 15,     /*!< 31 xinyi ROOTCLK pll Interrupt                       */
  QSPI_IRQn                   = 16,     /*!< 32 xinyi QSPI Interrupt                              */
  SEMAPHORE_IRQn              = 17,     /*!< 33 xinyi SEMAPHORE Interrupt                         */
  TIM3_IRQn                   = 18,     /*!< 34 xinyi TIM3 Interrupt                              */
  TIM4_IRQn                   = 19,     /*!< 35 xinyi TIM4 Interrupt                              */
  DMAC0_IRQn                  = 20,     /*!< 36 xinyi DMAC0 Interrupt                             */
  DMAC1_IRQn                  = 21,     /*!< 37 xinyi DMAC1 Interrupt                             */
  DMAC2_IRQn                  = 22,     /*!< 38 xinyi DMAC2 Interrupt                             */
  DMAC3_IRQn                  = 23,     /*!< 39 xinyi DMAC3 Interrupt                             */
  WAKEUP_IRQn                 = 24,     /*!< 40 xinyi WAKEUP Interrupt                            */
  GPIO_IRQn                   = 25,     /*!< 41 xinyi GPIO Interrupt                              */
  I2C2_IRQn                   = 26,     /*!< 42 xinyi I2C2 Interrupt                              */
  CSP3_IRQn                   = 27,     /*!< 43 xinyi CSP3 Interrupt                              */
  CSP4_IRQn                   = 28,     /*!< 44 xinyi CSP4 Interrupt                              */
  DMAC4_IRQn                  = 29,     /*!< 45 xinyi DMAC4 Interrupt                             */
  DMAC5_IRQn                  = 30,     /*!< 46 xinyi DMAC5 Interrupt                             */
  DMAC6_IRQn                  = 31,     /*!< 47 xinyi DMAC6 Interrupt                             */
  DMAC7_IRQn                  = 32,     /*!< 48 xinyi DMAC7 Interrupt                             */
  WDT_IRQn                    = 33,     /*!< 49 xinyi WDT Interrupt                               */
  CACHE_IRQn                  = 34,     /*!< 50 xinyi CACHE Interrupt                             */
  DFE_IRQn                    = 35,     /*!< 51 xinyi DFE Interrupt                               */
  PHYTMR_IRQn                 = 36,     /*!< 52 xinyi PHYTMR Interrupt                            */
  ADC_IRQn                    = 37,     /*!< 53 xinyi ADC Interrupt                               */
  TRNG_IRQn                   = 38,     /*!< 54 xinyi TRNG Interrupt                              */
  SHA_IRQn                    = 39,     /*!< 55 xinyi SHA Interrupt                               */
  BB_IRQn                     = 40,     /*!< 56 xinyi BB Interrupt                                */
  CMP_IRQn                    = 41,     /*!< 57 xinyi CMP Interrupt                               */
  KEYSCAN_IRQn                = 42,     /*!< 58 xinyi key scan Interrupt                          */
  ISO7816_IRQn                = 43,     /*!< 59 xinyi iso7816 Interrupt                           */
  SSPI_IRQn                   = 44,     /*!< 60 xinyi SSPI Interrupt                              */
  RC32K_IRQn                  = 45,     /*!< 61 xinyi rc 32K calibration Interrupt                */
  SVD_IRQn                    = 46,     /*!< 62 xinyi SVD Interrupt                */
  MAX_IRQn                    = 47,

/*------------------------------------------------------------------------------------------------*/
} IRQn_Type;


#include "core_cm3.h"
#include <stdint.h>
#include "hw_types.h"
#ifdef __CC_ARM
#pragma anon_unions
#endif



typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

typedef enum {DISABLE = 0, ENABLE = !DISABLE} FunctionalState;

typedef enum {ERROR = 0, SUCCESS = !ERROR} ErrorStatus;


/**
  * @}
  */

/** @addtogroup Peripheral_registers_structures
  * @{
  */

/**
  * @brief IPC_SEMA
  */

typedef struct
{
	__IO uint32_t SEMA_SLAVE[64];
	__I  uint64_t MASTER_GNT[8];
	__IO uint32_t CTL;// for reason of avoiding byte-writting-way
	//uint8_t  RESERVED0;
} SEMA_TypeDef;

typedef struct
{

	__IO uint8_t  SLPMODE_CTRL;
	__IO uint8_t  CP_REMAP;
	__IO uint8_t  RST_CTRL;
	__I  uint8_t  UP_STATUS;
	__IO uint8_t  RSTWKP_CFG;
	__I  uint8_t  RSTWKP_DATA;
	__IO uint8_t  RSTWKP_CTRL;
	uint8_t	RESERVED0;

	__I  uint32_t  WAKEUP_STATUS;

	__IO uint8_t  WAKUP_INT0;
	__IO uint8_t  AON_WAKUP_ENA;
	__IO uint16_t WAKUP_CTRL;


	__IO uint32_t  AGPIWKUP_CTRL;

	__IO uint32_t  GPIWKUP_CTRL;


	uint32_t  RESERVED1;

	__IO uint32_t  AON_DLY_CTRL_L;
	__IO uint32_t  AON_DLY_CTRL_H;

	__IO uint32_t  AONCLK_CTRL;

	__I  uint16_t  AONCLK_FLAG;

	__IO uint8_t  AONCLK_INT0;

	uint8_t	RESERVED2;

	__IO uint32_t  SYSCLK_CTRL;

	__IO uint32_t  AONPWR_CTRL;


	__IO uint32_t  SMEM_SLPCTRL;

	__IO uint8_t  CORE_PWR_CTRL0;
	__IO uint8_t  CORE_PWR_CTRL1;
	__IO uint8_t  CORE_PWR_CTRL2;
	__IO uint8_t  CORE_PWR_CTRL3;
	__IO uint8_t  BOOTMODE;
	__IO uint8_t  RST_CTRL1;
	__IO uint8_t  DEBUG_CTRL0;
	__IO uint8_t  DEBUG_CTRL1;

	__IO uint8_t  AONGPREG0;
	__IO uint8_t  AONGPREG1;
	__IO uint8_t  AONGPREG2;
	__IO uint8_t  AONGPREG3;

	__IO uint8_t  PWRCTL_TEST0;
	__IO uint8_t  PWRCTL_TEST1;
	__IO uint8_t  PWRCTL_TEST2;
	__IO uint8_t  PWRCTL_TEST3;

	uint8_t  RESERVED3[2];



	__IO uint8_t  PWRCTL_TEST6;
	__IO uint8_t  PWRCTL_TEST7;

	__I  uint8_t  AGPIO_CTRL0;
	__IO uint8_t  AGPIO_CTRL1;
	__IO uint8_t  AGPIO_CTRL2;
	__IO uint8_t  AGPIO_CTRL3;
	__IO uint8_t  AGPIO_CTRL4;
	__IO uint8_t  AGPIO_CTRL5;
	__IO uint8_t  AGPIO_CTRL6;
	__IO uint8_t  RESERVED4;

	__I  uint32_t  AONSYS_FLAG;

	__IO uint8_t  XTALM_CTL0;
	__IO uint8_t  XTALM_CTL1;
	__IO uint8_t  XTALM_CTL2;
	__IO uint8_t  RESERVED5;

	__IO uint8_t  UTC_PCLKEN_CTRL;
	uint8_t  RESERVED6[3];

	__IO uint32_t AP_WAKEUP_EN;
	__IO uint32_t CP_WAKEUP_EN;

	uint8_t  RESERVED7[4];


	__IO uint8_t  AONPRCM_TEST_MUX;
	__IO uint8_t  AONPRCM_CLK_TEST;
	uint8_t  RESERVED8[2];

	__IO uint8_t  CALIB_CTRL0;
	__IO uint8_t  CALIB_CTRL1;
	uint8_t  RESERVED9[2];

	__IO uint8_t  CALIB_CFG0;
	__IO uint8_t  CALIB_CFG1;
	__IO uint8_t  CALIB_CFG2;
	__IO uint8_t  CALIB_CFG3;

	__IO uint16_t  OSC_REF;

	__IO uint16_t  OSC_CAL;

	__IO uint16_t  LPTS_RESO;


	__IO uint8_t  TEMP_STEP;
	__IO uint8_t  TEMP_ADDR;
	__IO uint8_t  CALIB_CAPSEL;
	__IO uint8_t  CALIB_SDM_DATA;
	__IO uint8_t  FORCE_CALIB_CTRL0;
	__IO uint8_t  FORCE_CALIB_CTRL1;
	__O  uint8_t  CALIB_DONE;
	uint8_t  RESERVED10;

	__IO uint16_t  OSC_PTAT0;

	__O  uint8_t  TEMP_FORCE_WE;
	__O  uint8_t  TEMP_FORCE_RE;
	uint8_t	RESERVED11[2];

	__I  uint8_t  MEM_CALIB_CAPSEL;
	__I  uint8_t  MEM_CALIB_SDM_DATA;
	__I  uint8_t  TEMP_CODE;
	__IO uint8_t  TEMP_CODE_VLD_CLR;

	__IO uint8_t  CALIB_INT_STATUS;
	uint8_t	RESERVED12[7];

	__IO uint8_t  PWRMUX_CTRL;
	uint8_t	RESERVED13[3];



	__IO uint8_t  ISO7816_VDD_CTL;
	__IO uint8_t  TCXO_PU_CTL;
	uint8_t	RESERVED14[2];


	__IO uint8_t  LPUA1_CTRL;
	__I  uint8_t  LPUA1_CLKFLAG;
	uint8_t	RESERVED15[2];


	__IO uint8_t  FLASH_VCC_IO_CTRL;
	__IO uint8_t  USER_VCC_IO_CTRL;
	__IO uint8_t  FLASH_LPOFF_STATUS;
	uint8_t	RESERVED16;

	__IO uint8_t  PWRMUX0_TRIG;
	__IO uint8_t  PWRMUX0_CFG;
	__IO uint8_t  PWRMUX0_BYP;
	uint8_t	RESERVED17;

	__IO uint8_t  PWRMUX1_TRIG;
	__IO uint8_t  PWRMUX1_CFG;
	__IO uint8_t  PWRMUX1_BYP;
	uint8_t	RESERVED18;

	__IO uint8_t  PWRMUX2_TRIG;
	__IO uint8_t  PWRMUX2_CFG;
	__IO uint8_t  PWRMUX2_BYP;
	uint8_t	RESERVED19;

	__IO uint16_t  PWRMUX_PRECTL0;

	uint8_t	RESERVED20[2];


	__IO uint8_t  PWRMUX0_SET0;
	__IO uint8_t  PWRMUX0_SET1;
	__IO uint8_t  PWRMUX1_SET0;
	__IO uint8_t  PWRMUX1_SET1;
	__IO uint8_t  PWRMUX2_SET0;
	__IO uint8_t  PWRMUX2_SET1;
	__I  uint16_t  FLASHPMUX_SET;

	__IO uint32_t  SVD_CFG;

	__IO uint32_t  SVD_THRES_CTRL;

	__IO uint16_t  SVD_CTRL;

	__IO uint8_t  SVD_INT_STAT;
	__IO uint8_t  SVD_INT_CTRL;

	__I  uint8_t  SVD_DATA;
	uint8_t	RESERVED21[3];

	__IO uint8_t  LPTMR_IOCTRL;
	uint8_t	RESERVED22[3];

	__IO uint8_t  FLASHPMUX_TRIG;
	__IO uint8_t  FLASHPMUX_CFG;
	__IO uint8_t  FLASHPMUX_BYP;
	uint8_t	RESERVED23;

	__IO uint32_t  SVD_THRES_CTRL2;
	__IO uint8_t  SVD_CTRL2;
	
} AONPRCM_TypeDef;

typedef struct
{
	__I  uint8_t  CTRL0;
	__IO uint8_t  CTRL1;
	__IO uint8_t  CTRL2;
	__IO uint8_t  CTRL3;
	__IO uint8_t  CTRL4;
	__IO uint8_t  CTRL5;
	__IO uint8_t  CTRL6;
	__IO uint8_t  RESERVED;
} AGPIO_TypeDef;

typedef struct
{
	__IO uint32_t  MEASURECTRL;
    __IO uint32_t  CNT32K;
    __I  uint32_t  CNTMEASURE;
    __IO uint32_t  MCNTINT;
    __IO uint32_t  MCLKSRC;
} MCNT_TypeDef;

typedef struct
{
	__O  uint8_t   CTRL0;
	__IO uint8_t   CTRL1;
    __IO uint8_t   RESERVED0[2];
	__IO uint32_t  POLY_COEF;
	__IO uint32_t  INIT;
	__I  uint8_t   STATUS;
    __IO uint8_t   RESERVED1[3];
	__IO uint32_t  XOR;
	__IO uint32_t  RESERVED2[3];
	__IO uint8_t   DATA0;
    __IO uint8_t   DATA1;
    __IO uint8_t   DATA2;
    __IO uint8_t   DATA3;
    __IO uint32_t  RESERVED3[7];
} CRC_TypeDef;

typedef struct
{
    __IO uint8_t   CTRL;
    __IO uint8_t   PRESCALE;
    __IO uint8_t   AP_INT_EN;
    __IO uint8_t   CP_INT_EN;
    __IO uint32_t  AP_RELOAD_SET;
    __IO uint32_t  CP_RELOAD_SET;
    __I  uint8_t   AP_INTSTAT;
    __IO uint8_t   RESERVED0[3];
    __I  uint8_t   CP_INTSTAT;
    __IO uint8_t   RESERVED1[3];
    __IO uint32_t  COUNTER;
    __IO uint32_t  AP_COMPARE;
    __IO uint32_t  CP_COMPARE;
    __I  uint32_t  AP_RELOAD_READ;
    __I  uint32_t  CP_RELOAD_READ;
    __I  uint32_t  CNTREG_WR_DONE;
} TICK_TypeDef;

typedef struct
{
    __IO uint32_t    RESERVED0[64];
    __IO uint32_t    IMR_REG;
    __IO uint32_t    ISR_REG;
    __IO uint32_t    ICR_REG;
    __IO uint32_t    CONFIG_REG;
    __IO uint32_t    VALID_REG;
    __IO uint32_t    EHR_DATA_ADDR_0;
    __IO uint32_t    EHR_DATA_ADDR_1;
	  __IO uint32_t    EHR_DATA_ADDR_2;
    __IO uint32_t    EHR_DATA_ADDR_3;
		__IO uint32_t    EHR_DATA_ADDR_4;
    __IO uint32_t    EHR_DATA_ADDR_5;
    __IO uint32_t    RND_SOURCEE;
    __IO uint32_t    SAMPLE_CNT1;
    __IO uint32_t    AUTOCORR_STATISTIC;
    __IO uint32_t    DEBUG_CONTROL;
    __IO uint32_t    RESERVED3;
    __IO uint32_t    SW_RESET;
    __IO uint32_t    RESERVED4[32];
    __IO uint32_t    CLK_ENABLE;
} TRNG_TypeDef;

typedef struct
{
	__IO uint32_t CTRL;
    __IO uint32_t HOUR_MODE;
    __IO uint32_t TIMER;
    __IO uint32_t CAL;
    __IO uint32_t ALARM_TIMER;
    __IO uint32_t ALARM_CAL;
    __IO uint32_t ALARM_EN;
    __IO uint32_t INT_STAT;
    __O  uint32_t INT_EN;
    __O  uint32_t INT_DIS;
    __IO uint32_t INT_MSK;
    __IO uint32_t STATUS;
    __IO uint32_t KEEP_UTC;
    __IO uint32_t CLK_CNT;
    __IO uint32_t ALARM_CLK_CNT;
    __IO uint32_t RESERVED0;
    __IO uint32_t SEL_32768;
    __IO uint32_t WDT_CTRL;
    __IO uint32_t WDT_TICK_CONFIG;
    __IO uint32_t WDT_TIMER_DATA;
    __IO uint32_t WDT_CALENDAR_DATA;
    __O  uint32_t WDT_INT_STAT;

} UTC_TypeDef;

typedef struct
{
    __IO uint8_t   CTRL0;
    __IO uint8_t   CTRL1;
    __IO uint8_t   UVOL;
    __IO uint8_t   FCR;
    __IO uint32_t  COM0LOW;
	  __IO uint32_t  COM0HIGH;
    __IO uint32_t  COM1LOW;
	  __IO uint32_t  COM1HIGH;
    __IO uint32_t  COM2LOW;
	  __IO uint32_t  COM2HIGH;
    __IO uint32_t  COM3LOW;
	  __IO uint32_t  COM3HIGH;
    __IO uint32_t  COM4LOW;
	  __IO uint32_t  COM4HIGH;
    __IO uint32_t  COM5LOW;
	  __IO uint32_t  COM5HIGH;
    __IO uint32_t  COM6LOW;
	  __IO uint32_t  COM6HIGH;
    __IO uint32_t  COM7LOW;
	  __IO uint32_t  COM7HIGH;
    __IO uint8_t   COM_SEG_MUX;
    __IO uint8_t   RESERVED0[3];
    __I  uint8_t   UPDATE_DONE;
    __IO uint8_t   RESERVED1[3];
    __I  uint8_t   FRAME_FIN;
    __IO uint8_t   RESERVED2[3];
    __I  uint8_t   BUSY;
    __IO uint8_t   RESERVED3[3];
    __O  uint8_t   LCDC_EN;
    __IO uint8_t   RESERVED4[3];
    __O  uint8_t   UPDATE_REQ;
    __IO uint8_t   RESERVED5[3];
	__IO uint8_t   VBATENFORCE;
	__IO uint8_t   ROSCCNTL;
	__IO uint8_t   RESERVED6[2];
	__IO uint32_t  LCD_SEG_PAD_CTRL;
	__IO uint8_t   LCD_COM_PAD_CTRL;
	__IO uint8_t   RESERVED7[9];
} LCDC_TypeDef;

typedef struct
{
	__IO uint32_t  CFG;

	__IO uint32_t  THRES_CTRL;

	__IO uint16_t  CTRL;

	__IO uint8_t  INT_STAT;
	__IO uint8_t  INT_CTRL;

	__I  uint8_t  DATA;
	uint8_t	RESERVED0[3];
} SVD_TypeDef;

typedef struct
{
	__IO uint32_t  PWRCTL_CFG;

	__IO uint8_t  RST_CTRL0;
	__IO uint8_t  RST_CTRL1;
	__IO uint8_t  RST_CTRL2;
	__IO uint8_t  RST_STAT;

	__IO uint32_t  CKG_CTRL_L;
	__IO uint32_t  CKG_CTRL_H;

	__I  uint32_t  SYSCLK_FLAG;

	__IO uint32_t  SYSCLK_CTRL;

	__I  uint32_t  CHIP_VER;

	__IO uint32_t  WAKUP_STAT;


	__IO uint32_t  WAKUP_ENA;

	__IO uint32_t  DBG_CTRL;

	__IO uint32_t  LPM_CTRL;

	__IO uint32_t  LPM_AP_CTRL;

	__IO uint32_t  LPM_CP_CTRL;

	__IO uint8_t  TMRGATEINSEL;
	uint8_t	RESERVED0;
	__IO uint16_t  TESTMUX;

	__IO uint8_t  CORE_GPR0;
	__IO uint8_t  CORE_GPR1;
	__IO uint8_t  CORE_GPR2;
	__IO uint8_t  CORE_GPR3;

	__I  uint32_t  SYS_IDLE_STATUS;

	__IO uint8_t  RST_CTRL4;
	__IO uint8_t  RST_CTRL5;
	uint8_t RESERVED1[2];

	__IO uint8_t  USER1_VCC_IO_CTRL;
	uint8_t RESERVED2[7];

	__IO uint32_t  CMP_CTRL0;

	__IO uint8_t  CMP_FILTER0;
	__IO uint8_t  CMP_FILTER1;
	__IO uint8_t  CMP_OUT_CTRL;
	__I  uint8_t  CMP_DOUT;
	__IO uint8_t  CMP_INT_CTRL;
	__IO uint8_t  CMP_INT_STAT;
	uint8_t RESERVED3[6];

	__IO uint32_t  KEYSCAN_PAD_CTRL;

	uint8_t RESERVED4[12];

	__IO uint8_t  BBSYS_CLK_CTL0;
	__IO uint8_t  BBSYS_CLK_CTL1;
	uint8_t RESERVED5[2];

	__IO uint8_t  XTALPLL_TEST0;
	__IO uint8_t  XTALPLL_TEST1;
	__IO uint8_t  XTALPLL_TEST2;
	__IO uint8_t  BBPLL_CALCTL;
	__IO uint8_t  ANATRXBG_CTL;
	__I  uint8_t  ANATRXBG_STAT;
	__I  uint8_t  ANAXTALRDY;
	__IO uint8_t  ANAPWR_CTL;


} COREPRCM_TypeDef;

typedef struct
{
	__IO uint32_t  CTRL0;

	__IO uint8_t  FILTER0;
	__IO uint8_t  FILTER1;
	__IO uint8_t  OUT_CTRL;
	__I  uint8_t  DOUT;
	__IO uint8_t  INT_CTRL;
	__IO uint8_t  INT_STAT;
	uint8_t RESERVED[2];

} CMP_TypeDef;

typedef struct
{
	__IO	uint32_t	CON;
	__IO	uint32_t	DAT;
	__I		uint32_t	INT_STATUS;
} WDT_TypeDef;

typedef struct
{
	__IO	uint32_t	CONFIG;
	__IO	uint32_t	INT_STATUS;
	__O		uint32_t	IEN;
	__O 	uint32_t	IDIS;
	__I 	uint32_t	IMASK;
	__IO	uint32_t	ENABLE;
	__IO	uint32_t	DELAY;
	union
	{
		struct
		{
			__O uint8_t TXD_8;               /*!< SPI发送缓冲数据寄存器,8位    */
			uint8_t TXD_RESERVED1[3];
		};
		struct
		{
			__O uint16_t TXD_16;             /*!< SPI发送缓冲数据寄存器,16位 */
			uint16_t TXD_RESERVED2;
		};
		__O uint32_t TXD;                    /*!< SPI发送缓冲数据寄存器,32位 */
	};
	union
	{
		struct
		{
			__I uint8_t RXD_8;               /*!< SPI接收缓冲数据寄存器,8位     */
			uint8_t RXD_RESERVED1[3];
		};
		struct
		{
			__I uint16_t RXD_16;             /*!< SPI接收缓冲数据寄存器,16位 */
			uint16_t RXD_RESERVED2;
		};
		__I uint32_t RXD;                    /*!< SPI接收缓冲数据寄存器,32位 */
	};
	__IO	uint32_t	SIC;
	__IO	uint32_t	TX_THRESH;
	__IO	uint32_t	RX_THRESH;
	__IO	uint32_t	TX_FIFO_OP;
	__IO	uint32_t	RX_FIFO_OP;
	__I 	uint32_t	TX_FIFO_STATUS;
	__I 	uint32_t	RX_FIFO_STATUS;
	__IO	uint32_t    RESERVED0[47];
	__IO	uint32_t	MOD_ID;
} SPI_TypeDef;


typedef struct
{
	__IO	uint32_t	MODE1;
	__IO	uint32_t	MODE2;
	__IO	uint32_t	TX_FRAME_CTRL;
	__IO 	uint32_t	RX_FRAME_CTRL;
	__IO 	uint32_t	TX_RX_ENABLE;
	__IO	uint32_t	INT_ENABLE;
	__IO	uint32_t	INT_STATUS;
	__IO 	uint32_t	PIN_IO_DATA;
	__IO 	uint32_t	SYNC_SLAVE_CFG;
	__IO	uint32_t	AYSNC_PARAM_REG;
	__IO	uint32_t	IRDA_X_MODE_DIV;
	__IO	uint32_t	SM_CFG;
	__IO	uint32_t	CLOCK_DIVISOR;
	__IO	uint32_t    RESERVED0[51];
	__IO	uint32_t	TX_DMA_IO_CTRL;
	__IO	uint32_t	TX_DMA_IO_LEN;
	__IO	uint32_t	TX_FIFO_CTRL;
	__IO	uint32_t	TX_FIFO_LEVEL_CHK;
	__IO	uint32_t	TX_FIFO_OP;
	__I		uint32_t	TX_FIFO_STATUS;
	__O		uint32_t	TX_FIFO_DATA;
	__IO	uint32_t	RESERVED1;
	__IO	uint32_t	RX_DMA_IO_CTRL;
	__IO	uint32_t	RX_DMA_IO_LEN;
	__IO	uint32_t	RX_FIFO_CTRL;
	__IO	uint32_t	RX_FIFO_LEVEL_CHK;
	__IO	uint32_t	RX_FIFO_OP;
	__I		uint32_t	RX_FIFO_STATUS;
	__I		uint32_t	RX_FIFO_DATA;
} CSP_TypeDef;

typedef struct
{
	__IO	uint32_t	CTL;           //00
	__I		uint16_t	STAT;          //04
	__I		uint8_t		TX_FIFO_LEVEL; //06
	__I		uint8_t		RX_FIFO_LEVEL; //07
	__IO	uint16_t	ADDR;          //08
	__IO	uint16_t	CMD_DATA;      //0a
	__IO	uint8_t		HS_SPKLEN;//0c
	__IO	uint8_t 	FS_SPKLEN;//0d
	__IO	uint8_t		TX_FIFO_THD;//0e
	__IO	uint8_t 	RX_FIFO_THD;//0f
	__IO	uint16_t	FS_SCL_HCNT;//10
	__IO	uint16_t	FS_SCL_LCNT;//12
	__IO	uint16_t	HS_SCL_HCNT;//14
	__IO	uint16_t	HS_SCL_LCNT;//16
	__IO	uint8_t		FS_SDA_HOLD;//18
	__IO	uint8_t 	HS_SDA_HOLD;//19
	__IO	uint16_t	RESERVED0;
	__IO	uint8_t		HS_MC;//1C
	__IO	uint8_t 	BUS_IDLE_CNT;//1D
	__IO	uint16_t	TIMEOUT;//1E
	__IO	uint32_t	INT_STAT;//20
	__IO	uint32_t	INT_MASK;//24
	__I		uint32_t	FSM_STAT;//28
	__IO	uint32_t	D_SP_SR;//2C
	__IO	uint8_t		D_BUS_FREE;//30
	__IO	uint8_t 	D_BYTE;//31
}I2C_TypeDef;

typedef struct
{
	__I  uint32_t DIN[2];//0 4
	__IO uint32_t DOUT[2];//8 c
	__IO uint32_t MODE[2];//10 14
	__IO uint32_t PULL_UP[2];//18 1c
	__IO uint32_t PULL_DOWN[2];//20 24
	__IO uint32_t INPUT_EN[2];//28 2c
	__IO uint32_t OPEND_DRAIN[2];//30 34
	__IO uint32_t OUTPUT_EN[2];//38 3c
	__IO uint32_t CTL[2];//40 44
	__IO uint32_t INTEN[2];//48 4c
	__IO uint32_t INTS[2]; // gpio_ints  50 54
	__IO uint32_t INTBE[2]; // gpio_intbe 58 5c
 	__IO uint32_t INTEV[2];// gpio_intev 60 64
	__IO uint32_t RINTS[2];//68 6c
	__IO uint32_t INTM[2];//70 74
	__IO uint32_t PCFT[2];//78 7c
	__I  uint32_t PBLKS[2];//80 84
	__IO uint32_t ANAE[2];//88 8c
	__IO uint32_t DRVCTL[7];//90 94 98 9c a0 a4 a8
	__IO uint32_t PERI[64];//ac ~ 1a8
	uint32_t RESERVED0[1];//1ac
	__IO uint32_t PERILIN[128];//1b0 ~ 3ac
	__IO uint32_t PERILIN_EN[4];//3b0 3b4 3b8 3bc
	__IO uint32_t PERILIN_INV[4];//3c0 3c4 3cb 3cc
	__IO uint32_t PULL_UP_SEL[2];//3d0 3d4
} GPIO_TypeDef;

typedef struct
{
	__IO uint8_t  CTRL;//00
	__IO uint8_t  ABDEN;//01
	__IO uint8_t  TRIGGER;//02
	__IO uint8_t  TX_WAIT;//03
	__IO uint32_t BAUDRATE_DIV;//04
	__IO uint16_t INT_MASK;//08
	__IO  uint16_t INT_STA;//0a
	__I  uint8_t  FLAG_STA;//0c
	__IO uint8_t  FLUSH;//0d
	__I  uint8_t  RX_STA;//0e
	__I  uint8_t  TX_STA;//0f
	uint32_t RESERVED0[2];
	__IO uint32_t WAKEUP;//18
	__IO uint8_t  SEQUENCE_DETECT_CFG0;//1c
	__IO uint8_t  SEQUENCE_DETECT_CFG1;//1d
	__IO uint8_t  SEQUENCE_DETECT_CFG2;//1e
	uint8_t RESERVED2;
	__IO uint8_t  FLOW_CTL0;//20
	__IO uint8_t  FLOW_CTL1;//21
    uint8_t RESERVED3[2];
	__I  uint8_t  RX_BYTES;//24
	uint8_t RESERVED4;
	__I  uint8_t  TX_BYTES;//26
	uint8_t RESERVED5;
	__IO uint16_t RX_TIMEOUT_CFG;//28
    uint8_t RESERVED6[2];
	__IO uint16_t START_OFFSET_CFG;//2c
    uint8_t RESERVED7[18];
	__I  uint8_t  READ;//40
	uint32_t RESERVED8[15];
	__O  uint8_t  WRITE;//80
} UART_TypeDef;

typedef struct
{
	__IO uint8_t MUXCFG;
	uint8_t RESERVED0[3];
} DMA_ChMuxTypeDef;

typedef struct
{
	__IO uint8_t INT_STA;
	uint8_t  RESERVED0[3];
	__IO uint8_t PER_REQ;
	uint8_t  RESERVED1;
	__IO uint8_t ARBITRATE;
	uint8_t  RESERVED2;
	uint32_t RESERVED3[126];
	DMA_ChMuxTypeDef CH_MUX[8];
	__I  uint16_t REQ_PEND;
	uint16_t RESERVED4;
} DMA_TypeDef;

typedef struct
{
	__IO uint32_t SRC_ADDR;
	__IO uint32_t DST_ADDR;
	__IO uint32_t CTL;
	__IO uint32_t TRANS_COUNT;
	__IO uint32_t NEXT_POINT;
} DMA_ChannelTypeDef;

typedef struct
{
	__IO uint16_t HWD_CFG;
	uint8_t RESERVED0[14];
	__IO uint8_t  ABD_CTL;
	uint8_t RESERVED1[3];
	__I  uint8_t  ABD_STA;
	uint8_t RESERVED2[3];
	__I  uint16_t ABD_COUNTL;
	uint16_t RESERVED3;
	__I  uint16_t ABD_COUNTH;
	uint16_t RESERVED4;
} ABDHWD_TypeDef;

typedef struct
{
	__IO uint32_t COUNT;
	__IO uint32_t RELOAD;
	__IO uint32_t PWM;
	__IO uint16_t CTL;
	uint16_t RESERVED0;
	__IO uint32_t OFFSET;
} TMR_TypeDef;

typedef struct
{
	__IO uint16_t COUNT;
	uint16_t RESERVED0;
	__IO uint16_t RELOAD;
	uint16_t RESERVED1;
	__IO uint16_t PWM;
	uint16_t RESERVED2;
	__IO uint32_t CTL;
	__IO uint16_t CONFIG;
	__I  uint8_t  STATUS;
} LPTMR_TypeDef;

typedef struct
{
	__IO uint32_t CTL;
	__I  uint8_t  STA;
	__I  uint8_t  DONE;
	__I  uint8_t  S3G_ZUC_STATUS;
	uint8_t RESERVED;
	__O  uint32_t KEY[8];
	__IO uint32_t IV[4];
	__IO uint32_t ADDLEN[2];
	__IO uint32_t PLEN;
	__I  uint32_t TAG[4];
} AES_TypeDef;

/**
 * @brief Configuration of AP or CP Processor and Core Peripherals
 */

//#define PLATFORM_M3_AP         1

/**
  * @}
  */

/** @addtogroup Peripheral_memory_map
  * @{
  */
#ifdef PLATFORM_M3_AP
#define	AP_BOOTROM_BASE			0x00000000	// AP BootROM
#define AP_TCM0_BASE			0x01000000  // TCM0 for AP
#define AP_TCM1_BASE			0x01010000  // TCM1 for AP
#define CP_TCM0_BASE			0x20020000  // TCM0 for CP
#define CP_TCM1_BASE			0x20030000  // TCM1 for CP
#endif

#ifdef PLATFORM_M3_CP
#define	AP_BOOTROM_BASE			0x20100000	// AP BootROM
#define AP_TCM0_BASE			0x20000000  // TCM0 for AP
#define AP_TCM1_BASE			0x20010000  // TCM1 for AP
#define CP_TCM0_BASE			0x00000000  // TCM0 for CP
#define CP_TCM1_BASE			0x00010000  // TCM1 for CP
#endif

#define SHARE_RAM0_BASE			0x60000000  // Share Ram0 for AP & CP
#define SHARE_RAM1_BASE			0x60010000  // Share Ram1 for AP & CP

#define QSPI_DATA_BASE          0x30000000  // QSPI Flash Data
#define QSPI_DATA_LIMITED		0x40000000	// QSPI Flash Data Limited

#define AES_DATA_IN_BASE	    0x40020000  // AES Data In
#define AES_DATA_OUT_BASE	    0x40020010  // AES Data Out
#define DFE_BUFFER_BASE			0x40030010  // DFE Buffer
#define SEMA_BASE               0x40040000  // IPC-Semaphore
#define DMAC_BASE               0x40050000  // DMA Control
#define DMAC1_BASE              0x40058000  // DMA1 Control
#define CRC_BASE                0x40060000  // CRC
#define ADC_REG_BASE			0x40070000	// AUX ADC
#define SHA_BASE				0x40080000	// SHA
#define BB_BUFFER_BASE			0x50000000	// BB Buffer

// AHB2APB Bridge 1
#define AONPRCM_BASE			0x40000000  // AON PRCM
#define AONPRCM_ADIF_BASE		0x40000800  // AON ADIF PRCM     //nrx 自CP侧同步过来
#define AGPIO_BASE				0x4000004C  // AONGPIO
#define SVD_BASE				0x400000C0  // SVD

#define UTC_BASE                0x40001000  // UTC
#define UART1_BASE              0x40002000  // LPUART
#define LPTIMER1_BASE           0x40003000  // Low Power TIMER1
#define LPTIMER2_BASE           0x40003040  // Low Power TIMER2
#define COREPRCM_BASE			0x40004000  // Core PRCM
#define COREPRCM_ADIF_BASE      0x40004800
#define CMP_BASE				0x4000404C  // CMP

#define GPIO_BASE               0x40005000  // GPIO
#define I2C1_BASE               0x40006000  // IIC1
#define CSP1_BASE               0x40007000  // CSP1
#define QSPI_BASE               0x40008000  // QSPI Flash Control
#define KEYSCAN_BASE            0x40009000  // KEYSCAN
#define TIMER1_BASE             0x4000A000  // Timer1
#define MCNT_BASE				0x4000B000  // Measure Counter
#define UART2_BASE              0x4000C000  // UART
#define LCDC_BASE               0x4000D000  // LCDC
#define TICK_BASE               0x4000E000  // TICK TIMER

// AHB2APB Bridge 2
#define CSP2_BASE               0x40010000  // CSP2
#define ISO7816_BASE			0x40011000  // ISO7816
#define SPI_BASE                0x40012000  // SPI
#define CSP3_BASE               0x40013000  // CSP3
#define CSP4_BASE               0x40014000  // CSP4
#define TIMER3_BASE             0x40015000  // Timer3
#define TIMER4_BASE             0x40016000  // Timer4
#define AES_BASE                0x40017000  // AES REG
#define I2C2_BASE               0x40018000  // IIC2
#define TIMER2_BASE             0x40019000  // Timer2
#define DFE_REG_BASE			0x4001A000	// DFE REG
#define DFE_ADIF_BASE			0x4001A200	// DFE REG
#define BB_REG_BASE				0x4001B000	// BB REG
#define TRNG_BASE               0x4001C000  // TRNG
#define PHYTIMER_BASE 			0x4001D000	// Phytimer
#define ABDHWD_BASE             0x4001E000  // ABDHWD

#define AP_CACHE_BASE           0xA0000000  // AP Cache
#define AP_WDT_BASE             0xA0008000  // WatchDog AP
#define CP_CACHE_BASE           0xA0010000  // CP Cache
#define CP_WDT_BASE             0xA0018000  // WatchDog CP


#define MEM_TYPE_UNKNOWN        0x00
#define MEM_TYPE_REG            0x01
#define MEM_TYPE_RAM            0x02
#define MEM_TYPE_FLASH          0x03

//*****************************************************************************
//
// The following are defines the address transformation for AP and CP
// accessing memory through DMA (including CP remap).
//
//*****************************************************************************
//DMA
#define RAM0_DMA_BASE          0x20000000
#define RAM1_DMA_BASE          0x20020000

//AP Core
#define TCM_AP_BASE            0x01000000        //TCM for AP_Core
#define TCM_AP_SIZE            0x00020000        //32KB * 2 * 2
#define TCM_AP_LIMITED         (TCM_AP_BASE + TCM_AP_SIZE)
#define AP_TO_DMA_OFFSET       (RAM0_DMA_BASE - TCM_AP_BASE)

//TCM for CP Core
#define TCM_CP_BASE            0x00000000        //TCM for CP_Core
#define TCM_CP_SIZE            0x00028000        //32KB * 2 * 2
#define TCM_CP_LIMITED         (TCM_CP_BASE + TCM_CP_SIZE)
#define CP_TO_DMA_OFFSET       (RAM1_DMA_BASE - TCM_CP_BASE)

#define TCM_CP_REMAP_BASE      0x10000000
#define TCM_CP_REMAP_LIMITED   (TCM_CP_REMAP_BASE + TCM_CP_SIZE)
#define TCM_CP_BASE_OFFSET     (TCM_CP_REMAP_BASE - TCM_CP_BASE)

//Flash for CP Core
#define FLASH_CP_BASE          0x30000000        //Flash for CP_Core
#define FLASH_CP_SIZE          0x10000000        //256M
#define FLASH_CP_REMAP_BASE    0x00000000
#define FLASH_CP_REMAP_LIMITED (FLASH_CP_REMAP_BASE + FLASH_CP_SIZE)

#define FLASH_CP_BASE_OFFSET   (FLASH_CP_BASE - FLASH_CP_REMAP_BASE)

//#include "hw_memmap.h"

/**
  * @}
  */

/** @addtogroup Peripheral_declaration
  * @{
  */

#define SEMA					((SEMA_TypeDef *) SEMA_BASE)
#define AONPRCM                	((AONPRCM_TypeDef *) AONPRCM_BASE)
#define AGPIO                	((AGPIO_TypeDef *) AGPIO_BASE)
#define SVD                		((SVD_TypeDef *) SVD_BASE)
#define MCNT                	((MCNT_TypeDef *) MCNT_BASE)
#define UTC                  	((UTC_TypeDef *) UTC_BASE)
#define CRC                  	((CRC_TypeDef *) CRC_BASE)
#define TICK                  	((TICK_TypeDef *) TICK_BASE)
#define TRNG                  	((TRNG_TypeDef *) TRNG_BASE)
#define COREPRCM                ((COREPRCM_TypeDef *) COREPRCM_BASE)
#define LCDC                    ((LCDC_TypeDef *) LCDC_BASE)
#define AP_WDT					((WDT_TypeDef *) AP_WDT_BASE)
#define CP_WDT					((WDT_TypeDef *) CP_WDT_BASE)
#define SPI						((SPI_TypeDef *) SPI_BASE)
#define CSP1					((CSP_TypeDef *) CSP1_BASE)
#define CSP2					((CSP_TypeDef *) CSP2_BASE)
#define CSP3					((CSP_TypeDef *) CSP3_BASE)
#define CSP4					((CSP_TypeDef *) CSP4_BASE)
#define I2C1					((I2C_TypeDef *) I2C1_BASE)
#define I2C2					((I2C_TypeDef *) I2C2_BASE)
#define GPIO                    ((GPIO_TypeDef *) GPIO_BASE)
#define LPUART                  ((UART_TypeDef *) UART1_BASE)
#define UART                    ((UART_TypeDef *) UART2_BASE)
#define AES                     ((AES_TypeDef *) AES_BASE)

#ifdef DMA_NEW_DRIVER
#define DMA                     ((DMA_TypeDef *) DMAC_BASE)
#define DMA_Channel0            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0x20))
#define DMA_Channel1            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0x40))
#define DMA_Channel2            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0x60))
#define DMA_Channel3            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0x80))
#define DMA_Channel4            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0xA0))
#define DMA_Channel5            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0xC0))
#define DMA_Channel6            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0xE0))
#define DMA_Channel7            ((DMA_ChannelTypeDef *) (DMAC_BASE + 0x100))
#endif

#define ABDHWD                 ((ABDHWD_TypeDef *) ABDHWD_BASE)
#define TMR1                   ((TMR_TypeDef *) TIMER1_BASE)
#define TMR2                   ((TMR_TypeDef *) TIMER2_BASE)
#define TMR3                   ((TMR_TypeDef *) TIMER3_BASE)
#define TMR4                   ((TMR_TypeDef *) TIMER4_BASE)
#define LPTMR1                 ((LPTMR_TypeDef *) LPTIMER1_BASE)
#define LPTMR2                 ((LPTMR_TypeDef *) LPTIMER2_BASE)
#define ADCCTRL                ((ADCCTRL_TypeDef *)ADC_REG_BASE)

#include "driver_conf.h"

#ifdef __cplusplus
 }
#endif

#endif	/* end of __XINYI2100_H_ */
