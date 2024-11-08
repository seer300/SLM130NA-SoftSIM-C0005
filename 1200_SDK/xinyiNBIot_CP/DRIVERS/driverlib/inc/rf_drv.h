#ifndef _RF_DRV_H_
#define _RF_DRV_H_

#include "hw_dfe.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "prcm.h"

//#define XTAL_CLK                    26000000
//#define	BBPLL_CLK_DEFAULT			368640000

typedef enum {
    AccessModeUnknow = 0,
    AccessModeByte = 1,
    AccessModeWord = 2,
} RegAccessMode;

#define RFPLL_BAND_MODE_LOW	1
#define LO_HIGH_LOW_BAND_Hz 965000000
#define LO_B5_B8_BAND_Hz    868000000


#define RFPLL_DIV1_RANGE	2250000000
#define RFPLL_DIV2_RANGE	1100000000
#define RFPLL_DIV3_RANGE	 550000000
#define RFPLL_DIV4_RANGE	 366000000
#define RFPLL_DIV5_RANGE	 275000000
#define RFPLL_DIV6_RANGE	 183000000

// [191:188] = {tx_l, rx_l, rx_h, tx_h}
#define BAND_ENABLE_TXL             0x08
#define BAND_ENABLE_RXL             0x04
#define BAND_ENABLE_RXH             0x02
#define BAND_ENABLE_TXH             0x01

extern uint8_t REG_Bus_Field_Set(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long ulValue);

//extern void rf_drv_delay(unsigned long uldelay);

void RF_RFLDO_CNTL_En(void);
void RF_RFPLL_LO_Div_Set(uint8_t ucValue);
void RF_RFPLL_VCO_High_Band_Set(void);
void RF_RFPLL_VCO_Low_Band_Set(void);
//void RF_Band_Enable_Set(uint8_t ucValue);

extern void RF_RFPLL_LO_Freq_Set(uint32_t ulFreqHz, uint8_t trx_flag);

void RF_BBLDO_CNTL_En(void);
void RF_RFLDO_Power_On(void);
void RF_BBLDO_Power_On(void);
void RF_TX_BBLDO_Power_On(void);
void RF_TX_BBLDO_Power_Off(void);
void RF_BBLDO_TRX_EN(void);
void RF_TXBB_Power_On(void);
void RF_TXBB_RSTB(void);
void RF_TX_MIXLDO_Power_On(void);
void RF_TX_SIDO1p8_En(void);

void RF_TXLB_BIAS_Power_On(void);
void RF_TXLB_MX_Power_On(void);
void RF_TXLB_PAD_Power_On(void);
void RF_TXLB_PA_Power_On(void);
void RF_TXHB_BIAS_Power_On(void);
void RF_TXHB_MX_Power_On(void);
void RF_TXHB_PAD_Power_On(void);
void RF_TXHB_PA_Power_On(void);

void RF_TXLB_BIAS_Power_Off(void);
void RF_TXLB_MX_Power_Off(void);
void RF_TXLB_PAD_Power_Off(void);
void RF_TXLB_PA_Power_Off(void);

void RF_TXHB_BIAS_Power_Off(void);
void RF_TXHB_MX_Power_Off(void);
void RF_TXHB_PAD_Power_Off(void);
void RF_TXHB_PA_Power_Off(void);

void RF_RXBB_TIA_Power_on(void);
void RF_RXBB_PGA_BIAS_Power_On(void);
void RF_RXBB_PGA_I_Power_On(void);
void RF_RXBB_PGA_Q_Power_On(void);
void RF_RXBB_ADC_BIAS_Power_On(void);
void RF_RXBB_ADC_Power_On(void);
void RF_RXBB_ADC_RSTB(void);
void RF_RXHB_LNTA_BIAS_Power_On(void);
void RF_RXHB_LNTA_Power_On(void);
void RF_RXHB_LO_Power_On(void);
void RF_RXHB_LNTA_HG_RST(void);
void RF_RXHB_LNTA_LG_RST(void);
void RF_RXLB_LNTA_BIAS_Power_On(void);
void RF_RXLB_LNTA_Power_On(void);
void RF_RXLB_LO_Power_On(void);
void RF_RXLB_LNTA_HG_RST(void);
void RF_RXLB_LNTA_LG_RST(void);

void rf_tx_poweroff(uint32_t txLOFreqHz);
//extern void RF_TX_On(uint32_t ulLOFreqHz);

extern void RF_RX_On(uint32_t dlLOFreqHz);



extern void RF_RFPLL_Set_SSBREP(uint32_t ulFreqHz, int16_t waitUsCount);
extern void RF_RFPLL_Set_NormalMode(uint8_t trx_flag,uint32_t ulFreqHz);
extern void RF_RFPLL_Set_EXTMode(uint8_t trx_flag,uint32_t ulFreqHz);
extern void RF_RFPLL_Set_TestMode(uint8_t trx_flag,uint32_t ulFreqHz);
extern void RF_RFPLL_Set_TRXMode(uint8_t trx_flag,uint32_t ulFreqHz);
#endif /* _RF_DRV_H_ */
