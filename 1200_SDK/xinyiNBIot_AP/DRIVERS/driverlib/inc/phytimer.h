#ifndef __PHYTIMER_H__
#define __PHYTIMER_H__

#include "hw_ints.h"
#include "xinyi2100.h"

#include "hw_types.h"
#include "interrupt.h"
#include "gpio.h"
#include "hw_phytimer.h"

typedef struct 
{
	unsigned int countInSubFrame : 16;
	unsigned int subframe		 :	4;
	unsigned int UNUSED			 : 12;
}FRC_TIME_REG0_t;

typedef struct 
{
	unsigned int SFN_Number : 10;
	unsigned int HFN_Number	: 10;
	unsigned int UNUSED		: 12;
}FRC_TIME_REG1_t;

typedef struct 
{
	FRC_TIME_REG0_t FRC_Reg0;
	FRC_TIME_REG1_t FRC_Reg1;
}FRC_TIME_t;

typedef struct
{
	FRC_TIME_t 	callBackFRC;
	uint32_t	paramVal;
	void		(*pCallBackFunc)(uint32_t paramVal);
	uint8_t		lastTrigger;
	uint8_t 	activeTimerFlg;
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
	TRX_RX_BBGAIN_OFF_MODE,	
	TRX_TX_RF_ON_MODE,
	TRX_TX_RF_OFF_MODE,
};

extern FRC_TIME_t FRC_TIME_ZERO;

extern void PhyTimerOffsetTMRL(unsigned int offsetVal, int dir);
extern void PhyTimerOffsetTMRM(unsigned int offsetVal, int dir);
extern void PhyTimerOffsetTMRH(unsigned int offsetVal, int dir);

extern uint16_t PhyTimerIntStatGet(void);
extern void PhyTimerIntStatClear(uint16_t int_reg);
extern void PhyTimerIntEnable(uint16_t int_reg);
extern void PhyTimerEnable(void);
extern void PhyTimerDisable(void);

extern unsigned long PhyTimerIntCountGet(uint8_t IntType);
extern void PhyTimerIntCountClear(void);

extern void PhyTimerInit(void);
extern void PhyTimerIsr(void);

extern uint32_t FRC_GetLocalFRC(FRC_TIME_t * LocalFRC);

void TRX_CfgCnt_Mode(uint8_t index, uint8_t mode);
void TRX_CfgCnt_Set(uint8_t index, FRC_TIME_t* dealFRC);
#endif
