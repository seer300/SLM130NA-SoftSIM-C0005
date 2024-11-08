#ifndef __STANDBY_H__
#define __STANDBY_H__

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

#if STANDBY_SUPPORT

void StandBy_Recover();

int StandBy_Process( void );

void StandBy_Entry();

uint32_t StandBy_Admittance_Check();

uint32_t  StandBy_Power_Manage();

uint32_t  StandBy_WakeUp_Config(uint64_t sleep_ms );

uint64_t StandBy_Cal_SleepTime_Again();

uint32_t  StandBy_Context_Save(void);

#endif

#ifdef __cplusplus
}
#endif

#endif //  __LOW_POWER_H__
