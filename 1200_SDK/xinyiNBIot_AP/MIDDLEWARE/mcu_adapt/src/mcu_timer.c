#include "mcu_adapt.h"

uint32_t g_timer1_freq_div = 0;
uint32_t g_timer2_freq_div = 0;

uint32_t g_lptimer1_freq_div = 0;
uint32_t g_lptimer2_freq_div = 0;

typedef struct 
{
    uint8_t timer_use_func;//指示timer的使用；0：未使用；1：定时器使用；2：pwm使用，频率入参方式；3：pwm使用，周期入参方式；
    uint8_t pwmduty;
    uint8_t padding0[2];
    uint32_t pwmclk;
    uint32_t pwmperiod_ms;
    MCU_GPIO_PinTypeDef pwmgpio;
} TimerUseConfigTypeDef;

static TimerUseConfigTypeDef hard_timer[2] = {0};

//tick数=时长*频率/分频
#define ConvertTick(ms, freq, freq_div) (uint32_t)((uint64_t)(ms) * (freq) / (freq_div) / 1000 - 1)
//=====================================================================================
//================================LPTIMER==============================================
//=====================================================================================
/************************************************************************************
* @brief  lptimer1周期配置。接口耗时1218.3us
* @param  timeout_ms ：超时时长，单位ms，由于LPTIMER特性（16位计数，参考时钟32K，最大分频128），最长超时时长约为256000ms
* @return 0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuLptimerSetPeriod(uint32_t timeout_ms)
{
    //使能lptimer时钟，获取lptimer时钟频率
	PRCM_ClockEnable(CORE_CKG_CTL_LPTMR_EN); 
    uint32_t lptimer_freq = Get32kClockFreq();

    //检验超时入参
	uint32_t base_max_time = (uint32_t)(0xFFFFUL + 1) * 1000 / lptimer_freq ;
    if((timeout_ms < 1) || (timeout_ms > base_max_time * (1 << 7)))
    {
    	debug_assert(0);
        return -1;
    }

    //计算分频系数Division
    uint32_t lptimer_freq_div, Division = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        lptimer_freq_div = (1 << i);
        if(timeout_ms <= (base_max_time * lptimer_freq_div))
        {
            Division = (i << LPTIMER_CTL_PRES_Pos);
            break;
        }
    }

    g_lptimer1_freq_div = lptimer_freq_div;

    //计算Reload值
    uint16_t Reload = Common_Convert_Ms_to_Count(uint16_t, timeout_ms, lptimer_freq, lptimer_freq_div);

    //切换时钟源为APB时钟以配置LPTIMER1
	LPTimerClockSrcMux(LPTIMER1_BASE, LPTIMER_CONFIG_CLK_MUX_APB_ONLY);
	while (!LPTimerClockStateGet(LPTIMER1_BASE, LPTIMER_CLK_APB_FLAG_Msk)){;}

	//lptimer配置
	LPTimerDisable(LPTIMER1_BASE);
	LPTimerInitCountValueSet(LPTIMER1_BASE, 0x0000);
	LPTimerReloadValueSet(LPTIMER1_BASE, Reload);
	LPTimerConfigure(LPTIMER1_BASE, LPTIMER_CTL_TMODE_CONTINUOUS | Division);

	//切换时钟源为32k时钟
	LPTimerClockSrcMux(LPTIMER1_BASE, LPTIMER_CONFIG_CLK_MUX_INTER_ONLY);
	while(!LPTimerClockStateGet(LPTIMER1_BASE, LPTIMER_CLK_INTER_FLAG_Msk));

	//配置lptimer中断模式
    LPTimerIntEnable(LPTIMER1_BASE, LPTIMER_CTL_TICONFIG_INNER_EVENT);

    //睡眠配置
    extern uint8_t g_lptim_used;
	g_lptim_used = 1;

#if (MODULE_VER == 0x0)	// opencpu,避免深睡前配置耗时
    PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //force on in deesleep,lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。
#endif

    return 0;
}

/************************************************************************************
* @brief  lptimer2周期配置。接口耗时1218.3us
* @param  timeout_ms ：周期时间，单位：ms
* @return 0：成功。 -1：失败，非法参数
************************************************************************************/
int8_t McuLptimer2SetPeriod(uint32_t timeout_ms) 
{
    //使能lptimer时钟，获取lptimer时钟频率
	PRCM_ClockEnable(CORE_CKG_CTL_LPTMR_EN); 
    uint32_t lptimer_freq = Get32kClockFreq();

    //检验超时入参
	uint32_t base_max_time = (uint32_t)(0xFFFFUL + 1) * 1000 / lptimer_freq ;
    if((timeout_ms < 1) || (timeout_ms > base_max_time * (1 << 7)))
    {
    	debug_assert(0);
        return -1;
    }

    //计算分频系数Division
    uint32_t lptimer_freq_div, Division = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        lptimer_freq_div = (1 << i);
        if(timeout_ms <= (base_max_time * lptimer_freq_div))
        {
            Division = (i << LPTIMER_CTL_PRES_Pos);
            break;
        }
    }

    g_lptimer2_freq_div = lptimer_freq_div;

    //计算Reload值
    uint16_t Reload = Common_Convert_Ms_to_Count(uint16_t, timeout_ms, lptimer_freq, lptimer_freq_div);

    //切换时钟源为APB时钟以配置LPTIMER1
	LPTimerClockSrcMux(LPTIMER2_BASE, LPTIMER_CONFIG_CLK_MUX_APB_ONLY);
	while (!LPTimerClockStateGet(LPTIMER2_BASE, LPTIMER_CLK_APB_FLAG_Msk)){;}

	//lptimer配置
	LPTimerDisable(LPTIMER2_BASE);
	LPTimerInitCountValueSet(LPTIMER2_BASE, 0x0000);
	LPTimerReloadValueSet(LPTIMER2_BASE, Reload);
	LPTimerConfigure(LPTIMER2_BASE, LPTIMER_CTL_TMODE_CONTINUOUS | Division);

	//切换时钟源为32k时钟
	LPTimerClockSrcMux(LPTIMER2_BASE, LPTIMER_CONFIG_CLK_MUX_INTER_ONLY);
	while(!LPTimerClockStateGet(LPTIMER2_BASE, LPTIMER_CLK_INTER_FLAG_Msk));

	//配置lptimer中断模式
    LPTimerIntEnable(LPTIMER2_BASE, LPTIMER_CTL_TICONFIG_INNER_EVENT);

    //睡眠配置
    extern uint8_t g_lptim_used;
	g_lptim_used = 1;

#if (MODULE_VER == 0x0)	// opencpu,避免深睡前配置耗时
    PRCM_LPUA_PWR_Ctl(LPUA_ANY_MODE_ON); //force on in deesleep,lpuart和lptim中有一个维持工作，则需要维持外设寄存器不掉电。
#endif

    return 0;
}

/***********************************************************************************
* @brief  获取lptimer1对应的ms数。接口耗时：us
* @param  NA
* @return  lptimer1对应的ms数
***********************************************************************************/
uint32_t McuLptimerGetCountMs()
{
    return Common_Convert_Count_to_Ms(uint32_t,TimerCountValueGet(LPTIMER1_BASE), Get32kClockFreq(), g_lptimer1_freq_div);
}

/***********************************************************************************
* @brief  获取lptimer2对应的ms数。接口耗时：us
* @param  NA
* @return  lptimer2对应的ms数
***********************************************************************************/
uint32_t McuLptimer2GetCountMs()
{
    return Common_Convert_Count_to_Ms(uint32_t,TimerCountValueGet(LPTIMER2_BASE), Get32kClockFreq(), g_lptimer2_freq_div);
}

/************************************************************************************
* @brief   lptimer1使能。接口耗时6.1us
* @param   NA
* @return  NA
************************************************************************************/
void McuLptimerEn(void)
{
    LPTimerEnable(LPTIMER1_BASE);
}

/************************************************************************************
* @brief   lptimer2使能。接口耗时6.1us
* @param   NA
* @return  NA
************************************************************************************/
void McuLptimer2En(void)
{
    LPTimerEnable(LPTIMER2_BASE);
}

/************************************************************************************
* @brief   lptimer1禁能。接口耗时6.3us
* @param   NA
* @return  NA
************************************************************************************/
void McuLptimerDis(void)
{
    LPTimerDisable(LPTIMER1_BASE);
}

/************************************************************************************
* @brief   lptimer2禁能。接口耗时6.3us
* @param   NA
* @return  NA
************************************************************************************/
void McuLptimer2Dis(void)
{
    LPTimerDisable(LPTIMER2_BASE);
}

/************************************************************************************
* @brief   lptimer中断服务函数
* @param   NA
* @return  NA
************************************************************************************/
pFunType_void lptimer1_irq = NULL;
pFunType_void lptimer2_irq = NULL;
__RAM_FUNC static void LptimerIrqHandle(void)
{
	uint8_t lptimer_intsta = LPTimerIntStatus(); //读清lptimer中断标志位
    if((lptimer_intsta & LPTIMER_INT_LPTIMER1_Msk) && (lptimer1_irq != NULL))
    {
        lptimer1_irq();
    }
    if((lptimer_intsta & LPTIMER_INT_LPTIMER2_Msk) && (lptimer2_irq != NULL))
    {
        lptimer2_irq();
    }
}

/************************************************************************************
* @brief  lptimer1中断函数注册。接口耗时11.4us
* @param  p_fun：中断回调函数
* @return  NA
************************************************************************************/
void McuLptimerIrqReg(pFunType_void p_fun)
{
    NVIC_IntRegister(LPTIM_IRQn, LptimerIrqHandle, 1);
    lptimer1_irq = p_fun;
    mark_dyn_addr(&lptimer1_irq);
}

/************************************************************************************
* @brief  lptimer2中断函数注册。接口耗时11.4us
* @param  p_fun：中断回调函数
* @return  NA
************************************************************************************/
void McuLptimer2IrqReg(pFunType_void p_fun)
{
    NVIC_IntRegister(LPTIM_IRQn, LptimerIrqHandle, 1);
    lptimer2_irq = p_fun;
    mark_dyn_addr(&lptimer2_irq);
}

//=====================================================================================
//================================TIMER================================================
//=====================================================================================
/***********************************************************************************
* @brief  timer周期配置。接口耗时：num为1耗时230.5us，num为2耗时211.5us
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @param  timeout_ms ：周期时间，单位：ms
* @return 0：成功；-1：失败，非法参数
***********************************************************************************/
int8_t McuTimerSetPeriod(uint8_t num, uint32_t timeout_ms)
{
    //使能timer时钟，获取timer时钟频率
    uint32_t timer_freq, Instance;

    hard_timer[num-1].timer_use_func = 1;

    switch(num)
	{
		case 1:
        { 
            PRCM_ClockEnable(CORE_CKG_CTL_TMR1_EN);
            timer_freq = GetPCLK1Freq(); 
            Instance = TIMER1_BASE;
            break; 
        }
		case 2:
        { 
            PRCM_ClockEnable(CORE_CKG_CTL_TMR2_EN); 
            timer_freq = GetlsioFreq(); 
            Instance = TIMER2_BASE;
            break; 
        }
        default: 
        {
			debug_assert(0);
			return -1; //入参非法
        }
    }

    //检验超时入参
    uint64_t base_max_time = (uint64_t)(0xFFFFFFFFULL + 1)  * 1000 / timer_freq;
    if((timeout_ms < 1) || (timeout_ms > base_max_time * (1 << 7)))
    {
		debug_assert(0);
		return -1; //入参非法
    }

    //计算分频系数Division
    uint32_t timer_freq_div, Division = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        timer_freq_div = (1 << i);
        if(timeout_ms <= (base_max_time * timer_freq_div))
        {
            Division = (i << TIMER_CTL_PRES_Pos); 
            break;
        }
    }

    if(num == 1)
    {
        g_timer1_freq_div = timer_freq_div;
    }
    else
    {
        g_timer2_freq_div = timer_freq_div;
    }

    //计算Reload值
    uint32_t Reload = Common_Convert_Ms_to_Count(uint32_t, timeout_ms, timer_freq, timer_freq_div);

    //lptimer配置
	TimerDisable(Instance);
	TimerConfigure(Instance, TIMER_CTL_TMODE_CONTINUOUS | Division);
	TimerInitCountValueSet(Instance, 0x00000000);
	TimerReloadValueSet(Instance, Reload);

    //配置lptimer中断模式
    TimerIntEnable(Instance,TIMER_CTL_TICONFIG_INNER_EVENT);
        
    return 0;
}

/***********************************************************************************
* @brief  获取timer对应的us数。接口耗时：num为1耗时us，num为2耗时us
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @return  timer对应的us数
***********************************************************************************/
uint32_t McuTimerGetCountUs(uint8_t num)
{
   //使能timer时钟，获取timer时钟频率
    uint32_t timer_freq, Instance;
    uint32_t timer_freq_div;
    switch(num)
	{
		case 1:
        { 
            timer_freq = GetPCLK1Freq(); 
            Instance = TIMER1_BASE;
            timer_freq_div = g_timer1_freq_div;
            break; 
        }
		case 2:
        { 
            timer_freq = GetlsioFreq(); 
            Instance = TIMER2_BASE;
            timer_freq_div = g_timer2_freq_div;
            break; 
        }
        default: 
        {
			debug_assert(0);
			return -1; //入参非法
        }
    }

    return Common_Convert_Count_to_Us(uint32_t,TimerCountValueGet(Instance), timer_freq, timer_freq_div);
}

/***********************************************************************************
* @brief  timer使能。接口耗时：num为1耗时4.8us，num为2耗时6.6us
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @return  NA
***********************************************************************************/
void McuTimerEn(uint8_t num)
{
    if((hard_timer[num-1].timer_use_func == 2) && (hard_timer[num-1].pwmduty == 100))
    {
        return;
    }
    
    switch(num)
	{
		case 1: { TimerEnable(TIMER1_BASE); break; }
		case 2: { TimerEnable(TIMER2_BASE); break; }
        default:;
    }
}

/***********************************************************************************
* @brief  timer禁能。接口耗时：num为1耗时4.8us，num为2耗时6.6us
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @return  NA
***********************************************************************************/
void McuTimerDis(uint8_t num)
{
    if((hard_timer[num-1].timer_use_func == 2) && (hard_timer[num-1].pwmduty == 100))
    {
        return;
    }
    
    switch(num)
	{
		case 1: { TimerDisable(TIMER1_BASE); break; }
		case 2: { TimerDisable(TIMER2_BASE); break; }
        default:;
    }
}

/************************************************************************************
* @brief   timer1/2中断服务函数
* @param   NA
* @return  NA
************************************************************************************/
#define MCU_TIMER_NUM (2)
pFunType_void timer_irq[MCU_TIMER_NUM] = {NULL};
__RAM_FUNC static void Timer1IrqHandle(void)
{
	//timer1产生中断时，不需要手动清中断标志位，硬件会自动清除
    if(timer_irq[0] != NULL)
    {
        timer_irq[0]();
    }
}

__RAM_FUNC static void Timer2IrqHandle(void)
{
	//timer2产生中断时，不需要手动清中断标志位，硬件会自动清除
    if(timer_irq[1] != NULL)
    {
        timer_irq[1]();
    }
}

/************************************************************************************
* @brief  timer中断函数注册。接口耗时11.4us
* @param  p_fun：中断回调函数
* @return  NA
************************************************************************************/
void McuTimerIrqReg(uint8_t num, pFunType_void p_fun)
{
    switch(num)
    {
        case 1: { NVIC_IntRegister(TIM1_IRQn, Timer1IrqHandle, 1); break; }
        case 2: { NVIC_IntRegister(TIM2_IRQn, Timer2IrqHandle, 1); break; }
        default:;
    }
    timer_irq[num - 1] = p_fun;
    mark_dyn_addr(&timer_irq[num - 1]);
}

/***********************************************************************************
* @brief  timer周期配置。
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @param  pwmperiod_ms ：PWM周期，单位：ms
* @param  pwmduty：pwm波占空比,取值0-100
* @param  pwmgpio：pwm波输出引脚,取值（0-63）
* @return 0：成功；-1：失败，非法参数
***********************************************************************************/
__FLASH_FUNC int8_t McuTimerSetPWM2(uint8_t num, uint32_t pwmperiod_ms, uint8_t pwmduty, MCU_GPIO_PinTypeDef pwmgpio)
{
    hard_timer[num-1].timer_use_func = 3;
    hard_timer[num-1].pwmduty = pwmduty;
    hard_timer[num-1].pwmclk = pwmperiod_ms;
    hard_timer[num-1].pwmgpio = pwmgpio;

    if(pwmduty == 100)
    {
        McuGpioModeSet(pwmgpio,0x00);
        McuGpioWrite(pwmgpio,0x01);
        return 0; 
    }
    else
    {
        //使能timer时钟，获取timer时钟频率，配置OUTP、OUTN
        uint32_t timer_freq, Instance;
        GPIO_InitTypeDef pwm_gpio = {0};
        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);
        switch(num)
        {
            case 1:
            { 
                PRCM_ClockEnable(CORE_CKG_CTL_TMR1_EN);
                timer_freq = GetPCLK1Freq(); 
                Instance = TIMER1_BASE;
                pwm_gpio.PinRemap = GPIO_TMR1PWM_OUTP; 
                break; 
            }
            case 2:
            { 
                PRCM_ClockEnable(CORE_CKG_CTL_TMR2_EN); 
                timer_freq = GetlsioFreq(); 
                Instance = TIMER2_BASE;
                pwm_gpio.PinRemap = GPIO_TMR2PWM_OUTP;
                break; 
            }
            default: return -1;
        }
        pwm_gpio.Pin = pwmgpio;
        pwm_gpio.Mode = GPIO_MODE_HW_PER;
        GPIO_Init(&pwm_gpio, NORMAL_SPEED);

        //检验超时入参
        uint64_t base_max_time = (uint64_t)(0xFFFFFFFFULL + 1) * 1000 / timer_freq;
        if((pwmperiod_ms < 1) || (pwmperiod_ms > base_max_time * (1 << 7)))
        {
            return -1;
        }

        //计算分频系数Division
        uint32_t timer_freq_div, Division = 0;
        for (uint32_t i = 0; i < 8; i++)
        {
            timer_freq_div = (1 << i);
            if(pwmperiod_ms <= (base_max_time * timer_freq_div))
            {
                Division = (i << TIMER_CTL_PRES_Pos);
                break;
            }
        }

        //计算Reload值
        uint32_t Reload = ConvertTick(pwmperiod_ms, timer_freq, timer_freq_div);

        //timer配置
        TimerDisable(Instance);
        TimerConfigure(Instance, TIMER_CTL_TMODE_PWM_SINGLE | Division | TIMER_CTL_TPOL_FALSE); //当timer关闭时OUTP输出低
        TimerInitCountValueSet(Instance, 0x00000000);
        TimerReloadValueSet(Instance, Reload);
        TimerPWMValueSet(Instance, ((uint64_t)Reload * (100 - pwmduty)) / 100);

        //配置timer中断模式
        //单PWM模式不需要产生中断，所以TIMER中断类型选择外部事件触发，保证RELOAD时不会产生中断，
        TimerIntEnable(Instance,TIMER_CTL_TICONFIG_OUTER_EVENT);

        return 0;
    }
}

/***********************************************************************************
* @brief  带引脚配置的的PWM配置
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @param  pwmclk ：PWM频率，单位：Hz
* @param  pwmduty：pwm波占空比,取值0-100
* @param  pwmgpio：pwm波输出引脚,取值（0-63）
* @return 0：成功；-1：失败，非法参数
***********************************************************************************/
int8_t McuTimerSetPWM3(uint8_t num, uint32_t pwmclk, uint8_t pwmduty, MCU_GPIO_PinTypeDef pwmgpio)
{
    hard_timer[num-1].timer_use_func = 2;
    hard_timer[num-1].pwmduty = pwmduty;
    hard_timer[num-1].pwmclk = pwmclk;
    hard_timer[num-1].pwmgpio = pwmgpio;

    if(pwmduty == 100)
    {
        McuGpioModeSet(pwmgpio,0x00);
        McuGpioWrite(pwmgpio,0x01);
        return 0; 
    }
    else
    {
        //使能timer时钟，获取timer时钟频率，配置OUTP、OUTN
        uint32_t timer_freq, Instance;
        GPIO_InitTypeDef pwm_gpio = {0};

        PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

        switch(num)
        {
            case 1:
            { 
                PRCM_ClockEnable(CORE_CKG_CTL_TMR1_EN);
                timer_freq = GetPCLK1Freq(); 
                Instance = TIMER1_BASE;
                pwm_gpio.PinRemap = GPIO_TMR1PWM_OUTP; 
                break; 
            }
            case 2:
            { 
                PRCM_ClockEnable(CORE_CKG_CTL_TMR2_EN); 
                timer_freq = GetlsioFreq(); 
                Instance = TIMER2_BASE;
                pwm_gpio.PinRemap = GPIO_TMR2PWM_OUTP;
                break; 
            }
            default: 
            {
			    debug_assert(0);
			    return -1; //入参非法
            }
        }
        pwm_gpio.Pin = pwmgpio;
        pwm_gpio.Mode = GPIO_MODE_HW_PER;
        GPIO_Init(&pwm_gpio, NORMAL_SPEED);

        //检验超时入参
        if(pwmclk > timer_freq)
        {
			debug_assert(0);
            return -1;
        }

        //计算分频系数Division
        uint32_t timer_freq_div, Division = 0;
        for (uint32_t i = 0; i < 8; i++)
        {
            timer_freq_div = (1 << i);
            if(pwmclk <= (timer_freq / timer_freq_div))
            {
                Division = (i << TIMER_CTL_PRES_Pos);
                break;
            }
        }

        if(num == 1)
        {
            g_timer1_freq_div = timer_freq_div;
        }
        else
        {
            g_timer2_freq_div = timer_freq_div;
        }

        //计算Reload值
        uint32_t Reload = timer_freq/timer_freq_div/pwmclk;

        //timer配置
        TimerDisable(Instance);
        TimerConfigure(Instance, TIMER_CTL_TMODE_PWM_SINGLE | Division | TIMER_CTL_TPOL_FALSE); //当timer关闭时OUTP输出低
        TimerInitCountValueSet(Instance, 0x00000000);
        TimerReloadValueSet(Instance, Reload);
        TimerPWMValueSet(Instance, ((uint64_t)Reload * (100 - pwmduty) / 100));

        //配置timer中断模式
        //单PWM模式不需要产生中断，所以TIMER中断类型选择外部事件触发，保证RELOAD时不会产生中断，
        TimerIntEnable(Instance,TIMER_CTL_TICONFIG_OUTER_EVENT);

        return 0;        
    }

}

/***********************************************************************************
* @brief  PWM配置
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @param  pwmclk ：PWM频率，单位：Hz
* @param  pwmduty：pwm波占空比,取值0-100
* @return 0：成功；-1：失败，非法参数
***********************************************************************************/
int8_t McuTimerSetPWM(uint8_t num, uint32_t pwmclk, uint8_t pwmduty)
{
    return McuTimerSetPWM3(num, pwmclk, pwmduty, MCU_GPIO53);
}

/***********************************************************************************
* @brief  PWM占空比配置
* @param  num ：timer硬件号，可选1-2，分别对应TIMER1/TIMER2
* @param  pwmduty：pwm波占空比,取值0-100
* @return 无
***********************************************************************************/
void McuTimerSetPWMCMP(uint8_t num, uint8_t pwmduty)
{
    if(hard_timer[num-1].pwmduty == pwmduty)
    {
        return;
    }

    if((hard_timer[num-1].timer_use_func != 2) && (hard_timer[num-1].timer_use_func != 3))//仅当做PWM使用的时候能调用此接口
    {
        return;
    }

    if((hard_timer[num-1].pwmduty == 100) || (pwmduty == 100))
    {
        if(hard_timer[num-1].timer_use_func == 2)
        {
            McuTimerSetPWM3(num,hard_timer[num-1].pwmclk,pwmduty,hard_timer[num-1].pwmgpio);
            McuTimerEn(num);
        }
        else//==3
        {
            McuTimerSetPWM2(num,hard_timer[num-1].pwmperiod_ms,pwmduty,hard_timer[num-1].pwmgpio);
            McuTimerEn(num);
        }

    }
    else
    {
        hard_timer[num-1].pwmduty = pwmduty;

        switch(num)
        {
            case 1: { TimerPWMValueSet(TIMER1_BASE, ((uint64_t)TMR1->RELOAD * (100 - pwmduty) / 100)); break; }
            case 2: { TimerPWMValueSet(TIMER2_BASE, ((uint64_t)TMR2->RELOAD * (100 - pwmduty) / 100)); break; }
            default:;
        }
    }
}
