#ifndef __PHYTIMER_H__
#define __PHYTIMER_H__

#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "interrupt.h"
#include "gpio.h"
#include "PhyFRC_Time.h"
#include "cmsis_os2.h"
//*****************************************************************************
//
// The following are defines for Phy timer register address.
//
//*****************************************************************************
#define PHYTIMER_TMRL_CNT				(0x00 + PHYTIMER_BASE)
#define PHYTIMER_TMRM_CNT				(0x02 + PHYTIMER_BASE)
#define PHYTIMER_TMRH_CNT				(0x04 + PHYTIMER_BASE)

#define PHYTIMER_TMRL_RLD				(0x08 + PHYTIMER_BASE)
#define PHYTIMER_TMRM_RLD				(0x0A + PHYTIMER_BASE)
#define PHYTIMER_TMRH_RLD				(0x0C + PHYTIMER_BASE)

#define PHYTIMER_TMRL_OFFSET			(0x10 + PHYTIMER_BASE)
#define PHYTIMER_TMRM_OFFSET			(0x12 + PHYTIMER_BASE)
										
#define PHYTIMER_TMRH_OFFSET			(0x14 + PHYTIMER_BASE)

#define PHYTIMER_CTL					(0x18 + PHYTIMER_BASE)	
#define PHYTIMER_STAT					(0x1A + PHYTIMER_BASE)
#define PHYTIMER_INT_STAT				(0x1C + PHYTIMER_BASE)
#define PHYTIMER_INT_ENA				(0x1E + PHYTIMER_BASE)
#define PHYTIMER_SNAPF					(0x20 + PHYTIMER_BASE)
#define PHYTIMER_TRX0_CFG_CTL			(0x24 + PHYTIMER_BASE)
#define PHYTIMER_TRX1_CFG_CTL			(0x25 + PHYTIMER_BASE)
#define PHYTIMER_TRX2_CFG_CTL			(0x26 + PHYTIMER_BASE)
#define PHYTIMER_TRX3_CFG_CTL			(0x27 + PHYTIMER_BASE)
#define PHYTIMER_TRX0_CFG_CNT			(0x28 + PHYTIMER_BASE)

#define PHYTIMER_TMRL_SNAPSHOT			(0x2C + PHYTIMER_BASE)
#define PHYTIMER_TMRM_SNAPSHOT			(0x2E + PHYTIMER_BASE)
#define PHYTIMER_TMRH_SNAPSHOT			(0x30 + PHYTIMER_BASE)

#define PHYTIMER_DFECLKPHSADJ			(0x34 + PHYTIMER_BASE)
#define PHYTIMER_SFW1_CFG_CNT			(0x38 + PHYTIMER_BASE)
#define PHYTIMER_SFW2_CFG_CNT			(0x3C + PHYTIMER_BASE)
#define PHYTIMER_TRX1_CFG_CNT			(0x40 + PHYTIMER_BASE)
#define PHYTIMER_TRX2_CFG_CNT			(0x44 + PHYTIMER_BASE)
#define PHYTIMER_TRX3_CFG_CNT			(0x48 + PHYTIMER_BASE)

#define PHYTIMER_TMRL_DIR_Pos			15
#define	PHYTIMER_TMRL_DIR_Msk			(1UL << PHYTIMER_TMRL_DIR_Pos)

#define PHYTIMER_TMRM_DIR_Pos			7
#define	PHYTIMER_TMRM_DIR_Msk			(1UL << PHYTIMER_TMRM_DIR_Pos)

#define PHYTIMER_TMRH_DIR_Pos			23
#define	PHYTIMER_TMRH_DIR_Msk			(1UL << PHYTIMER_TMRH_DIR_Pos)

#define PHYTIMER_ENA_Pos				0
#define PHYTIMER_ENA_Msk				(1UL << PHYTIMER_ENA_Pos)
#define PHYTIMER_UTC_ALARM_EN_Pos		1
#define PHYTIMER_UTC_ALARM_EN_Msk		(1UL << PHYTIMER_UTC_ALARM_EN_Pos)

#define PHYTIMER_UTC_ALARM_RESET_Pos	2
#define PHYTIMER_UTC_ALARM_RESET_Msk		(1UL << PHYTIMER_UTC_ALARM_RESET_Pos)

#define PHYTIMER_TMRL_POL_Pos			6
#define PHYTIMER_UTC_ALARM_EN_Msk		(1UL << PHYTIMER_UTC_ALARM_EN_Pos)


#define PHYTIMER_TMRL_INT_EN_Pos		0
#define PHYTIMER_TMRL_INT_EN_Msk		(1UL << PHYTIMER_TMRL_INT_EN_Pos)
#define PHYTIMER_TMRM_INT_EN_Pos		1
#define PHYTIMER_TMRM_INT_EN_Msk		(1UL << PHYTIMER_TMRM_INT_EN_Pos)
#define PHYTIMER_TMRH_INT_EN_Pos		2
#define PHYTIMER_TMRH_INT_EN_Msk		(1UL << PHYTIMER_TMRH_INT_EN_Pos)
#define PHYTIMER_TRX0MATCH_INT_EN_Pos	3
#define PHYTIMER_TRX0MATCH_INT_EN_Msk	(1UL << PHYTIMER_TRX0MATCH_INT_EN_Pos)
#define PHYTIMER_TRX1MATCH_INT_EN_Pos	4
#define PHYTIMER_TRX1MATCH_INT_EN_Msk	(1UL << PHYTIMER_TRX1MATCH_INT_EN_Pos)
#define PHYTIMER_TRX2MATCH_INT_EN_Pos	5
#define PHYTIMER_TRX2MATCH_INT_EN_Msk	(1UL << PHYTIMER_TRX2MATCH_INT_EN_Pos)
#define PHYTIMER_TRX3MATCH_INT_EN_Pos	6
#define PHYTIMER_TRX3MATCH_INT_EN_Msk	(1UL << PHYTIMER_TRX3MATCH_INT_EN_Pos)
#define PHYTIMER_SFW1MATCH_INT_EN_Pos	7
#define PHYTIMER_SFW1MATCH_INT_EN_Msk	(1UL << PHYTIMER_SFW1MATCH_INT_EN_Pos)

#define PHYTIMER_SFW2MATCH_INT_EN_Pos	8
#define PHYTIMER_SFW2MATCH_INT_EN_Msk	(1UL << PHYTIMER_SFW2MATCH_INT_EN_Pos)
//#define PHYTIMER_HW_TRX_ENA_Pos			0
//#define PHYTIMER_HW_TRX_ENA_Msk			(3UL << PHYTIMER_HW_TRX_ENA_Pos)

#define PHYTIMER_CNT_ENA_Pos			0
#define PHYTIMER_CNT_ENA_Msk			(1UL << PHYTIMER_CNT_ENA_Pos)
#define PHYTIMER_HW_TRX_EN_Pos			1
#define PHYTIMER_HW_TRX_EN_Msk			(1UL << PHYTIMER_HW_TRX_EN_Pos)

#define PHYTIMER_TMRL_INT_STAT_Pos		0
#define PHYTIMER_TMRL_INT_STAT_Msk		(1UL << PHYTIMER_TMRL_INT_STAT_Pos)
#define PHYTIMER_TMRM_INT_STAT_Pos		1
#define PHYTIMER_TMRM_INT_STAT_Msk		(1UL << PHYTIMER_TMRM_INT_STAT_Pos)
#define PHYTIMER_TMRH_INT_STAT_Pos		2
#define PHYTIMER_TMRH_INT_STAT_Msk		(1UL << PHYTIMER_TMRH_INT_STAT_Pos)
#define PHYTIMER_TRX0MATCH_INT_STAT_Pos	3
#define PHYTIMER_TRX0MATCH_INT_STAT_Msk	(1UL << PHYTIMER_TRX0MATCH_INT_STAT_Pos)
#define PHYTIMER_TRX1MATCH_INT_STAT_Pos	4
#define PHYTIMER_TRX1MATCH_INT_STAT_Msk	(1UL << PHYTIMER_TRX1MATCH_INT_STAT_Pos)
#define PHYTIMER_TRX2MATCH_INT_STAT_Pos	5
#define PHYTIMER_TRX2MATCH_INT_STAT_Msk	(1UL << PHYTIMER_TRX2MATCH_INT_STAT_Pos)
#define PHYTIMER_TRX3MATCH_INT_STAT_Pos	6
#define PHYTIMER_TRX3MATCH_INT_STAT_Msk	(1UL << PHYTIMER_TRX3MATCH_INT_STAT_Pos)

#define PHYTIMER_SFW1MATCH_INT_STAT_Pos	7
#define PHYTIMER_SFW1MATCH_INT_STAT_Msk	(1UL << PHYTIMER_SFW1MATCH_INT_STAT_Pos)
#define PHYTIMER_SFW2MATCH_INT_STAT_Pos	8
#define PHYTIMER_SFW2MATCH_INT_STAT_Msk	(1UL << PHYTIMER_SFW2MATCH_INT_STAT_Pos)

typedef struct
{
    FRC_TIME_t           callBackFRC;
    uint32_t             paramVal;
    void                 (*pCallBackFunc)(uint32_t paraVal);
    uint8_t              lastTrigger;
    uint8_t              activeTimerFlg;
}PHY_TIMER_CB_t;

enum
{
	TRX_CFGCNT0 = 0,
	TRX_CFGCNT1,
	TRX_CFGCNT2,
	TRX_CFGCNT3,
	SFW_CFGCNT1,
	SFW_CFGCNT2,	
};
enum
{
	TRX_DISABLE_MODE = 0,
	TRX_RX_RF_ON_MODE,
	TRX_RX_RF_OFF_MODE,
	TRX_RX_EN_ON_MODE,
	TRX_RX_EN_OFF_MODE,
	TRX_TX_FIFO_ON_MODE,
	TRX_TX_FIFO_OFF_MODE,	
	TRX_TX_BBGAIN_ON_MODE,
	TRX_RX_BBGAIN_ON_MODE,	
	TRX_TX_RF_ON_MODE,
	TRX_TX_RF_OFF_MODE,
};
extern PHY_TIMER_CB_t gPhyTimerCallback;
extern uint32_t    gHyper_HyperNumber;
extern FRC_TIME_t  gHyperUpdateFRC;

extern void PhyTimerOffsetTMRL(unsigned int offsetVal, int dir);
extern void PhyTimerOffsetTMRM(unsigned int offsetVal, int dir);
extern void PhyTimerOffsetTMRH(unsigned int offsetVal, int dir);

uint16_t PhyTimerIntStatGet(void);
void PhyTimerIntStatClear(uint16_t int_reg);
void PhyTimerIntEnable(uint16_t int_reg);
void PhyTimerEnable(void);
void PhyTimerDisable(void);

void PhyTimerInit(void);
void PhyTimerIsr(void);

void FRC_GetLocalFRC_Critical(uint32_t ulTimrh_cnt, uint32_t ulTimerml_cnt, FRC_TIME_t * LocalFRC);
uint32_t FRC_GetLocalFRC(FRC_TIME_t * LocalFRC);

void TRX_CfgCnt_Mode(uint8_t index, uint8_t mode);

void TRX_CfgCnt_Set(uint8_t index, FRC_TIME_t* dealFRC);

void SFW_CfgCnt_Set_Reg(uint8_t index, uint32_t ulCfg_Cnt);

void SFW_CfgCnt_Get(uint8_t index, uint32_t *HFN, uint32_t *SFN, uint32_t *subframe, uint32_t *countsubframe);

uint32_t SFW_CfgCnt_Get_Reg(uint8_t index);

void PhyTimerSFW1IntDisable();

char PhyTimerSFW1IntGet();

int PhyTimerSetCallback( void (*pCallBackFunc)(uint32_t paraId),uint32_t paraVal, FRC_TIME_t *targetFRC);

#endif
