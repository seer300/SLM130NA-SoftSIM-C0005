#ifndef __PRCM_H__
#define __PRCM_H__

#include "hw_prcm.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"
#include "factory_nv.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#define LOAD_IMAGE          0x01
#define NOT_LOAD_IMAGE      0x00

#define	CLOCK_SRC_32K		0x00
#define	CLOCK_SRC_HRC		0x01
#define	CLOCK_SRC_XTAL		0x02
#define	CLOCK_SRC_PLL		0x03


#define	AON_HRC_DIV1		0x00
#define	AON_HRC_DIV2		0x01
#define	AON_HRC_DIV4		0x02
#define	AON_HRC_DIV8		0x03
#define	AON_HRC_DIV16		0x04
#define	AON_UTC_CLK		    0x05

#define LPUART1_CLKSRC_AON_HRC      (0x01)
#define LPUART1_CLKSRC_32K          (0x00)

#define	POWERUP_AP_JUMP_ADDR			0x60011FF4		// AP Reset while CP is Active
#define DEEPSLEEP_WAKEUP_AP_JUMP_ADDR	0x60011FF8		// SoC Wakeup from Deepsleep
#define DEEPSLEEP_WAKEUP_CP_ACTION		0x60011FFC		// SoC Wakeup from Deepsleep

//*****************************************************************************
//
// The following are defines for the clock dividing ratio in the AONCLK_CTRL register.
//
//*****************************************************************************
#define AONHRC_CLK_Div1				            ((uint32_t)0<<AONHRC_CLK_DIV_Pos)
#define AONHRC_CLK_Div2				            ((uint32_t)1<<AONHRC_CLK_DIV_Pos)
#define AONHRC_CLK_Div4				            ((uint32_t)2<<AONHRC_CLK_DIV_Pos)
#define AONHRC_CLK_Div8				            ((uint32_t)3<<AONHRC_CLK_DIV_Pos)


#define XTAL_CLK                    26000000
#define	BBPLL_CLK_DEFAULT			368640000


//*****************************************************************************
//
// The following are defines for the clock dividing ratio in the coreprcm SYSCLK_CTRL register.
//
//*****************************************************************************
#define AP_HCLK_Div4				            ((uint32_t)0<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div5				            ((uint32_t)1<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div6				            ((uint32_t)2<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div7				            ((uint32_t)3<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div8				            ((uint32_t)4<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div9				            ((uint32_t)5<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div10				            ((uint32_t)6<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div11				            ((uint32_t)7<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div12				            ((uint32_t)8<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div13				            ((uint32_t)9<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div14				            ((uint32_t)10<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div15				            ((uint32_t)11<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div16				            ((uint32_t)12<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div17				            ((uint32_t)13<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div18				            ((uint32_t)14<<AP_HCLK_DIV_Pos)
#define AP_HCLK_Div19				            ((uint32_t)15<<AP_HCLK_DIV_Pos)

#define CP_HCLK_Div2p5				            ((uint32_t)1<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div3				            ((uint32_t)2<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div3p5				            ((uint32_t)3<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div4				            ((uint32_t)0<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div4p5				            ((uint32_t)4<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div5				            ((uint32_t)5<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div6				            ((uint32_t)6<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div7				            ((uint32_t)7<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div8				            ((uint32_t)8<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div10				            ((uint32_t)9<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div12				            ((uint32_t)10<<CP_HCLK_DIV_Pos)
#define CP_HCLK_Div16				            ((uint32_t)11<<CP_HCLK_DIV_Pos)

#define SYS_HCLK_Div4				            ((uint32_t)0<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div5				            ((uint32_t)1<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div6				            ((uint32_t)2<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div7				            ((uint32_t)3<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div8				            ((uint32_t)4<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div9				            ((uint32_t)5<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div10				            ((uint32_t)6<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div11				            ((uint32_t)7<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div12				            ((uint32_t)8<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div13				            ((uint32_t)9<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div14				            ((uint32_t)10<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div15				            ((uint32_t)11<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div16				            ((uint32_t)12<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div17				            ((uint32_t)13<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div18				            ((uint32_t)14<<SYS_HCLK_DIV_Pos)
#define SYS_HCLK_Div19				            ((uint32_t)15<<SYS_HCLK_DIV_Pos)

#define PERI1_PCLK_Div1				            ((uint32_t)0<<PERI1_PCLK_DIV_Pos)
#define PERI1_PCLK_Div2				            ((uint32_t)1<<PERI1_PCLK_DIV_Pos)
#define PERI1_PCLK_Div3				            ((uint32_t)2<<PERI1_PCLK_DIV_Pos)
#define PERI1_PCLK_Div4				            ((uint32_t)3<<PERI1_PCLK_DIV_Pos)

#define PERI2_PCLK_Div1				            ((uint32_t)0<<PERI2_PCLK_DIV_Pos)
#define PERI2_PCLK_Div2				            ((uint32_t)1<<PERI2_PCLK_DIV_Pos)
#define PERI2_PCLK_Div3				            ((uint32_t)2<<PERI2_PCLK_DIV_Pos)
#define PERI2_PCLK_Div4				            ((uint32_t)3<<PERI2_PCLK_DIV_Pos)

#define SIDO_CLK_Div3				            ((uint32_t)1<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div3p5				            ((uint32_t)2<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div4				            ((uint32_t)0<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div4p5				            ((uint32_t)4<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div5				            ((uint32_t)5<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div6				            ((uint32_t)6<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div7				            ((uint32_t)7<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div8				            ((uint32_t)8<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div10				            ((uint32_t)9<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div12				            ((uint32_t)10<<SIDO_CLK_DIV_Pos)
#define SIDO_CLK_Div16				            ((uint32_t)11<<SIDO_CLK_DIV_Pos)

#define SSPI_CLK_Div3				            ((uint32_t)1<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div3p5				            ((uint32_t)2<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div4				            ((uint32_t)0<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div4p5				            ((uint32_t)4<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div5				            ((uint32_t)5<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div6				            ((uint32_t)6<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div7				            ((uint32_t)7<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div8				            ((uint32_t)8<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div10				            ((uint32_t)9<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div12				            ((uint32_t)10<<SSPI_CLK_DIV_Pos)
#define SSPI_CLK_Div16				            ((uint32_t)11<<SSPI_CLK_DIV_Pos)

#define I2C1_REFCLK_SEL_LSioclk					((uint32_t)0<<I2C1_REFCLK_SEL_Pos)
#define I2C1_REFCLK_SEL_Peri1pclk				((uint32_t)1<<I2C1_REFCLK_SEL_Pos)

#define I2C2_REFCLK_SEL_LSioclk					((uint32_t)0<<I2C2_REFCLK_SEL_Pos)
#define I2C2_REFCLK_SEL_Peri2pclk				((uint32_t)1<<I2C2_REFCLK_SEL_Pos)


#define QSPI_REFCLK_Div2				        ((uint32_t)0<<QSPI_REFCLK_DIV_Pos)
#define QSPI_REFCLK_Div3				        ((uint32_t)1<<QSPI_REFCLK_DIV_Pos)
#define QSPI_REFCLK_Div4				        ((uint32_t)2<<QSPI_REFCLK_DIV_Pos)
#define QSPI_REFCLK_Div5				        ((uint32_t)3<<QSPI_REFCLK_DIV_Pos)

#define TRNG_PCLK_Div1				            ((uint32_t)0<<TRNG_PCLK_DIV_Pos)
#define TRNG_PCLK_Div2				            ((uint32_t)1<<TRNG_PCLK_DIV_Pos)
#define TRNG_PCLK_Div3				            ((uint32_t)2<<TRNG_PCLK_DIV_Pos)
#define TRNG_PCLK_Div4				            ((uint32_t)3<<TRNG_PCLK_DIV_Pos)

//*****************************************************************************
//
// The following are possible sys-up reasons for PRCM_UpStatusGet().
//
//*****************************************************************************
typedef enum{
    UP_STAT_EXTPINT_RST,
    UP_STAT_LVD_RST,
    UP_STAT_UTCWDT_RST,
    UP_STAT_SOFT_RST,
    UP_STAT_POR,
    UP_STAT_DFTGLB_RST,
    UP_STAT_SVD_RST,
    UP_STAT_DEEPSLEEP_WKUP,
    UP_STAT_STANDBY_WKUP
}UP_STAT_TypeDef;

//*****************************************************************************
//
// The following are AON-WAKEUP sources sys-up reasons for PRCM_AonWkupSourceEnable() & PRCM_AonWkupSourceDisable().
//
//*****************************************************************************
typedef enum{
    AON_WKUP_SOURCE_EXTPINT,

    AON_WKUP_SOURCE_UTC,
    AON_WKUP_SOURCE_LPUART1,
    AON_WKUP_SOURCE_SVD,
    AON_WKUP_SOURCE_LPTMR,
    AON_WKUP_SOURCE_LPTMR_ASYNC,
    AON_WKUP_SOURCE_KEYSCAN,
    AON_WKUP_SOURCE_TMR2,
    AON_WKUP_SOURCE_APTICK,
    AON_WKUP_SOURCE_CPTICK,

    AON_WKUP_SOURCE_AGPI0,
    AON_WKUP_SOURCE_AGPI1,
    AON_WKUP_SOURCE_AGPI2,

    AON_WKUP_SOURCE_GPI0,
    AON_WKUP_SOURCE_GPI1,
    AON_WKUP_SOURCE_GPI2,
    AON_WKUP_SOURCE_GPI3,
    AON_WKUP_SOURCE_GPI4,
    AON_WKUP_SOURCE_GPI5,
    AON_WKUP_SOURCE_GPI6,
    AON_WKUP_SOURCE_GPI7,

}AON_WKUP_SOURCE_TypeDef;

//*****************************************************************************
//
// The following are possible sources for sysclk in the SYSCLK_CTRL register.
//
//*****************************************************************************
typedef enum{
    SYSCLK_SRC_32K,
    SYSCLK_SRC_HRC,
    SYSCLK_SRC_XTAL,
    SYSCLK_SRC_PLL,
}SYSCLK_SRC_TypeDef;

//*****************************************************************************
//
// The following are possible sources for LS_IOCLK in the SYSCLK_CTRL register.
//
//*****************************************************************************
typedef enum{
    LS_IOCLK_SRC_AUTO,//automatically select 32k and xtal, if xtal is ready, select xtal, else select 32k
    LS_IOCLK_SRC_32K,
    LS_IOCLK_SRC_XTAL,
    LS_IOCLK_SRC_HRC,
}LS_IOCLK_SRC_TypeDef;

#define AP_SRAM0_BANK0       ((uint32_t)0x00000001)    //TCM0 for AP (Lower 32K)
#define AP_SRAM0_BANK1       ((uint32_t)0x00000002)    //TCM0 for AP (Higher 32K)
#define AP_SRAM0            (AP_SRAM0_BANK0 | AP_SRAM0_BANK1)    //TCM0 for AP (32K*2)

#define AP_SRAM1_BANK0       ((uint32_t)0x00000004)    //TCM1 for AP (Lower 32K)
#define AP_SRAM1_BANK1       ((uint32_t)0x00000008)    //TCM1 for AP (Higher 32K)
#define AP_SRAM1            ((AP_SRAM1_BANK0 | AP_SRAM1_BANK1))    //TCM1 for AP (32K*2)

#define CP_SRAM0       ((uint32_t)0x00000010)    //TCM0 for CP (32K*2)
#define CP_SRAM1       ((uint32_t)0x00000020)    //TCM1 for CP (32K*2)
#define SH_SRAM64K     ((uint32_t)0x00000040)    //Share ram for AP and CP (32K*2)
#define SH_SRAM8K      ((uint32_t)0x00000080)    //Share ram for AP and CP(8K*1)

typedef enum
{
    SRAM_POWER_MODE_OFF_DEEPSLEEP = 0,
    SRAM_POWER_MODE_FORCE_ON,
    SRAM_POWER_MODE_FORCE_OFF
}SRAM_POWER_MODE_TypeDef;
typedef enum
{
    LOW_POWER_MODE_NORMAL = 0,
    //LOW_POWER_MODE_RESERVED,
    LOW_POWER_MODE_DEEPSLEEP = 2,
    LOW_POWER_MODE_STANDBY
}LOW_POWER_MODE_TypeDef;
/**
 * @brief defines for AONGPIO pins
 *
 */
#define AGPIO0 ((uint8_t)0)
#define AGPIO1 ((uint8_t)1)
#define AGPIO2 ((uint8_t)2)

/**
 * @brief defines for AONGPIO WKUP
 *
 */
#define AGPI_POL_CFG_HIGH_LEVEL				((uint32_t)0x00000000)
#define AGPI_POL_CFG_LOW_LEVEL				    ((uint32_t)0x00000002)

#define AGPI_WKUP_CFG_RISING				    ((uint32_t)0x00000000)
#define AGPI_WKUP_CFG_ASYNC				    ((uint32_t)0x00000010)
#define AGPI_WKUP_CFG_FALLING				    ((uint32_t)0x00000020)
#define AGPI_WKUP_CFG_BOTH				        ((uint32_t)0x00000030)

#define AGPI_PLS_CFG_1				        ((uint32_t)0x00000000)
#define AGPI_PLS_CFG_2				        ((uint32_t)0x00000040)
#define AGPI_PLS_CFG_3				        ((uint32_t)0x00000080)
#define AGPI_PLS_CFG_4				        ((uint32_t)0x000000C0)
/**
 * @brief defines for SVD thd
 *
 */
#define SVD_THDMIN    ((uint8_t)0)
#define SVD_THD1      ((uint8_t)1)
#define SVD_THD2      ((uint8_t)2)
#define SVD_THD3      ((uint8_t)3)

/**
 * @brief defines for SVD mode
 *
 */
#define SVD_MODE_DISABLE        ((uint32_t)0)
#define SVD_MODE_NO_FILTER      ((uint32_t)1)
/* filter unit 2.5us for hrc_clk. If use utc_clk, it's 1 utc_clk */
#define SVD_MODE_FILTER_HRC_2p5us      ((uint32_t)2)
#define SVD_MODE_FILTER_UTC_30us      ((uint32_t)2)
/* filter unit 10us for hrc_clk. If use utc_clk, it's 1 utc_clk */
#define SVD_MODE_FILTER_HRC_10us      ((uint32_t)3)

/**
 * @brief defines for SVD filter num
 *
 */
#define SVD_FILTER_NUM_0        ((uint32_t)0<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_6        ((uint32_t)1<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_24       ((uint32_t)2<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_30       ((uint32_t)3<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_96       ((uint32_t)4<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_102      ((uint32_t)5<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_120      ((uint32_t)6<<SVD_FILT_NUM_Pos)
#define SVD_FILTER_NUM_126      ((uint32_t)7<<SVD_FILT_NUM_Pos)

/**
 * @brief defines for SVD period num,it has to be a multiple of 8,period_time = period_num * period_unit
 *  SVD_PERIOD_NUM_0 means continuous detection
 *
 */
#define SVD_PERIOD_NUM_0            ((uint32_t)0<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_8            ((uint32_t)1<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_16           ((uint32_t)2<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_32           ((uint32_t)4<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_64           ((uint32_t)8<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_128          ((uint32_t)16<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_256          ((uint32_t)32<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_512          ((uint32_t)64<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_1024         ((uint32_t)128<<SVD_PRD_CFG_Pos)
#define SVD_PERIOD_NUM_2040         ((uint32_t)255<<SVD_PRD_CFG_Pos)

/**
 * @brief defines for SVD period unit
 *
 */
#define SVD_PRDUNIT_SEL_HRC10us             ((uint32_t)0<<SVD_PRDUNIT_SEL_Pos)
#define SVD_PRDUNIT_SEL_RC32K5ms            ((uint32_t)1<<SVD_PRDUNIT_SEL_Pos)
#define SVD_PRDUNIT_SEL_XTAK32K7p5ms        ((uint32_t)1<<SVD_PRDUNIT_SEL_Pos)

/**
 * @brief defines for SVD sample num
 *
 */
#define SVD_SAMPLE_NUM_1             ((uint32_t)0<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_3             ((uint32_t)1<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_5             ((uint32_t)2<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_7             ((uint32_t)3<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_9             ((uint32_t)4<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_11             ((uint32_t)5<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_13             ((uint32_t)6<<SVD_SAMPLE_NUM_Pos)
#define SVD_SAMPLE_NUM_15             ((uint32_t)7<<SVD_SAMPLE_NUM_Pos)

/**
 * @brief defines for SVD_THDMIN_CFG
 *
 */
#define SVD_THDMIN_CFG_GEN_INT            ((uint32_t)0<<SVD_THDMIN_CFG_Pos)
#define SVD_THDMIN_CFG_GEN_RST            ((uint32_t)1<<SVD_THDMIN_CFG_Pos)

/**
 * @brief defines for SVD_HCPD_CTL,generate to power-off high power domain
 *
 */
#define SVD_HCPD_CTL_Disable            ((uint32_t)0<<SVD_HCPD_CTL_Pos)
#define SVD_HCPD_CTL_Enable            ((uint32_t)1<<SVD_HCPD_CTL_Pos)

/**
 * @brief defines for SVD warm dly
 *
 */
#define SVD_WARM_DLY_0                  ((uint32_t)0<<SVD_WARM_DLY_Pos)
#define SVD_WARM_DLY_20us               ((uint32_t)1<<SVD_WARM_DLY_Pos)
#define SVD_WARM_DLY_40us               ((uint32_t)2<<SVD_WARM_DLY_Pos)
#define SVD_WARM_DLY_80us               ((uint32_t)3<<SVD_WARM_DLY_Pos)

/**
 * @brief defines for SVD thd ref-voltage  min & max,the unit is mv
 *
 */
#define SVD_THD_VSEL_MIN                  ((uint16_t)640)
#define SVD_THD_VSEL_MAX                  ((uint16_t)1260)
#define SVD_THD_VSEL_STEP                  ((uint16_t)20)
#define SVD_THD_CAL_VSEL_STEP                  ((uint16_t)10)

/**
 * @brief defines for SVD_DPSLP_CTL,generate to power-off high power domain
 *
 */
#define SVD_THD1_DPSLP_CTL_Disable            ((uint32_t)0<<SVD_THD1_DPSLP_CTL_Pos)
#define SVD_THD1_DPSLP_CTL_Enable            ((uint32_t)1<<SVD_THD1_DPSLP_CTL_Pos)

/**
 * @brief defines for the voltage source for thd detection
 *
 */
#define SVD_THD_SRC_SEL_SVDPIN            ((uint16_t)0)
#define SVD_THD_SRC_SEL_VBATP             ((uint16_t)1)

extern void prcm_delay(unsigned long uldelay);
extern unsigned char AON_DSP_Load_Flag_Get(void);
extern unsigned char AON_ARM_Load_Flag_Get(void);
extern unsigned char AON_SECONDARY_Load_Flag_Get(void);
extern void AON_DSP_Load_Flag_Set(uint8_t flag);
extern void AON_ARM_Load_Flag_Set(uint8_t flag);
extern void AON_SECONDARY_Load_Flag_Set(uint8_t flag);
extern void AON_Flash_Delay_Get(void);
extern void AON_Flash_Delay_Set(uint8_t value);	

//extern void PRCM_CSPHwDetectEnable(unsigned long ulBase,unsigned char ucValidBits, unsigned char ucCSP, unsigned char ucPattern);
//extern void PRCM_CSPHwDetectDisable(unsigned long ulBase);


extern unsigned char AON_BOOTMODE_GET(void);

extern void AON_CP_Memory_Remap_Enable(void);
extern void AON_CP_Memory_Remap_Disable(void);
extern void AON_CP_Core_Release(void);
extern void AON_CP_Core_Hold(void);

extern void CP_Core_Release(void);
extern void CP_Core_Hold(void);

extern unsigned long AON_UP_STATUS_Get(void);

// System Clock Select
extern void AON_System_Clock_Select(unsigned char ucClkSrc);
extern unsigned long PRCM_CHIP_VER_GET(unsigned long ulBase);

extern unsigned long Core_PRCM_CHIP_VER_GET(void);


// Clock Select
extern void PRCM_Clock_Mode_Force_BBPLL(void);
extern void PRCM_Clock_Mode_Auto(void);

void RF_BBPLL_Freq_Set_PllDivn_recovery(uint32_t pll_divn);
extern void RF_BBPLL_Freq_Set_PllDivn(uint32_t pll_divn);
extern void RF_BBPLL_Freq_Set(uint32_t ulFreqHz);
extern uint8_t RF_BBPLL_Lock_Status_Get(void);

void PhyInitBBPLLClocks(unsigned char ucInitFlag, unsigned int ulInitRegVal);
unsigned int PhyGetBBPLLClocks(void);
void PhyAdjustBBPLLClocks(int sFreqErr, unsigned int ulFrequency);
void PhyAdjustBBPLLClocksByRxSRC(int sFreqErr, unsigned int ulFrequency, int multiBy1p92M);


extern void DFE_BBPLL_Reset_Avoid_Spur(void);
extern void CPUWakeupIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void CPUWakeupIntUnregister(unsigned long *g_pRAMVectors);
extern void ApIntWakeupEnable(void);
extern void ApIntWakeupDisable(void);
extern void CpIntWakeupEnable(void);
extern void CpIntWakeupDisable(void);
extern void AP_CP_int_wakeup(void);
extern void AP_CP_int_clear(void);
extern void CP_AP_int_wakeup(void);
extern void CP_AP_int_clear(void);

extern unsigned char PRCM_BootmodeGet(void);

extern void PRCM_CP_MemoryRemapEnable(void);

extern void PRCM_CP_MemoryRemapDisable(void);
extern void PRCM_CP_CoreRelease(void);

extern void PRCM_CP_CoreHold(void);


extern uint32_t PRCM_UpStatusGet(void);
extern void PRCM_SysclkSrcSelect(uint32_t ClkSrc);
extern uint32_t PRCM_SysclkSrcGet(void);
extern void PRCM_LSioclkSrcSelect(uint32_t ClkSrc);
extern uint32_t PRCM_LSioclkSrcGet(void);

extern void PRCM_AonhrcclkDivSet(uint32_t AonhrcclkDiv);
extern void PRCM_SlowfreqDivEnable(void);
extern void PRCM_SlowfreqDivDisable(void);
extern void PRCM_AphclkDivSet(uint32_t AphclkDiv);
extern void PRCM_CphclkDivSet(uint32_t CphclkDiv);
extern void PRCM_SyshclkDivSet(uint32_t SyshclkDiv);
extern void PRCM_Peri1pclkDivSet(uint32_t PeripclkDiv);
extern void PRCM_Peri2pclkDivSet(uint32_t PeripclkDiv);
extern void PRCM_SSPIclkDivSet(uint32_t SSPIclkDiv);
extern void PRCM_QSPIrefclkDivSet(uint32_t QSPIrefclkDiv);
extern void PRCM_TRNGpclkDivSet(uint32_t TRNGpclkDiv);
extern void PRCM_I2C1refclkSet(uint32_t I2C1refclk);
extern void PRCM_I2C2refclkSet(uint32_t I2C2refclk);
extern void PRCM_ClockEnable(uint64_t ModuleClk);
extern void PRCM_ClockDisable(uint64_t ModuleClk);

extern uint32_t PRCM_ChipVerGet(void);
extern void PRCM_ApIntWakeupEnable(void);
extern void PRCM_ApIntWakeupDisable(void);
extern void PRCM_CpIntWakeupEnable(void);
extern void PRCM_CpIntWakeupDisable(void);
extern void PRCM_ApCpIntWkupTrigger(void);
extern uint32_t PRCM_ApCpIntWkupGet(void);
extern void PRCM_ApCpIntWkupClear(void);
extern void PRCM_CpApIntWkupTrigger(void);
extern uint32_t PRCM_CpApIntWkupGet(void);
extern void PRCM_CpApIntWkupClear(void);
extern void PRCM_AonWkupSourceEnable(uint32_t WkupSrc);
extern void LPmodeWakeupSourceConfig(AON_WKUP_SOURCE_TypeDef WakeupSource);

extern void PRCM_SramRetensionEnable(uint32_t Sramx);
extern void PRCM_SramRetensionDisable(uint32_t Sramx);
extern void PRCM_SramPowerModeSet(uint32_t Sramx,uint32_t SramPowerMode);

extern void PRCM_SlectXtal32k(void);
extern void PRCM_PowerOnRetldo(void);
extern void PRCM_PowerOffRetldo(void);
extern void PRCM_SetWkupAonDly(uint8_t ncorerst_dly,uint8_t sidordy_dly,uint8_t hrc_dly);
extern void PRCM_XtalM_LpcoreSel(void);
extern void PRCM_XtalM_HpcoreSel(void);
extern void PRCM_XtalM_ForceLpcore(void);
extern void PRCM_LPM_LpmCtrlEnable(void);
extern void PRCM_LPM_LpmCtrlDisable(void);
extern void PRCM_LPM_PeriIdleTimeEnable(void);
extern void PRCM_LPM_PeriIdleTimeDisable(void);
extern void PRCM_LPM_PeriIdleTimeSet(uint8_t IdleTimeCnt);
extern void PRCM_LPM_IdleTimeIntClear(void);
extern void PRCM_LPM_ApReqLpMode(uint32_t LowPowerMode);
extern void PRCM_LPM_CpReqLpMode(uint32_t LowPowerMode);

extern void AGPI_WakeupConfig(uint8_t AGPIx,uint32_t WakeupPolarity,uint32_t WakeupEdge,uint32_t WakeupPulse);
extern void AGPI_WakeupEnable(uint8_t AGPIx);
extern void AGPI_WakeupDisable(uint8_t AGPIx);

extern void AGPIO_InputEnable(uint8_t AGPIOx);
extern void AGPIO_InputDisable(uint8_t AGPIOx);

extern void AGPIO_OutputEnable(uint8_t AGPIOx);
extern void AGPIO_OutputDisable(uint8_t AGPIOx);

extern uint8_t AGPIO_Read(uint8_t AGPIOx);

extern void AGPIO_PullupEnable(uint8_t AGPIOx);
extern void AGPIO_PullupDisable(uint8_t AGPIOx);

extern void AGPIO_PulldownEnable(uint8_t AGPIOx);
extern void AGPIO_PulldownDisable(uint8_t AGPIOx);

extern void AGPIO_OpendrainEnable(uint8_t AGPIOx);
extern void AGPIO_OpendrainDisable(uint8_t AGPIOx);

extern void AGPIO_Set(uint8_t AGPIOx);
extern void AGPIO_Clear(uint8_t AGPIOx);
extern void AGPIO_GPIO0_7_RetentionEnable(void);
extern void AGPIO_GPIO0_7_RetentionDisable(void);
extern void AGPIO_GPIO0_7_Update(void);

extern void SVD_Stop(void);
extern void SVD_Config(uint32_t SVD_FilterMode,uint32_t FilterNum,uint32_t PeriodUnit,uint32_t PeriodNUm,uint32_t SampleNum,uint32_t WarmDelay);
extern void SVD_THDMIN_GenerateConfig(uint32_t GenIntOrReset);
extern void SVD_THDMIN_HCPD_Disable(void);
extern void SVD_THDMIN_HCPD_Enable(void);
extern void SVD_THD1_DeepsleepDisable(void);
extern void SVD_THD1_DeepsleepEnable(void);
extern void SVD_THD_VoltageSet(uint8_t SVD_THDx,uint16_t Voltage_mv);
extern void SVD_THD_SourceSel(uint8_t SVD_THDx,uint16_t VoltageSource);
extern uint8_t SVD_IntStatusGetBit(uint8_t SVD_THDx);
extern void SVD_IntStatusClearBit(uint8_t SVD_THDx);
extern void SVD_IntStatusClear(void);
extern uint8_t SVD_IntStatusGet(void);
extern void SVD_IntEnable(uint8_t SVD_THDx);
extern void SVD_IntDisable(uint8_t SVD_THDx);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __PRCM_H__
