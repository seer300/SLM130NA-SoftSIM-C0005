#ifndef __HW_KEYSCAN_H__
#define __HW_KEYSCAN_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the KEYSCAN register.
//
//*****************************************************************************
#define KEYSCAN_COL_NUM          (KEYSCAN_BASE + 0x00)
#define KEYSCAN_ROW_NUM          (KEYSCAN_BASE + 0x01)
#define KEYSCAN_CTRL             (KEYSCAN_BASE + 0x02)
#define KEYSCAN_DATA0_VALUE      (KEYSCAN_BASE + 0x04)
#define KEYSCAN_DATA3_VALUE      (KEYSCAN_BASE + 0x07)
#define KEYSCAN_KEY_VALID        (KEYSCAN_BASE + 0x08)
#define KEYSCAN_COL_VALID        (KEYSCAN_BASE + 0x0C)
#define KEYSCAN_SCAN_COUNT       (KEYSCAN_BASE + 0x10)
#define KEYSCAN_SCAN_ROW_NUM     (KEYSCAN_BASE + 0x11)

//*****************************************************************************
//
// The following are defines for the bit fields in the COL_NUM register.
//
//*****************************************************************************
#define KEYSCAN_COL_NUM_Pos             0
#define KEYSCAN_COL_NUM_Msk             (31UL << KEYSCAN_COL_NUM_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the ROW_NUM register.
//
//*****************************************************************************
#define KEYSCAN_ROW_NUM_Pos             0
#define KEYSCAN_ROW_NUM_Msk             (31UL << KEYSCAN_ROW_NUM_Pos)
#define KEYSCAN_LONG_PRESS_NUM_Pos      5
#define KEYSCAN_LONG_PRESS_NUM_Msk      (7UL << KEYSCAN_LONG_PRESS_NUM_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the KEY_CTRL register.
//
//*****************************************************************************
#define KEYSCAN_HOLD_COUNT_Pos          0
#define KEYSCAN_HOLD_COUNT_Msk          (0xFF << KEYSCAN_HOLD_COUNT_Pos)
#define KEYSCAN_CTRL_START_Pos          8
#define KEYSCAN_CTRL_START_Msk          (1UL << KEYSCAN_CTRL_START_Pos)
#define KEYSCAN_INT_ENABLE_Pos          9
#define KEYSCAN_INI_ENABLE_Msk          (1UL << KEYSCAN_INT_ENABLE_Pos)
#define KEYSCAN_LONG_PRESS_ENABLE_Pos   10
#define KEYSCAN_LONG_PRESS_ENABLE_Msk   (1UL << KEYSCAN_LONG_PRESS_ENABLE_Pos)
#define KEYSCAN_CLK_CONFIG_Pos          11
#define KEYSCAN_CLK_CONFIG_Msk          (1UL << KEYSCAN_CLK_CONFIG_Pos)
#define KEYSCAN_CLK_CONFIG_ALWAY        (0UL << KEYSCAN_CLK_CONFIG_Pos)
#define KEYSCAN_CLK_CONFIG_LIMT         (1UL << KEYSCAN_CLK_CONFIG_Pos)
#define KEYSCAN_VOL_INVER_Pos           12
#define KEYSCAN_VOL_INVER_Msk           (1UL << KEYSCAN_VOL_INVER_Pos)
#define KEYSCAN_VOL_OUT_LOW             (0UL << KEYSCAN_VOL_INVER_Pos)
#define KEYSCAN_VOL_OUT_HIGH            (1UL << KEYSCAN_VOL_INVER_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the DATA0_VALUE register.
//
//*****************************************************************************
#define KEYSCAN_KEY0_COL_Pos            0
#define KEYSCAN_KEY0_COL_Msk            (31UL << KEYSCAN_KEY0_ROW_Pos)
#define KEYSCAN_KEY0_ROW_Pos            5
#define KEYSCAN_KEY0_ROW_Msk            (31UL << KEYSCAN_KEY0_COL_Pos)
#define KEYSCAN_KEY1_COL_Pos            10
#define KEYSCAN_KEY1_COL_Msk            (31UL << KEYSCAN_KEY1_ROW_Pos)
#define KEYSCAN_KEY1_ROW_Pos            15
#define KEYSCAN_KEY1_ROW_Msk            (31UL << KEYSCAN_KEY1_COL_Pos)
#define KEYSCAN_KEY2_COL_Pos            20
#define KEYSCAN_KEY2_COL_Msk            (31UL << KEYSCAN_KEY2_ROW_Pos)
#define KEYSCAN_KEY2_ROW_Pos            25
#define KEYSCAN_KEY2_ROW_Msk            (31UL << KEYSCAN_KEY2_COL_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the DATA3_VALUE register.
//
//*****************************************************************************
#define KEYSCAN_KEY_VALID_CNT_Pos       6
#define KEYSCAN_KEY_VALID_CNT_Msk       (3UL << KEYSCAN_KEY_VALID_CNT_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the KEY_VALID register.
//
//*****************************************************************************
#define KEYSCAN_VALID_KEY_STATUS_Pos    0
#define KEYSCAN_VALID_KEY_STATUS_Msk    (1UL << KEYSCAN_VALID_KEY_STATUS_Pos)   

//*****************************************************************************
//
// The following are defines for the bit fields in the COL_VALID register.
//
//*****************************************************************************
#define KEYSCAN_COL_VALID_Pos           0
#define KEYSCAN_COL_VALID_Msk           (0x3FFFFFFFUL << KEYSCAN_COL_VALID_Pos)  

//*****************************************************************************
//
// The following are defines for the bit fields in the SCAN_COUNT register.
//
//*****************************************************************************
#define KEYSCAN_SCAN_COUNT_Pos          0
#define KEYSCAN_SCAN_COUNT_Msk          (0xFFUL << KEYSCAN_SCAN_COUNT_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the SCAN_ROW_NUM register.
//
//*****************************************************************************
#define KEYSCAN_SCAN_ROW_NUM_Pos        0
#define KEYSCAN_SCAN_ROW_NUM_Msk        (0xFFUL << KEYSCAN_SCAN_ROW_NUM_Pos)

#endif  //__HW_KEYSCAN_H__






