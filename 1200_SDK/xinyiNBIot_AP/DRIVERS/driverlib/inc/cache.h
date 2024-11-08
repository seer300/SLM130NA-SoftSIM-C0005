#ifndef __CACHE_H__
#define __CACHE_H__

#include "hw_types.h"
#include "hw_cache.h"
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

//*****************************************************************************
//
//! \addtogroup cache_api
//! @{
//
//*****************************************************************************



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void CacheEN(unsigned long ulBase);
extern void CacheDis(unsigned long ulBase);
extern void CacheBypassEN(unsigned long ulBase);
extern void CacheBypassDis(unsigned long ulBase);
extern unsigned char CacheEnabled(unsigned long ulBase);
extern void CacheStatisticEN(unsigned long ulBase);
extern void CacheStatisticDis(unsigned long ulBase);
extern void CacheWbPolicySet(unsigned long ulBase, unsigned char ucValue);
extern void CacheEntryCacheEN(unsigned long ulBase);
extern void CacheEntryCacheDis(unsigned long ulBase);
extern void CacheFlushEn(unsigned long ulBase);
extern void CacheCheckEn(unsigned long ulBase);
extern void CacheWRCleanEn(unsigned long ulBase);
extern unsigned char CacheEnableStatusGet(unsigned long ulBase);
extern unsigned char CacheFlushStatusGet(unsigned long ulBase);
extern unsigned char CacheOperationStatusGet(unsigned long ulBase);
extern unsigned char CacheLineValidGet(unsigned long ulBase);
extern unsigned char CacheLineDirtyGet(unsigned long ulBase);
extern unsigned long CacheRDHitCntGet(unsigned long ulBase);
extern unsigned long CacheRDMissCntGet(unsigned long ulBase);
extern void CacheRDHitCntSet(unsigned long ulBase, unsigned long value);
extern void CacheRDMissCntSet(unsigned long ulBase, unsigned long value);
extern void CacheWaysIdxSet(unsigned long ulBase, unsigned char ucValue);
extern unsigned char CacheWaysIdxGet(unsigned long ulBase);
extern void CacheSetIdxSet(unsigned long ulBase, unsigned long ulValue);
extern unsigned long CacheSetIdxGet(unsigned long ulBase);
extern unsigned long CacheElrAddrGet(unsigned long ulBase);
extern void CacheCRESet(unsigned long ulBase, unsigned long ulValue);
extern unsigned long CacheCREGet(unsigned long ulBase);
extern unsigned long CacheWRHitCntGet(unsigned long ulBase);
extern unsigned long CacheWRMissCntGet(unsigned long ulBase);
extern unsigned long CacheWRHitCntSet(unsigned long ulBase, unsigned long value);
extern unsigned long CacheWRMissCntSet(unsigned long ulBase, unsigned long value);
extern unsigned short CacheParamsGet(unsigned long ulBase);
extern void CacheBAddrSet(unsigned long ulBase, unsigned long ulValue);
extern unsigned long CacheBAddrGet(unsigned long ulBase);
extern void CacheTAddrSet(unsigned long ulBase, unsigned long ulValue);
extern unsigned long CacheTAddrGet(unsigned long ulBase);
extern void CacheIntRegister(unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void CacheIntUnregister(unsigned long *g_pRAMVectors);
extern void CacheIntMask(unsigned long ulBase, unsigned char ucConfig);
extern unsigned char CacheReadAndClearInt(unsigned long ulBase);
extern void CacheSetRange(unsigned long ulBase, unsigned long ulBAddr, unsigned long ulTAddr);
extern void CacheSetRangeIndex(unsigned long ulBase, unsigned long ulBAddr, unsigned long ulTAddr,unsigned char index);
extern void CacheEventClear(unsigned long ulBase);
extern void CacheCleanAll(unsigned long ulBase);
extern void CacheCleanInvalidAll(unsigned long ulBase);
extern void CacheFlush(unsigned long ulBase);


/*6.5M主频下耗时约40us,若极低功耗需要，可以关闭CACHE_ENABLE_SET*/
/*****************************************************************************************************
* @brief  用于开启AP核的cache功能，目前只有4K大小，且仅支持代码段运行在flash上的cache功能。耗时40us
* @note   在define.cmake里打开CACHE_ENABLE_SET宏即可
* @attention 深睡会下电，进而实际提升不大，默认未开启
*****************************************************************************************************/
extern void CacheInit(unsigned long ulBase, unsigned char ucWbPolicy, unsigned long ulBAddr, unsigned long ulTAddr);    
extern void CacheInit_Ext(unsigned long ulBase, unsigned char ucWbPolicy, unsigned long ulBAddr, unsigned long ulTAddr);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __CACHE_H__

