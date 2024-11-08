#include "hal_lpuart.h"
#include "hal_gpio.h"
#include "user_gpio.h"
#include "user_config.h"
#include "at_uart.h"

uint16_t Key_count = 0;
uint8_t g_hw_init = 0;
__RAM_FUNC void Scan_Key(void)
{
    uint8_t cur = 0;
    uint16_t Key_Value = 0;
    static uint8_t last = 0, saved = 0;
    static uint8_t tPush = 0;

    if(HAL_GPIO_ReadPin(USER_CHK_MD)==0)
    {
        cur = 1;              //按下键
    }

    if (cur != last)
    {
        last = cur;
        tPush = 0;
    }
    else
    { 
        tPush ++; 
    }

    if ((tPush >= 1) && (last != saved)) 
    {
        if (last)
        {
            Key_Value = KEY_VAL_SET;
        }
        saved = last;
    }

    if (Key_Value == KEY_VAL_SET)
    {
        Key_count = 0;
        set_event(EVENT_CTL);
        // Trigger_Send_Proc();
    }
}
#if AT_LPUART
void User_Lpuart_Deinit(void)
{
    HAL_LPUART_DeInit(&g_at_lpuart);
}
#endif
__RAM_FUNC void HW_Init(void)
{
    HWON();//打开红外
    // at_uart_init();
}

__RAM_FUNC void Key_Func(void)
{
    if (!is_event_set(EVENT_CTL)) //干簧管检测
    {
        Scan_Key();
    }
    else
    {
        if(g_hw_init == 0)
        {
            HW_Init();
            g_hw_init = 1; 
        }
    }    
}

__RAM_FUNC void Key_Check(void)
{
    if (is_event_set(EVENT_CTL))         //关闭红外计时
    {
        HAL_GPIO_TogglePin(USER_LEDGRE);  //指示灯

        Key_count++;

        if (Key_count >= KEYTIME)
        {
            Key_count = 0;
            clear_event(EVENT_CTL);               
            HWOFF();//关闭红外
            g_hw_init = 0;
            //User_Lpuart_Deinit();//关闭lpuart
        }
    }
}