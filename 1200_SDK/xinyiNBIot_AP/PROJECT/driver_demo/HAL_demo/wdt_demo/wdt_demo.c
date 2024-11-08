/**
* @file		wdt_demo.c
* @brief    demo验证看门狗中断模式和重启模式   
***********************************************************************************/
#include "hal_def.h"
#include "ap_watchdog.h"

//看门狗中断模式定时时长，单位秒
#define WDT_INT_TIME    (5)
//看门狗重启模式定时时长，单位秒
#define WDT_RESET_TIME  (5)

//times:表示进看门狗中断的次数
volatile uint32_t times = 0, print_flag = 0;

/**
 * @brief 看门狗中断回调函数，计时到达设置的时间时触发.
 */
__RAM_FUNC void AP_WDT_Int_Cbk()
{
    times++;
    print_flag = 1;
}

__RAM_FUNC int main(void)
{
    SystemInit();

	xy_printf("wdt demo start\r\n");

	AP_WDT_Int_Reg(AP_WDT_Int_Cbk);

    //目前是看门狗中断模式，若想执行看门狗重启模式，将AP_WDT_WORKMODE_INT改成AP_WDT_WORKMODE_RESET，此时需要将XY_DEBUG关闭。
    AP_WDT_Init(AP_WDT_WORKMODE_INT, WDT_INT_TIME);

    while(1)
    {
        xy_printf("delay 1s\r\n");
        HAL_Delay(1000);
        if(print_flag)
        {
            //喂狗操作
            AP_WDT_Refresh(WDT_INT_TIME);
            print_flag = 0;
            xy_printf("Enter WTD Interrupt %d times\r\n", times);
           
        }
    }

    return 0;
}
