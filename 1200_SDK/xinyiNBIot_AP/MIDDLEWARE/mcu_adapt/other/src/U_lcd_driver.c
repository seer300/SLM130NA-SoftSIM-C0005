/**
  ******************************************************************************
  * @file    U_lcd_driver.c
  * @author  (C)2015, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2016-5-4
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */
#include "U_lcd_driver.h"

static u8 g_char_data[JC_LCD_CHAR_NUM];
static u32 g_fun_value;

static uint8_t lcd_on_off=0;

/* Function Declare------------------------------------------------------------*/
/* Functions -------------------------------------------------------*/
/*************************************************
Function: void LcdInit(void)
Description: Lcd初始化
Input：无
Return:无
Others:处于MainSpace，第一类接口：上电初始化
*************************************************/
__RAM_FUNC void LcdInit(void)
{
    uint32_t lcd_segpad;
    uint32_t lcd_comPad;

    lcd_segpad = (uint32_t)((1u<<JC_LCD_REG_SEG1)|(1u<<JC_LCD_REG_SEG2)|(1u<<JC_LCD_REG_SEG3)|(1u<<JC_LCD_REG_SEG4)|(1u<<JC_LCD_REG_SEG5)|\
                            (1u<<JC_LCD_REG_SEG7)|(1u<<JC_LCD_REG_SEG8)|(1u<<JC_LCD_REG_SEG9)|(1u<<JC_LCD_REG_SEG10)|\
                            (1u<<JC_LCD_REG_SEG11)|(1u<<JC_LCD_REG_SEG12)|(1u<<JC_LCD_REG_SEG13)|(1u<<JC_LCD_REG_SEG14)|(1u<<JC_LCD_REG_SEG15)|\
                            (1u<<JC_LCD_REG_SEG16)|(1u<<JC_LCD_REG_SEG17)|(1u<<JC_LCD_REG_SEG18)|(1u<<JC_LCD_REG_SEG19)|(1u<<JC_LCD_REG_SEG20)|\
                            (1u<<JC_LCD_REG_SEG21));
    lcd_comPad = (1<<JC_LCD_REG_COM1)|(1<<JC_LCD_REG_COM2)|(1<<JC_LCD_REG_COM3)|(1<<JC_LCD_REG_COM4)|\
                            (1<<JC_LCD_REG_SEG6)|(1<<JC_LCD_REG_SEG22)|(1<<JC_LCD_REG_SEG23);

    McuLcdInit(lcd_comPad, lcd_segpad, 140, IOEQ3V_VLCDGT3V); //JC
}

/*************************************************
Function: void LcdMachineDriver(void)
Description:LCD驱动任务机
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdMachineDriver(void)
{

}

 /*************************************************
Function: void LcdClearAll(void)
Description:LCD清屏
Input:无
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdClearAll(void)
{
    McuLcdClear();
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayAll(void)
Description:LCD全显
Input:无
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayAll(void)
{
    McuLcdWriteStr("8888888888");
    McuLcdWriteStatus(0xFFFFFFFF);
    McuLcdUpdate();
}

 /*************************************************
Function: u8 LcdGetDisplayData(u8 position)
Description:获取LCD当前显示的数字
Input:u8 position：获取数码管显示数字的位置
Return:当前显示的数字
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayData(u8 position, u8 data)
{
    if (position >= JC_LCD_CHAR_NUM) return;
    if (0 == McuLcdWriteNum(2*position, data))
    {
        g_char_data[position] = data;
    }
    McuLcdUpdate();
}

/*************************************************
Function: void LcdDisplayFun(u32 fun)
Description:lcd显示固化字符
Input:u32 fun：按位定义的固化字符
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/ 
__RAM_FUNC void LcdDisplayFun(u32 fun)
{
    g_fun_value = fun;
    McuLcdWriteStatus(fun);
    McuLcdUpdate();
}

 /*************************************************
Function: u8 LcdGetDisplayData(u8 position)
Description:获取LCD当前显示的数字
Input:u8 position：获取数码管显示数字的位置
Return:当前显示的数字
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC u8 LcdGetDisplayData(u8 position)
{
    if (position >= JC_LCD_CHAR_NUM) return;
    return g_char_data[position];
}

 /*************************************************
Function: u32 LcdGetDisplayFun(void)
Description:获取lcd当前显示的固化字符
Input:无
Return:当前显示的字符
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC u32 LcdGetDisplayFun(void)
{
    return g_fun_value;
}

 /*************************************************
Function: void LcdDisplayValve(u8 flag)
Description:lcd显示阀门状态
Input:u8 flag：1关阀；0开阀 ；其它默认不显示
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayValve(u8 flag)
{
    McuLcdWriteOneStatus(FUN_VALVE_CLOSE, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayVol(u8 flag)
Description:lcd显示换电池
Input:u8 flag：1欠压；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayVol(u8 flag)
{
    McuLcdWriteOneStatus(FUN_BATTERY_LOW, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayAlarm(u8 flag)
Description:lcd显示报警
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayAlarm(u8 flag)
{
    McuLcdWriteOneStatus(FUN_ALARM, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayActiveGPRS(u8 flag)
Description:lcd显示正在GPRS通信
Input:u8 flag：1正在通信；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayActiveGPRS(u8 flag)
{
    McuLcdWriteOneStatus(FUN_GPRSING, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayPleasePay(u8 flag)
Description:lcd显示请缴费
Input:u8 flag：1请缴费；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayPleasePay(u8 flag)
{
    McuLcdWriteOneStatus(FUN_JF, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayEnable(void)
Description:使能LCD显示功能
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayEnable(void)
{
    DisablePrimask();

    if(lcd_on_off==0U)
    {
        LcdInit();
        McuLcdEn();
        lcd_on_off=1;
    }

    McuLcdUpdate();
    EnablePrimask();
}

 /*************************************************
Function: void LcdDisplayDisable(void)
Description:关闭LCD显示功能
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayDisable(void)
{
    DisablePrimask();

    McuLcdDis();
    lcd_on_off=0;
    delay_func_us(10 * 1000);

    EnablePrimask();
}

/*************************************************
Function: static void LcdClearData(u8 num)
Description:清LCD显示寄存器
Input:u8 num  num取值范围0~9代表数字段10个位置。
Return:无
Others:内部接口
*************************************************/
__RAM_FUNC void LcdClearData(u8 num)
{
    if (num >= JC_LCD_CHAR_NUM) return;
    if (0 == McuLcdWriteNum(2*num, DIS_BLANK))
    {
        g_char_data[num] = DIS_BLANK;
    }

    McuLcdUpdate();
}

//新加工作接口
/*************************************************
Function: void LcdDisplayFailure(u8 flag)
Description:lcd 故障检测
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayFailure(u8 flag)
{
    McuLcdWriteOneStatus(FUN_MALFUNCTION, flag);
    McuLcdUpdate();
}

 /*************************************************
Function: void LcdDisplayLowFlowAlarm(u8 flag)
Description:lcd 低流量检测
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
__RAM_FUNC void LcdDisplayLowFlowAlarm(u8 flag)
{
    McuLcdWriteOneStatus(FUN_FLOW_LOW, flag);
    McuLcdUpdate();
}

/*************************************************
Function: u8 LcdIfSleep(void)
Description:LCD是否允许休眠
Input:无
Return:TRUE允许；FALSE不允许
Others:处于Mainspace第三类接口：休眠前接口
*************************************************/
__RAM_FUNC u8 LcdIfSleep(void)
{

}

 /*************************************************
Function: void LcdPreSleep(void)
Description:LCD休眠前处理
Input:无
Return:无
Others:处于Mainspace第三类接口：休眠前接口
*************************************************/
__RAM_FUNC void LcdPreSleep(void)
{

}

/*************************************************
Function: void LcdWakeSleep(void)
Description:LCD唤醒后处理
Input:无
Return:无
Others:处于Mainspace，第四类接口：唤醒接口
*************************************************/
__RAM_FUNC void LcdWakeSleep(void)
{

}






