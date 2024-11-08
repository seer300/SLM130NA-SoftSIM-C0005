#ifndef SYSTICK_TIME_H
#define SYSTICK_TIME_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "xinyi_hardware.h"


__STATIC_INLINE void systick_time_reset(void)
{
	SysTick->LOAD = 0xFFFFFF;
	SysTick->VAL = 0xFFFFFF;
	SysTick->CTRL = 0x5;
}

__STATIC_INLINE uint32_t systick_time_get_count(void)
{
	uint32_t value = 0xFFFFFF - SysTick->VAL;

	SysTick->CTRL = 0x0;

	return value;
}


#ifdef __cplusplus
 }
#endif

#endif  /* SYSTICK_TIME_H */
