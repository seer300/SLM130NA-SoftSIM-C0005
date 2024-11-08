#pragma once
#include "xy_net_api.h"

typedef int (*proxy_config_callback)(uint8_t req_type,uint8_t *paramlist, uint8_t **prsp_cmd);
typedef int (*proxy_send_callback)(uint8_t req_type,uint8_t *paramlist, uint8_t **prsp_cmd);
typedef int (*proxy_recv_callback)(uint8_t req_type,uint8_t *paramlist, uint8_t **prsp_cmd);

typedef enum
{
	CLOUD_PROXY_SUCCESS = 0, //操作成功
	CLOUD_PROXY_ERR = -1,
	CLOUD_PROXY_OVERFLOW_ERR = 1,//下行数据缓存数量超过最大限制
}cloud_proxy_err_e;

typedef enum
{
	min_proxy = 0,
	cdp_proxy ,
	cis_proxy ,
	ctwing_proxy,
	max_proxy ,
}cloud_proxy_type_e;


typedef enum
{
	CLOUD_NOTIFY_RECV_SUCCESS = 0,//下行数据成功加入下行数据缓存链表
	CLOUD_NOTIFY_RECV_OVERFLOW = 1,//下行缓存个数超过上限
	CLOUD_NOTIFY_OTHER_NOTIFY =2,//未定义的URC上报类型,待扩展
}cloud_notify_type_e;//URC通知类型



typedef struct cloud_proxy_callback_s
{
	cloud_proxy_type_e       cloudType;
	proxy_config_callback    cloudConfigProc;
	proxy_send_callback      cloudSendProc;
	proxy_recv_callback      cloudRecvProc;
}cloud_proxy_callback_t;

typedef struct cloud_proxy_config_s
{
	uint8_t serverIP[XY_IPADDR_STRLEN_MAX];

	uint8_t	accessMode;
	uint8_t	bsEnable;
	uint8_t	dtlsEnable;
	uint8_t	authEnable;

	uint16_t objectID;
	uint16_t insID;
	uint16_t resCount;
	uint16_t serverPort;
	uint32_t lifetime;

	char *auth_code;
	char *resStr;
    char *pskID;
    char *psk;
}cloud_proxy_config_t;

typedef struct _cloud_proxy_buffer_pkt_msg{
    int     type;       ///< message type
    int     flag;       ///< uri flag
    int     objId;      ///< uri object id
    int     insId;      ///< uri instance id
    int     resId;      ///< uri resource id
    int     evtId;      ///< event id
    int     index;      ///< message index
    int     data_len;   ///< message data length
    int     valueType;  ///< message data type
    int		msgid;		///< message id
    char    data[0];    ///< message data content addres
}cloud_buffer_pkt_msg;

typedef struct _cloud_buffer_list_t
{
    struct cloud_buffer_list_t *next;
    cloud_buffer_pkt_msg *msg;
}cloud_buffer_list_t;

typedef struct
{
	cloud_buffer_list_t *first;
    int count;
} cloud_buffer_list_head_t;

void cloud_init_bufList(cloud_buffer_list_head_t **head);
void cloud_clear_bufList_head(cloud_buffer_list_head_t **head);
int32_t cloud_insert_bufList_node(cloud_buffer_list_head_t *head, cloud_buffer_list_t *node);
cloud_buffer_list_t *cloud_msg_bufList_exists(cloud_buffer_list_head_t *head,int msgid);
cloud_buffer_list_t *cloud_pop_bufList_first_node(cloud_buffer_list_head_t *head);
cloud_buffer_list_t *cloud_pop_bufList_node_byId(cloud_buffer_list_head_t *head, int msgid);


/**
 * @note  配置物联网平台的IP地址,端口号，lifetime等相关服务器配置后向云平台发起注册
 * @brief +XYCONFIG=<cloud_type>,<server_ip>[,<server_port>][,<lifetime>]
 * cloud_type 必填
 * server_ip  必填
 * server_port 选填 缺省默认port=5683
 * lifetime    选填 缺省默认 lifetime =86400
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 * 
 */
int at_XYCONFIG_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 配置注册成功后，通过该命令将数据发送至云平台
 * +XYSEND=<data_type>[,<data_len>][,data][,msg_type]
 * data_type 整型 0:数据发送 1:lifetime更新请求 2:发起注销请求
 * data_len 数据长度 data_type为0时必填
 * data 十六进制格式字符串类型 data_type为0时必填
 * msg_type 数据发送类型0:CON 1:NON data_type为0时必填
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_XYSEND_req(char *at_buf, char **prsp_cmd);


/**
 * @brief +XYRECV  从下行数据缓存链表中读取数据
 * +XYRECV?  查询当前下行数据链表中缓存数据的个数，若无，则查询数量为0
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_XYRECV_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_recv_callback cdpProxyRecvProc_test(uint8_t req_type,uint8_t* paramList,uint8_t **prsp_cmd);

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_send_callback cdpProxySendProc_test(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd);

/**
 * @brief 抽象云AT框架使用及测试示例代码
 * 
 */
proxy_config_callback cdpProxyConfigProc_test(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd);