#ifndef _RF_DRV_H_
#define _RF_DRV_H_

#include "hw_dfe.h"
#include "hw_analog.h"
#include "hw_types.h"

#define XTAL_CLK                    38400000
#define	BBPLL_CLK_DEFAULT			368640000

#define      DFE_TX_CTRL						(0x80 + DFE_REG_BASE)
#define      DFE_RX_CTRL						(0x04 + DFE_REG_BASE)


// System Clock Select
extern void PRCM_Clock_Mode_Force_XTAL(void);
extern void PRCM_Clock_Mode_Auto(void);

extern void RF_BBPLL_Freq_Set(uint32_t ulFreqHz);
extern uint8_t RF_BBPLL_Lock_Status_Get(void);

void rf_trx_close(void);
#endif /* _RF_DRV_H_ */
