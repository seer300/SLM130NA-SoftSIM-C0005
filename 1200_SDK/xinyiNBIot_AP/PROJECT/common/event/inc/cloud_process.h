#pragma once

/**************************************************************************************************************************************************************
 * @brief 对于OPENCPU产品形态，不建议CP核执行云的保存和自动UPDATE，以防止私自唤醒对AP核产生干扰，建议出厂NV参数save_cloud、CLOUDALIVE、set_CP_rtc三者皆设为0；close_drx_dsleep设为1
 *************************************************************************************************************************************************************/

#include "basic_config.h"
#include "at_process.h"
#include "at_CP_api.h"



/**
 * @brief CP远程通信异常时，执行CP核的断电上电容错的次数，当超过该次数时，表明当前无线网络很差，建议深睡一段时间后再尝试,see @ref TRY_RESEND_PERIOD
 */
#define TRY_RESET_CP_MAX       2


/**
 * @brief 当远程通信失败后，重新尝试发送的周期时长，单位毫秒
 */
#define TRY_RESEND_PERIOD      (30*60*1000)


/**
 * @brief 等待PDP激活成功的超时时长，需要考虑扫频、小区驻留、attach等各种流程的总忍耐时长
 */
#define  WAIT_CGATT_TIMEOUT                 120  //second


/**
 * @brief cdp单次发送数据最长字节数
 */
#define  CDP_ONCE_SEND_LEN      1000


/**
 * @brief onenet单次发送数据最长字节数
 */
#define  ONENET_ONCE_SEND_LEN      512


/**
 * @brief ctwing单次发送数据最长字节数
 */
#define  CTLW_ONCE_SEND_LEN      512

/**
 * @brief socket单次发送数据最长字节数
 * @warning 该数据长度不可修改，与cp核一致
 */
#define  SOCKET_SEND_MAX_LENGTH    1358


//定义发送的数据长度，用户可设（HTTP AT 指令并无最大长度限制，但是考虑到 AT 命令本身存在最大长度限制，以及服务器等限制，用户需设置合理的长度大小）
#define HTTP_SEND_DATA_MAX  128

/**
 * @brief mqtt单次发送数据最长字节数
 */
#define MQTT_ONCE_SEND_LEN   1024


/**
 * @brief onenet、cdp、ctwing三大公有云的保活时长，最小值为900s，最大值为30*86400s。
 * @warning  不建议客户启动云的UPDATE功能！如果启用，建议TAU时长务必设置超过该时长，以避免不必要的attach或TAU。
 */
#define CLOUD_LIFETIME          			((24*60*60)*100/85)    //second


//位图
typedef enum
{
	DO_NULL = 0,
	DO_SEND_DATA=1,
	DO_UPDATE=2,
}CLOUD_WAORK_STATUS;

extern int g_cdp_at_step;
extern int g_onenet_at_step;
extern int g_socket_at_step;
extern int g_mqtt_at_step;
extern int g_http_at_step;
extern int g_ctlw_at_step;



/**
 * @brief 云发送的统一封装接口，识别远程通信事件后，取数据进行远程通信。若通信异常，需用户自行进行容错策略
 * @warning 由于数据发送各个客户细节需求不一致，用户可以重新实现该接口，但对于具体的云发送AT流程函数，不建议用户修改！
 */
At_status_type Send_Data_By_Cloud(void);



/**
 * @brief 用户通过cdp云业务发送AT命令，异步等待AT结果
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Cdp_Asyc(void *data,int len,int *send_ret);



/**
 * @brief 用户通过onenet云业务发送AT命令，异步等待AT结果
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Onenet_Asyc(void *data,int len,int *send_ret);


/**
 * @brief 用户通过http云业务发送AT命令，异步等待AT结果
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Http_Asyc(void *data,int len,int *send_ret);


/**
 * @brief 用户通过mqtt云业务发送AT命令，异步等待AT结果
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Mqtt_Asyc(void *data,int len,int *send_ret);


/**
 * @brief 用户通过socket发送AT命令，异步等待AT结果，默认使用TCP，可以通过对宏SOCKET_PROTOCOL_TYPE进行设置选择使用TCP或者UDP
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Socket(void *data,int len,int *send_ret);


/**
 * @brief 用户通过ctwing云业务发送AT命令,异步等待AT结果
 * @param  send_ret  数据发送结果，-1为初始值，0表示发送成功，正值表示发送失败
 * @return AT结果码，@see At_status_type。该错误码必须与CP核选用的AT命令集错误码配套。
 * @warning 整个工程中只能有一个调用点，否则必然造成AT流程状态机异常
 */
At_status_type Send_Data_By_Ctlw_Asyc(void *data,int len,int *send_ret);

/**
 * @brief 重设云状态机
 * @warning 数据发送完成后需要重设云状态机至初始态，防止云状态切换异常
 */
void Reset_Cloud_State();
