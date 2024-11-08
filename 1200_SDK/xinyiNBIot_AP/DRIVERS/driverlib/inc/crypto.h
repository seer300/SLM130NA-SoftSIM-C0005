#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include "hw_crypto.h"

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

#define TRANSFER_MODE_IO			0
#define TRANSFER_MODE_DMA			1
#define TRANSFER_MODE_DMA_REQUEST	2
#define TRANSFER_MODE_DMA_RESULT	3
    
extern void AES_HardwareCtrlClock(void);
extern void AES_ClockAlwaysOn(void);
extern void AES_ClockAlwaysOn(void);
extern void AES_KeyLenSet(uint32_t KeyLenMode);
extern void AES_KeySet(uint8_t Offset, uint32_t KeyValue);
extern void AES_IVSet(uint8_t Offset, uint32_t IV);
extern void AES_AADLenSet(uint8_t Offset, uint32_t AadLen);
extern void AES_PayloadLenSet(uint32_t PayloadLenByte);
extern void AES_ModeSet(uint32_t AESMode);
extern void AES_EncDecSet(uint32_t AESEncDec);
extern void AES_SetCipherMode(uint8_t Mode);	
extern void AES_DMAEn(void);
extern void AES_DMADis(void);
extern void AES_BlockDataInput(uint8_t Offset, uint32_t DataIn);
extern uint32_t AES_BlockDataOutput(uint8_t Offset);
extern void AES_BlockStart(void);
extern void AES_KeyLoadWaitDone(void);
extern void AES_BlockTransWaitDone(void);
extern void S3G_ZUC_TransWaitDone(void);
extern void AES_ClockDiv2En(void);
extern void AES_ClockDiv2Dis(void);
extern void ZUCIntegrityEn(void);
extern void ZUCEncrypteEn(void);
extern void Snow3GIntegrityEn(void);
extern void Snow3GEncrypteEn(void);
extern void AES_ClearUiaAndEia(void);
extern void AES_EndianClear(void);
extern void AES_EndianInSmall(void);
extern void AES_EndianInBig(void);
extern void SZ1_EndianOutSmall(void);
extern void SZ1_EndianOutBig(void);
extern void SZ2_EndianOutSmall(void);
extern void SZ2_EndianOutBig(void);
extern void AES_CCMAuthLenSet(uint32_t AuthLenByte);
extern void AES_CCMLengthLenSet(uint8_t ByteLength);
extern uint32_t AES_TagGet(uint8_t Offset);
extern void AESBlock(uint8_t *pInput, uint8_t *pKey, 
	                 uint32_t KeyLen, uint8_t *pOutput, 
					 uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);					 
extern void AESECB(uint8_t *pInput, uint32_t InputLenByte, 
                   uint8_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pOutput, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);					   
extern void AESCBC(uint8_t *pInput, uint32_t InputLenByte, 
                   uint8_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pIV, uint8_t *pOutput, 
				   uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);			
extern void AESCFB(uint8_t *pInput, uint32_t InputLenByte, 
                   uint8_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pIV, uint8_t *pOutput, 
				   uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);						   
extern void AESOFB(uint8_t *pInput, uint32_t InputLenByte, 
                   uint8_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pIV, uint8_t *pOutput, 
				   uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);			
extern void AESCTR(uint8_t *pInput, uint32_t InputLenByte, 
                   uint8_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pIV, uint8_t *pOutput, 
				   uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);					   				   
extern void AESCCM(uint8_t *pAdata, uint32_t AdataByteLen, 
	               uint8_t *pPayload, uint32_t PayloadByteLen,
         	       uint32_t *pKey, uint32_t KeyBitsLen, 
				   uint8_t *pNonce, uint8_t NonceByteLen, 
				   uint8_t TagByteLen, uint8_t *pCipher, 
				   uint8_t *pDigest, uint32_t OperateMode, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);		
extern void AES_128_EEA2(uint8_t *pPlain, uint8_t *pKey, uint8_t *pCount,
	                     uint8_t Bearer, uint8_t Dir, uint32_t LengthBit,
						 uint8_t *pCipher, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);
extern void AES_128_EIA2(uint8_t *pMessage, uint8_t *pKey, uint8_t *pCount, 
	                     uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, 
					     uint8_t *pM, uint8_t *pMACT, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);
extern void AES_CMAC_XOR_128(uint8_t *pA, const uint8_t *pB, uint8_t *pOut);
extern void AES_CMAC_Leftshift_Onebit(uint8_t *pInput, uint8_t *pOutput);
extern void AES_CMAC_Generate_Subkey(uint8_t *pKey, uint8_t *pK1, uint8_t *pK2);					 
extern void snow_3g_basic(uint8_t *pInput, uint8_t *pKey, uint8_t *pIV, 
	                      uint8_t *pOutput, uint32_t LengthByte, 
						  uint8_t EndianOut, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);
extern void zuc_basic(uint8_t *pInput, uint8_t *pKey,
	                   uint8_t *pIV, uint8_t *pOutput, 
					  uint32_t LengthByte, uint8_t EndianOut, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);
extern void f_128_EEA1(uint8_t *pInput, uint8_t *pKey, 
	                   uint8_t *pCount, uint8_t Bearer, 
					   uint8_t Dir, uint32_t LengthBit, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);
extern void f_128_EIA1(uint8_t *pMessage, uint8_t *pKey, 
	                   uint8_t *pCountI, uint8_t Bearer, 
					   uint8_t Dir, uint32_t LengthBit, uint8_t *pMacT, uint8_t CoreTypeOrIO, uint32_t ChannelNum);
extern void f_128_EEA3(uint8_t *pKey, uint8_t *pCount, 
	                   uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, 
					   uint8_t *pPlain, uint8_t *pCipher, uint8_t CoreTypeOrIO, uint32_t DataInChannel, uint32_t DataOutChannel);				  
extern void f_128_EIA3(uint8_t *pKey, uint8_t *pCount, 
	                   uint8_t Bearer, uint8_t Dir, uint32_t LengthBit, 
					   uint8_t *pMessage, uint8_t *pMacI, uint8_t CoreTypeOrIO, uint32_t ChannelNum);
extern void sha1_process(uint8_t *pData, uint32_t lenBits, 
	                     uint8_t *pResult, uint8_t dmaFlag, uint8_t MemType, uint32_t ChannelNum);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __AES_H__
