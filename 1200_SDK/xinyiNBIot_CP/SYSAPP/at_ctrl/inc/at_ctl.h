#pragma once

/*******************************************************************************
 *							             Include header files							                 *
 ******************************************************************************/
#include "at_context.h"
#include "main_proxy.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
/**
* @brief 根据at_context 发送AT错误信息到对应的外设，该接口会执行reset context操作
* @param err_no  错误码，8007、8003 @see AT_ERRNO_E
* @param ctx  at_context上下文
* @note 用于at_ctl内部出错处理，需要执行reset context的场景
*/
#define AT_ERR_BY_CONTEXT(err_no, ctx)   send_at_err_to_context(err_no, ctx, __FILE__, __LINE__)

/**
* @brief 直接发送AT错误信息到对应的外设，该接口不会执行reset context操作
* @param err_no  错误码，8007、8003 @see AT_ERRNO_E
* @param ctx  at_context上下文
* @note 一般用于farps中的处理，不执行reset context可以避免因多线程操作全局context导致返回的error信息不正确
*/
#define SEND_ERR_RSP_TO_EXT(err_no, ctx) send_err_rsp_2_ext(err_no, ctx, __FILE__, __LINE__)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct at_msg
{
	at_context_t *ctx;
	int msg_id;       //@see @ref AT_MSG_ID
	int size;
  	int offset;       //at参数偏移长度
  	char data[0];
} at_msg_t;

/*用于封装AP与CP间发送的at数据*/
typedef struct message_data
{
    void *buf;
    unsigned int size;
} zero_msg_t;

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern osMessageQueueId_t at_farps_q;
extern struct at_serv_proc_e *g_at_basic_req;
extern at_context_t * g_at_proxy_ctx;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief at模块初始化接口,在main函数中调用
 * @note 初始化at模块资源，如全局的at上下文(uart/ble/nearps at上下文),互斥锁,ps回调注册以及创建at_ctrl线程
 */
void at_init();

/**
 * @brief at proxy模块初始化接口,在at_init函数中调用
 */
void at_proxy_init(void);

/**
 * @brief 发送at数据到at_ctrl线程 
 * @param msg_id [IN] 消息ID @see @ref AT_MSG_ID
 * @param data   [IN] at数据地址
 * @param size   [IN] at数据长度，不包括'\0'
 * @param ctx    [IN] at上下文 @see @ref at_context_t
 * @return return AT_OK, or return ATERR_NOT_ALLOWED
 */
int send_msg_2_atctl(int msg_id, void *data, int size, at_context_t* ctx);

/**
 * @brief 转发at命令到其他at上下文中
 * @param msg [IN] at消息 @see @ref at_msg_t
 * @note 一般用于将at命令转给3gpp处理的场景
 */
 void forward_req_at_proc(at_msg_t *msg);

 /**
 * @brief 发送错误信息到指定at上下文，该接口会执行reset context操作
 * @param  err_no [IN] 错误码 @see @ref AT_ERRNO_E
 * @param  ctx  [IN] at上下文地址, @see @ref at_context_t
 * @param  file [IN] 文件名, 一般使用__FILE__
 * @param  line [IN] 行号, 一般使用__LINE__
 * @note
 */
void send_at_err_to_context(int errno, at_context_t *ctx, char *file, int line);

/**
 * @brief 处理ap核发送过来的at数据
 * @param at_fd [IN] at上下文句柄
 * @param buf [IN] at数据地址
 * @param len [IN] at数据长度
 */
void at_recv_from_ap(AT_SRC_FD at_fd, char *buf, unsigned int len);

/**
 * @brief 处理uart发送过来的at数据
 * @param buf [IN] at数据
 * @param len [IN] at数据长度
 * @note uart发送过来的at数据受uart fifo影响，一次最大传输128字节
 */
void at_recv_from_log(uint32_t id,char *buf, unsigned int data_len);


/**
 * @brief 发送at数据给ap,由at_context farps_write调用,内部在处理at命令输出前会进行一些特殊处理，如重置at上下文，根据需求过滤urc信息功能等
 * @param ctx  [IN] at命令上下文, @see @ref at_context_t
 * @param buf  [IN] 需要输出的数据，字符串形式
 * @param size [IN] 输出的数据大小，不包含'\0'
 * @note  调用者需手动释放buf指向的数据
 * @warning AP核本地的同步和异步两个虚拟AT通道的发送接口
 */
bool at_send_to_ap(void* ctx, void *buf, int size);

/**
 * @brief 发送at数据给log,由at_context farps_write调用,内部在处理at命令输出前会进行一些特殊处理
 * 如重置at上下文，根据需求过滤urc信息，重新打开standby功能等
 * @param ctx  [IN] at命令上下文, @see @ref at_context_t
 * @param buf  [IN] 需要输出的数据，字符串形式
 * @param size [IN] 输出的数据大小，不包含'\0'
 * @note  调用者需手动释放buf指向的数据
 */
bool at_send_to_log(void* ctx, void *buf, int size);

/**
 * @brief  通过指定at上下文发送at数据
 * @param ctx [IN] at上下文, @see @ref at_context_t
 * @param buf [IN] at数据地址
 * @param size [IN] at数据长度，不包含'\0'
 * @return  XY_OK: 写成功, XY_ERR: 写失败
 */
bool at_write_by_ctx(at_context_t *ctx, void *buf, int size);

/**
 * @brief 接收nearps端发送过来的at数据，共核模式下，nearps为NAS
 * @param buf [IN] at数据地址
 * @param len [IN] at数据长度，不包含'\0'
 * @note
 */
void at_rcv_from_nearps(void *buf, unsigned int len);


/**
 * @brief 发送错误信息到外设，该接口不会执行reset context操作
 * @param  err_no [IN] 错误码 @see @ref AT_ERRNO_E
 * @param  ctx [IN] at上下文地址，@see @ref at_context_t
 * @param  file [IN] 文件名, 一般使用__FILE__
 * @param  line [IN] 行号, 一般使用__LINE__
 */
void send_err_rsp_2_ext(int err_no, at_context_t* ctx, char *file, int line);

/**
 * @brief  发送AT命令给AP核，仅用于OPENCPU形态的AP核本地客户端。模组形态，CP核直接写LPUART，该函数立即返回
 * @param  at_fd [IN] AT通道fd
 * @param  buf [IN] at数据
 * @param  len [IN] at数据的长度
 * @note  数据拷贝->地址转换->shm_msg_write
 */
void at_write_to_AP(AT_SRC_FD at_fd, char *buf, unsigned int len, uint8_t isResult);

/**
 * @brief  核间线程预处理ap侧发送过来的at数据，地址转换->数据拷贝->通知AP释放内存
 * @param  icm_id[IN] icm通道，通过异步AT接口和lpuart发的AT命令走ICM_AT通道，通过同步AT接口发的AT命令走ICM_AT_SYNC通道
 * @param  recv_buf[IN] 封装好的AP侧发给CP的at数据
 */
void icm_at_msg_recv(unsigned int icm_id, zero_msg_t *recv_buf);

/**
 * @brief  缓存AT URC数据，该功能默认开启
 * @param  ctx at上下文
 * @param  urc [IN] 主动上报的AT数据
 * @param  size [IN] 数据长度
 * @note AT命令执行期间，缓存期间上报的URC数据，直到当前AT命令处理完成后，将缓存的URC数据统一上报
 * @warning 仅LPUART和AP核异步AT命令两个通道支持URC缓存
 */
void at_add_urc_cache(at_context_t *ctx, char *urc, uint32_t size);

/**
 * @brief 上报缓存的 URC数据，该功能默认开启
 * @param ctx at上下文
 * @note AT命令执行期间，缓存期间上报的URC数据，直到当前AT命令处理完成后，将缓存的URC数据统一上报
 * @warning 仅LPUART和AP核异步AT命令两个通道支持URC缓存
 */
void at_report_urc_cache(at_context_t *ctx);

/**
 * @brief 发送at消息到at proxy线程，一般用于基础平台及用户定义的at命令处理
 * @param  id [IN] 消息ID @see @ref AT_PROXY_MSG_ID
 * @param  buff [IN] 传递的数据地址
 * @param  len [IN] 传递的数据长度
 * @note
 */
void send_msg_2_at_proxy(int id, void *buff, int len);

/**
 * @brief 注册ps urc回调，该接口必须在main函数中执行
 */
void ps_urc_register_callback_init();

