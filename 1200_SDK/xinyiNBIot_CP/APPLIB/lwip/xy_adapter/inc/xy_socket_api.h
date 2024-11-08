/**
 * @file xy_socket_api.h
 * @brief 芯翼提供的socket/dns操作API,供客户二次开发使用
 * @version 1.0
 * @date 2022-06-14
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 */
#pragma once

#include <stdint.h>
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define XY_DEFAULT_NTP_SERVER   "ntp1.aliyun.com"       // 默认NTP服务器域名
#define XY_DEFAULT_V4_PRIDNS    "114.114.114.114"       // 默认主DNS服务器IPV4地址，不允许为NULL或空字符串，可保持为默认状态
#define XY_DEFAULT_V4_SECDNS    "8.8.8.8"               // 默认辅DNS服务器IPV4地址，不允许为NULL或空字符串，可保持为默认状态
#define XY_DEFAULT_V6_PRIDNS    "240c::6666"            // 默认主DNS服务器IPV6地址，不允许为NULL或空字符串，可保持为默认状态
#define XY_DEFAULT_V6_SECDNS    "240c::6644"            // 默认辅DNS服务器IPV6地址，不允许为NULL或空字符串，可保持为默认状态

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief xy_socket_by_host接口参数，表示需要创建socket的IP类型
 */
typedef enum
{
    Sock_IPv4_Only,  // 仅IPV4
    Sock_IPv6_Only,  // 仅IPV6 
    Sock_IPv46,      // 46双栈，IPV4优先，IPV4网路不通时用IPV6
    Sock_IPv64       // IPV6优先，IPV6网路不通时用IPV4
} Sock_IPType;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief   以地址字符串方式设置lwip DNS地址,支持ipv4/ipv6 dns地址设置
 * @param   dns_addr [IN] dns地址,例如"114.114.114.114"
 * @param   index [IN] dns地址索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param   save  [IN] 1:更改默认dns地址配置并保存到文件系统,同时更新lwip dns地址
 *                     0:仅更新lwip dns地址，不会更改默认dns地址配置（不会保存到文件系统）
 * @warning 需要保存dns地址到文件系统时，index参数需要对应默认配置，即索引0: ipv4 pridns，索引1：ipv4 secdns，索引2：ipv6 pridns，索引3：ipv6 secdns，否则有DNS业务失败风险
 * @note    芯片上电会优先使用PDP激活后协议栈给的DNS地址（不更新到文件系统中），若协议栈没给，则使用默认的保存在文件系统的DNS地址
 *          此接口可直接更改lwip的原有的DNS配置，客户使用时，请务必要确认所设置DNS地址可靠。
 */
bool xy_dns_set(char *dns_addr, int index, bool save);

/**
 * @brief   以ip_addr_t类型设置lwip DNS地址,支持ipv4/ipv6 dns地址设置
 * @param   dns_addr [IN] dns地址，ip_addr_t形式
 * @param   index [IN] dns地址索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param   save  [IN] 1:更改默认dns地址配置并保存到文件系统,同时更新lwip dns地址
 *                     0:仅更新lwip dns地址，不会更改默认dns地址配置（不会保存到文件系统）
 * @warning 需要保存dns地址到文件系统时，index参数需要对应默认配置，即索引0: ipv4 pridns，索引1：ipv4 secdns，索引2：ipv6 pridns，索引3：ipv6 secdns，否则有DNS业务失败风险
 * @note    芯片上电会优先使用PDP激活后协议栈给的DNS地址（不更新到文件系统中），若协议栈没给，则使用默认的保存在文件系统的DNS地址
 *          此接口可直接更改lwip的原有的DNS配置，客户使用时，请务必要确认所设置DNS地址可靠。   
 */
bool xy_dns_set2(ip_addr_t *dns_addr, int index, bool save);

/**
 * @brief 根据索引获取dns服务器的字符串型地址
 * @param  index [IN] dns索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param  dns_addr [OUT] 保存字符串地址数据的内存地址，由用户外部申请传入,不得为空
 * @note 此接口获取的是lwip的DNS地址，可能与默认的保存在文件系统的DNS地址不一致，客户使用时请注意。
 */
bool xy_dns_get(int index, char *dns_addr);

/**
 * @brief 根据索引获取dns服务器的ip_addr_t类型地址
 * @param  index [IN] dns索引，索引范围由lwip配置决定，参考DNS_MAX_SERVERS宏，目前取值范围是[0,3]
 * @param  dns_addr [OUT] 保存ip_addr_t类型地址数据的内存地址，由用户外部申请传入，不得为空
 * @note 此接口获取的是lwip的DNS地址，可能与默认的保存在文件系统的DNS地址不一致，客户使用时请注意。
 */
bool xy_dns_get2(int index, ip_addr_t *dns_addr);

/**
 * @brief   根据远端域名和端口号创建和连接socket
 * @param   host [IN] 远端服务器域名或者IP地址，字符串形式，不得为空
 * @param   type [IN] 指定创建的socket IP类型,定义参考 @see @ref Sock_IPType
 * @param   proto [IN] 指定创建socket的类型,UDP/TCP
 * @param   local_port [IN] 本地端口号
 * @param   remote_port [IN] 远端端口号
 * @param   remote_addr [OUT] 由调用者提供的保存远端服务器IP数据的地址，域名解析成功后，返回的ip_addr_t类型的远端服务器IP数据.若不需要返回，可以设置为NULL。
 * @return  成功则返回创建的socket fd，失败返回-1
 * @note    此接口内部会在创建socket之后，会进行bind绑定本地端口（未绑定本地IP），以及connect连接远端IP和端口的操作
 * @warning 使用此接口时，需用户自己先保证所做业务对应的网络已通，否则socket连接可能会失败！！
 *          socketIP类型为Sock_IPv4_Only时，先通过xy_tcpip_v4_is_ok判断IPv4网路是否可用，若可用则调用该接口创建IPv4网路的socket；
 *          socketIP类型为Sock_IPv6_Only时，先通过xy_tcpip_v6_is_ok判断IPv6网路是否可用，若可用则调用该接口创建IPv6网路的socket；
 *          socketIP类型为Sock_IPv46时，先通过xy_tcpip_is_ok判断任意网路是否可用，若可用则优先尝试创建IPv4网路的socket，若无法创建则尝试创建IPv6网路的socket，当IPv4和IPv6均失败时，返回-1；
 *          socketIP类型为Sock_IPv64时，先通过xy_tcpip_is_ok判断任意网路是否可用，若可用则优先尝试创建IPv6网路的socket，若无法创建则尝试创建IPv4网路的socket，当IPv4和IPv6均失败时，返回-1；
 */
int32_t xy_socket_by_host(const char* host, Sock_IPType type, uint8_t proto, uint16_t local_port, uint16_t remote_port, ip_addr_t* remote_addr);

/**
 * @brief   根据远端域名和端口号创建socket
 * @param   host [IN] 远端服务器域名或者IP地址，字符串形式，不得为空
 * @param   type [IN] 指定创建的socket IP类型,定义参考 @see @ref Sock_IPType
 * @param   proto [IN] 指定创建socket的类型,UDP/TCP
 * @param   local_port [IN] 本地端口号
 * @param   remote_port [IN] 远端端口号
 * @param   remote_addr [OUT] 由调用者提供的保存远端服务器IP数据的地址，域名解析成功后，返回的ip_addr_t类型的远端服务器IP数据.若不需要返回，可以设置为NULL。
 * @return  成功则返回创建的socket fd，失败返回-1
 * @note    此接口内部会在创建socket之后，会进行bind绑定本地端口（未绑定本地IP），不会执行connect连接远端IP和端口的操作
 * @warning 使用此接口时，需用户自己先保证所做业务对应的网络已通，否则socket创建可能会失败！！
 *          socketIP类型为Sock_IPv4_Only时，先通过xy_tcpip_v4_is_ok判断IPv4网路是否可用，若可用则调用该接口创建IPv4网路的socket；
 *          socketIP类型为Sock_IPv6_Only时，先通过xy_tcpip_v6_is_ok判断IPv6网路是否可用，若可用则调用该接口创建IPv6网路的socket；
 *          socketIP类型为Sock_IPv46时，先通过xy_tcpip_is_ok判断任意网路是否可用，若可用则优先尝试创建IPv4网路的socket，若无法创建则尝试创建IPv6网路的socket，当IPv4和IPv6均失败时，返回-1；
 *          socketIP类型为Sock_IPv64时，先通过xy_tcpip_is_ok判断任意网路是否可用，若可用则优先尝试创建IPv6网路的socket，若无法创建则尝试创建IPv4网路的socket，当IPv4和IPv6均失败时，返回-1；
 */
int32_t xy_socket_by_host2(const char* host, Sock_IPType type, uint8_t proto, uint16_t local_port, uint16_t remote_port, ip_addr_t* remote_addr);

/**
 * @brief  根据socket套接字获取socket本地IP和端口号
 * @param  fd [IN] socket套接字
 * @param  local_ipaddr [OUT] 本地IP存放地址，用户传参
 * @param  local_port [OUT] 本地端口存放地址，用户传参
 * @note  如果不需要获取ipaddr,可以将入参local_ipaddr置NULL,如果不需要获取port,可以将入参local_port置NULL,
 * @warning 入参local_ipaddr和local_port不可同时为NULL; 
 *          调用此接口时，若需获取本地IP，socket必须执行过bind操作，若未执行过，建议直接调用xy_get_ipaddr获取真实有效的本地IP;
 *          bind本地IP时，需考虑CFUN0->CFUN1的情况，此情况下，本地IP可能会变，建议直接调用xy_get_ipaddr获取真实有效的本地IP;
 */
bool xy_socket_local_info(int32_t fd, ip_addr_t *local_ipaddr, uint16_t *local_port);

/**
 * @brief 用于pdp激活时，调整DNS服务器索引顺序
 * @param dns_ipaddr [IN] DNS_MAX_SERVERS个ip_addr_t型IPV4V6地址集合，@see @ref ip_addr_t，参考DNS_MAX_SERVERS宏
 * @param ip_type [IN] pdp激活的ip类型, @see @ref IPType_T
 * @note  此接口用于pdp激活时，调整lwip DNS服务器索引顺序，调整时应格外注意DNS服务器配置与默认的DNS服务器索引顺序的对应，默认使用IPV4主->IPV4辅->IPV6主->IPV6辅索引顺序
 *        会直接更改lwip的DNS配置，芯片上电后优先使用PDP激活后协议栈给的DNS地址（不更新到文件系统中），若协议栈没给，则使用默认的保存在文件系统的DNS地址
 *        若客户需要更改默认DNS配置，建议直接调用xy_dns_set或者发DNSCFG配置相关AT指令进行更改，也可直接修改XY_DEFAULT_V4_PRIDNS/XY_DEFAULT_V4_SECDNS/XY_DEFAULT_V6_PRIDNS/XY_DEFAULT_V6_SECDNS宏值
 */
void user_dns_config(ip_addr_t *dns_ipaddr, uint8_t ip_type);

/**
 * @brief 检测dns域名字符串有效性
 * @param  domain[IN] dns域名字符串
 * @note 参考RFC1035标准，域名字符仅包含英文字母、数字和连接符'-'，每个标签长度不得超过63
 * @attention 芯翼规定最大域名长度不得超过100字节，如需调整，可以修改DNS_MAX_NAME_LENGTH宏值大小
 * @warning 类似于"111.111.256.256"这种字符串会被当作正常域名来做解析的，如果明确输入的是ip地址且需要做地址合法性检测需调用xy_ipaddr_check接口
 *          域名有效性判断需做一定的字符串操作，若客户保证输入的域名一定正确，可不考虑使用该接口，以提高处理效率。
 */
bool xy_domain_is_valid(char *domain);

/**
 * @brief socket发送send扩展，携带seq和rai两个新增入参，以满足3GPP与IP报文的状态机互动
 * @param fd [IN] scoket句柄
 * @param dataptr [IN] 要发送的数据
 * @param size [IN] 要发送的数据长度
 * @param flags [IN] 用户一般不用关心，默认值0即可
 * @param seq [IN] 1-255，新增参数，默认值为0；指示数据包对应的发送序列，以便3GPP协议栈通过D_XY_PS_REG_EVENT_IPSN来告知业务层空口发送情况
 * @param rai [IN] 当前报文的RAI属性，默认值为0；具体解释见 @see @ref RAI_TYPE
 * @note  使用该函数时,socket需要执行过connect操作。seq参数使用较为繁琐，且无实际意义，建议客户无需关心！
 * @warning 若存在多socket业务并行，不建议使用rai参数值，因为可能造成3GPP频繁的进入RAI和建链流程，也可能影响伴随下行数据包的正常接收，例如FOTA下行指示报文
 */
#define send2(fd, dataptr, size, flags, seq, rai)                lwip_send2(fd, dataptr, size, flags, seq, rai)

/**
 * @brief socket发送sendto扩展，携带seq和rai两个新增入参，以满足3GPP与IP报文的状态机互动
 * @param fd [IN] scoket句柄
 * @param dataptr [IN] 要发送的数据
 * @param size [IN] 要发送的数据长度
 * @param flags [IN] 用户一般不用关心，默认值0即可
 * @param to [IN] 远端地址信息结构体
 * @param tolen [IN]  远端地址信息结构体长度
 * @param seq [IN] 1-255，新增参数，默认值为0；指示数据包对应的发送序列，以便3GPP协议栈通过D_XY_PS_REG_EVENT_IPSN来告知业务层空口发送情况
 * @param rai [IN] 当前报文的RAI属性，默认值为0；具体解释见 @see @ref RAI_TYPE
 * @note  seq参数使用较为繁琐，且无实际意义，建议客户无需关心！
 * @warning 若存在多socket业务并行，不建议使用rai参数值，因为可能造成3GPP频繁的进入RAI和建链流程，也可能影响伴随下行数据包的正常接收，例如FOTA下行指示报文
 */
#define sendto2(fd, dataptr, size, flags, to, tolen, seq, rai)  lwip_sendto2(fd, dataptr, size, flags, to, tolen, seq, rai)
