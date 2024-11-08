#ifndef __CRC_H__
#define __CRC_H__

#include "hw_crc.h"
#include "hw_ints.h"
#include "hw_memmap.h"
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
extern void CRC_ParameterSet(unsigned char ucPolySize, unsigned char ucInRevMode, unsigned char ucOutRevMode, unsigned char ucXorEn);
extern void CRC_PolyCoefSet(unsigned long ulPoly);
extern void CRC_InitSet(unsigned long ulPolyInit);
extern void CRC_WaitForDone(void);
extern void CRC_XORSet(unsigned long ulXor);
extern void CRC_DataWordInput(unsigned long ulDataIn);
extern void CRC_DataByteInput(unsigned char ucDataIn);
extern unsigned long CRC_ResultGet(void);
extern void CRC_ProcessStart(unsigned char *pMessageIn, unsigned long ulMessageInLenBytes, unsigned char ucDmaFlag, unsigned char ucDmaChannel, unsigned char ucMemType);
extern unsigned long CRC_ProcessResult(unsigned char ucDmaFlag, unsigned char ucDmaChannel);

#endif // __CRC_H__
