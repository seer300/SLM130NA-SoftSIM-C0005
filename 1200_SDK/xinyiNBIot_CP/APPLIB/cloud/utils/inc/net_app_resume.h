#ifndef _NET_APP_RESUME_H
#define _NET_APP_RESUME_H

#include "lwip/sockets.h"
#include "lwip/ip4_addr.h"
#include "cloud_utils.h"
#include "softap_nv.h"
#if AT_SOCKET
#include "at_socket_context.h"
#endif


#define CLOUD_LIFETIME_DELTA(param)             (((uint32_t)param)*9/10)  //距离lifetime 超时的时间间隔(s)

#define SET_RECOVERY_FLAG(param)  \
        {  \
                g_softap_var_nv->cdp_recovery_flag|=(1<<param);  \
        }

#define CLEAR_RECOVERY_FLAG(param)  \
        {  \
                g_softap_var_nv->cdp_recovery_flag&=(~(1<<param));  \
        }

#define NET_NEED_RECOVERY(param)    ((g_softap_var_nv->cdp_recovery_flag&(1<<param))==(1<<param))

#if VER_260Y
#define CDP_DSTREAM_NVM_FILE_NAME      	 	  "cdpDownStream.nvm"
#endif
#define CDP_CONFIG_NVM_FILE_NAME      	 	  "cdpConfig.nvm"
#define CDP_SESSION_NVM_FILE_NAME        	  "cdpSession.nvm"
#define CDP_FOTA_NVM_FILE_NAME       	 	  "cdpFota.nvm" //用来存g_cdp_fota_info上报信息
#define ONENET_CONFIG_NVM_FILE_NAME  	 	  "onenetConfig.nvm"
#define ONENET_SESSION_NVM_FILE_NAME 		  "onenetSession.nvm"
#define TELECOM_UNICOM_DM_NVM_FILE_NAME 	  "telecomUnicomDM.nvm"
#define CMCC_DM_NVM_FILE_NAME            	  "cmccDm.nvm"
#define SOCKET_SESSION_NVM_FILE_NAME    	  "socketSession.nvm"
#define LWM2M_COMMON_CONFIG_NVM_FILE_NAME     "lwm2mCommonConfig.nvm"
#define DNS_SERVER_ADDR_NVM_FILE_NAME   	  "dnsServerAddr.nvm"
#define CTLW_CFG_FILE_UNDER_DIR              "ctwingConfig.nvm"
#define CTLW_SESSION_FILE_UNDER_DIR          "ctwingSession.nvm"


typedef enum {
    NO_TASK   = -2,
    UNKNOWN_IP = -1,
    SOCKET_TASK = 0,
    CMCC_DM_TASK = 1,
    ONENET_TASK = 2,
    CDP_TASK,
}net_app_type_t;

typedef enum
{
    CLOUD_SAVE_NONEED_WRITE = -1,
    CLOUD_SAVE_NEED_WRITE,
}cloud_resume_save_type;

typedef enum
{
    IP_RECEIVE_ERROR = -2,
    IP_NO_CHANGED = -1,
    IP_IS_CHANGED = 0,
}IP_comparison_result_type;

typedef enum {
    RESUME_SUCCEED = 0,
    RESUME_SWITCH_INACTIVE ,    //恢复开关未打开
    RESUME_READ_FLASH_FAILED ,  //读取flash异常
    RESUME_FLAG_ERROR,          //云业务标志位错误
    RESUME_OTHER_ERROR,         //异常错误
    RESUME_LIFETIME_TIMEOUT,    //lifetime已到期
    RESUME_STATE_ERROR,          //状态机状态错误
}cloud_resume_result_type_t;

extern osMutexId_t g_cloud_fs_mutex;

int  is_IP_changed(net_app_type_t type);
void save_net_app_infos(void);
int cloud_save_file(const char * fileName,void * buf, uint32_t size);
int cloud_read_file(const char * fileName,void * buf, uint32_t size);
int cloud_remove_file(const char * fileName);
void *cloud_malloc(const char * fileName);

/**下行数据触发的网络业务恢复总入口*/
void net_resume();
#endif

