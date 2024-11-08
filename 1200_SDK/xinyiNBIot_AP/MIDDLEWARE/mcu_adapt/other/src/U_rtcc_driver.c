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
#include "U_timer1uS_driver.h"
#include "U_rtcc_driver.h"
#include "sys_clk.h"
//#include "U_DataTools_App.h"
//#include "U_frame_app.h"
//#include "soe.h"

/* Private variables ---------------------------------------------------------*/
//static u8 s_Rtcc_AlarmA_Count = 0;
//static u16 s_Rtcc_AlarmA_Cycle = 0;
//static u16 s_Rtcc_AlarmA_Bits = 0;
//static u8 s_Rtcc_AlarmA_MASK = 0;
//static u8 s_Rtcc_AlarmA_Reload = FALSE;
//static u16 s_Rtcc_AlarmA_Set = 0;
//static u8 s_Rtcc_AlarmA_Limit_Flg = FALSE;

static u8 s_Rtcc_Alarm_Msg_cnt = 0;
static u8 s_Rtcc_Alarm_Minite = 0;
static rtcc_Time PT;
static u8 s_Rtcc_Alarm_Msg = 0;   //Alarm msg :bit0: s msg    bit1: min msg   bit2:period msg   bit0:rtc timer msg
//static u32 s_Rtcc_Alarm_Tr = 0;   //Alarm TR
//static u32 s_Rtcc_Alarm_Dr = 0;   //Alarm DR
//static u8 s_Rtcc_WUT_Msg = FALSE; //WUT msg

static u16 s_Rtcc_Timer_Array[RTC_TIM_MAX_NUM]; //RTCC Timer
static u16 s_Rtcc_Timer_Period = 0;             //

//static u32 s_Rtcc_TickStart = 0; //timeout start time

/* Private function prototypes -----------------------------------------------*/
static u8 Rtcc_Get_Weekday(u8 year, u8 month, u8 day);
u8 Rtcc_Check_DateTime(DateTime *pDT);
//static u16 Rtcc_Get_Mask(u16 data);

static DateTime init_time_driver;
//用于定时,基本定时器使用基时2个32.768CLK
//static u32 s_rtcc_init_limit_time=0;
static u8 rtcc_clk_source = 0; //0:外部低速时钟,1:内部高速时钟
static RTC_TimeTypeDef g_rtc_time;

extern void Rtc_IRQHandler(void);
/**
 *******************************************************************************
 ** \brief 系统时钟源使能
 ** \param [in]  enSource   目标时钟源
 ** \param [in]  bFlag      使能1-开/0-关
 ** \retval      Ok         设定成功
 **             其他        设定失败
 ******************************************************************************/

#define en_result_t uint8_t
#define en_sysctrl_clk_source_t uint8_t
#define boolean_t uint8_t
en_result_t RTC_ClkSourceEnable(en_sysctrl_clk_source_t enSource, boolean_t bFlag)
{
	return 0;
}
/*******************************************************************************
* @fun_name     RtccInit
* @brief        RTCC初始化接口
* @param[in]    None
* @param[out]   None
* @retval       u8:0:使用外部低速时钟,1:使用内部高速时钟
* @other        None
*******************************************************************************/
#define JC_TIMER_RTC        		TIMER_LP_USER6
u8 RtccInit(void)
{
	/*RTC 中断设置*/
	#if RTC_TIMER_RPT_SEC
	s_Rtcc_Timer_Period = 1;
	#elif RTC_TIMER_RPT_MIN
	#elif RTC_TIMER_RPT_HOUR
	#elif RTC_TIMER_RPT_DAY
	#elif RTC_TIMER_RPT_WEEK //不支持周中断
	#else //RTC_TIMER_RPT_MON
	#endif

#if RTC_TIMER_EN
	DateTime pDT;
	pDT.s.second = 0;
	pDT.s.minute = 0x44;
	pDT.s.hour = 0x19;
	pDT.s.day = 0x24;
	pDT.s.month = 0x1;
	pDT.s.year = 0x24;
	RtccSetDateTime(&pDT);
	Timer_AddEvent(JC_TIMER_RTC, s_Rtcc_Timer_Period * 1000, Rtc_IRQHandler, 1);   //RTC中断
#endif
}

void RTCC_SetTime(RTC_TimeTypeDef *rtc_time)
{
	DisablePrimask();

	g_rtc_time.tm_year = rtc_time->tm_year;//DateTime结构体都是十六进制
	g_rtc_time.tm_mon  = rtc_time->tm_mon;
	g_rtc_time.tm_mday = rtc_time->tm_mday;
	g_rtc_time.tm_hour = rtc_time->tm_hour;
	g_rtc_time.tm_min  = rtc_time->tm_min;
	g_rtc_time.tm_sec  = rtc_time->tm_sec;

	EnablePrimask();
}

void RTCC_GetTime(RTC_TimeTypeDef *rtc_time)
{
	DisablePrimask();

	rtc_time->tm_year = g_rtc_time.tm_year;//DateTime结构体都是十六进制
	rtc_time->tm_mon  = g_rtc_time.tm_mon;
	rtc_time->tm_mday = g_rtc_time.tm_mday;
	rtc_time->tm_hour = g_rtc_time.tm_hour;
	rtc_time->tm_min  = g_rtc_time.tm_min;
	rtc_time->tm_sec  = g_rtc_time.tm_sec;
	rtc_time->tm_wday  = (uint32_t)clock_dayoftheweek((int)g_rtc_time.tm_mday, (int)g_rtc_time.tm_mon, (int)g_rtc_time.tm_year);
	rtc_time->tm_isdst = 0;

	EnablePrimask();
}

/*******************************************************************************
* @fun_name     RtccSetDateTime
* @brief        设置日期时间接口
* @param[in]    DateTime
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccSetDateTime(DateTime *pDT)
{
    RTC_TimeTypeDef rtc_time = {0};

    rtc_time.tm_year = BCD2DEC(pDT->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(pDT->s.month);
    rtc_time.tm_mday = BCD2DEC(pDT->s.day);
    rtc_time.tm_hour = BCD2DEC(pDT->s.hour);
    rtc_time.tm_min  = BCD2DEC(pDT->s.minute);
    rtc_time.tm_sec  = BCD2DEC(pDT->s.second);

    RTCC_SetTime(&rtc_time);
}

/*******************************************************************************
* @fun_name     RtccGetDateTime
* @brief        获取当前日期时间接口
* @param[in]    DateTime*
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
u8 RtccGetDateTime(DateTime *pDT)
{
	RTC_TimeTypeDef rtc_time;

	RTCC_GetTime(&rtc_time);

    pDT->s.second  = DEC2BCD(rtc_time.tm_sec);
    pDT->s.minute  = DEC2BCD(rtc_time.tm_min);
    pDT->s.hour    = DEC2BCD(rtc_time.tm_hour);
    pDT->s.day     = DEC2BCD(rtc_time.tm_mday);
    pDT->s.month   = DEC2BCD(rtc_time.tm_mon);
    pDT->s.year    = DEC2BCD(rtc_time.tm_year - 2000);
    pDT->s.weekday = DEC2BCD(rtc_time.tm_wday - 1);

    return Rtcc_Check_DateTime(pDT);
}

/*******************************************************************************
* @fun_name     RtccGetDate
* @brief        获取日期接口
* @param[in]    Date*
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccGetDate(Date *pD)
{
	RTC_TimeTypeDef rtc_time;

	RTCC_GetTime(&rtc_time);

	pD->s.day     = DEC2BCD(rtc_time.tm_mday);
	pD->s.month   = DEC2BCD(rtc_time.tm_mon);
	pD->s.year    = DEC2BCD(rtc_time.tm_year - 2000);
	pD->s.weekday = DEC2BCD(rtc_time.tm_wday - 1);
}

/*******************************************************************************
* @fun_name     RtccGetTime
* @brief        获取时间接口
* @param[in]    Time*
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccGetTime(rtcc_Time *pT)
{
	RTC_TimeTypeDef rtc_time;

	RTCC_GetTime(&rtc_time);

	pT->s.second  = DEC2BCD(rtc_time.tm_sec);
	pT->s.minute  = DEC2BCD(rtc_time.tm_min);
	pT->s.hour    = DEC2BCD(rtc_time.tm_hour);
}

/*******************************************************************************
* @fun_name     RtccAdjustDateTime
* @brief        调整日期时间接口
* @param[in]    u8 adjust：0减；非0加       u32 sec :调整秒数
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccAdjustDateTime(u8 adjust, u32 sec)
{
	uint64_t msec_offset;
	uint64_t rtc_msec;
	RTC_TimeTypeDef rtc_time;
    DateTime cdt;

	msec_offset = 1000 * sec;
	RTCC_GetTime(&rtc_time);

    if (0 == adjust) //0: 减
    {
    	rtc_msec = xy_mktime(&rtc_time) - ((uint64_t)(msec_offset)*XY_UTC_CLK/32000ULL);
    }
    else //非0：加
    {
    	rtc_msec = xy_mktime(&rtc_time) + ((uint64_t)(msec_offset)*XY_UTC_CLK/32000ULL);
    }

    xy_gmtime(rtc_msec, &rtc_time);


    cdt.s.second  = DEC2BCD(rtc_time.tm_sec);
    cdt.s.minute  = DEC2BCD(rtc_time.tm_min);
    cdt.s.hour    = DEC2BCD(rtc_time.tm_hour);
    cdt.s.day     = DEC2BCD(rtc_time.tm_mday);
    cdt.s.month   = DEC2BCD(rtc_time.tm_mon);
    cdt.s.year    = DEC2BCD(rtc_time.tm_year - 2000);
    cdt.s.weekday = DEC2BCD(rtc_time.tm_wday - 1);

    RtccSetDateTime(&cdt);
}

/*******************************************************************************
* @fun_name     RtccSetTimer
* @brief        RTCC TIM 设置定时接口
* @param[in]    u8 rtcc_timenum：申请的定时器； u16 rtcc_timespan：定时长短(s)
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccSetTimer(u8 rtcc_timenum, u16 rtcc_timespan)
{
    if (rtcc_timenum < RTC_TIM_MAX_NUM)
    {
        s_Rtcc_Timer_Array[rtcc_timenum] = rtcc_timespan;
    }
}

/*******************************************************************************
* @fun_name     RtccCheckTimer
* @brief        查询定时器接口
* @param[in]    u8 rtcc_timenum：申请的定时器
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
u16 RtccCheckTimer(u8 rtcc_timenum)
{
    if (rtcc_timenum < RTC_TIM_MAX_NUM)
    {
        return s_Rtcc_Timer_Array[rtcc_timenum];
    }
    else
    {
        return 0;
    }
}

/*******************************************************************************
* @fun_name     RtccSetAlrm
* @brief        设置闹钟接口
* @param[in]    u8 count:闹钟次数，0表示无限次；    u32 cycle：闹钟间隔(us)
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccSetAlrm(DateTime *pDT)
{
    //不用实现
}

/*******************************************************************************
* @fun_name     RtccGetAlrm
* @brief        获取闹钟接口
* @param[in]    u8 count:闹钟次数，0表示无限次；    u32 cycle：闹钟间隔(us)
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccGetAlrm(DateTime *pDT)
{
    //不用实现
}

/*******************************************************************************
* @fun_name     RtccCheckMsg
* @brief        查询闹钟msg接口
* @param[in]    None
* @param[out]   Msg：
* @retval       None
* @other
*******************************************************************************/
u8 RtccCheckMsg(void)
{
    return s_Rtcc_Alarm_Msg;
}

/*******************************************************************************
* @fun_name     RtccClearMsg
* @brief        清msg接口
* @param[in]    u8 bit：0：清bit0；  N：清bitN  (N<8)
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccClearMsg(u8 bit)
{
    if(bit > 7)
    {
        return;
    }

    s_Rtcc_Alarm_Msg &= ~(1 << bit);
}

/*******************************************************************************
* @fun_name     RtccGetAlrmTimeAfter
* @brief        获取最近整分时间接口
* @param[in]    DateTime*
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
//void RtccGetAlrmTimeAfter(DateTime *pDT)
//{
//	pDT->s.second = (u8)((s_Rtcc_Alarm_Tr >> 0) & (u32)0x7F);
//	pDT->s.minute = (u8)((s_Rtcc_Alarm_Tr >> 8) & (u32)0x7F);
//	pDT->s.hour = (u8)((s_Rtcc_Alarm_Tr >> 16) & (u32)0x3F);

//	pDT->s.day = (u8)((s_Rtcc_Alarm_Dr >> 0) & (u32)0x3F);
//	pDT->s.month = (u8)((s_Rtcc_Alarm_Dr >> 8) & (u32)0x1F);
//	pDT->s.year = (u8)((s_Rtcc_Alarm_Dr >> 16) & (u32)0xFF);

//	pDT->s.weekday = Rtcc_Get_Weekday(pDT->s.year, pDT->s.month, pDT->s.day);
//}
/*************************************************
Function：s32 RtcccalculateDiffTime(const DateTime *newpD, const DateTime *oldpD)
Description：计算时间差
Input:newData oldData带计算的时间
Output：void
Return：newData-oldData=差值时间(单位second)
Others:
 *************************************************/
int32_t RtcccalculateDiffTime(const DateTime *newpD, const DateTime *oldpD)
{
	uint64_t rtc_msec_newpD;
	uint64_t rtc_msec_oldpD;
	RTC_TimeTypeDef rtc_time;

    rtc_time.tm_year = BCD2DEC(newpD->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(newpD->s.month);
    rtc_time.tm_mday = BCD2DEC(newpD->s.day);
    rtc_time.tm_hour = BCD2DEC(newpD->s.hour);
    rtc_time.tm_min  = BCD2DEC(newpD->s.minute);
    rtc_time.tm_sec  = BCD2DEC(newpD->s.second);

    rtc_msec_newpD = xy_mktime(&rtc_time);

    rtc_time.tm_year = BCD2DEC(oldpD->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(oldpD->s.month);
    rtc_time.tm_mday = BCD2DEC(oldpD->s.day);
    rtc_time.tm_hour = BCD2DEC(oldpD->s.hour);
    rtc_time.tm_min  = BCD2DEC(oldpD->s.minute);
    rtc_time.tm_sec  = BCD2DEC(oldpD->s.second);

    rtc_msec_oldpD = xy_mktime(&rtc_time);

    return (rtc_msec_newpD - rtc_msec_oldpD) / 1000;
}


/*******************************************************************************
* @fun_name     RtccIfSleep
* @brief        查询RTCC是否允许休眠
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
u8 RtccIfSleep(void)
{
    return TRUE;
}

/*******************************************************************************
* @fun_name     RtccPreSleep
* @brief        休眠前处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccPreSleep(void)
{

}
//掉电后关闭RTC闹钟
void RtccPrePowerDown(void)
{
    Rtc_AlmEnCmd(FALSE);      //闹钟禁止
}
/*******************************************************************************
* @fun_name     RtccWakeSleep
* @brief        唤醒后处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
void RtccWakeSleep(void)
{
    ;
}

/*******************************************************************************
* @fun_name     Rtcc_Get_Weekday
* @brief        计算周几函数
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
static u8 Rtcc_Get_Weekday(u8 year, u8 month, u8 day)
{
	RTC_TimeTypeDef rtc_time, newrtc_time;
	uint64_t rtc_msec;

    rtc_time.tm_year = BCD2DEC(year) + 2000;
    rtc_time.tm_mon  = BCD2DEC(month);
    rtc_time.tm_mday = BCD2DEC(day);
    rtc_time.tm_hour = 0;
    rtc_time.tm_min  = 0;
    rtc_time.tm_sec  = 0;

    rtc_msec = xy_mktime(&rtc_time);
    xy_gmtime(rtc_msec, &newrtc_time);

    return newrtc_time.tm_wday;
}

/*******************************************************************************
* @fun_name     Rtcc_Check_DateTime
* @brief        检查日期合法性
* @param[in]    None
* @param[out]   SUCCESS：合法； ERROR：非法
* @retval       None
* @other        None
*******************************************************************************/
u8 Rtcc_Check_DateTime(DateTime *pDT)
{
    int i = 0;

    for (i = 0; i < 7; i++)
    {
        if ((pDT->b[i] >> 4) > 9 || (pDT->b[i] & 0x0F) > 9)
        {
            if(i != 3)//不判断week
            {
                return ERROR;
            }
        }
    }

    if ((0 == pDT->s.month) || (pDT->s.month > 0x12))
    {
        return ERROR;
    }

    if ((0 == pDT->s.day) || (pDT->s.day > 0x31))
    {
        return ERROR;
    }

    if (pDT->s.hour > 0x23)
    {
        return ERROR;
    }

    if (pDT->s.minute > 0x59)
    {
        return ERROR;
    }

    if (pDT->s.second > 0x59)
    {
        return ERROR;
    }

    if (pDT->s.month == 0x4 || pDT->s.month == 0x6 || pDT->s.month == 0x9 || pDT->s.month == 0x11)
    {
        if (pDT->s.day > 0x30)
        {
            return ERROR;
        }
    }

    if (pDT->s.month == 0x2)
    {
        if (((0 == (pDT->s.year) % 4) && (0 != (pDT->s.year) % 100)) || (0 == (pDT->s.year) % 400))
        {
            if (pDT->s.day > 0x29)
            {
                return ERROR;
            }
        }
        else
        {
            if (pDT->s.day > 0x28)
            {
                return ERROR;
            }
        }
    }

    return SUCCESS;
}

/*******************************************************************************
* @fun_name     Rtcc_Get_Mask
* @brief        计算sub sec mask bit 函数
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
/*static u16 Rtcc_Get_Mask(u16 data)
{
	u8 i = 0;
	for (i = 0; i < 15; i++)
	{
		if (data <= (1 << i))
		{
			if (data == (1 << i))
			{
				s_Rtcc_AlarmA_Reload = FALSE;
			}
			else
			{
				s_Rtcc_AlarmA_Reload = TRUE;
			}
			return ((i > 0) ? i : 1);
		}
	}
	return 14;
}*/

/*************************************************
Function：long RTCC_CalculateDiffTime(DataTime data_time)
Description：计算时间差
Input:newData oldData带计算的时间
Output：void
Return：newData-oldData=差值时间(单位second)
Others:
 *************************************************/
long RtccCalculateDiffTime(const DateTime *newpD, const DateTime *oldpD)
{
	uint64_t rtc_msec_newpD;
	uint64_t rtc_msec_oldpD;
	RTC_TimeTypeDef rtc_time;

    rtc_time.tm_year = BCD2DEC(newpD->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(newpD->s.month);
    rtc_time.tm_mday = BCD2DEC(newpD->s.day);
    rtc_time.tm_hour = BCD2DEC(newpD->s.hour);
    rtc_time.tm_min  = BCD2DEC(newpD->s.minute);
    rtc_time.tm_sec  = BCD2DEC(newpD->s.second);

    rtc_msec_newpD = xy_mktime(&rtc_time);

    rtc_time.tm_year = BCD2DEC(oldpD->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(oldpD->s.month);
    rtc_time.tm_mday = BCD2DEC(oldpD->s.day);
    rtc_time.tm_hour = BCD2DEC(oldpD->s.hour);
    rtc_time.tm_min  = BCD2DEC(oldpD->s.minute);
    rtc_time.tm_sec  = BCD2DEC(oldpD->s.second);

    rtc_msec_oldpD = xy_mktime(&rtc_time);

    return (rtc_msec_newpD - rtc_msec_oldpD) / 1000;
}

time_t get_mktime (DateTime *time_GMT)//标准时间准换为绝对秒数
{
	uint64_t rtc_msec;
	RTC_TimeTypeDef rtc_time;

    rtc_time.tm_year = BCD2DEC(time_GMT->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(time_GMT->s.month);
    rtc_time.tm_mday = BCD2DEC(time_GMT->s.day);
    rtc_time.tm_hour = BCD2DEC(time_GMT->s.hour);
    rtc_time.tm_min  = BCD2DEC(time_GMT->s.minute);
    rtc_time.tm_sec  = BCD2DEC(time_GMT->s.second);

    rtc_msec = xy_mktime(&rtc_time);
    return (rtc_msec/1000);
}

time_t get_mktime_noweek (DateTime_noweek *time_GMT)//标准时间准换为绝对秒数
{
	uint64_t rtc_msec;
	RTC_TimeTypeDef rtc_time;

    rtc_time.tm_year = BCD2DEC(time_GMT->s.year) + 2000;//DateTime结构体都是十六进制
    rtc_time.tm_mon  = BCD2DEC(time_GMT->s.month);
    rtc_time.tm_mday = BCD2DEC(time_GMT->s.day);
    rtc_time.tm_hour = BCD2DEC(time_GMT->s.hour);
    rtc_time.tm_min  = BCD2DEC(time_GMT->s.minute);
    rtc_time.tm_sec  = BCD2DEC(time_GMT->s.second);

    rtc_msec = xy_mktime(&rtc_time);
    return (rtc_msec/1000);
}
/*******************************************************************************
* @fun_name     RTC_Alarm_IRQHandler
* @brief        Alarm中断入口
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
uint8_t const rtccDayTab[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //月时间常量
void Rtc_IRQHandler(void)
{
   uint8_t DayNum;

	g_rtc_time.tm_sec++;

	if(g_rtc_time.tm_sec > 59)
	{
		g_rtc_time.tm_sec = 0;

		g_rtc_time.tm_min++;

		if(g_rtc_time.tm_min > 59)
		{
			g_rtc_time.tm_hour++;
			g_rtc_time.tm_min = 0;

			if(g_rtc_time.tm_hour > 23)
			{
				g_rtc_time.tm_hour = 0;
				g_rtc_time.tm_mday++;

				DayNum = rtccDayTab[g_rtc_time.tm_mon - 1];

				if(g_rtc_time.tm_mon == 2)
				{
					if((g_rtc_time.tm_year & 0x03) == 0)
					{
						DayNum = 29;
					}
				}

				if(g_rtc_time.tm_mday > DayNum)
				{
					g_rtc_time.tm_mday = 1;
					g_rtc_time.tm_mon++;
				}

				if(g_rtc_time.tm_mon > 12)
				{
					g_rtc_time.tm_mon = 1;
					g_rtc_time.tm_year++;
				}
			}
		}
	}

	s_Rtcc_Alarm_Msg |= 0x04;//抛周期唤醒msg
//	FrameClearMsgApp(MsgRtccAdjustTime);//清校时消息,该消息是防止检定0.5s超时复位用的
	s_Rtcc_Alarm_Msg |= 0x01;       //整秒
	s_Rtcc_Alarm_Msg |= 0x10;       //整秒,统计功耗用

	if(0 == g_rtc_time.tm_sec % 2)
	{
		s_Rtcc_Alarm_Msg |= 0x08;//整2秒
	}

	if(0 == g_rtc_time.tm_min)
	{
		s_Rtcc_Alarm_Msg |= 0x02;//整分
		s_Rtcc_Alarm_Msg |= 0x20;//整分,用于待机功耗计算
	}

	for (u8 i = 0; i < RTC_TIM_MAX_NUM; i++)
	{
		if(s_Rtcc_Timer_Array[i] >= s_Rtcc_Timer_Period)
		{
			s_Rtcc_Timer_Array[i] -= s_Rtcc_Timer_Period;
		}
		else
		{
			s_Rtcc_Timer_Array[i] = 0;
		}
	}
}
/*******************************************************************************
* @fun_name     RTC_ClkSource
* @brief        返回rtcc时钟源:0:使用外部低速时钟,1:使用内部低速时钟
*******************************************************************************/
u8 RTC_ClkSource(void)
{
    return rtcc_clk_source;
}


#ifdef __cplusplus
}
#endif

/***************************************************************END OF FILE****/
