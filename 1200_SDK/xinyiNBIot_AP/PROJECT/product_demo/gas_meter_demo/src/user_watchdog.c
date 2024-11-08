/*****************************************************************************************************************************	 
 * user_watchdog.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "ap_watchdog.h"
#include "user_eeprom.h"
#include "type_adapt.h"

__TYPE_IRQ_FUNC static void UserWdtCallback(void)
{
    jk_printf(" Enter WDT\r\n");
    UserEepromTest();
}

void UserWatchdogInit(uint16_t sec)
{
    AP_WDT_Init(AP_WDT_WORKMODE_INT, sec);
    AP_WDT_Int_Reg(UserWdtCallback);
}

void UserWatchdogFeed(uint16_t sec)
{
    AP_WDT_Refresh(sec);
}