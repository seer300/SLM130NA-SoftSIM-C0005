#include "sema.h"
#include "interrupt.h"

/**
  * @brief Reset all semaphores
  * @retval None
  */
void SEMA_ResetAll(void)
{
    SEMA->CTL |= SEMA_CTL_RST;
    while(SEMA->CTL & SEMA_CTL_RST);
}

/**
  * @brief Request and wair for granted from semaphores
  * @param ucSemaMaster: the core's number
  *   This parameter can be one of the following values:
  *     @arg SEMA_MASTER_AP
  *     @arg SEMA_MASTER_CP
  * @param ucSemaSlave: the semaphore's number that corresponding to peripheral
  *   This parameter can be one of the following values:
  *     @arg SEMA_SLAVE_AONPRCM         
  *     @arg SEMA_SLAVE_UTC     
  *     @arg SEMA_SLAVE_LPUART 
  *     @arg SEMA_SLAVE_LPTIMER 
  *     @arg SEMA_SLAVE_COREPRCM 
  *     @arg SEMA_SLAVE_GPIO 
  *     @arg SEMA_SLAVE_QSPI 
  *     @arg SEMA_SLAVE_MCNT 
  *     @arg SEMA_SLAVE_I2C1 
  *     @arg SEMA_SLAVE_SPI 
  *     @arg SEMA_SLAVE_CSP1 
  *     @arg SEMA_SLAVE_CSP2 
  *     @arg SEMA_SLAVE_SRAM0_0 
  *     @arg SEMA_SLAVE_SRAM0_1 
  *     @arg SEMA_SLAVE_SRAM1_0  
  *     @arg SEMA_SLAVE_SRAM1_1 
  *     @arg SEMA_SLAVE_SRAM2   
  *     @arg SEMA_SLAVE_SRAM_SH 
  *     @arg SEMA_SLAVE_TIMER1 
  *     @arg SEMA_SLAVE_TIMER2  
  *     @arg SEMA_SLAVE_TIMER3 
  *     @arg SEMA_SLAVE_TIMER4  
  *     @arg SEMA_SLAVE_DMAC 
  *     @arg SEMA_SLAVE_AES_DATA 
  *     @arg SEMA_SLAVE_AES_REG 
  *     @arg SEMA_SLAVE_I2C2 
  *     @arg SEMA_SLAVE_CSP3  
  *     @arg SEMA_SLAVE_CSP4 
  *     @arg SEMA_SLAVE_UART2 
  *     @arg SEMA_SLAVE_AUXADC 
  *     @arg SEMA_SLAVE_PHYTMR
  *     @arg SEMA_SLAVE_TRNG 
  *     @arg SEMA_SLAVE_SHA   
  *     @arg SEMA_SLAVE_LCDC 
  *     @arg SEMA_SLAVE_TICK_CTRL 
  *     @arg SEMA_SLAVE_BB_REG  
  *     @arg SEMA_SLAVE_DFE   
  *     @arg SEMA_SLAVE_ISO7816  
  *     @arg SEMA_SLAVE_KEYSCAN 
  *     @arg SEMA_SLAVE_AUX 
  * @param ulDmacReq: whether request semaphore for DMA
  *   This parameter can be one of the following values:
  *     @arg SEMA_SEMA_DMAC_REQ: request for DMA 
  *     @arg SEMA_SEMA_DMAC_NO_REQ: not request for DMA 
  * @param usSemaPority: priority of the semaphore authorization waiting in the queue
  *   This parameter can be one of the following values:
  *     @arg SEMA_SEMA_REQ_PRI_0: lowest priority
  *     @arg SEMA_SEMA_REQ_PRI_1
  *     @arg SEMA_SEMA_REQ_PRI_2
  *     @arg SEMA_SEMA_REQ_PRI_3: highest priority
  * @param ucMasterMask: the bit mask of allowing core to request semaphore,
  *   and each bit corresponds to a core.
  * @retval None
  */
void SEMA_Request(uint8_t SemaMaster, uint8_t SemaSlave, uint32_t DmacReq, uint16_t SemaPority, uint8_t MasterMask)
{
   
    /* Request sema*/
    SEMA->SEMA_SLAVE[SemaSlave] = (DmacReq | SemaPority | SEMA_SEMA_REQ | MasterMask);
    
    /* wait until actually granted */
    while((SEMA->SEMA_SLAVE[SemaSlave] & SEMA_SEMA_OWNER_Msk) != ((uint32_t)(SemaMaster+1) << SEMA_SEMA_OWNER_Pos))
    {
    }
}


/**
  * @brief Get core that currently occupies the semaphore
  * @param ucSemaSlave: the semaphore's number that corresponding to peripheral
  * @retval Returns the core's number, and if not occupied by any Master returns 0x0F
  */
uint8_t SEMA_MasterGet(uint8_t SemaSlave)
{ 
	uint8_t master;
	
	master = (SEMA->SEMA_SLAVE[SemaSlave] & SEMA_SEMA_OWNER_Msk) >> SEMA_SEMA_OWNER_Pos;
    return ((master == 0x0F) ? 0x0F : (master-1));
}

/**
  * @brief Request the semaphores with non-blocking
  * @param ucSemaSlave: the semaphore's number that corresponding to peripheral
  *	  This parameter is the same as SEMA_Request
  * @param ulDmacReq: whether request semaphore for DMA
  *   This parameter can be one of the following values:
  *     @arg SEMA_SEMA_DMAC_REQ: request for DMA 
  *     @arg SEMA_SEMA_DMAC_NO_REQ: not request for DMA 
  * @param usSemaPority: priority of the semaphore authorization waiting in the queue
  *   This parameter can be one of the following values:
  *     @arg SEMA_SEMA_REQ_PRI_0: lowest priority
  *     @arg SEMA_SEMA_REQ_PRI_1
  *     @arg SEMA_SEMA_REQ_PRI_2
  *     @arg SEMA_SEMA_REQ_PRI_3: highest priority
  * @param ucMasterMask: the bit mask of allowing core to request semaphore,
  *   and each bit corresponds to a core.
  * @retval None
  */
void SEMA_RequestNonBlocking(uint8_t SemaSlave, uint32_t DmacReq, uint16_t SemaPority, uint8_t MasterMask)
{
    if(SEMA_MasterGet(SemaSlave) == SEMA_MASTER_NONE)
    {
      SEMA->SEMA_SLAVE[SemaSlave] = (DmacReq | SemaPority | SEMA_SEMA_REQ | MasterMask);
    }
}
/**
  * @brief Release the semaphore
  * @param ucSemaSlave: the semaphore's number that corresponding to peripheral
  *	  This parameter is the same as SEMA_Request
  * @param ucMasterMask: the bit mask of allowing core to request semaphore,
  *   and each bit corresponds to a core.
  * @retval None
  */
void SEMA_Release(uint8_t SemaSlave, uint8_t MasterMask)
{
    // Release
    SEMA->SEMA_SLAVE[SemaSlave] = (SEMA->SEMA_SLAVE[SemaSlave] & ~(SEMA_SEMA_REQ_Msk)) | MasterMask;
}

/**
  * @brief Get the status of request queue 
  * @retval Returns 1 if the queue is full
  */
uint8_t SEMA_ReqQueueState(void)
{
    return ((SEMA->CTL & SEMA_CTL_REQ_FULL_Msk) ? 1 : 0);
}

/**
  * @brief Gets semaphores that occupied by the core
  * @param ucSemaMaster: the core's number
  *   This parameter can be one of the following values:
  *     @arg SEMA_MASTER_AP
  *     @arg SEMA_MASTER_CP
  * @param pulSlaveReg0: the lower 32-bit of the semaphore's number that corresponding to peripheral
  * @param pulSlaveReg1: the high 32-bit of the semaphore's number that corresponding to peripheral
  * @retval None
  */
void SEMA_SlaveGet(uint8_t SemaMaster, uint32_t* pSlaveReg0, uint32_t* pSlaveReg1)
{   
    *pSlaveReg0 = (uint32_t)SEMA->MASTER_GNT[SemaMaster];
	*pSlaveReg1 = (uint32_t)(SEMA->MASTER_GNT[SemaMaster] >> 32);
}

/**
  * @brief Registers an interrupt handler for the SMEA controller.
  * @param g_pRAMVectors: the global interrupt vectors table
  * @param pfnHandler: a pointer to the function to be called when 
  *   the interrupt occurs. 
  * @retval None
  */
void SEMA_IntRegister(uint32_t *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_SEMAPHORE, g_pRAMVectors, pfnHandler);

    IntEnable(INT_SEMAPHORE);


}

/**
  * @brief Unregisters an interrupt handler for the SMEA controller.
  * @param g_pRAMVectors: the global interrupt vectors table
  * @retval None
  */
void SEMA_IntUnregister(uint32_t *g_pRAMVectors)
{
	IntDisable(INT_SEMAPHORE);

	IntUnregister(INT_SEMAPHORE, (unsigned long *)g_pRAMVectors);
}

/**
  * @brief Enable the interrupt of master
  * @param ulSemaMaster: the core's number
  *   This parameter can be one of the following values:
  *     @arg SEMA_MASTER_AP
  *     @arg SEMA_MASTER_CP
  * @retval None
  */
void SEMA_MasterIntEnable(uint32_t SemaMaster)
{
	 SEMA->CTL |= (1<<(SemaMaster+SEMA_SEMA_CTL1_Pos));
	 
}
/**
  * @brief Disable the interrupt of master
  * @param ulSemaMaster: the core's number
  *   This parameter can be one of the following values:
  *     @arg SEMA_MASTER_AP
  *     @arg SEMA_MASTER_CP
  * @retval None
  */
void SEMA_MasterIntDisable(uint32_t SemaMaster)
{
	 SEMA->CTL &= ~(1<<(SemaMaster+SEMA_SEMA_CTL1_Pos));	
}

/**
  * @brief Clear the interrupt of master 
  * @param ulSemaMaster: the core's number
  *   This parameter can be one of the following values:
  *     @arg SEMA_MASTER_AP
  *     @arg SEMA_MASTER_CP
  * @retval None
  */
void SEMA_MasterIntClear(uint32_t SemaMaster)
{
	 SEMA->CTL = (1<<(SemaMaster+SEMA_SEMA_CTL2_Pos));
}

