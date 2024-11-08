#include "crc.h"

void CRC_Reset(void)
{
	CRC->CTRL0 |= CRC_CTRL0_SOFT_RESET_Msk;
}

void CRC_ParameterSet(uint8_t ucPolySize, uint8_t ucInRevMode, uint8_t ucOutRevMode, uint8_t ucXorEn)
{
	CRC->CTRL1 = (ucXorEn << CRC_CTRL1_XOR_EN_Pos) | (ucOutRevMode << CRC_CTRL1_REV_O_Pos) | (ucInRevMode << CRC_CTRL1_REV_IN_Pos) | ucPolySize;
}

void CRC_PolyCoefSet(uint32_t ulPoly)
{
	CRC->POLY_COEF = ulPoly;
}

void CRC_InitSet(uint32_t ulPolyInit)
{
	CRC->INIT = ulPolyInit;
}

void CRC_WaitForDone(void)
{
	while(!(CRC->STATUS & CRC_STATUS_CRC_DONE_Msk));
}

void CRC_XORSet(uint32_t ulXor)
{
	CRC->XOR = ulXor;
}

void CRC_DataWordInput(uint32_t ulDataIn)
{
	CRC->DATA0 = ulDataIn & 0xFF;
    CRC->DATA1 = (ulDataIn >> 8 )& 0xFF;
    CRC->DATA2 = (ulDataIn >> 16)& 0xFF;
    CRC->DATA3 = (ulDataIn >> 24)& 0xFF;
}

void CRC_DataByteInput(uint8_t ucDataIn)
{
	CRC->DATA0 = ucDataIn;
}

uint32_t CRC_ResultGet(void)
{
    uint32_t result;
    result = (CRC->DATA3<<24)|(CRC->DATA2<<16)|(CRC->DATA1<<8)|CRC->DATA0;
	return result;
}

void CRC_ProcessStart(uint8_t *pMessageIn, uint32_t ulMessageInLenBytes, uint8_t ucDmaFlag, uint8_t ucDmaChannel, uint8_t ucMemType)
{
	uint32_t ulCurAddr;
	uint32_t ulLenBytes0, ulLenWords, ulLenBytes1;
	uint32_t i;
	
	CRC->CTRL0 |= CRC_CTRL0_SOFT_RESET_Msk;
	
	ulCurAddr = (uint32_t)pMessageIn;
		
	ulLenBytes0 = 4 - (ulCurAddr & 0x3);
	
	if(ulLenBytes0 >= ulMessageInLenBytes)
	{
		ulLenBytes0 = ulMessageInLenBytes;
		ulLenWords  = 0;
		ulLenBytes1 = 0;
	}
	else
	{
		ulLenWords  = (ulMessageInLenBytes - ulLenBytes0) >> 2;
		ulLenBytes1 = (ulMessageInLenBytes - ulLenBytes0) & 0x03;
	}

	for(i = 0; i < ulLenBytes0; i++)
	{
		CRC->DATA0 = HWREGB(ulCurAddr);
		ulCurAddr++;
	}
		
	if(CRC_MODE_IO == ucDmaFlag)
	{
		for(i = 0; i < ulLenWords; i++)
		{
			CRC_DataWordInput(HWREG(ulCurAddr));
			ulCurAddr += 4;
		}
		
		for(i = 0; i < ulLenBytes1; i++)
		{
			CRC->DATA0 = HWREGB(ulCurAddr);
			ulCurAddr++;
		}
	}
	else
	{
		DMAChannelConfigure(ucDmaChannel, DMAC_CTRL_DDEC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W);
		DMAChannelTransferSet(ucDmaChannel, (void *)((uint32_t)pMessageIn + ulLenBytes0), (void *)CRC_DATA, ulMessageInLenBytes - ulLenBytes0, ucMemType);
		DMAChannelTransferStart(ucDmaChannel);
	}
}

uint32_t CRC_ProcessResult(uint8_t ucDmaFlag, uint8_t ucDmaChannel)
{
	if(CRC_MODE_DMA == ucDmaFlag)
	{
		DMAChannelWaitIdle(ucDmaChannel);
		DMAIntClear(ucDmaChannel);
	}
	
	while(!(CRC->STATUS & CRC_STATUS_CRC_DONE_Msk));
	
	return CRC_ResultGet();
}

