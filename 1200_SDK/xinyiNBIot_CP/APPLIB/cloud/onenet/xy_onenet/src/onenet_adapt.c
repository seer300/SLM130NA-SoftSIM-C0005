/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "xy_system.h"
#include "at_onenet.h"
#include <cis_if_sys.h>
#include <cis_def.h>
#if CIS_ENABLE_UPDATE
#include <cis_config.h>
#include <cis_api.h>
#include <qspi_flash.h>
#include "dma.h"
#include "sha.h"
#include "xy_rtc_api.h"
#include "xy_fota.h"
#include "factory_nv.h"
#include "cloud_utils.h"
#include "xy_flash.h"
#include "xy_ps_api.h"
#include "xy_socket_api.h"
#include "net_app_resume.h"
#include "onenet_utils.h"
#include "net_app_mem.h"
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
#define MAX_PACKET_SIZE            (1400)//512  // Maximal size of radio packet
#define CIST_SOCK_RECV_STACK_SIZE  1024*3

#define CIS_FOTA_OFFSET     3600

/*******************************************************************************
 *                        Local function declarations                          *
 ******************************************************************************/

/*******************************************************************************
 *                         Local variable definitions                          *
 ******************************************************************************/

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
static unsigned int  s_cis_fwState = 0;             //cis fota update state
static unsigned int  s_cis_fwRes = 0;               //cis fota update result
static unsigned int  s_cis_fwSize = 0;              //cis fota update data size

unsigned int rcv_size = 0;
osTimerId_t cis_fota_timer_overdue = NULL;

extern osSemaphoreId_t g_cis_rcv_sem;
extern osSemaphoreId_t cis_poll_sem;
extern onenet_session_info_t *g_onenet_session_info;
extern osMutexId_t g_onenet_mutex;
// extern int g_FOTAing_flag;
extern unsigned int  xy_getVbatCapacity(void);
/*******************************************************************************
 *                      Inline function implementations                        *
 ******************************************************************************/

/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/

//Get firmware version
uint32_t cissys_getFwVersion(uint8_t **version)
{
    char *tmpVersion = (char *)*version;
    unsigned int len = 0;
    sprintf(tmpVersion, "%s", g_softap_fac_nv->versionExt);

    xy_printf(0,XYAPP, WARN_LOG, "[CIS] get fwVersion:%s", *version);

    len = strlen(tmpVersion);
    return len;
}

//Get cell id from nv
void cissys_getCellId(int *cellid)
{
    xy_get_CELLID(cellid);
}

//Get battery level (TODO)
uint32_t cissys_getFwBatteryLevel()
{
    //TO DO
    uint32_t btLevel = xy_getVbatCapacity();
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] battery level:%u",btLevel);
    return btLevel;
}

//Get battery voltage (TODO)
uint32_t cissys_getFwBatteryVoltage()
{
    //TO DO
    uint32_t btVoltage = xy_getVbat();
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] battery Voltage:%u(mv)",btVoltage);

    return btVoltage;
}

//Get available memory for firmware udpate (TODO)
uint32_t cissys_getFwAvailableMemory()
{
    uint32_t availableMemory = cloud_get_ResveredMem();
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] available Memory:%u(Byte)",availableMemory);

    return availableMemory;
}

//Get radio signal strength (TODO)
uint32_t cissys_getRadioSignalStrength()
{
    //TO DO
    int radioSignalStrength = -1;
    xy_get_RSSI(&radioSignalStrength);
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] RadioSignal Strength:%u",radioSignalStrength);

    return (uint32_t)radioSignalStrength;
}



//check firmware data after downloading (TODO)
bool cissys_checkFwValidation(cissys_callback_t *cb)
{
    if(OTA_delta_check())
    {
        cb->onEvent((cissys_event_t)cissys_event_fw_validate_fail, NULL, cb->userData, 0);
        return false;

    }
    else
    {
        cb->onEvent((cissys_event_t)cissys_event_fw_validate_success, NULL, cb->userData, 0);
        return true;
    }
}

//erase flash data after updating (TODO)
bool cissys_eraseFwFlash(cissys_callback_t *cb)
{
    //TO DO
    if(1)
    {
        //erase fota data success
        cb->onEvent((cissys_event_t)cissys_event_fw_erase_success, NULL, cb->userData, 0);
        return true;
    }
    else
    {
        //erase fota data failed
        cb->onEvent((cissys_event_t)cissys_event_fw_erase_fail, NULL, cb->userData, 0);
        return false;
    }
}

//clear the firmware size info (TODO)
void cissys_ClearFwBytes(void)
{
    s_cis_fwSize = 0;
}

//save firmware data and write to flash
uint32_t    cissys_writeFwBytes(uint32_t size, uint8_t * buffer, cissys_callback_t * cb)
{
    //unsigned long offset = 0;
    st_context_t *context = cb->userData;
    if((s_cis_fwSize+size) > cloud_get_ResveredMem())
    {
        cb->onEvent((cissys_event_t)cissys_event_write_fail, NULL, cb->userData, 0);
        return 1;
    }

    if (OTA_save_one_packet(buffer, size) != XY_OK)
    {
        cb->onEvent((cissys_event_t)cissys_event_write_fail, NULL, cb->userData, 0);
        return 1;
    }

#if CIS_ENABLE_UPDATE
    if(context->fw_request.block1_more == 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]last fota pkt");
    }
#endif

    rcv_size = rcv_size + size;
    xy_printf(0,XYAPP, WARN_LOG, "[CIS]block size:%d, had rcv_size:%d", size, rcv_size);

    cb->onEvent((cissys_event_t)cissys_event_write_success, NULL, cb->userData, 0);
    return 1;

}


//record the firmware data size
void cissys_savewritebypes(uint32_t size)
{
    s_cis_fwSize = s_cis_fwSize + size;
}

//execute updating command after downloading firmware data
bool cissys_updateFirmware(cissys_callback_t * cb)
{
    cb->onEvent((cissys_event_t)cissys_event_fw_update_success, NULL, cb->userData, 0);
    return true;

}

//read fota context
bool cissys_readContext(cis_fw_context_t * context)
{
    cis_fw_context_t *fw_context = context;
    if(NULL == fw_context)
        return 1;

    fw_context->state = s_cis_fwState;
    fw_context->result = s_cis_fwRes;
    fw_context->savebytes = s_cis_fwSize;

    return 0;
}

//Get saved firmware data size
int cissys_getFwSavedBytes()
{
    return  s_cis_fwSize;
}

//Set updating result and reboot
bool cissys_setFwUpdateResult(uint8_t result)
{
    s_cis_fwRes = result;
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] fw result:%d", result);
    if(result != FOTA_RESULT_DISCONNECT)
    {
        if(cis_fota_timer_overdue != NULL)
        {
            osTimerDelete(cis_fota_timer_overdue);
            cis_fota_timer_overdue = NULL;
        }
    }

    if(result == FOTA_RESULT_SUCCESS)
    {
        osDelay(4000);
#if CIS_ENABLE_UPDATE
        g_onenet_config_data->onenet_fota_flag = 1;
        cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)g_onenet_config_data,sizeof(onenet_config_nvm_t));
        OTA_upgrade_start();
#endif
    }

    return true;
}

static void cissys_rtc_cb(void *para)
{
    if(para == NULL)
        return ;
    int fota_ret = 2;           //失败

    st_context_t* context = (st_context_t*)para;
    firmware_data_t * firmwareInstance = (firmware_data_t *)(context->firmware_inst);
    if (firmwareInstance == NULL)
    {
        LOGE("No firmware instance");
        return ;
    }
    cis_fw_context_t* fw_info = (cis_fw_context_t*)(firmwareInstance->fw_info);
    if (fw_info == NULL)
    {
        LOGE("No fota information");
        return ;
    }

    fw_info->result = FOTA_RESULT_DISCONNECT;
    cissys_setFwUpdateResult(fw_info->result);
    context->fw_request.write_state = PACKAGE_WRITE_IDIL;
    reset_fotaIDIL(context, CIS_EVENT_FIRMWARE_DOWNLOAD_FAILED);
}
//Reset for fota;
//if failed fota and unregister need reset
void cissys_resetFota(void)
{
    rcv_size = 0;
    s_cis_fwSize = 0;
    s_cis_fwState = 0;
    s_cis_fwRes = 0;
}

//Save firmware downloading state
bool cissys_setFwState(uint8_t state)
{

    if(s_cis_fwState == state)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS] fw state:%d, no need to set");
        return 0;
    }


    s_cis_fwState = state;
    xy_printf(0,XYAPP, WARN_LOG, "[CIS] fw state:%d", state);

    switch (state)
    {
        case  FOTA_STATE_DOWNLOADING:

            // g_FOTAing_flag = 1;
            //ota_set_state(XY_FOTA_DOWNLOADING);

            OTA_upgrade_init();

            //fota超时处理
            if(cis_fota_timer_overdue != NULL)
            {
                osTimerDelete(cis_fota_timer_overdue);
                cis_fota_timer_overdue = NULL;
            }

            osTimerAttr_t time_attr = {0};
            time_attr.name = "cis_fota_overdue";
            cis_fota_timer_overdue = osTimerNew((osTimerFunc_t)(cissys_rtc_cb), osTimerOnce, NULL, &time_attr);
            osTimerStart(cis_fota_timer_overdue, CIS_FOTA_OFFSET*1000);

            break;
        case  FOTA_STATE_DOWNLOADED:
            // g_FOTAing_flag = 1;
            //ota_set_state(XY_FOTA_DOWNLOADED);
            xy_printf(0,XYAPP, WARN_LOG, "[CIS]switch to FOTA DOWNLOADED STATE");
            break;
        case  FOTA_STATE_UPDATING:
            // g_FOTAing_flag = 1;
            //ota_set_state(XY_FOTA_UPGRADING);
            break;
        case  FOTA_STATE_IDIL:
            // g_FOTAing_flag = 0;
            //ota_set_state(XY_FOTA_IDLE);
            break;
        default:
            break;
    }

    return 0;
}

/*cis公有云FOTA升级结果URC上报*/
void cissys_ota_urc_send(void)
{
    onenet_config_nvm_t *temp_t = xy_malloc(sizeof(onenet_config_nvm_t));
    if(XY_ERR == cloud_read_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)(temp_t),sizeof(onenet_config_nvm_t))
        || temp_t->onenet_fota_flag == 0)
    {
            xy_free(temp_t);
            return;
    }


    int ret = OTA_get_upgrade_result();
    if(ret == XY_OK)
    {
        send_urc_to_ext("+FIRMWARE UPDATE SUCCESS", strlen("+FIRMWARE UPDATE SUCCESS"));
        // ota_state_callback(4);
        //ota_set_state(XY_FOTA_UPGRADE_SUCCESS);
    }
    else
    {
        send_urc_to_ext("+FIRMWARE UPDATE FAILED", strlen("+FIRMWARE UPDATE FAILED"));
        // ota_state_callback(5);
        //ota_set_state(XY_FOTA_UPGRADE_FAIL);
    }

    //清除config中的fota_flag标志位
    temp_t->onenet_fota_flag = 0;
    if(Is_OpenCpu_Ver())
        ((onenet_config_nvm_t *)(g_cloud_mem_p + CLOUD_BAKUP_HEAD))->onenet_fota_flag = 0;
    cloud_save_file(ONENET_CONFIG_NVM_FILE_NAME,(void*)(temp_t),sizeof(onenet_config_nvm_t));
    xy_free(temp_t);
}
#endif

cis_ret_t cissys_init(void *context,const cis_cfg_sys_t* cfg,cissys_callback_t* event_cb)
{
    (void) cfg;

#if CIS_ENABLE_UPDATE
    st_context_t *contextP = (st_context_t *)context;
    if(contextP == NULL)
        return CIS_RET_ERROR;
    //xy_printf(0,XYAPP, WARN_LOG, "[~~~~~~]init fw callback");
    contextP->g_fotacallback.onEvent = event_cb->onEvent;
    contextP->g_fotacallback.userData = event_cb->userData;

//  cis_memcmp(&contextP->g_fotacallback, event_cb, sizeof(cissys_callback_t));
#endif
    return CIS_RET_OK;
}
clock_t cissys_tick(void)
{
    return osKernelGetTickCount();
}
uint32_t cissys_gettime()
{
    return cloud_gettime_s();
}
void cissys_sleepms(uint32_t ms)
{
    osDelay(ms);
}
void cissys_logwrite(uint8_t* buffer,uint32_t length)
{
    (void) length;

    xy_printf(0,XYAPP, WARN_LOG, "%s", buffer);
}

void cissys_free(void* buffer)
{
    xy_free(buffer);
}

void* cissys_memset( void* s, int c, size_t n)
{
    return memset(s, c, n);
}
void* cissys_memcpy(void* dst, const void* src, size_t n)
{
    return memcpy(dst, src, n);
}
void* cissys_memmove(void* dst, const void* src, size_t n)
{
    return memmove(dst, src, n);
}
int cissys_memcmp( const void* s1, const void* s2, size_t n)
{
    return memcmp(s1, s2, n);
}
void cissys_fault(uint16_t id)
{
    (void) id;

    xy_printf(0,XYAPP, WARN_LOG, "cissys_fault");
}
uint32_t cissys_rand()
{
    return xy_rand();
}

void cissys_assert(bool flag)
{
    xy_assert(flag);
}

bool cissys_load(uint8_t* buffer,uint32_t length)
{
    (void) buffer;
    (void) length;

    return true;
}
bool cissys_save(uint8_t* buffer,uint32_t length)
{
    (void) buffer;
    (void) length;

    return true;
}

uint8_t     cissys_getSN(char* buffer,uint32_t maxlen)
{

    if(maxlen < 32)
        return 0;

    xy_get_SN(buffer, maxlen);

    //xy_printf(0,XYAPP, WARN_LOG, "TEST  SN: %s", buffer);
    return strlen(buffer);
}

void cissys_reboot()
{
    xy_Soft_Reset(SOFT_RB_BY_CP_USER);
}


static int create_cisnet_sock(cisnet_t ctx)
{
    if (ctx == NULL || g_onenet_session_info == NULL)
        return XY_ERR;

    int ret = XY_OK;
    ip_addr_t* remote_addr = (ip_addr_t*)xy_malloc(sizeof(ip_addr_t));

    if ((ctx->sock = xy_socket_by_host2(ctx->host, Sock_IPv46, IPPROTO_UDP, g_onenet_session_info->net_info.local_port, ctx->port, remote_addr)) == -1)
    {
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]create cisnet sock error[%d]\r\n", ctx->sock);
        ret = XY_ERR;
    }
    else
    {
        xy_socket_local_info(ctx->sock, NULL, &ctx->local_port);    //获取本地端口
        memcpy(&g_onenet_session_info->net_info.remote_ip, remote_addr, sizeof(ip_addr_t)); //保存远端ip
        memset(ctx->host, 0, sizeof(ctx->host));
        ipaddr_ntoa_r(remote_addr, ctx->host, sizeof(ctx->host));
        //session获取本地ip。    防止网口up（V6激活晚于V4）时，获取到的session中的本地ip为空，触发IP地址变化
        xy_get_ipaddr(g_onenet_session_info->net_info.remote_ip.type, &g_onenet_session_info->net_info.local_ip);
    }

    if (remote_addr != NULL)
        xy_free(remote_addr);

    return ret;
}

void cis_sock_recv_thread(void* lpParam)
{

    cisnet_t netctx = (cisnet_t)lpParam;
    //st_context_t* net_context = (st_context_t*)netctx->context;
    int sock = netctx->sock;
    while(0 == netctx->quit && netctx->state == 1)
    {
        struct timeval tv = {2,0};
        fd_set readfds, writefds, exceptfds;
        int result;

        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        /*
            * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
            * with the precedent function)
            */
        //[XY]Add for onenet loop
        result = select(sock + 1, &readfds, &writefds, &exceptfds, &tv);
        xy_printf(0,XYAPP, WARN_LOG, "[CIS]select result(%d): %d %s\n", result, errno, strerror(errno));
        if (result < 0)
        {
            xy_printf(0,XYAPP, WARN_LOG, "Error in select(): %d %s\n", errno, strerror(errno));
            goto TAG_END;
        }
        else if (result > 0)
        {
            uint8_t buffer[MAX_PACKET_SIZE];
            int numBytes;

            /*
                * If an event happens on the socket
                */
            if (FD_ISSET(sock, &readfds))
            {
                struct sockaddr_storage addr;
                socklen_t addrLen;

                addrLen = sizeof(addr);

                /*
                    * We retrieve the data received
                    */
                numBytes = recvfrom(sock, (char*)buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

                if (numBytes < 0)
                {
                    if(errno == 0)continue;
                    xy_printf(0,XYAPP, WARN_LOG, "Error in recvfrom(): %d %s\n", errno, strerror(errno));
                }
                else if (numBytes > 0)
                {
                    char str_addr[INET6_ADDRSTRLEN];
                    uint16_t  port;
                    if(addr.ss_family == AF_INET6)
                    {
                        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&addr;
                        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, str_addr, INET6_ADDRSTRLEN);
                        port = saddr->sin6_port;
                    }
                    else
                    {
                        struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                        inet_ntop(saddr->sin_family, &saddr->sin_addr, str_addr, INET6_ADDRSTRLEN);
                        port = saddr->sin_port;
                    }
                    xy_printf(0,XYAPP, WARN_LOG, "[CIS] %d bytes received from [%s]:%d \n", numBytes, str_addr, ntohs(port));
                    if(strcmp(netctx->host, str_addr) != 0)
                    {
                        osMutexAcquire(g_onenet_mutex, osWaitForever);
                        strcpy(netctx->host, str_addr);
                        osMutexRelease(g_onenet_mutex);
                    }

                    uint8_t* data = (uint8_t*)cis_malloc(numBytes);
                    cis_memcpy(data,buffer,numBytes);

                    struct st_net_packet *packet = (struct st_net_packet*)cis_malloc(sizeof(struct st_net_packet));
                    packet->next = NULL;
                    packet->buffer = data;
                    packet->length = numBytes;
                    //packet->netctx = netctx;
                    osMutexAcquire(netctx->onenet_socket_mutex, osWaitForever);
                    netctx->g_packetlist = (struct st_net_packet*)cis_packet_add(netctx->g_packetlist, packet);
                    osMutexRelease(netctx->onenet_socket_mutex);
                    //[XY]Add for onenet loop
                    if(cis_poll_sem != NULL)
                        osSemaphoreRelease(cis_poll_sem);

                }
            }
        }
    }
TAG_END:
    xy_printf(0,XYAPP, WARN_LOG, "socket recv thread exit..\n");
    close(sock);
    //netctx->cis_sock_recv_thread_id = -1;
    netctx->cis_sock_recv_thread_id = NULL;
    osSemaphoreRelease(g_cis_rcv_sem);


    osThreadExit();
    return ;
}

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/
cis_ret_t cisnet_init(void *context,const cisnet_config_t* config,cisnet_callback_t cb)
{
    xy_printf(0,XYAPP, WARN_LOG, "cisnet_init\n");
    cis_memcpy(&((struct st_cis_context *)context)->netConfig,config,sizeof(cisnet_config_t));
    ((struct st_cis_context *)context)->netCallback.onEvent = cb.onEvent;
     //vTaskDelay(1000);
    //((struct st_cis_context *)context)->netAttached = 1;
    return CIS_RET_OK;
}

cis_ret_t cisnet_create(cisnet_t* netctx,const char* host,void* context)
{
    xy_printf(0,XYAPP, WARN_LOG, "cisnet_create\n");
    st_context_t * ctx = (st_context_t *)context;

    //if (ctx->netAttached != 1) return CIS_RET_ERROR;

    cisnet_t pctx = (cisnet_t)cissys_malloc(sizeof(struct st_cisnet_context));
    if (pctx == NULL) {
        xy_printf(0,XYAPP, WARN_LOG, "Cannot malloc cisnet_t");
        return CIS_RET_ERROR;
    }
    memset(pctx, 0, sizeof(struct st_cisnet_context));

    /* PEER_PORT used for test, this port use as server port*/
    pctx->port = (int)strtol((const char *)(ctx->serverPort),NULL,10);
     /*The address is type string, need address gethostbyname ? */
    strcpy(pctx->host, host);
    pctx->context = context;
    pctx->cis_sock_recv_thread_id = NULL;

    if (create_cisnet_sock(pctx) < 0) {
        xy_printf(0,XYAPP, WARN_LOG, "Cannot create socket");
        cissys_free(pctx);
        return CIS_RET_ERROR;
    }

    cloud_mutex_create(&pctx->onenet_socket_mutex);
    (*netctx) = pctx;

    return CIS_RET_OK;
}

void cisnet_destroy(cisnet_t netctx)
{
    struct st_net_packet *temp, *node;
    xy_printf(0,XYAPP, WARN_LOG, "cisnet_destroy\n");
    if (netctx->context) {
        //close(netctx->sock); // let the sock die naturally, close the sock in recv thread before thread exits
        netctx->context = NULL;
    }
    //CIS_LIST_FREE(netctx->g_packetlist);
    node = netctx->g_packetlist;
    while (node != NULL)
    {
        temp = node;
        node = node->next;
        cis_free(temp->buffer);
        cis_free(temp);
    }
    osMutexDelete(netctx->onenet_socket_mutex);
    netctx->onenet_socket_mutex = NULL;
    cissys_free(netctx);
}

cis_ret_t cisnet_connect(cisnet_t netctx)
{
    osThreadAttr_t task_attr = {0};

    xy_printf(0,XYAPP, WARN_LOG, "cisnet_connect\n");
    //char cis_sock_recv_thread_name[32] = {0};
    st_context_t* net_context = (st_context_t*)netctx->context;
    netctx->state = 1;
    //snprintf(cis_sock_recv_thread_name, 32, "cis_sock_recv_task_%d", netctx->sock);
    if (netctx->cis_sock_recv_thread_id == NULL)
    {
#if CIS_ENABLE_DM
        if (net_context->isDM)
        {
            task_attr.name = "cisdm_rcv";
            task_attr.priority = osPriorityNormal1;
            task_attr.stack_size = osStackShared;
            netctx->cis_sock_recv_thread_id = osThreadNew ((osThreadFunc_t)(cis_sock_recv_thread),netctx,&task_attr);
        }
        else
        {
#endif
            task_attr.name = "cisck_rcv";
            task_attr.priority = osPriorityNormal1;
            task_attr.stack_size = osStackShared;
            netctx->cis_sock_recv_thread_id = osThreadNew ((osThreadFunc_t)(cis_sock_recv_thread),netctx,&task_attr);

            if (netctx->cis_sock_recv_thread_id != NULL)
                osThreadSetLowPowerFlag(netctx->cis_sock_recv_thread_id, osLpmNoRealtime);
#if CIS_ENABLE_DM

        }
#endif
    }
    if (netctx->cis_sock_recv_thread_id == NULL)
        return CIS_RET_ERROR;
    net_context->netCallback.onEvent(netctx, cisnet_event_connected, NULL, netctx->context);
    return CIS_RET_OK;
}

cis_ret_t cisnet_disconnect(cisnet_t netctx)
{
    xy_printf(0,XYAPP, WARN_LOG, "cisnet_disconnect\n");
    st_context_t* net_context = (st_context_t*)netctx->context;
    netctx->state = 0;

    if(netctx->cis_sock_recv_thread_id != NULL)
    {
        osDelay(2000); //收包线程不参与唤醒，此处唤醒会执行到收包线程释放信号量
        if(g_cis_rcv_sem != NULL)
            osSemaphoreAcquire(g_cis_rcv_sem, osWaitForever);
    }
    net_context->netCallback.onEvent(netctx, cisnet_event_disconnect, NULL, netctx->context);
    return CIS_RET_OK;
}

cis_ret_t cisnet_write(cisnet_t netctx,const uint8_t* buffer,uint32_t length, uint8_t raiflag)
{
    int nbSent;
    size_t addrlen;
    size_t offset;
    uint8_t type_offset = 2;

    struct sockaddr_in6 saddr;
    saddr.sin6_port = htons(netctx->port);
    if(g_onenet_session_info->net_info.remote_ip.type == IPADDR_TYPE_V6)
    {
        saddr.sin6_family = AF_INET6;
        inet6_aton(netctx->host, &saddr.sin6_addr);
    }
    else
    {
        saddr.sin6_family = AF_INET;
        ((struct sockaddr_in *)&saddr)->sin_addr.s_addr = inet_addr(netctx->host);
    }

    addrlen = sizeof(saddr);
    offset = 0;
    while (offset != length)
    {
        //nbSent = sendto(netctx->sock, (const char*)buffer + offset, length - offset, 0, (struct sockaddr *)&saddr, addrlen);

        nbSent = sendto2(netctx->sock, (const char*)buffer + offset, length - offset, 0, (struct sockaddr *)&saddr, addrlen, 0, raiflag);

        if (nbSent == -1){
            xy_printf(0,XYAPP, WARN_LOG, "socket sendto [%s:%d] failed.\n", netctx->host, ntohs(saddr.sin6_port));
            return -1;
        }else{
            xy_printf(0,XYAPP, WARN_LOG, "socket sendto [%s:%d] %d bytes\n", netctx->host, ntohs(saddr.sin6_port), nbSent);
        }
        offset += nbSent;
    }

    return CIS_RET_OK;
}

cis_ret_t cisnet_read(cisnet_t netctx,uint8_t** buffer,uint32_t *length)
{
    if(netctx->g_packetlist != NULL){
        osMutexAcquire(netctx->onenet_socket_mutex, osWaitForever);
        struct st_net_packet* delNode;
        *buffer = netctx->g_packetlist->buffer;
        *length = netctx->g_packetlist->length;
         delNode = netctx->g_packetlist;
         netctx->g_packetlist = netctx->g_packetlist->next;
         cis_free(delNode);
         osMutexRelease(netctx->onenet_socket_mutex);
         return CIS_RET_OK;
    }

    return CIS_RET_ERROR;
}

cis_ret_t cisnet_free(cisnet_t netctx,uint8_t* buffer,uint32_t length)
{
    (void) netctx;
    (void) length;

    cissys_free(buffer);
    return CIS_RET_OK;
}


