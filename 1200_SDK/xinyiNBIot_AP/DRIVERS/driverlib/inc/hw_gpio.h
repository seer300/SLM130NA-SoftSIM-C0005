#ifndef __HW_GPIO_H__
#define __HW_GPIO_H__

#include "xinyi2100.h"

//*****************************************************************************
//
// The following are defines for the GPIO register offsets.
//
//*****************************************************************************
#define GPIO_DIN0                       (GPIO_BASE + 0x00000000)
#define GPIO_DIN1                       (GPIO_BASE + 0x00000004)
#define GPIO_DOUT0                      (GPIO_BASE + 0x00000008)
#define GPIO_DOUT1                      (GPIO_BASE + 0x0000000C)
#define GPIO_CTRL0                      (GPIO_BASE + 0x00000010)
#define GPIO_CTRL1                      (GPIO_BASE + 0x00000014)
#define GPIO_PULLUP0                    (GPIO_BASE + 0x00000018)
#define GPIO_PULLUP1                    (GPIO_BASE + 0x0000001C)
#define GPIO_PULLDOWN0                  (GPIO_BASE + 0x00000020)
#define GPIO_PULLDOWN1                  (GPIO_BASE + 0x00000024)
#define GPIO_INPUTEN0                   (GPIO_BASE + 0x00000028)
#define GPIO_INPUTEN1                   (GPIO_BASE + 0x0000002C)
#define GPIO_OD0                        (GPIO_BASE + 0x00000030)
#define GPIO_OD1                        (GPIO_BASE + 0x00000034)
#define GPIO_OUTPUTEN0                  (GPIO_BASE + 0x00000038)
#define GPIO_OUTPUTEN1                  (GPIO_BASE + 0x0000003C)
#define GPIO_CFGSEL0                    (GPIO_BASE + 0x00000040)
#define GPIO_CFGSEL1                    (GPIO_BASE + 0x00000044)
#define GPIO_INTEN0                     (GPIO_BASE + 0x00000048)
#define GPIO_INTEN1                     (GPIO_BASE + 0x0000004C)
#define GPIO_INT_TYPE0                  (GPIO_BASE + 0x00000050)
#define GPIO_INT_TYPE1                  (GPIO_BASE + 0x00000054)
#define GPIO_INT_EDGE_TYPE0             (GPIO_BASE + 0x00000058)
#define GPIO_INT_EDGE_TYPE1             (GPIO_BASE + 0x0000005C)
#define GPIO_INT_SINGLE_EDGE_TYPE0      (GPIO_BASE + 0x00000060)
#define GPIO_INT_SINGLE_EDGE_TYPE1      (GPIO_BASE + 0x00000064)
#define GPIO_INT_STAT0                  (GPIO_BASE + 0x00000068)
#define GPIO_INT_STAT1                  (GPIO_BASE + 0x0000006C)
#define GPIO_INT_MASK0                  (GPIO_BASE + 0x00000070)
#define GPIO_INT_MASK1                  (GPIO_BASE + 0x00000074)
#define GPIO_PAD_CFT0                   (GPIO_BASE + 0x00000078)
#define GPIO_PAD_CFT1                   (GPIO_BASE + 0x0000007C)
#define GPIO_PAD_BLKS0                  (GPIO_BASE + 0x00000080)
#define GPIO_PAD_BLKS1                  (GPIO_BASE + 0x00000084)
#define GPIO_ANALOG_EN0                 (GPIO_BASE + 0x00000088)
#define GPIO_ANALOG_EN1                 (GPIO_BASE + 0x0000008C)
#define GPIO_DRVCTL0                    (GPIO_BASE + 0x00000090)
#define GPIO_DRVCTL1                    (GPIO_BASE + 0x00000094)
#define GPIO_DRVCTL2                    (GPIO_BASE + 0x00000098)
#define GPIO_DRVCTL3                    (GPIO_BASE + 0x0000009C)
#define GPIO_DRVCTL4                    (GPIO_BASE + 0x000000A0)
#define GPIO_DRVCTL5                    (GPIO_BASE + 0x000000A4)
#define GPIO_DRVCTL6                    (GPIO_BASE + 0x000000A8)
#define GPIO_PAD_SEL0                   (GPIO_BASE + 0x000000AC)
#define GPIO_PAD_SEL63                  (GPIO_BASE + 0x000001A8)
#define GPIO_PERI_IN_SEL0               (GPIO_BASE + 0x000001B0)
#define GPIO_PERI_IN_SEL127             (GPIO_BASE + 0x000003AC)
#define GPIO_PERI_IN_SEL_EN0            (GPIO_BASE + 0x000003B0)
#define GPIO_PERI_IN_SEL_EN1            (GPIO_BASE + 0x000003B4)
#define GPIO_PERI_IN_SEL_EN2            (GPIO_BASE + 0x000003B8)
#define GPIO_PERI_IN_SEL_EN3            (GPIO_BASE + 0x000003BC)
#define GPIO_PERI_IN_INVERT0            (GPIO_BASE + 0x000003C0)
#define GPIO_PERI_IN_INVERT1            (GPIO_BASE + 0x000003C4)
#define GPIO_PERI_IN_INVERT2            (GPIO_BASE + 0x000003C8)
#define GPIO_PERI_IN_INVERT3            (GPIO_BASE + 0x000003CC)

/**
  * @brief  gpio enumeration
  */
typedef enum {
	GPIO_PAD_NUM_0 = 0,
	GPIO_PAD_NUM_1,
	GPIO_PAD_NUM_2,
	GPIO_PAD_NUM_3,
	GPIO_PAD_NUM_4,
	GPIO_PAD_NUM_5,
	GPIO_PAD_NUM_6,
	GPIO_PAD_NUM_7,
	GPIO_PAD_NUM_8,
	GPIO_PAD_NUM_9,
	GPIO_PAD_NUM_10,
	GPIO_PAD_NUM_11,
	GPIO_PAD_NUM_12,
	GPIO_PAD_NUM_13,
	GPIO_PAD_NUM_14,
	GPIO_PAD_NUM_15,
	GPIO_PAD_NUM_16,
	GPIO_PAD_NUM_17,
	GPIO_PAD_NUM_18,
	GPIO_PAD_NUM_19,
	GPIO_PAD_NUM_20,
	GPIO_PAD_NUM_21,
	GPIO_PAD_NUM_22,
	GPIO_PAD_NUM_23,
	GPIO_PAD_NUM_24,
	GPIO_PAD_NUM_25,
	GPIO_PAD_NUM_26,
	GPIO_PAD_NUM_27,
	GPIO_PAD_NUM_28,
	GPIO_PAD_NUM_29,
	GPIO_PAD_NUM_30,
	GPIO_PAD_NUM_31,
	GPIO_PAD_NUM_32,
	GPIO_PAD_NUM_33,
	GPIO_PAD_NUM_34,
	GPIO_PAD_NUM_35,
	GPIO_PAD_NUM_36,
	GPIO_PAD_NUM_37,
	GPIO_PAD_NUM_38,
	GPIO_PAD_NUM_39,
	GPIO_PAD_NUM_40,
	GPIO_PAD_NUM_41,
	GPIO_PAD_NUM_42,
	GPIO_PAD_NUM_43,
	GPIO_PAD_NUM_44,
	GPIO_PAD_NUM_45,
	GPIO_PAD_NUM_46,
	GPIO_PAD_NUM_47,
	GPIO_PAD_NUM_48,
	GPIO_PAD_NUM_49,
	GPIO_PAD_NUM_50,
	GPIO_PAD_NUM_51,
	GPIO_PAD_NUM_52,
	GPIO_PAD_NUM_53,
	GPIO_PAD_NUM_54,
	GPIO_PAD_NUM_55,
	GPIO_PAD_NUM_56,
	GPIO_PAD_NUM_57,
	GPIO_PAD_NUM_58,
	GPIO_PAD_NUM_59,
	GPIO_PAD_NUM_60,
	GPIO_PAD_NUM_61,
	GPIO_PAD_NUM_62,
	GPIO_PAD_NUM_63
} GPIO_PadTypeDef;

//*****************************************************************************
//
// The following values define the number of signal for the peripheral argument to several
// of the APIs.
//
//*****************************************************************************
/**
  * @brief  gpio remapping peripheral enumeration
  */
typedef enum {
	GPIO_UART2_TXD      = 0,
	GPIO_UART2_RTS      = 1,
	GPIO_UART2_RXD      = 2,
	GPIO_UART2_CTS      = 3,
	GPIO_CSP1_SCLK      = 4,
	GPIO_CSP1_TXD       = 5,
	GPIO_CSP1_TFS       = 6,
	GPIO_CSP1_RXD       = 7,
	GPIO_CSP1_RFS       = 8,
	GPIO_CSP2_SCLK      = 9,
	GPIO_CSP2_TXD       = 10,
	GPIO_CSP2_TFS       = 11,
	GPIO_CSP2_RXD       = 12,
	GPIO_CSP2_RFS       = 13,
	GPIO_I2C1_SCL       = 14,
	GPIO_I2C1_SDA       = 15,
	GPIO_SPI_SCLK       = 16,
	GPIO_SPI_MOSI       = 17,
	GPIO_SPI_SS_N       = 18,
	GPIO_SPI_MISO       = 19,
	GPIO_SPI_SS1_N      = 20,
	GPIO_SPI_CS_N       = 21,
	GPIO_TMR1PWM_OUTP   = 22,
	GPIO_TMR1PWM_OUTN   = 23,
	GPIO_TMR1Gate       = 24,
	GPIO_TMR2PWM_OUTP   = 25,
	GPIO_TMR2PWM_OUTN   = 26,
	GPIO_TMR2Gate       = 27,
	GPIO_TMR3PWM_OUTP   = 28,
	GPIO_TMR3PWM_OUTN   = 29,
	GPIO_TMR3Gate       = 30,
	GPIO_TMR4PWM_OUTP   = 31,
	GPIO_TMR4PWM_OUTN   = 32,
	GPIO_TMR4Gate       = 33,
	GPIO_LPTMR1_EXTCLK  = 34,
	GPIO_LPTMR1PWM_OUTP = 35,
	GPIO_LPTMR1PWM_OUTN = 36,
	GPIO_LPTMR1Gate     = 37,
	GPIO_AP_SWCLKTCK    = 38,
	GPIO_AP_SWDIOTMS    = 39,
	GPIO_CP_SWCLKTCK    = 40,
	GPIO_CP_SWDIOTMS    = 41,
	GPIO_UART1_RTS      = 42,
	GPIO_UART1_CTS      = 43,
	GPIO_SM_CLK         = 44,
	GPIO_SM_RST         = 45,
	GPIO_SM_SIO         = 46,
	GPIO_CMP_OUT        = 47,
	GPIO_I2C2_SCL       = 48,
	GPIO_I2C2_SDA       = 49,
	GPIO_CSP3_SCLK      = 50,
	GPIO_CSP3_TXD       = 51,
	GPIO_CSP3_TFS       = 52,
	GPIO_CSP3_RXD       = 53,
	GPIO_CSP3_RFS       = 54,
	GPIO_CSP4_SCLK      = 55,
	GPIO_CSP4_TXD       = 56,
	GPIO_CSP4_TFS       = 57,
	GPIO_CSP4_RXD       = 58,
	GPIO_CSP4_RFS       = 59,
	GPIO_UHWD_RXD       = 60,
	GPIO_UART1_RXD      = 61,
	GPIO_UART1_TXD      = 62,
	GPIO_ABD_RXD        = 63,
	GPIO_LPTMR2_EXTCLK  = 64,
	GPIO_LPTMR2PWM_OUTP = 65,
	GPIO_LPTMR2PWM_OUTN = 66,
	GPIO_LPTMR2Gate     = 67,
	GPIO_SSPI_SCLK      = 68,
	GPIO_SSPI_CS_N      = 69,
	GPIO_SSPI_SI0       = 70,
	GPIO_SSPI_SI1       = 71,
	GPIO_SSPI_SI2       = 72,
	GPIO_SSPI_SI3       = 73,
	GPIO_SSPI_SO        = 74,
	GPIO_SSPI_MCLK      = 75
} GPIO_RemapTypeDef;

typedef enum{
    GPIO_INT_HIGH_LEVEL    = 0x01,   /* 高电平中断 */
    GPIO_INT_RISE_EDGE     = 0x02,   /* 上升沿中断 */
    GPIO_INT_FALL_EDGE     = 0x04,   /* 下降沿中断 */
    GPIO_INT_BOTH_EDGE     = 0x08    /* 双沿中断   */

} GPIO_IntTypeDef;

typedef enum{
    GPIO_MODE_HW_PER        = 0,      /* 硬件外设模式         */
    GPIO_MODE_SW_PER_INPUT  = 0x11,   /* 软件外设输入模式     */
    GPIO_MODE_SW_PER_OD     = 0x12,   /* 软件外设开漏输出模式 */
    GPIO_MODE_SW_PER_PP     = 0x14,   /* 软件外设推挽输出模式 */

    GPIO_MODE_INPUT         = 0x21,   /* 普通GPIO输入模式    */
    GPIO_MODE_OUTPUT_OD     = 0x22,   /* 普通GPIO开漏输出模式 */
    GPIO_MODE_OUTPUT_PP     = 0x24,   /* 普通GPIO推挽输出模式 */
    GPIO_MODE_INOUT         = 0x28,   /* 普通GPIO输入输出模式 */

    GPIO_MODE_HIS           = 0x30    /* GPIO高阻态(用于配置用户不使用的IO) */
} GPIO_ModeTypeDef;

typedef enum{
    GPIO_PULL_UP           = 0x01,
    GPIO_PULL_DOWN         = 0x02,
    GPIO_FLOAT             = 0x03
} GPIO_PullTypeDef;

typedef enum{
    GPIO_DRV_STRENGTH_0         = 0,
    GPIO_DRV_STRENGTH_1         = 1,
    GPIO_DRV_STRENGTH_2         = 2,
    GPIO_DRV_STRENGTH_3         = 3,
    GPIO_DRV_STRENGTH_4         = 4,
    GPIO_DRV_STRENGTH_5         = 5,
    GPIO_DRV_STRENGTH_6         = 6,
    GPIO_DRV_STRENGTH_7         = 7

} GPIO_DrvStrengthTypeDef;



#endif // __HW_GPIO_H__
