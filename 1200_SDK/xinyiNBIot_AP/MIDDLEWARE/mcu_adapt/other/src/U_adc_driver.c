/*******************************************************************************
* @Copyright (c)    :(C)2020, Qingdao ieslab Co., Ltd
* @FileName         :hc32_adc_driver.c
* @Author           :Kv-L
* @Version          :V1.0
* @Date             :2020-07-01 17:51:43
* @Description      :the function of the entity of ADC
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "U_adc_driver.h"
#include "U_timer1uS_driver.h"
#include "adc_adapt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private variables ---------------------------------------------------------*/
static u8 s_adc_stat = ADC_IDLE;			//ADC任务机状态
static u8 s_Adc_Status = ADC_OK;			//ADC状态
static u32 *s_Adc_Conv_Result_Addr;			//ADC存放转化结果的指针
static u32 s_Adc_Time_Interval;			    //转换间隔时间
static u8 *s_Adc_Return_Msg;				//ADC存放返回状态的指针
static u8 s_Adc_Conv_Channel_Num = 0;		//ADC已转换次数
static u32 AdcTargetConvResult[3] = {0};	//ADC目标通道转换值：MIN;SUM;MAX
static ADC_CHANNEL_TypeDef s_Adc_Target_Channel = ADC_VBAT;		//ADC目标转换通道
static u8 s_Adc_Conv_Target_Num = 0;		//ADC目标转换次数
static u32 s_ADC_Sam_TimeInterval = 0;// ADC采样间隔
static u32 s_ADC_Sam_Timeout = 0;// ADC采样超时


/* Private function prototypes -----------------------------------------------*/
static void My_GPIO_Init(void);
static void My_Adc_Init(void);


/*******************************************************************************
* @fun_name     void AdcStartConvert(u8 AdcChannel, u32 ConvNum, u32 *ConvResult, u8 *ConvIfDone)
* @brief        ADC启动接口
* @param[in]    u8 AdcChannel:ADC channel
*               u32 ConvNum:convert num; 
*				u32 TimeInterval,    单位ms
*               u32 *ConvResult:result point
*               u8 *ConvIfDone:convert states point
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC void AdcStartConvert(u8 AdcChannel, u32 ConvNum, u32 TimeInterval, u32 *ConvResult, u8 *ConvIfDone)
{
	if(ConvNum < MINCONVNUM) //至少转换三次
    {
		ConvNum = MINCONVNUM;
    }
    if (ConvNum > MAXCONVNUM)   //不能超过定义的最大转换次数
    {
        ConvNum = MAXCONVNUM;
    }
	
	s_Adc_Return_Msg = ConvIfDone;			
	if(AdcChannel > 25)
	{
		s_Adc_Status |= ADC_START_ERROR; //启动失败
		*s_Adc_Return_Msg = TRUE; //置完成
		return;
	}

	s_Adc_Conv_Result_Addr = ConvResult;
	*s_Adc_Return_Msg = FALSE; //清完成标志
	s_Adc_Conv_Channel_Num = 0; //清转换次数
	AdcTargetConvResult[SUM] = 0; //清buf
	AdcTargetConvResult[MIN] = 0;
	AdcTargetConvResult[MAX] = 0;
	s_Adc_Status = ADC_OK; // 清ADC状态
	s_Adc_Target_Channel = (ADC_CHANNEL_TypeDef)AdcChannel;
	s_Adc_Conv_Target_Num = ConvNum;
	s_Adc_Time_Interval = TimeInterval;///5;   //转换为5ms定时器
	Bgr_BgrEnable(); //使能BGR模块
	Set5msTimer(TIMER_5MS_ADC,1);//
	s_adc_stat = ADC_CFGADC;
}

/*******************************************************************************
* @fun_name     AdcMachineDriver
* @brief        ADC主任务机
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC void AdcMachineDriver(void)
{
    static int16_t adc_value = 0;
	switch(s_adc_stat)
    {
        case ADC_IDLE:       
		break;       
             
		case ADC_CFGADC:
        {
			if(0 == Check5msTimer(TIMER_5MS_ADC)) //延时5ms，等待ADC模块及BGR模块启动完成
            {
			    My_Adc_Init();
				s_adc_stat = ADC_START;               
            }            
        }
		break;
		
        case ADC_START:
        {
            s_ADC_Sam_TimeInterval = Timer1usGetTick();
            s_ADC_Sam_Timeout = Timer1usGetTick();
            adc_value = ADC_Single_GetValue(s_Adc_Target_Channel);

            //Set5msTimer(TIMER_5MS_ADC,s_Adc_Time_Interval);//转换间隔
			//Set5msTimer(TIMER_5MS_ADC_OVERTIME,10);//50ms超时
			s_adc_stat = ADC_CONV;

        }
		break;
		
        case ADC_CONV:
        {
			// if(TRUE == Adc_GetIrqStatus(AdcMskIrqSgl)) //单次转换完成
			if(1) //单次转换完成
            {
				
                //Set5msTimer(TIMER_5MS_ADC_OVERTIME,10);//50ms超时
				//if(0 == Check5msTimer(TIMER_5MS_ADC))  //转换间隔
                s_ADC_Sam_Timeout = Timer1usGetTick();//重新进行计超时时间
                if(Timer1usGetTick() - s_ADC_Sam_TimeInterval >= s_Adc_Time_Interval)
				{
					u32 tmp_value = 0;
           
					// tmp_value = Adc_GetSglResult();   //16.5.15
					tmp_value = adc_value;   //16.5.15

					AdcTargetConvResult[SUM] += tmp_value;
					if(0 == s_Adc_Conv_Channel_Num)
					{
						AdcTargetConvResult[MAX] = tmp_value;
						AdcTargetConvResult[MIN] = tmp_value;
					}
					else
					{
						AdcTargetConvResult[MAX] = ((tmp_value > AdcTargetConvResult[MAX]) ? tmp_value : AdcTargetConvResult[MAX]);
						AdcTargetConvResult[MIN] = ((tmp_value < AdcTargetConvResult[MIN]) ? tmp_value : AdcTargetConvResult[MIN]);
					}
					if(++s_Adc_Conv_Channel_Num >= s_Adc_Conv_Target_Num) //到达设定转换次数
					{
						u32 tmp_target = 0;
						tmp_target = AdcTargetConvResult[SUM] - AdcTargetConvResult[MIN] - AdcTargetConvResult[MAX];
						*s_Adc_Conv_Result_Addr = tmp_target / (s_Adc_Conv_Target_Num - 2);
						s_adc_stat = ADC_END;
					}
					else
					{
						s_adc_stat = ADC_START;
					}
				}
			}
			else
			{
                if(Timer1usGetTick() - s_ADC_Sam_Timeout > ADC_TIMEOUT)//AD转换超时
				{
					// Adc_SGL_Stop(); //停止AD转换
					s_Adc_Status |= ADC_CONV_TIMEOUT; //转换超时
					s_adc_stat = ADC_END;
				}
			}		
        }
		break;
		
		case ADC_END:
        {
			Bgr_BgrDisable(); //关闭BGR模块
            PRCM_ClockDisable(CORE_CKG_CTL_ADC_EN);//关闭ADC时钟
			*s_Adc_Return_Msg = TRUE; //置完成
			s_adc_stat = ADC_IDLE;            
        }
		break;
		
        default:
        break;        
    }
}

/*******************************************************************************
* @fun_name     u8 AdcIfIdle(void)
* @brief        查询ADC任务机是否空闲接口
* @param[in]    None
* @param[out]   u8 :TASK_IDLE | TASK_BUSY
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC u8 AdcIfIdle(void)
{
	if (ADC_IDLE == s_adc_stat)
    {
		return TASK_IDLE;
	}
	else
	{
		return TASK_BUSY;
	}
}

/*******************************************************************************
* @fun_name     u8 AdcGetStatus(void)
* @brief        获取ADC状态
* @param[in]    None
* @param[out]   0:正常; Bit1:启动错误; Bit2:转换超时
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC u8 AdcGetStatus(void)
{
	return s_Adc_Status;
}
/*******************************************************************************
* @fun_name     AdcIfSleep
* @brief        查询ADC任务机是否允许休眠接口
* @param[in]    None
* @param[out]   u8 :TRUE | FALSE
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC u8 AdcIfSleep(void)
{
	u8 tmp_adc_if_sleep = 0;
	if (ADC_IDLE == s_adc_stat)
	{
		tmp_adc_if_sleep = TRUE;
	}
	else
	{
		tmp_adc_if_sleep = FALSE;
	}
	return tmp_adc_if_sleep;
}

/*******************************************************************************
* @fun_name     AdcPreSleep
* @brief        ADC休眠前处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC void AdcPreSleep(void)
{
	// Adc_Disable();
    Bgr_BgrDisable();
    PRCM_ClockDisable(CORE_CKG_CTL_ADC_EN);//关闭ADC时钟
}

/*******************************************************************************
* @fun_name     AdcWakeSleep
* @brief        ADC唤醒前处理
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC void AdcWakeSleep(void)
{
    ;//唤醒不开启ADC，使用前再开启。
}


/*****************************Private functions********************************/

/*******************************************************************************
* @fun_name     My_GPIO_Init
* @brief        GPIO初始化
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC static void My_GPIO_Init(void)
{     
}

/*******************************************************************************
* @fun_name     My_Adc_Init
* @brief        ADC初始化
* @param[in]    None
* @param[out]   None
* @retval       None
* @other        None
*******************************************************************************/
__RAM_FUNC static void My_Adc_Init(void)
{
 
}


#ifdef __cplusplus
}
#endif
/***************************************************************END OF FILE****/
