#ifndef _ONENET_UTILS_H
#define _ONENET_UTILS_H
#include "at_onenet.h"
#include "xy_rtc_api.h"
#include "cloud_utils.h"
#include "lwip/ip_addr.h"
#include "xy_net_api.h"
#include "at_lwm2m.h"


#define SET_ONENET_REGINFO_PARAM(param,value)  \
        {  \
            if(g_onenet_session_info != NULL ) \
            {  \
                g_onenet_session_info->param = value;  \
            } \
        }

#define GET_ONENET_REGINFO_PARAM(param) ({(g_onenet_session_info != NULL)? (int)g_onenet_session_info->param : -1; })
extern osThreadId_t g_onenet_resume_TskHandle;
extern osThreadId_t g_onenet_rtc_resume_TskHandle;

typedef struct onenet_session_info_s
{
    net_infos_t  net_info;
    unsigned int ref;                       //onenet context ref index
    unsigned char endpointname[34];         //endpointname
    unsigned char location[24];             //server location
    unsigned int object_count;              //context object count
    onenet_object_t onenet_object[OBJECT_BACKUP_MAX];   //context object list
    // only record the security instance that describes lwm2m server, basically there is only one
    onenet_security_instance_t onenet_security_instance;    //std secruity instance
    unsigned int observed_count;            //observe count
    onenet_observed_t observed[OBSERVE_BACKUP_MAX];     //observe list
    int last_update_time;                   //the newest update time
    int life_time;                          //onenet life time
    cloud_platform_e cloud_platform;                        //cloud_platform type:[0]onenet[1]common [2]中移andlink（暂未使用）
    cloud_platform_common_type_e platform_common_type;      //lwm2m type:[0]yanfei  [2]andlink
} onenet_session_info_t;

//ONENET用户配置类参数
typedef struct onenet_config_nvm_s {
    uint8_t  server_host[CIS_IP_STR_LEN_MAX];
    uint16_t server_port;
    uint8_t  auth_code[20];     //auth_code
    uint8_t  psk[20];          //psk
    uint8_t  bs_enable;              //是否开启引导模式
    uint8_t  dtls_enable;        //是否启用dtls
    uint8_t obs_autoack;  //是否启用模块自动响应订阅请求,0 禁用自动响应，由终端通过命令 AT+MIPLOBSERVERSP 响应订阅请求,1 启用自动响应，终端设备不需要响应订阅请求
    uint8_t write_format; //接收写操作数据的输出模式,0 十六进制字符串显示,1 字符串显示
    uint8_t buf_cfg;      //下行数据缓存配置,0 不缓存下行数据,1 仅缓存下行写操作数据,2 仅缓存下行执行操作数据,3 同时缓存下行写操作和执行操作数据
    uint8_t buf_URC_mode; //是否启用下行数据缓存指示,0 禁用下行数据缓存指示, 启用下行数据缓存指示，在数据缓存区为空时收到下行数据缓存时会通过 URC “+MIPLEVENT:”指示
    uint8_t ack_timeout;  //应答超时重传时间
    uint8_t  onenet_fota_flag; //##0## FOTA 升级标志位,0未升级,1升级;FOTA升级结束后，是否上报的标志位
    uint32_t reg_timeout; //注册超时时间，从连接成功（输出+MIPLEVENT: 0,4）开始计时
} onenet_config_nvm_t;

extern onenet_session_info_t *g_onenet_session_info;
extern onenet_config_nvm_t *g_onenet_config_data;
extern uint8_t g_ONENET_ACK_TIMEOUT;
#if LWM2M_COMMON_VER
extern lwm2m_common_user_config_nvm_t *g_lwm2m_common_config_data;
#endif

void onenet_resume_task();
void onenet_rtc_resume_process();
void onenet_netif_up_resume_process();
void onenet_keeplive_update_process();
void onenet_rtc_resume_cb(void);
void onenet_resume_state_process(int code);
bool onenet_resume(void);
void cis_user_config_init();
void cis_module_init(void);
void onet_remove_session();
#endif //#ifndef _ONENET_BACKUP_PROC_H

