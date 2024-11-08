#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include "hw_crypto.h"
#include "hw_prcm.h"
#include "hw_ints.h"
#include "hw_types.h"
#include "dma.h"
#include "interrupt.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#define AES_MODE_DECRYPT        0
#define AES_MODE_ENCRYPT        1
	
#define CIPHER_MODE_AES         0
#define CIPHER_MODE_ZUC         1
#define CIPHER_MODE_SNOW3G      3
	
#define AES_BLOCK_BYTES         16

#define AES_KEY_LEN_BITS_128    128
#define AES_KEY_LEN_BITS_192    192
#define AES_KEY_LEN_BITS_256    256
	
#define ENDIAN_BIG              1
#define ENDIAN_SMALL            0

#define TRANSFER_MODE_DMA		1
#define TRANSFER_MODE_IO		0

extern void AESHardwareCtrlClock(void);
extern void AESClockAlwaysOn(void);
extern void AESClockAlwaysOn(void);
extern void AESKeyLenSet(unsigned long ulKeyLenMode);
extern void AESKeySet(unsigned char ucOffset, unsigned long ulKeyValue);
extern void AESIVSet(unsigned char ucOffset, unsigned long ucIV);
extern void AESAADLen0Set(unsigned long ulAadLen0);
extern void AESAADLen1Set(unsigned long ulAadLen1);
extern void AESPayloadLenSet(unsigned long ulPayloadLenByte);
extern void AESModeSet(unsigned long ucAESMode);
extern void AESEncDecSet(unsigned long ucAESEncDec);
extern void SetCipherMode(unsigned char ucMode);	
extern void AESDMAEn(void);
extern void AESDMADis(void);
extern void AESBlockDataInput(unsigned char ucOffset, unsigned long ulDataIn);
extern unsigned long AESBlockDataOutput(unsigned char ucOffset);
extern void AESBlockStart(void);
extern void AESKeyLoadWaitDone(void);
extern void AESBlockTransWaitDone(void);
extern void S3G_ZUC_TransWaitDone(void);
extern void AESClockDiv2En(void);
extern void AESClockDiv2Dis(void);
extern void ZUCIntegrityEn(void);
extern void ZUCEncrypteEn(void);
extern void Snow3GIntegrityEn(void);
extern void Snow3GEncrypteEn(void);
extern void AESClearUiaAndEia(void);
extern void AES_Endian_Clear(void);
extern void AES_Endian_In_Small(void);
extern void AES_Endian_In_Big(void);
extern void SZ1_Endian_Out_Small(void);
extern void SZ1_Endian_Out_Big(void);
extern void SZ2_Endian_Out_Small(void);
extern void SZ2_Endian_Out_Big(void);
extern void AESCCMAuthLenSet(unsigned long ucAuthLenByte);
extern void AESCCMLengthLenSet(unsigned char ucLengthLenByte);
extern unsigned long AESTagGet(unsigned char ucOffset);
extern void AESBlock(unsigned char *pucInput, unsigned char *pucKey, 
	                 unsigned long ulKeyLen, unsigned char *pucOutput, 
					 unsigned char ucOperateMode, unsigned char ucCoreTypeOrIO);					 
extern void AESECB(unsigned char *pucInput, unsigned long ulInputLenByte, 
                   unsigned char *pucKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucOutput, unsigned long ucOperateMode, unsigned char ucCoreTypeOrIO);				   
extern void AESCBC(unsigned char *pucInput, unsigned long ulInputLenByte, 
                   unsigned char *pucKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucIV, unsigned char *pucOutput, 
				   unsigned long ucOperateMode, unsigned char ucCoreTypeOrIO);		
extern void AESCFB(unsigned char *pucInput, unsigned long ulInputLenByte, 
                   unsigned char *pucKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucIV, unsigned char *pucOutput, 
				   unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO);					   
extern void AESOFB(unsigned char *pucInput, unsigned long ulInputLenByte, 
                   unsigned char *pucKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucIV, unsigned char *pucOutput, 
				   unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO);		
extern void AESCTR(unsigned char *pucInput, unsigned long ulInputLenByte, 
                   unsigned char *pucKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucIV, unsigned char *pucOutput, 
				   unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO);				   				   
extern void AESCCM(unsigned char *pucAdata, unsigned long ulAdataByteLen, 
	               unsigned char *pucPayload, unsigned long ulPayloadByteLen,
         	       unsigned long *pulKey, unsigned long ulKeyBitsLen, 
				   unsigned char *pucNonce, unsigned char ulNonceByteLen, 
				   unsigned char ulTagByteLen, unsigned char *pucCipher, 
				   unsigned char *pucDigest, unsigned long ulOperateMode, unsigned char ucCoreTypeOrIO);
extern void AES_128_EEA2(unsigned char *pucPlain, unsigned char *pucKey, unsigned char *pucCount,
	                     unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit,
						 unsigned char *pucCipher);
extern void AES_128_EIA2(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCount, 
	                     unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, 
					     unsigned char *ucM, unsigned char *pucMACT);
extern void AES_CMAC_XOR_128(unsigned char *pucA, const unsigned char *pucB, unsigned char *pucOut);
extern void AES_CMAC_Leftshift_Onebit(unsigned char *pucInput, unsigned char *pucOutput);
extern void AES_CMAC_Generate_Subkey(unsigned char *pucKey, unsigned char *pucK1, unsigned char *pucK2);					 
extern void snow_3g_basic(unsigned char *pucInput, unsigned char *pucKey, unsigned char *pucIV, 
	                      unsigned char *pucOutput, unsigned long ulLengthByte, 
						  unsigned char ucEndianOut, unsigned char ucCoreTypeOrIO);
extern void zuc_basic(unsigned char *pucInput, unsigned char *pucKey,
	                   unsigned char *pucIV, unsigned char *pucOutput, 
					  unsigned long ulLengthByte, unsigned char ucEndianOut, unsigned char ucCoreTypeOrIO);
extern void f_128_EEA1(unsigned char *pucInput, unsigned char *pucKey, 
	                   unsigned char *pucCount, unsigned char ucBearer, 
					   unsigned char ucDir, unsigned long ulLengthBit);
extern void f_128_EIA1(unsigned char *pucMessage, unsigned char *pucKey, 
	                   unsigned char *pucCountI, unsigned char ucBearer, 
					   unsigned char ucDir, unsigned long ulLengthBit, unsigned char *pucMacT);
extern void f_128_EEA3(unsigned char *pucKey, unsigned char *pucCount, 
	                   unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, 
					   unsigned char *pucPlain, unsigned char *pucCipher);				  
extern void f_128_EIA3(unsigned char *pucKey, unsigned char *pucCount, 
	                   unsigned char ucDir, unsigned char ucBearer, unsigned long ulLengthBit, 
					   unsigned char *pucMessage, unsigned char *pucMacI);
extern void sha1_process(unsigned char *pucData, unsigned long lenBits, 
	                     unsigned char *pucResult, unsigned char dmaFlag);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __AES_H__
