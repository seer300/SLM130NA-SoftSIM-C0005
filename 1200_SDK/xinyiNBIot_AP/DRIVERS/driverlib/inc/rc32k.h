#ifndef __RC32K_CALI_H__
#define __RC32K_CALI_H__

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

#include "xy_memmap.h"
//*****************************************************************************
//
// Values that can be passed to TimerConfigure as the ulConfig parameter.
//
//*****************************************************************************

extern uint32_t PRCM_32KClkSrcGet(void);
extern void PRCM_SelectXtal32k(void);
extern void PRCM_SelectRC32k(void);

extern void RC32K_Ctrl_Reset(void);
extern void RC32K_CALIB_WakeUp_Dis(void);
extern void RC32K_CALIB_WakeUp_En(void);
extern void RC32K_CALIB_En(void);
extern void RC32K_CALIB_Dis(void);
extern void RC32K_CALIB_TCOUNT_Set(uint8_t tCountVal);
extern void RC32K_CALIB_SDM_En(void);
extern int8_t rc32k_get_temperature(uint16_t oscCalVal, uint8_t lptsResoVal, uint8_t init_pro_flag);
extern void RC32K_CALIB_Done(void);
extern void RC32K_CALIB_LOOKUP_TABLE_Set(uint8_t tempAddr, uint8_t capSelVal, uint8_t sdmDataVal);
extern void RC32K_CALIB_LOOKUP_TABLE_Get(uint8_t tempAddr, uint8_t *capSelVal, uint8_t *sdmDataVal);
extern uint8_t RC32K_CALIB_CAPSEL_GetCur(void);
extern uint8_t RC32K_CALIB_SDMDATA_GetCur(void);
extern void RC32K_CALIB_NV_Save(uint8_t capSelVal, uint8_t sdmDataVal);
extern short RC32K_CALIB_TEMP_VAL_Get(void);
extern void RC32K_CALIB_FORCE_SDM_OUT_DATA(uint8_t sdmOutVal);
extern void RC32K_CALIB_CAPSEL_INIT_Set(uint8_t capSelInitVal);
extern void RC32K_CALIB_TEMP_VALID_Clr(void);
extern unsigned short RC32K_CALIB_OSC_PTAT_Get(void);
extern uint8_t RC32K_CALIB_TEMP_is_VALID(void);
extern void RC32K_CALIB_LPTS_RESO_Set(uint8_t lptsResoVal);
extern void RC32K_CALIB_OSC_CAL_Set(uint16_t oscCalVal);
extern uint8_t rc32k_get_last_temperature(void);
extern void RC32K_CALIB_OSC_REF_Set(uint16_t oscRefVal);
//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
