/*******************************************************************************
* @Copyright (c)    :(C)2020, Qingdao ieslab Co., Ltd
* @FileName         :hc32_rtcc_driver.c
* @Author           :Kv-L
* @Version          :V1.0
* @Date             :2020-07-01 15:30:52
* @Description      :the function of the entity of GP22Gas_rtcc_driver.c
*******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "U_lptimer_driver.h"
#include "mcu_adapt.h"


/* Private variables ---------------------------------------------------------*/
u8 lpTimer_flag = FALSE;

/* Private function prototypes -----------------------------------------------*/
void LpTim0_IRQHandler(void);

void LPTimer0Init(u8 Rtc_Clk_Source, u32 time)
{
    McuLptimerSetPeriod(time);
    McuLptimerIrqReg(LpTim0_IRQHandler);
    // McuLptimerEn();
}
/****************************************************************************
Function:    LPTimer1Init
Description: 低功耗时钟1初始化
Input:
Return:
Others:
*****************************************************************************/
void LPTimer1Init(u8 Rtc_Clk_Source)
{
    
}
/*************************************************
Function:  u32 LPTimerGetTick(u8 timerNum)
Description: 获取当前计数器的ms数值
Input：
Return:
Others:
*************************************************/
u16 LPTimer1GetTick()
{
    u16 b;
    b = McuLptimerGetCountMs();
    return b ;
}

void LPTimer0_Work(u8 enCmd)
{
    if (enCmd == TRUE)
    {
        PRCM_ClockEnable(CORE_CKG_CTL_LPTMR_EN); 
        McuLptimerEn();
    }
    else
    {
        McuLptimerDis();
        PRCM_ClockDisable(CORE_CKG_CTL_LPTMR_EN); 
    }
}

u8 GetLpTimerFlag(void)
{
    return lpTimer_flag;
}

void ClearLpTimerFlag(void)
{
    lpTimer_flag = FALSE;
}
/*******************************************************************************
* @fun_name     LpTim0_IRQHandler
* @brief
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void LpTim0_IRQHandler(void)
{
    lpTimer_flag = TRUE;
}

/*******************************************************************************
* @fun_name     LpTim0WakeSleep
* @brief        唤醒后处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void LpTim0WakeSleep(void)
{
    ;//唤醒不开启LpTime0，使用前再开启。
}

/*******************************************************************************
* @fun_name     LPTPreSleep
* @brief        LPT休眠前处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void LpTime0PreSleep(void)
{
    McuLptimerDis();
    PRCM_ClockDisable(CORE_CKG_CTL_LPTMR_EN); 
}
/*******************************************************************************
* @fun_name     LpTim0IfSleep
* @brief        是否允许休眠
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
u8 LpTime0IfSleep(void)
{
    return TRUE;
}
#ifdef __cplusplus
}
#endif
