/*****************************************************************************************************************************	 
 * user_key.h
 ****************************************************************************************************************************/

#ifndef USER_KEY_H__
#define USER_KEY_H__

#define KEY_PIN                         (201)
#define KEY_PRESS_UPTIME                (30*1000)
#define USER_KEY_EVENT         			EVENT_USER_DEFINE1

typedef enum
{
    KEY_PRESS_MODE_BASE = 0,
    MODE_SHORT = KEY_PRESS_MODE_BASE,
    MODE_LONG,
    MODE_LONGLONG,
    MODE_DEFAULT,
    KEY_PRESS_MODE_END

} KEY_TYPE;

void UserKeyInit(void);
void UserKeyFunc(void);

#endif

