#include "crypto.h"
#include "dma.h"
#include "system.h"

/* For CMAC Calcation */
const uint8_t AES_CMAC_Rb[AES_BLOCK_BYTES] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};

/**
  * @brief Enable the clock of AES is controled by hardware
  * @param None
  * @retval None
  */
void AES_HardwareCtrlClock(void)
{
    AES->CTL |= AES_CTL_FORCE_CLK_EN;
}

/**
  * @brief The clock of AES is always on.
  * @param None
  * @retval None
  */
void AES_ClockAlwaysOn(void)
{
    AES->CTL &= ~AES_CTL_FORCE_CLK_EN;
}

/**
  * @brief Set the length of the key.
  * @param KeyLenMode: The value of length 
  * @retval None
  */
void AES_KeyLenSet(uint32_t KeyLenMode)
{
	AES->CTL &= ~AES_CTL_KEY_LEN_Msk;
	AES->CTL |= KeyLenMode;
}
   
/**
  * @brief Set the the key.
  * @param Offset: the offset of the word block
  * @param KeyValue: the message of the key
  * @retval None
  */
void AES_KeySet(uint8_t Offset, uint32_t KeyValue)
{
	AES->KEY[Offset] = KeyValue;
}

/**
  * @brief Set the the IV.
  * @param Offset: the offset of the word block
  * @param IV: the message of the IV
  * @retval None
  */
void AES_IVSet(uint8_t Offset, uint32_t IV)
{
	AES->IV[Offset] = IV;
}

/**
  * @brief Set the additional authenticated data.
  * @param Offset: the offset of the word block
  * @param AadLen: The length0 of additional data 
  * @retval None
  */
void AES_AADLenSet(uint8_t Offset, uint32_t AadLen)
{
	AES->ADDLEN[Offset] = AadLen;
}

/**
  * @brief Set the byte length of the payload.
  * @param ByteLen: The byte length of payload 
  * @retval None
  */
void AES_PayloadLenSet(uint32_t ByteLen)
{
	AES->PLEN = ByteLen;
}

/**
  * @brief Set the AES mode.
  * @param AESMode: The mode
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_MODE_ECB 
  *     @arg AES_CTL_MODE_CBC 
  *     @arg AES_CTL_MODE_CFB
  *     @arg AES_CTL_MODE_OFB
  *     @arg AES_CTL_MODE_CTR
  *     @arg AES_CTL_MODE_CCM
  * @retval None
  */
void AES_ModeSet(uint32_t AESMode)
{
	AES->CTL &= ~AES_CTL_MODE_Msk;
	AES->CTL |= AESMode;
}

/**
  * @brief Set the AES mode.
  * @param AESEncDec: The operation
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_ENC: encryption
  *     @arg AES_CTL_DEC: decryption
  * @retval None
  */
void AES_EncDecSet(uint32_t AESEncDec)
{
	AES->CTL &= ~AES_CTL_ENC_DEC_Msk;
	AES->CTL |= AESEncDec;
}

/**
  * @brief Set the cipher mode.
  * @param Mode: The operation
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_SZ_SEL_AES: AES encrypt
  *     @arg AES_CTL_SZ_SEL_S3G: Snow3G
  *     @arg AES_CTL_SZ_SEL_ZUC: ZUC
  * @retval None
  */
void AES_SetCipherMode(uint8_t Mode)
{
	AES->CTL &= (~AES_CTL_SZ_SEL_Msk);
    AES->CTL |= ((Mode & 0x3) << AES_CTL_SZ_SEL_Pos);
}

/**
  * @brief Enable the DMA mode of AES.
  * @param None
  * @retval None
  */
void AES_DMAEn(void)
{
	AES->CTL |= AES_CTL_DMA_EN;
}

/**
  * @brief Disable the DMA mode of AES.
  * @param None
  * @retval None
  */
void AES_DMADis(void)
{
	AES->CTL &= (~AES_CTL_DMA_EN);
}

/**
  * @brief Input the data for encrypt.
  * @param Offset: The offset of the input address
  * @param DataIn: The input data 
  * @retval None
  */
void AES_BlockDataInput(uint8_t Offset, uint32_t DataIn)
{
	HWREG(AES_DATA_IN_BASE + Offset) = DataIn;
}

/**
  * @brief Get the encryption rest of the data.
  * @param Offset: The offset of the output address
  * @retval Return the encryption rest
  */
uint32_t AES_BlockDataOutput(uint8_t Offset)
{
    return (HWREG(AES_DATA_OUT_BASE + Offset));
}

/**
  * @brief Start up the AES.
  * @param None
  * @retval None
  */
void AES_BlockStart(void)
{
	AES->CTL |= AES_CTL_START;
}

/**
  * @brief Start and wait for key scheding to complete.
  * @note This function is only for decryption of ECB and CBC mode
  * @param  None
  * @retval None
  */
void AES_KeyLoadWaitDone(void)
{
	/* Start the key schede  */
	AES->CTL |= AES_CTL_KEY_LOAD;
    
	/* Blocking until key scheding to complete*/
    while(!(AES->DONE & AES_DONE_KEY_DONE));

}

/**
  * @brief Wait for AES to complete.
  * @param  None
  * @retval None
  */
void AES_BlockTransWaitDone(void)
{
	/* Blocking until AES to complete */
    while(!(AES->STA & AES_STA_AES_STA_DONE));
}

/**
  * @brief Wait for the encryption modes for SNOW3G and ZUC to be ready
  * @param  None
  * @retval None
  */
void S3G_ZUC_TransWaitDone(void)
{
	while(!(AES->S3G_ZUC_STATUS & SZ_STA_S3G_ZUC_DONE))
    {
    }
}

/**
  * @brief Enable the clk divide 2 for AES.
  * @param None
  * @retval None
  */
void AES_ClockDiv2En(void)
{
	AES->CTL |= AES_CTL_CLK_DIV_EN;
}

/**
  * @brief Disable the clk divide 2 for AES.
  * @param None
  * @retval None
  */
void AES_ClockDiv2Dis(void)
{
	AES->CTL &= (~AES_CTL_CLK_DIV_EN);
}

/**
  * @brief Enable integrity authentication based on ZUC
  * @param None
  * @retval None
  */
void ZUCIntegrityEn(void)
{
	AES->CTL |= AES_CTL_EIA_Msk;
	AES->CTL &= ~AES_CTL_UIA_Msk;
}
/**
  * @brief Enable encryption based on ZUC
  * @param None
  * @retval None
  */
void ZUCEncrypteEn(void)
{
	AES->CTL &= ~AES_CTL_EIA_Msk;
	AES->CTL &= ~AES_CTL_UIA_Msk;
	
}

/**
  * @brief Enable integrity authentication based on SNOW3G
  * @param None
  * @retval None
  */
void Snow3GIntegrityEn(void)
{
	AES->CTL |= AES_CTL_UIA_Msk;
	AES->CTL &= ~AES_CTL_EIA_Msk;
}

/**
  * @brief Enable encryption based on SNOW3G
  * @param None
  * @retval None
  */
void Snow3GEncrypteEn(void)
{   
	AES->CTL &= ~AES_CTL_UIA_Msk;
    AES->CTL &= ~AES_CTL_EIA_Msk;
}

/**
  * @brief Clear the bit of UIA and EIA 
  * @note when AES encrypts and decrypts, the uia_en and eia_en must be 0
  * @param None
  * @retval None
  */
void AES_ClearUiaAndEia(void)
{
    AES->CTL &= ~(AES_CTL_EIA_Msk | AES_CTL_UIA_Msk);
}

void AES_EndianClear(void)
{
    AES->CTL &=~(AES_CTL_INPUT_ENDIAN_Msk | AES_CTL_OUTPUT_ENDIAN_Msk | AES_CTL_KEY_ENDIAN_Msk | AES_CTL_IV_ENDIAN_Msk | AES_CTL_S3G_ZUC_ENDIAN_Msk); 
}

/**
  * @brief Enable the small endian of input
  * @param  None
  * @retval None
  */
void AES_EndianInSmall(void)
{
    AES->CTL |= AES_CTL_INPUT_ENDIAN_Msk;
}

/**
  * @brief Enable the big endian of input
  * @param  None
  * @retval None
  */
void AES_EndianInBig(void)
{
    AES->CTL &= ~AES_CTL_INPUT_ENDIAN_Msk;
}

/**
  * @brief Enable the samall endian output of Snow3G and ZUC 
  * @param  None
  * @retval None
  */
void SZ1_EndianOutSmall(void)
{
    AES->CTL |= AES_CTL_S3G_ZUC_ENDIAN_Msk;
}

/**
  * @brief Enable the big endian output of Snow3G and ZUC 
  * @param  None
  * @retval None
  */
void SZ1_EndianOutBig(void)
{
	AES->CTL &= ~AES_CTL_S3G_ZUC_ENDIAN_Msk;
}

/**
  * @brief Enable the small endian of output
  * @note This function is only for EA(Encryption Algorithm)
  * @param  None
  * @retval None
  */
void SZ2_EndianOutSmall(void)
{
    AES->CTL |= AES_CTL_OUTPUT_ENDIAN_Msk;
}

/**
  * @brief Enable the big endian of output
  * @note This function is only for EA(Encryption Algorithm)
  * @param  None
  * @retval None
  */
void SZ2_EndianOutBig(void)
{
    AES->CTL &= ~AES_CTL_OUTPUT_ENDIAN_Msk;
}

/**
  * @brief Set the length of Tag.
  * @note This function is only for CCM mode
  * @param AuthLenByte: The value of length 
  * @retval None
  */
void AES_CCMAuthLenSet(uint32_t AuthLenByte)
{
	AES->CTL &= ~AES_CTL_CCM_ADATA_LEN_Msk;
    AES->CTL |= (AuthLenByte << AES_CTL_CCM_ADATA_LEN_Pos);
}

/**
  * @brief Set the length of CCM load.
  * @note This function is only for CCM mode
  * @param ByteLength: The value of length 
  * @retval None
  */
void AES_CCMLengthLenSet(uint8_t ByteLength)
{
	AES->CTL &= ~AES_CTL_CCM_LOAD_LEN_Msk;
	AES->CTL |= (ByteLength << AES_CTL_CCM_LOAD_LEN_Pos);
}

/**
  * @brief Get the Tag(Mac) message.
  * @param Offset: The offset of Tag register 
  * @retval The value
  */
uint32_t AES_TagGet(uint8_t Offset)
{
    return (AES->TAG[Offset]);
}

/**
  * @brief Configure the AES block.
  * @param pInput: A pointer to the input plaintext  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key 
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_KEY_LEN_128 
  *     @arg AES_CTL_KEY_LEN_192 
  *     @arg AES_CTL_KEY_LEN_256
  * @param pOutput: A pointer to the output ciphertext 
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_MODE_ENCRYPT: encryption
  *     @arg AES_MODE_DECRYPT: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
void AESBlock(uint8_t *pInput, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp;

	AES->STA; // for pclk!=hclk 
	
	AES_SetCipherMode(CIPHER_MODE_AES);
	AES_ClearUiaAndEia();
	AES_EndianClear();

	/* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_ECB);

	AES_EncDecSet(OperateMode);

	if(OperateMode == AES_CTL_DEC)
	{
		AES_KeyLoadWaitDone();
	}
    
//    AES_ClockDiv2Dis();
    
	/* Set the length of input data*/
    AES_PayloadLenSet(AES_BLOCK_BYTES);
    	
	/* Set the length of the key */
    AES_KeyLenSet(KeyBitsLen);
	
	/* Set the key value*/
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    }   
	
    if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
 
	/* AES transfer by DMA */ 
	if(CoreTypeOrIO) 
	{
		/* Enable DMA mode */ 
		AES_DMAEn();
		
		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	
	    /* Start AES */ 
		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, AES_BLOCK_BYTES, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, AES_BLOCK_BYTES, CoreTypeOrIO);
		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);

		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		/* Disable DMA mode */ 
		AES_DMADis();
		
	}
	/* AES transfer by IO */ 
	else
	{
		AES_BlockDataInput( 0, pInput[3]  << 24 | pInput[2]  << 16 | pInput[1]  << 8 | pInput[0]);
		AES_BlockDataInput( 4, pInput[7]  << 24 | pInput[6]  << 16 | pInput[5]  << 8 | pInput[4]);
		AES_BlockDataInput( 8, pInput[11] << 24 | pInput[10] << 16 | pInput[9]  << 8 | pInput[8]);
		AES_BlockDataInput( 12, pInput[15] << 24 | pInput[14] << 16 | pInput[13] << 8 | pInput[12]);
		
		/* Start AES */ 
		AES_BlockStart();
		
		AES_BlockTransWaitDone();
		
		tmp = AES_BlockDataOutput(0);
		pOutput[0] = (uint8_t)tmp;
		pOutput[1] = (uint8_t)(tmp >> 8);
		pOutput[2] = (uint8_t)(tmp >> 16);
		pOutput[3] = (uint8_t)(tmp >> 24);
		
		tmp = AES_BlockDataOutput(4);
		pOutput[4] = (uint8_t)tmp;
		pOutput[5] = (uint8_t)(tmp >> 8);
		pOutput[6] = (uint8_t)(tmp >> 16);
		pOutput[7] = (uint8_t)(tmp >> 24);
		
		tmp = AES_BlockDataOutput(8);
		pOutput[8]  = (uint8_t)tmp;
		pOutput[9]  = (uint8_t)(tmp >> 8);
		pOutput[10] = (uint8_t)(tmp >> 16);
		pOutput[11] = (uint8_t)(tmp >> 24);
		
		tmp = AES_BlockDataOutput(12);
		pOutput[12] = (uint8_t)tmp;
		pOutput[13] = (uint8_t)(tmp >> 8);
		pOutput[14] = (uint8_t)(tmp >> 16);
		pOutput[15] = (uint8_t)(tmp >> 24);
	}
}

/**
  * @brief Configure the ECB mode of AES.
  * @param pInput: A pointer to the input plaintext  
  * @param InputLenByte: The byte length of input data  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pOutput: A pointer to the output ciphertext  
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_MODE_ENCRYPT: encryption
  *     @arg AES_MODE_DECRYPT: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
__FLASH_FUNC void AESECB(uint8_t *pInput, uint32_t InputLenByte, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp, i;
    uint32_t LenBlock;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_ECB);
		    
    AES_EncDecSet(OperateMode);	
    
	//AES_ClockDiv2Dis();
    
	/* Set the byte length of input data */
    LenBlock = (InputLenByte + 15) >> 4;
    AES_PayloadLenSet(LenBlock << 4);
    
	/* Set the bit length of the key */
	AES_KeyLenSet(KeyBitsLen);	
	
	/* Set the key */
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    } 
	
    if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
	
	if(OperateMode == AES_CTL_DEC)
    {
        AES_KeyLoadWaitDone();
    }
	
    /* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, InputLenByte, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, InputLenByte, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);
		
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else
	{
		/* AES IO Mode */
		for(i = 0; i < LenBlock; i++)
		{
			AES_BlockDataInput( 0, pInput[16*i+3]  << 24 | pInput[16*i+2]  << 16 | pInput[16*i+1]  << 8 | pInput[16*i+0]);
			AES_BlockDataInput( 4, pInput[16*i+7]  << 24 | pInput[16*i+6]  << 16 | pInput[16*i+5]  << 8 | pInput[16*i+4]);
			AES_BlockDataInput( 8, pInput[16*i+11] << 24 | pInput[16*i+10] << 16 | pInput[16*i+9]  << 8 | pInput[16*i+8]);
			AES_BlockDataInput(12, pInput[16*i+15] << 24 | pInput[16*i+14] << 16 | pInput[16*i+13] << 8 | pInput[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}
}

/**
  * @brief Configure the CBC mode of AES.
  * @param pInput: A pointer to the input plaintext  
  * @param InputLenByte: The byte length of input data  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pIV: A pointer to the input initial vector  
  * @param pOutput: A pointer to the output ciphertext  
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_MODE_ENCRYPT: encryption
  *     @arg AES_MODE_DECRYPT: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
__FLASH_FUNC void AESCBC(uint8_t *pInput, uint32_t InputLenByte, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pIV, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp, i;
    uint32_t LenBlock;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_CBC);
		   
    AES_EncDecSet(OperateMode);	
    
	//AES_ClockDiv2Dis();
    
	/* Set the byte length of input data */
    LenBlock = (InputLenByte + 15) >> 4;
    AES_PayloadLenSet(LenBlock << 4);
    
	/* Set the bit length of the key */
	AES_KeyLenSet(KeyBitsLen);	
	
	/* Set the key */
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    } 
    
	if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
	
	if(OperateMode == AES_CTL_DEC)
    {
        AES_KeyLoadWaitDone();
    }
	
	/* Set the IV */
    AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
    /* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, InputLenByte, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, InputLenByte, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);
	
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else
	{
		/* AES IO Mode */
		for(i = 0; i < LenBlock; i++)
		{
			AES_BlockDataInput( 0, pInput[16*i+3]  << 24 | pInput[16*i+2]  << 16 | pInput[16*i+1]  << 8 | pInput[16*i+0]);
			AES_BlockDataInput( 4, pInput[16*i+7]  << 24 | pInput[16*i+6]  << 16 | pInput[16*i+5]  << 8 | pInput[16*i+4]);
			AES_BlockDataInput( 8, pInput[16*i+11] << 24 | pInput[16*i+10] << 16 | pInput[16*i+9]  << 8 | pInput[16*i+8]);
			AES_BlockDataInput(12, pInput[16*i+15] << 24 | pInput[16*i+14] << 16 | pInput[16*i+13] << 8 | pInput[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}
}

/**
  * @brief Configure the CFB mode of AES.
  * @param pInput: A pointer to the input plaintext  
  * @param InputLenByte: The byte length of input data  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pIV: A pointer to the input initial vector  
  * @param pOutput: A pointer to the output ciphertext  
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_ENC: encryption
  *     @arg AES_CTL_DEC: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
__FLASH_FUNC void AESCFB(uint8_t *pInput, uint32_t InputLenByte, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pIV, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp, i;
    uint32_t LenBlock;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_CFB);
	
	/* Set encryption/decryption */
	AES_EncDecSet(OperateMode);
    
	//AES_ClockDiv2Dis();
    
	/* Set the byte length of input data */
    LenBlock = (InputLenByte + 15) >> 4;
    AES_PayloadLenSet(LenBlock << 4);
    
	/* Set the bit length of the key */
	AES_KeyLenSet(KeyBitsLen);	
	
	/* Set the key */
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    } 
	
    if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
	
	/* Set the IV */
    AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
    /* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, InputLenByte, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, InputLenByte, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);	
	
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);	
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 				
		
		AES_DMADis();
	}
	else
	{
		/* AES IO Mode */
		for(i = 0; i < LenBlock; i++)
		{
			AES_BlockDataInput( 0, pInput[16*i+3]  << 24 | pInput[16*i+2]  << 16 | pInput[16*i+1]  << 8 | pInput[16*i+0]);
			AES_BlockDataInput( 4, pInput[16*i+7]  << 24 | pInput[16*i+6]  << 16 | pInput[16*i+5]  << 8 | pInput[16*i+4]);
			AES_BlockDataInput( 8, pInput[16*i+11] << 24 | pInput[16*i+10] << 16 | pInput[16*i+9]  << 8 | pInput[16*i+8]);
			AES_BlockDataInput(12, pInput[16*i+15] << 24 | pInput[16*i+14] << 16 | pInput[16*i+13] << 8 | pInput[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}
}

/**
  * @brief Configure the OFB mode of AES.
  * @param pInput: A pointer to the input plaintext  
  * @param InputLenByte: The byte length of input data  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pIV: A pointer to the input initial vector  
  * @param pOutput: A pointer to the output ciphertext  
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_ENC: encryption
  *     @arg AES_CTL_DEC: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
__FLASH_FUNC void AESOFB(uint8_t *pInput, uint32_t InputLenByte, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pIV, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp, i;
    uint32_t LenBlock;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_OFB);
		    
	/* Set encryption/decryption */
	AES_EncDecSet(OperateMode);
    
	//AES_ClockDiv2Dis();
    
	/* Set the byte length of input data */
    LenBlock = (InputLenByte + 15) >> 4;
    AES_PayloadLenSet(LenBlock << 4);
    
	/* Set the bit length of the key */
	AES_KeyLenSet(KeyBitsLen);	
	
	/* Set the key */
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    } 
	
    if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
	
	/* Set the IV */
    AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
    /* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, InputLenByte, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, InputLenByte, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);	
		
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 	
		
		AES_DMADis();
	}
	else
	{
		/* AES IO Mode */
		for(i = 0; i < LenBlock; i++)
		{
			AES_BlockDataInput( 0, pInput[16*i+3]  << 24 | pInput[16*i+2]  << 16 | pInput[16*i+1]  << 8 | pInput[16*i+0]);
			AES_BlockDataInput( 4, pInput[16*i+7]  << 24 | pInput[16*i+6]  << 16 | pInput[16*i+5]  << 8 | pInput[16*i+4]);
			AES_BlockDataInput( 8, pInput[16*i+11] << 24 | pInput[16*i+10] << 16 | pInput[16*i+9]  << 8 | pInput[16*i+8]);
			AES_BlockDataInput(12, pInput[16*i+15] << 24 | pInput[16*i+14] << 16 | pInput[16*i+13] << 8 | pInput[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}
}

/**
  * @brief Configure the OFB mode of AES.
  * @param pInput: A pointer to the input plaintext  
  * @param InputLenByte: The byte length of input data  
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pIV: A pointer to the input initial vector  
  * @param pOutput: A pointer to the output ciphertext  
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_ENC: encryption
  *     @arg AES_CTL_DEC: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
__FLASH_FUNC void AESCTR(uint8_t *pInput, uint32_t InputLenByte, uint8_t *pKey, uint32_t KeyBitsLen, uint8_t *pIV, uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{	
    uint32_t tmp, i;
    uint32_t LenBlock;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_CTR);
		    
	/* Set encryption/decryption */
	AES_EncDecSet(OperateMode);
    
	//AES_ClockDiv2Dis();
    
	/* Set the byte length of input data */
    LenBlock = (InputLenByte + 15) >> 4;
    AES_PayloadLenSet(LenBlock << 4);
    
	/* Set the bit length of the key */
	AES_KeyLenSet(KeyBitsLen);	
	
	/* Set the key */
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[16] << 24 | pKey[17] << 16 | pKey[18] << 8 | pKey[19]);
        AES_KeySet(5, pKey[20] << 24 | pKey[21] << 16 | pKey[22] << 8 | pKey[23]);
    } 
    
	if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[24] << 24 | pKey[25] << 16 | pKey[26] << 8 | pKey[27]);
        AES_KeySet(7, pKey[28] << 24 | pKey[29] << 16 | pKey[30] << 8 | pKey[31]);
    }
	
	/* Set the IV */
    AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
    /* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, InputLenByte, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, InputLenByte, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);		
	
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else
	{
		/* AES IO Mode */
		for(i = 0; i < LenBlock; i++)
		{
			AES_BlockDataInput( 0, pInput[16*i+3]  << 24 | pInput[16*i+2]  << 16 | pInput[16*i+1]  << 8 | pInput[16*i+0]);
			AES_BlockDataInput( 4, pInput[16*i+7]  << 24 | pInput[16*i+6]  << 16 | pInput[16*i+5]  << 8 | pInput[16*i+4]);
			AES_BlockDataInput( 8, pInput[16*i+11] << 24 | pInput[16*i+10] << 16 | pInput[16*i+9]  << 8 | pInput[16*i+8]);
			AES_BlockDataInput(12, pInput[16*i+15] << 24 | pInput[16*i+14] << 16 | pInput[16*i+13] << 8 | pInput[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}
}


/**
  * @brief Configure the CCM mode of AES.
  * @param pAdata: A pointer to the additional authenticated data  
  * @param AdataByteLen: The byte length of additional authenticated data 
  * @param pPayload: A pointer to the message to authenticate and encrypt/decrypt;   
  * @param PayloadByteLen: The byte length of payload
  * @param pKey: A pointer to the key
  * @param KeyBitsLen: The bits length of the key
  *   This parameter can be one of the following values:
  *     @arg AES_KEY_LEN_BITS_128 
  *     @arg AES_KEY_LEN_BITS_192 
  *     @arg AES_KEY_LEN_BITS_256
  * @param pNonce: A pointer to the nonce  
  * @param NonceByteLen: Number of octets in nonce, shod be 7~13;
  * @param TagByteLen: Number of octets in authentication field, shod be 4/6/8/10/12/14/16 
  * @param pOutput: A pointer to the output ciphertext  
  * @param pDigest: A pointer to the integrity certification rests
  * @param OperateMode: The operation mode of AES 
  *   This parameter can be one of the following values:
  *     @arg AES_CTL_ENC: encryption
  *     @arg AES_CTL_DEC: decryption
  * @param CoreTypeOrIO: The core type 
  *   This parameter can be one of the following values:
  *     @arg 0x00:  AES transfer by IO
  *     @arg MEMORY_TYPE_AP: AES transfer by DMA, running on the AP core
  *     @arg MEMORY_TYPE_CP: AES transfer by DMA, running on the CP core
  * @retval None
  */
void AESCCM(uint8_t *pAdata, uint32_t AdataByteLen, uint8_t *pPayload, uint32_t PayloadByteLen,
         	uint32_t *pKey, uint32_t KeyBitsLen, uint8_t *pNonce, uint8_t NonceByteLen, uint8_t TagByteLen, 
			uint8_t *pOutput, uint8_t *pDigest, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
	uint8_t i;
	uint32_t tmp;
    uint32_t AdataWordLen, PayloadWordLen;
    uint32_t ByteLenTmp = 15 - NonceByteLen;
	
    uint8_t TempIV[8] = {0};

    if((TagByteLen % 2 == 1) || (TagByteLen < 4) || (TagByteLen > 16))
    {
        return;
    }
    
    if(NonceByteLen < 7 || NonceByteLen > 14)
    {
        return;
    }
       
    if(AdataByteLen == 0)
    {
        return;
    }

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
    AES_EndianClear();

    /* Set AES mode */
	AES_ModeSet(AES_CTL_MODE_CCM);
	
  	/* Set encryption/decryption */
	AES_EncDecSet(OperateMode);
    
	//AES_ClockDiv2Dis();
	
    AES_CCMAuthLenSet((TagByteLen - 2) >> 1);     // ccm_t=(tlen-2)/2,
    AES_CCMLengthLenSet(ByteLenTmp - 1);          // ccm_q=15 - nlen - 1,
	
	
    /* Set the Key */
	AES_KeyLenSet(KeyBitsLen);
	
    AES_KeySet(0, pKey[0]);
    AES_KeySet(1, pKey[1]);
    AES_KeySet(2, pKey[2]);
    AES_KeySet(3, pKey[3]);
    
    if(KeyBitsLen >= AES_CTL_KEY_LEN_192)
    {
        AES_KeySet(4, pKey[4]);
        AES_KeySet(5, pKey[5]);
    }
	
    if(KeyBitsLen == AES_CTL_KEY_LEN_256)
    {
        AES_KeySet(6, pKey[6]);
        AES_KeySet(7, pKey[7]);
    }
	
	/* Set the IV */
	for(i = 7; i < NonceByteLen; i++)
	{
	    TempIV[i - 7] =  pNonce[i]; 
	}
	
    AES_IVSet(0, ((ByteLenTmp-1)  << 24) | (pNonce[0] << 16) | (pNonce[1] << 8) | pNonce[2]);
    AES_IVSet(1,       (pNonce[3] << 24) | (pNonce[4] << 16) | (pNonce[5] << 8) | pNonce[6]);   
    AES_IVSet(2,       (TempIV[0] << 24) | (TempIV[1] << 16) | (TempIV[2] << 8) | TempIV[3]);
    AES_IVSet(3,       (TempIV[4] << 24) | (TempIV[5] << 16) | (TempIV[6] << 8) | TempIV[7]);
	
    /* Set the length of authentication data*/	
	AES_AADLenSet(0, AdataByteLen);
    
	/* Set the length of payload */	
    AES_PayloadLenSet(PayloadByteLen);
	
	/* DMA mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();
		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		
		AES_BlockStart();
		
		/* Input the authentication data*/
		DMAChannelTransferSet(DataInChannel, (void *)pAdata, (void *)AES_DATA_IN_BASE, AdataByteLen, CoreTypeOrIO);

		DMAChannelTransferStart(DataInChannel);
		DMAChannelWaitIdle(DataInChannel);
		DMAIntClear(DataInChannel);
		
		/* Input the payload */
		DMAChannelTransferSet(DataInChannel, (void *)pPayload, (void *)AES_DATA_IN_BASE, PayloadByteLen, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, PayloadByteLen, CoreTypeOrIO);		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);

		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);

		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else /* IO Mode */
	{	
		
		AdataWordLen = (AdataByteLen + 3) / 4;

		/* Input the block of authentication data */
		for(i = 0; i < AdataWordLen / 4; i++)
		{
			AES_BlockDataInput( 0, pAdata[16*i+3]  << 24 | pAdata[16*i+2]  << 16 | pAdata[16*i+1]  << 8 | pAdata[16*i+0]);
			AES_BlockDataInput( 4, pAdata[16*i+7]  << 24 | pAdata[16*i+6]  << 16 | pAdata[16*i+5]  << 8 | pAdata[16*i+4]);
			AES_BlockDataInput( 8, pAdata[16*i+11] << 24 | pAdata[16*i+10] << 16 | pAdata[16*i+9]  << 8 | pAdata[16*i+8]);
			AES_BlockDataInput(12, pAdata[16*i+15] << 24 | pAdata[16*i+14] << 16 | pAdata[16*i+13] << 8 | pAdata[16*i+12]);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
		}
		
		/* Input the remaining data and make up with 0x00 */		
		if(AdataWordLen % 4 == 1)
		{
			AES_BlockDataInput( 0, pAdata[16*i+3]  << 24 | pAdata[16*i+2]  << 16 | pAdata[16*i+1]  << 8 | pAdata[16*i+0]);
			AES_BlockDataInput( 4, 0x00000000);
			AES_BlockDataInput( 8, 0x00000000);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
		}
		else if(AdataWordLen % 4 == 2)
		{
			AES_BlockDataInput( 0, pAdata[16*i+3]  << 24 | pAdata[16*i+2]  << 16 | pAdata[16*i+1]  << 8 | pAdata[16*i+0]);
			AES_BlockDataInput( 4, pAdata[16*i+7]  << 24 | pAdata[16*i+6]  << 16 | pAdata[16*i+5]  << 8 | pAdata[16*i+4]);
			AES_BlockDataInput( 8, 0x00000000);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
		}
		else if(AdataWordLen % 4 == 3)
		{
			AES_BlockDataInput( 0, pAdata[16*i+3]  << 24 | pAdata[16*i+2]  << 16 | pAdata[16*i+1]  << 8 | pAdata[16*i+0]);
			AES_BlockDataInput( 4, pAdata[16*i+7]  << 24 | pAdata[16*i+6]  << 16 | pAdata[16*i+5]  << 8 | pAdata[16*i+4]);
			AES_BlockDataInput( 8, pAdata[16*i+11] << 24 | pAdata[16*i+10] << 16 | pAdata[16*i+9]  << 8 | pAdata[16*i+8]);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
		}


		PayloadWordLen = (PayloadByteLen + 3) / 4;
		
		/* Input the block of payload */
		for(i = 0; i < PayloadWordLen / 4; i++)
		{
			AES_BlockDataInput( 0, pPayload[16*i+3]  << 24 | pPayload[16*i+2]  << 16 | pPayload[16*i+1]  << 8 | pPayload[16*i+0]);
			AES_BlockDataInput( 4, pPayload[16*i+7]  << 24 | pPayload[16*i+6]  << 16 | pPayload[16*i+5]  << 8 | pPayload[16*i+4]);
			AES_BlockDataInput( 8, pPayload[16*i+11] << 24 | pPayload[16*i+10] << 16 | pPayload[16*i+9]  << 8 | pPayload[16*i+8]);
			AES_BlockDataInput(12, pPayload[16*i+15] << 24 | pPayload[16*i+14] << 16 | pPayload[16*i+13] << 8 | pPayload[16*i+12]);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();

			/* Output the cipher */
			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(12);
			pOutput[16*i+12] = (uint8_t)tmp;
			pOutput[16*i+13] = (uint8_t)(tmp >> 8);
			pOutput[16*i+14] = (uint8_t)(tmp >> 16);
			pOutput[16*i+15] = (uint8_t)(tmp >> 24);
		}
		
		/* Input the remaining data and make up with 0x00*/
		if(PayloadWordLen % 4 == 1)
		{
			AES_BlockDataInput( 0, pPayload[16*i+3]  << 24 | pPayload[16*i+2]  << 16 | pPayload[16*i+1]  << 8 | pPayload[16*i+0]);
			AES_BlockDataInput( 4, 0x00000000);
			AES_BlockDataInput( 8, 0x00000000);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);
		}
		else if(PayloadWordLen % 4 == 2)
		{
			AES_BlockDataInput( 0, pPayload[16*i+3]  << 24 | pPayload[16*i+2]  << 16 | pPayload[16*i+1]  << 8 | pPayload[16*i+0]);
			AES_BlockDataInput( 4, pPayload[16*i+7]  << 24 | pPayload[16*i+6]  << 16 | pPayload[16*i+5]  << 8 | pPayload[16*i+4]);
			AES_BlockDataInput( 8, 0x00000000);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);
		}
		else if(PayloadWordLen % 4 == 3)
		{
			AES_BlockDataInput( 0, pPayload[16*i+3]  << 24 | pPayload[16*i+2]  << 16 | pPayload[16*i+1]  << 8 | pPayload[16*i+0]);
			AES_BlockDataInput( 4, pPayload[16*i+7]  << 24 | pPayload[16*i+6]  << 16 | pPayload[16*i+5]  << 8 | pPayload[16*i+4]);
			AES_BlockDataInput( 8, pPayload[16*i+11] << 24 | pPayload[16*i+10] << 16 | pPayload[16*i+9]  << 8 | pPayload[16*i+8]);
			AES_BlockDataInput(12, 0x00000000);
			
			AES_BlockStart();
			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			pOutput[16*i+0] = (uint8_t)tmp;
			pOutput[16*i+1] = (uint8_t)(tmp >> 8);
			pOutput[16*i+2] = (uint8_t)(tmp >> 16);
			pOutput[16*i+3] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(4);
			pOutput[16*i+4] = (uint8_t)tmp;
			pOutput[16*i+5] = (uint8_t)(tmp >> 8);
			pOutput[16*i+6] = (uint8_t)(tmp >> 16);
			pOutput[16*i+7] = (uint8_t)(tmp >> 24);
			
			tmp = AES_BlockDataOutput(8);
			pOutput[16*i+8]  = (uint8_t)tmp;
			pOutput[16*i+9]  = (uint8_t)(tmp >> 8);
			pOutput[16*i+10] = (uint8_t)(tmp >> 16);
			pOutput[16*i+11] = (uint8_t)(tmp >> 24);
		}
	}
    
    // Digest
    tmp = AES_TagGet(3);
    pDigest[3] = (uint8_t)tmp;
    pDigest[2] = (uint8_t)(tmp >> 8);
    pDigest[1] = (uint8_t)(tmp >> 16);
    pDigest[0] = (uint8_t)(tmp >> 24);
    
    tmp = AES_TagGet(2);
    pDigest[7] = (uint8_t)tmp;
    pDigest[6] = (uint8_t)(tmp >> 8);
    pDigest[5] = (uint8_t)(tmp >> 16);
    pDigest[4] = (uint8_t)(tmp >> 24);
    
    tmp = AES_TagGet(1);
    pDigest[11] = (uint8_t)tmp;
    pDigest[10] = (uint8_t)(tmp >> 8);
    pDigest[9]  = (uint8_t)(tmp >> 16);
    pDigest[8]  = (uint8_t)(tmp >> 24);
    
    tmp = AES_TagGet(0);
    pDigest[15] = (uint8_t)tmp;
    pDigest[14] = (uint8_t)(tmp >> 8);
    pDigest[13] = (uint8_t)(tmp >> 16);
    pDigest[12] = (uint8_t)(tmp >> 24);
}

/**
  * @brief Configure the 128-EEA2.
  * @param pPlain: A pointer to the input plaintext  
  * @param pKey: A pointer to the key
  * @param pCount: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext
  * @param pCipher: A pointer to the output ciphertext  
  * @retval None
  */
void AES_128_EEA2(uint8_t *pPlain, uint8_t *pKey, uint8_t *pCount,
	              uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, 
				  uint8_t *pCipher, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{

	uint32_t i = 0, j = 0;
	uint32_t tmp;	
    uint32_t BlockLen = 0;
	uint32_t PlainByteLen = 0;
	uint32_t ByteLenInLastBlock = 0;
	
    uint32_t IV[2];
	uint8_t tmpchar[16];

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
	AES_EndianClear();
	/* Set AES CTR mode */    
	AES_ModeSet(AES_CTL_MODE_CTR);
	
    AES_EncDecSet(AES_CTL_ENC);
	
	/* Set the Key */
    AES_KeyLenSet(AES_CTL_KEY_LEN_128);
    
	AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);	
	
	/* Set T1 : COUNT[0] .. COUNT[31] | BEARER[0] .. BEARER[4] | DIRECTION | 0..0 (i.e. 26 zero bits) */
	IV[0] = (uint32_t)(pCount[0] << 24 | pCount[1] << 16 | pCount[2] << 8 | pCount[3]);
	IV[1] = (uint32_t)((Bearer << 27) | (Dir << 26));
		
	AES_IVSet(0, IV[0]);
	AES_IVSet(1, IV[1]);
	AES_IVSet(2, 0x00000000);
	AES_IVSet(3, 0x00000000);

    PlainByteLen  = (LengthBit + 7) >> 3;
	
//	AES_ClockDiv2Dis();

    /* DMA mode */
    if(CoreTypeOrIO)
	{
		AES_DMAEn();
		
		/* Set the length of plain in byte */ 
		AES_PayloadLenSet(PlainByteLen);

		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		
		AES_BlockStart();

		DMAChannelTransferSet(DataInChannel, (void *)pPlain, (void *)AES_DATA_IN_BASE, PlainByteLen, CoreTypeOrIO);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pCipher, PlainByteLen, CoreTypeOrIO);
		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);	

		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
	
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else /* IO mode */
	{		
		
		BlockLen = PlainByteLen >> 4;
	    ByteLenInLastBlock = PlainByteLen & 0x0F;
		
		/* Set the length of plain and aligned by 16 bytes */ 
		if(ByteLenInLastBlock == 0)
		{
			AES_PayloadLenSet(BlockLen << 4);
		}
		else
		{
			AES_PayloadLenSet((BlockLen+1) << 4);
		}
		
		 /* Input the Plain */
		for(i = 0; i < BlockLen; i++)
		{
			AES_BlockDataInput( 0, pPlain[16*i+3]  << 24 | pPlain[16*i+2]  << 16 | pPlain[16*i+1]  << 8 | pPlain[16*i+0]);
			AES_BlockDataInput( 4, pPlain[16*i+7]  << 24 | pPlain[16*i+6]  << 16 | pPlain[16*i+5]  << 8 | pPlain[16*i+4]);
			AES_BlockDataInput( 8, pPlain[16*i+11] << 24 | pPlain[16*i+10] << 16 | pPlain[16*i+9]  << 8 | pPlain[16*i+8]);
			AES_BlockDataInput(12, pPlain[16*i+15] << 24 | pPlain[16*i+14] << 16 | pPlain[16*i+13] << 8 | pPlain[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			pCipher[16*i+0] = (uint8_t)tmp;
			pCipher[16*i+1] = (uint8_t)(tmp >> 8);
			pCipher[16*i+2] = (uint8_t)(tmp >> 16);
			pCipher[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			pCipher[16*i+4] = (uint8_t)tmp;
			pCipher[16*i+5] = (uint8_t)(tmp >> 8);
			pCipher[16*i+6] = (uint8_t)(tmp >> 16);
			pCipher[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			pCipher[16*i+8]  = (uint8_t)tmp;
			pCipher[16*i+9]  = (uint8_t)(tmp >> 8);
			pCipher[16*i+10] = (uint8_t)(tmp >> 16);
			pCipher[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			pCipher[16*i+12] = (uint8_t)tmp;
			pCipher[16*i+13] = (uint8_t)(tmp >> 8);
			pCipher[16*i+14] = (uint8_t)(tmp >> 16);
			pCipher[16*i+15] = (uint8_t)(tmp >> 24);
		}
         /* Input the last block of Plain */
		if(ByteLenInLastBlock > 0)
		{
			AES_BlockDataInput( 0, pPlain[16*i+3]  << 24 | pPlain[16*i+2]  << 16 | pPlain[16*i+1]  << 8 | pPlain[16*i+0]);
			AES_BlockDataInput( 4, pPlain[16*i+7]  << 24 | pPlain[16*i+6]  << 16 | pPlain[16*i+5]  << 8 | pPlain[16*i+4]);
			AES_BlockDataInput( 8, pPlain[16*i+11] << 24 | pPlain[16*i+10] << 16 | pPlain[16*i+9]  << 8 | pPlain[16*i+8]);
			AES_BlockDataInput(12, pPlain[16*i+15] << 24 | pPlain[16*i+14] << 16 | pPlain[16*i+13] << 8 | pPlain[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			tmpchar[0] = (uint8_t)tmp;
			tmpchar[1] = (uint8_t)(tmp >> 8);
			tmpchar[2] = (uint8_t)(tmp >> 16);
			tmpchar[3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			tmpchar[4] = (uint8_t)tmp;
			tmpchar[5] = (uint8_t)(tmp >> 8);
			tmpchar[6] = (uint8_t)(tmp >> 16);
			tmpchar[7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			tmpchar[8]  = (uint8_t)tmp;
			tmpchar[9]  = (uint8_t)(tmp >> 8);
			tmpchar[10] = (uint8_t)(tmp >> 16);
			tmpchar[11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			tmpchar[12] = (uint8_t)tmp;
			tmpchar[13] = (uint8_t)(tmp >> 8);
			tmpchar[14] = (uint8_t)(tmp >> 16);
			tmpchar[15] = (uint8_t)(tmp >> 24);

			for(j = 0; j < ByteLenInLastBlock; j++)
			{
				pCipher[16*i+j] = tmpchar[j];
			}
		}	
	}
}

/**
  * @brief Configure the 128-EIA2.
  * @param pMessage: A pointer to the input message  
  * @param pKey: A pointer to the key
  * @param pCount: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext
  * @param M: A pointer to the buf for temporary storage of intermediate data
  * @param pMACT: A pointer to the output message authentication code   
  * @retval None
  */
void AES_128_EIA2(uint8_t *pMessage, uint8_t *pKey, uint8_t *pCount, 
	              uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, 
				  uint8_t *M, uint8_t *pMACT, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
	uint32_t i, j;
	uint32_t tmp;
	uint32_t MsgByteLen = 0;
	uint32_t LastBlockOffset = 0;
    uint8_t pK1[AES_BLOCK_BYTES];
    uint8_t pK2[AES_BLOCK_BYTES];
	
	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_CMAC_Generate_Subkey(pKey, pK1, pK2);
	
	M[0] = pCount[0];
	M[1] = pCount[1];
	M[2] = pCount[2];
	M[3] = pCount[3];
	M[4] = ((Bearer << 3) | (Dir << 2));
	M[5] = 0;
	M[6] = 0;
	M[7] = 0;
	  
	MsgByteLen = (LengthBit + 7) >> 3;
	
	// Copy Message to unM (Shod use DMA)
	for(i = 0; i < MsgByteLen; i++)
	{
		M[8+i] = pMessage[i];
	}
	
	if(LengthBit % 128 == 64)
	{
		// Complete Block, No Pad.
		LastBlockOffset = MsgByteLen - 8;
		
		// Mn XOR K1
		i = 0;
		for(j = LastBlockOffset; j < LastBlockOffset + AES_BLOCK_BYTES; j++)
		{
			M[j] ^= pK1[i++];
		}
	}
	else
	{
		// Need to Pad.
		LastBlockOffset = ((8 + (LengthBit >> 3)) >> 4) << 4;
		
		if(LengthBit % 8 == 0)
		{
			M[8+i] = 0x80;
			
			for(j = 9+i; j < LastBlockOffset + AES_BLOCK_BYTES; j++)
			{
				M[j] = 0;
			}
		}
		else
		{
			M[7+i] |= (1 << (7 - LengthBit % 8));
			
			for(j = 8+i; j < LastBlockOffset + AES_BLOCK_BYTES; j++)
			{
				M[j] = 0;
			}
		}
		
		// Mn XOR K2
		i = 0;
		for(j = LastBlockOffset; j < LastBlockOffset + AES_BLOCK_BYTES; j++)
		{
			M[j] ^= pK2[i++];
		}
	}

    AES_SetCipherMode(CIPHER_MODE_AES);
    AES_ClearUiaAndEia();
	AES_EndianClear();

	/* AES CBC Mode */
	AES_ModeSet(AES_CTL_MODE_CBC);
	
    AES_EncDecSet(AES_CTL_ENC);
    
	//AES_ClockDiv2Dis();   
    
    AES_KeyLenSet(AES_CTL_KEY_LEN_128);
    
	AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
	AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
	AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
	
	AES_PayloadLenSet(LastBlockOffset + AES_BLOCK_BYTES);

	AES_IVSet(0, 0x00000000);
	AES_IVSet(1, 0x00000000);
	AES_IVSet(2, 0x00000000);
	AES_IVSet(3, 0x00000000);
	
	/* AES DMA Mode */
	if(CoreTypeOrIO)
	{   
		AES_DMAEn();
		
		DMAChannelConfigure(DataInChannel,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
        
		AES_BlockStart();
		
		DMAChannelTransferSet(DataInChannel, (void *)M, (void *)AES_DATA_IN_BASE, LastBlockOffset + AES_BLOCK_BYTES, MEMORY_TYPE_AP);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)M, LastBlockOffset + AES_BLOCK_BYTES, MEMORY_TYPE_AP);
		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);

		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk 
		
		AES_DMADis();
	}
	else
	{
		// AES IO Mode (DMA Mode is Better.)
		for(i = 0; i < (LastBlockOffset+16) / AES_BLOCK_BYTES; i++)
		{
			AES_BlockDataInput( 0, M[16*i+3]  << 24 | M[16*i+2]  << 16 | M[16*i+1]  << 8 | M[16*i+0]);
			AES_BlockDataInput( 4, M[16*i+7]  << 24 | M[16*i+6]  << 16 | M[16*i+5]  << 8 | M[16*i+4]);
			AES_BlockDataInput( 8, M[16*i+11] << 24 | M[16*i+10] << 16 | M[16*i+9]  << 8 | M[16*i+8]);
			AES_BlockDataInput(12, M[16*i+15] << 24 | M[16*i+14] << 16 | M[16*i+13] << 8 | M[16*i+12]);

			AES_BlockStart();
			AES_BlockTransWaitDone();

			tmp = AES_BlockDataOutput(0);
			M[16*i+0] = (uint8_t)tmp;
			M[16*i+1] = (uint8_t)(tmp >> 8);
			M[16*i+2] = (uint8_t)(tmp >> 16);
			M[16*i+3] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(4);
			M[16*i+4] = (uint8_t)tmp;
			M[16*i+5] = (uint8_t)(tmp >> 8);
			M[16*i+6] = (uint8_t)(tmp >> 16);
			M[16*i+7] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(8);
			M[16*i+8]  = (uint8_t)tmp;
			M[16*i+9]  = (uint8_t)(tmp >> 8);
			M[16*i+10] = (uint8_t)(tmp >> 16);
			M[16*i+11] = (uint8_t)(tmp >> 24);

			tmp = AES_BlockDataOutput(12);
			M[16*i+12] = (uint8_t)tmp;
			M[16*i+13] = (uint8_t)(tmp >> 8);
			M[16*i+14] = (uint8_t)(tmp >> 16);
			M[16*i+15] = (uint8_t)(tmp >> 24);
		}
	}

    pMACT[0] = M[LastBlockOffset];
    pMACT[1] = M[LastBlockOffset + 1];
    pMACT[2] = M[LastBlockOffset + 2];
    pMACT[3] = M[LastBlockOffset + 3];
}

/* Basic Functions For CMAC*/
void AES_CMAC_XOR_128(uint8_t *pA, const uint8_t *pB, uint8_t *pOut)
{
    uint8_t i;
  
    for (i = 0; i < AES_BLOCK_BYTES; i++)
    {
        pOut[i] = pA[i] ^ pB[i];
    }
}

void AES_CMAC_Leftshift_Onebit(uint8_t *pInput, uint8_t *pOutput)
{
	signed char i;
	uint8_t overflow = 0;
	
	for(i = 15; i >= 0; i--)
	{
		pOutput[i] = pInput[i] << 1;
		pOutput[i] |= overflow;
		overflow = (pInput[i] & 0x80) ? 1 : 0;
	}
}

void AES_CMAC_Generate_Subkey(uint8_t *pKey, uint8_t *pK1, uint8_t *pK2)
{
	uint8_t i;
	uint8_t L[AES_BLOCK_BYTES];
	uint8_t Z[AES_BLOCK_BYTES];
	uint8_t tmp[AES_BLOCK_BYTES];


	for(i = 0; i < AES_BLOCK_BYTES; i++)
	{
		Z[i] = 0;
	}
	
    AESECB(Z, AES_BLOCK_BYTES, pKey, AES_CTL_KEY_LEN_128, L, AES_CTL_ENC, 0, 0, 0);

	if((L[0] & 0x80) == 0)
	{
		/* If MSB(L) = 0, then K1 = L << 1 */
		AES_CMAC_Leftshift_Onebit(L, pK1);
	}
	else
	{
		/* Else K1 = ( L << 1 ) (+) Rb */
		AES_CMAC_Leftshift_Onebit(L, tmp);
		AES_CMAC_XOR_128(tmp, AES_CMAC_Rb, pK1);
	}

	if((pK1[0] & 0x80) == 0)
	{
		AES_CMAC_Leftshift_Onebit(pK1, pK2);
	}
	else
	{
		AES_CMAC_Leftshift_Onebit(pK1, tmp);
		AES_CMAC_XOR_128(tmp, AES_CMAC_Rb, pK2);
	}
}

void snow_3g_basic(uint8_t *pInput, uint8_t *pKey, uint8_t *pIV, uint8_t *pOutput, uint32_t LengthByte, uint8_t EndianOut, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
    uint32_t tmp;
	uint32_t LengthWord;
    uint32_t LeftByte;
    
    volatile uint32_t i;
    
    LengthWord = LengthByte >> 2;
    LeftByte   = LengthByte & 0x3;
 
	HWREGB(REG_AES_STA); // for pclk!=hclk
	
    /* AES CBC Mode */
	AES_ModeSet(AES_CTL_MODE_CBC);
    AES_EncDecSet(AES_CTL_ENC);
    AES_EndianClear();

    // K3~K0
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
    AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
    AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
    AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
	
	AES_PayloadLenSet(LengthByte);

    // IV3~IV0
	AES_IVSet(0, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
    AES_IVSet(1, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
    AES_IVSet(2, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
    AES_IVSet(3, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	
    AES_SetCipherMode(CIPHER_MODE_SNOW3G);
    
    if(ENDIAN_SMALL == EndianOut)
    {
		SZ1_EndianOutSmall();
    }
    else
    {
		SZ1_EndianOutBig();
    }
	
	SZ2_EndianOutSmall();
    
    AES_EndianInSmall();

    if(CoreTypeOrIO)
    {
        AES_DMAEn();		
    }
	
    Snow3GEncrypteEn();
	
    AES_BlockStart();

    S3G_ZUC_TransWaitDone();

	/* AES DMA Mode */
	if(CoreTypeOrIO)
	{		
		
		DMAChannelConfigure(DataInChannel,DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel,DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W | DMAC_CTRL_WORD_SIZE_32b);
		
		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);	
				
		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);	
		
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, LengthByte, MEMORY_TYPE_AP);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, LengthByte, MEMORY_TYPE_AP);
		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);
		
		DMAChannelMuxDisable(DataInChannel); // for bug 7575
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk
		
		AES_DMADis();
	
	}
	else
	{		
		for(i = 0; i < LengthWord; i++)
		{
			AES_BlockDataInput(0, pInput[4*i+3]  << 24 | pInput[4*i+2]  << 16 | pInput[4*i+1]  << 8 | pInput[4*i]);

			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			
			pOutput[4*i]   = (uint8_t)tmp;
			pOutput[4*i+1] = (uint8_t)(tmp >> 8);
			pOutput[4*i+2] = (uint8_t)(tmp >> 16);
			pOutput[4*i+3] = (uint8_t)(tmp >> 24);
		}
		
		if(LeftByte != 0)
		{
			AES_BlockDataInput(0, pInput[4*i+3]  << 24 | pInput[4*i+2]  << 16 | pInput[4*i+1]  << 8 | pInput[4*i]);

			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			
			pOutput[4*i]   = (uint8_t)tmp;
			
			if(LeftByte > 1)
			{
				pOutput[4*i+1] = (uint8_t)(tmp >> 8);
			}
			
			if(LeftByte > 2)
			{
				pOutput[4*i+2] = (uint8_t)(tmp >> 16);
			}
		}
	}
}

void z_basic(uint8_t *pInput, uint8_t *pKey, uint8_t *pIV,  uint8_t *pOutput, uint32_t LengthByte, uint8_t EndianOut, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
    uint32_t LengthWord;
    uint32_t LeftByte;
    uint32_t i;
    uint32_t tmp;
    
    LengthWord = LengthByte >> 2;
    LeftByte   = LengthByte & 0x3;
    
	HWREGB(REG_AES_STA); // for pclk!=hclk
	
    /* AES CBC Mode */
	AES_ModeSet(AES_CTL_MODE_CBC);
	
    AES_EncDecSet(AES_CTL_ENC);
    AES_EndianClear();

	AES_KeySet(0, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    AES_KeySet(1, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
    AES_KeySet(2, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
    AES_KeySet(3, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
    
	AES_PayloadLenSet(LengthByte);
	
	AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
    AES_SetCipherMode(CIPHER_MODE_ZUC);
    
    if(ENDIAN_SMALL == EndianOut)
    {
        SZ1_EndianOutSmall();
    }
    else
    {
        SZ1_EndianOutBig();
    }
	
	SZ2_EndianOutSmall();
    
	AES_EndianInSmall();

    if(CoreTypeOrIO)
    {
        AES_DMAEn();
    }
	
    ZUCEncrypteEn();
	
    AES_BlockStart();
    
    S3G_ZUC_TransWaitDone();
    
//  for(i=100;;i--)
//	{
//	    if(i == 0){break;}
//	}
    
	/* AES DMA Mode */
	if(CoreTypeOrIO)
	{		
		DMAChannelConfigure(DataInChannel, DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelConfigure(DataOutChannel, DMAC_CTRL_DINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W | DMAC_CTRL_WORD_SIZE_32b);

		DMAChannelPeriphReq(DataInChannel, DMA_REQNUM_AES_IN);
		DMAChannelPeriphReq(DataOutChannel, DMA_REQNUM_AES_OUT);
		
		DMAPeriphReqEn(DataInChannel);
		DMAPeriphReqEn(DataOutChannel);
        
		DMAChannelTransferSet(DataInChannel, (void *)pInput, (void *)AES_DATA_IN_BASE, LengthByte, MEMORY_TYPE_AP);
		DMAChannelTransferSet(DataOutChannel, (void *)AES_DATA_OUT_BASE, (void *)pOutput, LengthByte, MEMORY_TYPE_AP);
		
		DMAChannelTransferStart(DataInChannel);
		DMAChannelTransferStart(DataOutChannel);
		
		DMAChannelWaitIdle(DataInChannel);
		DMAChannelWaitIdle(DataOutChannel);
		DMAIntClear(DataInChannel);
		DMAIntClear(DataOutChannel);
		
		DMAChannelMuxDisable(DataInChannel);
		DMAChannelMuxDisable(DataOutChannel);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk
		
		AES_DMADis();
	}
    else	
	{
		/* AES IO Mode (DMA Mode is Better.) */
		for(i = 0; i < LengthWord; i++)
		{
			AES_BlockDataInput(0, pInput[4*i+3]  << 24 | pInput[4*i+2]  << 16 | pInput[4*i+1]  << 8 | pInput[4*i]);

			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);
			
			pOutput[4*i]   = (uint8_t)tmp;
			pOutput[4*i+1] = (uint8_t)(tmp >> 8);
			pOutput[4*i+2] = (uint8_t)(tmp >> 16);
			pOutput[4*i+3] = (uint8_t)(tmp >> 24);
		}
		
		if(LeftByte != 0)
		{
			AES_BlockDataInput(0, pInput[4*i+3]  << 24 | pInput[4*i+2]  << 16 | pInput[4*i+1]  << 8 | pInput[4*i]);

			AES_BlockTransWaitDone();
			
			tmp = AES_BlockDataOutput(0);

			pOutput[4*i]   = (uint8_t)tmp;
			
			if(LeftByte > 1)
			{
				pOutput[4*i+1] = (uint8_t)(tmp >> 8);
			}
			
			if(LeftByte > 2)
			{
				pOutput[4*i+2] = (uint8_t)(tmp >> 16);
			}
		}
	}
}

/**
  * @brief Configure the 128-EEA1.
  * @param pInput: A pointer to the input plaintext  
  * @note  The pInput parameter is also an output data pointer
  * @param pKey: A pointer to the key
  * @param pCount: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext 
  * @retval None
  */
void f_128_EEA1(uint8_t *pInput, uint8_t *pKey,uint8_t *pCount, uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
    uint32_t IV[4];
    uint32_t MsgByteLen;
    MsgByteLen = (LengthBit + 7) >> 3;
    
    IV[0] = (uint32_t)(pCount[3] << 24 | pCount[2] << 16 | pCount[1] << 8 | pCount[0]);
    IV[1] = ((Bearer << 3) | (Dir << 2));
	IV[2] = IV[0];
	IV[3] = IV[1];

    snow_3g_basic(pInput, pKey, (uint8_t*)IV, pInput, MsgByteLen, ENDIAN_SMALL, CoreTypeOrIO, DataInChannel, DataOutChannel);
}

/**
  * @brief Configure the 128-EIA1.
  * @param pMessage: A pointer to the input message  
  * @param pKey: A pointer to the key
  * @param pCountI: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext
  * @param pMACT: A pointer to the output message authentication code   
  * @retval None
  */
void f_128_EIA1(uint8_t *pMessage, uint8_t *pKey, uint8_t *pCountI, uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, uint8_t *pMacT, uint8_t CoreTypeOrIO, uint32_t ChannelNum)
{
	uint32_t i;
    uint32_t tmp;
    uint32_t IV[4];
    uint32_t MsgByteLen;
	uint32_t LengthDWord;
    uint32_t LeftByte;
	uint8_t *pIV;
    
    MsgByteLen = (LengthBit + 7) >> 3;
    
    IV[0] = (uint32_t)(pCountI[3] << 24 | pCountI[2] << 16 | pCountI[1] << 8 | pCountI[0]);
    IV[1] = (uint32_t)(Bearer << 3);  //fresh
    
    if(Dir == 1)
    {
        IV[2] = IV[0] ^ 0x00000080;
        IV[3] = IV[1] ^ 0x00800000;
    }
    else
    {
        IV[2] = IV[0];
        IV[3] = IV[1];
    }
	
    LengthDWord = MsgByteLen >> 3;
    LeftByte    = MsgByteLen & 0x7;
	
	if(LeftByte > 4)
	{
		LengthDWord++;
	}

	HWREGB(REG_AES_STA); // for pclk!=hclk
	
    AES_EndianClear();

    // K3~K0
    AES_KeySet(0, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
    AES_KeySet(1, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
    AES_KeySet(2, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
    AES_KeySet(3, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
	
	AES_PayloadLenSet(LengthBit);

    // IV3~IV0
	pIV = (uint8_t *)&IV[0];
	AES_IVSet(0, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
    AES_IVSet(1, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
    AES_IVSet(2, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
    AES_IVSet(3, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	
    AES_SetCipherMode(CIPHER_MODE_SNOW3G);
	
	SZ1_EndianOutBig();
    
	AES_EndianInBig();

	Snow3GIntegrityEn();

    if(CoreTypeOrIO) 
    { 
        AES_DMAEn(); 
    }
    
    AES_BlockStart();
    
	/* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		DMAPeriphReqEn(ChannelNum);
		
		DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_AES_IN);
		
		DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_2W | DMAC_CTRL_WORD_SIZE_32b);
		
		DMAChannelTransferSet(ChannelNum, (void *)pMessage, (void *)AES_DATA_IN_BASE, 0, MEMORY_TYPE_AP);
		DMAChannelTransferStart(ChannelNum);
		AES_BlockTransWaitDone();

        DMAChannelTransferStop(ChannelNum);
		
		DMAChannelMuxDisable(ChannelNum);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk
		
		AES_DMADis();
		
		
	}
	else
	{
		for(i = 0; i < LengthDWord; i++)
		{
			AES_BlockDataInput(0, pMessage[8*i+3]  << 24 | pMessage[8*i+2]  << 16 | pMessage[8*i+1]  << 8 | pMessage[8*i]);
			AES_BlockDataInput(4, pMessage[8*i+7]  << 24 | pMessage[8*i+6]  << 16 | pMessage[8*i+5]  << 8 | pMessage[8*i+4]);

			AES_BlockTransWaitDone();
		}
		
		if(LeftByte > 0 && LeftByte <= 4)
		{
			AES_BlockDataInput(0, pMessage[8*i+3]  << 24 | pMessage[8*i+2]  << 16 | pMessage[8*i+1]  << 8 | pMessage[8*i]);
			AES_BlockTransWaitDone();	
		}
	}
	
	tmp = AES_TagGet(0);
	pMacT[0] = (tmp >> 24) & 0xFF;
	pMacT[1] = (tmp >> 16) & 0xFF;
	pMacT[2] = (tmp >>  8) & 0xFF;
	pMacT[3] =  tmp        & 0xFF;
}

/**
  * @brief Configure the 128-EEA3.
  * @param pKey: A pointer to the key
  * @param pCount: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext
  * @param pPlain: A pointer to the input plaintext  
  * @param pCipher: A pointer to the output ciphertext  
  * @retval None
  */
void f_128_EEA3(uint8_t *pKey, uint8_t *pCount, uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, uint8_t *pPlain, uint8_t *pCipher, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel)
{
    uint32_t IV[4];
    uint32_t MsgByteLen;
    
    MsgByteLen = (LengthBit + 7) >> 3;
    
    IV[0] = (uint32_t)(pCount[3] << 24 | pCount[2] << 16 | pCount[1] << 8 | pCount[0]);
    IV[1] = ((Bearer << 3) | (((uint8_t)Dir) << 2));
	IV[2] = IV[0];
	IV[3] = IV[1];
    
    z_basic(pPlain, pKey, (uint8_t *)IV, pCipher, MsgByteLen, ENDIAN_SMALL, CoreTypeOrIO, DataInChannel, DataOutChannel);
}

/**
  * @brief Configure the 128-EIA3.
  * @param pKey: A pointer to the key
  * @param pCount: A pointer to the counter
  * @param Bearer: The 5-bit bearer identity
  * @param Dir: The direction of the transmission 
  * @param LengthBit: The bit length of the plaintext
  * @param pMessage: A pointer to the input message  
  * @param pMACT: A pointer to the output message authentication code   
  * @retval None
  */
void f_128_EIA3(uint8_t *pKey, uint8_t *pCount, uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, uint8_t *pMessage, uint8_t *pMacI, uint8_t CoreTypeOrIO, uint32_t ChannelNum)
{
    uint32_t IV[4] = {0};
    uint32_t i;
    uint32_t tmp;
	uint32_t LengthWord;
	uint8_t *pIV;
    
    IV[0] = (uint32_t)(pCount[3] << 24 | pCount[2] << 16 | pCount[1] << 8 | pCount[0]);
    IV[1] = Bearer << 3;
    
    if(Dir == 1)
    {
        IV[2] = IV[0] ^ 0x00000080;
        IV[3] = IV[1] ^ 0x00800000;
    }
    else
    {
        IV[2] = IV[0];
        IV[3] = IV[1];
    }
    
    LengthWord = (LengthBit + 31) / 32;

	HWREGB(REG_AES_STA); // for pclk!=hclk 
	
    AES_EndianClear();

	AES_KeySet(0, pKey[12] << 24 | pKey[13] << 16 | pKey[14] << 8 | pKey[15]);
    AES_KeySet(1, pKey[8]  << 24 | pKey[9]  << 16 | pKey[10] << 8 | pKey[11]);
    AES_KeySet(2, pKey[4]  << 24 | pKey[5]  << 16 | pKey[6]  << 8 | pKey[7]);
    AES_KeySet(3, pKey[0]  << 24 | pKey[1]  << 16 | pKey[2]  << 8 | pKey[3]);
	
	AES_PayloadLenSet(LengthBit);	// bit length

	pIV = (uint8_t *)&IV[0];
	AES_IVSet(0, pIV[0]  << 24 | pIV[1]  << 16 | pIV[2]  << 8 | pIV[3]);
	AES_IVSet(1, pIV[4]  << 24 | pIV[5]  << 16 | pIV[6]  << 8 | pIV[7]);
	AES_IVSet(2, pIV[8]  << 24 | pIV[9]  << 16 | pIV[10] << 8 | pIV[11]);
	AES_IVSet(3, pIV[12] << 24 | pIV[13] << 16 | pIV[14] << 8 | pIV[15]);
	
	AES_SetCipherMode(CIPHER_MODE_ZUC);
    
	SZ1_EndianOutBig();

	AES_EndianInBig();
	
	ZUCIntegrityEn();

	if(CoreTypeOrIO)
	{
        AES_DMAEn();
    }

	AES_BlockStart();
    
	/* AES DMA Mode */
	if(CoreTypeOrIO)
	{
		AES_DMAEn();
        
		DMAPeriphReqEn(ChannelNum);		
		DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_AES_IN);	
		DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_1W | DMAC_CTRL_WORD_SIZE_32b);
		DMAChannelTransferSet(ChannelNum, (void *)pMessage, (void *)AES_DATA_IN_BASE, 0, MEMORY_TYPE_AP);
		DMAChannelTransferStart(ChannelNum);
		AES_BlockTransWaitDone();
        
		DMAChannelTransferStop(ChannelNum);
		
		DMAChannelMuxDisable(ChannelNum);
		
		HWREGB(REG_AES_STA); // for pclk!=hclk bug
		
		AES_DMADis();
	}
	else
	{
		for(i = 0; i < LengthWord; i++)
		{
			AES_BlockDataInput(0, pMessage[4*i+3]  << 24 | pMessage[4*i+2]  << 16 | pMessage[4*i+1]  << 8 | pMessage[4*i]);

			AES_BlockTransWaitDone();
		}
	}
    
	tmp = AES_TagGet(0);
    pMacI[0] = (tmp >> 24) & 0xFF;
    pMacI[1] = (tmp >> 16) & 0xFF;
    pMacI[2] = (tmp >>  8) & 0xFF;
    pMacI[3] =  tmp        & 0xFF;
}

// For SHA
void sha1_process(uint8_t *pData, uint32_t lenBits, uint8_t *pRest, uint8_t dmaFlag, uint8_t MemType, uint32_t ChannelNum)
{
	uint32_t lenBlocks;
	uint32_t leftWords;
	uint32_t i, j, index, tmp;
	volatile uint32_t delay;
	
	if(TRANSFER_MODE_DMA_RESULT != dmaFlag)
	{
		lenBlocks = lenBits >> 9;
		leftWords = ((lenBits & 0x1FF) + 31) >> 5;
	
		HWREGB(REG_SHA_CTRL) |= SHA_CTRL_INPUT_ENDIAN_Msk;
	
		HWREG(REG_SHA_LEN) = lenBits;
	}
	
	if(TRANSFER_MODE_IO != dmaFlag)
	{
		if(TRANSFER_MODE_DMA == dmaFlag || TRANSFER_MODE_DMA_REQUEST == dmaFlag)
		{
			DMAPeriphReqEn(ChannelNum);
			
			DMAChannelPeriphReq(ChannelNum, DMA_REQNUM_SHA_WR);
			
			DMAChannelConfigure(ChannelNum, DMAC_CTRL_SINC_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_16W | DMAC_CTRL_WORD_SIZE_32b);
			
			DMAChannelTransferSet(ChannelNum, (void *)pData, (void *)REG_SHA_DATA_IN, 0, MemType);

			DMAChannelTransferStart(ChannelNum);
			
			HWREGB(REG_SHA_CTRL) |= SHA_CTRL_DMA_EN;
			
			HWREGB(REG_SHA_START) |= SHA_START;
		}
		
		if(TRANSFER_MODE_DMA == dmaFlag || TRANSFER_MODE_DMA_RESULT == dmaFlag)
		{
			while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_ALL_FIN));
			
			HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
			
			HWREGB(REG_SHA_CTRL) &= (~SHA_CTRL_DMA_EN);
		}
	}
	else
	{
		HWREGB(REG_SHA_START) |= SHA_START;
		
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
		}
		
		if(leftWords > 0)
		{
			for(j = 0; j < leftWords; j++)
			{
				index = 64*i + 4*j;
				
				HWREG(REG_SHA_DATA_IN) = (pData[index+3] << 24) | (pData[index+2] << 16) | (pData[index+1] << 8) | pData[index];
			}
			
			while(!(HWREGB(REG_SHA_STAT) & SHA_STAT_BLOCK_FIN));
			
			// delay, need to fix by rtl
			delay = 1000;
			while(delay--);
			
			HWREGB(REG_SHA_INTR_CLR) = SHA_FIN_INTR_CLR;
		}
	}
	
	if(TRANSFER_MODE_DMA_REQUEST != dmaFlag)
	{
		for(i = 0; i < 5; i++)
		{
			j = i << 2;
			
			tmp = HWREG(REG_SHA_RESULT + 16 - j);
			
			pRest[j]   = (tmp >> 24) & 0xFF;
			pRest[j+1] = (tmp >> 16) & 0xFF;
			pRest[j+2] = (tmp >>  8) & 0xFF;
			pRest[j+3] =  tmp        & 0xFF;
		}
	}
}

