#ifndef __DFE_H__
#define __DFE_H__

#include "hw_dfe.h"
#include "PhyFRC_Time.h"
#include "phytimer.h"

//void rf_drv_delay(unsigned long uldelay);

#define RF_DL_FREQUENCY_FIXED_OFFSET (-600000)


#define GAIN_TABLE_SIZE_RX_HB_LNA   10
#define GAIN_TABLE_SIZE_RX_LB_LNA   10

#define GAIN_TABLE_SIZE_TX_LB_MIXER 20
#define GAIN_TABLE_SIZE_TX_LB_PAD   35
#define GAIN_TABLE_SIZE_TX_LB_PA    8

#define GAIN_TABLE_SIZE_TX_BB       10

#define GAIN_TABLE_SIZE_TX_HB_MIXER 20
#define GAIN_TABLE_SIZE_TX_HB_PAD   35
#define GAIN_TABLE_SIZE_TX_HB_PA    8

#define RX_HB_GAIN_LNA              1
#define RX_LB_GAIN_LNA              2
#define RX_BB_GAIN_PGA              3
#define RX_DFE_GAIN_CIC             4
#define RX_DFE_GAIN_ACI             5

#define TX_HB_MX_CAS_GC             1
#define TX_HB_PAD_GC                2
#define TX_HB_PA_GC                 3
#define TX_LB_MX_CAS_GC             4
#define TX_LB_PAD_GC                5
#define TX_LB_PA_GC                 6
#define TX_BB_GC                    7
#define TX_DFE_DVGA                 8

typedef struct {
	int16_t dBTen;
	uint8_t GainReg;
} TableType1;

typedef struct {
	int16_t dBTen;
	int16_t GainReg1;
    int16_t GainReg2;
} TableType2;

typedef struct {
	int16_t dBTen;
	int16_t GainReg1;
	int16_t GainReg2;
	int16_t GainReg3;
} TableType3;

//void DFERSTCTL_CLR(void);
//void DFERSTCTL_SET(void);

void DFE_TX_EN_DISABLE(void);
void DFE_TX_EN_ENABLE(void);
int IF_DFE_RX_IDLE(void);
int IF_DFE_TX_IDLE(void);
void DFE_RX_EN_DISABLE(void);
void DFE_RX_EN_ENABLE(void);
void DFE_TX_MODE(void);
void DFE_TX_MODE_Disable(void);
void DFE_TXFIFO_CLK_ENABLE(void);
//void DFE_TXFIFO_CLK_DISABLE(void);
//void TX_GAIN_DLY_BYPASS_DISABLE(void);
void TX_GAIN_DLY_BYPASS_ENABLE(void);
void TX_RF_DLY_BYPASS_DISABLE(void);
void TX_RF_DLY_BYPASS_ENABLE(void);

void DFE_HWTX_CTL_ENABLE(void);
void DFE_HWTX_CTL_DISABLE(void);
void TX_DDFS_TWOTONE_MODE_DISABLE(void);
void TX_DDFS_TWOTONE_MODE_ENABLE(void);
void TX_DDFS_BYPASS_DISABLE(void);
void TX_DDFS_BYPASS_ENABLE(void);
void TX_IQ_COMP_BYPASS_DISABLE(void);
//void TX_IQ_COMP_BYPASS_ENABLE(void);
void TX_DC_COMP_BYPASS_DISABLE(void);
//void TX_DC_COMP_BYPASS_ENABLE(void);
void TX_ACI_BYPASS_DISABLE(void);
void TX_ACI_BYPASS_ENABLE(void);
void TX_SRC_BYPASS_DISABLE(void);
void TX_SRC_BYPASS_ENABLE(void);
void TX_DDFS0_PHS_SET(int32_t ddfsPHS);
void TX_DDFS0_FCW_SET(int32_t ddfsHz);
//void TX_DDFS1_PHS_SET(int32_t ddfsPHS);
//void TX_DDFS1_FCW_SET(int32_t ddfsHz);

void TX_SRC_MUK_SET(int32_t srcMuk);
void TX_IQ_AMP_EST_SET(int16_t iqAMP);
void TX_IQ_PHS_EST_I_SET(int16_t iqPHS_I);
void TX_IQ_PHS_EST_Q_SET(int16_t iqPHS_Q);
void DFE_DVGA_SET(uint16_t dvgaVal);
void TX_DC_EST_I_SET(int16_t dc_I);
void TX_DC_EST_Q_SET(int16_t dc_Q);
//void TX_RF_DLY_SET(int32_t rfDelay);
//void TX_BB_GAIN_DLY_SET(int32_t bbGainDelay);

void DFE_RX_MODE(void);
void DFE_RXFIFO_CLK_ENABLE(void);
//void DFE_RXFIFO_CLK_DISABLE(void);
void DFE_RXFIFO_DLY_SET(uint16_t delay);
//void RX_GAIN_DLY_BYPASS_DISABLE(void);
void RX_GAIN_DLY_BYPASS_ENABLE(void);
//void RX_RF_DLY_BYPASS_DISABLE(void);
void RX_RF_DLY_BYPASS_ENABLE(void);


void DFE_HWRX_CTL_ENABLE(void);
void DFE_HWRX_CTL_DISABLE(void);
void RX_TSSI_DISABLE(void);
void RX_TSSI_ENABLE(void);
void RX_TSSI_CLR_VALID(void);
//uint8_t RX_TSSI_GET_VALID(void);
uint16_t RX_TSSI_GET_DATA(void);
void RX_SPUR_DET_DISABLE(void);
void RX_SPUR_DET_ENABLE(void);
//void RX_SPUR_DET_SET_AVG_NUM(uint8_t avgNum);
//void RX_SPUR_DET_START(void);
//void RX_SPUR_DET_CLEAR_DONE(void);
//uint8_t RX_SPUR_DET_GET_DONE(void);
//void RX_SPUR_DET_RAM_ADDR(uint8_t ramAddr);
//void RX_SPUR_DET_RAM_RE(void);
//uint16_t RX_SPUR_DET_RAM_DOUT(void);
//void RX_TNF_DISABLE(void);
void RX_TNF_ENABLE(void);
void RX_ACI_BW_SEL(uint8_t ACI_BW_MODE);
void RX_TNF0_BYPASS_DISABLE(void);
void RX_TNF0_BYPASS_ENABLE(void);
void RX_TNF1_BYPASS_DISABLE(void);
void RX_TNF1_BYPASS_ENABLE(void);
void RX_LO_FCW_SET(int data);
void RX_LO_PHS_ADJ_SET(uint32_t data);

void HW_RX_EN_ON(FRC_TIME_t *openFRC);
void HW_RX_EN_OFF(FRC_TIME_t *shutdownFRC);
void HW_RX_RF_ON(FRC_TIME_t *openFRC);
void HW_TX_EN_ON(FRC_TIME_t *openFRC);
void HW_RX_RF_OFF(FRC_TIME_t *shutdownFRC);

//void DFE_FIFO_RX_BYPASS_ENABLE();
void DFE_FIFO_RX_BYPASS_DISABLE();
//void DFE_FIFO_TX_BYPASS_ENABLE();
//void DFE_FIFO_TX_BYPASS_DISABLE();
void BB_FIFO_RX_BYPASS_ENABLE();
void BB_FIFO_RX_BYPASS_DISABLE();
//void BB_FIFO_TX_BYPASS_ENABLE();
//void BB_FIFO_TX_BYPASS_DISABLE();

void RF_SIDO1P8_VADJ(uint8_t vadj);
void RF_SIDO1P8_POWERUP(void);
void RF_SIDO1P8_POWEROFF(void);
void DFERF_CNTL_En(void);
void DFERF_CNTL_Dis(void);

//void RXBB_TIA_OS_BYPASS(void);
void RX_PGA_SWAP_IQ(uint8_t swap_flag);
void RXBB_TIA_OS_ENABLE(void);
void RXBB_TIA_OS_PUSH_I(uint8_t push_value);
void RXBB_TIA_OS_PULL_I(uint8_t pull_value);
void RXBB_TIA_OS_PUSH_Q(uint8_t push_value);
void RXBB_TIA_OS_PULL_Q(uint8_t pull_value);
void RXBB_TIA_OS_DCOC_IDAC_I(uint8_t iDac_I);
void RXBB_TIA_OS_DCOC_IDAC_Q(uint8_t iDac_Q);
//uint8_t RXBB_TIA_OS_PUSH_I_Get(void);
//uint8_t RXBB_TIA_OS_PULL_I_Get(void);
//uint8_t RXBB_TIA_OS_PUSH_Q_Get(void);
//uint8_t RXBB_TIA_OS_PULL_Q_Get(void);

int dfeRxInit(unsigned int dlRxLOFreqHz, int CFO);
void dfeRXOpen(FRC_TIME_t *openFRC);
void dfeRXClose(FRC_TIME_t *shutdownFRC);
void dfeRxGainSetByIdx(unsigned int dlFreqHz, uint8_t GainIdx);
int dfeRxGainGetByIdx(unsigned int dlFreqHz, uint8_t GainIdx);
void dfeTxGainSetBydBTen(int ulFreqHz, int16_t targetdBTen);


void dfeTxDDFS(unsigned int txLOFreqHz, int txPowdBm, int txDDFSFreqHz, unsigned int dvgaVal);
int dfeTxInit(unsigned int ulLOFreqHz, int CFO, int powerDB, uint8_t eventId, uint8_t bMaxTxPowerEnh, int16_t ssPowClassdBTen);

//int dfeTxInit(unsigned int ulLOFreqHz, int powerDB, unsigned int dvgaVal);

void dfeTxOpen(FRC_TIME_t *openFRC);
void dfeTxClose(FRC_TIME_t *shutdownFRC);
void RfTXClose(void);

void rf_trx_close(void);

extern void dfeRxSrcMukSet(int8_t k,int8_t flag);

//void rf_uart_init(void);


extern int 		gu32DDFSADCClk;
extern int  	gu8BPLL_K_Value;
extern int     guaBPLL_K_Fixed_Flag;
extern int     guaBPLL_K_Fixed_Value;

extern uint8_t g_sido_need_bypass;



extern void COMPENSATE_FREQOFFSET_AT_DFE(int CFO);
extern void DFE_SetCloseWaitFalg(int flag);
extern void nvRfInit();
extern void DFE_TXFIFO_CLK_ENABLE();
extern void DFE_TX_MODE();

extern int calcRxCalibrationAGC(uint32_t rxFreqLO, uint16_t index);
#endif
