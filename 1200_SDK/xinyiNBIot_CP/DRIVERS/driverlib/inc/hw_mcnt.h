#ifndef __HW_MCNT_H__
#define __HW_MCNT_H__

#include "hw_memmap.h"

//*****************************************************************************
//
// The following are defines for the Measure Counter register addresses.
//
//*****************************************************************************

#define MCNT_MEASURECTRL        (MCNT_BASE + 0x00)
#define MCNT_CNT32K             (MCNT_BASE + 0x04)
#define MCNT_CNTMEASURE         (MCNT_BASE + 0x08)
#define MCNT_MCNTINT            (MCNT_BASE + 0x0c)
#define MCNT_MCLKSRC            (MCNT_BASE + 0x10)

#endif // __HW_MCNT_H__
