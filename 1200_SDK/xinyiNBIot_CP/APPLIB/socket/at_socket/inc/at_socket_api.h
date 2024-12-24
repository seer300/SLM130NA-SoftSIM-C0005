#pragma once

#include <stdint.h>

typedef enum
{
	SOCKET_STATE_CREATED = 0,
	SOCKET_STATE_CONNECTING,
	SOCKET_STATE_CONNECTED,
	SOCKET_STATE_CLOSING,
	SOCKET_STATE_CLOSED,
	SOCKET_STATE_MAX,
} SOCKET_STATE_E;

/* Socket服务的数据访问模式 */      
typedef enum                        
{                                   
	SOCKET_BUFFER_MODE = 0,	        // 缓存模式
	SOCKET_DIRECT_SPIT_MODE,        // 直吐模式
	SOCKET_PASSTHR_MODE	            // 透传模式
} SOCKET_ACCESS_MODE_E;   

/*用于创建socket时使用的参数结构体*/
typedef struct socket_create_param
{
	int	id;					//scoket id	
	int proto;				//6:TCP, 17:UDP
	int af_type;			//AF_INET:ipv4, AF_INET6:ipv6
	char *remote_ip;
	char *local_ip;
	uint16_t remote_port;
	uint16_t local_port;
	uint16_t local_port_ori;
	uint8_t recv_ctrl;
	uint8_t cid;			//上下文ID
	uint8_t  service_type;		    // [IN] socket服务类型，@see @ref SOCKET_SERVICE_TYPE_E
	uint8_t  access_mode;		    // [IN] socket服务的数据访问模式，@see @ref SOCKET_SERVICE_TYPE_E
} socket_create_param_t;

/*用于连接socket时使用的参数结构体*/
typedef struct socket_conn_param
{
	int id;
	char *remote_ip;
	uint8_t reserved[2];
	uint16_t remote_port;
} socket_conn_param_t;

/*用于socket发送数据时使用的参数结构体*/
typedef struct socket_send_param
{
	int8_t id;
	int8_t rai_type;
	uint16_t remote_port;
	char* remote_ip;
	char* data;
	int data_len;
	uint32_t data_str_len;
	uint8_t net_type;	//1:UDP,DGRAM; 0:TCP,STREAM
	uint8_t sequence;
	uint8_t udp_connectd;//upd是否已连接,已连接时不需要再设置远端地址与端口号
	uint8_t passthr_data_mode;//透传数据格式
	int flag;
	uint8_t data_echo;
	uint8_t unused[3];
} socket_send_param_t;

/*用于使用socket读取数据时使用的参数结构体*/
typedef struct socket_readbuffer_param
{
	int id;
	uint16_t remote_port;
	uint8_t nsonmi_rpt; //置1时表示存在下一条未处理的消息,需要重新上报一条nsonmi
	uint8_t reserved;
	char *remote_ip;
	uint32_t want_len;
	uint32_t read_len;
	char *data;
	uint32_t remaining_len;
} socket_readbuffer_param_t;

/**
 * @brief 用于创建socket的函数接口
 * @param arg [IN] 创建scoket时使用的参数 @see @ref socket_create_param_t,
 * @return return -1 or 所创建成功的socket id
 */
int socket_create(socket_create_param_t* arg);

/**
 * @brief 用于连接socket的函数接口
 * @param arg [IN] 连接scoket时使用的参数 @see @ref socket_conn_param_t
 * @return return 结果码 @see @ref socket_result_code_t 
 */
int socket_connect(socket_conn_param_t* arg);

/**
 * @brief 用于建立指定类型的socket连接的函数接口
 * @param arg [IN] 连接scoket时使用的参数 @see @ref socket_create_param_t
 * @return return 结果码 @see @ref XY_TCPIP_ERR_E 
 */
int socket_open(socket_create_param_t *arg);

/**
 * @brief 用于创建socket的异步函数接口
 * @param arg [IN] 创建scoket时使用的参数 @see @ref socket_create_param_t,
 * @return return -1 or 所创建成功的socket id
 */
void socket_open_async(socket_create_param_t *arg);

/**
 * @brief 用于使用socket发送数据给远端的函数接口
 * @param arg [IN] scoket发送数据时使用的参数 @see @ref socket_send_param_t
 * @return return 结果码 @see @ref socket_result_code_t 
 */
int socket_send(socket_send_param_t* arg);

/**
 * @brief 用于关闭socket的函数接口
 * @param sock_id [IN] 关闭scoket时使用的参数
 * @return return 结果码 @see @ref socket_result_code_t 
 */
int socket_close(int sock_id);

/**
 * @brief 用于设置socket的阻塞读取超时时间
 * @param fd [IN] scoket句柄
 * @param timeout [IN] 超时时间(单位:秒)
 * @return return 0:成功 -1:失败
 */
int socket_setprop_rcvtimeout(int fd, int timeout);

/**
 * @brief 用于设置socket非阻塞读
 * @param fd [IN] scoket句柄
 * @return return 0:成功 -1:失败
 */
int socket_setprop_nonblock(int fd);

/**
 * @brief 用于获取socket接收到的下行数据中,未被读取的数据的长度
 * @param fd [IN] scoket句柄
 * @return return 0 or 剩余数据长度
 * @note 若g_at_sck_report_mode设置为下行数据不缓存时,返回值始终为0
 */
int socket_get_remaining_buffer_len(int id);

/**
 * @brief 用于获取socket接收到的下行数据
 * @param arg [IN]/[OUT] 获取scoket接收到的下行数据时使用的参数 @see @ref socket_readbuffer_param_t
 * @return return 结果码 @see @ref socket_result_code_t 
 * @note 使用时应该配合socket_get_remaining_buffer_len函数使用,当返回剩余长度为0时,该函数不应该被调用
 */
int socket_read_buffer(socket_readbuffer_param_t *arg);

/**
 * @brief socket进入透传模式函数接口
 * @param param [IN] 进入透传模式时,发送数据相关设置参数 @see @ref socket_send_param_t
 * @return return 结果码 @see @ref socket_result_code_t 
 */
int socket_enter_passthr_mode(socket_send_param_t *param);

/**
 * @brief socket组建查询待处理的socket消息清单 at指令应答报文函数接口
 * @param socket_ids [IN] 需要查询的socket id数组
 * @param rsp_fmt [IN] 应答报文单条的报文格式
 * @param prsp_cmd [out] 查询完所有scoket id和对应符合条件的sequence后所组建的应答报文
 * @return
 * @note 该函数使用完成后,还需要再在回复报文中增加对应格式的OK报文
 */
void socket_pack_qsosrsp(int *socket_ids, char *rsp_fmt, char **prsp_cmd);

/**
 * @brief 调试打印接口,打印所选ID的socket的相关信息
 * @param id [IN] socket id
 * @return
 */
void socket_debug(int id);

void socket_set_state(int id, int state);

typedef enum
{
	SOCK_TCP_CTL_TIMEOUT = 0,
	SOCK_UDP_CTL_TIMEOUT,
	SOCK_HTTP_CTL_TIMEOUT,
	SOCK_MQTT_CTL_TIMEOUT,
	SOCK_CANCEL_CHECK,
    SOCK_MAX
} SOCK_TIMEOUT_STATUS;

#define SOCKT_TIMEOUT_30S	30000
#define SOCKT_TIMEOUT_10S	10000

struct sock_tm_msg {
	SOCK_TIMEOUT_STATUS		msg_id;
    int socket_id;
	int delay_ms;
	int sequence;
};

void call_socket_delay(SOCK_TIMEOUT_STATUS msg_type, int sockid,int delay_ms,int sequence);
void init_socket_timeout(void);
void del_socket_timeout(int id);
