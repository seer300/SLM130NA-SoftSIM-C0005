#ifndef __HW_SHA1_H
#define __HW_SHA1_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hw_memmap.h"

typedef enum
{
    SHA1_WORK_MODE_IO = 0,
    SHA1_WORK_MODE_DMA = 1,
}SHA1_WorkModeTypeDef;
    
#pragma pack (4)
typedef struct
{
    uint8_t innerBuf[64];
    uint32_t totalBitsLength;
    uint32_t dmaChannelNum;
    uint8_t memType;
    uint8_t currentInnerBufLength;
    SHA1_WorkModeTypeDef mode;
}SHA1_InitTypeDef;
#pragma pack ()




void sha1_DeInit(void);
int sha1_Init(SHA1_InitTypeDef *pSHA1_Context, uint32_t totalByteLength, SHA1_WorkModeTypeDef mode, uint8_t memType, uint32_t dmaChannelNum);
int sha1_Input(SHA1_InitTypeDef *pSHA1_Context, uint8_t *dataBuf, uint32_t dataByteLength);
int sha1_Result(SHA1_InitTypeDef *pSHA1_Context, uint8_t result[20]);

#ifdef __cplusplus
}
#endif


#endif

