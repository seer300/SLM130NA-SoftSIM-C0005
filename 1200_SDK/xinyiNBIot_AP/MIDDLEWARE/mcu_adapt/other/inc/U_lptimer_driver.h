/**
  ******************************************************************************
  * @file    hc32_timer_driver.h
  * @author  (C)2020, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2020-7-1
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
	 
#define TRUE					(1)
#define FALSE					(0)
 
/* Function Declare------------------------------------------------------------*/	 
void LPTimer0Init(u8 Rtc_Clk_Source, u32 time);
void LPTimer1Init(u8 Rtc_Clk_Source);
u16 LPTimer1GetTick(void);
void LPTimer0_Work(u8 enCmd);
void ClearLpTimerFlag(void);
u8 GetLpTimerFlag(void);
u8 LpTime0IfSleep(void);
#ifdef __cplusplus
}
#endif

//#endif /* __U_TIMER_DRIVER_H */

