#ifndef __LCDC_H__
#define __LCDC_H__

#include "hw_types.h"
#include "hw_lcdc.h"
#include "hw_gpio.h"
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
//! \addtogroup lcdc_api
//! @{
//
//*****************************************************************************



//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************
extern void LCDC_TmuxSelSet(uint8_t ucValue);
extern void LCDC_DutySet(uint8_t ucValue);
extern void LCDC_BiasSet(uint8_t ucValue);
extern void LCDC_BypassEnSet(uint8_t ucValue);
extern void LCDC_LwaveSet(uint8_t ucValue);
extern uint8_t LCDC_LwaveGet(void);
extern void LCDC_ForceclkEnable(void);
extern void LCDC_ForceclkDisable(void);
extern void LCDC_BlinkSet(uint8_t ucValue);
extern void LCDC_DeadSet(uint8_t ucValue);
extern void LCDC_UvolVlcdCHSSet(uint8_t ucValue);
extern void LCDC_UvolLCPGainSet(uint8_t ucValue);
extern void LCDC_UvolSupplySelSet(uint8_t ucValue);
extern void LCDC_UvolClkCtrl(uint8_t ucValue);
extern void LCDC_FCRpsSet(uint8_t ucValue);
extern void LCDC_FCRLcdPuSet(uint8_t ucValue);
extern void LCDC_FCRdivSet(uint8_t ucValue);
extern void LCDC_UPDATEReqSet(uint8_t ucValue);
extern void LCDC_ComsegmuxSet(uint8_t ucValue);
extern uint8_t LCDC_ComsegmuxGet(void);
extern void LCDC_WriteCom(uint8_t ComNum,uint8_t SegNum,uint8_t *SegGroup,uint8_t *UserPattern);
extern uint32_t LCDC_GetSeg(uint8_t ComNum,uint8_t SegNum,uint8_t *SegGroup,uint8_t *UserPattern,uint32_t *ValueLow,uint32_t *ValueHigh);
extern uint32_t LCDC_GetSegX(uint8_t ComNum,uint8_t UserPattern);
extern void LCDC_WriteComXValue(uint8_t ComX,  uint32_t ValueLow, uint32_t ValueHigh);
extern void LCDC_ComBiasSet(uint8_t comNum);
extern void LCDC_CTRL0Set(uint8_t tmuxSel,uint8_t duty,uint8_t comNum,int bypassEn);
extern void LCDC_CTRL1Set(uint8_t blinkSet,uint8_t deadSet,int lwaveSet,int Forceclk);
extern void LCDC_FDRSet(uint8_t rpsSet,int div);
extern void LCDC_UVolSet(uint8_t lcpGain, int ClkCtrl, int supplySel,int vlcdCHS);
extern void LCDC_SegGPIOSet(GPIO_PadTypeDef PadNum);
extern void LCDC_WORKStart(void);
extern void LCDC_WORKUpdateWait(uint32_t LcdRate);
extern void LCDC_WORKUpdate(void);
extern void LCDC_WORKStop(void);
extern void LCDC_ENStart(void);
extern void LCDC_ENStop(void);
extern void LCDC_PAD_Enable_All(void);
extern void LCDC_PAD_Disable(void);
extern void LCDC_PWR_Force_On(void);
extern void LCDC_Init_Default(uint32_t Segpad,uint32_t ComPad);
extern void LCDC_Init_Config(uint8_t mux,uint8_t rps,uint8_t div,uint8_t SupplySel,uint8_t VLCDchs,uint8_t Duty,uint8_t Bias,uint8_t BypassEn);
extern void LCDC_PAD_SetMode(uint32_t Segpad,uint32_t ComPad);
//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  __LCDC_H__
