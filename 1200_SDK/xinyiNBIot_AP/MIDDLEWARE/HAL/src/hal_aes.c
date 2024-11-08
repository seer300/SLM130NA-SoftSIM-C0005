/**
 ******************************************************************************
 * @file    hal_aes.c
 * @brief   HAL库AES
 ******************************************************************************
 */

#include "hal_aes.h"
#include "hal_def.h"
#include "xy_printf.h"
#include <string.h>

/**
 * @brief AES阻塞数据加密API
 * 
 * @param aes 详情参考 @ref HAL_AES_Init_HandleTypeDef.
 * @param key 详情参考 @ref HAL_AES_Key_HandleTypeDef.
 * @param pInput 明文存储地址
 * @param InputLenByte 明文长度
 * @param pOutput 密文存储地址
 * @return 函数执行状态. 详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_AES_Encrypt(HAL_AES_Init_HandleTypeDef *aes, HAL_AES_Key_HandleTypeDef *key, uint8_t *pInput, uint32_t InputLenByte, uint8_t *pOutput)
{
	if (aes->State != HAL_AES_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pInput == NULL) || (key->pIV == NULL) || (key->pKey == NULL) || \
	    (pOutput == NULL) || (InputLenByte == 0) || (key->KeyBitsLen == 0) || \
		(aes->WorkMode == HAL_AES_NONE_MODE))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(aes);

	aes->State = HAL_AES_STATE_BUSY;

	//开启SEMA、AES、DMA时钟
	PRCM_ClockEnable(CORE_CKG_CTL_SEMA_EN);
	PRCM_ClockEnable(CORE_CKG_CTL_AES_EN);
	PRCM_ClockEnable(CORE_CKG_CTL_DMAC_EN);

	//若有硬件锁，则请求抢占AES的REG和DATA区域
	if(aes->UseSema == HAL_AES_SEMA_USE)
	{
		SEMA_Request(SEMA_MASTER_AP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, aes->SemaReqPri, SEMA_MASK_NULL);
		SEMA_Request(SEMA_MASTER_AP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, aes->SemaReqPri, SEMA_MASK_NULL);
		
	}

	switch((uint32_t)aes->WorkMode)
	{
		case (uint32_t)HAL_AES_ECB_MODE:
		{
			AESECB(pInput, InputLenByte, key->pKey, key->KeyBitsLen,pOutput, AES_CTL_ENC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CBC_MODE:
		{
			AESCBC(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_ENC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CFB_MODE:
		{
			AESCFB(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_ENC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_OFB_MODE:
		{
			AESOFB(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_ENC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CTR_MODE:
		{
			AESCTR(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_ENC, 0, 0, 0);
			break;
		}
		default :return HAL_ERROR;
	}

	//若有硬件锁，则释放AES的REG和DATA区域
	if(aes->UseSema == HAL_AES_SEMA_USE)
	{
		SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
		SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
	}

	aes->State = HAL_AES_STATE_READY;

	__HAL_UNLOCK(aes);

	return HAL_OK;
}

/**
 * @brief AES阻塞数据解密API
 * 
 * @param aes 详情参考 @ref HAL_AES_Init_HandleTypeDef.
 * @param key 详情参考 @ref HAL_AES_Key_HandleTypeDef.
 * @param pInput 密文存储地址
 * @param InputLenByte 密文长度
 * @param pOutput 明文存储地址
 * @return 函数执行状态. 详情参考 @ref HAL_StatusTypeDef
 */
HAL_StatusTypeDef HAL_AES_Decrypt(HAL_AES_Init_HandleTypeDef *aes, HAL_AES_Key_HandleTypeDef *key, uint8_t *pInput, uint32_t InputLenByte, uint8_t *pOutput)
{
	if (aes->State != HAL_AES_STATE_READY)
	{
		return HAL_BUSY;
	}

	if ((pInput == NULL) || (key->pIV == NULL) || (key->pKey == NULL) || \
	    (pOutput == NULL) || (InputLenByte == 0) || (key->KeyBitsLen == 0) || \
		(aes->WorkMode == HAL_AES_NONE_MODE))
	{
		return HAL_ERROR;
	}

	__HAL_LOCK(aes);

	aes->State = HAL_AES_STATE_BUSY;

	//开启SEMA、AES、DMA时钟
	PRCM_ClockEnable(CORE_CKG_CTL_SEMA_EN);
	PRCM_ClockEnable(CORE_CKG_CTL_AES_EN);
	PRCM_ClockEnable(CORE_CKG_CTL_DMAC_EN);

	//若有硬件锁，则请求抢占AES的REG和DATA区域
	if(aes->UseSema == HAL_AES_SEMA_USE)
	{
		SEMA_Request(SEMA_MASTER_AP, SEMA_SLAVE_AES_REG, SEMA_SEMA_DMAC_NO_REQ, aes->SemaReqPri, SEMA_MASK_NULL);
		SEMA_Request(SEMA_MASTER_AP, SEMA_SLAVE_AES_DATA, SEMA_SEMA_DMAC_NO_REQ, aes->SemaReqPri, SEMA_MASK_NULL);	
	}

	//根据AES模式进行解密操作
	switch((uint32_t)aes->WorkMode)
	{
		case (uint32_t)HAL_AES_ECB_MODE:
		{
			AESECB(pInput, InputLenByte, key->pKey, key->KeyBitsLen, pOutput, AES_CTL_DEC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CBC_MODE:
		{
			AESCBC(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_DEC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CFB_MODE:
		{
			AESCFB(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_DEC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_OFB_MODE:
		{
			AESOFB(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_DEC, 0, 0, 0);
			break;
		}
		case (uint32_t)HAL_AES_CTR_MODE:
		{
			AESCTR(pInput, InputLenByte, key->pKey, key->KeyBitsLen, key->pIV, pOutput, AES_CTL_DEC, 0, 0, 0);
			break;
		}
		default :return HAL_ERROR;
	}

	//若有硬件锁，则释放AES的REG和DATA区域
	if(aes->UseSema == HAL_AES_SEMA_USE)
	{
		SEMA_Release(SEMA_SLAVE_AES_REG, SEMA_MASK_NULL);
		SEMA_Release(SEMA_SLAVE_AES_DATA, SEMA_MASK_NULL);
	}

	aes->State = HAL_AES_STATE_READY;

	__HAL_UNLOCK(aes);

	return HAL_OK;
}
