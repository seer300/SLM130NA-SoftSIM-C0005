#pragma once
#include <stdbool.h>
/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 平台实现，PS调用，发送PS相关的URC及结果码
 */
void SendAtInd2User(char *pAt, unsigned int ulAtLen);

/**
 * @brief PS实现，平台调用，发送从串口接收来的PS相关AT命令给ATC
 */
void SendAt2AtcAp(char *pAt, unsigned int ulAtLen);


/**
 * @brief 由PS调用，透传PS的跨核消息给对方核，内部使用ICM_PS_SHM_MSG
 */
int send_ps_shm_msg(void *msg, int msg_len);


/*AT框架中，收到AT命令进行定制拦截，识别PS的一些特殊AT命令处理*/
void special_ps_at_proc(char *at_str);


