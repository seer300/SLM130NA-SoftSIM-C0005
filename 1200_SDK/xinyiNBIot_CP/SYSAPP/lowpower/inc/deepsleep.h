#ifndef __DEEPSLEEP_H__
#define __DEEPSLEEP_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif
//#include "PhyDummy.h"

enum DEEPSLEEP_WAKEUP_SOUCRE_Def
{
	DEEPSLEEP_WAKEUP_UNDEFINE = 0,
	DEEPSLEEP_WAKEUP_BY_UTC,
	DEEPSLEEP_WAKEUP_BY_XYTICK,
};

int DeepSleep_Admittance_Check();
void DeepSleep_Recover();
int DeepSleep_Process(void);
void DeepSleep_Entry();
int DeepSleep_Power_Manage();
int DeepSleep_WakeUp_Config();
uint64_t DeepSleep_Cal_SleepTime_Again();
void DeepSleep_Context_Save(void);

#ifdef __cplusplus
}
#endif

#endif //  __LOW_POWER_H__
