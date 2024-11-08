#pragma once

/*******************************************************************************
 *						      Include header files							   *
 ******************************************************************************/
#include "xy_net_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define PS_PDP_CID_MAX              2
#define INVALID_CID 				0xFF
#define NETIF_WAKEUP_WAIT_MAX 		2000     // 深睡唤醒后网路状态判断等待超时最大时长，单位ms

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief 
 */
typedef struct ps_netif
{
	uint8_t 		cid;
	struct netif *	ps_eth;
} Ps_Netif_T;

/**
 * @brief netif信息
 */
typedef struct
{
    uint8_t      ip_type;                    // pdp激活的ip类型, @see @ref PsNetif_IPType_T
    uint8_t      workingCid;                 // pdp链路id
    uint16_t     reserved;                   // 保留字段
    ip_addr_t    dns[DNS_MAX_SERVERS];       // lwip dns服务器地址,默认dns的索引为：索引0: ipv4 pridns，索引1：ipv4 secdns，索引2：ipv6 pridns，索引3：ipv6 secdns
    ip_addr_t    ip4;                        // ipv4地址
	ip_addr_t    ip6_local;                  // ipv6本地链路地址，协议栈上报的地址
    ip_addr_t    ip6_prefered;               // ipv6有效地址
} PsNetifInfo;

/**
 * @brief
 */
typedef struct Ps_Ipdata_Info
{
	uint8_t 	cid;
	uint8_t 	rai;
	uint8_t 	data_type;
	uint8_t 	padding;
	uint16_t 	sequence;
	uint16_t 	data_len;
	void *		data;
} Ps_Ipdata_Info_T;

typedef struct psNetifEventCallbackList
{
	uint32_t 							eventGroup;
	psNetifEventCallback_t 				callback;
	struct psNetifEventCallbackList*	next;
} psNetifEventCallbackList_T;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern uint8_t g_working_cid;
extern uint8_t g_ipv6_resume_flag;
extern uint8_t g_udp_send_rai[MEMP_NUM_NETCONN];
extern uint16_t g_udp_send_seq[MEMP_NUM_NETCONN];
extern uint32_t g_rate_test;
extern int32_t g_null_udp_rai;
extern osSemaphoreId_t g_out_OOS_sem;
extern osMutexId_t g_netif_callbacklist_mutex;
extern osSemaphoreId_t g_net_ok_sem;
extern psNetifEventCallbackList_T *g_netif_callback_list;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 仅供PS调用，用于传递下行IP数据
 * @param cid[IN] PDP链路ID
 * @param len[IN] IP数据长度
 * @param data[IN] IP数据
 * @return 0:由协议栈释放内存; 1:零拷贝，由平台释放内存
 */
int send_packet_to_user(unsigned char cid, int len, char *data);

/**
 * @brief 查找已经激活的网口
 * @return 返回已激活的网口，netif结构为lwip标准定义
 * @note
 */
struct netif* find_active_netif();

/**
 * @brief PDP激活处理
 * @param pdp_info[IN] 参考PsNetifInfo
 * @note
 */
void ps_netif_activate(PsNetifInfo *pdp_info);

/**
 * @brief PDP去激活处理
 * @param cid[IN] PDP链路ID
 */
int ps_netif_deactivate(uint8_t cid);

/**
 * @brief 查询对应cid的netif是否激活
 */
bool is_netif_active(uint8_t cid);

/**
 * @brief psNetif事件上报
 * @param event: Netif状态事件
 */
void psNetifEventInd(PsStateChangeEvent event);

/**
 * @brief 用于PSM/RTC唤醒后，IPv6地址恢复
 */
bool resume_ipv6_addr(ip6_addr_t *ip6_addr);

/**
 * @brief DNS服务器初始化
 */
void dns_server_init(void);

/**
 * @brief 仅供内部网络接口使用，用于启动后深睡唤醒条件判断
 * @return true 深睡唤醒
 * @return false 非深睡唤醒或者启动时间已超过阈值
 * @note 深睡唤醒后协议栈可能会出现长久无法驻网的场景，或者唤醒驻网后用户又执行了CFUN0/CFUN1操作，增加启动时间超过阈值的判断可以保证调用接口不会长久阻塞
 */
bool netif_is_wakeup_suitation();
