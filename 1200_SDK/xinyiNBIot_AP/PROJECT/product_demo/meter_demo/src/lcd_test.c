#include "user_config.h"
#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
#include "string.h"

/**
 * @brief LCD工作时长的宏定义、全局定义
 *        LCD工作时长：基数 x 最大次数 = 5000ms
 */
#define LCD_WROK_DURATION_BASE  (1000) //LCD工作时长基数，单位ms
#define LCD_WORK_DURATION_TIMES (5)    //LCD工作时长最大次数，单位次

static volatile uint32_t lcd_work_cnt = 0; //记录LCD工作时长次数
static volatile bool lcd_work_done = 0;    //指示LCD工作是否完成，

/**
 * @brief LCD显示数字的宏定义、全局定义、字符串定义
 * 
 */
#define LCD_DISPLAY_MAX_NUM   (99999999) //LCD显示数字最大值
#define LCD_DISPLAY_START_NUM (10000000) //LCD显示数字起始值

static volatile uint32_t lcd_display_num = LCD_DISPLAY_START_NUM; //指示LCD显示数字

//8个数组元素对应LCD屏幕的8个字符，对应关系：str[0]-->正视角度下LCD屏幕最左边，……，str[7]-->正视角度下LCD屏幕最右边
static uint8_t lcd_display_num_str[8] = {0}; //存放LCD显示数字对应ASCII码的字符串

//=====================================================================================
//====================================LCD_TEST=========================================
//=====================================================================================
__RAM_FUNC void lptimer_callback(void)
{
    //LCD显示数字管理
    if(lcd_display_num == LCD_DISPLAY_MAX_NUM)
    {
        lcd_display_num = LCD_DISPLAY_START_NUM;
    }
    else
    {
        lcd_display_num++;
    }

    McuLcdDecToAscii(lcd_display_num, lcd_display_num_str);
    McuLcdWriteStr((char *)lcd_display_num_str);
    McuLcdUpdate();

    //LCD工作时长次数管理
    if(lcd_work_cnt == LCD_WORK_DURATION_TIMES)
    {
        McuTimerDis(1);
        lcd_work_cnt = 0;
        lcd_work_done = 1;
    }
    else
    {
        lcd_work_cnt++;
    }
}

void Lcd_Work(void)
{
    //lptimer周期定时LCD_WROK_DURATION_BASE
    McuLptimerSetPeriod(LCD_WROK_DURATION_BASE);
    McuLptimerIrqReg(lptimer_callback);
    McuLptimerEn();

    if(lcd_display_num == LCD_DISPLAY_START_NUM)
    {
        McuLcdInit(0xFF, 0x0E1F8000, 140, IOEQ3V_VLCDGT3V);
        McuLcdEn();

        McuLcdDecToAscii(lcd_display_num, lcd_display_num_str);
        McuLcdWriteStr((char *)lcd_display_num_str);
        McuLcdUpdate();
    }

    McuTimerEn(1);

    while(!lcd_work_done)
    {
        McuLcdWriteStr("HAPPiNES");
        McuLcdUpdate();

        HAL_Delay(500);
    }
    lcd_work_done = 0;
}