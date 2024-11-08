#pragma once
#include <stdint.h>

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
enum XY_PROXY_MSG_ID
{
    PROXY_MSG_PS_PDP_ACT,               //pdp激活
    PROXY_MSG_PS_PDP_DEACT,             //pdp去激活
    PROXY_MSG_INTER_CORE_WAIT_FREE,     //由于零拷贝内存释放机制，灌包测试场景可能会出现ap/cp两侧写通道满，造成死锁，因此FREE的操作不能在inter_core_msg线程中执行
    PROXY_MSG_RESUME_CDP_UPDATE,        // CDP update handling
    PROXY_MSG_RESUME_ONENET_UPDATE,     // ONENET update handling
    PROXY_MSG_RESUME_CTWING_UPDATE,     // CTWING update handling
    PROXY_MSG_REMOVE_DELAY_LOCK,        //处理延迟锁超时事件，避免在软timer超时回调中做复杂操作可能导致栈溢出及耗时问题。
    PROXY_MSG_TCP_ACK,                  //处理lwip上报的tcp ack信息
    PROXY_MSG_FRAME_TIME,		        //物理层获取到超帧信息后发送消息，通过主控进行快照更新
    PROXY_MSG_DNS_INIT,                 //dns server初始化
    PROXY_WRITE_AP_NV_PARAM,            //写AP传递来的出厂NV参数
    PROXY_MSG_IPDATA,                   //处理下行数据
};


typedef struct xy_proxy_msg
{
    int msg_id;             //@see @ref XY_PROXY_MSG_ID
    uint32_t size;          //消息数据长度
    char data[0];           //消息数据 
} xy_proxy_msg_t;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 主动线程初始化函数，在main函数中调用
 * @note 创建相关信号量，队列及线程
 */
void xy_proxy_init(void);

/**
 * @brief 发送消息到主控线程
 * @param  msg_id [IN]  消息ID @see @ref XY_PROXY_MSG_ID
 * @param  buff [IN] 需要传递的数据地址
 * @param  len [IN] 需要传递的数据长度
 */
int send_msg_2_proxy(int msg_id, void *buff, int len);

