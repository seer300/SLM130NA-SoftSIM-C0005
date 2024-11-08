#ifndef _RX_CALIBRATION_H_
#define _RX_CALIBRATION_H_

//#define DCOC_CURRENT_BASE_STEP 		(6.38)
//#define DCOC_TIA_PUSH_PULL_STEP		(1.75)
#define DCOC_CURRENT_BASE_STEP 		(11.35)
#define DCOC_TIA_PUSH_PULL_STEP		(2.33)

typedef struct 
{
	uint8_t rxTIAPush_I;
	uint8_t rxTIAPull_I;
	uint8_t rxTIAPush_Q;
	uint8_t rxTIAPull_Q;
	uint8_t dcCurrentBase_I;
	uint8_t dcCurrentBase_Q;
}RX_DC_TIA_State;

extern void DCOffsetCalibrationInit(uint32_t dlRxLOFreqHz, uint8_t pgaGain);

extern void getDCOffsetVal(int8_t *dcI, int8_t *dcQ, int iq_swap);

extern void calDcocTest(uint32_t dlRxLOFreqHz, int16_t tempera);

//extern void RXBB_TIA_OS_BYPASS(void);
extern void RXBB_TIA_OS_PUSH_I(uint8_t push_value);
extern void RXBB_TIA_OS_PULL_I(uint8_t pull_value);
extern void RXBB_TIA_OS_PUSH_Q(uint8_t push_value);
extern void RXBB_TIA_OS_PULL_Q(uint8_t pull_value);

extern void RXBB_TIA_OS_DCOC_IDAC_I(uint8_t iDac_I);
extern void RXBB_TIA_OS_DCOC_IDAC_Q(uint8_t iDac_Q);

extern void RXBB_TIA_OS_ENABLE(void);
extern int8_t get_idx_by_tempera(int16_t tempera);
extern void calDcocNvSave(void);
extern void rf_rx_dc_nv_set(uint32_t dlLOFreqHz, int16_t tempera, uint8_t rxGainIndex);
extern void selfRxDcocCalibration(uint32_t dlRxLOFreqHz);
extern int8_t selfRxDcocCalibrationFlag(uint32_t dlRxLOFreqHz);

#endif /* _TX_CALIBRATION_H_ */
