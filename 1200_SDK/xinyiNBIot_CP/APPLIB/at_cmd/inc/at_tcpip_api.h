#pragma once

 /**
 * @file at_tcpip_api.h
 * @brief tcpip at命令相关api接口，供客户开发相关at命令使用
 * @note 客户应尽可能使用该头文件提供的开发接口，避免直接使用lwip内部接口
 * @version 1.0
 * @date 2022-06-14
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 */

#include <stdbool.h>
#include <stdint.h>
#include "xy_rtc_api.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct ntp_zone
{
	char zone[4];				// 字符串形式，例如"+32"
	int zone_sec;				// 时区相较于格林尼治时间的差值，单位秒
} ntp_zone_t;

typedef struct dns_cfg_param
{
	char* pridns;				// 主dns
	char* secdns;				// 辅dns
	bool save;					// 是否保存dns设置到文件系统
	uint8_t reserved[3];
} dns_cfg_param_t;

typedef struct ntp_query_param
{
    uint8_t update;             // 1:启用自动更新本地RTC实时时钟，即查询到的时间会保存到本地CCLK时间
    uint8_t type;               // 用于at ntp命令,表示模组类型
	uint16_t port;              // 设置ntp端口，一般为0，由lwip内部设置
	uint32_t timeout;           // 设置ntp超时时间，一般为0，由lwip内部设置
	char *host;
	RTC_TimeTypeDef rtctime;    // 记录sntp时间转换成的RTC时间
} ntp_query_param_t;

typedef struct dns_query_param
{
	char *host;
	uint8_t type;               // 用于at dns命令，表示模组类型
	uint8_t nocache;		    // dns查询后不缓存
	uint16_t timeout;           // 重置dns超时时间
    int af_family;              // 记录域名IP对应的协议簇
    char* ipaddr_list;          // 获取的ip地址链表，字符串形式
} dns_query_param_t;

typedef enum tcpip_cmd_version
{
	TCPIP_CMD_DEFAULT = 0,		// 芯翼自研
	TCPIP_CMD_QUECTEL,			// 移远
	TCPIP_CMD_CMCC,				// 中移物联
	TCPIP_CMD_MEIGE,			// 美格
	TCPIP_CMD_TELECOM,			// 电信
    TCPIP_CMD_BC25,
} tcpip_cmd_version_e;

typedef enum
{
    TCPIP_OP_OK = 0,           // Operation ok
    TCPIP_Err_Unknown = 550,   // Operation unknown error
    TCPIP_Err_Block,           // Operation block
    TCPIP_Err_Parameter,       // input parameter error
    TCPIP_Err_NoMemory,        // System is out of memory: it was impossible to allocate or reserve memory for the operation.
    TCPIP_Err_SockCreate,      // socket create fail
    TCPIP_Err_NotSupport,      // Operation not support
    TCPIP_Err_SockBind,        // socket bind fail
    TCPIP_Err_SockListen,      // socket listen fail
    TCPIP_Err_SockWrite,       // socket write fail
    TCPIP_Err_SockRead,        // socket read fail
    TCPIP_Err_SockAccept,      // socket accept fail
    TCPIP_Err_PdpOpen,         // PDP context opening failed
    TCPIP_Err_PdpClose,        // PDP context closure failed
    TCPIP_Err_SockInuse,       // socket has in used
    TCPIP_Err_DnsBusy,         // dns busy
    TCPIP_Err_DnsFail,         // dns parse fail
    TCPIP_Err_SockConnect,     // socket connect fail
    TCPIP_Err_SockClosed,      // socket has been closed
    TCPIP_Err_InProgress,      // other progress is doing
    TCPIP_Err_Timeout,         // Operation not completed within the timeout period.
    TCPIP_Err_PdpBroken,       // PDP context broken down
    TCPIP_Err_CancelSend,      // Cancel sending
    TCPIP_Err_NotAllowed,      // Operation not allowed
    TCPIP_Err_ApnConfig,       // APN not configured
    TCPIP_Err_PortBusy,        // port busy
} XY_TCPIP_ERR_E;

#define AT_TCPIP_ERR(err)                    TCPIP_Err_info_build(err, __FILE__, __LINE__)

/**
 * @brief 用于NTP命令内查询时区信息 
 * @return @see @ref ntp_zone_t 
 */
ntp_zone_t get_zone_info(void);

/**
 * @brief 用于at命令内同步方式查询ntp时间信息
 * @param arg [IN] ntp时间信息查询请求入参 @see @ref ntp_query_param_t。客户可自行定制一些特性，如重置ntp tmr超时时间/是否更新本地保存的世界时间信息等
 * @return 结果码参考 @see @ref xy_ret_Status_t
 * @note 对于异步查询ntp时间方式，建议使用at_query_ntp_async接口，同时修改query_ntp_task中的at urc上报即可。
 * 		 若客户有自己的异步处理线程，可在相应的线程中调用query_ntp接口来进行时间信息查询 
 */
int query_ntp(ntp_query_param_t* arg);

/**
 * @brief 用于at命令内异步方式查询ntp时间信息
 * @param arg [IN] ntp时间信息查询请求入参 @see @ref ntp_query_param_t。客户可自行定制一些特性，如重置ntp tmr超时时间/是否更新本地保存的世界时间信息等
 * @return 结果码参考 @see @ref xy_ret_Status_t
 * @note  使用该接口，默认使用芯翼的query_ntp_task异步线程进行时间信息查询，客户需修改query_ntp_task中的at urc上报
 * 		  若客户有自己的异步处理线程，可以将该接口内部的query_ntp_task替换为自己的异步处理线程
 */
int at_query_ntp_async(ntp_query_param_t *arg);

/**
 * @brief 用于at命令内同步方式解析dns域名
 * @param arg [IN] dns解析请求入参 @see @ref dns_query_param_t。客户可自行定制一些特性，如指定协议簇/重置dns tmr超时时间/查询结果不做缓存等
 * @return 结果码参考 @see @ref xy_ret_Status_t 
 * @attention 若DNS命令采用同步方式解析dns域名，可在命令内直接调用该接口；
 * @note 对于异步解析DNS方式，建议使用at_query_dns_async接口，同时修改query_dns_task中的at urc上报即可。
 * 		 若客户有自己的异步处理线程，可在相应的线程中调用query_dns接口来进行域名解析
 */
int query_dns(dns_query_param_t* arg);

/**
 * @brief 用于at命令内异步方式解析dns域名
 * @param arg [IN] dns解析请求入参 @see @ref dns_query_param_t。客户可自行定制一些特性，如指定协议簇/重置dns tmr超时时间/查询结果不做缓存等
 * @return 结果码参考 @see @ref xy_ret_Status_t
 * @note  使用该接口，默认使用芯翼的query_dns_task异步线程进行dns解析，客户需修改query_dns_task中的at urc上报
 * 		  若客户有自己的异步处理线程，可以将该接口内部的query_dns_task替换为自己的异步处理线程
 */
int at_query_dns_async(dns_query_param_t* arg);

/**
 * @brief 用于at命令内dns服务器地址配置
 * @param arg [IN] dns配置入参 @see @ref dns_cfg_param_t。客户可自行设定主辅dns，自行决定是否保存相关配置到文件系统中
 * @return 结果码参考 @see @ref xy_ret_Status_t 
 * @warning 此接口可直接更改lwip的原有的DNS配置，客户使用时，请务必要确认所设置DNS地址可靠
 */
int at_dns_config(dns_cfg_param_t *arg);
