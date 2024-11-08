/**
 * @file net_api_priv.c
 * @brief 芯翼内部定义的网络业务相关api接口，客户不得直接使用
 * @version 1.0
 * @date 2022-06-22
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 

/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "net_api_priv.h"
#include "lwip/netif.h"
#include "lwip/apps/sntp.h"
#include "lwip/netdb.h"
#include "xy_socket_api.h"
#include "ps_netif_api.h"
#include "xy_utils.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct net_err_map
{
    int lwip_err;
    int xy_err;
} net_err_map_t;

/*******************************************************************************
 *						   Local variable definitions				           *
 ******************************************************************************/
const net_err_map_t net_priv_err_table[] = {
    {EAI_NONAME, XY_Err_Parameter},
    {EAI_SERVICE, XY_Err_Parameter},
    {HOST_NOT_FOUND, XY_Err_DnsFail},
    {EAI_FAMILY, XY_Err_NotAllowed},
    {EAI_MEMORY, XY_Err_NoMemory},
    {EAI_FAIL, XY_Err_DnsFail},
    {SNTP_ERR_INPROGRESS, XY_Err_InProgress},
    {SNTP_ERR_TIMEOUT, XY_Err_Timeout},
    {SNTP_ERR_ARG, XY_Err_Parameter},
    {SNTP_ERR_DNS, XY_Err_DnsFail},
    {SNTP_ERR_NOTALLOWED, XY_Err_NotAllowed}
};

// 用于将一些常用的LWIP返回的错误码转换成网络通用错误码
int covert_to_xy_net_errcode(int lwip_err)
{
    int i = 0;
    for (i = 0; i < (sizeof(net_priv_err_table) / sizeof(net_err_map_t)); i++)
    {
        if (net_priv_err_table[i].lwip_err == lwip_err)
            return net_priv_err_table[i].xy_err;
    }
    return XY_ERR;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
bool psnetif_is_ok()
{
	return ((netif_default != NULL) && netif_is_up(netif_default) && netif_is_link_up(netif_default));
}

int ps_get_ip6addr(unsigned int *ip6addr)
{
	if (!xy_tcpip_v6_is_ok() || ip6addr == NULL)
		return 0;

    /* 整型ipv6地址大小为16字节 */
	memcpy(ip6addr, netif_ip6_addr(netif_default, 1)->addr, 16);
	return 1;
}
