#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"


__RAM_FUNC void lptimer1_callback(void)
{
    McuGpioToggle(MCU_GPIO20);
}

__RAM_FUNC void lptimer2_callback(void)
{
    McuGpioToggle(MCU_GPIO21);
}

__RAM_FUNC void timer1_callback(void)
{
    McuGpioToggle(MCU_GPIO22);
}

__RAM_FUNC int main(void)
{
    SystemInit();

    //推挽输出，默认低电平
    McuGpioModeSet(MCU_GPIO20, 0x00), McuGpioWrite(MCU_GPIO20, 0);
    McuGpioModeSet(MCU_GPIO21, 0x00), McuGpioWrite(MCU_GPIO21, 0);
    McuGpioModeSet(MCU_GPIO22, 0x00), McuGpioWrite(MCU_GPIO22, 0);

    //lptimer1周期定时1ms
    McuLptimerSetPeriod(5);
    McuLptimerIrqReg(lptimer1_callback);
    McuLptimerEn();

    //lptimer2周期定时10ms
    McuLptimer2SetPeriod(10);
    McuLptimer2IrqReg(lptimer2_callback);
    McuLptimer2En();

    //timer1周期定时1ms
    McuTimerSetPeriod(1, 1);
    McuTimerIrqReg(1, timer1_callback);
    McuTimerEn(1);

    //timer2周期输出频率为50k、占空比为50%的PWM
    McuTimerSetPWM2(2, 50, 50, MCU_GPIO53);
    McuTimerEn(2);

    while (1)
    {
		;
    }
}

