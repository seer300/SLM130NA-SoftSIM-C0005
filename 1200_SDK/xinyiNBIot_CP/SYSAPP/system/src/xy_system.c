/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_at_api.h"
#include "csp.h"
#include "at_ctl.h"
#include "low_power.h"
#include "xy_rtc_api.h"
#include "prcm.h"
#include "softap_nv.h"
#include "xy_memmap.h"
#include "xy_ps_api.h"
#include "ipc_msg.h"
#include "oss_nv.h"
#include "at_uart.h"
#include "xy_net_api.h"
#include "adc.h"
#include "xy_system.h"
#include "rc32k_cali.h"
#include "net_app_resume.h"

/*******************************************************************************
 *								MARCO definitions							   *
 ******************************************************************************/
#define WAIT_CTZEU_MS (2 * 1000)		//PDP激活后等待世界时间OK的延时

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*执行CFUN=5本地软关机，以快速深睡*/
int g_fast_off = 0;

uint64_t g_sys_start_time = 0;
int g_soft_reset_no_exc_cfun5 = 0;

/*由于软重启期间flash/RF等硬件资源可能在运行，进而走普通软件流程来保证硬件状态*/
void xy_Soft_Reset(Soft_Reset_Type soft_rst_reason)
{
	/*部分AT命令需要输出"RESETING""REBOOTING",此处确保通过AT口输出完成*/
	at_uart_wait_send_done();
	
	/*RF校准完成后触发的NRB不能通知PS执行CFUN5*/
	if(g_orig_RF_mode == 0 && g_soft_reset_no_exc_cfun5 == 0)
		xy_cfun_excute(NET_CFUN5);

	// 等待DMA Channel 0-3 空闲 ,保证当前擦写一次流程可以正常跑完
    extern osThreadId_t g_flash_TskHandle;
    volatile eTaskState stat = eBlocked;

	/*osDelay会释放调度，进而需要锁睡眠*/
	app_delay_lock(1000);
	
	do{
		if(g_flash_TskHandle)
			stat = eTaskGetState((TaskHandle_t)g_flash_TskHandle);
		if((DMAChannelGetStartStatus(FLASH_DMA_CHANNEL) && DMAChannelTransferRemainCNT(FLASH_DMA_CHANNEL)) \
				|| stat == eRunning || stat == eReady)
		{
			osDelay(50);
		}
		else
		{
			break;
		}
	}while(1);

	shm_msg_write(&soft_rst_reason, 4, ICM_SOFT_RESET);

	while(1);
}

void xy_fast_power_off()
{
	xy_printf(0,PLATFORM, WARN_LOG, "FASTOFF:AT+CFUN=5");

	g_fast_off = 1;
	clear_sleep_lock(LPM_ALL);

	/*fastoff 刪除云业务会话文件防止误恢复*/
	cloud_remove_file(ONENET_SESSION_NVM_FILE_NAME);
	cloud_remove_file(CTLW_SESSION_FILE_UNDER_DIR);

	/*通知PS执行软关机，以加快调度出深睡时长*/
	xy_cfun_excute(NET_CFUN5);
}


bool Is_UP_From_FastRecvry()
{
	return (HWREG(DEEPSLEEP_CP_FASTRECOVERY_FLAG) == 0x43);
}


typedef enum
{
	FORCE_STOP_CP_NONE = 0,	//默认值
	FORCE_STOP_CP_REQ = 0x5A,	//AP发送的停止CP信号
	FORCE_STOP_CP_ACK = 0xA5,	//CP返回AP应答
}STOP_CP_HANDSHAKE;

__RAM_FUNC void check_force_stop_cp(void)
{
	//检查AP force stop CP标志，应答后执行while1
	if(HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) == FORCE_STOP_CP_REQ)
	{
		vLpmEnterCritical();//wakeup不是最高优先级的中断，需锁中断
		HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) = FORCE_STOP_CP_ACK;
		while(1);
	}
}

uint16_t xy_getVbat()
{
	return get_adc_value_incpcore(ADC_VBAT);
}

int16_t xy_getempera()
{
	return  get_adc_value_incpcore(ADC_TSENSOR);
}

/*获取RTC的count的ms数，不是严格的ms精度.仅限底层使用，不能用xy_gmtime_r进行转换*/
uint64_t get_utc_tick()
{
	uint64_t utc_cnt = rtc_get_cnt();

	return (utc_cnt/32);
}

/*仅供云业务开发使用，获取UTC中的毫秒数，只能用于差值计算，不能直接转化为世界时间*/
uint64_t get_utc_ms()
{
	uint64_t ret = 0;

	ret = ((uint64_t)(get_utc_tick())*32000ULL/g_32k_default_freq);

	/*构造随机偏差，以模拟RC32精度不足的问题*/
	if(HWREGB(BAK_MEM_XY_DUMP) == 1 && HWREGB(BAK_MEM_32K_CLK_SRC) == 0)
	{
		int  delta = 0;
		static uint64_t pre_sec = 0;

		ret = (ret/1000);

		if(pre_sec != 0)
		{
			delta = ret - pre_sec;

			/*10分钟以上构造1分钟误差*/
			if(delta > 10*60)
				delta = 2*60;
			/*1-10分钟内构造4秒误差*/
			else if(delta > 60)
				delta = 8;
			/*小于1分钟无误差*/
			else
				delta = 1;
			
			delta = xy_rand()%delta - (delta/2);	
		}
		
		ret = (ret + delta);
		pre_sec = ret;
		if(ret < pre_sec+1)
			ret = pre_sec+1;

		ret = ret*1000;
	}
		
	return ret;
}

uint64_t xy_get_UT_ms()
{
	uint64_t ut_ms = get_cur_UT_ms();
	return ut_ms;
}

int get_Soc_ver()
{
	return (int)HWREGB(BAK_MEM_SOC_VER);
}

void xy_mutex_acquire(osMutexId_t mutex_id, uint32_t timeout)
{
	if(osKernelGetState() >= osKernelRunning && osCoreGetState() == osCoreNormal)
		osMutexAcquire(mutex_id, timeout);
}

void xy_mutex_release(osMutexId_t mutex_id)
{
	if(osKernelGetState() >= osKernelRunning && osCoreGetState() == osCoreNormal)
		osMutexRelease(mutex_id);
}

//section初始化链表的方式运行软件（指定主函数函数的调用区域，实现后，主函数内客户函数仅是一个寻址，链接调用
void app_section_init_start()
{
    extern uint8_t *__appRegTable_start__; //定义在sections.ld文件
    extern uint8_t *__appRegTable_end__; //定义在sections.ld文件
    appRegItem_t *appRegTable_start = (appRegItem_t *)&__appRegTable_start__;
    appRegItem_t *appRegTable_end = (appRegItem_t *)&__appRegTable_end__;

    appRegItem_t *cur = appRegTable_start;
    while (cur < appRegTable_end)
    {
        cur->app_init_entry();
        cur += 1;
    }
}


#if RUNTIME_DEBUG
#include "hw_timer.h"
#include "timer.h"
typedef struct
{
	uint32_t rtc_record_timer;
}rtc_record_t;

typedef enum
{
	RTC_RECORD_FLASH_WRITE = RC32K_IRQn + 1,
	RTC_RECORD_FLASH_ERASE,
	RTC_RECORD_FLASH_ERASE1,
	RTC_RECORD_FLASH_PROTECT_EN,
	RTC_RECORD_FLASH_PROTECT_DIS,
	RTC_RECORD_MAX,
};

rtc_record_t rtc_record[RTC_RECORD_MAX] = {0};

void xy_runtime_init(void)
{
	PRCM_ClockEnable(CORE_CKG_CTL_TMR3_EN);

	TimerDisable(TIMER3_BASE);
	TimerInitCountValueSet(TIMER3_BASE, 0xfffffff3);
	TimerReloadValueSet(TIMER3_BASE, 0xffffffff);
	TimerConfigure(TIMER3_BASE, TIMER_CTL_TPOL_TRUE | TIMER_CTL_PRES_DIVIDE_128| TIMER_CTL_TMODE_COMPARE);

	TimerEnable(TIMER3_BASE);
}

__RAM_FUNC uint32_t xy_runtime_get_enter(void)
{
	return TimerCountValueGet(TIMER3_BASE);
}

__RAM_FUNC void xy_runtime_get_exit(uint32_t id, uint32_t time_enter)
{
	uint32_t time_exit = TimerCountValueGet(TIMER3_BASE);
	uint32_t time_delta = 0;
	if(time_exit < time_enter)
		time_delta = 0xffffffff - (time_enter - time_exit - 1);
	else
		time_delta = time_exit - time_enter;

	if(rtc_record[id].rtc_record_timer < time_delta)
		rtc_record[id].rtc_record_timer = time_delta;
}

void xy_runtime_debug(void)
{
	xy_printf(0,PLATFORM, DEBUG_LOG, "xy_runtime_debug freq 720000:PHYTMR_IRQn %d,TIM4_IRQn %d,DMAC1_IRQn %d,FLASH_WRITE %d,FLASH_ERASE %d,FLASH_ERASE1 %d,FLASH_PROTECT_EN %d,FLASH_PROTECT_DIS %d,MCNT_IRQn %d,UTC_IRQn %d,WAKEUP_IRQn %d,ISO7816_IRQn %d",  \
			 rtc_record[PHYTMR_IRQn].rtc_record_timer, rtc_record[TIM4_IRQn].rtc_record_timer, rtc_record[DMAC1_IRQn].rtc_record_timer,\
			 rtc_record[RTC_RECORD_FLASH_WRITE].rtc_record_timer, rtc_record[RTC_RECORD_FLASH_ERASE].rtc_record_timer, rtc_record[RTC_RECORD_FLASH_ERASE1].rtc_record_timer, \
			 rtc_record[RTC_RECORD_FLASH_PROTECT_EN].rtc_record_timer, rtc_record[RTC_RECORD_FLASH_PROTECT_DIS].rtc_record_timer,rtc_record[MCNT_IRQn].rtc_record_timer,\
			 rtc_record[UTC_IRQn].rtc_record_timer, rtc_record[WAKEUP_IRQn].rtc_record_timer, rtc_record[ISO7816_IRQn].rtc_record_timer);
}
#endif
