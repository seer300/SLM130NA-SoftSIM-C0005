#ifndef __CRC_H__
#define __CRC_H__

#include "hw_crc.h"
#include "hw_ints.h"
#include "xinyi2100.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"
#include "dma.h"


#define CRC_POLY_SIZE_8					0
#define CRC_POLY_SIZE_12				1
#define CRC_POLY_SIZE_16				2
#define CRC_POLY_SIZE_32				3

#define CRC_INPUT_NO_REVERSE			0
#define CRC_INPUT_REVERSE_BY_BYTE		1
#define CRC_INPUT_REVERSE_BY_HALF_WORD	2
#define CRC_INPUT_REVERSE_BY_WORD		3

#define CRC_OUTPUT_NO_REVERSE			0
#define CRC_OUTPUT_REVERSE				1

#define CRC_XOR_DISABLE					0
#define CRC_XOR_EN						1

#define CRC_MODE_DMA					1
#define CRC_MODE_IO						0

extern void CRC_Reset(void);
extern void CRC_ParameterSet(uint8_t ucPolySize, uint8_t ucInRevMode, uint8_t ucOutRevMode, uint8_t ucXorEn);
extern void CRC_PolyCoefSet(uint32_t ulPoly);
extern void CRC_InitSet(uint32_t ulPolyInit);
extern void CRC_WaitForDone(void);
extern void CRC_XORSet(uint32_t ulXor);
extern void CRC_DataWordInput(uint32_t ulDataIn);
extern void CRC_DataByteInput(uint8_t ucDataIn);
extern uint32_t CRC_ResultGet(void);
extern void CRC_ProcessStart(uint8_t *pMessageIn, uint32_t ulMessageInLenBytes, uint8_t ucDmaFlag, uint8_t ucDmaChannel, uint8_t ucMemType);
extern uint32_t CRC_ProcessResult(uint8_t ucDmaFlag, uint8_t ucDmaChannel);

#endif // __CRC_H__
