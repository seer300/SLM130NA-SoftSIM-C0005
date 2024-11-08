#pragma once

#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define PASSTHR_BLOCK_LEN           512
#define PASSTHR_CTRLZ               0x1A
#define PASSTHR_ESC                 0x1B
#define PASSTHR_PPP_QUIT_SYMBOL     "+++"
#define PASSTHR_THD_PROI            osPriorityNormal
#define PASSTHR_THD_STACKSIZE       osStackShared

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
  * @brief   用户实现的透传数据处理回调函数声明
  * @warning 用户需自行申请内存存放及处理透传数据，并在退出时及时释放内存
  */
typedef void (*app_passthrough_proc)(char *buf, uint32_t len);

/**
  * @brief  用户实现的透传退出客制化回调函数声明
  * 用户可在此接口内做内存释放及一些客制化需求，比如部分透传需要退出后上报"NO CARRIER"，部分透传需要上报"EXIT"信息，可在此实现
  */
typedef void (*app_passthrough_exit)(void);

/**
  * @brief  AT通道传输透传数据的回调函数声明,目前设定是xy_passthr_data_proc_hook,用户无需关注
  */
typedef bool (*at_passthrough_hook)(char *data, uint32_t data_len);

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern at_passthrough_hook g_at_passthr_hook;   //AT通道传输透传数据的回调函数
extern char *passthr_rcv_buff;                  //用于透传的AT命令的数据缓存
extern uint32_t passthr_rcvd_len;               //指示当前已接收到多少字节的透传数据
extern uint32_t passthr_fixed_buff_len;         //指示需要接收的透传数据总长度，仅针对指定透传长度的AT命令有效
extern uint32_t passthr_dynmic_buff_len;        //动态分配的buffer数据总长度

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief  用户关注！收到切换为透传模式AT指令后，调用该接口切换为透传模式接收状态
 * @param  proc [IN] 用户实现的透传数据处理回调函数,该参数不能为空
 * @param  exit [IN] 用户实现的透传退出客制化回调函数
 * @note   该接口内部实现不建议客户做任何修改！！
 */
void xy_enterPassthroughMode(app_passthrough_proc proc, app_passthrough_exit exit);

/**
 * @brief  切换回AT命令模式，该接口调用点为完成透传数据的传递后，或者接收超时后。
 * @note   该接口内部实现不建议客户做任何修改
 */
void xy_exitPassthroughMode();

/**
 * @brief  不定长度透传模式缓存数据接口，该接口使用点在不定长度透传接口中，缓存接收到的全部透传数据，
 *         最终缓存下的全部透传数据在passthr_rcv_buff中, 数据长度为passthr_rcvd_len
 * @param  data_buf[IN] 接收到的数据 
 * @param  rev_len [IN] 接收到的数据长度
 * @note   该接口内部实现不建议客户做任何修改
 */
void passthr_get_unfixedlen_data(char *data_buf,uint32_t rev_len);

