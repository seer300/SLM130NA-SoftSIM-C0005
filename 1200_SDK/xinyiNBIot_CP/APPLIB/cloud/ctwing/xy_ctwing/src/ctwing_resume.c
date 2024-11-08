#include "ctwing_util.h"
#include "ctlw_NV_data.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctwing_resume.h"
#include "net_app_resume.h"
#include "xy_rtc_api.h"
#include "main_proxy.h"
#include "factory_nv.h"
#include "xy_system.h"
#include "ctlw_sdk_internals.h"
#include "xy_fs.h"


osSemaphoreId_t g_ctlw_sdk_resume_sem = NULL;//SDK主线程,恢复流程结束信号量(包括恢复成功和失败)


extern thread_handle_t sendRecvThreadHandle;

extern osTimerId_t g_ctlw_auto_update_tmr;

extern ctiot_context_t * contextInstance;







/**
 * @brief 创建sdk深睡恢复或断电恢复信号量
 *
 */
void xy_ctlw_create_sdk_resume_sem()
{
    if(g_ctlw_sdk_resume_sem == NULL)
    {
        g_ctlw_sdk_resume_sem = osSemaphoreNew(0xFFFF, 0, NULL);
    }
}


/**
 * @brief 释放sdk深睡恢复或断电恢复信号量
 */
void xy_ctlw_release_sdk_resume_sem()
{
    if(g_ctlw_sdk_resume_sem !=NULL)
        osSemaphoreRelease(g_ctlw_sdk_resume_sem);
}


/**
 * @brief 获取sdk深睡恢复或断电恢复信号量
 */
void xy_ctlw_acquire_sdk_resume_sem()
{
    //等待主线程恢复结束,释放信号量
    if(g_ctlw_sdk_resume_sem !=NULL)
    {
        osSemaphoreAcquire(g_ctlw_sdk_resume_sem, osWaitForever);
    }
}

/**
 * @brief 删除sdk深睡恢复或断电恢复信号量
 */
void xy_ctlw_delete_sdk_resume_sem()
{
    if(g_ctlw_sdk_resume_sem !=NULL)
        osSemaphoreDelete(g_ctlw_sdk_resume_sem);

    g_ctlw_sdk_resume_sem = NULL;
}




/**
 * @brief 检查session会话文件是否存在
 *
 * @return true 存在
 * @return false 不存在
 */
bool xy_ctlw_check_if_session_file_exist(void)
{
	xy_file fp = NULL;

	fp = xy_fopen(CTLW_SESSION_FILE_UNDER_DIR, "rb", FS_DEFAULT);

	if (fp != NULL)
	{
		xy_fclose(fp);
		return true;	
	}
	else
	{
		return false;
	}
}




/**
 * @brief CTwing业务恢复接口
 * 
 * @note 目前使用场景：
 * 1、用于主动update的RTC深睡唤醒 2、下行数据触发的深睡唤醒 3、本地FOTA升级后的上报恢复
 * @return true 存在CTwing业务,已执行恢复
 * @return false 不存在CTwing业务,未执行恢复
 */
bool xy_ctlw_resume(void)
{
    //检查是否存在Session会话文件,判断CTwing业务是否存在
    if(!xy_ctlw_check_if_session_file_exist())
    {
        return false;//不存在CTwing业务
    }

    int32_t ret = xy_ctlw_module_entry();

    if(ret != CTIOT_NB_SUCCESS)
    {
        ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_resume err=%d\r\n",ret);
    }
    return true;
}


/**
 * @brief 主动update RTC发起的业务恢复任务
 * 
 */
/**若唤醒原因为深睡唤醒,仅唤醒后第一次执行callback需要考虑业务线程的恢复,线程退出时句柄不置NULL*/
void xy_ctlw_auto_update_rtc_resume_task(void)
{
    ctiot_log_info(LOG_OTHER_CLASS,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_resume_task,enter\r\n");
    
    app_delay_lock(1000);
    
    if(xy_ctlw_module_entry() == CTIOT_NETWORK_ERROR_BASE)
    {
        ctiot_log_info(LOG_OTHER_CLASS,LOG_OTHER_CLASS,"rtc_resume_task,entry net err!\r\n");
    }

    if(xy_ctlw_check_if_task_running())
    {
        ctiot_set_auto_update_flag(1);
        ctiot_start_auto_update_timer();
        ctiot_log_info(LOG_OTHER_CLASS,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_resume_task,set update\r\n");
    }
    osThreadExit();
}
