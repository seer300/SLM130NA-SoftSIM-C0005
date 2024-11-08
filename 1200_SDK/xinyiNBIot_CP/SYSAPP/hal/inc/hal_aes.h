#ifndef __HAL_AES_H__
#define __HAL_AES_H__

#include "crypto.h"
#include "sema.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 ********************AES硬件锁介绍**********************
 * AES硬件锁有两个，一个是寄存器硬件锁，一个是数据区域硬件锁
 * 寄存器硬件锁只有IO硬件锁，数据区域硬件锁既有IO硬件锁又有DMA硬件锁
*/
#define USE_AES_SEMA				1 //1:AES使用硬件锁机制

/**
 ***************AES数据区域硬件锁使用注意****************
 * !!!注意该宏值必须与被调用的AES底层接口内变量ucCoreTypeOrIO一致
 * AES底层接口内变量ucCoreTypeOrIO默认值为0，因此AES数据区域默认使用IO硬件锁
*/
    #define USE_AES_DATA_SEMA_DMA	0 //AES数据区域使用IO硬件锁

/**
 **************AES硬件锁请求优先级使用注意**************** 
 * AES硬件锁请求优先级必须与AP侧的AES硬件锁的请求优先级保持一致，AP与CP默认均为0级
*/
#define AES_SEMA_REQ_PRI			SEMA_SEMA_REQ_PRI_0 //AES硬件锁请求优先级0


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
					  unsigned char *pucCipher);

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
					  unsigned char *ucM, unsigned char *pucMACT);

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
					unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit);

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
					unsigned char *pucMacT);

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
					unsigned char *pucCipher);

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
					unsigned char *pucMacI);


#ifdef __cplusplus
}
#endif

#endif  /* __HAL_AES_H__ */
