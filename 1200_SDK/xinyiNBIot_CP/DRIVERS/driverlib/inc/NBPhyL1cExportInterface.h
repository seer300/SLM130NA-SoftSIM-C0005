#ifndef _NBPHY_L1C_EXPORT_INTERFACE_H_
#define _NBPHY_L1C_EXPORT_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "PhyBasicType.h"

/*set exit lower flag*/
extern unsigned int get_elapsed_1ms_counts_standby(void);
extern void NL1cSetStandbyExitStatus(void);
extern void NL1cBackupInfoSaveToRetentionMem(void);
extern void PhyInit();
extern void PhyNL1cGetMeasRslt(int16_t *sSinr, int16_t *sNrsrp, int16_t *sRssi);
extern uint32_t PhyL1cGetRetentionMemSize(void);
extern void PHY_RXAGC_FixedGain(int16_t s16Enable, int16_t s16GainIdx);
extern void NL1cRLMSetOffset(int16_t sOutOfSyncOffset, int16_t sInSyncOffset);

#ifdef __cplusplus
}
#endif

#endif





