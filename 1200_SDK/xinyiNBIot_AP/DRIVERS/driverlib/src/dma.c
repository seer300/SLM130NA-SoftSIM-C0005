#include "dma.h"
#include "interrupt.h"

/**
  * @brief Config the DMA channel.
  * @param ulChannelNum: the DMA channel number
  * @param ulConfig: the configuration for DMA
  *    This parameter can be any combination of the following values:
  *     @arg DMAC_CTRL_BURST_SIZE_1W: burst size = 4 bytes
  *     @arg DMAC_CTRL_BURST_SIZE_2W: burst size = 2*4 bytes
  *     @arg DMAC_CTRL_BURST_SIZE_4W: burst size = 4*4 bytes
  *     @arg DMAC_CTRL_BURST_SIZE_8W: burst size = 8*4 bytes
  *     @arg DMAC_CTRL_BURST_SIZE_16W: burst size = 16*4 bytes
  *
  *     @arg DMAC_CTRL_TYPE_MEM_TO_MEM: memory to memory transfer.
  *     @arg DMAC_CTRL_TYPE_IO_TO_MEM: peripheral to memory transfer.
  *     @arg DMAC_CTRL_TYPE_MEM_TO_IO: memory to peripheral transfer.
  *
  *     @arg DMAC_CTRL_INT_SET: if set, enabled DMA transfer completion interrupt.
  *
  *     @arg DMAC_CTRL_TC_SET: if set, when transfer count reaches zero,the DMA 
  *          terminates data transfers; if not,the DMA transfer never finishes. 
  *
  *     @arg DMAC_CTRL_SINC_SET: auto increases the source address
  *     @arg DMAC_CTRL_SINC_DIS: default
  *     @arg DMAC_CTRL_SDEC_SET: auto decrements the source address
  *     @arg DMAC_CTRL_SDEC_DIS: default
  *          If SINC and SDEC are set to the same value, the source address 
  *          remains unchanged.  
  *     @arg DMAC_CTRL_DINC_SET: auto increases the destination address
  *     @arg DMAC_CTRL_DINC_DIS: default
  *     @arg DMAC_CTRL_DDEC_SET: auto decrements the destination address
  *     @arg DMAC_CTRL_DDEC_DIS: default
  *          If DINC and DDEC are set to the same value, the destination address 
  *          remains unchanged.
  * @retval None
  */
void DMAChannelConfigure(unsigned long ulChannelNum, unsigned long ulConfig)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) = ulConfig;
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) = ulConfig;
	}
}

/**
  * @brief Enable peripheral hardware to initiate DMA transfer function.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAPeriphReqEn(unsigned long ulChannelNum)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_PERIREQ_EN) |= (unsigned long)1 << ulChannelNum;
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC1_PERIREQ_EN) |= (unsigned long)1 << ulChannelNum;
	}
}

/**
  * @brief Disable peripheral hardware to initiate DMA transfer function.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAPeriphReqDis(unsigned long ulChannelNum)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_PERIREQ_EN) &= ~((unsigned long)1 << ulChannelNum);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC1_PERIREQ_EN) &= ~((unsigned long)1 << ulChannelNum);
	}
}

/**
  * @brief Start the DMA channel transmission.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAChannelTransferStart(unsigned long ulChannelNum)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) |= (DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk);
	}	
}

/**
  * @brief Get the DMA channel start status.
  * @param ulChannelNum: the DMA channel number
  * @return the status
  *  	@retval 1 start
  *  	@retval 0 stop
  */
uint8_t DMAChannelGetStartStatus(unsigned long ulChannelNum)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		return (HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) & DMAC_CTRL_STRT_Msk ? 1 : 0);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		return (HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) & DMAC_CTRL_STRT_Msk ? 1 : 0);
	}	
}


/**
  * @brief Stop the DMA channel transmission.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAChannelTransferStop(unsigned long ulChannelNum)
{   
	unsigned long ulDataTmp = 0;
	
	if(ulChannelNum < DMA_CHANNEL_4){
		ulDataTmp = HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5));	
		HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) = (ulDataTmp | DMAC_CTRL_CHG_Msk) & (~ DMAC_CTRL_STRT_Msk);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		ulDataTmp = HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5));	
		HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) = (ulDataTmp | DMAC_CTRL_CHG_Msk) & (~ DMAC_CTRL_STRT_Msk);
	}	
}


/**
  * @brief Setting the request relationship between peripheral and DMA channel.
  * @param ulChannelNum: the DMA channel number
  * @param ucPeriSrc: the peripheral request signal
  *   This parameter can be one of the following values:
  *     @arg DMA_REQNUM_CSP2_RX:
  *     @arg DMA_REQNUM_CSP2_TX: 
  *     @arg DMA_REQNUM_CSP3_RX: 
  *     @arg DMA_REQNUM_CSP3_TX: 
  *     @arg DMA_REQNUM_CSP4_RX: 
  *     @arg DMA_REQNUM_CSP4_TX: 
  *     @arg DMA_REQNUM_AES_IN: 
  *     @arg DMA_REQNUM_AES_OUT: 
  *     @arg DMA_REQNUM_QSPI_RD: 
  *     @arg DMA_REQNUM_QSPI_WR: 
  *     @arg DMA_REQNUM_ADC_RD: 
  *     @arg DMA_REQNUM_SHA_WR:
  *     @arg DMA_REQNUM_UART2_RX:
  *     @arg DMA_REQNUM_UART2_WR:
  *     @arg DMA_REQNUM_UART1_RX:
  *     @arg DMA_REQNUM_UART1_WR:
  *     @arg DMA_REQNUM_CS_REQ0:
  *     @arg DMA_REQNUM_CS_REQ1:
  *     @arg DMA_REQNUM_CS_REQ2:
  *     @arg DMA_REQNUM_CS_REQ3:
  * @retval None
  */
void DMAChannelPeriphReq(unsigned long ulChannelNum, unsigned char ucPeriSrc)
{
	HWREGB(DMAC_CH0_MUXCFG + (ulChannelNum << 2)) = DMAC_MUXCFG_EN_Msk | ucPeriSrc;
}

void DMAChannelMuxDisable(unsigned long ulChannelNum)
{
	HWREGB(DMAC_CH0_MUXCFG + (ulChannelNum << 2)) = 0;
}

/**
  * @brief Start DMA transfer once.
  * @param ulChannelNum: the DMA channel number
  * @param pvSrcAddr: the source address of DMA transmission
  * @param pvDstAddr: the destination address of DMA transmission
  * @param ulLenByte: the number of DMA transfer bytes
  * @param ucMemType: the core type of the source and destination address
  *   This parameter can be one of the following values:
  *     @arg MEMORY_TYPE_AP: ap core access
  *     @arg MEMORY_TYPE_CP: cp core access
  *     @arg MEMORY_TYPE_DMA: DMA direct access
  * @retval None
  */
void DMAChannelTransfer(unsigned long ulChannelNum, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned char ucMemType)
{
    if(ulLenByte != 0)
    {
        DMAChannelTransferSet(ulChannelNum, (void *)ulSrcAddr, (void *)ulDstAddr, ulLenByte, ucMemType);
        DMAChannelTransferStart(ulChannelNum);
        DMAChannelWaitIdle(ulChannelNum);
        DMAIntClear(ulChannelNum);
    }
}

/**
  * @brief Config the address of source and destination.
  * @param ulChannelNum: the DMA channel number
  * @param pvSrcAddr: the source address of DMA transmission
  * @param pvDstAddr: the destination address of DMA transmission
  * @param ulLenByte: the number of DMA transfer bytes
  * @param ucMemType: the core type of the source and destination address
  *   This parameter can be one of the following values:
  *     @arg MEMORY_TYPE_AP: ap core access
  *     @arg MEMORY_TYPE_CP: cp core access
  *     @arg MEMORY_TYPE_DMA: DMA direct access
  * @retval None
  */
void DMAChannelTransferSet(unsigned long ulChannelNum, void *pvSrcAddr, void *pvDstAddr, unsigned long ulTransferSize, unsigned char ucMemType)
{  
	unsigned long uldmaSrc = (unsigned long)pvSrcAddr;
	unsigned long uldmaDst = (unsigned long)pvDstAddr;
	unsigned long ulTCMCP = TCM_CP_BASE;
	unsigned long ulFlashCP = FLASH_CP_REMAP_BASE;
	
	/* Calculate the address of DMA Source/Destination */
	if(ucMemType == MEMORY_TYPE_AP)
	{
	    if(uldmaSrc >= TCM_AP_BASE && uldmaSrc < TCM_AP_LIMITED)
		{
			uldmaSrc += AP_TO_DMA_OFFSET;
		}
		if(uldmaDst >= TCM_AP_BASE && uldmaDst < TCM_AP_LIMITED)
		{
			uldmaDst += AP_TO_DMA_OFFSET;
		}	
	}
	else if(ucMemType == MEMORY_TYPE_CP)
	{
		if((HWREGB(AONPRCM_BASE + 0x01) & 0x01) ) //CP Remap
		{
			if(uldmaSrc >= TCM_CP_REMAP_BASE && uldmaSrc < TCM_CP_REMAP_LIMITED)
			{
				uldmaSrc += (CP_TO_DMA_OFFSET - TCM_CP_BASE_OFFSET);
			}
			else if(uldmaSrc >= ulFlashCP && uldmaSrc < FLASH_CP_REMAP_LIMITED)
			{		
				uldmaSrc += FLASH_CP_BASE_OFFSET;
		    }	
		  
		    if(uldmaDst >= TCM_CP_REMAP_BASE && uldmaDst < TCM_CP_REMAP_LIMITED)
			{
				uldmaDst += (CP_TO_DMA_OFFSET - TCM_CP_BASE_OFFSET);
			}
			else if(uldmaDst >= ulFlashCP && uldmaDst < FLASH_CP_REMAP_LIMITED)
			{		
				uldmaDst += FLASH_CP_BASE_OFFSET;
		    }	
		}
		else
		{
			if(uldmaSrc >= ulTCMCP && uldmaSrc < TCM_CP_LIMITED )
			{
				uldmaSrc += CP_TO_DMA_OFFSET;
			}

			if(uldmaDst >= ulTCMCP && uldmaDst < TCM_CP_LIMITED )
			{
				uldmaDst += CP_TO_DMA_OFFSET;
			}
		}	
	}
	
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_CH0_SA + (ulChannelNum << 5)) = (unsigned long)uldmaSrc;
		HWREG(DMAC_CH0_DA + (ulChannelNum << 5)) = (unsigned long)uldmaDst;
		HWREG(DMAC_CH0_TC + (ulChannelNum << 5)) = ulTransferSize;
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC_CH4_SA + (ulChannelNum << 5)) = (unsigned long)uldmaSrc;
		HWREG(DMAC_CH4_DA + (ulChannelNum << 5)) = (unsigned long)uldmaDst;
		HWREG(DMAC_CH4_TC + (ulChannelNum << 5)) = ulTransferSize;
	}
}

/**
  * @brief Enable the arbitration mechanism for channels
  * @param num: the dma controler num
  *     @arg 0: dma   ch0 > ch1 > ch2 > ch3 
  *     @arg 1: dma1  ch4 > ch5 > ch6 > ch7
  * @retval None
  */
void DMAChannelArbitrateEnable(unsigned char num)
{
	if(num == 0){
		HWREG(DMAC_PERIREQ_EN) |= DMAC_CHANNEL_ARBITER_Msk;
	}else{
		HWREG(DMAC1_PERIREQ_EN) |= DMAC_CHANNEL_ARBITER_Msk;
	}
}

/**
  * @brief Disable the arbitration mechanism for channels
  * @param num: the dma controler num
  * @retval None
  */
void DMAChannelArbitrateDisable(unsigned char num)
{
	if(num == 0){
		HWREG(DMAC_PERIREQ_EN) &= ~DMAC_CHANNEL_ARBITER_Msk;
	}else{
		HWREG(DMAC1_PERIREQ_EN) &= ~DMAC_CHANNEL_ARBITER_Msk;
	}
}

/**
  * @brief Setup a link list of DMA transfers.
  * @param ulChannelNum: the DMA channel number
  * @param pvNextPointer: a pointer to link list
  * @retval None
  */
void DMAChannelNextPointerSet(unsigned long ulChannelNum, void *pvNextPointer)
{
   	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_CH0_NP + (ulChannelNum << 5)) = (unsigned long)pvNextPointer;
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC_CH4_NP + (ulChannelNum << 5)) = (unsigned long)pvNextPointer;
	}
}

/**
  * @brief Clear the DMA error status.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAErrorStatusClear(unsigned long ulChannelNum)
{
   	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) |= DMAC_CTRL_ERR_Msk;
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) |= DMAC_CTRL_ERR_Msk;
	}	
}

/**
  * @brief Gets the DMA error status.
  * @param ulChannelNum: the DMA channel number
  * @retval Returns true if DMA transmit error
  */
unsigned char DMAErrorStatusGet(unsigned long ulChannelNum)
{
   	if(ulChannelNum < DMA_CHANNEL_4){
		return((HWREG(DMAC_CH0_CTRL + (ulChannelNum << 5)) & DMAC_CTRL_ERR_Msk) ? 1 : 0);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		return((HWREG(DMAC_CH4_CTRL + (ulChannelNum << 5)) & DMAC_CTRL_ERR_Msk) ? 1 : 0);
	}		
}

/**
  * @brief Gets the DMA request sources pending status.
  * @param None
  * @retval Returns the current pending status of all DMA request sources
  *    Each bit represents a request source. See ucPeriSrc for details of DMAChannelPeriphReq().
  */
unsigned long DMAReqSrcPendingGet(void)
{
	return HWREG(DMAC_REQ_PEND0);
}

/**
  * @brief Get the number of remaining data to be transferred.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
unsigned long DMAChannelTransferRemainCNT(unsigned long ulChannelNum)
{   
   	if(ulChannelNum < DMA_CHANNEL_4){
		 return HWREG(DMAC_CH0_TC + (ulChannelNum << 5));
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		 return HWREG(DMAC_CH4_TC + (ulChannelNum << 5));
	}			
}

/**
  * @brief Fills the memory with a given value.
  * @param ulChannelNum: the DMA channel number
  * @param pvDstAddr: a pointer to the memory
  * @param ucValue: the value to be filled
  * @param ulSize: the number of characters to be set to ucValue
  * @param ucMemType: the core type of the source and destination address
  *   This parameter can be one of the following values:
  *     @arg MEMORY_TYPE_AP: ap core access
  *     @arg MEMORY_TYPE_CP: cp core access
  *     @arg MEMORY_TYPE_DMA: DMA direct access
  * @retval None
  */
void DMACMemset(unsigned long ulChannelNum, void *pvDstAddr, unsigned char ucValue,unsigned long ulSize, unsigned char ucMemType)
{
    unsigned long i;
	unsigned char emptyBuf[64] = {0}; 

	if(NULL == pvDstAddr || ulSize <= 0)
	{
        return ;
    }
	for(i = 0;i < 64;i++)
	{
		emptyBuf[i] = ucValue;
	}
	
    DMAChannelConfigure(ulChannelNum, DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W);
    DMAChannelTransferSet(ulChannelNum, (void *)emptyBuf, (void *)pvDstAddr, ulSize, ucMemType);
    DMAChannelTransferStart(ulChannelNum);	
	DMAChannelWaitIdle(ulChannelNum);
	DMAIntClear(ulChannelNum);	
}

/**
  * @brief Registers an interrupt handler for the DMA controller.
  * @param ulIntChannel: the interrupt assignments of DMA
  * @param g_pRAMVectors: the global interrupt vectors table
  * @param pfnHandler: a pointer to the function to be called when 
  *        the DMA interrupt occurs. 
  * @retval None
  */
void DMAIntRegister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(ulIntChannel, g_pRAMVectors, pfnHandler);

    IntEnable(ulIntChannel);
}

/**
  * @brief Unregisters an interrupt handler for the DMA controller.
  * @param g_pRAMVectors: the global interrupt vectors table
  * @retval None
  */
void DMAIntUnregister(unsigned long ulIntChannel, unsigned long *g_pRAMVectors)
{
    IntDisable(ulIntChannel);

    IntUnregister(ulIntChannel, g_pRAMVectors);
}

/**
  * @brief Clears DMA interrupt status.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAIntClear(unsigned long ulChannelNum)
{
	if(ulChannelNum < DMA_CHANNEL_4){
		HWREG(DMAC_INT_STAT) = (1UL << ulChannelNum);
	}else{
		ulChannelNum -= DMA_CHANNEL_4;
		HWREG(DMAC1_INT_STAT) = (1UL << ulChannelNum);
	}		
}

/**
  * @brief Wait untile the specific DMA channel transmission done.
  * @param ulChannelNum: the DMA channel number
  * @retval None
  */
void DMAChannelWaitIdle(unsigned long ulChannelNum)
{
    unsigned long ulBitOffset = 0;
    
	if(ulChannelNum < DMA_CHANNEL_4){
		
		ulBitOffset = (1 << ulChannelNum);	
		while((HWREG(DMAC_INT_STAT) & ulBitOffset) != ulBitOffset);
		
	}else{
		
		ulChannelNum -= DMA_CHANNEL_4;
		ulBitOffset = (1 << ulChannelNum);	
		while((HWREG(DMAC1_INT_STAT) & ulBitOffset) != ulBitOffset);
	}
}

/**
  * @brief Gets the DMA controller channel interrupt status.
  * @retval Returns true to indicate the channel occurs interrupt.
  */
unsigned char DMAIntStatus(unsigned long ulChannelNum)
{
    unsigned long ulBitOffset = 0;
    
	if(ulChannelNum < DMA_CHANNEL_4){
		
		ulBitOffset = (1 << ulChannelNum);
		return((HWREG(DMAC_INT_STAT) & ulBitOffset) ? 1 : 0);
		
	}else{
		
		ulChannelNum -= DMA_CHANNEL_4;
		ulBitOffset = (1 << ulChannelNum);
		return((HWREG(DMAC1_INT_STAT) & ulBitOffset) ? 1 : 0);
	}	
}

/**
  * @brief Gets the DMA all channels interrupt status.
  * @retval The bit mask of the interrupt status for all channels.
  */
unsigned char DMAIntAllStatus(void)
{
    return(HWREGB(DMAC1_INT_STAT) << 4 | HWREGB(DMAC_INT_STAT));
}

/**
  * @brief Enable auto clock gating function for low power(default)
  * @param num: the dma controler num
  *     @arg 0: dma   
  *     @arg 1: dma1 
  * @retval None
  */
void DMAClockGateEnable(unsigned char num)
{
	if(num == 0){
		HWREG(DMAC_CLK_GATE) &= ~DMAC_CHANNEL_CLK_GATE_Msk;
	}else{
		HWREG(DMAC1_CLK_GATE) &= ~DMAC_CHANNEL_CLK_GATE_Msk;
	}	
}

/**
  * @brief Disable auto clock gating function
  * @param num: the dma controler num
  *     @arg 0: dma   
  *     @arg 1: dma1 
  * @retval None
  */
void DMAClockGateDisable(unsigned char num)
{
	if(num == 0){
		HWREG(DMAC_CLK_GATE) |= DMAC_CHANNEL_CLK_GATE_Msk;
	}else{
		HWREG(DMAC1_CLK_GATE) |= DMAC_CHANNEL_CLK_GATE_Msk;
	}	
}

