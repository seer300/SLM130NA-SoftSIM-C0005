/*****************************************************************************************************************************	 
 * user_watchdog.h
 ****************************************************************************************************************************/

#ifndef USER_WATCHDOG_H__
#define USER_WATCHDOG_H__

extern void UserWatchdogInit(uint16_t sec);
extern void UserWatchdogFeed(uint16_t sec);

#endif

