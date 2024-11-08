#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
 #include "xy_at_api.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
//指示AT后面的标识符，通常为+
#define  IS_HEAD_TAG(a) (a == '+' || a == '^' || a == '&' || a == '#' || a == '*')

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
struct at_serv_proc_e
{
	char *at_prefix;
	ser_req_func proc;
};

struct at_fifo_msg
{
	char data[0];
};

typedef enum AT_PARAM_PARSE_FLAG
{
	AT_PARAM_PARSE_DEFAULT = 0,  /*常规字符*/
	AT_PARAM_PARSE_ESC,  /*转义字符*/
} AT_PARAM_PARSE_FLAG_E;

/*******************************************************************************
 *						  Global variable declarations						   *
 ******************************************************************************/
extern int g_NITZ_mode;
extern uint8_t g_CombCmd_Doing;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 获取AT请求命令的前缀和参数首地址，仅内部使用！
 * @param at_cmd [IN] at cmd data
 * @param at_prefix [OUT] 仅返回AT命令前缀中有效字符串，不携带头尾标识，如“NRB” “ATI” “AT” "WORKLOCK"等
 * @param type [OUT] 返回AT请求命令的类型，@see @ref AT_REQ_TYPE
 */
char *at_get_prefix_and_param(char *at_cmd, char *at_prefix, uint8_t *type);

/**
 * @brief 获取AT URC消息的前缀和参数首地址，返回：后的参数首地址
 * @param at_cmd [IN] at cmd data
 * @param at_prefix [OUT] 仅返回AT命令前缀中有效字符串，不携带头尾标识，如“NRB” “ATI” “AT” "WORKLOCK"等
 */
char *at_get_prefix_for_URC(char *at_cmd, char *at_prefix);


/**
 * @brief 判断是否是短信at命令, 如CMGS/CMGC/CNMA
 * @param at_prefix [IN] at命令前缀
 */
bool is_sms_atcmd(char *at_prefix);

/**
 * @brief  该接口仅内部函数调用，参数解析以可变入参方式提供，类似scanf
 */
int parse_param(char *fmt_param, char *buf, int is_strict, int *arg, int flag, va_list *ap);

/**
 * @brief 用于输出平台AT命令到串口
 * @note 该接口为PS定制，当PS该收到AT+CLAC命令是调用接口，返回芯翼支持的所有AT命令列表
 */
void send_at_cmd_list();

/**
 * @brief  指示是否有Uart口发送过来的AT请求正在处理，以实现AT处理过程中，不发送URC，先缓存着
 */
bool is_at_cmd_processing();

/**
 * @brief  根据错误码组应答字符串
 */
char *get_at_err_string(int error_number);

/**
 * @brief  at框架获取上电原因，返回值为string 
 */
char *at_get_power_on_string();

/**
 * @brief  at框架获取上电原因，返回值为int型 @see @ref AT_REBOOT_CAUSE_E
 */
int at_get_power_on();

/**
 * @brief  设置回显模式，上电初始化和收到ATE命令时设置
 */
void set_echo_mode(uint8_t mode);

/**
 * @brief  判断当前是否回显模式
 */
bool is_echo_mode();

/**
 * @brief  设置错误显示模式，上电初始化和收到AT+CMEE命令时设置
 */
void set_cmee_mode(uint8_t mode);

/**
 * @brief  设置lpuart at命令处理状态，0：收到结果吗，处理结束，1: AP核收到已经执行投递到CP核
 */
void set_at_lpuart_state(uint8_t state);

/**
 * @brief  AP核lpuart收到的AT命令是否正在处理
 */
bool is_at_lpuart_doing();

/*RF专用*/
int at_READNV_req(char *at_buf, char **prsp_cmd);
int at_READRFNV_req(char *at_buf, char **prsp_cmd);
int at_rf_mt(char *at_buf, char **prsp_cmd);
int at_qrf_mt(char *at_buf, char **prsp_cmd);
int at_QRFTESTMODE_req(char *at_buf, char **prsp_cmd);

