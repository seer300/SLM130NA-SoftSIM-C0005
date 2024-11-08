/*****************************************************************************************************************************
 * @brief 对于客户常用的CP核相关能力的接口化实现，内部通过阻塞式同步AT接口进行实现，进而可能造成main主线程被阻塞住。
 *        用户也可以仿造接口实现，自行进行相关AT命令的API接口化实现。
*****************************************************************************************************************************/
/*****************************************************************************************************************************
 * @brief For the interface implementation of CP core related capabilities commonly used by customers.
 * API is implemented through the blocking synchronous mechanism, which may cause the main thread to be blocked some time.
 * The user can also program some AT interfaces himself by refering to these APIs.
*****************************************************************************************************************************/

#pragma once

#include "at_process.h"



extern int g_tcpip_ok;
extern uint64_t g_cgatt_start_tick;


/*****************************************************************************************************************************
 * @brief  慎用！超时等待TCPIP网路建立成功，会阻塞main主线程一段时间，造成其他事件不能及时处理
 * @param  wait_sec 单位秒，0表示立即返回
 * @return @ref At_status_type
 * @warning 若超时失败，从功耗角度考虑，建议客户调用Stop_CP接口强行停止3GPP，以防止功耗过高
 *****************************************************************************************************************************/
 /*****************************************************************************************************************************
 * @brief  Use with caution!!!Timeout waiting for TCPIP network to be established successfully
 * @param  wait_sec Unit is seconds, 0 means immediate return
 * @return see At_status_type
 * @warning If API fails timeout, it is recommended that user call Stop_CP to prevent excessive power consumption
 *****************************************************************************************************************************/
At_status_type xy_wait_tcpip_ok(int wait_sec);


/*****************************************************************************************************************************
 * @brief  异步方式查询等待TCPIP网路建立成功，不会阻塞在该函数内部，超时后会返回XY_ERR_NOT_NET_CONNECT
 * @param  wait_sec 单位秒，0表示立即返回
 * @return @ref At_status_type
 * @warning 若超时失败，从功耗角度考虑，建议客户调用Stop_CP接口强行停止3GPP，以防止功耗过高
 *****************************************************************************************************************************/
At_status_type xy_wait_tcpip_ok_asyc(int wait_sec);


