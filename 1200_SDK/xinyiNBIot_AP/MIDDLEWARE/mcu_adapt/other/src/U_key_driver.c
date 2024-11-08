/***********************************************************************************
 * @Copyright (c)   :(C)2020, Qindao ieslab Co., Ltd
 * @FileName        :hc32_key_driver.c
 * @Author          :
 * @Version         :V1.0
 * @Date            :2020-07-01
 * @Description :the function of the entity of system processor
 ************************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "U_timer1uS_driver.h"
#include "U_key_driver.h"
#include "U_gpio_driver.h"
// #include "U_frame_app.h"
// #include "sys_processor.h"

/* Private variables ---------------------------------------------------------*/
volatile u8 v_g_key_press_msg = 0;          //中断产生的MSG
static u8 s_g_key_machine_state = KEY_M_S0; //按键查询状态
static u8 s_g_key_press_sum = 0;            //报警采样次数
static u8 s_g_key_press_happen_msg;         //短按事件发生msg
static u8 s_g_key_long_press_happen_flg = 0;         //长按事件发生标志
static u32 s_key_time_num = 0;
static u32 s_key_time_old_num = 0;
static u32 s_key_1stime_num = 0;
static u32 s_key_1stime_old_num = 0;
static u8 s_key_type = NONE_KEY;
static u16 s_tch_key_press_limit_cnt = 0; //触摸按键次数限制
/* Exported functions -------------------------------------------------------*/
static u8 CheckPressDownMsg(void);
static void ClearPressDownMsg(void);
static void KeySet5msTimer(u16 time5ms_span);
static u8 KeyCheck5msTimer(void);
static void KeySet1sTimer(u16 time1s_span);
static u8 KeyCheck1sTimer(void);
static u8 KeyReadPin(u8 KeyType);

void key_gpio_callback(void);
void tch_key_gpio_callback(void);

/*************************************************
Function:void KeyInit(void)
Description: 按键初始化
Input:  None
Return: None
Others:	处于MainSpace，第一类接口：上电初始化接口。
*************************************************/
__RAM_FUNC static void TchKeyInit(void)
{
    McuGpioModeSet(TCH_KEY_POWER_GPIO_PIN,0x00);    //推挽输出
    McuGpioDrvStrengthSet(TCH_KEY_POWER_GPIO_PIN,0); //低驱动能力
    McuGpioWrite(TCH_KEY_POWER_GPIO_PIN,0); //关闭供电


    McuGpioModeSet(TCH_KEY_GPIO_PIN,0x11);    //数字浮空输入
    McuGpioDrvStrengthSet(TCH_KEY_GPIO_PIN,0); //低驱动能力
    McuGpioWrite(TCH_KEY_POWER_GPIO_PIN,0); //关闭供电


    // if(((FrameCheckParaApp(FramePara_FuncCfg0_3) >> 24) & 0xFF) == 0xAA)
    {
        McuGpioWrite(TCH_KEY_POWER_GPIO_PIN,1);//供电打开

        /* 上升沿或者下降沿 */
        if (KEY_RISING_EDGE)
        {
            McuGpioIrqReg(TCH_KEY_GPIO_PIN, tch_key_gpio_callback, MCU_GPIO_INT_RISING);
        }

        if (KEY_FALLING_EDGE)
        {
            McuGpioIrqReg(TCH_KEY_GPIO_PIN, tch_key_gpio_callback, MCU_GPIO_INT_FALLING);
        }
        McuGpioIrqEn(TCH_KEY_GPIO_PIN);
    }
}
__RAM_FUNC static void MegnetKeyInit(void)
{
    McuGpioModeSet(KEY_GPIO_PIN,0x11);    //数字浮空输入
    McuGpioDrvStrengthSet(KEY_GPIO_PIN,0); //低驱动能力

    /* 上升沿或者下降沿 */
    if (KEY_RISING_EDGE)
    {
        McuGpioIrqReg(KEY_GPIO_PIN, key_gpio_callback, MCU_GPIO_INT_RISING);
    }

    if (KEY_FALLING_EDGE)
    {
        McuGpioIrqReg(KEY_GPIO_PIN, key_gpio_callback, MCU_GPIO_INT_FALLING);
    }
    McuGpioIrqEn(KEY_GPIO_PIN);
}
__RAM_FUNC void KeyInit(void)
{
    TchKeyInit();
    MegnetKeyInit();
}
/*************************************************
Function: void KeyMachineDriver(void)
Description: 主循环前台处理机，用于系统处理机在主循环调度此接口
Input:  None
Return: None
Others:	处于MainSpace，第二类接口：工作接口。
 *************************************************/
__RAM_FUNC void KeyMachineDriver(void)
{
    switch (s_g_key_machine_state)
    {
        case KEY_M_S0:
        {
            if(s_g_key_long_press_happen_flg)
            {
                s_g_key_long_press_happen_flg = 0;

                if((0 == Check100msTimer(TIMER_100MS_LONGLONGKEYSTART)) && (1 == Check100msTimer(TIMER_100MS_LONGLONGKEYEND)))
                {
                    s_g_key_press_happen_msg |= LONGLONG_PRESS; //抛出开阀按键MSG
                }
            }

            s_key_type = CheckPressDownMsg();

            if (s_key_type != NONE_KEY)
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type)) //抖动
                {
                    ClearPressDownMsg(); //清中断MSG
                    s_g_key_machine_state = KEY_M_S0;
                }
                else
                {
                    ClearPressDownMsg();
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S1;
                }
            }
            else
            {
                // if(FrameCheckMsgApp(MsgTchKey_ClearKeyLimit))
                {
                    // FrameClearMsgApp(MsgTchKey_ClearKeyLimit);
                    s_tch_key_press_limit_cnt = 0;
                    // FrameSetMsgApp(MsgTouchKeyReset);//复位触摸按键
                }

                s_g_key_machine_state = KEY_M_S0;
            }
        }
        break;

        case KEY_M_S1:
        {
            if (0 == KeyCheck5msTimer()) //定时时间到
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S2;
                }
                else
                {
                    if (++s_g_key_press_sum >= SHORT_START)
                    {
                        if(s_key_type == TOUCH_KEY)//触摸按键
                        {
                            s_tch_key_press_limit_cnt++;

                            if(s_tch_key_press_limit_cnt > TCH_PRESS_LIMIT_CNT)
                            {
                                McuGpioWrite(TCH_KEY_POWER_GPIO_PIN,0); //关闭触摸芯片供电
                                s_g_key_press_happen_msg |= ERR_PRESS; //抛出异常按键MSG
                                s_g_key_machine_state = KEY_M_S0;
                                return;
                            }
                        }
                        else
                        {
                            if(s_tch_key_press_limit_cnt > TCH_PRESS_LIMIT_CNT)
                            {
                                // FrameSetMsgApp(MsgTchKey_ClearKeyLimit);
                            }
                        }

                        s_g_key_press_happen_msg |= SHORT_PRESS; //抛出短按键MSG
                        s_g_key_machine_state = KEY_M_S3;
                    }
                    else
                    {
                        s_g_key_machine_state = KEY_M_S1;
                    }

                    KeySet5msTimer(KEY_CHECK_TIME);
                }
            }
            else //定时时间未到
            {
                s_g_key_machine_state = KEY_M_S1;
            }
        }
        break;

        case KEY_M_S2:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    s_g_key_press_sum = 0;
                    ClearPressDownMsg();
                    s_g_key_machine_state = KEY_M_S0;
                }
                else
                {
                    ClearPressDownMsg();
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S1;
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S2;
            }
        }
        break;

        case KEY_M_S3:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S4;
                }
                else
                {
                    if (++s_g_key_press_sum >= SHORT_END)
                    {
                        if(s_key_type == TOUCH_KEY)//触摸按键
                        {
                            s_tch_key_press_limit_cnt++;

                            if(s_tch_key_press_limit_cnt > TCH_PRESS_LIMIT_CNT)
                            {
                                McuGpioWrite(TCH_KEY_POWER_GPIO_PIN,0); //关闭触摸芯片供电
                                s_g_key_press_happen_msg |= ERR_PRESS; //抛出异常按键MSG
                                s_g_key_machine_state = KEY_M_S0;
                                return;
                            }
                        }
                        else
                        {
                            if(s_tch_key_press_limit_cnt > TCH_PRESS_LIMIT_CNT)
                            {
                                // FrameSetMsgApp(MsgTchKey_ClearKeyLimit);
                            }
                        }

                        s_g_key_press_happen_msg |= LONG_PRESS; //抛出长按键MSG
                        s_g_key_long_press_happen_flg = 1;
                        Set100msTimer(TIMER_100MS_LONGLONGKEYSTART, 0);
                        Set100msTimer(TIMER_100MS_LONGLONGKEYEND, 150);
                        s_g_key_machine_state = KEY_M_S5;
                    }
                    else
                    {
                        s_g_key_machine_state = KEY_M_S3;
                    }

                    KeySet5msTimer(KEY_CHECK_TIME);
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S3;
            }
        }
        break;

        case KEY_M_S4:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    s_g_key_press_sum = 0;
                    s_g_key_machine_state = KEY_M_S0;
                }
                else
                {
                    ClearPressDownMsg();
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S3;
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S4;
            }
        }
        break;

        case KEY_M_S5:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S6;
                }
                else
                {
                    if (FALSE == (KeyCheckMsg() & LONG_PRESS))
                    {
                        s_g_key_press_sum = 0;
                        KeySet1sTimer(30);
                        s_g_key_machine_state = KEY_M_S7;
                    }
                    else
                    {
                        KeySet5msTimer(KEY_CHECK_TIME);
                        s_g_key_machine_state = KEY_M_S5;
                    }
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S5;
            }
        }
        break;

        case KEY_M_S6:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    s_g_key_press_sum = 0;
                    s_g_key_machine_state = KEY_M_S0;
                }
                else
                {
                    ClearPressDownMsg();
                    KeySet5msTimer(KEY_CHECK_TIME);
                    s_g_key_machine_state = KEY_M_S5;
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S6;
            }
        }
        break;

        case KEY_M_S7:
        {
            if (0 == KeyCheck1sTimer())
            {
                s_g_key_press_sum = 0;
                s_g_key_press_happen_msg |= ERR_PRESS; //抛出异常按键MSG
                s_g_key_machine_state = KEY_M_S0;
            }
            else
            {
                KeySet5msTimer(KEY_CHECK_TIME);
                s_g_key_machine_state = KEY_M_S8;
            }
        }
        break;

        case KEY_M_S8:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    s_g_key_machine_state = KEY_M_S9;
                    KeySet5msTimer(KEY_CHECK_TIME);
                }
                else
                {
                    if (++s_g_key_press_sum >= LONG_INTERVAL)
                    {
                        s_g_key_press_sum = 0;
                        s_g_key_press_happen_msg |= LONG_PRESS;
                        s_g_key_machine_state = KEY_M_S7;
                    }
                    else
                    {
                        s_g_key_machine_state = KEY_M_S7;
                    }
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S8;
            }
        }
        break;

        case KEY_M_S9:
        {
            if (0 == KeyCheck5msTimer())
            {
                if (KEY_NO_PRESS == KeyReadPin(s_key_type))
                {
                    s_g_key_press_sum = 0;
                    s_g_key_machine_state = KEY_M_S0;
                }
                else
                {
                    ClearPressDownMsg();
                    s_g_key_machine_state = KEY_M_S7;
                }
            }
            else
            {
                s_g_key_machine_state = KEY_M_S9;
            }
        }
        break;

        default:
        {
            s_g_key_machine_state = KEY_M_S0;
        }
        break;
    }
}

/*************************************************
Function: u8 KeyIfIdle(void)
Description: 查询按键是否空闲
Input: 	None
Return: TASK_IDLE:空闲;TASK_BUSY:忙碌
Others:	处于MainSpace，第二类接口：工作接口。
 *************************************************/
__RAM_FUNC u8 KeyIfIdle(void)
{
    if (KEY_M_S0 == s_g_key_machine_state)
    {
        return TASK_IDLE;
    }
    else
    {
        return TASK_BUSY;
    }
}

/*************************************************
Function: u8 KeyCheckMsg(void)
Description: 检测当前按键事件Msg
Input:  None
Return: 按键事件Msg,按位与NO_EVENT；SHORT_PRESS ；LONG_PRESS；ERR_PRESS；
Others: 处于MainSpace，第二类接口：工作接口。
 *************************************************/
__RAM_FUNC u8 KeyCheckMsg(void)
{
    return s_g_key_press_happen_msg;
}

/*************************************************
Function: void KeyClearMsg(void)
Description: 清按键MSG
Input:  清除msg的位置
Return: None
Others:	处于MainSpace，第二类接口：工作接口。
 *************************************************/
__RAM_FUNC void KeyClearMsg(u8 msg)
{
    if ((msg <= 0) || (msg > 8))
    {
        return;
    }

    u8 temp;
    temp = 0x01 << (msg - 1);
    s_g_key_press_happen_msg &= ~temp;
}
/*************************************************
Function: u8 KeyIfSleep(void)
Description: 查询是否允许休眠
Input:  None
Return: TRUE允许休眠 ;FALSE不允许休眠
Others:	处于MainSpace，第三类接口：休眠前接口。
 *************************************************/
__RAM_FUNC u8 KeyIfSleep(void)
{
    if (KEY_M_S0 == s_g_key_machine_state)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*************************************************
Function: void KeyPreSleep(void)
Description: 休眠前处理
Input:  None
Return: None
Others:	处于MainSpace，第三类接口：休眠前接口。
 *************************************************/
__RAM_FUNC void KeyPreSleep(void)
{
}
/*************************************************
Function:void KeyWakeSleep(void)
Description: 休眠后模块初始化
Input:  None
Return: None
Others:	处于MainSpace，第四类接口：唤醒后接口。
 *************************************************/
__RAM_FUNC void KeyWakeSleep(void)
{
}
/*************************************************
Function: u8 CheckPressDownMsg(void)
Description: 查询是否有中断按下Msg
Input: 	None
Return:	TRUE：有按键按下；FALSE：无按键按下；
Others:	内部接口
 *************************************************/
__RAM_FUNC static u8 CheckPressDownMsg(void)
{
    return v_g_key_press_msg;
}
/*************************************************
Function: void ClearPressDownMsg(void)
Description: 清除中断按下Msg
Input: 	None
Return:	None
Others:	内部接口
 *************************************************/
__RAM_FUNC static void ClearPressDownMsg(void)
{
    v_g_key_press_msg = NONE_KEY;
}

__RAM_FUNC static u8 KeyReadPin(u8 KeyType)
{
    if(KeyType == TOUCH_KEY)
    {
        return McuGpioRead(TCH_KEY_GPIO_PIN);
    }
    else
    {
        return McuGpioRead(KEY_GPIO_PIN);
    }
}
/*************************************************
Function: u8 TchKey_GetPressLimitState(void)
Description: 检查触摸按键次数是否超限
*************************************************/
__RAM_FUNC u8 TchKey_GetPressLimitState(void)
{
    if(s_tch_key_press_limit_cnt > TCH_PRESS_LIMIT_CNT)
    {
        return 1;
    }

    return 0;
}
/*************************************************
Function:void KeySet5msTimer(u16 time5ms_span)
Description: 设置5ms定时器
Input:	time5ms_span:时间
Return: _SUCCESS：初始化成功；_ERROR ：初始化失败
Others:内部接口
*************************************************/
__RAM_FUNC static void KeySet5msTimer(u16 time5ms_span)
{
    s_key_time_num = (u32)time5ms_span * 1000 * 5;
    s_key_time_old_num = Timer1usGetTick();
}

/*************************************************
Function: u8 KeyCheck5msTimer(void)
Description: 检查定时器当前值，如果为0则说明定时时间到
Input:	void
Return: 定时器当前值
Others:	内部接口
*************************************************/
__RAM_FUNC static u8 KeyCheck5msTimer(void)
{
    u32 tmp_count = 0;

    tmp_count = Timer1usGetTick() - s_key_time_old_num;

    if (tmp_count >= s_key_time_num)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
/*************************************************
Function:void KeySet1msTimer(u16 time1s_span)
Description: 设置5ms定时器
Input:	time5ms_span:时间
Return: _SUCCESS：初始化成功；_ERROR ：初始化失败
Others:	处于MainSpace，第二类接口：工作接口。
*************************************************/
__RAM_FUNC static void KeySet1sTimer(u16 time1s_span)
{
    s_key_1stime_num = (u32)time1s_span * 1000000;
    s_key_1stime_old_num = Timer1usGetTick();
}

/*************************************************
Function: u8 KeyCheck1msTimer(void)
Description: 检查定时器当前值，如果为0则说明定时时间到
Input:	void
Return: 定时器当前值
Others:	处于MainSpace，第二类接口：工作接口。
*************************************************/
__RAM_FUNC static u8 KeyCheck1sTimer(void)
{
    u32 tmp_count = 0;

    tmp_count = Timer1usGetTick() - s_key_1stime_old_num;

    if (tmp_count >= s_key_1stime_num)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//以下是外部中断入口
__RAM_FUNC void key_gpio_callback(void)
{
    v_g_key_press_msg = MEGNET_KEY;
}

__RAM_FUNC void tch_key_gpio_callback(void)
{
    // if(((FrameCheckParaApp(FramePara_FuncCfg0_3) >> 24) & 0xFF) == 0xAA) //触摸功能开启
    {
        // if(FixedTimeNbIoTUploadIfIdle() == TASK_IDLE) //NB上报空闲
        {
            v_g_key_press_msg = TOUCH_KEY;
        }
    }
}


