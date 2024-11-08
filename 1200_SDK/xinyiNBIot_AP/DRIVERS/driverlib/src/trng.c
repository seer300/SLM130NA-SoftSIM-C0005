#include "trng.h"

void TRNG_Resetblock(void)
{
    TRNG->SW_RESET = HW_RNG_SW_RESET_REG_SET;
}

void TRNG_EnableHwRngClock(void)
{
    TRNG->CLK_ENABLE = HW_RNG_CLK_ENABLE_REG_SET;
}

void TRNG_SetSamlpeCountValue(uint32_t countValue)
{
	TRNG->SAMPLE_CNT1 = countValue;
}

uint32_t TRNG_ReadSamlpeCountValue(void)
{
    uint32_t countValue;
	countValue = TRNG->SAMPLE_CNT1;
	return countValue;
}

void TRNG_SetRngRoscLength(uint32_t roscLength)
{
	TRNG->CONFIG_REG = roscLength;
}

void TRNG_FastModeBypass(void)
{
	TRNG->DEBUG_CONTROL = HW_TRNG_DEBUG_CONTROL_REG_FAST;
}

void TRNG_FeModeBypass(void)
{
	TRNG->DEBUG_CONTROL = HW_TRNG_DEBUG_CONTROL_REG_FE;
}

void TRNG_80090bModeBypass(void)
{
	TRNG->DEBUG_CONTROL = HW_TRNG_DEBUG_CONTROL_REG_80090B;
}

void TRNG_EnableRndSource(void)
{
	TRNG->RND_SOURCEE = HW_RND_SOURCE_ENABLE_REG_SET;
}

void TRNG_DisableRndSource(void)
{
	TRNG->RND_SOURCEE = HW_RND_SOURCE_ENABLE_REG_CLR;
}

uint32_t TRNG_ReadValidReg(void)
{
    uint32_t validValue;
	validValue = TRNG->VALID_REG;
	return validValue;
}

uint32_t TRNG_ReadValidISRReg(void)
{
    uint32_t validValue;
	validValue = TRNG->ISR_REG;
	return validValue;
}

uint32_t TRNG_ReadEHR_Data(uint32_t offSet)
{
    uint32_t ehr_Data;
	ehr_Data = HWREG((TRNG_EHR_DATA_ADDR_0+offSet));
	return ehr_Data;
}


void TRNG_CleanUpInterruptStatus(void)
{
	TRNG->ISR_REG = (~0UL);
}

void TRNG_CollectU8Array(uint32_t p, uint8_t* u8Array, uint32_t* u32Array)
{
	u8Array[4*p+3] = (uint8_t)(u32Array[p]);
	u8Array[4*p+2] = (uint8_t)(u32Array[p]>>8);
	u8Array[4*p+1] = (uint8_t)(u32Array[p]>>16);
	u8Array[4*p+0] = (uint8_t)(u32Array[p]>>24);
}

int TRNG_CheckInput(uint32_t TRNGMode,
                    uint32_t roscLength,
                    uint32_t sampleCount)
{
	/* ............... validate inputs .................................... */
	   if (TRNGMode > TRNG_MODE_80090B)
	   {
		   return CC_TRNG_INVALID_PARAM_TRNG_MODE;
	   }
	
	   if (roscLength > TRNG_ROSC_MAX_LENGTH)
	   {
		   return CC_TRNG_INVALID_PARAM_ROSC_LEN;
	   }
	
	   if (sampleCount < MINIUM_SAMPLE_CNT)
	   {
		   return CC_TRNG_INVALID_PARAM_SAMPLE_CNT;
	   }
	   return CC_TRNG_INVALID_PARAM_OK;
}

void TRNG_InitializingHardware(uint32_t TRNGMode,uint32_t roscLength,uint32_t sampleCount )
{
	uint32_t tmpSampleCnt = 0;

	TRNG_Resetblock();
						
	/* enable RNG clock and set sample counter value untile it is set correctly*/
	do
	{
		TRNG_EnableHwRngClock();
								
		TRNG_SetSamlpeCountValue(sampleCount);
						
						
		tmpSampleCnt = TRNG_ReadSamlpeCountValue();
								
	}while (tmpSampleCnt != sampleCount); 
	/* wait until the sample counter is set correctly*/
						
	TRNG_SetRngRoscLength(roscLength);
						
	/* configure TRNG debug control register based on different mode. */
	if (TRNGMode == TRNG_MODE_FAST)
	{
		/* fast TRNG: bypass VNC, CRNGT and auto correlate, activate none. */
		TRNG_FastModeBypass();
	}
	else if (TRNGMode == TRNG_MODE_FE)
	{
		/* FE TRNG: bypass none, activate all */
		TRNG_FeModeBypass();
	}
	else if (TRNGMode == TRNG_MODE_80090B)
	{
		/* 800-90B TRNG: bypass VNC and auto correlate, activate CRNGT */
		TRNG_80090bModeBypass();
	}
						
	TRNG_EnableRndSource();


}

void TRNG_StopHardware(void)
{
	TRNG_DisableRndSource();
}

uint8_t TRNG_LoadEHR(uint8_t* dataU8Array)
{
    uint32_t EhrSizeInWords = EHR_SIZE_IN_WORDS;
	/* loop variable */
    uint32_t j = 0;
	/*return value*/
    uint32_t Error = 0;
	
		uint32_t dataArray[TRNG_BUFFER_SIZE_IN_WORDS] = {0};
		uint32_t *dataBuff_ptr = dataArray;

				uint32_t valid_at_start, valid;
				/*TRNG data collecting must be continuous.*/
				valid_at_start = TRNG_ReadValidReg();
					
				valid = valid_at_start;
				/*bit[0]-EHR_VALID, bit[1]-AUTOCORR_ERR, bit[2]-CRNGT_ERR, bit[3]-VN_ERR*/
				while ((valid & (HW_RNG_ISR_REG_EHR_VALID|HW_RNG_ISR_REG_AUTOCORR_ERR)) == 0x0)
				{
					valid = TRNG_ReadValidISRReg();
				}
	
				/*pass bit[31:1] to Error. mask out bit[0] which is used for CC_TRNG_SAMPLE_LOST*/
				if ((valid & ~CC_TRNG_SAMPLE_LOST) != 0)
				{
					Error |= (valid & ~CC_TRNG_SAMPLE_LOST);
				}
		
				if (Error & HW_RNG_ISR_REG_AUTOCORR_ERR)
				{
					return Error; /* autocorrelation error is irrecoverable */
				}
		
				TRNG_CleanUpInterruptStatus();
		
				/*load the current random data from EHR registers to the output buffer.*/
				for (j=0; j<EhrSizeInWords; j++)
				{
							*(dataBuff_ptr++) = TRNG_ReadEHR_Data(j*sizeof(uint32_t));
							/* load the current random data from EHR to dataArray  */
							TRNG_CollectU8Array(j, dataU8Array, dataArray);
				}
			return Error;
		/* END TIMING: end time measurement at this point */
}

void TRNG_IntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void))
{
    IntRegister(INT_TRNG, g_pRAMVectors, pfnHandler);

    IntEnable(INT_TRNG);
}

/**
  * @brief  Unregisters an interrupt handler for the TickTimer interrupt.
  * @param  g_pRAMVectors is the global interrupt vectors table.
  * @retval None
  */
void TRNG_IntUnregister(unsigned long *g_pRAMVectors)
{
    IntDisable(INT_TRNG);

    IntUnregister(INT_TRNG, g_pRAMVectors);
}

void TRNG_ConfigSmpcntRndsel(uint8_t sample_cnt_indx,uint8_t rnd_sel_indx)
{
	TRNG->IMR_REG = 0xFFFFFF00;
	TRNG->RND_SOURCEE = 0x00000000;
	TRNG->DEBUG_CONTROL = 0x00;
	TRNG->CONFIG_REG = rnd_sel_indx;
	while(TRNG->CONFIG_REG != rnd_sel_indx);
	TRNG->SAMPLE_CNT1 = sample_cnt_indx;
	while(TRNG->SAMPLE_CNT1 != sample_cnt_indx);
	TRNG->AUTOCORR_STATISTIC = 0x0;
	TRNG->RND_SOURCEE = 0x1;
	while(TRNG->RND_SOURCEE != 0x1);
}
