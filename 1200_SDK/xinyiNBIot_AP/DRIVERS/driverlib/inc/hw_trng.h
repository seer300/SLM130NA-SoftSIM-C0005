#ifndef __HW_TRNG_H__
#define __HW_TRNG_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the TRNG register offsets.
//
//*****************************************************************************
#define TRNG_SW_RESET					(TRNG_BASE + 0x140UL)
#define TRNG_CLK_ENABLE					(TRNG_BASE + 0x1C4UL)
#define TRNG_SAMPLE_CNT1				(TRNG_BASE + 0x130UL)
#define TRNG_CONFIG_REG					(TRNG_BASE + 0x10CUL)
#define TRNG_DEBUG_CONTROL				(TRNG_BASE + 0x138UL)
#define TRNG_RND_SOURCEE				(TRNG_BASE + 0x12CUL)
#define TRNG_VALID_REG					(TRNG_BASE + 0x110UL)
#define TRNG_IMR_REG					(TRNG_BASE + 0x100UL)
#define TRNG_ISR_REG					(TRNG_BASE + 0x104UL)
#define TRNG_ICR_REG					(TRNG_BASE + 0x108UL)
#define TRNG_EHR_DATA_ADDR_0			(TRNG_BASE + 0x114UL)

//*****************************************************************************
//
// RNG module value defination.
//
//*****************************************************************************
#define HW_RNG_ISR_REG_EHR_VALID            0x1
#define HW_RNG_ISR_REG_AUTOCORR_ERR         0x2
#define HW_TRNG_VALID_REG_EHR_NOT_READY     0x0
#define HW_RND_SOURCE_ENABLE_REG_SET        0x1
#define HW_RND_SOURCE_ENABLE_REG_CLR        0x0
#define HW_TRNG_DEBUG_CONTROL_REG_VNC_BYPASS         0x2
#define HW_TRNG_DEBUG_CONTROL_REG_CRNGT_BYPASS       0x4
#define HW_TRNG_DEBUG_CONTROL_REG_AUTOCORR_BYPASS    0x8
#define HW_TRNG_DEBUG_CONTROL_REG_FAST      (HW_TRNG_DEBUG_CONTROL_REG_VNC_BYPASS | \
                                            HW_TRNG_DEBUG_CONTROL_REG_CRNGT_BYPASS | \
                                            HW_TRNG_DEBUG_CONTROL_REG_AUTOCORR_BYPASS)
#define HW_TRNG_DEBUG_CONTROL_REG_FE        0x0
#define HW_TRNG_DEBUG_CONTROL_REG_80090B    (HW_TRNG_DEBUG_CONTROL_REG_VNC_BYPASS | \
                                            HW_TRNG_DEBUG_CONTROL_REG_AUTOCORR_BYPASS)
#define HW_RNG_SW_RESET_REG_SET             0x1
#define HW_RNG_CLK_ENABLE_REG_SET           0x1

//*****************************************************************************
// the output format defination
// the output buffer consists of header + sample bits + footer
// header: 4 words.
//         word0/3=0xAABBCCDD
//         word1=TRNGmode[31:30]+roscLen[25:24]+simpleLen[23:0]
//         word2=sampleCount
// footer: 3 words.
//         wordn/n+2=0xDDCCBBAA
//         wordn+1=error flags
//*****************************************************************************
#define OUTPUT_FORMAT_HEADER_LENGTH_IN_WORDS   4
#define OUTPUT_FORMAT_FOOTER_LENGTH_IN_WORDS   3
#define OUTPUT_FORMAT_OVERHEAD_LENGTH_IN_WORDS (OUTPUT_FORMAT_HEADER_LENGTH_IN_WORDS + \
                                                OUTPUT_FORMAT_FOOTER_LENGTH_IN_WORDS)
#define OUTPUT_FORMAT_HEADER_SIG_VAL           0xAABBCCDD
#define OUTPUT_FORMAT_FOOTER_SIG_VAL           0xDDCCBBAA
#define OUTPUT_FORMAT_TRNG_MODE_SHIFT          24
#define OUTPUT_FORMAT_ROSC_LEN_SHIFT           30
#define OUTPUT_FORMAT_HEADER_LENGTH_IN_BYTES   (4*sizeof(uint32_t))
#define OUTPUT_FORMAT_FOOTER_LENGTH_IN_BYTES   (3*sizeof(uint32_t))

//*****************************************************************************
//
// TRNG mode defination.
//
//*****************************************************************************
#define TRNG_MODE_FAST                         0
#define TRNG_MODE_FE                           1
#define TRNG_MODE_80090B                       2

//*****************************************************************************
//
// error code defination.
//
//*****************************************************************************
#define CC_TRNG_INVALID_PARAM_TRNG_MODE        (-1)
#define CC_TRNG_INVALID_PARAM_ROSC_LEN         (-2)
#define CC_TRNG_INVALID_PARAM_SAMPLE_CNT       (-3)
#define CC_TRNG_INVALID_PARAM_BUF_SIZE         (-4)
#define CC_TRNG_INVALID_PARAM_NULL_PTR         (-5)
#define CC_TRNG_SAMPLE_LOST                    0x1
#define CC_TRNG_INVALID_PARAM_OK               0x0


//*****************************************************************************
//
// other size and length defination.
//
//*****************************************************************************
#define EHR_SIZE_IN_WORDS                      6
#define EHR_SIZE_IN_BYTES                      (6*sizeof(uint32_t))
#define MAX_BUFFER_LENGTH                      (1<<24)
#define MIN_BUFFER_LENGTH                      ((OUTPUT_FORMAT_OVERHEAD_LENGTH_IN_WORDS + \
                                               EHR_SIZE_IN_WORDS) * sizeof(uint32_t))
#define DATA_COLLECTION_START_INDEX            0
#define TRNG_ROSC_MAX_LENGTH                   3
#define TRNG_BUFFER_SIZE_IN_WORDS              6
#define MINIUM_SAMPLE_CNT                      1
#ifndef NULL
#define NULL  0
#endif


#endif // __HW_TRNG_H__
