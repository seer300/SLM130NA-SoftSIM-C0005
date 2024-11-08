/*
 * xy_at_api.h
 */


#pragma once

#include <stdint.h>
#include "at_utils.h"




/*AT命令错误码范围分布：1-500，3GPP相关错误码；500-600，移远BC95扩展错误码；800-900，电信ctwing扩展错误码；余下值皆为私定错误码*/
typedef enum
{
	/*以下-1值的宏，表示当前AT流程处于等待外部条件满足的暂态，需要继续main循环运行并等待最终结果*/
	XY_WAITING_RSP = -1,   //仅存在于异步非阻塞AT命令机制，表示Get_AT_Rsp尚未等待CP核应答结果，需要退出到main主线程继续执行
	XY_ATTACHING = -1,     //表明当前3GPP在努力执行attach动作，此时不能执行云通信
	XY_CPBOOTING = -1,     //表明当前正在加载CP核
	
	XY_OK = 0,       //表明AT处理正常，可以继续往下执行
	XY_FORWARD,
	/*以下错误码为模组自定义的业务错误码，需要与CP核AT命令集和错误码完全对应*/
#if VER_BC95
	XY_ERR = 526,
	XY_ERR_PARAM_INVALID = 512,
	XY_ERR_NOT_ALLOWED = 535,
	XY_ERR_DROP_MORE = 523,
	XY_ERR_DOING_FOTA = 529,   //如果用户想缩短SOFT_WATCHDOG_TIME的监控时长，收到该报错时必须重设足够大的时长，以防止FOTA下载期间看门狗超时
	XY_ERR_MORE_PARAM = 514,
	XY_ERR_WAIT_RSP_TIMEOUT = 514, //当CP核异常时会报超时错误，客户需要进行容错，例如超过2次超时，可以调用dump_info_to_flash接口把CP核异常线程保存到flash中，以供芯翼分析
	XY_ERR_CHANNEL_BUSY = 525,
    XY_ERR_NO_MEM = 514,
	XY_ERR_NOT_NET_CONNECT = 518,
	XY_ERR_CONN_NOT_CONNECTED = 516,
	XY_ERR_INVALID_PREFIX = 514,
#elif VER_BC25
	XY_ERR = 49,
	XY_ERR_PARAM_INVALID = 53,
	XY_ERR_NOT_ALLOWED = 302,
	XY_ERR_DROP_MORE = 58,
	XY_ERR_DOING_FOTA = 160,   //如果用户想缩短SOFT_WATCHDOG_TIME的监控时长，收到该报错时必须重设足够大的时长，以防止FOTA下载期间看门狗超时
	XY_ERR_MORE_PARAM = 24,
	XY_ERR_WAIT_RSP_TIMEOUT = 50, //当CP核异常时会报超时错误，客户需要进行容错，例如超过2次超时，可以调用dump_info_to_flash接口把CP核异常线程保存到flash中，以供芯翼分析
	XY_ERR_CHANNEL_BUSY = 52,
    XY_ERR_NO_MEM = 51,
	XY_ERR_NOT_NET_CONNECT = 332,
	XY_ERR_CONN_NOT_CONNECTED = 332,
	XY_ERR_INVALID_PREFIX = 50,
#elif VER_260Y
	XY_ERR = 301,
	XY_ERR_PARAM_INVALID = 50,
	XY_ERR_NOT_ALLOWED = 55,
	XY_ERR_DROP_MORE = 4,
	XY_ERR_DOING_FOTA = 60,   //如果用户想缩短SOFT_WATCHDOG_TIME的监控时长，收到该报错时必须重设足够大的时长，以防止FOTA下载期间看门狗超时
	XY_ERR_MORE_PARAM = 50,
	XY_ERR_WAIT_RSP_TIMEOUT = 308, //当CP核异常时会报超时错误，客户需要进行容错，例如超过2次超时，可以调用dump_info_to_flash接口把CP核异常线程保存到flash中，以供芯翼分析
	XY_ERR_CHANNEL_BUSY = 302,
    XY_ERR_NO_MEM = 23,
	XY_ERR_NOT_NET_CONNECT = 304,
	XY_ERR_CONN_NOT_CONNECTED = 304,
	XY_ERR_INVALID_PREFIX = 21,
#else
	XY_ERR = 8000,
	XY_ERR_PARAM_INVALID,       
	XY_ERR_NOT_ALLOWED,       
	XY_ERR_DROP_MORE,        
	XY_ERR_DOING_FOTA,  
	XY_ERR_MORE_PARAM, 
	XY_ERR_WAIT_RSP_TIMEOUT,  //当CP核异常时会报超时错误，客户需要进行容错，例如超过2次超时，可以调用dump_info_to_flash接口把CP核异常线程保存到flash中，以供芯翼分析
	XY_ERR_CHANNEL_BUSY, 
    XY_ERR_NO_MEM, 
	XY_ERR_NOT_NET_CONNECT,
	XY_ERR_UNUSED1, 
	XY_ERR_CONN_NOT_CONNECTED, 
	XY_ERR_INVALID_PREFIX,	
	XY_ERR_UNUSED2,    
#endif //VER_BC95

	/*以下错误码为AP核本地发现的错误码*/
	AP_EXTERR_BASE = 9000,
	XY_ERR_IPC_FAIL = AP_EXTERR_BASE,  //双核核间通信异常
	XY_ERR_CP_NOT_RUN,            //仅用于AP核本地检查
	XY_ERR_CP_BOOT , //Boot_CP失败
	XY_ERR_CP_DEAD,  //AP核底层发现CP核挂死
}At_status_type;



/*AP核本地检测出来的CP核相关的异常错误，最终体现到AT命令中，供用户容错*/
extern int g_errno;


/******************************************************************************************************************************
  * @brief	 AP核用户发送AT命令给CP核，仅在宏XY_AT_CTL关闭情况下才有意义，以供客户搭建自己的AT框架
  * @param	 cid   暂无实际意义，填0即可
  * @return	 参见At_status_type定义
  ******************************************************************************************************************************/
 /******************************************************************************************************************************
  * @brief	 AP core user sends AT command to CP core. API is used by XY_AT_CTL closed, so that customers can build their own AT framework
  * @param	 cid:	No practical significance, just fill in 0
  * @return  see  At_status_type
  ******************************************************************************************************************************/
int at_uart_write(int cid,uint8_t *atstr,int len);


/******************************************************************************************************************************
  * @brief	 通过该接口读取CP核传递过来的AT命令。仅在宏XY_AT_CTL关闭情况下才有意义，以供客户搭建自己的AT框架
  * @param	 cid   暂无实际意义，填0即可
  * @param	 atstr   接收缓存，由调用者管理内存空间
  * @param	 len     接收缓存字节长度
  * @return	 0表示未读取到数据,正值表示读取到的有效长度
  * @warning 如果有多条AT命令被读取到，每条AT命令之间会有‘\0’，进而慎用string库函数,如strlen。
  * @note    接口内部按照入参内存大小进行遍历拷贝读取，如果缓存的多条AT命令不能一次读完，用户需要多次调用该接口进行读取，直至返回0为止。
  ******************************************************************************************************************************/
 /******************************************************************************************************************************
  * @brief	 Read AT command passed from CP core and process through this API. 
  *          API is used by XY_AT_CTL closed, so that customers can build their own AT framework
  * @param	 cid:	No practical significance, just fill in 0
  * @param	 atstr: Receive cache, memory space managed by the caller
  * @param	 len: Receive Cache Byte Length
  * @return	 0 means not read, and a positive value means the effective length read
  * @warning If multiple AT commands are read, there will be '\0' between each AT command, so use string library functions with caution, such as "strlen".
  ******************************************************************************************************************************/
int at_uart_read(int cid,uint8_t *atstr,int len); 



/******************************************************************************************************************************
  * @brief	 清空与CP核AT通信相关的状态机信息，通常与Stop_CP和Force_Stop_CP一起执行
  ******************************************************************************************************************************/
 /******************************************************************************************************************************
  * @brief	Clear the state machine information related to CP core AT communication, usually with Stop_CP and Force_Stop_CP execute together
  ******************************************************************************************************************************/
void clear_at_info();


char *at_err_build(uint16_t err_no);

