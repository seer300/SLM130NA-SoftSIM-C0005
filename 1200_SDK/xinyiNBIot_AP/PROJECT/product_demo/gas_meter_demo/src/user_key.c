/*****************************************************************************************************************************	 
 * user_key.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_key.h"
#include "user_gpio.h"
#include "vmcu.h"
#include "user_lcd.h"
#include "user_uart.h"
#include "user_timer.h"

volatile uint32_t g_key_press_tick = 0;
volatile uint32_t g_key_falling_tick = 0;
volatile uint32_t g_key_rising_tick = 0;

__RAM_FUNC void Key_Sent_Mark(void)
{
    if(VmcuGpioRead(KEY_PIN) == IO_LOW)//下降沿先到
    {
        g_key_falling_tick = Get_Tick();
    }
    else
    {
        g_key_rising_tick = Get_Tick();
        set_event(USER_KEY_EVENT);
    }
}

//wkp1初始化函数
void UserKeyInit(void)
{
    //初始化为双边沿触发模式
    VmcuGpioModeSet(KEY_PIN, Mode_In_DI);  //KEY--Mode_In_DI
    VmcuGpioIrqReg(KEY_PIN,(pFunIRQ)Key_Sent_Mark,MCU_GPIO_INT_BOTH);
    VmcuGpioIrqEn(KEY_PIN);
}

bool Is_Key_Func_Abnormal(void)
{
    if((Check_Ms_Timeout(g_key_falling_tick,KEY_PRESS_UPTIME)) && (g_key_rising_tick == 0))
    {
        UserKeyInit();
        g_key_falling_tick = 0;
        g_key_rising_tick = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void UserKeyFunc(void)
{
    uint32_t key_press_time  = 0;
    uint8_t key_mode = 0;

    if(is_event_set(USER_KEY_EVENT))
	{
        clear_event(USER_KEY_EVENT);
        if(Is_Key_Func_Abnormal())
        {
            return;
        }
        else
        {
            if(g_key_rising_tick == 0)
            {
                return;
            }

            key_press_time = (g_key_rising_tick - g_key_falling_tick);

            g_key_rising_tick = 0;
            g_key_falling_tick = 0;

            if(key_press_time > 10 && key_press_time < 1001)
            {
                key_mode = MODE_SHORT;
            }
            else if(key_press_time > 1000 && key_press_time < 3001)
            {
                key_mode = MODE_LONG;
            }
            else if(key_press_time > 3000 && key_press_time < 15001)
            {
                key_mode = MODE_LONGLONG;
            }
            else if(key_press_time > 15000)
            {
                key_mode = MODE_DEFAULT;
            }
            else
            {
                UserKeyInit();
                return;
            }

            switch(key_mode)
            {
                case MODE_SHORT:
                {
                    UserLcdMenuSwitch();
                    break;
                }

                case MODE_LONG:
                {
                    VmcuNbShutdown(0u);
                    UserTestUartInit();
                    jk_printf("start uart2\r\n");
                    break;
                }

                case MODE_LONGLONG:
                {
                    UserTestUartDis();

                    jk_printf("set cloud event\r\n");
                    set_event(EVENT_CLOUD_SEND); 
                    break;
                }

                case MODE_DEFAULT:
                {
                    UserKeyInit();
                    break;
                }

                default:
                {
                    xy_assert(0);
                }

                break;
            }
        }
	}
}


