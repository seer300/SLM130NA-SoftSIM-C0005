
#ifndef _CDP_BACKUP_H
#define _CDP_BACKUP_H

#include <stdint.h>
#include <stdio.h>
#include "liblwm2m.h"
#include "agent_tiny_demo.h"
#include "xy_rtc_api.h"
#include "lwip/ip_addr.h"
#include "lwip/netdb.h"
#include "cloud_utils.h"
#include "softap_nv.h"
#include "xy_net_api.h"
#include "atiny_socket.h"


#define CONFIG_FEATURE_FOTA  1


/*******************************************************************************
 *							   Macro definitions							   *
 ******************************************************************************/
#define CDP_BACKUP_OBSERVE_MAX		4
#define CDP_BACKUP_WATCHERP_MAX		4

extern osThreadId_t g_cdp_timeout_restart_TskHandle;
extern osThreadId_t g_cdp_resume_TskHandle;
extern osThreadId_t g_cdp_rtc_resume_TskHandle;
extern osSemaphoreId_t cdp_task_delete_sem;
/*******************************************************************************
 *							   Type definitions 							   *
 ******************************************************************************/

typedef struct 
{
	bool active;
	bool update;		
	xy_lwm2m_media_type_t format;	
	unsigned char  	token[8];
	unsigned int  	tokenLen;
	unsigned int 	lastTime;
	uint32_t counter;
	uint16_t lastMid;
	union
	{
		int64_t asInteger;
		double	asFloat;
	} lastValue;
}cdp_lwm2m_watcher_t;

typedef struct 
{
	lwm2m_uri_t uri;					//observe uri
	unsigned int wather_count;			//observe water count
	cdp_lwm2m_watcher_t watcherList[CDP_BACKUP_WATCHERP_MAX];	//observe watcher list
} cdp_lwm2m_observed_t;



typedef struct {
    net_infos_t     net_info;
	uint8_t         endpointname[256];		//endpointname 接入云平台端点名称，长度最大265字节
	unsigned int    regtime;		//registration time
	unsigned int    lifetime;		//cdp lifetime
	unsigned int    location_len;	//server location len
	char            server_location[64];	//server location
	cdp_lwm2m_observed_t observed[CDP_BACKUP_OBSERVE_MAX];  //context observe list
	unsigned int    observed_count;	//context observe count
	uint8_t         pskid[16];				//psk id for dtls
	uint8_t         psk[16];                ////psk for dtls
	uint8_t 		cdp_event_report_enable;  //cdp lwm2m event report 0:disable, 1:enable
	uint8_t         cdp_lwm2m_event_status;   //cdp lwm2m 事件状态查询(0-10)
	uint8_t         cdp_con_send_status;
	int 		    cdp_nnmi;
	int 		    cdp_nsmi;
} cdp_session_info_t;

typedef struct cdp_config_nvm_s {
    uint8_t  cloud_server_ip[48]; //##""##&& CDP云服务器IP地址或者域名,最大长度46个字节
    uint16_t cloud_server_port;   //##0## CDP和onenet的云服务器port端口
    uint32_t cdp_lifetime;     //##0## CDP的lifetime时长，单位秒；取值为0-30*86400,0:默认周期86400；1-30*86400 开启生命周期
    uint8_t  binding_mode;     //##0## CDP专用的lwm2m绑定模式；1:UDP mode  2:UDP queue mode
    uint8_t  cdp_dtls_switch;  //##0## CDP使用DTLS的开关;1,open
    uint8_t  cdp_dtls_nat_type; //##0## CDP自动协商NAT类型，0 使能NAT，重新协商时间间隔30s，1 禁止重新协商
	uint8_t  cdp_dfota_type; //##0## DFOTA 升级模式,0自动升级,1受控升级
	uint8_t  cdp_pskid[16]; //##""##&& pskid for cdp 用来存储15位pskid，cdp dtls 设置psk时用到
	uint8_t  cloud_server_auth[16]; //##""##&& CDP的PSK
	uint8_t  psk_len;
} cdp_config_nvm_t;

extern cdp_config_nvm_t *g_cdp_config_data;
extern cdp_session_info_t *g_cdp_session_info;


void cdp_attach_resume_process();
void cdp_netif_up_resume_process(void *param);
void cdp_rtc_resume_update_process();
void cdp_keeplive_update_process();
void cdp_notice_update_process(void);
void cdp_resuem();
int cdp_resume_app();
int cdp_resume_task();
void cdp_resume_unlock(void);
void cdp_resume_state_process(int code);
int cdp_resume_session_info(lwm2m_context_t  * contextP);
void cdp_resume_update_process();
int cdp_bak_srvip_port(int fd, ip_addr_t *remote_ip, uint16_t port);
int cdp_delete_task();
int cdp_restart_task();

#ifdef CONFIG_FEATURE_FOTA
void cdp_fota_info_resume();
void cdp_ota_state_hook(int state);
#endif


void cdp_bak_downstream();
void cdp_resume_downstream();
#endif //#ifndef _AT_ONENET_H

