#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
 #include "xy_at_api.h"
 #include "at_config.h"
 #include "xy_utils.h"
 
/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
* @brief farps at命令输出函数声明
* @param at_ctx   at命令上下文，@see @ref at_context_t;
* @param at_rsp   at_cmd progress result or response;
* @param rsp_size size of at_cmd progress result or response;
*/
typedef bool (*farps_write_func)(void* at_ctx, void *at_rsp, int rsp_size);

enum at_proc_state
{
	REQ_IDLE = 0x0,
	SEND_REQ_NOW,  //标明当前at上下文对应的at命令已经投递给PS，尚未处理完成，需等待PS返回。
	RCV_REQ_NOW,   //标明当前at上下文对应的at命令已经接收，尚未处理完成。
};

enum at_context_position
{
	NEAR_PS = 0,
	FAR_PS,
};

typedef struct at_urc_cache
{
	char* urc;
	uint32_t urc_size;
	struct at_urc_cache* next;
} at_urc_cache_t;

typedef struct at_context
{
	struct at_context *fwd_ctx;			//记录需要切换的at上下文，比如投递给ps的at命令需要记录当前的命令来自uart还是ap
	char *g_farps_rcv_mem;				//接收的at数据起始地址
	int g_have_rcved_len;				//已经接收的at数据长度
	int g_rcv_buf_len;					//分配用于接收at数据的buffer大小,非已接收的数据实际长度。uart接收以128字节为单位分配缓存大小
	ser_req_func at_proc;              	//代理线程处理的命令的对应函数
	farps_write_func farps_write;      	//farps 回写接口,根据不同外设赋值
	osMessageQueueId_t user_queue_Id;	//用户线程创建的队列id,用于at_ReqAndRsp_to_ps接口
	osTimerId_t user_ctx_tmr;			//用户线程软定时器ID，,用于at_ReqAndRsp_to_ps接口
	char *at_param;						//at参数指针
	at_urc_cache_t* urcData;			//缓存的URC数据
	osMutexId_t urcMutex;
	int error_no;						//at命令错误编号
	int fd;								//at上下文fd,指示AT命令发起源, @see @AT_SRC_FDPS
	uint8_t at_type;					//at命令类型    @see @ref AT_REQ_TYPE
	uint8_t position:4;					//at上下文位置  @see @ref at_context_position
	uint8_t state:4;					//at命令状态    @see @ref at_proc_state
	uint8_t retrans_count;				//命令重发计数，主要用于用户线程并发调用情况下,在当前PS命令尚未处理完成情况下，新的PS命令可以缓存重发
	char at_cmd_prefix[AT_CMD_PREFIX];	//letter string XXX,such as "WORKLOCK"/"AT"/"ATI"....
} at_context_t;

typedef struct at_context_dict
{
	struct at_context_dict *pre;
	at_context_t *node;
	struct at_context_dict *next;
} at_context_dict_t;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern at_context_t nearps_ctx;   	//cp at context
extern at_context_t log_ctx;		//log at context
extern at_context_t user_app_ctx;	//user response at context
extern at_context_t ap_async_ctx;	//ap core async at context
extern at_context_t ap_sync_ctx;	//ap core sync at context
extern at_context_t ap_ext_ctx;		//ap core ext at context
extern at_context_t ap_ble_ctx;		//ap core ble at context
extern at_context_dict_t *g_at_context_dict;  //全局链表，记录当前系统存在的所有at_context,方便查询删除操作和问题定位
extern osMutexId_t at_ctx_dict_m;   //全局链表互斥锁，保证增删查操作同步

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 重置at上下文,如at命令状态，at命令类型等信息
 * @param ctx [IN] 指定的at上下文，如uart_ctx/ap_async_ctx 
 * @note at参数指针指向的内存必须在此释放
 */
void reset_ctx(at_context_t *ctx);

/**
 * @brief 注册一个at上下文，并将新的上下文记录到全局链表g_at_context_dict中，方便查询删除操作
 * @param ctx [IN] 指向需要注册的at上下文，对应上下文由用户通过xy_malloc创建
 * @return 	XY_OK: 注册成功, XY_ERR:注册失败
 * @warning	相同fd的上下文同一时间只能注册一个，再次尝试注册会返回XY_ERR
 */
bool register_at_context(at_context_t *ctx);

/**
 * @brief 根据给定fd去注册对应的at上下文，并将上下文记录从全局链表g_at_context_dict中删除
 * @param fd [IN] at上下文fd @see @ref AT_SRC_FD
 * @return XY_OK:去注册成功, XY_ERR:去注册失败
 * @note  未找到与给定fd相匹配的at上下文则返回XY_ERR
 * @warning 该接口内部会调用free释放at上下文！！！
 */
bool deregister_at_context(int fd);

/**
 * @brief 根据给定fd在全局链表g_at_context_dict中查找对应的at上下文
 * @param fd [IN] at上下文fd @see @ref AT_SRC_FD
 * @return 找到后返回对应at上下文地址，否则返回NULL
 */
at_context_t *search_at_context(int fd);

/**
 * @brief   获取可用的at上下文,主要用于at_ReqAndRsp_to_ps接口
 * @param from_proxy [IN]  1:调用者是xy_proxy线程, 0: 其他线程
 * @return  返回NULL表示无可用at_context，否则会malloc一个at上下文并返回其地址
 * @note   可用的user at上下文数受FARPS_USER_MIN和FARPS_USER_MAX宏控制
 * 如果当前并发执行的user at上下文数>=FARPS_USER_MAX，再次尝试调用接口会返回NULL
 * @warning 获取at上下文成功时接口内部会malloc at_context,需在外部调用deregister_at_context接口释放
 */
at_context_t* get_avail_atctx_4_user(int from_proxy);

/**
 * @brief  通过软定时器id获取对应的队列ID
 * @param timerId  [IN]软定时器id
 * @return  成功返回对应的队列ID,失败返回NULL
 * @note 仅用于at_ReqAndRsp_to_ps接口
 */
osMessageQueueId_t at_related_queue_4_user(osTimerId_t timerId);

