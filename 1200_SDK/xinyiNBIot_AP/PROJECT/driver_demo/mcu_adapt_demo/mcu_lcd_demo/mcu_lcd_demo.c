#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
#include "UWater_lcd_driver.h"


volatile uint32_t lptimer_cnt = 0;
volatile uint8_t lock = 0;

static char char_str[5][8] =
{
    "ACEFGHJL",
    "NOPSU   ",
    "bcdghiln",
    "oqruy   ",
    "  -_~   "
};

__RAM_FUNC void lptimer_callback(void)
{
    char *str = NULL;
    uint8_t number_str[8] = {0};

    if(lock == 0)
    {
        if(lptimer_cnt < 5)
        {
            str = char_str[lptimer_cnt];
        }
        else
        {
            lock = 1;
            lptimer_cnt = 0;
            McuLcdDecToAscii(lptimer_cnt, number_str);
            str = (char *)number_str;
        }
    }
    else
    {
        McuLcdDecToAscii(lptimer_cnt, number_str);
        str = (char *)number_str;
    }
    lptimer_cnt++;

    McuLcdWriteStr(str);
    McuLcdUpdate();
}

__RAM_FUNC int main(void)
{
    uint32_t lcd_segpad;
    uint32_t lcd_comPad;

    SystemInit();
    EnablePrimask();

    McuLptimerSetPeriod(3000);
    McuLptimerIrqReg(lptimer_callback);
    McuLptimerEn(); 

    lcd_segpad = (uint32_t)((1u<<JC_LCD_REG_SEG1)|(1u<<JC_LCD_REG_SEG2)|(1u<<JC_LCD_REG_SEG3)|(1u<<JC_LCD_REG_SEG4)|(1u<<JC_LCD_REG_SEG5)|\
                            (1u<<JC_LCD_REG_SEG7)|(1u<<JC_LCD_REG_SEG8)|(1u<<JC_LCD_REG_SEG9)|(1u<<JC_LCD_REG_SEG10)|\
                            (1u<<JC_LCD_REG_SEG11)|(1u<<JC_LCD_REG_SEG12)|(1u<<JC_LCD_REG_SEG13)|(1u<<JC_LCD_REG_SEG14)|(1u<<JC_LCD_REG_SEG15)|\
                            (1u<<JC_LCD_REG_SEG16)|(1u<<JC_LCD_REG_SEG17)|(1u<<JC_LCD_REG_SEG18)|(1u<<JC_LCD_REG_SEG19)|(1u<<JC_LCD_REG_SEG20)|\
                            (1u<<JC_LCD_REG_SEG21));
    lcd_comPad = (uint32_t)((1<<JC_LCD_REG_COM1)|(1<<JC_LCD_REG_COM2)|(1<<JC_LCD_REG_COM3)|(1<<JC_LCD_REG_COM4)|\
                            (1<<JC_LCD_REG_SEG6)|(1<<JC_LCD_REG_SEG22)|(1<<JC_LCD_REG_SEG23));

    //McuLcdInit(0xFF, 0x0E1F8000, 140, IOEQ3V_VLCDGT3V); //JK
    // McuLcdInit(0xFF, 0xE16420B, 35, IOEQ3V_VLCDGT3V); //WX
    McuLcdInit(lcd_comPad, lcd_segpad, 140, IOEQ3V_VLCDGT3V); //JC
    McuLcdEn();
    McuLcdWriteStr("--HELLO-");
    McuLcdUpdate();

    while (1)
    {
        ;
    }
}

