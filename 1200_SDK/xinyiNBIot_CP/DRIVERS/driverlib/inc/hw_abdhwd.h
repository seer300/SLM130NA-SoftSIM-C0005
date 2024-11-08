#ifndef __HW_ABDHWD_H__
#define __HW_ABDHWD_H__

#include "hw_memmap.h"

//*****************************************************************************
//
// The following are defines for the ABDHWD register offsets.
//
//*****************************************************************************
#define ABDHWD_UHWD_CFG               (ABDHWD_BASE + 0x00000000)
#define ABDHWD_ABD_CTRL               (ABDHWD_BASE + 0x00000010)
#define ABDHWD_ABD_STATUS             (ABDHWD_BASE + 0x00000014)
#define ABDHWD_ABD_COUNT_L            (ABDHWD_BASE + 0x00000018)
#define ABDHWD_ABD_COUNT_H            (ABDHWD_BASE + 0x0000001C)


//*****************************************************************************
//
// The following are defines for the bit fields in the UHWD_CFG register.
//
//*****************************************************************************
#define ABDHWD_CFG_CSP_Pos             0
#define ABDHWD_CFG_CSP_Msk             (3UL << ABDHWD_CFG_CSP_Pos)
#define ABDHWD_CFG_CSP2                (0UL << ABDHWD_CFG_CSP_Pos)
#define ABDHWD_CFG_CSP3                (1UL << ABDHWD_CFG_CSP_Pos)
#define ABDHWD_CFG_CSP4                (2UL << ABDHWD_CFG_CSP_Pos)

#define ABDHWD_CFG_Valid_Bit_Pos       4
#define ABDHWD_CFG_Valid_Bit_Msk       (3UL << ABDHWD_CFG_Valid_Bit_Pos)
#define ABDHWD_CFG_Valid_Bit_5         (0UL << ABDHWD_CFG_Valid_Bit_Pos)
#define ABDHWD_CFG_Valid_Bit_6         (1UL << ABDHWD_CFG_Valid_Bit_Pos)
#define ABDHWD_CFG_Valid_Bit_7         (2UL << ABDHWD_CFG_Valid_Bit_Pos)
#define ABDHWD_CFG_Valid_Bit_8         (3UL << ABDHWD_CFG_Valid_Bit_Pos)

#define ABDHWD_CFG_HW_Pos              7
#define ABDHWD_CFG_HW_Msk              (1UL << ABDHWD_CFG_HW_Pos)
#define ABDHWD_CFG_HW_En               (1UL << ABDHWD_CFG_HW_Pos)

#define ABDHWD_CFG_PATTERN_Pos         8
#define ABDHWD_CFG_PATTERN_Msk         (0xFF << ABDHWD_CFG_PATTERN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the ABD_CTRL register.
//
//*****************************************************************************
#define ABDHWD_ABD_ENA_Pos             0
#define ABDHWD_ABD_ENA_Msk             (1UL << ABDHWD_ABD_ENA_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the ABD_STATUS register.
//
//*****************************************************************************
#define ABDHWD_ABD_END_Pos             0
#define ABDHWD_ABD_END_Msk             (1UL << ABDHWD_ABD_END_Pos)


#endif // __HW_ABDHWD_H__
