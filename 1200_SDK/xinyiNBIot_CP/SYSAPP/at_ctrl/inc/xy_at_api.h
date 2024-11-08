/** 
* @file        xy_at_api.h
* @brief       用户开发使用的AT相关接口及定义
* @warning     不经同意用户不得修改at框架中的任何代码，也不得调用非该头文件的任何接口！
*/
#pragma once
#include <ctype.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "at_utils.h"
#include "cmsis_os2.h"
#include "xy_system.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
* @brief AT process result macro
* @param AT_OK			         success
* @param ATERR_XY_ERR			 fatal error need reset
* @param ATERR_PARAM_INVALID     param  invalid
* @param ATERR_NOT_ALLOWED       operate not allow
* @param ATERR_DROP_MORE		 because sleep,at string head drop too more
* @param ATERR_DOING_FOTA		 For the duration of FOTA,cannot proc any AT req
* @param ATERR_MORE_PARAM        too more param,can not parase this param,only used for strict check
* @param ATERR_WAIT_RSP_TIMEOUT  wait AT response result timeout
* @param ATERR_CHANNEL_BUSY  	 now have one req at working,receive new  req at 
* @param ATERR_NO_MEM            heap memory not enough
* @param ATERR_NOT_NET_CONNECT   NB PS can not PDP activate within the agreed time
* @param ATERR_NEED_OFFTIME      extern MCU power off XY Soc,and not send time bias extensional AT CMD when power up again
* @param ATERR_CONN_NOT_CONNECTED  connecting socket be resetted by server for unknown case,user must close current socket
* @param ATERR_INVALID_PREFIX 	 invalid prefix
* @param ATERR_LOW_VOL           voltage  too low,and SoC can not work normal
* @param USER_EXTAND_ERR_BASE    user can add self  err number from  here!
* @warning  错误码的分布：1-500，3GPP错误码；500-600，移远扩展AT错误码；800-900，ctwing扩展AT错误码；其它值皆为用户自定义错误码
*/
#if VER_BC95

typedef enum 
{
	AT_OK = 0,

	ATERR_XY_ERR 				= 526,
	ATERR_PARAM_INVALID			= 512,		//8001
	ATERR_NOT_ALLOWED			= 535,      //8002
	ATERR_DROP_MORE				= 523,  	//8003
	ATERR_DOING_FOTA			= 529,    	//8004
	ATERR_MORE_PARAM			= 514,   	//8005
	ATERR_WAIT_RSP_TIMEOUT		= 514,      //8006
	ATERR_CHANNEL_BUSY			= 525,		//8007
	ATERR_NO_MEM				= 514,		//8008
	ATERR_NOT_NET_CONNECT		= 518,      //8009
	ATERR_CONN_NOT_CONNECTED	= 516,    	//8011
	ATERR_INVALID_PREFIX		= 514,	    //8012

	USER_EXTAND_ERR_BASE = 9000,
}AT_ERRNO_E;

#elif VER_BC25
typedef enum 
{
	AT_OK = 0,

	ATERR_XY_ERR 				= 49,
	ATERR_PARAM_INVALID			= 53,		//8001
	ATERR_NOT_ALLOWED			= 302,      //8002
	ATERR_DROP_MORE				= 58,  	//8003
	ATERR_DOING_FOTA			= 160,    	//8004
	ATERR_MORE_PARAM			= 24,   	//8005
	ATERR_WAIT_RSP_TIMEOUT		= 50,      //8006
	ATERR_CHANNEL_BUSY			= 52,		//8007
	ATERR_NO_MEM				= 51,		//8008
	ATERR_NOT_NET_CONNECT		= 332,      //8009
	ATERR_CONN_NOT_CONNECTED	= 332,    	//8011
	ATERR_INVALID_PREFIX		= 50,	    //8012

	USER_EXTAND_ERR_BASE = 9000,
}AT_ERRNO_E;

#elif VER_260Y
typedef enum 
{
	AT_OK = 0,

	ATERR_XY_ERR 				= 301,
	ATERR_PARAM_INVALID			= 50,		//8001
	ATERR_NOT_ALLOWED			= 55,      //8002
	ATERR_DROP_MORE				= 4,  	//8003
	ATERR_DOING_FOTA			= 60,    	//8004
	ATERR_MORE_PARAM			= 50,   	//8005
	ATERR_WAIT_RSP_TIMEOUT		= 308,      //8006
	ATERR_CHANNEL_BUSY			= 302,		//8007
	ATERR_NO_MEM				= 23,		//8008
	ATERR_NOT_NET_CONNECT		= 304,      //8009
	ATERR_CONN_NOT_CONNECTED	= 304,    	//8011
	ATERR_INVALID_PREFIX		= 21,	    //8012

	USER_EXTAND_ERR_BASE = 9000,
}AT_ERRNO_E;
#else

typedef enum 
{
	AT_OK = 0,

	ATERR_XY_ERR = 8000,
	ATERR_PARAM_INVALID,         //8001
	ATERR_NOT_ALLOWED,           //8002
	ATERR_DROP_MORE,             //8003
	ATERR_DOING_FOTA,            //8004
	ATERR_MORE_PARAM,            //8005
	ATERR_WAIT_RSP_TIMEOUT,      //8006
	ATERR_CHANNEL_BUSY,          //8007
	ATERR_NO_MEM,				 //8008
	ATERR_NOT_NET_CONNECT,		 //8009
	ATERR_UNUSED1,               //8010
	ATERR_CONN_NOT_CONNECTED,    //8011
	ATERR_INVALID_PREFIX,	     //8012
	ATERR_UNUSED2,               //8013

	USER_EXTAND_ERR_BASE = 9000,
}AT_ERRNO_E;

#endif //VER_BC95

/**
* @brief at_basic_req中注册的AT请求处理函数的返回值
*/
typedef enum 
{
	AT_END = 0,      /*at_basic_req的注册函数中直接正确处理完毕时，返回该值，由框架组装"\r\nOK\r\n"发送给外部MCU*/
	AT_ASYN,         /*at_basic_req的注册函数中不能直接处理的，一般通过消息发送给业务线程，最终由业务线程调用send_rsp_at_to_ext接口发送结果码*/
	AT_FORWARD,      /*用户禁用！通常用于3GPP相关AT命令的拦截处理，处理完毕后还要继续转发给3GPP处理*/
	AT_ROUTE_MAX,    /*at_basic_req的注册函数中，处理失败则返回错误码，且大于该宏值，则由AT框架直接组装“+ERROR:”应答字符串*/
}AT_PROCESS_RLT_E;

/**
* @brief AT动作的种类，用全局g_req_type来记录
*/
typedef enum AT_REQ_TYPE
{
	AT_CMD_INVALID = 0,
	AT_CMD_REQ, 	//AT+XXX=param
	AT_CMD_ACTIVE,	//AT+XXX,not include param
	AT_CMD_QUERY,	//AT+XXX?
	AT_CMD_TEST,	//AT+XXX=?
} AT_REQ_TYPE_E;

/**
 * @brief at请求类型, @see @ref AT_REQ_TYPE_E
 */
extern char g_req_type;

/**
* @brief  请求类AT命令的注册回调函数声明
* @param  at_params [IN] AT请求的参数头指针，例如 "1,5,CMNET"
* @param  rsp_cmd   [OUT] 由具体解析函数内部申请内存，并填充应答结果字符串；对于简单的“OK”“ERROR”类应答，建议直接返回结果码，交由框架组AT应答命令
* @return 解析处理结果 @see @ref AT_PROCESS_RLT_E
* @note   rsp_cmd的内存由解析函数内部申请，由AT框架进行内存释放。如果没有中间结果要外发，不建议使用该参数。
*/
typedef int (*ser_req_func)(char *at_params, char **rsp_cmd);


/**
 * @brief  仅用于通过AT口发送调试类URC主动上报，正常功能开发禁用！
 * @param data [IN] 调试的URC信息，例如 "\r\n+DBGINFO:NV ERROR\r\n"
 * @warning  执行AT+NV=SET,CLOSEDEBUG,0后才会输出，该接口仅用于发送调试信息，不得发送正常功能性AT命令，
 */
void send_debug_by_at_uart(char *buf);

/**
 * @brief  用于发送中间结果和结果码给外部MCU。仅用于异步应答结果的发送，即业务线程的应答结果发送。
 * @param  data [IN] 应答结果字符串，如"\r\nOK\r\n"，"\r\nNO CARRIER\r\n\r\nOK\r\n"，"\r\n+MIPLOBSERVE:3,0,1,-1\r\n"
 * @note   ser_req_func类型的注册函数内部，严禁调用该接口，而应该是申请rsp_cmd内存空间，并赋值应答字符串后，交由AT框架发送出去
 * @note   仅URC上报推荐使用send_urc_to_ext接口，执行效率更高
 */
void send_rsp_at_to_ext(void *data);

/**
 * @brief 普通的URC或码流的发送接口，支持URC缓存，不经过at_ctl主框架
 * @param  data [IN] 需要输出的数据首地址
 * @param  size [IN] 数据长度
 * @note  不能够发送含"\r\nOK\r\n"结果码的字符串，因为结果码必须经at_ctl主框架，可使用send_rsp_at_to_ext。
 * @warning 仅LPUART和AP核异步AT命令两个通道支持URC缓存
 * @warning 内部识别尾部未加"\r\n"时，会由后台框架自动在头尾添加"\r\n"
 */
void send_urc_to_ext(void* data, uint32_t size);

/**
 * @brief send_urc_to_ext的精简版
 * @param  data [IN] 需要输出的数据首地址
 * @note  不能够发送含"\r\nOK\r\n"结果码的字符串，因为结果码必须经at_ctl主框架，可使用send_rsp_at_to_ext。
 * @warning 内部识别尾部未加"\r\n"时，会由后台框架自动在头尾添加"\r\n"
 */
void send_urc_to_ext2(void* data);


/**
 * @brief 仅用于特殊的URC或透传数据的发送，如"REBOOTING""RESETING"等，不支持URC缓存，不经过at_ctl主框架
 * @param  data [IN] 需要输出的数据首地址
 * @param  size [IN] 数据长度
 * @note  与send_urc_to_ext的差异仅在于是否支持URC缓存，即在有AT命令正在处理过程中，是否容许URC主动上报发送出去。
 *        对于普通的AT命令处理，建议URC缓存，待完成AT请求后再发送，以免对底板MCU流程产生干扰；但对于紧急上报、重启上报等特殊URC，不建议缓存。
 */
void send_urc_to_ext_NoCache(void* data, uint32_t size);

/**
 * @brief  按照fmt格式解析每个参数值，类似scanf格式
 * @param fmt  [IN] AT命令参数对应的格式化字符串,其中()表示必选参数，[]表示可选参数。具体参数类型有：
 *
 *       %d,%1d,%2d分别对应int，char，short整形参数,支持16进制和10进制两类数值;后面还可以添加()表示必选参数，[]表示可选参数；括号内部可以使用-来指定参数值的上下限；
 *       如%d(0-100)表示可选4字节整形参数，取值范围为0到100，若解析该参数时发现不在此范围，会报错。括号内可以使用|来指定离散取值，
 *       如%d(0|0x20|0x100)表示可选4字节整形参数，取值为0/0x20/0x100三个值中的一个，若解析该参数时发现不是这三个值中的一个，会报参数错。
 *
 *       %s，对应字符串类型的参数；中间可以携带数字指示字符串缓存的内存大小，如%10s，若真实字符串长度大于9，则会报参数错误；
 *      
 *       %p，对应长字符串类型的参数，将对应长字符串首地址赋值给对应的参数，以供解析方直接使用；可单独使用，也可搭配%l使用
 *
 *       %h，对应16进制字符串类型的参数，接口内部会将16进制码流字符串转换成码流；一般搭配%l使用
 *
 *       %l，用法同%d，指示%p或%h对应字符串的传输长度，通常用于长度合法性检查；最多只能有一个%l，优先配合%h使用，若无%h，则配合%p使用，
 *
 * @param buf  [IN] AT字符串参数首地址，如"2,5,cmnet"
 * @param va_args ... [IN/OUT] 每个参数的地址空间，参数个数与fmt中的个数保持一致
 * @note 
 * @warning   该接口类似scanf形式，通过灵活的fmt格式化，来达到参数检错和解析一步到位，简化具体AT命令的开发。
 */
AT_ERRNO_E at_parse_param(char *fmt, char *buf, ...);

/**
 * @brief  仅用于含转义字符串参数的解析，通常用于http等特殊命令中,由于支持字符串中转义字符解析，输入的字符串必须用""包含!!!
 * @param fmt  [IN] 同at_parse_param函数的使用
 * @param buf  [IN] 待解析的字符串头地址,例如 "2,5,cmnet"
 * @param parse_num  [OUT] 实际解析的参数个数，例如AT+HTTPHEADER=1,"" parse_num=2，而AT+HTTPHEADER=1  parse_num=1
 * @return 解析结果 参考@AT_ERRNO_E
 * @note 字符串转义字符解析示例: "\101xinyin\x42" => "AxinyiB"
 * @note 字符串转义字符解析示例: "\r\nxinyi\?" = "'\r''\n'xinyi?" 原先字符串中\r占用两个字节，解析成功后转为'\r' ACSII字符占用一个字节
 */
AT_ERRNO_E at_parse_param_escape(char *fmt, char *buf, int *parse_num, ...);

/**
 * @brief  不区分大小写的字符串比较，常用于AT命令参数解析时，字符串参数的识别；例如“IPV6” “ipv6”
 */
bool at_strcasecmp(const char *s1, const char *s2);

/**
 * @brief  将系统错误码转换为AT命令错误码
 * @param err_code  [IN] 系统错误码,see @ref xy_ret_Status_t
 * @return  AT返回值,see @ref AT_ERRNO_E
 */
AT_ERRNO_E at_get_errno_by_syserr(xy_ret_Status_t err_code);

/**
 * @brief drx/edrx唤醒场景下测试功耗时需屏蔽主动上报，减少唤醒深睡的流程时间
 */
bool is_urc_drop(void);


