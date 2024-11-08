#pragma once
#include "xy_utils.h"
#include "lwip/ip_addr.h"
#include "net_app_resume.h"
#include "ctlw_lwm2m_sdk.h"
#include "xy_flash.h"


#define CTLW_LIFETIME_DELTA(param)             (((uint32_t)param)*9/10)  //距离lifetime 超时的时间间隔(s)
#define XY_CTLW_AUTO_REG_HOST_NAME "lwm2m.ctwing.cn"

typedef enum
{
    CONFIG_FILE = 0,
    SESSION_FILE = 1,
}ctlw_file_type_e;

typedef enum
{
    CTLW_GET_MANUFACTURER,
    CTLW_GET_MODEL_NUMBER,
    CTLW_GET_SERIAL_NUMBER,
    CTLW_GET_FIRMWARE_VER,
    CTLW_DO_DEV_REBOOT,
    CTLW_DO_FACTORY_RESET,
    CTLW_GET_POWER_SOURCE,
    CTLW_GET_SOURCE_VOLTAGE,
    CTLW_GET_POWER_CURRENT,
    CTLW_GET_BATERRY_LEVEL,
    CTLW_GET_MEMORY_FREE,
    CTLW_GET_DEV_ERR,
    CTLW_DO_RESET_DEV_ERR,
    CTLW_GET_CURRENT_TIME,
    CTLW_SET_CURRENT_TIME,
    CTLW_GET_UTC_OFFSET,
    CTLW_SET_UTC_OFFSET,
    CTLW_GET_TIMEZONE,
    CTLW_SET_TIMEZONE,
    CTLW_GET_BINDING_MODES,
    CTLW_GET_FIRMWARE_STATE,
    CTLW_GET_NETWORK_BEARER,
    CTLW_GET_SIGNAL_STRENGTH,
    CTLW_GET_CELL_ID,
    CTLW_GET_LINK_QUALITY,
    CTLW_GET_LINK_UTILIZATION,
    CTLW_WRITE_APP_DATA,
    CTLW_UPDATE_PSK,
    CTLW_GET_LATITUDE,
    CTLW_GET_LONGITUDE,
    CTLW_GET_ALTITUDE,
    CTLW_GET_RADIUS,
    CTLW_GET_SPEED,
    CTLW_GET_TIMESTAMP,
    CTLW_GET_VELOCITY,
}ctlw_cmd_e;

typedef enum/*dns查询IP地址类型*/
{
    CTLW_DNS_V4ONLY = 0,//仅V4
    CTLW_DNS_V6ONLY,//仅V6
    CTLW_DNS_V4AndV6,//V4和V6
}ctlw_dns_req_ip_type_e;


typedef enum
{
    XY_CTLW_STATE_REGISTERED=0, //注册成功
    XY_CTLW_STATE_UPDATE_SUCESSED, //update成功
    XY_CTLW_STATE_UPDATE_FAILED,   //update失败
    XY_CTLW_STATE_DEREGISTERED, //去注册
    XY_CTLW_STATE_RELEASE,      //各种原因造成资源释放
    XY_CTLW_STATE_RECOVER_FAILED,//恢复失败
    XY_CTLW_STATE_19OBSERVED,//19/0/0 observed
    XY_CTLW_DATA_SENT_SUCCESS,//数据发送成功
    XY_CTLW_STATE_MAX,
}xy_ctlw_notify_state_e;


#define MAX_VELOCITY_LEN 16
#define XY_CTLW_HOSTNAME_MAX_LEN 33 
#define XY_CTLW_DNS_IP_MAX_LEN 47
#define XY_CTLW_PSKID_LEN 32
#define XY_CTLW_SESSION_FILE_LEN 800
#define XY_CTLW_CONFIG_FILE_LEN sizeof(NV_params_t)


#define CTIOT_NB_FAILED -1

typedef struct
{
    uint8_t opaque[MAX_VELOCITY_LEN];
    int length;
} ctlw_velocity_s;


/**
 * @brief 检查是否有缓存数据未被读取,若有缓存数据,则加锁,不允许芯片进入deepsleep
 * 
 * @param ctlwClientStatus 
 * @param pContext 
 */
void xy_ctlw_check_if_sleep_allow(ctiot_client_status_e ctlwClientStatus, ctiot_context_t *pContext);

/**
 * @brief 设置注册模式 @ref xy_ctlw_reg_mode_e
 * 
 * @param reg_mode (XY_CTLW_MANUAL_REG:手动注册,通过AT命令连接云平台)
 * ,(XY_CTLW_AUTO_REG:自动注册,上电自动连接AEP云平台)
 */
void xy_ctlw_set_reg_mode(xy_ctlw_reg_mode_e reg_mode);


/**
 * @brief 根据入参值，返回当前notify事件原因
 * 
 * @return @ref xy_ctlw_notify_state_e类型值
 */
xy_ctlw_notify_state_e xy_ctlw_get_notify_state(uint8_t subType, uint16_t value, uint32_t data1);


/**
 * @brief 根据SDK的notify主动上报，执行对应的处理
 */
void xy_ctlw_procedure_with_notify(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1);


/**
 * @brief 检查业务运行中IP地址是否发生了变化
 */
bool xy_ctlw_check_if_ip_changed(void);

/**
 * @brief FOTA流程URC上报
 */
void xy_ctlw_fota_notify(int fotaState, int fotaResult);

/**
 * @brief 检查业务是否在运行(处于可收发数据状态)
 */
bool xy_ctlw_check_if_task_running(void);








/**
 * @brief 从FS中获取最近一次业务注册状态
 */
void xy_ctlw_get_nvm_bootflag(ctiot_context_t *pContext);

/**
 * @brief 释放g_ctlw_user_cache,与NV_get_cache配对使用
 */
void NV_free_cache(void);

/**
 * @brief 根据config文件中保存的fotaflag，判断当前fota升级使用的云是否为CTWING
 */
bool xy_ctlw_check_if_fota_running(void);


/**
 * @brief 获取update query参数
 */
void xy_ctlw_get_uri_query(lwm2m_server_t *server, char* query);

/**
 * @brief Ctwing平台修改本地binding_mode参数
 * 
 * @param binding_mode 
 * @return true  修改成功
 * @return false 修改失败
 */
bool xy_ctlw_set_binding_by_iot_plat(const char * binding_mode, size_t size);

/**
 * @brief dns处理线程
 */
void xy_ctlw_dns_task(void);

/**
 * @brief 清除深睡恢复标志位及深睡恢复相关session
 * 
 */
void xy_ctlw_clear_recover_info(void);
