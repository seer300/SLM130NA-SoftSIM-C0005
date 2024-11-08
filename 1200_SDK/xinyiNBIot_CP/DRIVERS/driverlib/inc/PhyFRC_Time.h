#ifndef _FRC_TIME_H_
#define _FRC_TIME_H_

#include"PhyBasicType.h"
typedef struct
{
    uint32_t countInSubFrame : 16; /*0-1919*/
    uint32_t subframe        : 4; /*0-1023*/
    uint32_t UNUSED          : 12; /*unused*/
}FRC_TIME_REG0_t;


typedef struct
{
    uint32_t SFN_Number      : 10; /*0-1023*/
    uint32_t HFN_Number      : 10; /*upper frame*/
    uint32_t UNUSED          : 12; 
}FRC_TIME_REG1_t;

typedef struct
{
    FRC_TIME_REG0_t FRC_Reg0;
    FRC_TIME_REG1_t FRC_Reg1;
}FRC_TIME_t;


#define SYMBOLS_PER_1MS       (1920)
#define VAL_1024M1            (1023)
#define SYMBOLS_PER_FRAME     (19200)
#define SUB_FRMAE_PER_FRAME   (10)
#define SUB_FRMAE_PER_FRAMEM1 (9)

extern const FRC_TIME_t FRC_TIME_ZERO;
extern const FRC_TIME_t FRC_TIME_ONE_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_INVALID;
extern const FRC_TIME_t FRC_TIME_ONE_FRAME;
extern const FRC_TIME_t FRC_TIME_FIVE_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_SIX_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_FOUR_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_NINE_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_FOUR_FRAMES;
extern const FRC_TIME_t FRC_TIME_THRESS_FRAMES;
extern const FRC_TIME_t FRC_TIME_THREE_SUBFRAME;
extern const FRC_TIME_t FRC_TIME_TWO_SUBFRAME;
//extern const FRC_TIME_t FRC_TIME_TWO_FRAME;
extern const FRC_TIME_t FRC_TIME_TEN_FRAME;
extern const FRC_TIME_t FRC_TIME_0_P_3MS;
extern const FRC_TIME_t FRC_TIME_0_P_5MS;
extern const FRC_TIME_t FRC_TIME_0_P_6MS;
extern const FRC_TIME_t FRC_TIME_1_P_5MS;
extern const FRC_TIME_t FRC_TIME_MAX;
extern const FRC_TIME_t FRC_TIME_RFF_OFF_MARGIN;
extern const FRC_TIME_t FRC_TIME_HALF_HSFN;

#define FRC_EQU(A,B)       (*((int64_t *)(&(A))) == *((int64_t *)(&(B))))

#define FRC_LESS_THAN(A,B) (*((int64_t *)(&(A))) < *((int64_t *)(&(B))))

extern uint32_t  gRetExtendHyperNumber;

extern FRC_TIME_t FRC_GetCurFRC( void );
extern void FRC_UpdateSoftFRC_1ms( void );
//extern void FRC_Init( void );
extern void FRC_Sleep( void );
extern FRC_TIME_t FRC_DecFRC(FRC_TIME_t srcFRC,FRC_TIME_t deFRC);
extern FRC_TIME_t FRC_AddFRC(FRC_TIME_t srcFRC,FRC_TIME_t addFRC);
extern FRC_TIME_t FRC_GetFRC_Diff(FRC_TIME_t targetFrc,FRC_TIME_t curFrc, FRC_TIME_t marginFRC,uint16_t SFN_Module,uint16_t HFN_Module);
FRC_TIME_t FRC_GetSubframeToFRC(int64_t slSubframe);
FRC_TIME_t FRC_GetPhyCurNetWorkFRC(FRC_TIME_t stTimeSfo);
int64_t nl1cGetPhyCurSubframeIdxByFrcTime(FRC_TIME_t stFrcTime, FRC_TIME_t stCurFrcTime);
int64_t nl1cGetPhyCurSubframeIdx(FRC_TIME_t stTimeSfo, FRC_TIME_t *stpCurFrcTime);
int64_t FRC_GetFRCeToSubframe(FRC_TIME_t curFrc);
int64_t nl1cGetPhyCurSubfAndFrcTime(FRC_TIME_t stTimeSfo, FRC_TIME_t *stpCurFrcTime, FRC_TIME_t *stpLocalFrcTime);


void PhyHalGotoSleepHandler(void);
//void PhyHalWakeupSleepHandler(void);


#endif /*  */
