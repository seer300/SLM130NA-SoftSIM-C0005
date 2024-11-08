#include "at_uart.h"
#include "hal_aes.h"

#define InputLen (32)

uint8_t plain_txt[InputLen]={0};      //初始明文
uint8_t cipher_txt[InputLen] = {0};   //密文
uint8_t decipher_txt[InputLen] = {0}; //解密明文

uint8_t key_128[] = "0123456789abcdef";//密钥
uint8_t IV_128[] = "0123456789123456"; //初始向量

HAL_AES_Init_HandleTypeDef aes = {0};
HAL_AES_Key_HandleTypeDef key = {0};

void AES_ECB_Case1_128(void)
{
	HAL_StatusTypeDef ret = HAL_OK;
	
	//初始化数据缓存区
	memset(cipher_txt, 0, InputLen);
	memset(decipher_txt, 0, InputLen);
	for(uint16_t i=0; i<InputLen; i++)
	{
		plain_txt[i] = (uint8_t)i;
	}

	//初始化AES模式
	aes.WorkMode = HAL_AES_ECB_MODE;
	aes.UseSema = HAL_AES_SEMA_USE;
	aes.SemaReqPri = HAL_AES_SEMA_REQ_PRI_0;

	//初始化Key
	key.pKey = key_128;
	key.KeyBitsLen = HAL_AES_KEYLENBITS_128;
	key.pIV = IV_128;

	//加密
	ret = HAL_AES_Encrypt(&aes, &key, plain_txt, InputLen, cipher_txt);
	if(ret != HAL_OK)
	{
		Send_AT_to_Ext("\r\n+AES_ECB Encrypt Fail!\r\n");
	}

	//解密
	ret = HAL_AES_Decrypt(&aes, &key, cipher_txt, InputLen, decipher_txt);
	if(ret != HAL_OK)
	{
		Send_AT_to_Ext("\r\n+AES_ECB Encrypt Fail!\r\n");
	}

	//检查加解密结果
	if(0 == memcmp(decipher_txt, plain_txt, InputLen))
	{
		Send_AT_to_Ext("\r\n+AES_ECB Pass!\r\n");
	}
	else
	{
		Send_AT_to_Ext("\r\n+AES_ECB Fail!\r\n");
	}
}

void AES_CBC_Case1_128(void)
{
	HAL_StatusTypeDef ret = HAL_OK;
	
	//初始化数据缓存区
	memset(cipher_txt, 0, InputLen);
	memset(decipher_txt, 0, InputLen);
	for(uint16_t i=0; i<InputLen; i++)
	{
		plain_txt[i] = (uint8_t)i;
	}

	//初始化AES模式
	aes.WorkMode = HAL_AES_CBC_MODE;
	aes.UseSema = HAL_AES_SEMA_USE;
	aes.SemaReqPri = HAL_AES_SEMA_REQ_PRI_0;

	//初始化Key
	key.pKey = key_128;
	key.KeyBitsLen = HAL_AES_KEYLENBITS_128;
	key.pIV = IV_128;

	//加密
	ret = HAL_AES_Encrypt(&aes, &key, plain_txt, InputLen, cipher_txt);
	if(ret != HAL_OK)
	{
		Send_AT_to_Ext("\r\n+AES_CCB Encrypt Fail!\r\n");
	}

	//解密
	ret = HAL_AES_Decrypt(&aes, &key, cipher_txt, InputLen, decipher_txt);
	if(ret != HAL_OK)
	{
		Send_AT_to_Ext("\r\n+AES_CCB Encrypt Fail!\r\n");
	}

	//检查加解密结果
	if(0 == memcmp(decipher_txt, plain_txt, InputLen))
	{
		Send_AT_to_Ext("\r\n+AES_CCB Pass!\r\n");
	}
	else
	{
		Send_AT_to_Ext("\r\n+AES_CCB Fail!\r\n");
	}
}