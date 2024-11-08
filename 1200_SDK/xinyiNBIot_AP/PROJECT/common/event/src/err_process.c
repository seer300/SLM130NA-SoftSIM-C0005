#include "xy_system.h"
#include "xy_printf.h"
#include "hal_timer.h"
#include "system.h"
#include "user_config.h"
#include "cloud_process.h"
#include "basic_config.h"
#include "data_gather.h"
#include "at_process.h"
#include "err_process.h"


extern Sending_LocatDef  g_LocationOfSendingData;
extern uint8_t *g_sending_data;


/*CP核容错执行的连续软复位次数，当正常完成通信时需要清零*/
int	g_cp_reset_num = 0;


/*AP核发现CP核异常后，清空CP相关的所有信息，通常用于异常容错，与Force_Stop_CP绑定使用，即对CP执行断电再上电的容错*/
__RAM_FUNC void Clear_Info_of_CP()
{
	/*强行清空CP相关的事件*/
	clear_event(EVENT_CLOUD_UPDATE);
	clear_event(EVENT_CLOUD_SEND);

	/*将尚未发送成功的数据存放到缓存中*/
	Update_Info_By_Send_Result(0);	

	/*将AP侧AT相关的缓存清空*/
	clear_at_info();

	/*云状态机复位*/
	Reset_Cloud_State();
}

/*用户自行保证上次发送失败的数据缓存正常,具体执行请客户设计！*/
__RAM_FUNC void Send_user_data_again(void)
{
	set_event(EVENT_CLOUD_SEND);
}

/* 当检测出CP核通信异常后，通常为尝试给CP核强行下电再上电，并初始化AP核与CP核相关的全局信息，重新进行远程通信。尝试若干次仍然通信失败，则建议客户放弃此次上报，设置下一次重新尝试发送的timer定时器，以节省功耗开销 */
__RAM_FUNC void CP_Err_Process()
{
	/*若CP已经重启若干次仍然失败，则整个芯片深睡一段时间后再起来远程通信*/
	if(g_cp_reset_num >= TRY_RESET_CP_MAX)
	{
		xy_printf("CP_Err_Process set next timer=%d, time=%d\n",TRY_RESEND_PERIOD,(int)(Get_Tick()/1000));

		/*客户可以通过AT错误码来识别CP核的异常，若多次死机，可以将现场保存到flash中，导出后定位死机具体原因*/
		//dump_into_to_flash();
		
		Timer_AddEvent(TIMER_LP_USER5,TRY_RESEND_PERIOD, Send_user_data_again, 0);
		
		g_cp_reset_num = 0;
		
		Stop_CP(0);

		Clear_Info_of_CP();
		
	}
	/*尝试CP核断电上电方式容错，以期望完成远程通信*/
	else
	{
		xy_printf("CP_Err_Process resend time=%d\n",(int)(Get_Tick()/1000));
		
		g_cp_reset_num++;
		
	    Stop_CP(0);

		Clear_Info_of_CP();

		/*重新开始执行CP核远程通信*/
		set_event(EVENT_CLOUD_SEND);
	}
}

/**
  * @brief  业务层面的用户容错策略，目前主要是AT命令相关
  * @param  错误码，具体范围分布：1-500，3GPP相关错误码；500-600，移远BC95扩展错误码；800-900，电信ctwing扩展错误码；余下值皆为私定错误码
  * @return  0表示继续处理；其他值表示流程异常，需及时退出，以防止看门狗异常
  * @warning 异常容错通常是放弃当前操作，待下一次操作时再传输；如果没有RTC周期性事件，建议客户自行设置下一次重新尝试发送的RTC
  */
__RAM_FUNC __WEAK int User_Err_Process(int errno)
{
	if(errno > XY_OK)
	{
		xy_printf("User_Err_Process:%d\n", errno);
	}
	
	switch(errno)
	{
		case  XY_WAITING_RSP:  //表示尚未等待CP核应答结果，需要退出到main主线程继续执行
		case  XY_OK:           //等到OK结果码，正常处理
			break;

		case  XY_ERR_WAIT_RSP_TIMEOUT:  //AT相应超时，一般意味着CP核异常
		case  XY_ERR_IPC_FAIL:          //双核核间通信异常
		case  XY_ERR_CP_NOT_RUN:        //仅用于AP核本地检查
		case  XY_ERR_CP_BOOT:           //Boot_CP失败
		case  XY_ERR_CP_DEAD:           //AP核底层发现CP核挂死
			CP_Err_Process();
			break;

		//如果用户想缩短SOFT_WATCHDOG_TIME的监控时长，必须在此处重设足够大的时长，以防止FOTA下载期间看门狗超时
		case  XY_ERR_DOING_FOTA:
			break;

		//用户可以对特别关注的错误单独执行定制策略
		default:
			CP_Err_Process();
			break;
	}

	return  errno;
}


