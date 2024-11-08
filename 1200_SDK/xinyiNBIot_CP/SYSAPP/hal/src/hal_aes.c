#include "hal_aes.h"

/**
  * @brief 配置128-EEA2.
  * @param pucPlain: A pointer to the input plaintext  
  * @param pucKey: A pointer to the key
  * @param pucCount: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext
  * @param pucCipher: A pointer to the output ciphertext  
  * @retval None
  */
void HAL_AES_128_EEA2(unsigned char *pucPlain, unsigned char *pucKey, unsigned char *pucCount,
	                  unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,
				      unsigned char *pucCipher)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	AES_128_EEA2(pucPlain, pucKey, pucCount, ucBearer, ucDir, ulLengthBit, pucCipher);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}

/**
  * @brief 配置128-EIA2.
  * @param pucMessage: A pointer to the input message  
  * @param pucKey: A pointer to the key
  * @param pucCount: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext
  * @param ucM: A pointer to the buf for temporary storage of intermediate data
  * @param pucMACT: A pointer to the output message authentication code   
  * @retval None
  */
void HAL_AES_128_EIA2(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCount, 
                      unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, 
                      unsigned char *ucM, unsigned char *pucMACT)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	AES_128_EIA2(pucMessage, pucKey, pucCount, ucBearer, ucDir, ulLengthBit, ucM, pucMACT);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}

/**
  * @brief 配置128-EEA1.
  * @param pucInput: A pointer to the input plaintext  
  * @note  The pucInput parameter is also an output data pointer
  * @param pucKey: A pointer to the key
  * @param pucCount: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext 
  * @retval None
  */
void HAL_f_128_EEA1(unsigned char *pucInput, unsigned char *pucKey,unsigned char *pucCount, 
                    unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	f_128_EEA1(pucInput, pucKey, pucCount, ucBearer, ucDir, ulLengthBit);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}

/**
  * @brief 配置128-EIA1.
  * @param pucMessage: A pointer to the input message  
  * @param pucKey: A pointer to the key
  * @param pucCountI: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext
  * @param pucMACT: A pointer to the output message authentication code   
  * @retval None
  */
void HAL_f_128_EIA1(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCountI, 
                    unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, 
                    unsigned char *pucMacT)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	f_128_EIA1(pucMessage, pucKey, pucCountI, ucBearer, ucDir, ulLengthBit, pucMacT);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}


/**
  * @brief 配置128-EEA3.
  * @param pucKey: A pointer to the key
  * @param pucCount: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext
  * @param pucPlain: A pointer to the input plaintext  
  * @param pucCipher: A pointer to the output ciphertext  
  * @retval None
  */
void HAL_f_128_EEA3(unsigned char *pucKey, unsigned char *pucCount, unsigned char ucBearer, 
                    unsigned char ucDir, unsigned long ulLengthBit, unsigned char *pucPlain, 
                    unsigned char *pucCipher)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	f_128_EEA3(pucKey, pucCount, ucBearer, ucDir, ulLengthBit, pucPlain, pucCipher);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}

/**
  * @brief 配置128-EIA3.
  * @param pucKey: A pointer to the key
  * @param pucCount: A pointer to the counter
  * @param ucBearer: The 5-bit bearer identity
  * @param ucDir: The direction of the transmission 
  * @param ulLengthBit: The bit length of the plaintext
  * @param pucMessage: A pointer to the input message  
  * @param pucMACT: A pointer to the output message authentication code   
  * @retval None
  */
void HAL_f_128_EIA3(unsigned char *pucKey, unsigned char *pucCount, unsigned char ucDir, 
                    unsigned char ucBearer, unsigned long ulLengthBit, unsigned char *pucMessage, 
                    unsigned char *pucMacI)
{
#if USE_AES_SEMA
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#if (USE_AES_DATA_SEMA_DMA == 0)
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#else
	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_REQ, AES_SEMA_REQ_PRI, SEMA_MASK_NULL);
	#endif
#endif

	f_128_EIA3(pucKey, pucCount, ucDir, ucBearer, ulLengthBit, pucMessage, pucMacI);

#if USE_AES_SEMA
	SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
	SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
#endif
}
