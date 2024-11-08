#include "xy_lpm.h"
#include "xy_system.h"
#include "xy_memmap.h"
#include "xy_at_api.h"
#include "csp.h"
#include "at_ctl.h"
#include "at_worklock.h"
#include "low_power.h"
#include "xy_rtc_api.h"
#include "prcm.h"
#include "softap_nv.h"
#include "xy_ps_api.h"

#include "fs_al.h"
#include "xy_fs.h"



typedef struct SleepLock
{
    struct SleepLock_t *next;
	uint8_t lock_flag;          //表示当前锁持有的sleep_type类型，@see @ref Lock_Type_E
	int8_t lock_fd;
    char* lock_name;            //表示当前锁的名称，必须是RO属性
} SleepLock_t;

typedef struct
{
    uint32_t fd_bitmap; /*工作锁fd位图,某位为1表示对应的工作锁被创建,最多可创建32把*/
    SleepLock_t *head;
} LockHeader_t;

LockHeader_t g_lock_list = {0};


#define LPM_BITMAP_LEN    32


static int8_t get_unused_lockfd()
{
	osCoreEnterCritical();

    for (int8_t i = 0; i < LPM_BITMAP_LEN; i++)
    {
        if (!(g_lock_list.fd_bitmap & (1 << i)))
        {
			g_lock_list.fd_bitmap |= 1 << i;
			osCoreExitCritical();
			return i;
		}
	}

	osCoreExitCritical();
	xy_assert(0);
	return -1;
}


/**/
int8_t create_sleep_lock(char* lock_name)
{
	SleepLock_t *node = NULL;

	osCoreEnterCritical();

	node = g_lock_list.head;

    while (node != NULL)
    {
        if (!strcmp(node->lock_name, lock_name))
        {
			osCoreExitCritical();
			return node->lock_fd;
		}
		
		node = node->next;
	}

	node = xy_malloc(sizeof(SleepLock_t));
	node->lock_name = lock_name;
	node->lock_flag = 0;
	node->lock_fd = get_unused_lockfd();
	node->next = g_lock_list.head;
	g_lock_list.head = node;

	osCoreExitCritical();
	
	return node->lock_fd;

}

void clear_sleep_lock(Lock_Type_E sleep_type)
{
	osCoreEnterCritical();
	SleepLock_t *plocklist = g_lock_list.head;
	while(plocklist != NULL)
	{
		plocklist->lock_flag &= ~(sleep_type);
		plocklist = plocklist->next;
	}

	osCoreExitCritical();
}

void delete_sleep_lock(int8_t lockfd)
{
	osCoreEnterCritical();
	if (g_lock_list.head != NULL)
	{
		SleepLock_t *plocklist = g_lock_list.head;
		SleepLock_t *pre_node = NULL;
		while (plocklist != NULL)
		{
			if (lockfd == plocklist->lock_fd)
			{
				if (plocklist == g_lock_list.head)
				{
					g_lock_list.head = plocklist->next;
				}
				else
				{
					pre_node->next = plocklist->next;
				}
				xy_free(plocklist);
				break;
			}
			else
			{
				pre_node = plocklist;
				plocklist = plocklist->next;
			}
		}
	}
	osCoreExitCritical();
}

int8_t get_lock_stat(int8_t lockfd, Lock_Type_E sleep_type)
{
	int8_t ret = 0;
	osCoreEnterCritical();
	if(g_lock_list.head != NULL)
	{
		SleepLock_t *plocklist = g_lock_list.head;
		while (plocklist != NULL)
		{
			if (lockfd == plocklist->lock_fd)
			{
				ret = plocklist->lock_flag & sleep_type;
				break;
			}
			else
			{
				plocklist = plocklist->next;
			}
		}
	}
	osCoreExitCritical();
	if(ret > 0)
		ret = 1;
	return ret;
}

uint8_t get_sleep_lock_num(Lock_Type_E sleep_type)
{
	osCoreEnterCritical();
	SleepLock_t *plocklist = g_lock_list.head;
	uint8_t lock_num = 0;
	while(plocklist != NULL)
	{
		if(plocklist->lock_flag & sleep_type)
		{
			lock_num++;
		}
		plocklist = plocklist->next;
	}
	osCoreExitCritical();

	return lock_num;
}

uint8_t is_sleep_locked(Lock_Type_E sleep_type)
{
	osCoreEnterCritical();
	SleepLock_t *plocklist = g_lock_list.head;
	while(plocklist != NULL)
	{
		if(plocklist->lock_flag & sleep_type)
		{
			osCoreExitCritical();
			return 1;
		}
		plocklist = plocklist->next;
	}
	osCoreExitCritical();

	return 0;
}

void sleep_lock(int8_t lockfd, Lock_Type_E sleep_type)
{
	osCoreEnterCritical();

	SleepLock_t *plocklist = g_lock_list.head;
	while (plocklist != NULL)
	{
		if (lockfd == plocklist->lock_fd)
		{
			plocklist->lock_flag |= sleep_type;
			break;
		}
		else
		{
			plocklist = plocklist->next;
		}
	}
	osCoreExitCritical();
}

void sleep_unlock(int8_t lockfd, Lock_Type_E sleep_type)
{
	osCoreEnterCritical();

	SleepLock_t *plocklist = g_lock_list.head;
	
	while (plocklist != NULL)
	{
		if (lockfd == plocklist->lock_fd)
		{
			plocklist->lock_flag &= ~(sleep_type);
			break;
		}
		else
		{
			plocklist = plocklist->next;
		}
	}

	osCoreExitCritical();
}



int g_app_lock_fd = -1;
osTimerId_t g_app_lock_tmr = NULL;


static void app_lock_timeout_cb(osTimerId_t arg)
{
    UNUSED_ARG(arg);
	delete_sleep_lock(g_app_lock_fd);
}

/*应用会使用osdelay进行任务调度，而osdelay不参与DEEP/STANDBY睡眠裁决，进而提供该接口供3GPP尚未开始运行时临时锁睡眠*/
int app_delay_lock(uint32_t timeout_ms)
{
    if (g_app_lock_fd == -1)
    {
        if ((g_app_lock_fd = create_sleep_lock("netserv_lock")) == -1)
        {
            xy_assert(0);
            return XY_ERR;
        }
    }
    sleep_lock(g_app_lock_fd,LPM_ALL);
    if (g_app_lock_tmr == NULL)
    {
        g_app_lock_tmr = osTimerNew((osTimerFunc_t)(app_lock_timeout_cb), osTimerOnce, NULL, NULL);	
    }
	
	if(timeout_ms != 0)
    	osTimerStart(g_app_lock_tmr, timeout_ms);
	
    return XY_OK;
}

void app_delay_unlock()
{
	if (g_app_lock_fd != -1)
	{
		sleep_unlock(g_app_lock_fd,LPM_ALL);
	}
	
	if (g_app_lock_tmr != NULL && osTimerIsRunning(g_app_lock_tmr) == 1)
    {
    	osTimerStop(g_app_lock_tmr);
	}
}


void Before_DeepSleep_Hook(void)
{
#if !VER_260Y
    if(HWREGB(BAK_MEM_XY_DUMP) == 1 && !Is_OpenCpu_Ver())
    {
        //示例代码:深睡前调用专用的open和写接口，唤醒后调用普通的文件系统读接口进行查询
        send_debug_by_at_uart("Before_DeepSleep_Hook\r\n");
        xy_file fp = NULL;
        int writeCount = XY_ERR;
        int readCount = -1;
        char *writebuf = xy_malloc(sizeof(softap_fac_nv_t));
        fp = lpm_fs_fopen("xyfile.nvm", "w+", FS_DEFAULT );
        if (fp != NULL)
        {
            readCount = lpm_fs_fread(writebuf, sizeof(softap_fac_nv_t), fp);
            if(readCount > 0)
            {
                if(writebuf[0] == 1)
                    writebuf[0] = 0;
                else
                    writebuf[0] = 1;
            }
            else
                memcpy(writebuf, g_softap_fac_nv, sizeof(softap_fac_nv_t));
            writeCount = lpm_fs_fwrite(writebuf, sizeof(softap_fac_nv_t), fp);
            lpm_fs_fclose(fp);
        }
        else
        {
            send_debug_by_at_uart("+DBGINFO:fs open error\r\n");
        }

        xy_free(writebuf);
    }
#endif

#if VER_260Y
    extern void cdp_bak_downstream();
    cdp_bak_downstream();
#endif
}


