#ifndef __HW_TICK_H__
#define __HW_TICK_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the clktickTimer register addresses.
//
//*****************************************************************************

#define TICK_CTRL               (TICK_BASE + 0x00)  //8 bits
#define TICK_PRESCALE           (TICK_BASE + 0x01)  //8 bits
#define TICK_AP_INT_EN          (TICK_BASE + 0x02)  //8 bits
#define TICK_CP_INT_EN          (TICK_BASE + 0x03)  //8 bits
#define TICK_AP_RELOAD_SET      (TICK_BASE + 0x04)  //32 bits
#define TICK_CP_RELOAD_SET      (TICK_BASE + 0x08)  //32 bits
#define TICK_AP_INTSTAT         (TICK_BASE + 0x0C)  //8 bits
#define TICK_CP_INTSTAT         (TICK_BASE + 0x10)  //8 bits
#define TICK_COUNTER            (TICK_BASE + 0x14)  //32 bits
#define TICK_AP_COMPARE         (TICK_BASE + 0x18)  //32 bits
#define TICK_CP_COMPARE         (TICK_BASE + 0x1C)  //32 bits
#define TICK_AP_RELOAD_READ     (TICK_BASE + 0x20)  //32 bits
#define TICK_CP_RELOAD_READ     (TICK_BASE + 0x24)  //32 bits
#define TICK_CNTREG_WR_DONE     (TICK_BASE + 0x28)  //32 bits


//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_CTRL register.
//
//*****************************************************************************
#define TICK_CTRL_EN_Pos                    0
#define TICK_CTRL_EN_Msk                    (1UL << TICK_CTRL_EN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_PRESCALE register.
//
//*****************************************************************************
#define TICK_PRESCALE_CFG_Pos               0
#define TICK_PRESCALE_CFG_Msk               (7UL << TICK_PRESCALE_CFG_Pos)
#define TICK_PRESCALE_NONDIV                (0)
#define TICK_PRESCALE_DIV2                  (1)
#define TICK_PRESCALE_DIV4                  (2)
#define TICK_PRESCALE_DIV8                  (3)
#define TICK_PRESCALE_DIV16                 (4)
#define TICK_PRESCALE_DIV32                 (5)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_AP_INT_EN register.
//
//*****************************************************************************
#define TICK_INT_AP_PERIOD_Pos              0
#define TICK_INT_AP_PERIOD_Msk              (1UL << TICK_INT_AP_PERIOD_Pos)
#define TICK_INT_AP_COMPARE_Pos             1
#define TICK_INT_AP_COMPARE_Msk             (1UL << TICK_INT_AP_COMPARE_Pos)
#define TICK_INT_AP_OVERFLOW_Pos            2
#define TICK_INT_AP_OVERFLOW_Msk            (1UL << TICK_INT_AP_OVERFLOW_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_CP_INT_EN register.
//
//*****************************************************************************
#define TICK_INT_CP_PERIOD_Pos              0
#define TICK_INT_CP_PERIOD_Msk              (1UL << TICK_INT_CP_PERIOD_Pos)
#define TICK_INT_CP_COMPARE_Pos             1
#define TICK_INT_CP_COMPARE_Msk             (1UL << TICK_INT_CP_COMPARE_Pos)
#define TICK_INT_CP_OVERFLOW_Pos            2
#define TICK_INT_CP_OVERFLOW_Msk            (1UL << TICK_INT_CP_OVERFLOW_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_AP_INTSTAT register.
//
//*****************************************************************************
#define TICK_AP_INTSTAT_PERIOD_Pos          0
#define TICK_AP_INTSTAT_PERIOD_Msk          (1UL << TICK_AP_INTSTAT_PERIOD_Pos)

#define TICK_AP_INTSTAT_COMPARE_Pos         1
#define TICK_AP_INTSTAT_COMPARE_Msk         (1UL << TICK_AP_INTSTAT_COMPARE_Pos)

#define TICK_AP_INTSTAT_OVERFLOW_Pos        2
#define TICK_AP_INTSTAT_OVERFLOW_Msk        (1UL << TICK_AP_INTSTAT_OVERFLOW_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_CP_INTSTAT register.
//
//*****************************************************************************
#define TICK_CP_INTSTAT_PERIOD_Pos          0
#define TICK_CP_INTSTAT_PERIOD_Msk          (1UL << TICK_AP_INTSTAT_PERIOD_Pos)

#define TICK_CP_INTSTAT_COMPARE_Pos         1
#define TICK_CP_INTSTAT_COMPARE_Msk         (1UL << TICK_AP_INTSTAT_COMPARE_Pos)

#define TICK_CP_INTSTAT_OVERFLOW_Pos        2
#define TICK_CP_INTSTAT_OVERFLOW_Msk        (1UL << TICK_AP_INTSTAT_OVERFLOW_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the TICK_CNTREG_WR_DONE register.
//
//*****************************************************************************
#define TICK_CNTREG_WR_DONE_Pos            0
#define TICK_CNTREG_WR_DONE_Msk            (1UL << TICK_CNTREG_WR_DONE_Pos)

#endif // __HW_TICK_H__

