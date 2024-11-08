#ifndef OSAL_CPU_UTILIZATION
#define OSAL_CPU_UTILIZATION

#include "xinyi_hardware.h"


void osUtilTimerEnable(unsigned long ulBase);
void osUtilTimerDisable(unsigned long ulBase);
void osUtilTimerInitValueSet(unsigned long ulBase, unsigned long ulValue);
void osUtilTimerReloadValueSet(unsigned long ulBase, unsigned long ulValue);

void osUtilTimerConfigure(unsigned long ulBase);
void osTimer_init(void);
void osThreadCPUUtilizationStart(void);
void osThreadCPUUtilizationPrint(void);
void osThreadCPUUtilizationStop(void);
uint32_t osThreadTimeBaseGetTime(void);


#undef portCONFIGURE_TIMER_FOR_RUN_TIME_STATS
#undef portGET_RUN_TIME_COUNTER_VALUE

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()            osThreadTimeBaseGetTime()


#endif  /* OSAL_CPU_UTILIZATION */
