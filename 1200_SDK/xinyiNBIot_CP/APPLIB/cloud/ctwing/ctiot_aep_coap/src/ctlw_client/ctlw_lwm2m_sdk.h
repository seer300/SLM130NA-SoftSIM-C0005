/****************************************************************************

            (c) Copyright 2019 by 天翼物联科技有限公司. All rights reserved.

****************************************************************************/

#ifndef _CTLW_LWM2M_SDK_H
#define _CTLW_LWM2M_SDK_H

#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "lwip/inet.h"
#endif


#include "ctlw_config.h"
#include "ctlw_liblwm2m.h"
#include "ctlw_aep_msg_queue.h"
#ifdef CTLW_APP_DEMO
#include "ctlw_user_objects.h"
#endif
#if CTIOT_CHIPSUPPORT_DTLS == 1
//添加mbed dtls 用到的头
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/timing.h"
#include "mbedtls/debug.h"

#endif

#include "ctlw_connection.h"
#include "ctlw_log.h"


#ifdef PLATFORM_XINYI
#include "ctwing_proxy.h"//Ctwing抽象云AT功能
#endif


#if CTIOT_SIMID_ENABLED == 1
#include "../simid/simid.h"
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define CTIOT_CTLW_VERSION "2.1.1"

#ifdef PLATFORM_XINYI //此处增加芯翼宏,为适配电信代码与白皮书不一致
#define CTIOT_SDK_VERSION "2.1"
#else
#define CTIOT_SDK_VERSION "2.1.1"
#endif

#define CTIOT_LWM2M_SDK_VERSION "1.0.2"

#include "ctlw_log.h"

#ifdef PLATFORM_XINYI
#define ctiot_log_info(module, class, ...) xy_ctiot_log_info(LOG_INFO, module, class,  __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ctiot_log_debug(module, class, ...) xy_ctiot_log_info(LOG_DEBUG, module, class,  __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ctiot_log_error(module, class, ...) xy_ctiot_log_info(LOG_ERR, module, class,  __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define ctiot_log_info(module, class, ...) CTIOT_LOG(LOG_INFO, module, class, ##__VA_ARGS__)
#define ctiot_log_debug(module, class, ...) CTIOT_LOG(LOG_DEBUG, module, class, ##__VA_ARGS__)
#define ctiot_log_error(module, class, ...) CTIOT_LOG(LOG_ERR, module, class, ##__VA_ARGS__)
#endif
#define ctiot_log_payload(module,class,refmsg,msgBuffer,length) CTIOT_LOG_BINARY(LOG_DEBUG,module,class,refmsg,msgBuffer,length)

#ifdef __cplusplus
extern "C"
{
#endif

	typedef enum{
		DTLS_OK,
		DTLS_NOK,
		DTLS_SWITCHING,
	}ctiot_dtls_status_e;

#ifdef PLATFORM_XINYI
	typedef enum
	{
		XY_CTLW_MANUAL_REG = 0,//手动注册，通过AT命令连接云平台
		XY_CTLW_AUTO_REG = 1,//自动注册，上电自动连接云平台
	}xy_ctlw_reg_mode_e;/*芯翼适配自注册功能*/
#endif

	typedef enum
	{
		SYSTEM_ON_REBOOT_STOP_SESSION = 0, //!< SDK不跨Reboot会话
		SYSTEM_ON_REBOOT_KEEP_SESSION = 1,  //!< SDK跨Reboot会话
	#if CTIOT_TIMER_AUTO_UPDATE == 1
		SYSTEM_ON_REBOOT_STOP_SESSION_WITH_AUTO_UPDATE = 2,//!< SDK不跨Reboot会话,支持自动update
	#endif
		SYSTEM_ON_REBOOT_MAX,
	} ctiot_keep_session_e; 			   //!< AT+CTLWSETMOD mod1
	typedef enum
	{
		CTIOT_IP_TYPE_INVALID,				//!< ip地址不可用
		CTIOT_IP_TYPE_IPV4,					//!< 仅存在ipv4
		CTIOT_IP_TYPE_IPV6,					//!< 仅存在ipv6
		CTIOT_IP_TYPE_IPV4V6,				//!< ipv4、ipv6都存在
		CTIOT_IP_TYPE_IPV6preparing,		//!< ipv6准备中
		CTIOT_IP_TYPE_IPV4_IPV6preparing,	//!< 存在ipv4，ipv6准备中
	} ctiot_ip_type_e;						//!< ip地址状态
	typedef enum
	{
		CTIOT_NOTIFY_ASYNC_NOTICE,			//!< 异步通知
		CTIOT_NOTIFY_ERROR,					//!< 错误通知
		CTIOT_NOTIFY_RECV_DATA = 2			//!< 下行消息通知
	} ctiot_notify_type;					//!< 通知类型
	typedef enum
	{
		CTIOT_NOTIFY_SUBTYPE_REG,			//!< 登录消息通知
		CTIOT_NOTIFY_SUBTYPE_LWEVENT,		//!< observe消息通知
		CTIOT_NOTIFY_SUBTYPE_UPDATE,		//!< update消息通知
		CTIOT_NOTIFY_SUBTYPE_DEREG,			//!< 登出消息通知
		CTIOT_NOTIFY_SUBTYPE_SEND,			//!< 数据发送消息通知
		CTIOT_NOTIFY_SUBTYPE_LSTATUS,		//!< lstatus消息通知
		CTIOT_NOTIFY_SUBTYPE_DTLS_HS,        //!< dtls hs通知
		CTIOT_NOTIFY_SUBTYPE_SESSION_RESTORE //!< 会话恢复通知
	} ctiot_notify_subType;					//!< 通知子类型
	typedef enum
	{
		CTIOT_DATA_UNCHANGED,		//!< 数据未改变
		CTIOT_DATA_CHANGED,			//!< 数据已改变
	} ctiot_fresh_data_e;			//!< 会话数据改变标记
	typedef enum
	{
		CTIOT_NOTIFY_RECV_MSG,			//!< 通知数据
		CTIOT_NOTIFY_NULL_MSG,			//!< 空通知
	} ctiot_downMsg_notify_type;	//!< 下行数据通知类型
	typedef enum
	{
		CTIOT_UNKNOWN_STATE,			//!< 未知原因
		CTIOT_ACTIVE_REBOOT, 			//!< 上电启动
		CTIOT_ACTIVE_WAKEUP, 			//!< 休眠苏醒
	} system_boot_reason_e;				//!< 启动原因
	typedef enum
	{
		SYSTEM_STATUS_FREE,			//!< 系统闲
		SYSTEM_STATUS_BUSY,			//!< 系统忙
	} system_status_e;				//!< 系统忙闲
	typedef enum
	{
		SYSTEM_SIGNAL_NONE_EVENT,
		SYSTEM_SIGNAL_EPS_EVENT = 1,	//!< eps事件
		SYSTEM_SIGNAL_IP_CHANGED,	//!< ip变更时间
		SYSTEM_SIGNAL_IP_DETACH,	//!< detach事件
	} ctiot_system_signal_e;	//!< 异步通知消息类型
	typedef enum
	{
		SEND_DATA_INITIALIZE,  //!< 初始化
		SEND_DATA_CACHEING,	//!< 缓存中
		SEND_DATA_SENDOUT,	 //!< 已发送
		SEND_DATA_DELIVERED,   //!< 已送达
		SEND_DATA_TIMEOUT,	 //!< 发送超时
		SEND_DATA_RST,		   //!< RST
	} ctiot_data_send_status_e; //!< 数据处理状态
	typedef enum
	{
		ONKEEPSESSION_MODE = 0x01,	//!< 会话跨Reboot模式，0-不支持，1-支持
		REGISTER_MODE = 0x02,	//!< Register报文参数携带模式， 0-电信AEP平台模式，1-普通模式
		AUTH_PROTOCOL_TYPE = 0x03, //!< 终端认证协议类型 	0 --- 常规认证方式（如LWM2M协议规定的urn:imei-imsi或urn:imei）		1 --- 扩展认证方式
		MODE_DTLS_TYPE = 0x04,	 //!< mode_connection_type_e mode_id=5 平台DTLS协议类型：0 --- 明文 //1 --- DTLS  //2 --- DTLS+ // 3 仅加密payload
		CLIENT_U_OR_UQ = 0x05,	//!< 0为PSM模式，1位DRX模式
		CLIENT_IP_ADDR_MODE = 0x06,   //!<  0 为不分配固定IP地址  ，1为使用固定IP地址
		SDK_SLEEP_MODE = 0x07,  //!< SDK软件休眠模式：在网络连接未进入PSM态时，SDK软件是否休眠（缺省值为0）0 --- 可以休眠,1 --- 不休眠

	} ctiot_mode_id_e;

	typedef enum
	{
		THREAD_STATUS_NOT_LOADED = 0,   //!< 未启动状态：此时线程未启动，处理完成后线程状态不变；
		THREAD_STATUS_WORKING = 1,		//!< 工作状态：此时线程已启动，处理正常发送和接收报文业务，处理完成后根据线程当前状态变更最终状态值；
	} ctiot_thread_status_e;			//!< 线程工作状态
	typedef enum
	{
		RELEASE_MODE_DEREG_0,
		RELEASE_MODE_DEREG_1,
		RELEASE_MODE_RECOVER_SESSION_FAIL_L0,
		RELEASE_MODE_LOGIN_FAIL,
		RELEASE_MODE_L1,
		RELEASE_MODE_INNER_UPDATE_404_L0,
		RELEASE_MODE_INNER_UPDATE_TIMEOUT_L0,
		RELEASE_MODE_OUTER_UPDATE_404_L0,
		RELEASE_MODE_LOGIN_FAIL_2,
		RELEASE_MODE_L1_WITHOUT_NOTIFY,
		RELEASE_MODE_DRX_DTLS_IP_CHANGE_L0,
	//	RELEASE_MODE_DRX_DTLS_NO_SSL_L0,
	//	RELEASE_MODE_DRX_DTLS_SSL_RESTORE_FAIL_L0,
		RELEASE_MODE_DRX_DTLS_SEND_FAIL_L0,
		RELEASE_MODE_DRX_DTLS_RECV_FAIL_L0,
		RELEASE_MODE_DRX_DTLS_HS_FAIL_L0,
		RELEASE_MODE_PAYLOAD_ENCRYPT_INIT_FAIL_L0,
		RELEASE_MODE_PAYLOAD_ENCRYPT_PIN_FAIL_L0,
		RELEASE_MODE_APP_ERROR,
		RELEASE_MODE_INNER_UPDATE_WRITE_SSP_FAIL,
		RELEASE_MODE_INNER_UPDATE_OTHER_FAIL,
	}ctiot_release_mode_e;
	typedef enum
	{
		setmoderr = 0  ,//该模式下不支持跨Reboot和跨深度休眠会话
		initdatafail = 1,//初始化时初始化会话数据失败（ctiot_session_init方法失败）
		//pcktcryptinierr = 2 ,//报文加密初始化失败返回(“收发报文加密”模式适用)
		//pcktcryptpinerr = 3 ,//报文加密初始化PIN码失败返回(“收发报文加密”模式适用)

		atupdate404 = 2 ,//外部Update遇到4.04应答
		dtlssendfail = 3  ,//发送DTLS报文失败（致命错误）(U+DTLS+模式适用)
		dtlsrecvdatafail= 4 ,//接收DTLS报文发生致命错误(U模式+DTLS+模式适用)
		hsfail = 5 ,//HS失败 (U+DTLS+模式适用)
		inupdate404 = 6 ,//内部Update遇到4.04应答
		//inupdatewritesesipfail = 7,//内部update遇到writesesip失败
		inupdatedtlssendfail = 7 ,//内部Update发送dtls报文出错
		inupdatetimeout = 8,//内部update遇到timeout
		deregothererr = 9, //DEREG AT指令执行错误
		//inupdateotherfail = 9, //内部update失败因transation节点无法入队
	}ctiot_quit_session_reason_e;
	typedef enum
	{
		configchecksumfail = 0 ,//预初始化Checksum校验失败，恢复出厂设置, “未登录”
		sesdatachecksumfail = 1 ,//预初始化Checksum校验失败，会话按“未登录”处理
	}ctiot_lwengine_ini_fail_reason_e;

	typedef enum
	{
		DATA_FORMAT_TLV = 5,	//!< 按照TLV封装发送
		DATA_FORMAT_OPAQUE = 2, //!< 按照application/octet-stream封装发送
		DATA_FORMAT_TEXT = 7,   //!< 按照text/plain封装发送
		DATA_FORMAT_JSON = 8,   //!< 按照application/vnd.oma.lwm2m+json封装发送
		DATA_FORMAT_LINK = 9	//!< 按照application/link-format封装发送
	} ctiot_send_format_e;	//!< 发送时数据格式
	typedef enum
	{
		DATA_TYPE_STRING = 1,	//!< 字符串类型
		DATA_TYPE_OPAQUE,		//!< 透传数据
		DATA_TYPE_INTEGER,		//!< 整形数据
		DATA_TYPE_FLOAT,		//!< 浮点型数据
		DATA_TYPE_BOOL			//!< bool型数据
	} ctiot_data_type_e; 		//!< 数据类型定义

	typedef enum
	{
		AUTHMODE_STANDARD = 0, 	//!< 常规认证方式，urn:imei
		AUTHMODE_STANDARD_2,	//!< urn:imei-imsi
		AUTHMODE_SIMPLIFIED,	//!< imei
		AUTHMODE_EXTEND_MCU,	//!< 扩展认证方式1：登录时MCU登录时提供认证串
		AUTHMODE_EXTEND_MODULE,	//!< 扩展认证方式2：登录时模组提供认证串
		AUTHMODE_MAX,
	} ctiot_id_auth_mode_e; //!< 终端认证协议类型
	typedef enum
	{
		AUTHTYPE_NO = 0,		//!< 无，未启用
		AUTHTYPE_SIMID = 1,		//!< simid认证，启用
		AUTHTYPE_SM9 = 2,		//!< sm9认证，启用
		AUTHTYPE_MAX
	} ctiot_id_auth_type_e; //!< 终端认证类型
	typedef enum
	{
		NETWORK_UNCONNECTED = 0,		//!< 未连接
		NETWORK_CONNECTED_HOME_NETWORK, //!< 已连接，归属网络
		NETWORK_SEARCHING,				//!< 未连接,搜网或附着中
		NETWORK_AUTHORIZE_FAILED,		//!< 未连接,鉴权失败
		NETWORK_NO_SIGNAL,				//!< 未连接，无法联网（如：无信号覆盖）
		NETWORK_CONNECTED_ROAMING		//!< 已连接，漫游
	} ctiot_wireless_status_e;			//!< 无线连接状态

	typedef enum
	{
		UE_LOGIN_INITIALIZE,//!< 初始化中
		UE_NOT_LOGINED,		//!< 未登录
		UE_LOGINING,		//!< 登录中
		UE_LOGINED,			//!< 已登录
		UE_LOGINED_OBSERVED,//!< 已登录已observe,只做状态反错，不做其他用
		UE_LOGIN_OUTING,	//!< 登出中
		UE_LOGIN_INITIALIZE_FAILED = 9,//!< 系统不可用
	} ctiot_session_status_e; //!< 登录状态

	typedef enum
	{
		BOOT_NOT_LOAD = 0,	//!< 不需要恢复会话
		BOOT_LOCAL_BOOTUP = 1 //!< 需要恢复会话
							  //!< BOOT_BOOTUP_WAKE = 7
	} ctiot_boot_flag_e;	//!< 加载方式

	typedef enum
	{
		PAYLOAD_ENCRYPT_NO,
		PAYLOAD_ENCRYPT_SM2,
	}ctiot_payload_encrypt_algorithm_e;
	typedef enum
	{
		CTIOT_IPTYPE_V4,
		CTIOT_IPTYPE_V6,
		CTIOT_IPTYPE_MAX
	}ctiot_at_ip_type_e;
	typedef struct
	{
		uint8_t reserve : 4;
		uint8_t sIpFlag : 1;
		uint8_t pskFlag : 1;
		uint8_t sObjectFlag : 1;
		uint8_t regFlag : 1;
	} ctiot_data_state_t;

	typedef enum
	{
		OPERATE_TYPE_READ = 0,
		OPERATE_TYPE_OBSERVE,
		OPERATE_TYPE_WRITE,
		OPERATE_TYPE_EXECUTE,
		OPERATE_TYPE_DISCOVER,
		OPERATE_TYPE_CREATE,
		OPERATE_TYPE_DELETE
	} ctiot_operate_type_e;
	typedef enum
	{
		CTIOT_SUCCESS,
		CTIOT_FALSE
	}ctiot_bool_e;
	typedef enum
	{
		SENDMODE_NON,		//!< NON方式
		SENDMODE_CON,		//!< CON方式
		SENDMODE_NON_REL,	//!< NON报文并立即释放空口（RAI）
		SENDMODE_NON_RECV_REL, //!< NON报文并立即释放空口（RAI,发出报文后收到下行的报文后释放空口）
		SENDMODE_CON_REL,		//!< CON报文并立即释放空口（RAI,发出报文后收到下行的报文后释放空口）
	} ctiot_send_mode_e; //!< 数据发送模式
	typedef enum
	{
		CTIOT_PSK_MODE_ASCII,
		CTIOT_PSK_MODE_BINARY,
	}ctiot_psk_mode_e;
	typedef enum
	{
		CMD_TYPE_READ,
		CMD_TYPE_OBSERVE,
		CMD_TYPE_WRITE,
		CMD_TYPE_WRITE_PARTIAL,
		CMD_TYPE_WRITE_ATTRIBUTE,
		CMD_TYPE_DISCOVER,
		CMD_TYPE_EXECUTE,
		CMD_TYPE_CREATE,
		CMD_TYPE_DELETE,
	} ctiot_cmd_type_e;
	typedef enum{
		CTIOT_LSTATUS_QUITSESSION = 0,
		CTIOT_LSTATUS_QUITENGINE  = 1,
		CTIOT_LSTATUS_LWENGINE_INI_FAIL = 2,
		CTIOT_LSTATUS_IP_TYPE_NOT_MATCH = 3,
	}ctiot_lstatus_e;

	typedef enum
	{
		CTIOT_BINDMODE_U,	//!< U
		CTIOT_BINDMODE_UQ   //!< UQ
	} ctiot_bind_mode_e;//!<绑定模式

	typedef enum
	{
		STATUS_TYPE_SESSION,	  //!< 会话状态
		STATUS_TYPE_CONNECT,	  //!< 连接状态
		STATUS_TYPE_MSG,		  //!< 消息状态
		STATUS_TYPE_QUEUE_LEN,	//!< 队列剩余长度
		STATUS_TYPE_RECV_MSG_LEN, //!< 未被指令取走的报文条数
		STATUS_TYPE_CURRENT_IP,//!< 当前连接的ip地址
		STATUS_TYPE_DTLS_STATUS, //!< 模组当前DTLS连接状态
		STATUS_TYPE_CURRENT_SERVER,//!< 当前连接的服务器地址
	} ctiot_query_status_type_e;  //状态查询状态返回类型

	/*AT ERRORs*/
	typedef enum
	{
		CTIOT_NB_SUCCESS,				//!< 操作成功

		//内部错误码,不对用户开放
		CTIOT_SOCK_CREATE_FAIL = 2000,	 //!< 创建socket失败
		CTIOT_YIELD_READ_TIMEOUT = 2001, //!< socket或dtls读取数据超时
		//CTIOT_IN_UPDATE_OTHER_FAIL = 2002, //< 原877,sdk内部使用,不对用户开放,内部update socket发送失败，transaction节点被释放无法加入队列

		CTIOT_SESSION_ERROR_BASE = 800, //!< 会话错误基准值, 当前会话状态不允许 SESNOKERR  (X为会话状态编码)
		CTIOT_NETWORK_ERROR_BASE = 810, //!< 网络状态错误基准值,当前网络状态不满足处理条件 EPSNOKERR (X为EPS状态编码)

		CTIOT_IP_NOK_ERROR = 820,		 //!< 无IP地址无法处理 IPNOKERR
		CTIOT_IPV6_ONGOING_ERROR = 821, //!< IPv6地址获取中 IPv6ONGOINGERR
		CTIOT_IP_TYPE_ERROR = 822,		 //!< 本地IP地址与平台IP地址类型不匹配 IPTYPERR
		CTIOT_NO_DTLS_ERROR = 823,		 //!< 当前不是DTLS会话  NODTLSERR
		CTIOT_DTLS_SWITCH_ERROR = 824,		 //!< 当前DTLS状态为Connecting无法执行该指令或发出报文 DTLSHSINGERR
		CTIOT_DNS_ING_ERROR = 825,		//!< 当前正在通过dns获取平台ip地址

		CTIOT_PARA_NUM_ERROR = 835,		 //!< 参数数量不合法 PRANUMERR
		CTIOT_PARA_VALUE_ERROR = 836,	//!< 失败，参数值不合法 PRAVALERR
		CTIOT_PARA_NOSUP_ERROR = 837,	//!< 参数值合法但模组不支持  PRANSUPERR
		CTIOT_OPERATOR_NOT_SUPPORTED = 838,   //!< 该AT操作不支持
		CTIOT_PARA_NOT_INIT_ERROR = 839, //!< 失败，物联网开放平台连接参数未初始化 NOPRAERR
		CTIOT_DATA_LEN_ERROR = 840,		 //!< 失败，Data字段超长，不能执行该指令 DATALENERR
		CTIOT_DATA_VAL_ERROR = 841,		 //!< 失败，Data字段不合法（长度不是偶数，字符不合法等），不能执行该指令 DATAVALERR
		CTIOT_NO_QUEUE_ERROR = 842,		 //!< 失败，已配置无下行报文队列, NOQUEUEERR
		CTIOT_PARAM_CONFLICATE = 843, 	 //!< 失败，参数设置冲突，PRACONFLICTERR

		CTIOT_TIME_OUT_ERROR = 850,  //!< OVERTIMEERR: 超时无响应（CON模式下生效：重发达到上限后仍未收到ACK）
		CTIOT_ENC_INIT_ERROR = 851, //!<  ENCINIERR: 报文加密所需通道初始化失败
		CTIOT_OTHER_ERROR = 852,	 //!< OTHERERR 因其它原因而处理失败（其它失败）【MCU可重试】
		CTIOT_RST_ERROR = 853,		 //!< RSTERR报文发送失败（收到平台下发的RST）
		CTIOT_IN_PARA_ERROR = 854,		 //!< SIMERR报文发送失败（无法从SIM卡获取IMSI等参数）【操作员查询是否插卡等】
		CTIOT_OVERRUN_ERROR = 855,   //!< OVERRUNERR同类型报文已达到同时并发处理的上限而不能执行该指令
		CTIOT_SYS_API_ERROR = 856,   //!< SYSAPIERR系统调用失败
		CTIOT_AUTH_API_ERROR = 857,		 //!< AUTHAPIERR,
		CTIOT_DTLS_HS_ERROR = 858, 		//!< DTLS连接建立操作失败
		CTIOT_DTLS_OPER_ERROR = 859,   //!< DTLS发送数据操作失败【MCU可重试】
		CTIOT_DTLS_NOK_ERROR = 860, 	//!< DTLS发送数据失败，进入DTLS NOK状态
		CTIOT_ENC_API_ERROR=861, 		//!< ENCAPIERR 调用模组内部加密处理失败
		CTIOT_ENC_LEN_ERROR=862,  		//!< ENCLENERR 调用模组内部加密处理生成加密串超长
		CTIOT_ENC_PIN_INIT_ERROR = 863, //!< 模组初始化PIN码失败
		CTIOT_SOCKET_SEND_BUFFER_OVERRUN=864,//!< 失败，SOCKET发送缓冲区已满

		CTIOT_EPNAME_ERROR = 870,   //!< EPNAMEERR Endpoint Name无法识别或参数错误（平台返回 4.00 Bad Request）
		CTIOT_FORBID_ERROR = 871,   //!< FORBIDERR 鉴权失败，Server拒绝接入（平台返回 4.03 Forbidden）
		CTIOT_PRECON_ERROR = 872,   //!< PRECONERR  IOT Protocol或LWM2M版本不支持（平台返回 4.12 Precondition Failed）
		CTIOT_LOC_PATH_ERROR = 873, //!< LOCPATHERR 平台应答报文location_path格式错误
		CTIOT_PLAT_OTH_ERROR = 874, //!< OTHPLATERR 平台应答错误（其它平台应答报文相关错误）【待定】【开发】
		CTIOT_BAD_REQ_ERROR = 875,  //!< BADREQERR 参数错误（平台返回4.00 Bad Request）,和860的区别?
	} CTIOT_NB_ERRORS;

	typedef enum{
		CTIOT_STATE_INITIAL = 0, //!< SDK需要执行初始化（进一步区分为需要恢复会话和需要登录）
		CTIOT_STATE_RECOVER_REQUIRED, //!< SDK需要执行恢复会话
		CTIOT_STATE_REG_REQUIRED, //!< SDK需要执行登录
		CTIOT_STATE_REG_PENDING, //!< SDK处于登录中状态，需接收登录应答
		CTIOT_STATE_READY, //!< SDK处于已登录状态，正常进行业务
		CTIOT_DEREG_REQUIRED, //!< SDK需要执行DREG
		CTIOT_DEREG_PENDING, //!<  SDK处于登出中状态，需要接收应答
		CTIOT_FREE_REQUIRED, //!<  SDK准备退出，释放资源
	} ctiot_client_status_e;

	typedef enum
	{
		CTIOT_TIMEMODE_0,
		CTIOT_TIMEMODE_1,
		CTIOT_TIMEMODE_2,
		CTIOT_TIMEMODE_8,
		CTIOT_TIMEMODE_16,
		CTIOT_TIMEMODE_32,
		CTIOT_TIMEMODE_64,
		CTIOT_TIMEMODE_128,
		CTIOT_TIMEMODE_256,
	} ctiot_timemode_e;

	typedef enum
	{
		QUERY_STATUS_SESSION_NOT_EXIST = 0, //!< 会话不存在
		QUERY_STATUS_NOT_LOGIN = 1,			//!< 会话未登录
		QUERY_STATUS_LOGINING,				//!< 会话登录中
		QUERY_STATUS_LOGIN,					//!< 会话已登录
		QUERY_STATUS_NO_TRANSCHANNEL = 5,			   //!< 传送通道不存在
		QUERY_STATUS_TRANSCHANNEL_ESTABLISHED,		   //!< 传送通道已建立
		QUERY_STATUS_MSGID_NOT_EXIST = 7,			   //!< msgid查询不到（已发送结束从缓存中清除或不存在）
		QUERY_STATUS_MSG_IN_CACHE,					   //!< 消息缓存中
		QUERY_STATUS_MSG_SENDOUT,					   //!< 消息已发送
		QUERY_STATUS_MSG_DELIVERED,					   //!< 消息已送达
		QUERY_STATUS_PLATFORM_TIMEOUT,				   //!< 平台响应超时
		QUERY_STATUS_PLATFORM_RST,					   //!< 平台响应RST
		QUERY_STATUS_CONNECTION_UNAVAILABLE = 13,	  //!< 连接不可用
		QUERY_STATUS_CONNECTION_UNAVAILABLE_TEMPORARY, //!< 连接暂不可用
		QUERY_STATUS_CONNECTION_AVAILABLE,			   //!< 连接正常（携带覆盖等级）
		QUERY_STATUS_NETWORK_TRAFFIC,				   //!< 网络拥塞（携带拥塞剩余时长）
		QUERY_STATUS_ENGINE_EXCEPTION_RECOVERING,	  //!< Engine异常，恢复中
		QUERY_STATUS_ENGINE_EXCEPTION_REBOOT,		   //!< Engine异常，需reboot重启
		QUERY_STATUS_MAX
	} ctiot_query_status_e; //状态查询返回连接状态


	typedef struct _ctiot_data_t
	{
		struct _ctiot_data_t *next;
		ctiot_data_type_e dataType;
		union {
			bool asBoolean;
			int64_t asInteger;
			double asFloat;
			struct
			{
				size_t length;
				uint8_t *buffer;
			} asBuffer;
		} u;
	} ctiot_data_list;

	typedef struct ctiot_up_msg_node_t
	{
		struct ctiot_up_msg_node_t *next;
		uint16_t msgId;
		uint16_t msgStatus;
		ctiot_send_mode_e mode;
		ctiot_send_format_e sendFormat;
		uint8_t *uri;

		void *node;
	} ctiot_up_msg_node;

	typedef struct _ctiot_update_node_t
	{
		struct _ctiot_update_node_t *next;
		uint16_t msgId;
		uint16_t msgStatus;
		uint8_t raiIndex;
	} ctiot_update_node;
	typedef struct _ctiot_updata_t
	{
		struct _ctiot_updata_t *next;
		//COAP
		uint16_t msgId;
		uint8_t msgStatus;
		uint16_t messageid;
		uint8_t token[8];
		uint8_t tokenLen;
		uint16_t responseCode;
		ctiot_send_mode_e mode;
		ctiot_data_send_status_e status;
		ctiot_send_format_e sendFormat;

		//LWM2M
		uint8_t *uri;

		//CLIENT
		ctiot_data_list *updata;
	} ctiot_updata_list;
	typedef struct _ctiot_down_msg_t
	{
		struct _ctiot_down_msg_t *next;
		uint16_t msgId;
		uint16_t msgStatus; //复用该参数，作为downMsgList的type属性

		time_t recvtime;
		uint8_t *payload;
	} ctiot_down_msg_list;

	typedef struct
	{
		char *baseInfo;
		void *extraInfo; //内容为char*时,需要填extraInfoLen,否则extraInfoLen=0
		uint16_t extraInfoLen;
	} ctiot_status_t;
	typedef struct
	{
		ctiot_query_status_type_e queryType;
		ctiot_query_status_e queryResult;
		union {
			uint64_t extraInt;
			struct
			{
				uint8_t *buffer;
				uint16_t bufferLen;
			} extraInfo;
		} u;
	} ctiot_query_status_t;

	typedef struct
	{
		uint16_t msgId;
		ctiot_cmd_type_e cmdType;
		uint8_t token[8];
		uint16_t tokenLen;
		uint8_t *uri;
		uint8_t observe;
		lwm2m_media_type_t dataFormat;
		uint8_t *data;
		uint16_t dataLen;
	} ctiot_object_operation_t;
	typedef struct
	{
		char apn[CTIOT_CHIP_LEN+1];         //apn
		char imsi[CTIOT_IMSI_LEN+1];        //SIM卡IMSI
		char imei[CTIOT_IMEI_LEN+1];        //imei
		char iccid[CTIOT_ICCID_LEN+1];      //SIM卡ICCID

		char sv[CTIOT_SV_LEN+1];			//模组软件版本
		char module[CTIOT_MODULE_LEN+1];	//模组型号
		char chip[CTIOT_CHIP_LEN+1];        //模组芯片型号
	} ctiot_chip_module_info, *ctiot_chip_module_info_ptr;

	typedef struct
	{
		char rsrp[7];
		char sinr[7];
		char txpower[7];
		char cellid[CTIOT_CELLID_LEN+1];
	}ctiot_wireless_signal_info_t;//无线信号信息

	typedef struct
	{
		ctlw_lwm2m_object_t *securityObjP;
		ctlw_lwm2m_object_t *serverObject;
		int sock;
		connection_t *connList;
		int addressFamily;
	} ctiot_client_data_t;
	typedef enum
	{
		AT_TO_MCU_RECEIVE,
		AT_TO_MCU_STATUS,
		AT_TO_MCU_COMMAND,
		AT_TO_MCU_QUERY_STATUS,
		AT_TO_MCU_SENDSTATUS,
		AT_TO_MCU_SENDERROR,
		AT_TO_MCU_QUERY_PARAM,
	} ctiot_at_to_mcu_type_e;

	typedef enum
	{
		PARAM_CT_ENHANCED_MODE,
		PARAM_NORMAL_MODE,
		PARAM_MODE_MAX,
	} reg_param_mode_e;

	typedef enum
	{
		DTLS_AUTO_UPDATE,
		DTLS_NO_AUTO_UPDATE
	} dtls_update_mode_e;
	typedef enum
	{
		DRX_PSM,
		DRX_NO_AUTO_UPDATE,
		DRX_AUTO_UPDATE
	} drx_update_mode_e;
	typedef enum
	{
		RECV_DATA_MODE_0,
		RECV_DATA_MODE_1,
		RECV_DATA_MODE_2
	} recv_data_mode_e;

	typedef enum
	{
		UQ_WORK_MODE,
		U_WORK_MODE,
		WORK_MODE_MAX,
	} client_work_mode_e;
	//优先发送消息
	typedef struct
	{
		uint16_t msgId;
		uint8_t msgStatus;
	} ctiot_priority_msg;

	typedef enum
	{
		MODE_NO_DTLS = 0,
		MODE_DTLS = 1,
		MODE_DTLS_PLUS = 9,
		MODE_ENCRYPTED_PAYLOAD = 2
	} mode_connection_type_e; //mode_id=5 平台connection协议类型：0 --- 明文 //1 --- DTLS  //9 --- DTLS+ //2 --- payload加密
	typedef enum
	{
		REASON_RECOVER,
		REASON_INNER_UPDATE,
		REASON_USER_UPDATE,
	} lstatus0_reaseon_e;
	typedef enum
	{
		CTIOT_IP_UNCHANGED,
		CTIOT_IP_CHANGED,
	} ctiot_ipchange_e;
	typedef enum
	{
		SESSION_STATUS_ERROR,
		SESSION_STATUS_NOTRECOVER,
		SESSION_STATUS_OK_WITHOUT_NETWORK,
		SESSION_STATUS_OK,
		SESSION_NETWORK_ERROR,
	} ctiot_session_recover_status_e;
	typedef enum
	{
		MODE_SERVER_CMD_DRX,
		MODE_SERVER_CMD_PSM,
	} mode_server_cmd_e; //mode_id = 6平台指令下行模式（缺省值为0）：//0 --- PSM模式 //1 --- DRX模式
	typedef enum
	{
		CTIOT_ONIP_NO_RECONNECT,
		CTIOT_ONIP_RECONNECT
	} onip_reconnect_e;
//通知逻辑，新增
	typedef void (*ctiot_app_notify_cb)(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);
	typedef struct _ctiot_app_notify_cb{
		struct _ctiot_app_notify_cb * next;
		lwm2m_uri_t uri;
		uint8_t type;//0为app通知，1为系统通知

		ctiot_app_notify_cb cb;
	}ctiot_app_notify_cb_node;

	typedef enum
	{
		CTIOT_APP_NOTIFICATION,
		CTIOT_SYSTEM_NOTIFICATION
	} ctiot_notify_cb_type;

//结束
	typedef struct
	{
		/*lwm2m_context*/
		lwm2m_context_t *lwm2mContext;

		/*lwm2m使用的userData*/
		ctiot_client_data_t clientInfo;

		/*模组信息*/
		ctiot_chip_module_info_ptr chipInfo;

		/*登录状态，处理时要注意和lwm2m的state同步*/
		volatile ctiot_session_status_e sessionStatus;

		/*无线信号信息,易变*/
		ctiot_wireless_signal_info_t wirelessSignal;

#if CTIOT_CHIPSUPPORT_DTLS == 1
		/*mbed dtls用到的数据结构*/
		ctiot_dtls_status_e dtlsStatus;
		mbedtls_ssl_context *ssl;
		int16_t lastDtlsErrCode;

		/*AT设置：DTLS参数设置*/
		char *pskID;
		char *psk;
		uint16_t pskLen;
		uint8_t pskMode;
		/*0-无hs,1-有hs;*/
		uint8_t atDtlsHsFlag;
#endif
		/*当前是否处于dns处理中:0-不在进行dns处理,1-正在进行dns处理*/
		uint8_t dns_processing_status;

		/*当前socket使用的IP类型:1是V4; 2是V6 */
		int socketIPType;

		/*当前socket使用的address family类型:AF_INET; AF_INET6 */
		int32_t addressFamily;

		/*ip事件标记: 0是无事件;1是有事件*/
		uint8_t IPEventFlag;

		/*ctiot_release_mode_e 释放资源场景类型*/
		uint8_t releaseFlag;

		/*会话IP，需要跟踪会话IP时从芯片读取*/
		char localIP[INET6_ADDRSTRLEN];

		/*临时存储当前IP，需要跟会话IP作比较后更新会话IP*/
		char tmpLocalIP[INET6_ADDRSTRLEN];

		/*是否跟踪IP的标志*/
		uint8_t onIP;

		/*启动时会话加载方式*/
		ctiot_boot_flag_e bootFlag;

		/*环境数据状态*/
		/* 是否跨reboot会话*/
		ctiot_keep_session_e onKeepSession;

#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
		/*payload加密初始化参数*/
		ctiot_payload_encrypt_algorithm_e payloadEncryptAlgorithm;
		uint8_t* payloadEncryptPin;
		uint8_t payloadEncryptInitialized;
#endif

		/*FOTA*/
		uint8_t fotaFlag;

		/*异步登出结果：0成功，1失败*/
		uint8_t deregResult;

		/*上下行队列*/
		ctiot_msg_list_head *upMsgList;//AT发送数据队列
		ctiot_msg_list_head *downMsgList;//下行数据队列
#ifdef CTLW_APP_FUNCTION
		ctiot_msg_list_head *appMsgList;//APP发送数据队列
#endif
		ctiot_update_node *updateMsg;// update节点

		/*SDK通知回调函数注册列表*/
		ctiot_app_notify_cb_node *appNotifyCbList;
#ifdef PLATFORM_XINYI
		
		//volatile ctiot_thread_status_e selectThreadStatus;///*监听线程状态*/ no use select Thread
		
		xy_ctlw_reg_mode_e regMode;/*AT设置：模组注册模式：0：手动注册 1：自动注册*/

		int32_t abstractCloudFlag;/*指示是否使用抽象云AT*/

		int32_t abstractDownMsgId;/*抽象云AT记录下行数据mesageId*/

#endif
		/*收发线程状态*/
		volatile ctiot_thread_status_e sendAndRecvThreadStatus;

		/*收发线程中使用的终端状态*/
        ctiot_client_status_e ctlwClientStatus;
		uint16_t releaseReason;

		/*存储最后一次内部update的msgID*/
		uint16_t lastInnerUpdateTransID;

		/*AT设置：MCU提供认证加密串*/
		char *authTokenStr;
		char *authModeStr;
		ctiot_id_auth_type_e idAuthType;
		ctiot_id_auth_mode_e idAuthMode;

		/*AT set mod设置*/
		reg_param_mode_e regParaMode;

		/*AT recv设置*/
		recv_data_mode_e recvDataMode;
		uint16_t recvDataMaxCacheTime;
		ctiot_timemode_e recvTimeMode;

		/*AT设置SDK使用的网络连接模式*/
		mode_connection_type_e connectionType;

		/*AT设置：平台参数初始化,由用户设置*/
		char *serverIPV4;
		uint16_t portV4;
		char *serverIPV6;
		uint16_t portV6;

		/*AT设置的保活时间*/
		uint32_t lifetime;

		/*AT设置终端工作模式： U 或 UQ */
		client_work_mode_e clientWorkMode;

		/*server_ip_type_e:根据AT设置的平台初始化参数已经模组当前网络情况确定*/
		uint16_t serverIPType;

		/*会话实际使用的保活时间*/
		uint32_t contextLifetime;

		/* 终端bind mode： U 或 UQ */
		ctiot_bind_mode_e contextBindMode;
	} ctiot_context_t;

	//**************************************************
	//
	//! @brief  引擎初始化(异步),只能调一次,不能重入
	//
	//! @param uint8_t autoLoginFlag，未使用
	//
	//**************************************************
	void ctiot_engine_entry(uint8_t autoLoginFlag);

	//**************************************************
	//
	//! @brief  收发线程
	//
	//**************************************************
	//void ctiot_select_thread_step(void *arg);// no use select Thread
	void ctiot_send_recv_thread_step(void *arg);

	//**************************************************
	//
	//! @brief  获取芯片信息
	//!
	//! @retval  ctiot_chip_info* 芯片信息，见@ref ctiot_chip_info;
	//
	//**************************************************
	ctiot_chip_module_info *ctiot_get_chip_instance(void);

	//**************************************************
	//
	//! @brief  获取SDK上下文
	//!
	//! @retval  ctiot_context_t* SDK上下文，见@ref ctiot_context_t;
	//
	//**************************************************
	ctiot_context_t *ctiot_get_context(void);

	//**************************************************
	//
	//! @brief  AT异步通知接口
	//!
	//! @param notifyType 通知类型，见@ref ctiot_notify_type
	//! @param subType 通知子类型，见@ref ctiot_notify_subType(对应notifyType=CTIOT_NOTIFY_ASYNC_NOTICE)、0-直接通知，1-空通知(对应notifyType=CTIOT_NOTIFY_RECV_DATA)
	//! @param value 参数
	//! @param data1 参数长度
	//! @param data2 参数长度
	//! @param reservedData 扩展数据，目前该参数备用
	//
	//**************************************************
	void ctiot_sdk_notification(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);

	//**************************************************
	//
	//! @brief  查询模组接入物联网开放平台软件版本(AT+CTLWVER)
	//!
	//! @param buff 出参，AT同步响应内容，详见AT说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_query_version(uint8_t *buff); //AT+CTLWVER

	//**************************************************
	//
	//! @brief  设置模组工作模式
	//!
	//! @param modeId 模式ID值
	//! @param modeValue 对应模式ID的设置值
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_set_mod( ctiot_mode_id_e modeId, uint8_t modeValue);

	//**************************************************
	//
	//! @brief  查询终端工作模式
	//!
	//! @param buff 出参，AT同步响应内容，详见AT说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_mod(uint8_t *buff);

	//**************************************************
	//
	//! @brief  设置PSK参数
	//!
	//! @param mode 整形： PSK 类型,0 - ASCII 字符串(模组透传 PSK),1 - 16 进制字符串(模组对PSK执行 2-1 转换)
	//! @param pskId <PSKID>字符串：<Security mode> =2,3时，必须提供
	//! @param psk <PSK>字符串：<Security mode> =2,3时，必须提供
	//!
	//! @retval 详见CTIOT_NB_ERRORS;
	//
	//**************************************************
	uint16_t ctiot_set_psk(uint8_t     mode,uint8_t *pskId, uint8_t *psk);

	//**************************************************
	//
	//! @brief  查询PSK参数
	//!
	//! @param buff 出参，AT同步响应内容，详见AT说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_psk(uint8_t *buff);

	//**************************************************
	//
	//! @brief  AT+CTLWSETAUTH对应接口，设置authType参数
	//!
	//! @param authType 终端认证方式，目前只支持方式0-无，1-simid，2-sm9
	//!
	//! @retval 详见CTIOT_NB_ERRORS;
	//
	//**************************************************
	uint16_t ctiot_set_auth_type(ctiot_id_auth_type_e authType/*, uint8_t serverType*/);

	//**************************************************
	//
	//! @brief  查询AT+CTLWSETAUTH设置的authType参数
	//!
	//! @param buff 出参，AT同步响应内容，详见AT说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_auth_type(uint8_t *buff);

	//**************************************************
	//
	//! @brief  登录物联网开放平台
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_reg_step(void);

	//**************************************************
	//
	//! @brief  查询物联网开放平台登录状态
	//!
	//! @param buff 出参，AT同步响应内容，详见AT说明文档
	//!
	//! @retval ctiot_session_status_e 登录状态
	//
	//**************************************************
	uint16_t ctiot_get_reg_status( uint8_t *buff); //AT+CTLWREG ?

	//**************************************************
	//
	//! @brief  会话更新
	//!
	//! @param raiIndex 是否释放空口
	//! @param msgId 更新报文msgId
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_update(uint8_t raiIndex, uint16_t *msgId);


	//**************************************************
	//
	//! @brief  登出物联网开放平台（异步）
	//!
	//! @param deregMode 登出模式：0、通过登出报文登出；1、无报文直接登出
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_dereg(uint8_t deregMode); //异步版本

	//**************************************************
	//
	//! @brief  发送业务数据到物联网开放平台
	//!
	//! @param data 数据1-2-1格式
	//! @param sendMode 发送模式 见@ref ctiot_send_mode_e
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_send( char *data, ctiot_send_mode_e sendMode, uint8_t *buff); //AT+CTLWSEND sendMode:0 CON模式，1 NON模式，2 NON且释放，3 CON且释放

	//**************************************************
	//
	//! @brief  SDK状态查询
	//!
	//! @param queryType 查询类型
	//! @param msgId 消息ID
	//! @param buff出参，AT同步响应内容，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_status(uint8_t queryType, uint16_t msgId, uint8_t *buff);

	//**************************************************
	//
	//! @brief  将数据进行1->2编码（将一个字节转换为两个HEX字符）
	//!
	//! @param data 数据1-2-1格式
	//! @param datalen 数据长度
	//!
	//! @retval 编码后的字符串;
	//
	//**************************************************
	char *ctiot_at_encode(uint8_t *data, int datalen);

	//**************************************************
	//
	//! @brief  将数据进行2->1解码（将两个HEX字符转换为一个字节）
	//!
	//! @param data 数据1-2-1格式
	//! @param datalen 数据长度
	//! @param retBuf  出参,解析后的二进制串
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_at_decode(char *data, int datalen,uint8_t** retBuf);

	//**************************************************
	//
	//! @brief  设置接收消息的模式
	//!
	//! @param mode 接收消息模式
	//! @param maxCacheTimeMode，数据缓存时间模式
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_set_recv_data_mode(recv_data_mode_e mode, ctiot_timemode_e maxCacheTimeMode);

	//**************************************************
	//
	//! @brief  查询接收消息的模式
	//!
	//! @param buff出参，AT同步响应内容，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_recv_data_mode( uint8_t *buff);

	//**************************************************
	//
	//! @brief  获取下行缓存消息
	//!
	//! @param buff出参，AT同步响应内容，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_recv_data(uint8_t *buff);

	//**************************************************
	//
	//! @brief  修改上下文中的资源信息
	//!
	//! @param uri objectList的URL字符串
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	int ctlw_resource_value_changed(char *uri);

	//**************************************************
	//
	//! @brief  登录上下文发生变化时，设置变化标记，系统进入空闲时，系统根据该标记进行是否进行上下文数据序列化操作
	//!
	//! @retval 设置后的变化标记,暂时未使用
	//
	//**************************************************
#ifndef PLATFORM_XINYI
	uint8_t ctiot_update_sdataflash_needed();
#else
	uint8_t ctiot_update_sdataflash_needed(bool isImmediately);
#endif
	//**************************************************
	//
	//! @brief  释放上下文信息
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_free_context(void);

	//**************************************************
	//
	//! @brief	设置lifetime
	//!
	//! @param lifetime
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_set_lifetime( uint32_t lifetime);

	//**************************************************
	//
	//! @brief  设置server信息
	//!
	//!@param  action, 操作类型0:更新,1:删除
	//!@param  ipType, 地址类型
	//!@param  serverIp, 服务器地址
	//!@param  port, 端口号
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_set_server(uint8_t action,uint8_t ipType,char *serverIp, uint16_t port);

	//**************************************************
	//
	//! @brief  根据msgId获取消息状态
	//!
	//! @param msgId 需要查询的msgId值
	//! @param buff出参，AT同步返回信息，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_msg_status(uint16_t msgId, uint8_t *buff);

	//**************************************************
	//
	//! @brief  获取当前队列剩余长度
	//!
	//! @param buff出参，AT同步返回信息，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_current_queue_len(uint8_t *buff);

	//**************************************************
	//
	//! @brief  登录物联网开放平台(异步)
	//!
	//! @param   regModeStr, 认证模式
	//! @param   regAuthStr, 认证串
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_reg(uint8_t *regModeStr, uint8_t *regAuthStr);

	//**************************************************
	//
	//! @brief  发送普通上报数据
	//!
	//! @retval true-处理成功，false-处理失败，需重启
	//
	//**************************************************
	bool ctiot_send_step(void);

	//**************************************************
	//
	//! @brief  发送外部update数据
	//!
	//! @retval true-处理成功，false-处理失败，需重启
	//
	//**************************************************
	bool ctiot_send_update_step(void);

	//**************************************************
	//
	//! @brief  往flash中刷新会话数据
	//!
	//! @param action, 目前固定为0
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_session_data(uint8_t action);

	//**************************************************
	//
	//! @brief  恢复出厂设置
	//!
	//! @param resetMode 恢复模式，目前只支持模式0
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_cfg_reset(uint8_t resetMode); //恢复出厂设置


	//**************************************************
	//
	//! @brief  资源释放流程处理
	//!
	//**************************************************
	void ctiot_release_step(void);

	//**************************************************
	//
	//! @brief  启动释放线程
	//! @param mode 释放模式,0-dereg 0,1-dereg 1,2-释放并发lstatus 0，3-仅释放,4-进入会话不可用状态
	//! @param reason 释放原因
	//!
	//**************************************************
	void ctiot_set_release_flag(uint8_t mode, uint16_t reason);

	//**************************************************
	//
	//! @brief  启动收发线程
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_start_send_recv_thread(void);

	//**************************************************
	//
	//! @brief  发送update消息
	//! @param raiIndex 是否使用RAI，0-不使用，1-使用
	//! @param msgId 消息id
	//! @param updateType update类型，0-外部update，1-内部update
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_send_update_msg(uint8_t raiIndex, uint16_t msgId, uint8_t updateType);

	//**************************************************
	//
	//! @brief  获取服务器配置参数
	//!
	//! @param buff出参，AT同步返回信息，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_server(uint8_t *buff);

	//**************************************************
	//
	//! @brief  获取lifetime配置参数
	//!
	//! @param buff出参，AT同步返回信息，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_lifetime(uint8_t *buff);

	//**************************************************
	//
	//! @brief  ctiot_deregister_step
	//
	//! @param deregMode, 0:常规登出,1:本地登出
	//
	//**************************************************
	void ctiot_deregister_step(uint8_t deregMode);
#ifdef CTLW_APP_FUNCTION
	//**************************************************
	//
	//! @brief	app数据发送单元
	//!
	//! @retval  true处理成功,false处理失败
	//
	//**************************************************
	bool ctiot_app_send_step(void);
#endif

	//**************************************************
	//
	//! @brief	注册系统通知回调函数
	//!
	//! @param  callback, 系统通知回调函数
	//
	//**************************************************
	void ctiot_add_system_notify_cb(ctiot_app_notify_cb        callback);

	//**************************************************
	//
	//! @brief	注册APP通知回调函数
	//!
	//! @param uri,
	//! @param uriLen ,
	//! @param callback, 系统通知回调函数
	//!
	//
	//**************************************************
	void ctiot_add_app_notify_cb(uint8_t * uri, uint8_t uriLen,ctiot_app_notify_cb  callback);

	//**************************************************
	//
	//! @brief	发布SDK通知
	//!
	//! @param uint8_t notifyCbType,调用回调函数类型
	//! @param notifyType,通知类型
	//! @param subType, 通知子类型
	//! @param value, 通知值
	//! @param data1, 通知数据1
	//! @param data2, 通知数据2
	//! @param reservedData 通知附加数据
	//!
	//
	//**************************************************
	void ctiot_publish_sdk_notification(char* uri, uint8_t notifyCbType, uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);

	//**************************************************
	//
	//! @brief	清除通知回调函数列表
	//
	//**************************************************
	void ctiot_free_app_notify_list(void);

	//**************************************************
	//
	//! @brief	上行数据入队
	//!
	//! @param data, 数据
	//! @param dataLen, 数据长度
	//! @param sendMode,发送模式
	//! @param sendFormat,数据格式
	//! @param  uri,
	//! @param msgID,返回该上行数据的msgid
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_lwm2m_obj_notify(char *data, uint16_t dataLen, ctiot_send_mode_e sendMode, ctiot_send_format_e sendFormat, char* uri, uint16_t *msgID);

	//**************************************************
	//
	//! @brief	终端报文加密参数设置
	//!
	//! @param type,设置的类型
	//! @param value,该类型的值
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_set_payload_encrypt(uint8_t type,uint8_t* value);

	//**************************************************
	//
	//! @brief	触发模组主动发出DTLS HS
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_at_dtls_hs(void);

	//**************************************************
	//
	//! @brief	APP触发进行需重启状态
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_app_release_sdk(void);

	//**************************************************
	//
	//! @brief	获取终端报文加密参数设置
	//!
	//! @param buff出参，AT同步返回信息，详见AT指令说明文档
	//!
	//! @retval 详见CTIOT_NB_ERRORS
	//
	//**************************************************
	uint16_t ctiot_get_payload_encrypt( uint8_t *buff);

	//**************************************************
	//
	//! @brief	设置当前dns处理状态
	//!
	//! @param curStatus 入参,当前dns处理状态,0-dns过程处理完成,1-开始进入dns处理
	//!
	//! @retval 0-设置成功,1设置失败
	//
	//**************************************************
	uint16_t ctiot_set_dns_process_status(uint8_t curStatus);
//正常非测试状态下，SDK执行的代码处理逻辑
		void ctiot_chip_vote(uint8_t slpHandler, system_status_e status);

//****************************************************************
//! @brief	ctiot_get_vote_status:查询当前芯片投票状态忙还是闲
//! @param	无参数
//! @retval uint16_t 0：闲， 1：忙
//****************************************************************
uint8_t ctiot_get_vote_status(void);


#ifdef __cplusplus
}
#endif
#endif //_CTLW_LWM2M_SDK_H
