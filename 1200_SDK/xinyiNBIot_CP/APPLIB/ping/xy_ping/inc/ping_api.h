#pragma once

/*******************************************************************************
 *							   Include header files						       *
 ******************************************************************************/
#include  "xy_utils.h"
#include  "xy_net_api.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define  XY_PING_MAX_DATA_LENGTH    1400
#define  PING_THREAD_NAME           "ping_thd" 
#define  PING_THREAD_STACK_SIZE     osStackShared
#define  PING_THREAD_PRIO           osPriorityNormal1  
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
// 此结构体所占内存比较大，应使用malloc方式申请内存
typedef struct
{
    char       host[DNS_MAX_NAME_LENGTH];
    int        time_out;
    int        interval_time;	
    uint16_t   data_len;
	uint16_t   ping_num;
	uint8_t    rai_val;	
	uint8_t    ip_type;
	ip_addr_t  ip_addr;
} ping_para_t;

typedef struct
{
	char*    host_ip;
    uint8_t  ip_type;        // 协议类型
	uint8_t  ttl;            // ping数据包在计算机网络中可以被转发的最大次数（跳数）
	uint16_t len;            // 单次ping请求的数据内容大小
	uint16_t ping_send_num;  // 发送ping请求包次数
	uint16_t ping_reply_num; // 接收ping回复包次数
	int      rtt;            // 单次ping请求所花费的时间
	int      longest_rtt;    // 记录ping_send_num次从发送ping请求包到接收到ping回复包过程的最长时间
    int      shortest_rtt;   // 记录ping_send_num次从发送ping请求包到接收到ping回复包过程的最短时间
	int      time_average;   // 记录ping_send_num次从发送ping请求包到接收到ping回复包过程的平均时间
} ping_info_t;

typedef enum
{
	PING_UNKNOWEN,
	SINGLE_PAC_SUCC,
	SINGLE_PAC_TIMEOUT,
	PING_PAC_NO_MEM,
	PING_END_REPLY
} ping_reply_info_e;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
int stop_ping();
int start_ping(ping_para_t *ping_para);
void user_ping_reply_info_hook(ping_info_t* ping_info, char *rsp_cmd, ping_reply_info_e type);

