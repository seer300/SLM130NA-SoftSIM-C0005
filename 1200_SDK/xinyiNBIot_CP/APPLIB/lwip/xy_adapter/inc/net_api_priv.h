#pragma once

#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_V4_PRIDNS_INDEX   0                  // 默认主DNS服务器IPV4地址索引
#define DEFAULT_V4_SECDNS_INDEX   1                  // 默认辅DNS服务器IPV4地址索引
#define DEFAULT_V6_PRIDNS_INDEX   2                  // 默认主DNS服务器IPV6地址索引
#define DEFAULT_V6_SECDNS_INDEX   3                  // 默认辅DNS服务器IPV6地址索引

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct proxy_downlink_data
{
	uint8_t cid;
    uint8_t reserved[3];
	int len;
	char *data;
} proxy_downlink_data_t;

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief 供协议栈调用，用于获取字节型ipv6地址
 * @return 0：表示尚未获取到有效的ipv6地址或者入参错误，1：表示已获取到有效的ipv6地址
 */
int ps_get_ip6addr(unsigned int *ip6addr);

/**
 * @brief 处理下行ip数据函数
 */
void proc_downlink_packet(proxy_downlink_data_t* downlink_data);

/**
 * @brief 查询LWIP网卡是否激活，仅芯翼内部使用，用户不得调用
 * @warning 用户只能使用xy_tcpip_is_ok/xy_tcpip_v4_ok/xy_tcpip_v6_ok接口来判断网路是否已准备好
 */
bool psnetif_is_ok();
