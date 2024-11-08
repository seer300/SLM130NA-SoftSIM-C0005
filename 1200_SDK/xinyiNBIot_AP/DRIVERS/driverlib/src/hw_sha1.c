#include <string.h>
#include "hw_types.h"
#include "hw_crypto.h"
#include "dma.h"
#include "hw_sha1.h"
#include "uart.h"


/* If this macro is turned on, the entered data address and length are required to be 4-byte aligned */
#define LIGHTWEIGHT_SHA1

/**
  * @brief  Reset the sha module control related register.
  * @param  None.
  * @retval None
  */
void sha1_DeInit(void)
{
    HWREGB(REG_SHA_CTRL) = 0;
    HWREG(REG_SHA_LEN) = 0;
    HWREGB(REG_SHA_START) = 0;

    while( HWREGB(REG_SHA_CTRL) || HWREG(REG_SHA_LEN) || HWREGB(REG_SHA_START) );
}
#ifndef LIGHTWEIGHT_SHA1
/**
  * @brief   The hardware I/O mode is used to calculate SHA1.
  * @param   *pSHA1_Context Sha1 context
  * @param   *pData    Data to be calculated this time.
  * @param   lenBits   Length of pData, unit in bits.
  * @retval  0 success, others failed.
  */
static int sha1_IO(SHA1_InitTypeDef *pSHA1_Context, uint8_t *pData, uint32_t lenBits)
{
    uint32_t lenBlocks;
    uint32_t leftWords;
    uint32_t i, j, index;
    volatile uint32_t delay;
    uint8_t lessWordBytes;

    lenBlocks = lenBits >> 9;   //get the number of 16 words
    leftWords = ((lenBits & 0x1FF) + 31) >> 5; 
    lessWordBytes = (lenBits & 0x1f) >> 3;

     //The length register is used to determine if sha is running for the first time
    if( !HWREG(REG_SHA_LEN) )
    {
        HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
        HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
        HWREGB(REG_SHA_START) |= SHA_START;
    }
    
    pSHA1_Context->totalBitsLength -= lenBits;
    for(i = 0; i < lenBlocks; i++)
    {
        for(j = 0; j < 16; j++)
        {
            index = 64*i + 4*j;
            HWREG(REG_SHA_DATA_IN) = (pData[index+3] << 24) | (pData[index+2] << 16) | (pData[index+1] << 8) | pData[index];
        }
        
        while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_BLOCK_FIN));

        // delay, need to fix by rtl
        delay = 1000;
        while(delay--);
        HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
        while((HWREGB(REG_SHA_STAT) & SHA_STAT_BLOCK_FIN));
    }

    if(leftWords > 0)
    {
        for(j = 0; j < leftWords; j++)
        {
            index = 64*i + 4*j;
            if( j == (leftWords - 1) && lessWordBytes )
            {
                uint32_t temp = 0;
                uint8_t i = 0;
                while(lessWordBytes--)
                {
                    temp |= (pData[index + i] << (i << 3));
                    ++i;
                }
                HWREG(REG_SHA_DATA_IN) = (pData[index+3] << 24) | (pData[index+2] << 16) | (pData[index+1] << 8) | pData[index];
            }
            else
            {
                HWREG(REG_SHA_DATA_IN) = (pData[index+3] << 24) | (pData[index+2] << 16) | (pData[index+1] << 8) | pData[index];
            }
        }

        while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));

        // delay, need to fix by rtl
        delay = 1000;
        while(delay--);

        HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
        while((HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
    }

    return 0;
}

/**
  * @brief   The hardware DMA mode is used to calculate SHA1.
  * @details This interface can be invoked repeatedly to complete sha calculations when input data needs to be sliced.
  * @param   *pSHA1_Context Sha1 context
  * @param   *pData    Data to be calculated this time.
  * @param   dataByte  Length of pData, unit in byte.
  * @param   MemType   memory address space type.
  * @param   ChannelNum dma channel
  * @retval  None
  */
static void sha1_DMA(SHA1_InitTypeDef *pSHA1_Context, uint8_t *pData, uint32_t dataByte, uint8_t MemType, uint32_t ChannelNum)
{
    volatile uint32_t delay;

    while(DMAChannelTransferRemainCNT(ChannelNum));

    /* The address is not 4-byte aligned */
    if( (uint32_t)pData % 4 )
    {
        uint32_t i;
        for(i = 0; i < dataByte; i += 64)
        {
            /* copy data to ram, the ram address is 4-byte aligned */
            memcpy((void *)pSHA1_Context->innerBuf, pData + i, 64);
            /* copy data to sha */
            DMAChannelTransferStop(ChannelNum);
            DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
            DMAPeriphReqEn(ChannelNum);
            DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_SDEC_DIS | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
            DMAChannelTransferSet(ChannelNum, (void *)pSHA1_Context->innerBuf, (void *)REG_SHA_DATA_IN, 64, MemType);

            if( !HWREG(REG_SHA_LEN) )
            {
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
                HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
                HWREGB(REG_SHA_START) |= SHA_START;
            }
            DMAChannelTransferStart(ChannelNum);
            DMAChannelWaitIdle(ChannelNum);
            DMAIntClear(ChannelNum);
            pSHA1_Context->totalBitsLength -= (64 << 3);
        }
    }
    else
    {   /* dma can transfer up to 1M data at a time */
        uint8_t block_1M = dataByte >> 20;
        uint32_t byteLess_1M = dataByte % 0x100000;
        uint8_t i;
        
        for(i = 0; i < block_1M; i++)
        {
            DMAChannelTransferStop(ChannelNum);
            DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
            DMAPeriphReqEn(ChannelNum);
            DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_SDEC_DIS | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
            DMAChannelTransferSet(ChannelNum, (void *)(pData + (i << 20)), (void *)REG_SHA_DATA_IN, 0x100000, MemType);

             //The length register is used to determine if sha is running for the first time
            if( !HWREG(REG_SHA_LEN) )
            {
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
                HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
                HWREGB(REG_SHA_START) |= SHA_START;
            }
            DMAChannelTransferStart(ChannelNum);
            DMAChannelWaitIdle(ChannelNum);
            DMAIntClear(ChannelNum);
            pSHA1_Context->totalBitsLength -= (0x100000 << 3);
        }

        if( byteLess_1M )
        {   
            DMAChannelTransferStop(ChannelNum);
            DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
            DMAPeriphReqEn(ChannelNum);
            DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_SDEC_DIS | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
            DMAChannelTransferSet(ChannelNum, (void *)(pData + (i << 20)), (void *)REG_SHA_DATA_IN, byteLess_1M, MemType);

            //The length register is used to determine if sha is running for the first time
            if( !HWREG(REG_SHA_LEN) )
            {
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
                HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
                HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
                HWREGB(REG_SHA_START) |= SHA_START;
            }
            DMAChannelTransferStart(ChannelNum);
            DMAChannelWaitIdle(ChannelNum);
            DMAIntClear(ChannelNum);
            pSHA1_Context->totalBitsLength -= (byteLess_1M << 3);
        }
    }
    
    /*
    * If dma or dma+io mode is used, fin_over will set to 1 after the last calculation, and calc_over will not set to 1.
    */
    if( !pSHA1_Context->totalBitsLength )
    {
        while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
        // delay, need to fix by rtl
        delay = 1000;
        while(delay--);
        HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
        while((HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
    }
}

/**
  * @brief   Input the data to be calculated.
  * @details This interface can be invoked repeatedly to complete sha calculations when input data needs to be sliced.
  * @param   *pSHA1_Context Sha1 context
  * @param   *dataBuf    Data to be calculated this time.
  * @param   dataByteLength  Length of dataBuf, unit in byte.
  * @retval  0 success, others failed.
  */
int sha1_Input(SHA1_InitTypeDef *pSHA1_Context, uint8_t *dataBuf, uint32_t dataByteLength)
{
    uint32_t tempByteLength = dataByteLength + pSHA1_Context->currentInnerBufLength;
    
    if( !pSHA1_Context || !dataBuf || !dataByteLength )
    {
        return -1;
    }

    if( (tempByteLength << 3) > pSHA1_Context->totalBitsLength )
    {
        return -2;
    }

    if( (tempByteLength << 3) == pSHA1_Context->totalBitsLength )
    {
        if( !pSHA1_Context->currentInnerBufLength )
        {        
            if( pSHA1_Context->mode == SHA1_WORK_MODE_IO )
            {
                sha1_IO(pSHA1_Context, dataBuf, pSHA1_Context->totalBitsLength);
            }
            else
            {
                uint32_t blockNum = tempByteLength >> 6;
                uint32_t leftBytes = tempByteLength % 64; 
                
                if( blockNum )
                {
                    sha1_DMA(pSHA1_Context, dataBuf, blockNum << 6, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
                }
                
                if( leftBytes )
                {
                    sha1_IO(pSHA1_Context, dataBuf + (blockNum << 6), pSHA1_Context->totalBitsLength);
                }
            }
        }
        else if( tempByteLength >= 64 )
        {
            uint8_t paddingLength = 64 - pSHA1_Context->currentInnerBufLength;
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, paddingLength);
            
            if( pSHA1_Context->mode == SHA1_WORK_MODE_IO )
            {
                sha1_IO(pSHA1_Context, pSHA1_Context->innerBuf, (64 << 3));
            }
            else
            {
                sha1_DMA(pSHA1_Context, pSHA1_Context->innerBuf, 64, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            }
            
            if( pSHA1_Context->totalBitsLength )
            {
                if( pSHA1_Context->mode == SHA1_WORK_MODE_IO )
                {
                    sha1_IO(pSHA1_Context, dataBuf + paddingLength, pSHA1_Context->totalBitsLength);
                }
                else
                {
                    uint32_t blockNum = (dataByteLength - paddingLength) >> 6;
                    uint32_t leftBytes = (dataByteLength - paddingLength) % 64; 
                    
                    if( blockNum )
                    {
                        sha1_DMA(pSHA1_Context, dataBuf + paddingLength, blockNum << 6, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
                    }
                    
                    if( leftBytes )
                    {
                        sha1_IO(pSHA1_Context, dataBuf + paddingLength + (blockNum << 6), pSHA1_Context->totalBitsLength);
                    }
                }
            }
        }
        else
        {
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, dataByteLength);
            sha1_IO(pSHA1_Context, pSHA1_Context->innerBuf, pSHA1_Context->totalBitsLength);
        }

    }
    else
    {
        if( tempByteLength < 64 )
        {
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, dataByteLength);
            pSHA1_Context->currentInnerBufLength += dataByteLength;
        }
        else
        {
            uint8_t paddingLength = 64 - pSHA1_Context->currentInnerBufLength;
            uint32_t blockNum = (tempByteLength - 64) >> 6;
            uint32_t leftBytes = tempByteLength % 64; 
            
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, paddingLength);

            if( pSHA1_Context->mode == SHA1_WORK_MODE_IO )
            {
                sha1_IO(pSHA1_Context, pSHA1_Context->innerBuf, (64 << 3));
            }
            else
            {
                sha1_DMA(pSHA1_Context, pSHA1_Context->innerBuf, 64, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            }
            
            pSHA1_Context->currentInnerBufLength = 0;
            
            if( blockNum )
            {
                if( pSHA1_Context->mode == SHA1_WORK_MODE_IO )
                {
                    sha1_IO(pSHA1_Context, dataBuf + paddingLength, (blockNum << 9));
                }
                else
                {
                    sha1_DMA(pSHA1_Context, dataBuf + paddingLength, (blockNum << 6), pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
                }
            }

            if( leftBytes )
            {
                memcpy(pSHA1_Context->innerBuf, dataBuf + paddingLength + (blockNum << 6), leftBytes);
                pSHA1_Context->currentInnerBufLength = leftBytes;
            }
        }
    }            

    return 0;
}
#else
/**
  * @brief   The hardware DMA mode is used to calculate SHA1.
  * @details This interface can be invoked repeatedly to complete sha calculations when input data needs to be sliced.
  * @param   *pSHA1_Context Sha1 context
  * @param   *pData    Data to be calculated this time.
  * @param   dataByte  Length of pData, unit in byte.
  * @param   MemType   memory address space type.
  * @param   ChannelNum dma channel
  * @retval  None
  */
static void sha1_DMA(SHA1_InitTypeDef *pSHA1_Context, uint8_t *pData, uint32_t dataByte, uint8_t MemType, uint32_t ChannelNum)
{
    volatile uint32_t delay;
    /* dma can transfer up to 1M data at a time */
    uint8_t block_1M = dataByte >> 20;
    uint32_t byteLess_1M = dataByte % 0x100000;
    uint8_t i;

    #ifdef LIGHTWEIGHT_SHA1
    /* address in flash space */
    if( (((uint32_t)pData & 0x30000000) >>  28) == 0x3 )
    {
        if( ((uint32_t)pData % 4)  || (dataByte % 4) )
        {
            return;
        }
    }
    #endif
    while(DMAChannelTransferRemainCNT(ChannelNum));
    
    for(i = 0; i < block_1M; i++)
    {
        DMAChannelTransferStop(ChannelNum);
        DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
        DMAPeriphReqEn(ChannelNum);
        DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_SDEC_DIS | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
        DMAChannelTransferSet(ChannelNum, (void *)(pData + (i << 20)), (void *)REG_SHA_DATA_IN, 0x100000, MemType);

         //The length register is used to determine if sha is running for the first time
        if( !HWREG(REG_SHA_LEN) )
        {
            HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
            HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
            HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
            HWREGB(REG_SHA_START) |= SHA_START;
        }
        DMAChannelTransferStart(ChannelNum);
        DMAChannelWaitIdle(ChannelNum);
        DMAIntClear(ChannelNum);
        pSHA1_Context->totalBitsLength -= (0x100000 << 3);
    }

    if( byteLess_1M )
    {
        DMAChannelTransferStop(ChannelNum);
        DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
        DMAPeriphReqEn(ChannelNum);
        DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_SDEC_DIS | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
        DMAChannelTransferSet(ChannelNum, (void *)(pData + (i << 20)), (void *)REG_SHA_DATA_IN, byteLess_1M, MemType);

         //The length register is used to determine if sha is running for the first time
        if( !HWREG(REG_SHA_LEN) )
        {
            HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
            HWREG(REG_SHA_LEN) = pSHA1_Context->totalBitsLength;
            HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
            HWREGB(REG_SHA_START) |= SHA_START;
        }
        DMAChannelTransferStart(ChannelNum);
        DMAChannelWaitIdle(ChannelNum);
        DMAIntClear(ChannelNum);
        pSHA1_Context->totalBitsLength -= (byteLess_1M << 3);
    }
    
    /*
    * If dma or dma+io mode is used, fin_over will set to 1 after the last calculation, and calc_over will not set to 1.
    */
    if( !pSHA1_Context->totalBitsLength )
    {
        while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
        // delay, need to fix by rtl
        delay = 1000;
        while(delay--);
        HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
        while((HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
    }
}

/**
  * @brief   Input the data to be calculated.
  * @details This interface can be invoked repeatedly to complete sha calculations when input data needs to be sliced.
  * @param   *pSHA1_Context Sha1 context
  * @param   *dataBuf    Data to be calculated this time.
  * @param   dataByteLength  Length of dataBuf, unit in byte.
  * @retval  0 success, others failed.
  */
int sha1_Input(SHA1_InitTypeDef *pSHA1_Context, uint8_t *dataBuf, uint32_t dataByteLength)
{
    uint32_t tempByteLength = dataByteLength + pSHA1_Context->currentInnerBufLength;
    
    if( !pSHA1_Context || !dataBuf || !dataByteLength || (pSHA1_Context->mode != SHA1_WORK_MODE_DMA) || (dataByteLength % 4) )
    {
        return -1;
    }

    if( (tempByteLength << 3) > pSHA1_Context->totalBitsLength )
    {
        return -2;
    }

    if( (tempByteLength << 3) == pSHA1_Context->totalBitsLength )
    {
        if( !pSHA1_Context->currentInnerBufLength )
        {
            sha1_DMA(pSHA1_Context, dataBuf, dataByteLength, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
        }
        else if( tempByteLength >= 64 )
        {
            uint8_t paddingLength = 64 - pSHA1_Context->currentInnerBufLength;
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, paddingLength);
            sha1_DMA(pSHA1_Context, pSHA1_Context->innerBuf, 64, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            
            if( pSHA1_Context->totalBitsLength )
            {
                sha1_DMA(pSHA1_Context, dataBuf + paddingLength, pSHA1_Context->totalBitsLength >> 3, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            }
        }
        else
        {
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, dataByteLength);
            sha1_DMA(pSHA1_Context, pSHA1_Context->innerBuf, pSHA1_Context->totalBitsLength >> 3, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
        }
    }
    else
    {
        if( tempByteLength < 64 )
        {
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, dataByteLength);
            pSHA1_Context->currentInnerBufLength += dataByteLength;
        }
        else
        {
            uint8_t paddingLength = 64 - pSHA1_Context->currentInnerBufLength;
            uint32_t blockNum = (tempByteLength - 64) >> 6;
            uint32_t leftBytes = tempByteLength % 64; 
            
            memcpy(pSHA1_Context->innerBuf + pSHA1_Context->currentInnerBufLength, dataBuf, paddingLength);
            sha1_DMA(pSHA1_Context, pSHA1_Context->innerBuf, 64, pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            pSHA1_Context->currentInnerBufLength = 0;
            
            if( blockNum )
            {
                sha1_DMA(pSHA1_Context, dataBuf + paddingLength, (blockNum << 6), pSHA1_Context->memType, pSHA1_Context->dmaChannelNum);
            }

            if( leftBytes )
            {
                memcpy(pSHA1_Context->innerBuf, dataBuf + paddingLength + (blockNum << 6), leftBytes);
                pSHA1_Context->currentInnerBufLength = leftBytes;
            }
        }
    }            

    return 0;
}


#endif

/**
  * @brief   Initialize the sha1's work parameters'.
  * @param   *pSHA1_Context Sha1 context
  * @param   totalByteLength The total length of data to be calculated, uint in byte.
  * @param   mode  sha work type, io or dma.
  * @param   MemType   memory address space type.
  * @param   ChannelNum dma channel
  * @retval  None
  */
int sha1_Init(SHA1_InitTypeDef *pSHA1_Context, uint32_t totalByteLength, SHA1_WorkModeTypeDef mode, uint8_t memType, uint32_t dmaChannelNum)
{
    sha1_DeInit();

    if( !pSHA1_Context || !totalByteLength || ((mode != SHA1_WORK_MODE_IO) && (mode != SHA1_WORK_MODE_DMA)) )
    {
       return -1;
    }
    
    pSHA1_Context->mode = mode;
    pSHA1_Context->totalBitsLength = totalByteLength << 3;
    pSHA1_Context->memType = memType;
    pSHA1_Context->dmaChannelNum = dmaChannelNum;
    pSHA1_Context->currentInnerBufLength = 0;
    DMAPeriphReqDis(dmaChannelNum);
    DMAChannelMuxDisable(dmaChannelNum);
    DMAChannelTransferStop(dmaChannelNum);
    return 0;
}

/**
  * @brief   Get the calculated result.
  * @param   *pSHA1_Context Sha1 context
  * @param   result save the calculated result.
  * @retval  0 success, others failed.
  */

int sha1_Result(SHA1_InitTypeDef *pSHA1_Context, uint8_t result[20])
{
    uint32_t i, j, index, tmp;

    (void)index;

    if( !pSHA1_Context || !result || pSHA1_Context->totalBitsLength )
    {
        return -1;
    }

    for(i = 0; i < 5; i++)
    {
        j = i << 2; 
        tmp = HWREG(REG_SHA_RESULT + 16 - j);
        result[j]   = (tmp >> 24) & 0xFF;
        result[j+1] = (tmp >> 16) & 0xFF;
        result[j+2] = (tmp >>  8) & 0xFF;
        result[j+3] =  tmp        & 0xFF;
    }
    sha1_DeInit();
    DMAPeriphReqDis(pSHA1_Context->dmaChannelNum);
    DMAChannelMuxDisable(pSHA1_Context->dmaChannelNum);
    DMAChannelTransferStop(pSHA1_Context->dmaChannelNum);
    return 0;
}


