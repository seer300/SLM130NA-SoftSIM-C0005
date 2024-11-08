#include "xy_system.h"
#include "at_uart.h"
#include "xy_timer.h"
#include "xy_lpm.h"

/*************************************************************************************************************************
当串口波特率大于9600时，一些动作如下：
1.深睡或STANDBY睡眠时，任何AT命令都可以唤醒芯片，但不会有任何动作，因为这条命令脏乱。参见drop_dirty_at_from_sleep
2.外部触发的深睡唤醒，如AT命令、PIN中断等，会延迟NV参数deepsleep_delay所代表的秒数才容许再次进入深睡，具体实现参见CP核的xy_delaylock_active调用点
3.外部AT命令触发的STANDBY唤醒，会延迟NV参数standby_delay所代表的秒数才容许再次进入STANDBY，具体实现参见at_uart_standby_ctl调用点
4.工作过程中收到AT命令，重设深睡和STANDBY睡眠的延迟定时器，保证一段时间内不会再进睡眠
5.AT命令动态修改波特率，当超过9600时，目前是关闭STANDBY，用户可自行修改，参见set_standby_by_rate接口
*************************************************************************************************************************/

#if (AT_LPUART == 1)


extern HAL_LPUART_HandleTypeDef g_at_lpuart;

extern char g_at_RecvBuffer[];

/*指示是否为唤醒深睡或STANDBY的AT命令*/
uint8_t g_wakup_at = 0;

//波特率大于9600时，AT唤醒深睡或STANDBY，首条命令丢失不报错，第二条命令正常执行
__OPENCPU_FUNC int drop_dirty_at_from_sleep(void)
{
	/*小于等于9600，唤醒AT不会脏乱或缺失*/
	if((GET_BAUDRATE() <= 9600))
	{
		g_wakup_at = 0;
		return 0;
	}
	else
	{
		/*第一条唤醒AT，若高于9600波特率，丢弃*/
		if(g_wakup_at == 1)
		{
			g_wakup_at = 0;
			return 1;
		}
		else
			return 0;
	}
}

/*通过AT命令动态设置波特率超过9600时，关闭STANDBY*/
__OPENCPU_FUNC void set_standby_by_rate(uint32_t rate)
{
    if(rate > 9600)
    {
        LPM_LOCK(STANDBY_AT_RATE_LOCK);
    }
	else
	{
		LPM_UNLOCK(STANDBY_AT_RATE_LOCK);
	}
}

//定时器超时后打开standby
__OPENCPU_FUNC void at_standby_timeout_callback(void)
{
	LPM_UNLOCK(STANDBY_AT_RATE_LOCK);
}

//波特率大于9600时，收到第一条AT命令后关闭standby并开启定时器，定时时间内再发AT会重新计时
__OPENCPU_FUNC void set_at_standby_timer(void)
{
	LPM_LOCK(STANDBY_AT_RATE_LOCK);

	Timer_AddEvent(TIMER_AT_STANDBY, g_at_standby_timeout * 1000, at_standby_timeout_callback, 0);
}

/*波特率超过9600，AT触发唤醒或者运行过程中收到AT命令，延迟若干秒开启STANDBY*/
__OPENCPU_FUNC void at_uart_standby_ctl(void)
{
	if((GET_BAUDRATE() > 9600) )
		set_at_standby_timer();
}

__RAM_FUNC void at_uart_wakup_init()
{
#if (MODULE_VER && AT_WAKEUP_SUPPORT)
	/*外部AT唤醒*/
	if((Get_Boot_Sub_Reason() & 1 << AT_WAKUP))
	{
		/*低压问题导致高于9600的AT唤醒，可能uartfifo里没有数据，从而把下一条正常的AT都给丢掉*/
        if( !UARTRxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY) )
        {
		    g_wakup_at = 1;	
        }
		at_uart_standby_ctl();	
	}
#endif	

}

#endif

