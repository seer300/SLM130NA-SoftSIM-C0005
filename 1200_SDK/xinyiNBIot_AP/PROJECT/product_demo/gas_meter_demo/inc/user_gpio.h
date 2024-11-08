/*****************************************************************************************************************************	 
 * user_gpio.h
 ****************************************************************************************************************************/

#ifndef USER_GPIO_H__
#define USER_GPIO_H__

#include "basic_config.h"

#define USER_VALVE_EVENT                    EVENT_USER_DEFINE2
#define SWITCHING_VALVE_HOLD_TIME           (1400)   // 单位：毫秒  开阀门持续时间
#define VALVE_DEEPSLEEP_LOCK                USER_DSLEEP_LOCK2

#define IO_LOW                              (0)
#define IO_HIGH                             (1)


void UserValveFunc(void);
void UserValveStop(void);

void UnusedGpioInit(void);

#endif

