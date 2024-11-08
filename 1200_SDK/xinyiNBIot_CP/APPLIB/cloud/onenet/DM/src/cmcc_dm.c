#include <cis_def.h>
#include <cis_api.h>
#include <cis_if_sys.h>
#include <cis_def.h>
#include <cis_api.h>
#include "cis_internals.h"
#include "factory_nv.h"
#include "at_onenet.h"
#include "onenet_utils.h"
#include "net_app_resume.h"
#include "xy_system.h"
#include "xy_rtc_api.h"
#include "softap_nv.h"
#include "cmcc_dm.h"
#include "xy_at_api.h"
#include "oss_nv.h"
#include "ps_netif_api.h"
#include "xy_ps_api.h"
#include "xy_socket_api.h"

cmcc_dm_regInfo_t *g_cmcc_dm_regInfo = NULL;
osThreadId_t g_cmcc_dm_TskHandle = NULL;

#define RETRANS_MAX_TIMES           4

#define REG_PACKET_PAYLOAD      "</>;rt=\"oma.lwm2m\",</0/0>,</668>;ver=1.2,</668/0>,</669>;ver=1.2,</669/0>"
typedef struct
{
    int             sock;
    char            host[128];
    uint16_t        port;
    uint16_t        local_port; // xy add
    ip_addr_t       remote_addr;
}cmccdm_netinfo_t;

typedef struct
{
    uint16_t    msgid;              //主动请求的msgid
    uint8_t     reTranTimes;        //重传次数
    uint8_t     reqType;            //主动上行请求类型  0：register; 1: update
}cmccdm_request_t;

//static DMOptions *default_dm_config = NULL;

static void cmccdm_rtc_timeout_cb(void)
{
	xy_printf(0,XYAPP, WARN_LOG, "[CMCCDM]RTC callback, cmdmp state[%d]\n", g_softap_var_nv->cmdm_state);

   // g_cmcc_dm_rtcflag = 1;
    if(g_softap_var_nv->cmdm_state == CMDM_REG_SUCCESS || g_softap_var_nv->cmdm_state == CMDM_UPDATE_SUCCESS
            || g_softap_var_nv->cmdm_state == CMDM_UPDATE_TIMEOUT || g_softap_var_nv->cmdm_state == CMDM_UPDATE_NEED)
        g_softap_var_nv->cmdm_state = CMDM_UPDATE_NEED;
    else
        g_softap_var_nv->cmdm_state = CMDM_REG_NEED;

    cmcc_dm_init();
    return;
}

static int cmcc_send(cmccdm_netinfo_t *net_info,const uint8_t* buffer,uint32_t length)
{
    int nbSent = 0;
    struct sockaddr_in6 saddr;
    saddr.sin6_port = htons(net_info->port);
    if(net_info->remote_addr.type == IPADDR_TYPE_V6)
    {
        saddr.sin6_family = AF_INET6;
        inet6_aton(net_info->host, &saddr.sin6_addr);
    }
    else
    {
        saddr.sin6_family = AF_INET;
        ((struct sockaddr_in *)&saddr)->sin_addr.s_addr = inet_addr(net_info->host);
    }

    nbSent = sendto2(net_info->sock, (const char*)buffer, length, 0, (struct sockaddr *)&saddr, sizeof(saddr), 0, 0);

    if (nbSent == -1){
        xy_printf(0,XYAPP, WARN_LOG, "cmcc send [%s:%d] failed errno[%d]\n", net_info->host, ntohs(saddr.sin6_port), errno);
        return -1;
    }else{
        xy_printf(0,XYAPP, WARN_LOG, "cmcc send [%s:%d] %d bytes\n", net_info->host, ntohs(saddr.sin6_port), nbSent);
    }

    return XY_OK;
}
uint32_t get_query_items(uint8_t *payload)
{
    //DM服务器可能查询的项目 共24个
    char *str_items[] = {"imsi","mac","rom","ram","cpu","sysVersion",
                "softwareVer","softwareName","volte","netType","phoneNumber",
                "routerMac","bluetoothMac","sn","gpu","board","resolution",
                "batteryCapacity","screenSize","networkStatus","wearingStatus",
                "appInfo","imsi2","batteryCapacityCurr"};
    uint32_t ret = 0;
    char *tmp = NULL;
    char *p = payload + 4;      //跳过TLV header
    int i = 0;

    for(int i = 0; i < 24; i++)         //共24个
    {
        if((tmp = strstr(p, *(str_items + i))) != NULL)
        {
            if(*(tmp + strlen(*(str_items + i)) + 3) == 'Y')           //"imsi":"Y","mac":"N"
                ret |= (1 << i);
        }
    }

    xy_printf(0,XYAPP, WARN_LOG,"[cmcc_dm] query items 0x%x",ret);
    return ret;
}

uint32_t g_query_items = 0;             //服务器下发的查询内容bitmap

extern uint8_t cmdm_obj_read(int *numDataP, st_data_t ** dataArrayP);
static int socket_recv(cmccdm_netinfo_t *net_info, cmccdm_request_t *upstream_req)
{
    int result = -1, i = 0;
    struct timeval tv = {0};
    if(upstream_req == NULL)
        tv.tv_sec = 10;
    else
        tv.tv_sec = 4 * (1 + upstream_req->reTranTimes);
    fd_set readfds;
    uint8_t coap_error_code = COAP_NO_ERROR;
    coap_packet_t message[1];
    coap_packet_t response[1];

    FD_ZERO(&readfds);
    FD_SET(net_info->sock, &readfds);

    result = select(net_info->sock + 1, &readfds, NULL, NULL, &tv);
    if (result < 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CMCCDM]Err select(): %d %s\n", errno, strerror(errno));
        return -1;
    }
    else if (result > 0)
    {
        int numBytes;
        uint8_t* recv_data = xy_malloc(512);
        memset(recv_data, 0x00, 512);

        result = -1;
        /*
        * If an event happens on the socket
        */
        if (FD_ISSET(net_info->sock, &readfds))
        {
            struct sockaddr_storage addr;
            socklen_t addrLen;

            addrLen = sizeof(addr);

            /*
            * We retrieve the data received
            */
            numBytes = recvfrom(net_info->sock, (char*)recv_data, 512, 0, (struct sockaddr *)&addr, &addrLen);

            if (numBytes <= 0)
            {
                xy_printf(0,XYAPP, WARN_LOG, "[CMCCDM]Err recvfrom(): %d %s %d\n", errno, strerror(errno), numBytes);
                result = -1;
            }
            else if (numBytes > 0)
            {
                coap_error_code = coap_parse_message(message, recv_data, numBytes);

                if (coap_error_code == COAP_NO_ERROR)
                {
                    switch (message->type)
                    {
                    case COAP_TYPE_ACK:         //收到服务器回复的应答
                        if(upstream_req != NULL && message->mid == upstream_req->msgid)
                        {
                            if(upstream_req->reqType == 0)      //解析register请求应答
                            {
                                if(message->code == COAP_201_CREATED)
                                {
                                    if(message->location_path != NULL)  //解析下发的location path
                                    {
                                        memset(g_cmcc_dm_regInfo->serv_location, 0, sizeof(g_cmcc_dm_regInfo->serv_location));
                                        char *tmp = coap_get_multi_option_as_string(message->location_path);
                                        strcpy(g_cmcc_dm_regInfo->serv_location, tmp);
                                        xy_free(tmp);
                                    }

                                    result = CMDM_REG_SUCCESS;
                                }
                                else
                                {
                                    result = CMDM_REG_FAILED;
                                }
                            }
                            else            //解析update请求应答
                            {
                                if(message->code == COAP_204_CHANGED)
                                {
                                    result = CMDM_UPDATE_SUCCESS;
                                }
                                else
                                {
                                    result = CMDM_UPDATE_FAILED;
                                }
                            }
                        }
                        break;
                    case COAP_TYPE_CON:
                    {
                        st_uri_t *uriP = uri_decode(NULL, message->uri_path);
                        if(message->code == COAP_GET)
                        {
                            if(uriP->objectId == 669 && uriP->instanceId == 0)
                            {
                                st_data_t *dataP = data_new(1);
                                int size = 0,length = 0, pktBufferLen = 0;
                                uint8_t* bufferP = NULL;
                                uint8_t* pktBuffer = NULL;
                                et_media_type_t formatP = utils_convertMediaType(message->content_type);
                                uint8_t result = cmdm_obj_read(&size, &dataP);
                                if(result == COAP_205_CONTENT && dataP != NULL)
                                {
                                    length = data_serialize(uriP, 1, dataP, &formatP, &bufferP);
                                    if (length <= -1)
                                    {
                                        LOGD("object_asyn_ack_readdata data serialize failed.");
                                        result = COAP_500_INTERNAL_SERVER_ERROR;
                                    }
                                    data_free(1, dataP);
                                }

                                coap_init_message(response, COAP_TYPE_ACK, result, message->mid);
                                coap_set_header_token(response, message->token, message->token_len);
                                if(length > 0)
                                {
                                    coap_set_header_content_type(response, formatP);
                                    coap_set_payload(response, bufferP, length);
                                }

                                length = coap_serialize_get_size(response);
                                pktBuffer = (uint8_t *)cis_malloc(length);
                                pktBufferLen = coap_serialize_message(response, pktBuffer);
                                cmcc_send(net_info, pktBuffer, pktBufferLen); //发送
                                xy_free(pktBuffer);
                                if(bufferP != NULL)
                                    xy_free(bufferP);
                            }
                        }
                        else if(message->code == COAP_PUT)  //668应答
                        {
                            if(uriP->objectId == 668 && uriP->instanceId == 0)
                            {
                                int size = 0,length = 0, pktBufferLen = 0;
                                uint8_t* pktBuffer = NULL;

                                //xy_printf(">>>>[%d]%s", message->payload_len, message->payload);
                                g_query_items = get_query_items(message->payload);

                                coap_init_message(response, COAP_TYPE_ACK, COAP_204_CHANGED, message->mid);
                                coap_set_header_token(response, message->token, message->token_len);
                                length = coap_serialize_get_size(response);
                                pktBuffer = (uint8_t *)cis_malloc(length);
                                pktBufferLen = coap_serialize_message(response, pktBuffer);
                                cmcc_send(net_info, pktBuffer, pktBufferLen); //发送
                                xy_free(pktBuffer);
                            }
                        }
                        if(uriP != NULL)
                            xy_free(uriP);
                        break;
                    }
                    default:
                    xy_printf(0,XYAPP, WARN_LOG, "[CMCCDM]recv default: type[%d] code[%d]\n", message->type, message->code);
                        break;
                }
                coap_free_header(message);
                }
            }
        }
        xy_free(recv_data);
    }
    else
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CMCCDM]selecting... \n");
    }

    return result;
}

bool cmdm_sock_create(cmccdm_netinfo_t *net_info)
{
    if((g_softap_var_nv->cmdm_state == CMDM_UPDATE_NEED) && (xy_IpAddr_Check(g_cmcc_dm_regInfo->dm_reg_ip, IPV4_TYPE) || xy_IpAddr_Check(g_cmcc_dm_regInfo->dm_reg_ip, IPV6_TYPE)))
    {
        strcpy(net_info->host, g_cmcc_dm_regInfo->dm_reg_ip);
    }
    else
    {
        if(g_cmcc_dm_regInfo->dm_serv_flag == 0)
            strcpy(net_info->host, "m.fxltsbl.com");   //商用平台
        else
            strcpy(net_info->host, "shipei.fxltsbl.com");  //测试平台
    }

    net_info->port = 5683;
    if ((net_info->sock = xy_socket_by_host(net_info->host, 2, IPPROTO_UDP, 0, net_info->port, &net_info->remote_addr)) < 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cmdm]create sock error[%d]\r\n", net_info->sock);
        return false;
    }

    memset(net_info->host, 0, sizeof(net_info->host));
    ipaddr_ntoa_r(&net_info->remote_addr, net_info->host, sizeof(net_info->host));
    memset(g_cmcc_dm_regInfo->dm_reg_ip, 0, sizeof(g_cmcc_dm_regInfo->dm_reg_ip));
    strcpy(g_cmcc_dm_regInfo->dm_reg_ip, net_info->host);

    return true;
}

//清除失败的注册信息
void cmdm_regfail_clear()
{
    memset(g_cmcc_dm_regInfo->dm_reg_ip, 0, sizeof(g_cmcc_dm_regInfo->dm_reg_ip));
    memset(g_cmcc_dm_regInfo->serv_location, 0, sizeof(g_cmcc_dm_regInfo->serv_location));
    cloud_save_file(CMCC_DM_NVM_FILE_NAME,(void*)g_cmcc_dm_regInfo,sizeof(cmcc_dm_regInfo_t));
}

int genDmRegEndpointName(char ** data)
{
#define EP_NAME_LEN  (512)

    char brand[24] = {0};
    char model[24] = {0};
    char *sdkVer = "***";//"4.0.1";//"***";
    char *apiVer = "4.0.1";
    int apiType = 1;
    char imei1[20] = "***";
    char imei2[20] = "***";
    char *epNameStr = cis_malloc(EP_NAME_LEN);

    cmccdm_getBrandinfo(brand, sizeof(brand));
    cmccdm_getModelinfo(model,sizeof(model));
    cmccdm_getIMEI2info(imei2, sizeof(imei2));
    xy_get_IMEI(imei1, sizeof(imei1));

    snprintf((char *)epNameStr, EP_NAME_LEN, "%s||%s||%s||%s||%s||%s||%s||%s||%s||***||***||***", brand, model,
            g_cmcc_dm_regInfo->dm_app_key, sdkVer, apiVer, (apiType == 0)? "A":"I", DM_TEMPLEATEID_DEFAULT, imei1, imei2);

    int len = strlen(epNameStr);
    *data = cis_malloc(len + 1);
    strcpy(*data, epNameStr);
    xy_free(epNameStr);
    return len;
}

int make_req_packet(uint16_t coap_msgid, uint8_t **outbuf, uint8_t reqType)
{
    coap_packet_t message = {0};
    uint8_t temp_token[COAP_TOKEN_LEN] = {0};
    cis_time_t tv_sec = utils_gettime_s();
    char *query = NULL;

    coap_init_message(&message, COAP_TYPE_CON, COAP_POST, coap_msgid);
    // initialize first 6 bytes, leave the last 2 random
    temp_token[0] = (uint8_t)(coap_msgid);
    temp_token[1] = (uint8_t)(coap_msgid >> 8);
    temp_token[2] = (uint8_t)(tv_sec);
    temp_token[3] = (uint8_t)(tv_sec >> 8);
    temp_token[4] = (uint8_t)(tv_sec >> 16);
    temp_token[5] = (uint8_t)(tv_sec >> 24);
    // use just the provided amount of bytes
    coap_set_header_token(&message, temp_token, 4);

    if(reqType == 0)    //注册包
    {
        char *deviceName = NULL;
        genDmRegEndpointName(&deviceName);

        query = (char*)cis_malloc(CIS_COFNIG_REG_QUERY_SIZE);
        memset(query, 0, CIS_COFNIG_REG_QUERY_SIZE);
        strcpy(query, "?lwm2m=1.1&ep=");
        strcat(query, deviceName);
        strcat(query, "&b=U");
        strcat(query, "&lt=");
        utils_intCopy(query + strlen(query), CIS_COFNIG_REG_QUERY_SIZE - strlen(query), g_cmcc_dm_regInfo->dm_inteval_time * 60 * 10);

        coap_set_header_uri_path(&message, "/rd");
        coap_set_header_uri_query(&message, query);
        coap_set_header_content_type(&message, LWM2M_CONTENT_LINK);
        coap_set_payload(&message, REG_PACKET_PAYLOAD, strlen(REG_PACKET_PAYLOAD));
        xy_free(deviceName);
        xy_free(query);
    }
    else    //update包
    {
		coap_set_header_uri_path(&message, g_cmcc_dm_regInfo->serv_location);
    }
    int buf_len = coap_serialize_get_size(&message);
    *outbuf = (uint8_t*)cis_malloc(buf_len);
    buf_len = coap_serialize_message(&message, *outbuf);

    return buf_len;
}

void set_cmdm_rtc_by_state(int state)
{
    g_cmcc_dm_regInfo->have_retry_num++;

    //重试上限
    if(g_cmcc_dm_regInfo->have_retry_num> g_cmcc_dm_regInfo->retry_num)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[cmcc_dm] have tried %d times, stop retry ", g_cmcc_dm_regInfo->have_retry_num);
        g_cmcc_dm_regInfo->have_retry_num = 0;
        cloud_save_file(CMCC_DM_NVM_FILE_NAME,(void*)g_cmcc_dm_regInfo,sizeof(cmcc_dm_regInfo_t));
        return;
    }

    if(HWREGB(BAK_MEM_XY_DUMP) == 1)
    {
        char degStr[48] = {0};
        snprintf(degStr, 48, "+DBGINFO:[CMCCDM] run state[%d]\r\n", state);
        send_debug_by_at_uart(degStr);
    }

    if(state == CMDM_REG_SUCCESS || state == CMDM_UPDATE_SUCCESS)
    {
        //成功心跳RTC
        g_cmcc_dm_regInfo->have_retry_num = 0;
        xy_rtc_timer_create(RTC_TIMER_CMCCDM, g_cmcc_dm_regInfo->dm_inteval_time*60, cmccdm_rtc_timeout_cb, NULL);
        xy_printf(0,XYAPP, WARN_LOG, "[cmcc_dm] reg/update success[%d], next rtc[%d]", state, g_cmcc_dm_regInfo->dm_inteval_time*60);
    }
    else
    {
        //失败重试RTC
        xy_rtc_timer_create(RTC_TIMER_CMCCDM, g_cmcc_dm_regInfo->retry_time*60, cmccdm_rtc_timeout_cb, NULL);
        xy_printf(0,XYAPP, WARN_LOG, "[cmcc_dm] reg/update fail[%d], runtimes[%d] next rtc[%d]", state, g_cmcc_dm_regInfo->have_retry_num, g_cmcc_dm_regInfo->retry_time*60);
    }

    cloud_save_file(CMCC_DM_NVM_FILE_NAME,(void*)g_cmcc_dm_regInfo,sizeof(cmcc_dm_regInfo_t));

    return;
}

void cmcc_dm_run(void)
{
    coap_packet_t message = {0};
    cmccdm_request_t upstream_req = {0};
    cmccdm_netinfo_t *net_info = NULL;
    char *send_buf = NULL;
    int send_buf_len = 0, i = 0, send_result = 0;
    uint32_t startTime = 0;
    upstream_req.msgid = (uint16_t)xy_rand();

    //非上电初始化,需要reg/update, 直接退出
    if(g_softap_var_nv->cmdm_state != CMDM_INITIAL_STATE
            && g_softap_var_nv->cmdm_state != CMDM_UPDATE_NEED
            && g_softap_var_nv->cmdm_state != CMDM_REG_NEED)
    {
        g_cmcc_dm_TskHandle = NULL;
        osThreadExit();
    }

    cmcc_dm_config_init();

    if(g_softap_var_nv->cmdm_state == CMDM_INITIAL_STATE)
        g_cmcc_dm_regInfo->have_retry_num = 0;

    net_info = xy_malloc(sizeof(cmccdm_netinfo_t));
    memset(net_info, 0, sizeof(cmccdm_netinfo_t));

    if(!cmdm_sock_create(net_info)) //创建socket
    {
		set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);
        goto out;
    }

    if(g_softap_var_nv->cmdm_state == CMDM_UPDATE_NEED)
    {
        //send update packet
        upstream_req.msgid++;
        upstream_req.reqType = 1;
        send_buf_len = make_req_packet(upstream_req.msgid, &send_buf, upstream_req.reqType);

        for(i = 0; i < RETRANS_MAX_TIMES; i++)
        {
            upstream_req.reTranTimes = i;
            if(cmcc_send(net_info, send_buf, send_buf_len) < 0)
                continue;
            send_result = socket_recv(net_info, &upstream_req);   //接收处理发送结果
            if(send_result == CMDM_UPDATE_SUCCESS || send_result == CMDM_UPDATE_FAILED)
                break;
        }

        if(send_result == 0)
        {
            g_softap_var_nv->cmdm_state = CMDM_UPDATE_TIMEOUT;
            set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);
        }
        else if(send_result == -1 || send_result == CMDM_UPDATE_FAILED)
        {
            g_softap_var_nv->cmdm_state = CMDM_UPDATE_FAILED;
            set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);
        }
        else
        {
            g_softap_var_nv->cmdm_state = CMDM_UPDATE_SUCCESS;
            set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);//在等待服务器读取信息前设RTC,否则update唤醒时间将延迟

            startTime = cloud_gettime_s();
            while((cloud_gettime_s() - startTime) < 30)         //等待服务器读取信息,共等30s
               socket_recv(net_info, NULL);
        }
    }
    else
    {
        //send register packet
        upstream_req.reqType = 0;
        send_buf_len = make_req_packet(upstream_req.msgid, &send_buf, upstream_req.reqType);
        //send_debug_by_at_uart("+DBGINFO: [CMCCDM]dm register start\r\n");

        for(i = 0; i < RETRANS_MAX_TIMES; i++)
        {
            upstream_req.reTranTimes = i;
            if(cmcc_send(net_info, send_buf, send_buf_len) < 0) //发送失败直接退出,socket或网络异常,待下次触发
                continue;
            send_result = socket_recv(net_info, &upstream_req);   //接收处理发送结果
            if(send_result == CMDM_REG_SUCCESS || send_result == CMDM_REG_FAILED)
                break;
       }

       if(send_result == 0)
       {
           g_softap_var_nv->cmdm_state = CMDM_REG_TIMEOUT;
           set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);
       }
       else if(send_result == -1 || send_result == CMDM_REG_FAILED)
       {
           g_softap_var_nv->cmdm_state = CMDM_REG_FAILED;
           set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);
       }
       else
       {
           g_softap_var_nv->cmdm_state = CMDM_REG_SUCCESS;
            set_cmdm_rtc_by_state(g_softap_var_nv->cmdm_state);//在等待服务器读取信息前设RTC,否则update唤醒时间将延迟

            startTime = cloud_gettime_s();
           //等待服务器读取 /666/0,防止平台收不到ACK，共等20s
            while((cloud_gettime_s() - startTime) < 30)     //等待服务器读取信息,共等30s
               socket_recv(net_info, NULL);
       }

    }

    close(net_info->sock);
    xy_free(send_buf);
out:
    xy_free(net_info);
    xy_printf(0,XYAPP, WARN_LOG,"[CMCCDM] cmcc_dm thread exit..");
    g_cmcc_dm_TskHandle = NULL;
    osThreadExit();
}

void cmcc_user_config_init()
{
    /*cmcc_dm 初始值配置*/
    g_cmcc_dm_regInfo->dm_inteval_time = 1440;
    strcpy(g_cmcc_dm_regInfo->dm_app_key,"M100000329");
    strcpy(g_cmcc_dm_regInfo->dm_app_pwd,"43648As94o1K8Otya74T2719D51cmy58");
    g_cmcc_dm_regInfo->retry_num = 5;
    g_cmcc_dm_regInfo->retry_time = 10;
}

void cmcc_dm_config_init()
{
    static uint16_t  s_cmccdm_inited = 0;
    if(!s_cmccdm_inited)
    {
        s_cmccdm_inited = 1;
        g_cmcc_dm_regInfo = xy_malloc(sizeof(cmcc_dm_regInfo_t));
        memset(g_cmcc_dm_regInfo, 0, sizeof(cmcc_dm_regInfo_t));
        if( XY_ERR == cloud_read_file(CMCC_DM_NVM_FILE_NAME,(void*)g_cmcc_dm_regInfo,sizeof(cmcc_dm_regInfo_t)))
            cmcc_user_config_init();
    }
}

void cmcc_dm_init()
{
    osThreadAttr_t task_attr = {0};
    if (g_cmcc_dm_TskHandle == NULL && g_softap_fac_nv->need_start_dm)
    {
        xy_printf(0,XYAPP, WARN_LOG,"start cmcc dm task!!!");
        task_attr.name = "cmcc_dm";
        task_attr.priority = osPriorityNormal1;
        task_attr.stack_size = osStackShared;
        g_cmcc_dm_TskHandle = osThreadNew((osThreadFunc_t)(cmcc_dm_run), NULL, &task_attr);
    }
}

//AT+XYDMP=<mode>,<interval_time>,<key>,<pwd>,<test>
int at_XYDMP_req(char *at_buf, char **prsp_cmd)
{
    cmcc_dm_config_init();
    if(g_req_type == AT_CMD_REQ)
    {
        int mode;
        int interval = 0;
        char *key = NULL;
        char *pwd = NULL;
        int test = 0;
        int retry_num = -1;
        int retry_time = -1;
        if (at_parse_param("%d(0-1),%d(1-1440),%p(),%p(),%d(0-1),%d[1-255],%d[1-65535]",
                at_buf, &mode, &interval, &key, &pwd, &test, &retry_num, &retry_time) != AT_OK
                || strlen(key) > 64 || strlen(pwd) > 64)
        {
            return ATERR_PARAM_INVALID;
        }

        if(mode == 0)
            g_softap_fac_nv->need_start_dm = 0;
        else
        {
            g_softap_fac_nv->need_start_dm = 1;
            g_cmcc_dm_regInfo->dm_inteval_time = interval;

            memset(g_cmcc_dm_regInfo->dm_app_key, 0, 64);
            memset(g_cmcc_dm_regInfo->dm_app_pwd, 0, 64);
            strcpy(g_cmcc_dm_regInfo->dm_app_key, key);
            strcpy(g_cmcc_dm_regInfo->dm_app_pwd, pwd);
            g_cmcc_dm_regInfo->dm_serv_flag = test;
            if(retry_num > 0)
                g_cmcc_dm_regInfo->retry_num = retry_num;
            if(retry_time > 0)
                g_cmcc_dm_regInfo->retry_time = retry_time;
        }
        SAVE_FAC_PARAM(need_start_dm);
        cloud_save_file(CMCC_DM_NVM_FILE_NAME,(void*)g_cmcc_dm_regInfo,sizeof(cmcc_dm_regInfo_t));
    }
    else if(g_req_type == AT_CMD_QUERY)
    {
        *prsp_cmd = xy_malloc(100);
        snprintf(*prsp_cmd, 100, "%d,%d,%s,%s,%d,%d,%d",
            g_softap_fac_nv->need_start_dm, g_cmcc_dm_regInfo->dm_inteval_time,
            g_cmcc_dm_regInfo->dm_app_key, g_cmcc_dm_regInfo->dm_app_pwd,
            g_cmcc_dm_regInfo->dm_serv_flag, g_cmcc_dm_regInfo->retry_num,
            g_cmcc_dm_regInfo->retry_time);
    }
    else
    {
       return ATERR_PARAM_INVALID;
    }
    return AT_END;
}

