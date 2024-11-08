 /**
 * @file xy_net_api.h
 * @brief 网络相关api接口，例如获取ip地址，设置dns，判断网络状态等，以供客户二次开发使用
 * @note 客户应尽可能使用该头文件提供的开发接口，避免直接使用lwip内部接口
 * @version 1.0
 * @date 2022-06-14
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 */

#pragma once

#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include <stdint.h>

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XY_IP4ADDR_STRLEN       INET_ADDRSTRLEN         // IPv4地址字符串最大长度
#define XY_IP6ADDR_STRLEN       INET6_ADDRSTRLEN        // IPv6地址字符串最大长度
#define XY_IPADDR_STRLEN_MAX    INET6_ADDRSTRLEN        // IP地址字符串最大长度

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief Netif网口IP类型定义
 */
typedef enum
{
    IPV4_TYPE  = IPADDR_TYPE_V4,    /* IPv4单栈激活且网络已可用/IPv4地址类型 */
    IPV6_TYPE  = IPADDR_TYPE_V6,    /* IPv6单栈激活且网络已可用/IPv6地址类型 */
    IPV46_TYPE = IPADDR_TYPE_ANY,   /* IPv4v6双栈激活且IPv4和IPv6网络都可用 */
    IPV6PREPARING_TYPE,             /* IPv6单栈激活但在等待RS/RA完成 */
    IPV4_IPV6PREPARING_TYPE,        /* IPv4v6双栈激活，IPv4网络可用，IPv6仍在等待RS/RA完成 */
    IP_TYPE_INVALID  = 0xFF,        /* IP网络不可用 */
} Netif_IPType_T;

/**
 * @brief Netif状态事件定义，供网络业务模块通过xy_reg_psnetif_callback注册相应事件的回调接口
 */
typedef enum
{
    EVENT_PSNETIF_ENTER_OOS     = 1 << 0,       //OOS状态，小区驻留失败，比如在隧道中
    EVENT_PSNETIF_ENTER_IS      = 1 << 1,       //从OOS进入IS
    EVENT_PSNETIF_INVALID       = 1 << 2,       //NEITF未激活
    EVENT_PSNETIF_IPV4_VALID    = 1 << 3,       //IPV4地址可用
    EVENT_PSNETIF_IPV6_VALID    = 1 << 4,       //IPV6地址可用
    EVENT_PSNETIF_IPV4V6_VALID  = 1 << 5,       //IPV4V6双栈激活,V4和V6地址均可用
    EVENT_PSNETIF_DNS_CHNAGED   = 1 << 6,       //NEITF已激活状态下,网卡信息发生改变，如dns server变化
    EVENT_PSNETIF_VALID = (EVENT_PSNETIF_IPV4_VALID | EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_IPV4V6_VALID),
    EVENT_PSNETIF_ALL_MASK = (EVENT_PSNETIF_ENTER_OOS | EVENT_PSNETIF_ENTER_IS |
                              EVENT_PSNETIF_INVALID | EVENT_PSNETIF_IPV4_VALID |
                              EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_IPV4V6_VALID | EVENT_PSNETIF_DNS_CHNAGED),
} PsStateChangeEvent;

/**
 * @brief 消息传输携带的快速释放标记 RAI，该标记用于指示核心网如何释放与模块的RRC连接
 */
typedef enum 
{
	RAI_NULL = 0,       //默认值，表示非最后一个业务报文
	RAI_REL_UP,         //指示当前报文为最后一个上行报文，且无下行应答报文，通常用于UDP的非确认模式上传数据
	RAI_REL_DOWN,       //指示当前报文为最后一个上行报文，且有下行应答报文，通常为确认式报文交互场景
} RAI_TYPE;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 指示PS协议栈是否进入OOS
 * @warning 一旦进入OOS，此时无线通信无法正常进行，待发送的上行数据包会缓存在PS内部，
 * 当堆积到一定数量后则会触发流控丢包，是否进入流控请参见is_Uplink_FlowCtl_Open()
 */
bool ps_is_oos();


/**
 * @brief  指示PS协议栈是否在执行上行流控丢包
 * @warning 一旦上行流控开启，建议业务模块暂缓发送上行数据，因为此时发送给PS也会被丢包处理
 */
bool is_Uplink_FlowCtl_Open();


/**
 * @brief 用于识别IPv4网路是否可用
 * @attention  对于刚开机就调用场景，需要充分考虑3GPP的小区驻留、PDP激活的时长
 */
bool xy_tcpip_v4_is_ok(void);

/**
 * @brief 用于识别IPv6网路是否可用
 */
bool xy_tcpip_v6_is_ok(void);

/**
 * @brief 用于识别任意网路是否可用
 */
bool xy_tcpip_is_ok(void);

/**
 * @brief 不建议使用！获取ipv4字符串型地址，存在获取不到ipv4地址或者入参错误的情况，因此调用该接口需判断返回值
 * @param ipAddr [OUT]用户的申请保存ipv4字符串型地址的内存
 * @param addrLen [IN]用户的申请保存ipv4字符串型地址的内存大小，不能小于16字节
 * @warning 注意入参内存申请长度，否则会造成内存越界
 *          若PDP尚未激活成功，ipAddr不会被赋值
 * @note  该接口得到的是字符串类型地址，如需要转成int型，需要自行调用inet_aton接口
 */
bool xy_getIP4Addr(char *ipAddr, int addrLen);

/**
 * @brief 不建议使用！获取可用的ipv6字符串型地址，非本地链路地址（以FE80开头的v6地址，例如FE80::1，存在获取不到ipv6地址或者入参错误的情况，因此调用该接口需判断返回值
 * @param ip6Addr [OUT]用户申请的保存ipv6字符串型地址的内存
 * @param addrLen [IN]用户申请的保存ipv6字符串型地址的内存大小，不能小于46字节
 * @warning 注意入参内存申请长度，否则会造成内存越界。
 *         若PDP尚未激活或者没有获取到有效的ipv6地址，ip6Addr不会被赋值
 * @note   该接口得到的是字符串类型地址，如需要转成整型，需自行调用inet_pton接口
 */
bool xy_getIP6Addr(char *ip6Addr, int addrLen);

/**
 * @brief  根据指定的IP类型获取ip_addr_t类型的IP地址
 * @param  ipType [IN] 指定的IP类型, 参数必须是IPV4_TYPE或者IPV6_TYPE
 * @param  ipaddr [OUT]指针类型，指向用户申请的保存ip_addr_t型IP地址的内存
 * @attention 存在获取不到ip地址或者入参错误的情况，因此调用该接口需判断返回值
 */
bool xy_get_ipaddr(uint8_t ipType, ip_addr_t *ipaddr);


/**
 * @brief  获取当前输入IP地址字符串的类型
 * @param  ipaddr [IN] IP地址，如"192.168.0.1", "ABCD:EF01:2345:6789:ABCD:EF01:2314:1111","ABCD:EF01:2345:6789:ABCD:EF01:111.111.111.111" 
 * @return 成功则返回IPV4_TYPE或者IPV6_TYPE，失败返回-1
 * @warning 类似于"111.111.256.256"这种字符串会被当作正常域名，非合法的IP地址，无法获取具体的IP类型
 *          此接口需做一定的字符串操作，若客户明确了输入的IP地址，可不考虑使用该接口，以提高处理效率。
 */
int xy_get_IpAddr_type(char *ipaddr);

/**
 * @brief  检测字符串类型的IP地址有效性,支持IPv4和IPv6地址检测
 * @param  ipaddr [IN] IP地址，如"192.168.0.1", "ABCD:EF01:2345:6789:ABCD:EF01:2314:1111","ABCD:EF01:2345:6789:ABCD:EF01:111.111.111.111"     
 * @param  iptype [IN] 指定的IP类型, 参数必须是IPV4_TYPE或者IPV6_TYPE
 */
bool xy_IpAddr_Check(char *ipaddr, uint8_t iptype);

/**
 * @brief 获取本地外网口的IP地址类型
 * @return 返回IP类型 @see @ref Netif_IPType_T
 * @note 该接口需要在PDP激活后调用
 */
int xy_get_netif_iptype(void);

/**
 * @brief psnetif事件回调接口类型定义
 * @param eventId  用户需要监听的psnetif事件 @see @ref PsStateChangeEvent
 * @warning 注册psnetif回调函数必须在系统初始化中执行，否则可能出现事件遗漏问题 
 *          在回调函数中若有阻塞或者耗时操作，请投递至其他线程处理，否则可能影响lwip tcpip线程调度！
 */
typedef void (*psNetifEventCallback_t)(uint32_t eventId);

/**
 * @brief  注册netif事件回调接口
 * @param  eventGroup [IN] 用户需要监听的事件组，可以是多个事件组合，也可以是单个事件
 * @param  callback [IN] 回调接口，类型 @see @ref psNetifEventCallback_t
 * @warning 注册用户回调必须在初始化过程中完成！
 */
void xy_reg_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback);

/**
 * @brief 删除netif事件回调接口
 * @param  eventGroup [IN] 用户需要删除的事件组，可以是多个事件组合，也可以是单个事件
 * @param  callback [IN] 回调接口，类型 @see @ref psNetifEventCallback_t
 * @warning 如果注册的回调函数中有向其他线程发消息队列之类的操作，应在接收线程退出前删除该事件回调，否则有死机的风险！
 */
bool xy_deReg_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback);

