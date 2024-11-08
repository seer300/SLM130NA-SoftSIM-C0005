#ifndef _TX_CALIBRATION_H_
#define _TX_CALIBRATION_H_

extern void tssiTimerCfg(void);
extern void tssiTimer4Isr(void);
extern void calDCTest(uint32_t txLOFreqHz, int16_t txPowdBm, int16_t tempera);
extern void calAMPTest(uint32_t txLOFreqHz, int16_t txPowdBm, int16_t tempera);
extern void rf_dc_iq_set(uint32_t txLOFreqHz, int16_t tempera, int16_t *pDcI, int16_t *pDcQ, int16_t *pIqAmp);
extern void selfCalibration(void);
extern uint8_t selfCalibrationFlag(void);
extern void TSSI_RxClose(void);

#endif /* _TX_CALIBRATION_H_ */
