#include "prcm.h"
#include "xy_log.h"
#include "xy_system.h"
#include "diag_list.h"
#include "attribute.h"
#include "low_power.h"
#include "deepsleep.h"
#if STANDBY_SUPPORT
#include "standby.h"
#endif
#include "ipc_msg.h"
#include "xy_wdt.h"

extern T_LPM_INFO Ps_Lpminfo;
volatile uint64_t g_idle_tick;
volatile uint64_t g_idle_tick_old;
volatile uint64_t g_idle_tick_utcwdt;
volatile uint64_t g_idle_tick_old_utcwdt;
volatile int g_sleep_check;
extern void ap_status_check(void);
extern uint8_t RF_isManufactryMode(void);
volatile int standby_degrade_flag = 0;
extern uint8_t PhySubfIntExceptionCheck(uint8_t bSendMsg);

void __RAM_FUNC vApplicationIdleHook( void )
{
	int ret = 0;
	ap_status_check();
	icm_buf_check();
	g_idle_tick = get_utc_tick();
	g_idle_tick_utcwdt = g_idle_tick;
	if((g_idle_tick_utcwdt - g_idle_tick_old_utcwdt) > 5000)
	{
#if RUNTIME_DEBUG
		extern void xy_runtime_debug(void);
		xy_runtime_debug();
#endif
		cp_utcwdt_refresh(UTC_WDT_TRIGGER_SECOND);
		g_idle_tick_old_utcwdt = g_idle_tick_utcwdt;
	}

	/*RF校准过程中，禁止进入低功耗*/
	if (RF_isManufactryMode())
	{
		return ;
	}

	//打印bbpll lock lose count
	if((g_idle_tick - g_idle_tick_old) > 2000)
	{
		xy_printf(0,PLATFORM, DEBUG_LOG, "bbpll_lock_lose_count: %d", (uint8_t)(HWREGB(0x4000483D) >> 4));
		g_idle_tick_old = g_idle_tick;
	}

    PhySubfIntExceptionCheck(1);

	//阻塞等待log全部输出完成
	while((diag_list_is_all_list_empty() == 0) || (diag_port_is_send_complete() == 0));

	ps_next_work_time(&Ps_Lpminfo);
	g_sleep_check = Ps_Lpminfo.state << 4;
	switch (Ps_Lpminfo.state)
	{
	 	case READY_TO_DEEPSLEEP:

               	ret = DeepSleep_Process();
               	if(ret != ALLOW_DEEPSLEEP)
               	{
    	 			xy_printf(0,PLATFORM, WARN_LOG, "DeepSleep_Process %d test_rc:%d %d", ret, HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));

//开启此宏定义时，若由于核间消息未处理完导致深睡失败，打印所有待处理消息，供内部调试查看					
#if ICM_ZERO_COPY_LIST_ENABLE 
					if(ret == NOT_ALLOW_DEEPSLEEP_ICM_ZERO_COPY)
					{
						zero_copy_list_t *pmsg = zero_copy_info->head;
						int num = zero_copy_info->pending_num;
				
						while(num > 0 && pmsg != NULL)
						{
							xy_printf(0,PLATFORM,WARN_LOG,"ICM_ZERO_COPY_PENDING_LIST, addr:%x, len:%d, current_thread:%s", pmsg->msg_addr, pmsg->msg_len, pmsg->tskName); 
							pmsg = pmsg->next;
							num--;
						}
					}
#endif
				}
	 	  	break;

	 	case READY_TO_STANDBY:
#if STANDBY_SUPPORT
	 	   		ret = StandBy_Process();
               	if((ret != ALLOW_STANDBY) && (ret != NOT_ALLOW_STANDBY_RC32K_CALI))
               	{
					standby_degrade_flag = 1;
					int temp = Lpm_WFI_Process();
    	 			xy_printf(0,PLATFORM, WARN_LOG, "StandBy_Process %d test_rc:%d %d", ret, HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));
					if(temp != ALLOW_WFI)
					{
						xy_printf(0,PLATFORM, WARN_LOG, "LPM_WFI_Process %d test_rc:%d %d", temp, HWREGB(BAK_MEM_RC32K_CALI_FLAG),HWREGB(BAK_MEM_RC32K_CALI_PERMIT));
					}
				}
#else
				ret = Lpm_WFI_Process();
#endif
	 	   	break;
	 	
		case READY_TO_WFI:
        case KEEP_ACTIVE:
	 		    ret = Lpm_WFI_Process();
                /*
                //避免打印太多
			    if(ret != ALLOW_WFI)
			    {
				    xy_printf(0,PLATFORM, WARN_LOG, "LPM_WFI_Process %d", ret);
			    }
			    */
	 	    break;
	 	
		default:
	 	    break;
	}
	g_sleep_check += ret;
}

