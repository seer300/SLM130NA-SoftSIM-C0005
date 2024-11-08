#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"


__RAM_FUNC void wkp1_callback(void) //上升沿中断
{
    if(McuGpioRead(MCU_WKP1) == 1)
    {
        McuGpioToggle(MCU_GPIO20);
    }
}

__RAM_FUNC void wkp2_callback(void) //下降沿中断
{
    if(McuGpioRead(MCU_WKP2) == 0)
    {
        McuGpioToggle(MCU_GPIO21);
    }
}

__RAM_FUNC void wkp3_callback(void) //双边沿
{
    if(McuGpioRead(MCU_WKP3) == 1)
    {
        McuGpioWrite(MCU_GPIO22, 1);
    }
    else
    {
        McuGpioWrite(MCU_GPIO22, 0);
    }
}

__RAM_FUNC void gpio0_callback(void) //上升沿中断
{
    if(McuGpioRead(MCU_GPIO0) == 1)
    {
        McuGpioToggle(MCU_GPIO23);
    }
}

__RAM_FUNC void gpio1_callback(void) //下降沿中断
{
    if(McuGpioRead(MCU_GPIO1) == 0)
    {
        McuGpioToggle(MCU_GPIO24);
    }
}
 
__RAM_FUNC void gpio2_callback(void) //双边沿
{
    if(McuGpioRead(MCU_GPIO2) == 1)
    {
        McuGpioWrite(MCU_GPIO25, 1);
    }
    else
    {
        McuGpioWrite(MCU_GPIO25, 0);
    }
}

__RAM_FUNC void gpio9_callback(void) //上升沿中断
{
    if(McuGpioRead(MCU_GPIO9) == 1)
    {
        McuGpioToggle(MCU_GPIO53);
    }
}

__RAM_FUNC void gpio26_callback(void) //下降沿中断
{
    if(McuGpioRead(MCU_GPIO26) == 0)
    {
        McuGpioToggle(MCU_GPIO52);
    }
}

__RAM_FUNC void gpio54_callback(void) //双边沿
{
    if(McuGpioRead(MCU_GPIO54) == 1)
    {
        McuGpioWrite(MCU_GPIO5, 1);
    }
    else
    {
        McuGpioWrite(MCU_GPIO5, 0);
    }
}

__RAM_FUNC int main(void)
{
    SystemInit();

    //WKP1-3(AGPIO0-2)中断输入
    McuGpioModeSet(MCU_WKP1, 0x11);
    McuGpioIrqReg(MCU_WKP1, wkp1_callback, MCU_GPIO_INT_RISING);
    McuGpioIrqEn(MCU_WKP1);
    McuGpioModeSet(MCU_WKP2, 0x11);
    McuGpioIrqReg(MCU_WKP2, wkp2_callback, MCU_GPIO_INT_FALLING);
    McuGpioIrqEn(MCU_WKP2);
    McuGpioModeSet(MCU_WKP3, 0x11);
    McuGpioIrqReg(MCU_WKP3, wkp3_callback, MCU_GPIO_INT_BOTH);
    McuGpioIrqEn(MCU_WKP3);

    //推挽输出，默认低电平
    McuGpioModeSet(MCU_GPIO20, 0x00), McuGpioWrite(MCU_GPIO20, 0);
    McuGpioModeSet(MCU_GPIO21, 0x00), McuGpioWrite(MCU_GPIO21, 0);
    McuGpioModeSet(MCU_GPIO22, 0x00), McuGpioWrite(MCU_GPIO22, 0);

    //GPI0~2中断输入
    McuGpioModeSet(MCU_GPIO0, 0x11);
    McuGpioIrqReg(MCU_GPIO0, gpio0_callback, MCU_GPIO_INT_RISING);
    McuGpioIrqEn(MCU_GPIO0);
    McuGpioModeSet(MCU_GPIO1, 0x11);
    McuGpioIrqReg(MCU_GPIO1, gpio1_callback, MCU_GPIO_INT_FALLING);
    McuGpioIrqEn(MCU_GPIO1);
    McuGpioModeSet(MCU_GPIO2, 0x11);
    McuGpioIrqReg(MCU_GPIO2, gpio2_callback, MCU_GPIO_INT_BOTH);
    McuGpioIrqEn(MCU_GPIO2);

    //推挽输出，默认低电平
    McuGpioModeSet(MCU_GPIO23, 0x00), McuGpioWrite(MCU_GPIO23, 0);
    McuGpioModeSet(MCU_GPIO24, 0x00), McuGpioWrite(MCU_GPIO24, 0);
    McuGpioModeSet(MCU_GPIO25, 0x00), McuGpioWrite(MCU_GPIO25, 0);

    //GPIO9、26、54中断输入
    McuGpioModeSet(MCU_GPIO9, 0x11);
    McuGpioIrqReg(MCU_GPIO9, gpio9_callback, MCU_GPIO_INT_RISING);
    McuGpioIrqEn(MCU_GPIO9);
    McuGpioModeSet(MCU_GPIO26, 0x11);
    McuGpioIrqReg(MCU_GPIO26, gpio26_callback, MCU_GPIO_INT_FALLING);
    McuGpioIrqEn(MCU_GPIO26);
    McuGpioModeSet(MCU_GPIO54, 0x11);
    McuGpioIrqReg(MCU_GPIO54, gpio54_callback, MCU_GPIO_INT_BOTH);
    McuGpioIrqEn(MCU_GPIO54);

    //推挽输出，默认低电平
    McuGpioModeSet(MCU_GPIO52, 0x00), McuGpioWrite(MCU_GPIO52, 0);
    McuGpioModeSet(MCU_GPIO53, 0x00), McuGpioWrite(MCU_GPIO53, 0);
    McuGpioModeSet(MCU_GPIO5,  0x00), McuGpioWrite(MCU_GPIO5,  0);

    while (1)
    {
        ;
    }
}

