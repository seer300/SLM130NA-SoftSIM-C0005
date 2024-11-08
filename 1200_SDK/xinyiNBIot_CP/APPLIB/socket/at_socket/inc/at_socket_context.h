#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "at_socket_config.h"
#include "cloud_utils.h"
#include "lwip/ip_addr.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define SOCKET_CTX_LOCK()   	 	osMutexAcquire(g_socket_mux, osWaitForever)
#define SOCKET_CTX_UNLOCK()		 	osMutexRelease(g_socket_mux)		 	
#define SOCKET_SNINFO_LOCK()        osMutexAcquire(g_sninfo_mux, osWaitForever)
#define SOCKET_SNINFO_UNLOCK()      osMutexRelease(g_sninfo_mux)
#define SOCKET_SAFE_FREE(mem) \
	do                        \
	{                         \
		if (mem != NULL)      \
			xy_free(mem);     \
	} while (0)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct rcv_data_nod
{
	void *next;
	int len;
	char *data;
	struct sockaddr_storage *sockaddr_info;
} recv_data_node_t;

typedef struct socket_udp_context
{
	uint16_t local_port;
	uint16_t remote_port;

	uint16_t local_port_ori; //原始本地端口号，比如port=0
	int8_t socket_idx;
	uint8_t af_type;

	uint8_t recv_ctl;
	uint8_t data_mode; //@see @ref at_sck_report_mode_e
	uint8_t access_mode;
	uint8_t reserved;

	uint8_t ai_addr[28];
} socket_udp_context_t;

typedef struct socket_udp_info
{
    socket_udp_context_t udp_socket[SOCK_NUM];
} socket_udp_info_t;

typedef struct sock_context
{
	uint8_t sock_id;				//用户自定义socket id
	uint8_t af_type;				//ai family类型，1表示AF_INET6，0表示AF_INET
	uint8_t is_deact;				//pdp去激活标识
	uint8_t quit;					//socket数据接收线程退出标识

	uint8_t fd;						//socket fd
	uint8_t firt_recv;				//首次收到下行数据标识
	uint8_t recv_ctl;				//1:指定socket_id接收传入的下行消息，默认值; 0:指定socket_id忽略传入的下行消息
	uint8_t net_type;				//1:UDP,DGRAM; 0:TCP,STREAM

	uint16_t local_port_ori; 		//原始本地端口号，比如port=0
	uint16_t local_port;			//本地指定或者lwip内部随机分配的端口号

	uint16_t remote_port; 			//远端端口号
	uint8_t reserved[2];

	uint8_t ai_addr[28];			//socket addr，深睡唤醒时恢复UDP上行用

	uint32_t sended_size;			//已发送的数据总大小
	uint32_t acked_size;			//已经回复ack的数据总大小，仅用于tcp
	char *remote_ip;				//远端IP,以字符串表示
	recv_data_node_t *data_list;

	uint8_t state;					//socket状态
	uint8_t cid;					//pdp链路id
	uint8_t access_mode;		    //socket服务的数据访问模式，@see @ref SOCKET_SERVICE_TYPE_E
	uint8_t unused;
	uint8_t service_type;
	int8_t sequence_state[SEQUENCE_MAX];		//记录对应序列数据的发送状态，1:发送成功，0:发送失败
} socket_context_t;

typedef struct sock_sn_node
{
	struct sock_sn_node *next;
	uint32_t per_sn;
	uint32_t data_len;
	uint8_t  socket_id;
	uint8_t  sequence;
	uint8_t  net_type;
	uint8_t  reserved;
} sock_sn_node_t;

typedef struct tcp_ack_info
{
	int16_t  socket;
	uint16_t ack_len;
	uint32_t ack_no;
} tcp_ack_info_t;

/* Socket服务类型 */
typedef enum
{
	SOCKET_TCP = 0,                 // 客户端建立TCP连接
	SOCKET_UDP,                     // 客户端建立UDP连接
	SOCKET_TCP_LISTENER,            // 建立TCP服务器监听TCP连接
	SOCKET_UDP_SERVICE,             // 建立UDP服务，暂不支持
	SOCKET_TCP_INCOMING,            // 建立TCP服务器接受的TCP连接
	SOCKET_UDP_INCOMING             // 建立TCP服务器接受的UDP连接，暂不支持
} SOCKET_SERVICE_TYPE_E;  

typedef enum socket_data_mode
{
	ASCII_STRING = 0,  
	HEX_ASCII_STRING,   //such as: "1234"=>1852(2 bytes)
	BIT_STREAM,
} socket_data_mode_e;

/* bc95 socket下行数据上报模式 */
typedef enum at_sck_report_mode
{
	BUFFER_NO_HINT,			//下行数据无提示，只存储4000个字节，多余的丢弃
	BUFFER_WITH_HINT,		//下行数据提示，第一个下行数据来临时，会主动上报，内容为“+NSONMI:<socketid>,<length>”，最多存储4000个字节，多余的丢弃
	HINT_WITH_REMOTE_INFO,	//下行数据提示，内容为“+NSONMI:<socket>,<remote_addr>,<remote_port>,<length>,<data>”，不储存数据
	HINT_NO_REMOTE_INFO, 	//下行数据提示，内容为+NSONMI: <socket>,<length>,<data>，不储存数据
} at_sck_report_mode_e;

/*数据包发送状态*/
typedef enum sequence_send_status
{
	SEND_STATUS_UNUSED = -1,
	SEND_STATUS_FAILED,
	SEND_STATUS_SUCCESS,
	SEND_STATUS_SENDING,
} sequence_send_status_e;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern uint8_t g_data_recv_mode;    //at socket接收数据类型，参考ipdata_mode_e定义
extern uint8_t g_data_send_mode;    //at socket发送数据类型，参考ipdata_mode_e定义
extern uint8_t g_show_length_mode;
extern uint8_t g_recv_data_viewmode;
extern int g_at_sck_report_mode; 	//at socket下行数据上报模式,参考bc95_report_mode_e定义
extern socket_context_t *g_socket_ctx[];
extern osThreadId_t g_at_socket_rcv_thd;
extern osMutexId_t g_socket_mux;
extern osMutexId_t g_sninfo_mux;
extern socket_udp_info_t *g_socket_udp_info;
extern sock_sn_node_t sn_info;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
void start_at_socket_recv_thread(void);
int del_socket_ctx_by_index(uint8_t socket_id, bool isquit);
int at_socket_create(socket_context_t *socket_ctx, int *fd);
int add_new_rcv_data_node(uint8_t socket_id, int rcv_len, char *buf, struct sockaddr_storage *sockaddr_info);
bool is_socketId_valid(int socket_id);
void add_sninfo_node(uint8_t socket_id, uint32_t len, uint8_t sequence, uint32_t pre_sn);
void del_sninfo_node(uint8_t socket_id, uint8_t sequence);
int check_socket_valid(uint8_t net_type, uint16_t remote_port, char *remote_ip, uint16_t local_port, int flag);
int is_all_socket_exit();
int at_socket_connect(socket_context_t *socket_ctx);

bool check_socket_sendlen(uint32_t str_data_size, uint32_t data_len);
int check_socket_seq(uint8_t id, uint8_t sequence);
void reset_socket_seq_state(uint8_t id);
int get_socket_id_by_fd(int fd, uint8_t* id);
int get_socket_avail_id();
int get_socket_net_type(int id);
int get_socket_af_type(int id);
int get_socket_fd(int id);
int get_socket_seq_state(int id, int seq);
int get_socket_pre_sn(int id, int sequence);

int proc_tcp_ack(int fd, uint32_t ackno, uint16_t ack_len);
int proc_tcp_accept(int fd);
void clear_socket_backup(void);
/**
 * @brief 判断socket上下文是否需要备份
 * @return true:需要备份 false:不需要备份
 * @attention 用于深睡唤醒场景下云保存回复功能
 * @warning 目前只支持udp socket保持
 */
bool is_socket_need_backup();

/**
 * @brief 更新socket上下文信息到文件系统中
 * @return XY_OK:保存信息成功; XY_ERR:保存信息失败 
 * @attention
 * @warning
 */
int update_socket_infos_to_fs(uint8_t socket_id, bool add);

/**
 * @brief 从文件系统中回复socket上下文信息
 * @return XY_OK:恢复信息成功; XY_ERR:恢复信息失败  
 * @attention 深睡唤醒后收到与记录的socket端口号相匹配的下行报文后调用该接口
 * @warning
 */
int restore_socket_infos_from_fs();
void at_socket_init();
bool socket_resume(void);