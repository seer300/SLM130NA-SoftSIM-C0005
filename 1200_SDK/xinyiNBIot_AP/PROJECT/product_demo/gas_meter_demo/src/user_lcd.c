/*****************************************************************************************************************************	 
 * user_lcd.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_lcd.h"
#include "vmcu.h"
#include "xy_event.h"
#include "type_adapt.h"

static uint8_t s_display_menu = USER_LCD_MENU1;
static sDateYMDHMS s_rtc_time={0};

/**
  * @brief  Hex transform to ascii string
  * @note   16进制数转换为字符串
  * @param  *src  : 数组指针
  * @param  *dest : 字符串指针
  * @param  len   : 字符串长度  strlen(c1)不包括结束字符（即 null 字符）
  * @retval void
  */
static void HexToAscii(uint8_t *src, char *dest, int len)
{
	char dh1, dl2;  //字符串的高位和低位
	int i;
	for (i = 0; i < len; i++)
	{
		dh1 = '0' + src[i] / 16;        //十位 
		dl2 = '0' + src[i] % 16;		//个位以0为基准 dh='0'+AB/16=48+10=58 
		dest[2 * i] = dh1;
		dest[2 * i + 1] = dl2;
	}
	dest[2 * i] = '\0';   //字符串结尾
}

//添加连接符'-'
static void AppendSymbol(char *dest)
{
    dest[7] = dest[5];
    dest[6] = dest[4];
    dest[5] = '-';
    dest[4] = dest[3];
    dest[3] = dest[2];
    dest[2] = '-';
}

__TYPE_IRQ_FUNC void UserGetTime(void)
{
    VmcuRtcRead(&s_rtc_time);
}

static void UserShowTime(void)
{
    uint8_t lcd_time[3] = {0};
    char lcd_str[10] = {0};

    lcd_time[0]=s_rtc_time.hour;
    lcd_time[1]=s_rtc_time.min;
    lcd_time[2]=s_rtc_time.sec;

    HexToAscii(lcd_time,lcd_str,3);
    AppendSymbol(lcd_str);

    VmcuLcdWriteStr(lcd_str);
    VmcuLcdEn();
}

void UserLcdMenuSwitch(void)
{
    s_display_menu++;
    s_display_menu %= 4;
}

void UserLcdShowSend(void)
{
    VmcuLcdWriteStr("--SEnd--");
    VmcuLcdEn();
    s_display_menu = USER_LCD_MENU5;
}

void UserLcdFunc(void)
{
    //每次按键切换LCD的显示
    switch (s_display_menu)
    {
        case USER_LCD_MENU1:
            VmcuLcdWriteStr("11111111");
            VmcuLcdEn();
            break;
        case USER_LCD_MENU2:
            VmcuLcdWriteStr("22222222");
            VmcuLcdEn();
            break;
        case USER_LCD_MENU3://熄屏
            VmcuLcdDis();
            break;
        case USER_LCD_MENU4://显示时间
            UserShowTime();
            break;
        default:
            break;
    }
}