/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_worklock.h"
#include "ps_netif_api.h"
#include "low_power.h"
#include "oss_nv.h"
#include "xy_at_api.h"
#include "main_proxy.h"
#include "xy_ps_api.h"
#include "xy_system.h"
#include "xy_lpm.h"
#include "xy_utils.h"

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
 
/*外部MCU通过AT命令操控的外部锁*/
int8_t g_extlock_fd = -1;

/*外部MCU通过QSCLK命令操控的锁*/
int8_t g_qsclklock_fd = -1;

/*AT延迟锁句柄*/
int 	 g_AT_lock_fd = -1;
osTimerId_t g_AT_lock_timer = NULL;


extern int g_Sim_Is_Valid;


/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/


/*AT+WORKLOCK=0触发DEEPSLEEP锁释放后，需要触发3GPP特殊流程*/
void unlock_special_process()
{
	if(is_sleep_locked(LPM_DEEPSLEEP) == 0)
	{
		//产线测试时不插卡,设置NOSIMST为127时协议栈无法进入深睡,此时发AT+WORKLOCK=0释放锁时触发CFUN5可以强制PS软关机，从而进深睡。
		if(g_Sim_Is_Valid == 0)
			xy_cfun_excute(NET_CFUN5);
		xy_send_rai();
	}
}


//AT+WORKLOCK=enable
int at_WORKLOCK_req(char *at_buf,char **prsp_cmd)
{
	int enable = 0;

	if (g_req_type == AT_CMD_REQ)
	{
		if (g_softap_fac_nv->deepsleep_enable == 0)
		{
			return AT_END;
		}

		if (at_parse_param("%d(0-1)", at_buf, &enable) != AT_OK)
		{
			return ATERR_PARAM_INVALID;
		}

		if (enable == 1)
		{
			sleep_lock(g_extlock_fd, LPM_DEEPSLEEP);
		}
		else if (enable == 0)
		{
			at_delaylock_deact();
			sleep_unlock(g_extlock_fd, LPM_DEEPSLEEP);
			unlock_special_process();
		}
		else
		{
			return ATERR_PARAM_INVALID;
		}
		return AT_END;
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "+WORKLOCK:(0-1)");
		return AT_END;
	}
#endif
	else if (g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(48);

		sprintf(*prsp_cmd, "+WORKLOCK:%d",(get_lock_stat(g_extlock_fd,LPM_DEEPSLEEP)));

		return AT_END;
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
}


void qsck_lock_init(void)
{
	g_qsclklock_fd = create_sleep_lock("QsclkLock");
#if VER_BC25
	// if (g_softap_var_nv->qsclk_mode == 0)
	// 	sleep_lock(g_qsclklock_fd, LPM_ALL);
	// else if (g_softap_var_nv->qsclk_mode == 2)
	// 	sleep_lock(g_qsclklock_fd, LPM_DEEPSLEEP);
#endif
}

void set_qsclk_lock(uint8_t qsclk_mode)
{
	if (qsclk_mode == 0)
	{
		sleep_lock(g_qsclklock_fd, LPM_ALL);
	}
	else if (qsclk_mode == 1)
	{
		sleep_unlock(g_qsclklock_fd, LPM_ALL);
	}
	else if (qsclk_mode == 2)
	{
		sleep_unlock(g_qsclklock_fd, LPM_STANDBY);
		sleep_lock(g_qsclklock_fd, LPM_DEEPSLEEP);
	}
}

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/

void delaylock_timeout_cb()
{
	send_msg_2_proxy(PROXY_MSG_REMOVE_DELAY_LOCK, NULL, 0);
}

/*AT唤醒后申请DEEPSLEEP/STANDBY延迟锁，防止云业务osdelay调度机制没有及时被执行*/
int at_delaylock_act()
{
	uint8_t delay_sec = g_softap_fac_nv->deepsleep_delay;

    if(delay_sec==0)
	{
        return XY_ERR;
	}

	if (g_AT_lock_fd == -1)
	{
		if ((g_AT_lock_fd = create_sleep_lock("AT_lock")) == -1)
		{
			xy_assert(0);
			return XY_ERR;
		}
	}

	if (g_AT_lock_timer == NULL)
    {
        osTimerAttr_t timer_attr = {0};
		timer_attr.name = "delaylock_tmr";
		g_AT_lock_timer = osTimerNew((osTimerFunc_t)delaylock_timeout_cb, \
            osTimerOnce, NULL, &timer_attr);

		xy_printf(0, PLATFORM_AP, WARN_LOG, "create delay timer");
    }
	
    osTimerStart(g_AT_lock_timer,delay_sec * 1000);

	/*云业务使用osdelay驱动主线程，osdelay默认不参与STANDBY唤醒，进而也需要申请延迟锁*/
    sleep_lock(g_AT_lock_fd, LPM_ALL);

	xy_printf(0, PLATFORM_AP, WARN_LOG, "at_delaylock_act and sleep_lock");   
	
    return XY_OK;
}

int at_delaylock_deact()
{
	uint8_t delay_sec = g_softap_fac_nv->deepsleep_delay;
	
    if(delay_sec == 0 || g_AT_lock_fd == -1)
	{
		return XY_ERR;
	}

	if (g_AT_lock_timer != NULL && osTimerIsRunning(g_AT_lock_timer) == 1)
	{
		osTimerStop(g_AT_lock_timer);
		xy_printf(0, PLATFORM_AP, WARN_LOG, "at_delaylock_deact  osTimerStop(g_AT_lock_timer)");   
	}
	else
	{
		xy_printf(0, PLATFORM_AP, WARN_LOG, "at_delaylock_deact  osTimerStop fail!!!");   
	}
	
	sleep_unlock(g_AT_lock_fd, LPM_ALL);

    return XY_OK;
}


void sleep_lock_init()
{
    /* 创建并激活AT+WORKLOCK外部锁 */
	g_extlock_fd = create_sleep_lock("ExtLock");
	
	/*外部MCU触发唤醒，必须启动延迟锁，否则云通信中有sleep操作会释放调度进入idle深睡*/
	if (!IS_WAKEUP_BY_SOC())
	{	
		if(g_softap_fac_nv->deepsleep_delay != 0)
		{
			at_delaylock_act();
		}
	}
#if VER_BC25 || VER_260Y
	qsck_lock_init();
#endif
}

