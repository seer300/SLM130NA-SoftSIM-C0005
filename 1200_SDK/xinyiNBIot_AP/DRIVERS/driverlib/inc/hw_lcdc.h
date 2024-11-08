#ifndef __HW_LCDC_H__
#define __HW_LCDC_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the LCDC register addresses.
//
//*****************************************************************************

#define LCD_CTRL0               (LCDC_BASE + 0x00)  //8 bits
#define LCD_CTRL1               (LCDC_BASE + 0x01)  //8 bits
#define LCD_UVOL                (LCDC_BASE + 0x02)  //8 bits
#define LCD_FCR                 (LCDC_BASE + 0x03)  //8 bits
#define LCD_COM0                (LCDC_BASE + 0x04)  //54 bits
#define LCD_COM1                (LCDC_BASE + 0x0C)  //54 bits
#define LCD_COM2                (LCDC_BASE + 0x14)  //54 bits
#define LCD_COM3                (LCDC_BASE + 0x1C)  //54 bits
#define LCD_COM4                (LCDC_BASE + 0x24)  //54 bits
#define LCD_COM5                (LCDC_BASE + 0x2C)  //54 bits
#define LCD_COM6                (LCDC_BASE + 0x34)  //54 bits
#define LCD_COM7                (LCDC_BASE + 0x3C)  //54 bits
#define LCD_COM_SEG_MUX         (LCDC_BASE + 0x44)  //8 bits
#define LCD_UPDATE_DONE         (LCDC_BASE + 0x48)  //8 bits
#define LCD_FRAME_FIN           (LCDC_BASE + 0x4C)  //8 bits
#define LCD_BUSY                (LCDC_BASE + 0x50)  //8 bits
#define LCD_LCDC_EN             (LCDC_BASE + 0x54)  //8 bits
#define LCD_UPDATE_REQ          (LCDC_BASE + 0x58)  //8 bits

//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_CTRL0 register.
//
//*****************************************************************************
#define LCDC_CTRL0_TMUX_Pos                 0
#define LCDC_CTRL0_TMUX_Msk                 (3UL << LCDC_CTRL0_TMUX_Pos)
#define LCDC_CTRL0_DUTY_Pos                 2
#define LCDC_CTRL0_DUTY_Msk                 (7UL << LCDC_CTRL0_DUTY_Pos)
#define LCDC_CTRL0_BIAS_Pos                 5
#define LCDC_CTRL0_BIAS_Msk                 (3UL << LCDC_CTRL0_BIAS_Pos)
#define LCDC_CTRL0_BYPASS_EN_Pos            7
#define LCDC_CTRL0_BYPASS_EN_Msk            (1UL << LCDC_CTRL0_BYPASS_EN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_CTRL1 register.
//
//*****************************************************************************
#define LCDC_CTRL1_LWAVE_Pos                0
#define LCDC_CTRL1_LWAVE_Msk                (1UL << LCDC_CTRL1_LWAVE_Pos)
#define LCDC_CTRL1_LWAVE_A                  (0UL << LCDC_CTRL1_LWAVE_Pos)
#define LCDC_CTRL1_LWAVE_B                  (1UL << LCDC_CTRL1_LWAVE_Pos)

#define LCDC_CTRL1_FORCECLK_EN_Pos          1
#define LCDC_CTRL1_FORCECLK_EN_Msk          (1UL << LCDC_CTRL1_FORCECLK_EN_Pos)
#define LCDC_CTRL1_BLINK_Pos                2
#define LCDC_CTRL1_BLINK_Msk                (7UL << LCDC_CTRL1_BLINK_Pos)
#define LCDC_CTRL1_DEAD_Pos                 5
#define LCDC_CTRL1_DEAD_Msk                 (7UL << LCDC_CTRL1_DEAD_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_UVOL register.
//
//*****************************************************************************
#define LCDC_UVOL_VLCD_CHS_Pos              0
#define LCDC_UVOL_VLCD_CHS_Msk              (3UL << LCDC_UVOL_VLCD_CHS_Pos)
#define LCDC_UVOL_LCP_GAIN_CTRL_Pos         2
#define LCDC_UVOL_LCP_GAIN_CTRL_Msk         (3UL << LCDC_UVOL_LCP_GAIN_CTRL_Pos)
#define LCDC_UVOL_CLK_CTRL_Pos              4
#define LCDC_UVOL_CLK_CTRL_Msk              (3UL << LCDC_UVOL_CLK_CTRL_Pos)
#define LCDC_UVOL_SUPPLY_SEL_Pos            7
#define LCDC_UVOL_SUPPLY_SEL_Msk            (1UL << LCDC_UVOL_SUPPLY_SEL_Pos)
//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_FCR register.
//
//*****************************************************************************
#define LCDC_FCR_PS_Pos                     0
#define LCDC_FCR_PS_Msk                     (7UL << LCDC_FCR_PS_Pos)
#define LCDC_FCR_LCDPU_Pos                  3                              
#define LCDC_FCR_LCDPU_Msk                  (1UL << LCDC_FCR_LCDPU_Pos)
#define LCDC_FCR_DIV_Pos                    4
#define LCDC_FCR_DIV_Msk                    (0xF << LCDC_FCR_DIV_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_COM_SEG_MUX register.
//
//*****************************************************************************
#define LCDC_COM7_SEG47_Pos                 0
#define LCDC_COM7_SEG47_Msk                 (1 << LCDC_COM7_SEG47_Pos)
#define LCDC_COM6_SEG48_Pos                 1
#define LCDC_COM6_SEG48_Msk                 (1 << LCDC_COM6_SEG48_Pos)
#define LCDC_COM5_SEG49_Pos                 2
#define LCDC_COM5_SEG49_Msk                 (1 << LCDC_COM5_SEG49_Pos)
#define LCDC_COM4_SEG50_Pos                 3
#define LCDC_COM4_SEG50_Msk                 (1 << LCDC_COM4_SEG50_Pos)
#define LCDC_COM3_SEG51_Pos                 4
#define LCDC_COM3_SEG51_Msk                 (1 << LCDC_COM3_SEG51_Pos)
#define LCDC_COM2_SEG52_Pos                 5
#define LCDC_COM2_SEG52_Msk                 (1 << LCDC_COM2_SEG52_Pos)
#define LCDC_COM1_SEG53_Pos                 6
#define LCDC_COM1_SEG53_Msk                 (1 << LCDC_COM1_SEG53_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the LCD_UPDATE_DONE register.
//
//*****************************************************************************
#define LCD_UPDATE_DONE_Pos                 0
#define LCD_UPDATE_DONE_Msk                 (1UL << LCD_UPDATE_DONE_Pos)

#define LCD_FRAME_FIN_Pos                   0
#define LCD_FRAME_FIN_Msk                   (1UL << LCD_FRAME_FIN_Pos)

#define LCD_BUSY_Pos                        0
#define LCD_BUSY_Msk                        (1UL << LCD_BUSY_Pos)

#define LCD_LCDC_EN_Pos                     0
#define LCD_LCDC_EN_Msk                     (1UL << LCD_LCDC_EN_Pos)

#define LCD_UPDATE_REQ_Pos                  0
#define LCD_UPDATE_REQ_Msk                  (1UL << LCD_UPDATE_REQ_Pos)

#endif // __HW_LCDC_H__

