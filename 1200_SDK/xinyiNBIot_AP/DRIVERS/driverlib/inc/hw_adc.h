#include "xinyi2100.h"

#ifndef __HW_ADC_H__
#define __HW_ADC_H__

//*****************************************************************************
//
// The following are defines for ADC register address.
//
//*****************************************************************************
#define ADC_CTRL_EN_Pos						0UL
#define ADC_AUX_DIV_Pos						1UL
#define ADC_SAMPLE_EN_Pos					2UL
#define ADC_DUTY_EN_Pos						3UL
#define ADC_AUX_PU_Pos						4UL
#define ADC_FUNC_TRIGGER_Pos				5UL

#define ADC_AUX_RDY_Pos						6UL
#define ADC_AUX_SIGN_SWAP_Pos				7UL

#define ADC_FIFO_THR_Pos					4UL

#define ADC_DMA_EN_Pos						0UL
#define ADC_FIFO_FLUSH_Pos					1UL
#define ADC_FORCE_NO_CLK_Pos				2UL
#define ADC_SCAN_EN_Pos						3UL
#define ADC_CHANNEL_NUM						4UL

#define ADC_FIFO_UPTHR_INT_Pos				3UL
#define ADC_FIFO_UPTHR_INT_Msk				(1UL << ADC_FIFO_UPTHR_INT_Pos)
#define ADC_FIFO_UPTHR_INT_En				(1UL << ADC_FIFO_UPTHR_INT_Pos)
#define ADC_FIFO_UPTHR_INT_Dis				(0UL << ADC_FIFO_UPTHR_INT_Pos)

#define ADC_FIFO_EMPTY_INT_Pos				2UL
#define ADC_FIFO_EMPTY_INT_Msk				(1UL << ADC_FIFO_EMPTY_INT_Pos)
#define ADC_FIFO_EMPTY_INT_En				(1UL << ADC_FIFO_EMPTY_INT_Pos)
#define ADC_FIFO_EMPTY_INT_Dis				(0UL << ADC_FIFO_EMPTY_INT_Pos)

#define ADC_FIFO_FULL_INT_Pos				1UL
#define ADC_FIFO_FULL_INT_Msk				(1UL << ADC_FIFO_FULL_INT_Pos)
#define ADC_FIFO_FULL_INT_En				(1UL << ADC_FIFO_FULL_INT_Pos)
#define ADC_FIFO_FULL_INT_Dis				(0UL << ADC_FIFO_FULL_INT_Pos)

#define ADC_FIFO_OVERFLOW_INT_Pos			0UL
#define ADC_FIFO_OVERFLOW_INT_Msk			(1UL << ADC_FIFO_OVERFLOW_INT_Pos)
#define ADC_FIFO_OVERFLOW_INT_En			(1UL << ADC_FIFO_OVERFLOW_INT_Pos)
#define ADC_FIFO_OVERFLOW_INT_Dis			(0UL << ADC_FIFO_OVERFLOW_INT_Pos)

#define ADC_DIV_Pos                         0UL
#define ADC_DIV_Msk                         (256UL << ADC_DIV_Pos)

#define ADC_Avrg_Pos                        16UL
#define ADC_Avrg_Msk                        (31UL << ADC_Avrg_Pos)

typedef struct
{
  __IO uint8_t scan_channel[8];
  __IO uint8_t ADC_CTRL0;
  __IO uint8_t ADC_CTRL1;
  __IO uint8_t ADC_CTRL2;  
  __IO uint8_t ADC_CTRL3;

  __IO uint8_t  ADC_SWITCH_TIME;
  __IO uint8_t  ADC_AVE_NUM;
  __IO uint8_t  ADC_INT_STAT;
  __I  uint8_t  ADC_FIFO_STAT;

  __IO uint32_t  ADCCTRL_CFG[8];
  
  __I  uint16_t  ADC_DIRECT_DATA;
  uint16_t   RESERVED0;

  __I  uint8_t	ADC_DIRECT_RDY;
  uint8_t	 RESERVED1[3];

  __IO uint32_t scan_channel_select;
  uint32_t	 RESERVED2;

  __I  uint32_t  ANAAUXADC;
} ADCCTRL_TypeDef;

#endif
