#include "cache.h"

/**
  * @brief  enable the cache.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheEN(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) |= 0x01;
}

/**
  * @brief  disable the cache.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheDis(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) &= 0xFE;
}

/**
  * @brief  enable bypass cache.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheBypassEN(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) |= 0x02;
}

/**
  * @brief  disable bypass cache.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheBypassDis(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) &= 0xFD;
}

/**
  * @brief  get the cache enable status.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache enable status
  */
unsigned char CacheEnabled(unsigned long ulBase)
{
    return (((HWREGB(ulBase + CACHE_CCR0) & 0x3) == 1)? 1 : 0);
}

/**
  * @brief  enable cache statistic.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheStatisticEN(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) |= 0x04;
}

/**
  * @brief  disable cache statistic.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheStatisticDis(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) &= 0xFB;
}

/**
  * @brief  set cache write back policy.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the policy of cache write back as following:
  *     @arg CACHE_CCR0_WRITE_THROUGH
  *     @arg CACHE_CCR0_WRITE_BACK
  * @retval None
  */
void CacheWbPolicySet(unsigned long ulBase, unsigned char ucValue)
{
	HWREGB(ulBase + CACHE_CCR0) = (HWREGB(ulBase + CACHE_CCR0) & 0xF7) | ucValue;
}

/**
  * @brief  enable the cache entry address range.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheEntryCacheEN(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) |= 0x10;
}

/**
  * @brief  disable the cache entry address range.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheEntryCacheDis(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR0) &= 0xEF;
}

/**
  * @brief  enable the cache flush.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheFlushEn(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR1) |= 0x04;
}

/**
  * @brief  enable the cache check internal status.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheCheckEn(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR1) |= 0x08;
}

/**
  * @brief  enable the cache write clean.
  * @param  ulBase is the base address of the cache module.
  * @retval None
  */
void CacheWRCleanEn(unsigned long ulBase)
{
	HWREGB(ulBase + CACHE_CCR1) |= 0x10;
}

/**
  * @brief  get the cache enable status.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache enable status, 0:cache is not enable, 1:cache has been enabled.
  */
unsigned char CacheEnableStatusGet(unsigned long ulBase)
{
	return (HWREGB(ulBase + CACHE_SR0) & 0x01);
}

/**
  * @brief  get the cache flush status.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache flush status, 0:flush is idle,  1:flush is ongoing.
  */
unsigned char CacheFlushStatusGet(unsigned long ulBase)
{
	return (HWREGB(ulBase + CACHE_SR0) & 0x02);
}

/**
  * @brief  get the cache operation status.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache operation status.
  *         bit[0] clean_all_done
  *         bit[1] clean_invalid_all_done
  *         bit[2] flush_done
  *         bit[3] check_done
  *         bit[4] wr_clean_done
  */
unsigned char CacheOperationStatusGet(unsigned long ulBase)
{
	return HWREGB(ulBase + CACHE_SR1);
}

/**
  * @brief  get the cache line valid.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache line valid.
  */
unsigned char CacheLineValidGet(unsigned long ulBase)
{
	return (HWREGB(ulBase + CACHE_SR2) & 0x0F);
}

/**
  * @brief  get the cache line dirty.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache line dirty.
  */
unsigned char CacheLineDirtyGet(unsigned long ulBase)
{
	return (HWREGB(ulBase + CACHE_SR2) >> 4);
}

/**
  * @brief  get the cache read hit count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache read hit count
  */
unsigned long CacheRDHitCntGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_RD_HITCNT);
}

/**
  * @brief  get the cache read miss count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache read miss count
  */
unsigned long CacheRDMissCntGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_RD_MISSCNT);
}

/**
  * @brief  set the cache read hit count.
  * @param  ulBase is the base address of the cache module.
  * @param  value is the cache read hit count
  * @retval None
  */
void CacheRDHitCntSet(unsigned long ulBase, unsigned long value)
{
    HWREG(ulBase + CACHE_RD_HITCNT) = value;
}

/**
  * @brief  set the cache read miss count.
  * @param  ulBase is the base address of the cache module.
  * @param  value is the cache read miss count
  * @retval None
  */
void CacheRDMissCntSet(unsigned long ulBase, unsigned long value)
{
    HWREG(ulBase + CACHE_RD_MISSCNT) = value;
}

/**
  * @brief  set the cache way index.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the cache way index.
  * @retval None
  */
void CacheWaysIdxSet(unsigned long ulBase, unsigned char ucValue)
{
	HWREGB(ulBase + CACHE_IDX) = (HWREGB(ulBase + CACHE_IDX) & 0xF0) | (ucValue & 0x0F);
}

/**
  * @brief  get the cache way index.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache way index.
  */
unsigned char CacheWaysIdxGet(unsigned long ulBase)
{
	return (HWREGB(ulBase + CACHE_IDX) & 0x0F);
}

/**
  * @brief  set the 'cache set' index.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the 'cache set' index.
  * @retval None.
  */
void CacheSetIdxSet(unsigned long ulBase, unsigned long ulValue)
{
	HWREG(ulBase + CACHE_IDX) = (HWREG(ulBase + CACHE_IDX) & 0xF) | (ulValue<<4);
}

/**
  * @brief  get the 'cache set' index.
  * @param  ulBase is the base address of the cache module.
  * @retval the 'cache set' index.
  */
unsigned long CacheSetIdxGet(unsigned long ulBase)
{
	return (HWREG(ulBase + CACHE_IDX) >> 4);
}

/**
  * @brief  get the cache error log of address.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache error log of address.
  */
unsigned long CacheElrAddrGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_ELR_ADDR);
}

/**
  * @brief  set the cache CRE.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the cache CRE.
  * @retval None.
  */
void CacheCRESet(unsigned long ulBase, unsigned long ulValue)
{
	HWREG(ulBase + CACHE_CRE) = ulValue;
}

/**
  * @brief  get the cache CRE.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache CRE.
  */
unsigned long CacheCREGet(unsigned long ulBase)
{
	return (HWREG(ulBase + CACHE_CRE) & 0xFFFF);
}

/**
  * @brief  get the cache write hit count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache write hit count.
  */
unsigned long CacheWRHitCntGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_WR_HITCNT);
}

/**
  * @brief  Set the cache write hit count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache write hit count.
  */
unsigned long CacheWRHitCntSet(unsigned long ulBase, unsigned long value)
{
    HWREG(ulBase + CACHE_WR_HITCNT) = value;
    return 0;
}

/**
  * @brief  get the cache write miss count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache write miss count.
  */
unsigned long CacheWRMissCntGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_WR_MISS);
}

/**
  * @brief  Set the cache write miss count.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache write miss count.
  */
unsigned long CacheWRMissCntSet(unsigned long ulBase, unsigned long value)
{
	HWREG(ulBase + CACHE_WR_MISS) = value;
    return 0;
}
/**
  * @brief  get the cache param.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache param
  */
unsigned short CacheParamsGet(unsigned long ulBase)
{
    return HWREGH(ulBase + CACHE_PARAMS0);
}

/**
  * @brief  set the cache begin address.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the cache begin address.
  * @retval None.
  */
void CacheBAddrSet(unsigned long ulBase, unsigned long ulValue)
{
	HWREG(ulBase + CACHE_BADDR) = ulValue;
}

/**
  * @brief  get the cache begin address.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache begin address.
  */
unsigned long CacheBAddrGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_BADDR);
}

/**
  * @brief  set the cache termination address.
  * @param  ulBase is the base address of the cache module.
  * @param  ucValue is the cache termination address.
  * @retval None.
  */
void CacheTAddrSet(unsigned long ulBase, unsigned long ulValue)
{
	HWREG(ulBase + CACHE_TADDR) = ulValue;
}

/**
  * @brief  get the cache termination address.
  * @param  ulBase is the base address of the cache module.
  * @retval the cache termination address.
  */
unsigned long CacheTAddrGet(unsigned long ulBase)
{
	return HWREG(ulBase + CACHE_TADDR);
}

/**
  * @brief  Registers an interrupt handler for Cache interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @param  pfnHandler is a pointer to the function to be called when the Cache interrupt occurs.
  * @retval None
  */
void CacheIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_CACHE, g_pRAMVectors, pfnHandler);

    IntEnable(INT_CACHE);
}

/**
  * @brief  Unregisters an interrupt handler for the Cache interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void CacheIntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_CACHE);

    IntUnregister(INT_CACHE, g_pRAMVectors);
}

/**
  * @brief  set the Cache Interrupt mask
            0: unmask corresponding interrupt
            1: mask corresponding interrupt.
  * @param  ulBase is the base address of the cache module.
  * @param  ucConfig is the Cache Interrupt config.
  *   This parameter can be any combination of the following values:
  *     @arg CACHE_IRQM_ERR_RESP_Msk
  *     @arg CACHE_IRQM_ERR_WRTHR_Msk
  *     @arg CACHE_IRQM_ERR_FLUSH_Msk
  * @retval None
  */
void CacheIntMask(unsigned long ulBase, unsigned char ucConfig)
{
    HWREGB(ulBase + CACHE_IRQM) = ucConfig;
}

/**
  * @brief  Get the interrupt status and clear the Cache interrupt.
  * @param  ulBase is the base address of the cache module.
  * @retval the Cache interrupt status
  */
unsigned char CacheReadAndClearInt(unsigned long ulBase)
{
    return HWREGB(ulBase + CACHE_IRQS);
}


/**
  * @brief  set the cache address range.
  * @param  ulBase is the base address of the cache module.
  * @param  ulBAddr is the cache begin address.
  * @param  ulTAddr is the cache termination address.
  * @retval None.
  */
void CacheSetRange(unsigned long ulBase, unsigned long ulBAddr, unsigned long ulTAddr)
{
    HWREG(ulBase + CACHE_BADDR) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR) = ulTAddr;
	HWREG(ulBase + CACHE_ATTR) = 0x1;//use range 0
}

/**
  * @brief  set the cache address range.
  * @param  ulBase is the base address of the cache module.
  * @param  ulBAddr is the cache begin address.
  * @param  ulTAddr is the cache termination address.
  * @param  index is the range No (0-7)termination address.
  * @retval None.
  */
void CacheSetRangeIndex(unsigned long ulBase, unsigned long ulBAddr, unsigned long ulTAddr,unsigned char index)
{
	switch(index)
	{
		case 0://use range 0
    HWREG(ulBase + CACHE_BADDR) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x1;
		break;
		case 1://use range 1   
    HWREG(ulBase + CACHE_BADDR1) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR1) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x2;
		break;
		case 2://use range 2   
    HWREG(ulBase + CACHE_BADDR2) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR2) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x4;
		break; 
		case 3://use range 3   
    HWREG(ulBase + CACHE_BADDR3) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR3) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x8;
		break; 
		case 4://use range 4   
    HWREG(ulBase + CACHE_BADDR4) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR4) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x10;
		break; 
		case 5://use range 5   
    HWREG(ulBase + CACHE_BADDR5) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR5) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x20;
		break; 
		case 6://use range 6   
    HWREG(ulBase + CACHE_BADDR6) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR6) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x40;
		break; 
		case 7://use range 7   
    HWREG(ulBase + CACHE_BADDR7) = ulBAddr;
    HWREG(ulBase + CACHE_TADDR7) = ulTAddr;
	  HWREG(ulBase + CACHE_ATTR) |= 0x80;
		break; 		
		default:
				break;  
	}
}

/**
  * @brief  clear the cache event.
  * @param  ulBase is the base address of the cache module.
  * @retval None.
  */
void CacheEventClear(unsigned long ulBase)
{
    HWREG(ulBase + CACHE_RD_HITCNT) = 0x0;
    HWREG(ulBase + CACHE_RD_MISSCNT) = 0x0;
    HWREG(ulBase + CACHE_WR_HITCNT) = 0x0;
    HWREG(ulBase + CACHE_WR_MISS) = 0x0;
    HWREG(ulBase + CACHE_CCR0) |= 0x04;  //enable statistic
}

/**
  * @brief  write back the cache data to the sram.
  * @param  ulBase is the base address of the cache module.
  * @retval None.
  */
void CacheCleanAll(unsigned long ulBase)
{
    if (CacheEnabled(ulBase)) {
        HWREGB(ulBase + CACHE_CCR1) = 0x01;
        while((CacheOperationStatusGet(ulBase) & 0x01) == 0); //wait until clean_all done
    }
}

/**
  * @brief  write back the cache data to the sram, and clear the cache data.
  * @param  ulBase is the base address of the cache module.
  * @retval None.
  */
void CacheCleanInvalidAll(unsigned long ulBase)
{
    if (CacheEnabled(ulBase)) {
        HWREGB(ulBase + CACHE_CCR1) |= 0x02;
        while((CacheOperationStatusGet(ulBase) & 0x02) == 0); //wait until clean_invalid_all done
    }
}

/**
  * @brief  clear the cache data.
  * @param  ulBase is the base address of the cache module.
  * @retval None.
  */
void CacheFlush(unsigned long ulBase)
{
    if (CacheEnabled(ulBase)) {
        HWREGB(ulBase + CACHE_CCR1) = 0x04;
        while((CacheOperationStatusGet(ulBase) & 0x04) == 0); //wait until flush done
    }
}

/**
  * @brief  init the cache data.
  * @param  ulBase is the base address of the cache module.
  * @param  ulBAddr is the cache begin address.
  * @param  ulTAddr is the cache termination address.
  * @param  ucWbPolicy is the policy of cache write back as following:
  *     @arg CACHE_CCR0_WRITE_THROUGH
  *     @arg CACHE_CCR0_WRITE_BACK
  * @retval None.
  */
void CacheInit(unsigned long ulBase, unsigned char ucWbPolicy, unsigned long ulBAddr, unsigned long ulTAddr)
{
    CacheBypassEN(ulBase);

    HWREGB(ulBase + CACHE_CCR1) = 0x04; //cache flush
    while((CacheOperationStatusGet(ulBase) & 0x04) == 0); //wait until flush done

    CacheEntryCacheEN(ulBase);
    
    CacheWbPolicySet(ulBase, ucWbPolicy);
    
    CacheSetRange(ulBase, ulBAddr, ulTAddr);

    CacheEventClear(ulBase);

    CacheCRESet(ulBase, 0);
    
    CacheBypassDis(ulBase);
    
    CacheEN(ulBase);
}

void CacheInit_Ext(unsigned long ulBase, unsigned char ucWbPolicy, unsigned long ulBAddr, unsigned long ulTAddr)
{
        (void)ucWbPolicy;
		//HWREGB(ulBase + CACHE_CCR0) |= 0x02;
		HWREGB(ulBase + CACHE_CCR1) = 0x04; //cache flush
		
		//HWREGB(ulBase + CACHE_CCR0) |= 0x10;
		//HWREGB(ulBase + CACHE_CCR0) = (HWREGB(ulBase + CACHE_CCR0) & 0xF7) | ucWbPolicy;
		HWREG(ulBase + CACHE_BADDR) = ulBAddr;
		HWREG(ulBase + CACHE_TADDR) = ulTAddr;
		HWREG(ulBase + CACHE_ATTR) = 0x1;//use range 0
		// HWREG(ulBase + CACHE_RD_HITCNT) = 0x0;
		// HWREG(ulBase + CACHE_RD_MISSCNT) = 0x0;
		// HWREG(ulBase + CACHE_WR_HITCNT) = 0x0;
		// HWREG(ulBase + CACHE_WR_MISS) = 0x0;
		// HWREG(ulBase + CACHE_CCR0) |= 0x04;  //enable statistic
		HWREG(ulBase + CACHE_CRE) = 0;
        while((HWREGB(ulBase + CACHE_SR1) & 0x04) == 0); //wait until flush done
		//HWREGB(ulBase + CACHE_CCR0) &= 0xFD;
		HWREGB(ulBase + CACHE_CCR0) = 0x11;//CACHE_CCR0_WRITE_THROUGH
}

