#include "xy_net_api.h"
#include "xy_socket_api.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "net_app_resume.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "net_api_priv.h"

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
static int hex_digit_value(char ch)
{
    if ('0' <= ch && ch <= '9')
        return ch - '0';
    if ('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    if ('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

static bool inet_pton4(const char *src, const char *end, unsigned char *dst)
{
    int saw_digit, octets, ch;
    unsigned char tmp[4], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while (src < end)
    {
        ch = *src++;
        if (ch >= '0' && ch <= '9')
        {
            unsigned int new = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;
            if (new > 255)
                return 0;
            *tp = new;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;
    memcpy(dst, tmp, 4);
    return 1;
}

static bool inet_pton6(const char *src, const char *src_endp, unsigned char *dst)
{
    unsigned char tmp[16], *tp, *endp, *colonp;
    const char *curtok;
    int ch;
    unsigned int xdigits_seen; /* Number of hex digits since colon.  */
    unsigned int val;

    tp = memset(tmp, '\0', 16);
    endp = tp + 16;
    colonp = NULL;

    /* Leading :: requires some special handling.  */
    if (src == src_endp)
        return 0;
    if (*src == ':')
    {
        ++src;
        if (src == src_endp || *src != ':')
            return 0;
    }

    curtok = src;
    xdigits_seen = 0;
    val = 0;
    while (src < src_endp)
    {
        ch = *src++;
        int digit = hex_digit_value(ch);
        if (digit >= 0)
        {
            if (xdigits_seen == 4)
                return 0;
            val <<= 4;
            val |= digit;
            if (val > 0xffff)
                return 0;
            ++xdigits_seen;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (xdigits_seen == 0)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (src == src_endp)
                return 0;
            if (tp + 2 > endp)
                return 0;
            *tp++ = (unsigned char)(val >> 8) & 0xff;
            *tp++ = (unsigned char)val & 0xff;
            xdigits_seen = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + 4) <= endp) && inet_pton4(curtok, src_endp, tp) > 0)
        {
            tp += 4;
            xdigits_seen = 0;
            break; /* '\0' was seen by inet_pton4.  */
        }
        return 0;
    }
    if (xdigits_seen > 0)
    {
        if (tp + 2 > endp)
            return 0;
        *tp++ = (unsigned char)(val >> 8) & 0xff;
        *tp++ = (unsigned char)val & 0xff;
    }
    if (colonp != NULL)
    {
        /* Replace :: with zeros.  */
        if (tp == endp)
            /* :: would expand to a zero-width field.  */
            return 0;
        unsigned int n = tp - colonp;
        memmove(endp - n, colonp, n);
        memset(colonp, 0, endp - n - colonp);
        tp = endp;
    }
    if (tp != endp)
        return 0;
    memcpy(dst, tmp, 16);
    return 1;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
bool xy_dns_set(char *dns_addr, int index, bool save)
{
    if (index < 0 || index >= DNS_MAX_SERVERS || dns_addr == NULL || strlen(dns_addr) == 0)
    {
        return 0;
    }

    /* 检测字符串型ip地址数据的有效性，防止非法输入 */
    if ( (!xy_IpAddr_Check(dns_addr, IPV4_TYPE)) && (!xy_IpAddr_Check(dns_addr, IPV6_TYPE)) )
    {
        return 0;
    }

    ip_addr_t dns_t = {0};
    if (ipaddr_aton(dns_addr, &dns_t) == 0)
        return 0; 
    else
        return xy_dns_set2(&dns_t, index, save);
}

bool xy_dns_set2(ip_addr_t *ip_addr, int index, bool save)
{
    ip_addr_t dns_default[DNS_MAX_SERVERS];

    if (index < 0 || index >= DNS_MAX_SERVERS || ip_addr == NULL)
    {
        return 0;
    }

    dns_setserver(index, ip_addr);
    if (save)
    {
        xy_printf(0,XYAPP, WARN_LOG, "dns addr backup for index:%d", index);

        // 写文件系统需一次性写整块内容，故这里先将原本文件系统的内容读出来，根据index修改对应的dns配置，整合后再将整块内容写入文件系统
        if (XY_ERR == cloud_read_file(DNS_SERVER_ADDR_NVM_FILE_NAME, (void *)&dns_default[0], sizeof(ip_addr_t) * DNS_MAX_SERVERS))
        {
            // 这里不允许读文件系统失败的情况，开机后PDP激活前已保证文件系统里有默认dns配置
            xy_printf(0, XYAPP, WARN_LOG, "dns addr backup for index:%d fail", index);
        }

        dns_default[index] = *ip_addr;

        cloud_save_file(DNS_SERVER_ADDR_NVM_FILE_NAME, (void *)&dns_default[0], sizeof(ip_addr_t) * DNS_MAX_SERVERS);
    }

    psNetifEventInd(EVENT_PSNETIF_DNS_CHNAGED);
    return 1;
}

bool xy_dns_get(int index, char *ipaddr)
{
    if (index >= DNS_MAX_SERVERS || ipaddr == NULL)
    {
        return 0;
    }

    ip_addr_t addr = {0};
    if (!xy_dns_get2(index, &addr))
    {
        return 0;
    }
    if (ipaddr_ntoa_r(&addr, ipaddr, XY_IPADDR_STRLEN_MAX) == NULL)
    {
        return 0;
    }
    return 1;
}

bool xy_dns_get2(int index, ip_addr_t *ipaddr)
{
    if (index >= DNS_MAX_SERVERS || ipaddr == NULL)
    {
        return 0;
    }

    *ipaddr = *((ip_addr_t *)dns_getserver(index));
    return 1;
}

bool xy_getIP4Addr(char *ipAddr, int addrLen)
{
    struct netif* netif = find_active_netif();
    if (!xy_tcpip_v4_is_ok() || ipAddr == NULL || addrLen < XY_IP4ADDR_STRLEN || netif == NULL)
        return 0;

    if (inet_ntop(AF_INET, &netif->ip_addr.u_addr.ip4, ipAddr, addrLen) == NULL)
        return 0;

    return 1;
}

bool xy_getIP6Addr(char *ip6Addr, int addrLen)
{
    struct netif* netif = find_active_netif();
    if (ip6Addr == NULL || addrLen < XY_IP6ADDR_STRLEN || netif == NULL)
    {
        return 0;
    }

    if (netif_ip6_addr_state(netif, 1) == IP6_ADDR_PREFERRED)
    {
        if (inet_ntop(AF_INET6, &netif_ip6_addr(netif, 1)->addr, ip6Addr, addrLen) != NULL)
            return 1;
    }
    else if (netif_ip6_addr_state(netif, 0) == IP6_ADDR_VALID)
    {
        /* 有效的ipv6地址尚未获取，返回协议栈上报的本地链路地址 */
        if (inet_ntop(AF_INET6, &netif_ip6_addr(netif, 0)->addr, ip6Addr, addrLen) != NULL)
            return 1;
    }

    return 0;
}

bool xy_get_ipaddr(uint8_t ipType, ip_addr_t *ipaddr)
{
    struct netif* netif = find_active_netif();

    if (!xy_tcpip_is_ok() || ipaddr == NULL || netif == NULL)
        return 0;

    if (ipType == IPV4_TYPE)
    {
        ip_addr_copy(*ipaddr, netif->ip_addr);
    }
    else if (ipType == IPV6_TYPE)
    {
        ip_addr_copy(*ipaddr, netif->ip6_addr[1]);
    }
    else
    {
        return 0;
    }

    return 1;
}

bool xy_tcpip_v4_is_ok(void)
{ 
    bool havecheck = false;
CHECK:
    if (xy_get_netif_iptype() == IPV4_TYPE || xy_get_netif_iptype() == IPV46_TYPE ||
        xy_get_netif_iptype() == IPV4_IPV6PREPARING_TYPE)
    {
        return 1;
    }
    else if (netif_is_wakeup_suitation() && !havecheck)
    {
        /* wakeup唤醒后，网卡尚未激活，调用该接口超时等待网卡激活 */
        osSemaphoreAcquire(g_net_ok_sem, NETIF_WAKEUP_WAIT_MAX);
        havecheck = true;
        goto CHECK;
    }
    else
    {
        send_debug_by_at_uart("\r\n+DBGINFO:xy_tcpip_v4_is_not_ok\r\n");
        return 0;
    }
}

bool xy_tcpip_v6_is_ok(void)
{
    bool havecheck = false;
CHECK:
    if (xy_get_netif_iptype() == IPV46_TYPE || xy_get_netif_iptype() == IPV6_TYPE)
    {
        return 1;
    }
    else if (netif_is_wakeup_suitation() && !havecheck)
    {
        /* wakeup唤醒后，网卡尚未激活，调用该接口超时等待网卡激活 */
        osSemaphoreAcquire(g_net_ok_sem, NETIF_WAKEUP_WAIT_MAX);
        havecheck = true;
        goto CHECK;
    }
    else
    {
        send_debug_by_at_uart("\r\n+DBGINFO:xy_tcpip_v6_is_not_ok\r\n");
        return 0;
    }
}

bool xy_tcpip_is_ok(void)
{
    bool havecheck = false;
CHECK:
    if (xy_get_netif_iptype() == IPV4_TYPE || xy_get_netif_iptype() == IPV46_TYPE ||
        xy_get_netif_iptype() == IPV4_IPV6PREPARING_TYPE)
    {
        return 1;
    }
    else if (xy_get_netif_iptype() == IPV6_TYPE)
    {
        return 1;
    }
    else if (netif_is_wakeup_suitation() && !havecheck)
    {
        /* wakeup唤醒后，网卡尚未激活，调用该接口超时等待网卡激活 */
        osSemaphoreAcquire(g_net_ok_sem, NETIF_WAKEUP_WAIT_MAX);
        havecheck = true;
        goto CHECK;
    }

    send_debug_by_at_uart("\r\n+DBGINFO:xy_tcpip_is_not_ok\r\n");
    return 0;
}

bool xy_get_PsNetifInfo(PsNetifInfo *netifInfo)
{
    if (netifInfo == NULL || !xy_tcpip_is_ok())
        return 0;

    struct netif *netif = find_active_netif();
    if (netif == NULL)
        return 0;

    netifInfo->ip_type = xy_get_netif_iptype();
    netifInfo->workingCid = g_working_cid;

    int index = 0;
    for (index = 0; index < DNS_MAX_SERVERS; index++)
    {
        if (ip_addr_isany(dns_getserver(index)))
            break;
        ip_addr_copy(netifInfo->dns[index], *(dns_getserver(index)));
    }

    if (netifInfo->ip_type == IPV4_TYPE || netifInfo->ip_type == IPV46_TYPE || netifInfo->ip_type == IPV4_IPV6PREPARING_TYPE)
    {
        ip_addr_copy(netifInfo->ip4, netif->ip_addr);
    }

    if (netifInfo->ip_type == IPV6_TYPE || netifInfo->ip_type == IPV46_TYPE)
    {
        ip_addr_copy(netifInfo->ip6_local, netif->ip6_addr[0]);
        if (netif_ip6_addr_state(netif, 1) == IP6_ADDR_PREFERRED)
        {
            ip_addr_copy(netifInfo->ip6_prefered, netif->ip6_addr[1]);
        }
    }
    else if (netifInfo->ip_type == IPV6PREPARING_TYPE || netifInfo->ip_type == IPV4_IPV6PREPARING_TYPE)
    {
        ip_addr_copy(netifInfo->ip6_local, netif->ip6_addr[0]);
    }

    return 1;
}

void xy_reg_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback)
{
    if (osKernelGetState() != osKernelInactive)
        osMutexAcquire(g_netif_callbacklist_mutex, osWaitForever);

    if (g_netif_callback_list == NULL)
    {
        g_netif_callback_list = (psNetifEventCallbackList_T *)xy_malloc(sizeof(psNetifEventCallbackList_T));
        g_netif_callback_list->callback = callback;
        g_netif_callback_list->eventGroup = eventGroup;
        g_netif_callback_list->next = NULL;
    }
    else
    {
        psNetifEventCallbackList_T *node = (psNetifEventCallbackList_T *)xy_malloc(sizeof(psNetifEventCallbackList_T));
        node->callback = callback;
        node->eventGroup = eventGroup;
        node->next = g_netif_callback_list;
        g_netif_callback_list = node;
    }
    if (osKernelGetState() != osKernelInactive)
        osMutexRelease(g_netif_callbacklist_mutex);
}

bool xy_deReg_psnetif_callback(uint32_t eventGroup, psNetifEventCallback_t callback)
{
    psNetifEventCallbackList_T *prev = NULL;
    psNetifEventCallbackList_T *cur = NULL;
    psNetifEventCallbackList_T *temp = NULL;

    if (g_netif_callback_list == NULL)
        return 0;

    osMutexAcquire(g_netif_callbacklist_mutex, osWaitForever);

    if (g_netif_callback_list->eventGroup == eventGroup && g_netif_callback_list->callback == callback)
    {
        temp = g_netif_callback_list;
        g_netif_callback_list = g_netif_callback_list->next;
        xy_free(temp);
    }
    else
    {
        prev = g_netif_callback_list;
        cur = g_netif_callback_list->next;
        while (cur != NULL)
        {
            if (cur->eventGroup == eventGroup && cur->callback == callback)
            {
                temp = cur;
                prev->next = cur->next;
                xy_free(temp);
                break;
            }
            prev = cur;
            cur = cur->next;
        }
    }
    osMutexRelease(g_netif_callbacklist_mutex);
    return 1;
}

extern uint8_t g_OOS_flag;
bool ps_is_oos()
{
	if(g_OOS_flag == 1)
			xy_printf(0,XYAPP, WARN_LOG, "OOS will drop packet!");

    return g_OOS_flag;
}

uint32_t g_flow_ctl = 0;


/*业务获取流控状态，以决定丢包和容错*/
bool is_Uplink_FlowCtl_Open()
{
	if(g_flow_ctl == 1)
		xy_printf(0,XYAPP, WARN_LOG, "uplink flow ctl drop!");
	
	return g_flow_ctl;
}

int xy_get_IpAddr_type(char *ipaddr)
{
    if (ipaddr == NULL)
        return -1;

    uint32_t len = strlen(ipaddr);

    if (len == 0 || len > XY_IPADDR_STRLEN_MAX)
        return -1;

    /* 检测ip字符串 */
    ip_addr_t addr = {0};
    if (ipaddr_aton(ipaddr, &addr) && xy_IpAddr_Check(ipaddr, addr.type))
    {
        if (addr.type == IPADDR_TYPE_V4)
            return IPV4_TYPE;
        else if (addr.type == IPADDR_TYPE_V6)
            return IPV6_TYPE;
        else
            return -1;
    }

    return -1;
}

bool xy_IpAddr_Check(char *ipaddr, uint8_t iptype)
{
    uint32_t ip_len;
    if (ipaddr == NULL || (ip_len = strlen(ipaddr)) == 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "check ip vaild but ipaddr null");
        return 0;
    }
    if (iptype != IPV4_TYPE && iptype != IPV6_TYPE)
    {
        xy_printf(0,XYAPP, WARN_LOG, "check ip vaild not support type:%d", iptype);
        return 0;
    }

    if (iptype == IPV4_TYPE && ip_len >= INET_ADDRSTRLEN)
    {
        xy_printf(0,XYAPP, WARN_LOG, "check ip vaild v4 addr len overlay:%d", ip_len);
        return 0;
    }
    if (iptype == IPV6_TYPE && ip_len >= INET6_ADDRSTRLEN)
    {
        xy_printf(0,XYAPP, WARN_LOG, "check ip vaild v6 addr len overlay:%d", ip_len);
        return 0;
    }

    if (iptype == IPV4_TYPE)
    {
        struct sockaddr_in addr;
        if (inet_pton4(ipaddr, ipaddr + ip_len, &addr.sin_addr) != 1)
        {
            xy_printf(0,XYAPP, WARN_LOG, "check ip vaild v4 addr[%s] invalid", ipaddr);
            return 0;
        }
    }
    else
    {
        struct sockaddr_in6 addr;
        if (inet_pton6(ipaddr, ipaddr + ip_len, &addr.sin6_addr) != 1)
        {
            xy_printf(0,XYAPP, WARN_LOG, "check ip vaild v6 addr[%s] invalid", ipaddr);
            return 0;
        }
    }
    xy_printf(0,XYAPP, WARN_LOG, "check ip vaild addr[%s] valid", ipaddr);
    return 1;
}

int32_t xy_socket_by_host(const char* host, Sock_IPType type, uint8_t proto, uint16_t local_port, uint16_t remote_port, ip_addr_t* remote_addr)
{
    if (host == NULL || strlen(host) == 0)
        return -1;

    if (proto != IPPROTO_UDP && proto != IPPROTO_TCP)
        return -1;

    xy_printf(0, XYAPP, WARN_LOG, "socket by host[%s] ip_type:%d,proto:%d,local port:%d", host, type, proto, local_port);

    int32_t fd = -1;
    int32_t dns_ret = -1;
    ip_addr_t dns_t = {0};
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    struct sockaddr_storage bind_addr = {0};
    // 检测host是否是IP地址字符串格式，是则返回1
    int32_t aton_ret = ipaddr_aton(host, &dns_t);    
    uint8_t strPort[7] = {0};
    snprintf(strPort, sizeof(strPort) - 1, "%d", remote_port);

    hint.ai_protocol = proto;
    hint.ai_socktype = (proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;

    // 用户自行保证在使用此接口之前IPV4或者IPV6网络可用
    if ((Sock_IPv4_Only == type) || (Sock_IPv46 == type))
    {
IPV4_PROCESS:
        dns_ret = -1;         
        // IPV4网络可用情况下，host为域名格式或点分十进制的IPV4地址字符串格式，执行DNS流程         
        if ( xy_tcpip_v4_is_ok() && (!aton_ret || dns_t.type == IPADDR_TYPE_V4) )
        {
            hint.ai_family = AF_INET;
            if ((dns_ret = getaddrinfo(host, strPort, &hint, &result)) != 0)
                xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host getaddrinfo ipv4 err:%d", dns_ret);
        }
        // 优先尝试创建IPv4网路的socket，若失败，再尝试创建IPv6网路的socket，若均失败，返回-1
        if ( (Sock_IPv46 == type) && (dns_ret != 0) )
            goto IPV6_PROCESS;
    } 
    else if ((Sock_IPv6_Only == type) || (Sock_IPv64 == type))
    {
IPV6_PROCESS:
        dns_ret = -1;
        // IPV6网络可用情况下，host为域名格式或十六进制的IPV6地址字符串格式，执行DNS流程
        if ( xy_tcpip_v6_is_ok() && (!aton_ret || dns_t.type == IPADDR_TYPE_V6) )
        {
            hint.ai_family = AF_INET6;
            if ((dns_ret = getaddrinfo(host, strPort, &hint, &result)) != 0)
                xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host getaddrinfo ipv6 err:%d", dns_ret);
        }
        // 优先尝试创建IPv6网路的socket，若失败，再尝试创建IPv4网路的socket，若均失败，返回-1
        if ( (Sock_IPv64 == type) && (dns_ret != 0) )
            goto IPV4_PROCESS;       
    }

    if (dns_ret != 0)
    	return -1;

    if (result->ai_family == AF_INET6)
    {
        ((struct sockaddr_in6 *)&bind_addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&bind_addr)->sin6_port = htons(local_port);

        if (remote_addr != NULL)
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V6;
            memcpy(remote_addr->u_addr.ip6.addr, addr6->sin6_addr.s6_addr, sizeof(addr6->sin6_addr.s6_addr));
            remote_addr->u_addr.ip6.zone = 0;
        }
    }
    else if (result->ai_family == AF_INET)
    {
        ((struct sockaddr_in *)&bind_addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)&bind_addr)->sin_port = htons(local_port);

        if (remote_addr != NULL)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V4;
            remote_addr->u_addr.ip4.addr = addr->sin_addr.s_addr;
        }
    }

    if ((fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host socket create err:%d %d", errno, fd);
        goto ERROR;
    }
    xy_printf(0, XYAPP, WARN_LOG, "xy_socket_by_host create succ:%d,%d,%d", result->ai_family, result->ai_socktype, fd);
    
    //local_port参数为0时，bind过程中由LWIP内部随机分配本地端口号
    //特殊场景，比如drx/edrx唤醒接收下行包时，需要绑定具体的本地端口，否则无法接收到下行数据。
	if (bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
    {
        xy_printf(0, XYAPP, WARN_LOG, "xy_socket_by_host socket bind err:%d %d", errno, fd);
        close(fd);
        fd = -1;
        goto ERROR;
    }

    // 建立TCP连接需要一次connect完成3次握手，对于同一个远端UDP connect能够使用send等接口节省使用sendto等带来的开支
	if (connect(fd, result->ai_addr, result->ai_addrlen) == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host socket connect err:%d %d", errno, fd);
        close(fd);
        int family = result->ai_family;
        // 这里一定要先释放，因为可能会重走DNS域名解析流程
        freeaddrinfo(result);
        // 同一个域名，可绑定多个IPV4或者IPV6地址，TCP connect IPV4服务器失败后，connect IPV6服务器不受影响
        if ( (AF_INET == family) && (Sock_IPv46 == type) )
            goto IPV6_PROCESS;
        else if ( (AF_INET6 == family) && (Sock_IPv64 == type) )
            goto IPV4_PROCESS;
        
        return -1;
    }

ERROR:
	freeaddrinfo(result);
        
    return fd;
}

int32_t xy_socket_by_host2(const char* host, Sock_IPType type, uint8_t proto, uint16_t local_port, uint16_t remote_port, ip_addr_t* remote_addr)
{
    if (host == NULL || strlen(host) == 0)
        return -1;

    if (proto != IPPROTO_UDP && proto != IPPROTO_TCP)
        return -1;


    xy_printf(0, XYAPP, WARN_LOG, "socket by host[%s] ip_type:%d,proto:%d,local port:%d", host, type, proto, local_port);

    int32_t fd = -1;
    int32_t dns_ret = -1;
    ip_addr_t dns_t = {0};
    struct addrinfo hint = {0};
    struct addrinfo *result = NULL;
    struct sockaddr_storage bind_addr = {0};
    // 检测host是否是IP地址字符串格式，是则返回1
    int32_t aton_ret = ipaddr_aton(host, &dns_t);    
    uint8_t strPort[7] = {0};
    snprintf(strPort, sizeof(strPort) - 1, "%d", remote_port);

    hint.ai_protocol = proto;
    hint.ai_socktype = (proto == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;

    // 用户自行保证在使用此接口之前IPV4或者IPV6网络可用
    if ((Sock_IPv4_Only == type) || (Sock_IPv46 == type))
    {
IPV4_PROCESS:
        dns_ret = -1;         
        // IPV4网络可用情况下，host为域名格式或点分十进制的IPV4地址字符串格式，执行DNS流程         
        if ( xy_tcpip_v4_is_ok() && (!aton_ret || dns_t.type == IPADDR_TYPE_V4) )
        {
            hint.ai_family = AF_INET;
            if ((dns_ret = getaddrinfo(host, strPort, &hint, &result)) != 0)
                xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host getaddrinfo ipv4 err:%d", dns_ret);
        }
        // 优先尝试创建IPv4网路的socket，若失败，再尝试创建IPv6网路的socket，若均失败，返回-1
        if ( (Sock_IPv46 == type) && (dns_ret != 0) )
            goto IPV6_PROCESS;
    } 
    else if ((Sock_IPv6_Only == type) || (Sock_IPv64 == type))
    {
IPV6_PROCESS:
        dns_ret = -1;
        // IPV6网络可用情况下，host为域名格式或十六进制的IPV6地址字符串格式，执行DNS流程
        if ( xy_tcpip_v6_is_ok() && (!aton_ret || dns_t.type == IPADDR_TYPE_V6) )
        {
            hint.ai_family = AF_INET6;
            if ((dns_ret = getaddrinfo(host, strPort, &hint, &result)) != 0)
                xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host getaddrinfo ipv6 err:%d", dns_ret);
        }
        // 优先尝试创建IPv6网路的socket，若失败，再尝试创建IPv4网路的socket，若均失败，返回-1
        if ( (Sock_IPv64 == type) && (dns_ret != 0) )
            goto IPV4_PROCESS;       
    }

    if (dns_ret != 0)
    	return -1;

    if (result->ai_family == AF_INET6)
    {
        ((struct sockaddr_in6 *)&bind_addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *)&bind_addr)->sin6_port = htons(local_port);

        if (remote_addr != NULL)
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V6;
            memcpy(remote_addr->u_addr.ip6.addr, addr6->sin6_addr.s6_addr, sizeof(addr6->sin6_addr.s6_addr));
            remote_addr->u_addr.ip6.zone = 0;
        }
    }
    else if (result->ai_family == AF_INET)
    {
        ((struct sockaddr_in *)&bind_addr)->sin_family = AF_INET;
        ((struct sockaddr_in *)&bind_addr)->sin_port = htons(local_port);

        if (remote_addr != NULL)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
            remote_addr->type = IPADDR_TYPE_V4;
            remote_addr->u_addr.ip4.addr = addr->sin_addr.s_addr;
        }
    }

    if ((fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "xy_socket_by_host socket create err:%d %d", errno, fd);
        goto ERROR;
    }
    xy_printf(0, XYAPP, WARN_LOG, "xy_socket_by_host create succ:%d,%d,%d", result->ai_family, result->ai_socktype, fd);
    
    //local_port参数为0时，bind过程中由LWIP内部随机分配本地端口号
    //特殊场景，比如drx/edrx唤醒接收下行包时，需要绑定具体的本地端口，否则无法接收到下行数据。
	if (bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1)
    {
        xy_printf(0, XYAPP, WARN_LOG, "xy_socket_by_host socket bind err:%d %d", errno, fd);
        close(fd);
        fd = -1;
        goto ERROR;
    }

ERROR:
	freeaddrinfo(result);   
    return fd;
}


bool xy_socket_local_info(int32_t fd, ip_addr_t *local_ipaddr, uint16_t *local_port)
{
    if ((local_ipaddr == NULL && local_port == NULL) || !xy_tcpip_is_ok())
        return 0;
        
    struct sockaddr_storage local_addr;
    int sockaddr_len = sizeof(struct sockaddr_storage);
    if (getsockname(fd, (struct sockaddr *)&local_addr, &sockaddr_len) != 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "get sock local addr fail:%d", errno);
        return 0;
    }

    if (local_addr.ss_family == AF_INET)
    {
        // 避免访问0地址
        if (local_ipaddr != NULL)
        {
            inet_addr_to_ip4addr(ip_2_ip4(local_ipaddr), &(((struct sockaddr_in *)&local_addr)->sin_addr));
            local_ipaddr->type = IPADDR_TYPE_V4;
        }
        if (local_port != NULL)
            *local_port = ntohs(((struct sockaddr_in *)&local_addr)->sin_port);
    }
    else if (local_addr.ss_family == AF_INET6)
    {
        if (local_ipaddr != NULL)
        {
            inet6_addr_to_ip6addr(ip_2_ip6(local_ipaddr), &(((struct sockaddr_in6 *)&local_addr)->sin6_addr));
            local_ipaddr->type = IPADDR_TYPE_V6;
        }
        if (local_port != NULL)
            *local_port = ntohs(((struct sockaddr_in6 *)&local_addr)->sin6_port);
    }
    else
    {
        return 0;
    }
    return 1;
}

bool xy_domain_is_valid(char *domain)
{
    if (domain == NULL)
    {
        return 0;
    }

    uint32_t len = strlen(domain);

    if (len == 0 || len >= DNS_MAX_NAME_LENGTH)
    {
        return 0;
    }

    /* 检测ip字符串 */
    ip_addr_t addr = {0};
    if (ipaddr_aton(domain, &addr))
    {
        return xy_IpAddr_Check(domain, addr.type);
    }

    /* 首字母必须是英文字符或者数字 */
    if (!isalpha(*domain) && !isdigit(*domain))
    {
        return 0;
    }

    /* 末尾字母必须是英文字符或者数字 */
    if (!isalpha(*(domain + len - 1)) && !isdigit(*(domain + len - 1)))
    {
        return 0;
    }

    if (strchr(domain, '.') == NULL)
    {
        /* 域名中没有任何.字符 */
        return 0;
    }

    char *name = xy_malloc(len + 1);
    strcpy(name, domain);
    char *p = NULL;
    char *q = NULL;
    char *tmp = name;

    /* 获取第一个子字符串 */
    p = strtok_r(tmp, ".", &q);

    uint32_t i = 0;
    uint32_t label_len = 0;
    while (p != NULL)
    {
        label_len = strlen(p);
        if (label_len > 63)
        {
            xy_free(name);
            return 0;
        }
        for (i = 0; i < label_len; i++)
        {
            if (!isdigit(p[i]) && !isalpha(p[i]) && p[i] != '-')
            {
                xy_free(name);
                return 0;
            }
        }
        p = strtok_r(NULL, ".", &q);
    }

    xy_free(name);
    return 1;
}

