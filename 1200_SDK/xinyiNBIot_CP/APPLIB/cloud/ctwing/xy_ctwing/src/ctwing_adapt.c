#pragma once
#include "net_app_resume.h"
#include "ctlw_lwm2m_sdk.h"
#include "ctlw_NV_data.h"
#include "ctlw_abstract_signal.h"
#include "atc_ps_def.h"
#include "xy_ps_api.h"
#include "xy_system.h"
#include "ctlw_sdk_internals.h"
#include "rtc_utils.h"
#include "xy_rtc_api.h"
#include "ctwing_adapt.h"
#include "ctwing_resume.h"

#define CTLW_POWER_VOLTAGE     3800
#define CTLW_BATTERY_LEVEL     90
#define CTLW_MEMORY_FREE       50
#define CTLW_NETWORK_BEARER    5
#define CTLW_SIGNAL_STRENGTH   90
#define CTLW_CELL_ID           21103
#define CTLW_LINK_QUALITY      98
#define CTLW_LINK_UTRILIZATION 10
#define CTLW_POWER_SOURCE      1
#define CTLW_POWER_CURRENT     125
#define CTLW_LATITUDE          27.986065f
#define CTLW_LONGITUDE         86.922623f
#define CTLW_ALTITUDE          8495.0000f
#define CTLW_RADIUS            0.0f
#define CTLW_SPEED             0.0f
#define CTLW_TIME_CODE         1367491215
#define EVENT_PSNETIF_CTLW (EVENT_PSNETIF_IPV4_VALID | EVENT_PSNETIF_IPV6_VALID | EVENT_PSNETIF_IPV4V6_VALID | EVENT_PSNETIF_INVALID)

static char g_UTC_offset[7] = "+01:00";
static char g_timezone[25] = "Europe/Berlin";
//Ctwing 全局结构体，SDK运行时动态分配空间，保存SDk运行时的实时配置类和会话类数据信息
ctiot_context_t * contextInstance = NULL;

//Ctwing 全局，用于读写配置和会话类文件
unsigned char *g_ctlw_user_cache = NULL;


thread_handle_t g_ctlw_auto_update_rtc_resume_handle = NULL;//主动upate RTC线程句柄
osMutexId_t g_ctlw_module_entry_mutex = NULL;//模块初始化时使用的互斥锁
osTimerId_t g_ctlw_auto_update_tmr = NULL;//定时器，每90%(lifetime)自动向云平台发送update

extern thread_handle_t sendRecvThreadHandle;

/**
 * @brief
 * 适配电信SDK xy_ctlw_ctiot_get_context 函数
 * 内存优化,动态申请替代原SDK静态分配
 * @return ctiot_context_t*
 */
ctiot_context_t * xy_ctlw_ctiot_get_context()
{
    if(contextInstance == NULL)
    {
    	contextInstance = xy_malloc(sizeof(ctiot_context_t));
        memset(contextInstance, 0x00, sizeof(ctiot_context_t));
    }
    if(contextInstance->chipInfo == NULL)
    {
        contextInstance->chipInfo = xy_malloc(sizeof(ctiot_chip_module_info));
        memset(contextInstance->chipInfo, 0x00, sizeof(ctiot_chip_module_info));
    }

    return contextInstance;
}

static void xy_ctlw_auto_update_rtc_resume(PsStateChangeEvent event)
{
    if(g_ctlw_auto_update_rtc_resume_handle == NULL)
    {
        osThreadAttr_t thread_attr = {0};
        thread_attr.name = "ctlw_update_rtc";
        thread_attr.priority = osPriorityNormal1;
        thread_attr.stack_size = osStackShared;
        g_ctlw_auto_update_rtc_resume_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_auto_update_rtc_resume_task), NULL, &thread_attr);
    }
}


/**
 * @brief 主动update rtc回调
 * @note 适配电信SDK timer_callback接口
 */
void xy_ctlw_auto_update_rtc_callback()
{
    /**若唤醒原因为深睡唤醒,仅唤醒后第一次执行callback需要考虑业务线程的恢复,线程退出时句柄不置NULL*/
    if(Is_WakeUp_From_Dsleep())
    {
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_callback,WakeUp_From_Dsleep\r\n");
        
        if(xy_tcpip_is_ok())
        {
            ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_callback,tcpip is ok\r\n");
            if(g_ctlw_auto_update_rtc_resume_handle == NULL)
            {
                osThreadAttr_t thread_attr = {0};
                thread_attr.name = "ctlw_update_rtc";
                thread_attr.priority = osPriorityNormal1;
                thread_attr.stack_size = osStackShared;
                g_ctlw_auto_update_rtc_resume_handle = osThreadNew((osThreadFunc_t)(xy_ctlw_auto_update_rtc_resume_task), NULL, &thread_attr);
            }
        }
        else
        {
            ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_callback,reg_ps_call\r\n");
            xy_reg_psnetif_callback(EVENT_PSNETIF_VALID, xy_ctlw_auto_update_rtc_resume);
        }
    }
    else/**非deepsleep,会话状态正常*/
    {
        ctiot_set_auto_update_flag(1);
        ctiot_start_auto_update_timer();
        ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_auto_update_rtc_callback\r\n");
    }
}


/**
 * @brief 创建RTC,用于主动update
 * @note  适配电信SDK ctchip_start_sleep_timer接口
 * @param timeMs 
 */
void xy_ctlw_start_auto_update_rtc(uint32_t timeMs)
{
    xy_rtc_timer_create(RTC_TIMER_CTWING, timeMs, timer_callback, 0);

    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_start_auto_update_rtc,timeMs= %d \r\n",timeMs);
}


/**
 * @brief 删除用于主动update的RTC
 * @note 适配电信SDK ctchip_stop_sleep_timer接口
 */
void xy_ctlw_stop_auto_update_rtc(void)
{
	xy_rtc_timer_delete(RTC_TIMER_CTWING);
    
    ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_OTHER_CLASS,"xy_ctlw_stop_auto_update_rtc\r\n");
    return;
}




/**
 * @brief SDK从文件系统中读取session和config内容
 * @note 适配电信SDK接口 ctchip_get_nv
 * @return void* 
 */
void *xy_ctlw_get_sdk_nv(void)
{
	if(g_ctlw_user_cache == NULL)
	{
		g_ctlw_user_cache = (uint8_t *)xy_malloc(FLASH_CACHE_SIZE);
		memset(g_ctlw_user_cache, 0x00, sizeof(FLASH_CACHE_SIZE));
	}

    //读config文件
    cloud_read_file(CTLW_CFG_FILE_UNDER_DIR, (void*)g_ctlw_user_cache, sizeof(NV_params_t));
    
    //读session文件
    cloud_read_file(CTLW_SESSION_FILE_UNDER_DIR, (void*)(g_ctlw_user_cache + (sizeof(NV_params_t))), 
    (FLASH_CACHE_SIZE - sizeof(NV_params_t)));

	return g_ctlw_user_cache;
}

void xy_ctlw_flush_sdk_nv(ctlw_file_type_e type)
{
	ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"xy_ctlw_flush_sdk_nv,type=%d!\r\n",type);
    
    int32_t ret = XY_ERR;


    if(type == CONFIG_FILE)
    {
        ret = cloud_save_file(CTLW_CFG_FILE_UNDER_DIR,(void *)g_ctlw_user_cache, sizeof(NV_params_t));
        if(ret != XY_OK)
        {
            ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"xy_ctlw_flush_sdk_nv,cloud save config_file err\r\n");
        }
    }
    else if(type == SESSION_FILE)
    {
        if(pContext->sessionStatus != UE_LOGIN_OUTING)
        {
            ret = cloud_save_file(CTLW_SESSION_FILE_UNDER_DIR,(void *)(g_ctlw_user_cache + (sizeof(NV_params_t))), (FLASH_CACHE_SIZE - (sizeof(NV_params_t))));
        }
        else
        {
            ret = cloud_remove_file(CTLW_SESSION_FILE_UNDER_DIR);//删除Session文件
            
            if(ret != XY_OK)
            {
                ctiot_log_info(LOG_COMMON_MODULE, LOG_OTHER_CLASS,"CTLW cloud remove session err!\r\n");
            }
        }
    }
    else
    {
        xy_assert(0);//系统级别异常,理论不可能分支
    }
}

/*适配ctchip_updata_nv:托管写入数据区接口，芯翼暂不支持*/
void xy_update_SDK_NV(void)
{
}


/**
 * @brief 获取本地RTC时间
 * 
 * @return uint32_t 
 */
uint32_t xy_ctlw_gettime()
{
    return (uint32_t)(get_utc_ms()/1000);
}

void xy_ctlw_usleep(uint32_t usec)
{
	uint32_t i = 1000;
	uint32_t ms, tick;

	if (i < 1000)
	{
		while (i--)
			;
	}
	else
	{
		ms = usec / 1000;
		tick = ms * osKernelGetTickFreq() / 1000;
		osDelay(tick);
	}
}

int xy_ctlw_thread_create(osThreadId_t *thread, const osThreadAttr_t *attr, void *(*start_routine)(void *), void *arg)
{
    if(attr == NULL)
    { 
        osThreadAttr_t thread_attr = {0};
	    thread_attr.name	   = "ctlw_send_recv";
	    thread_attr.priority   = osPriorityNormal1;
	    thread_attr.stack_size = osStackShared;
        *thread = osThreadNew((osThreadFunc_t)start_routine, arg, &thread_attr);
    }
    else
        *thread = osThreadNew((osThreadFunc_t)start_routine, arg, attr);

	if (*thread != NULL)
		return 0;
	else
		return -1;
}

int xy_ctlw_thread_exit()
{
    osThreadExit();
	return 0;
}

int xy_ctlw_thread_cancel(osThreadId_t thread_id)
{
    osThreadTerminate(thread_id);
    return 0;
}

int xy_ctlw_mutex_init(osMutexId_t *mutex)
{
    *mutex = osMutexNew(NULL);
    return 0;
}

int xy_ctlw_mutex_lock(osMutexId_t *mutex)
{
    return osMutexAcquire(*mutex, osWaitForever);
}

int xy_ctlw_mutex_unlock(osMutexId_t *mutex)
{
    return osMutexRelease(*mutex);
}

int xy_ctlw_mutex_destroy(osMutexId_t *mutex)
{
    return osMutexDelete(*mutex);
}

uint16_t xy_ctlw_random(void)
{
    return xy_rand();
}


/**
 * @brief 获取本地IP类型，适配电信开源SDK ctlw_get_ip_type接口
 * 
 * @return uint16_t 
 */
uint16_t xy_ctlw_get_iptype()
{
	uint8_t ip_type = CHIP_IP_TYPE_FALSE;
    ip_type = xy_get_netif_iptype();

    switch (ip_type)
    {
    case IPV4_TYPE:
        ip_type = CHIP_IP_TYPE_V4ONLY;
        break;
    case IPV6_TYPE:
        ip_type = CHIP_IP_TYPE_V6ONLY;
        break;
    case IPV46_TYPE:
        ip_type = CHIP_IP_TYPE_V4V6;
        break;
    case IPV6PREPARING_TYPE:
        ip_type = CHIP_IP_TYPE_V6ONLY_V6PREPARING;
        break;
    case IPV4_IPV6PREPARING_TYPE:
        ip_type = CHIP_IP_TYPE_V4V6_V6PREPARING;
        break;
    default:
        ip_type = CHIP_IP_TYPE_FALSE;
		break;
    }
END:
	ctiot_log_debug(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ip_type:%d", ip_type);
	return ip_type;
}


uint16_t xy_ctlw_get_local_ip(char *localIP, int addrFamily)
{
	uint16_t result = CTIOT_NB_SUCCESS;
    uint16_t ip_type = ctlw_get_ip_type();
    ctiot_signal_set_chip_ip_type(ip_type);

	if(addrFamily == AF_INET)
	{
		if(ip_type == CHIP_IP_TYPE_V4ONLY || ip_type == CHIP_IP_TYPE_V4V6 || ip_type == CHIP_IP_TYPE_V4V6_V6PREPARING)
		{
            if(xy_getIP4Addr(localIP, XY_IP4ADDR_STRLEN) == 1)
                result = CTIOT_NB_SUCCESS;    
            else
                result = CTIOT_IP_NOK_ERROR;
		}

		else if(ip_type == CHIP_IP_TYPE_FALSE)
		{
			ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"CTIOT_IP_NOK_ERROR");
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ip_type == CHIP_IP_TYPE_V6ONLY || ip_type == CHIP_IP_TYPE_V6ONLY_V6PREPARING)
		{
			ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"CTIOT_IP_TYPE_ERROR");
			result = CTIOT_IP_TYPE_ERROR;
		}
	}
	else if(addrFamily == AF_INET6)
	{
		if(ip_type == CHIP_IP_TYPE_V6ONLY || ip_type == CHIP_IP_TYPE_V4V6)
		{
			if(xy_getIP6Addr(localIP, XY_IP6ADDR_STRLEN) == 1)
			    result = CTIOT_NB_SUCCESS;
            else
                result = CTIOT_IP_NOK_ERROR;
		}
		else if(ip_type == CHIP_IP_TYPE_FALSE)
		{
			result = CTIOT_IP_NOK_ERROR;
		}
		else if(ip_type == CHIP_IP_TYPE_V4ONLY)
		{
			result = CTIOT_IP_TYPE_ERROR;
		}
		else if(ip_type == CHIP_IP_TYPE_V6ONLY_V6PREPARING || ip_type == CHIP_IP_TYPE_V4V6_V6PREPARING)
		{
			result = CTIOT_IPV6_ONGOING_ERROR;
		}
	}

	return result;
}


void xy_ctlw_asyn_notify(char *at_str)
{
    send_urc_to_ext(at_str, strlen(at_str));
}

system_boot_reason_e xy_ctlw_get_system_boot_reason(void)
{
    system_boot_reason_e result = CTIOT_UNKNOWN_STATE;
    uint8_t bootupCause;

	bootupCause = Get_Boot_Reason();
	switch (bootupCause)
	{
		case POWER_ON:
		case GLOBAL_RESET:
		case SOFT_RESET:
		{
			result = CTIOT_ACTIVE_REBOOT;
			ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_ACTIVE_STATE,value=%d\r\n",result);
			break;
		}

		case WAKEUP_DSLEEP:
		{
			result = CTIOT_ACTIVE_WAKEUP;
			ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_HIB_STATE_MCU,value=%d\r\n",result);
			break;
		}
		default:
		{
			result = CTIOT_UNKNOWN_STATE;
			ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctchip_get_system_boot_reason:CTIOT_HIB_STATE_OTH,value=%d\r\n",result);
			break;
		}
	}
    return result;
}

uint16_t xy_ctlw_sync_cstate(void)
{
    int state = 0;
    uint16_t wirelessStatus = 0;

	xy_cereg_read(&state);
	switch(state)
	{
		case 0:
			wirelessStatus = NETWORK_UNCONNECTED;
		break;
		case 1:
			wirelessStatus = NETWORK_CONNECTED_HOME_NETWORK;
		break;
		case 2:
			wirelessStatus = NETWORK_SEARCHING;
		break;
		case 3:
			wirelessStatus = NETWORK_AUTHORIZE_FAILED;
		break;
        case 4:
            wirelessStatus = NETWORK_NO_SIGNAL;
        break;
		case 5:
			wirelessStatus = NETWORK_CONNECTED_ROAMING;
		break;
		default:
			wirelessStatus = NETWORK_UNCONNECTED;
		break;
	}

    return wirelessStatus;
}

uint16_t xy_ctlw_get_imsi_info(uint8_t *imsi,uint16_t maxImsiLen)
{
    uint16_t result = CTIOT_NB_SUCCESS;
    memset(imsi, 0, maxImsiLen);
	if(xy_get_IMSI(imsi,maxImsiLen) == ATC_AP_FALSE)
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_debug(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"xy_ctlw_get_imsi_info:get imsi failed\r\n");
	}
	else
	{
		ctiot_log_debug(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"xy_ctlw_get_imsi_info:imsi=%s\r\n", imsi);
	}
    return result;
}

uint16_t xy_ctlw_get_imei_info(uint8_t *imei,uint16_t maxImeiLen)
{
    uint16_t result = CTIOT_NB_SUCCESS;
	memset(imei, 0, maxImeiLen);
	if (xy_get_IMEI(imei, maxImeiLen) == ATC_AP_FALSE) 
	{
		result = CTIOT_OTHER_ERROR;
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"xy_ctlw_get_imsi_info:get imei failed\r\n");
	}
	else
	{
		ctiot_log_info(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"xy_ctlw_get_imsi_info:imsi=%s\r\n", imei);
	}

    return result;
}

uint8_t xy_ctlw_get_psm_mode(void)
{
    char *psmResult = xy_malloc(30);
    if(xy_atc_interface_call("AT+CPSMS?\r\n",NULL, psmResult) != ATC_AP_TRUE)
		xy_assert(0);

    char psmModes = strtok(psmResult,",");
    int psmMode = atoi(&psmModes);
	if(psmMode == 2 || psmMode == 0)
		psmMode = STATUS_NO_PSMING;

    xy_free(psmResult);
    
    return psmMode;
}

bool xy_ctlw_is_net_in_oos(void)
{
    if(ps_is_oos() == 1)
    	return true;
    return false;
}

uint16_t xy_ctlw_get_iccid_info(uint8_t* iccid,uint16_t maxIccidLen)
{
	if(xy_get_NCCID(iccid, maxIccidLen) == ATC_AP_FALSE)
	{
		return CTIOT_OTHER_ERROR;
	}
	else
	{
		ctiot_log_debug(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctiot_get_iccid_info:iccid=%s\r\n", iccid);
	}
    return CTIOT_NB_SUCCESS;
}

uint16_t xy_ctlw_get_apn_info(uint8_t* apn,uint16_t maxApnLen)
{
	if(xy_get_PDP_APN(apn, maxApnLen, -1) == ATC_AP_FALSE)
	{
		return CTIOT_OTHER_ERROR;
	}
	else
	{
		ctiot_log_debug(LOG_PREINIT_MODULE,LOG_OTHER_CLASS,"ctiot_get_apn_info:iccid=%s\r\n", apn);
	}

    return CTIOT_NB_SUCCESS;
}

uint16_t xy_ctlw_get_module_info(uint8_t* sv,uint16_t maxSvLen,uint8_t* chip,uint16_t maxChipLen,uint8_t* module,uint16_t maxModuleLen)
{
    //兼容Bx系列旧otp
	if((get_Soc_ver() == 0) || (get_Soc_ver() == 3))
    {
        strcpy(chip, "XY1200");
    }
    else if((get_Soc_ver() == 1) || (get_Soc_ver() == 4))
    {
        strcpy(chip, "XY1200S");
    }
    else if((get_Soc_ver() == 2) || (get_Soc_ver() == 5))
    {
        strcpy(chip, "XY2100S");
    }
    user_get_VERSIONEXT(sv, maxSvLen);	
    user_get_MODULVER(module, maxModuleLen);

    return CTIOT_NB_SUCCESS;
}

uint16_t xy_ctlw_get_rsrp(int32_t* rsrp)
{
    *rsrp = -120;
    return CTIOT_NB_SUCCESS;
}

uint16_t xy_ctlw_get_wireless_signal_info(uint8_t *rsrp, uint16_t maxRsrpLen, uint8_t *sinr, uint16_t maxSinrLen, uint8_t *txPower, uint16_t maxTxPowerLen, uint8_t *cellID, uint16_t maxCellIDLen)
{
	memset(rsrp,0,maxRsrpLen);
	memset(sinr,0,maxSinrLen);
	memset(txPower,0,maxTxPowerLen);
	memset(cellID,0,maxCellIDLen);
    unsigned char plmn_str[8]={0};

    ril_serving_cell_info_t *rcv_servingcell_info = xy_malloc(sizeof(ril_serving_cell_info_t));

    if(xy_get_servingcell_info(rcv_servingcell_info) != ATC_AP_TRUE)
    {
        xy_free(rcv_servingcell_info); 
        return 0;
    }

    if (NULL != rcv_servingcell_info->plmn)
    {
        plmn_str[0] = (unsigned char)(((rcv_servingcell_info->plmn) >> 16) & 0x0F) + 0x30;
        plmn_str[1] = (unsigned char)(((rcv_servingcell_info->plmn) >> 20) & 0x0F) + 0x30;
        plmn_str[2] = (unsigned char)(((rcv_servingcell_info->plmn) >> 8) & 0x0F) + 0x30;
        plmn_str[3] = (unsigned char)((rcv_servingcell_info->plmn) & 0x0F) + 0x30;
        plmn_str[4] = (unsigned char)(((rcv_servingcell_info->plmn) >> 4) & 0x0F) + 0x30;
        if (0x0F != (((rcv_servingcell_info->plmn) >> 12) & 0x0F))
        {
            plmn_str[5] = (((rcv_servingcell_info->plmn) >> 12) & 0x0F) + 0x30;
        }
    }

	sprintf(rsrp,"%d", rcv_servingcell_info->Signalpower/10);
	sprintf(sinr,"%d", rcv_servingcell_info->SNR/10);
	sprintf(txPower,"%d", rcv_servingcell_info->TXpower/10);
	sprintf(cellID,"%s%d", plmn_str, rcv_servingcell_info->CellID);

    xy_free(rcv_servingcell_info);
    return CTIOT_NB_SUCCESS;
}

uint16_t xy_ctlw_get_cell_id(uint32_t* cellID)
{
	if(cellID == NULL)
		return CTIOT_NB_FAILED;
	
	xy_get_CELLID(cellID);

	xy_printf(0,XYAPP, WARN_LOG, "cell_id %d", *cellID);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_manufacturer(char *manu, int len)
{
    user_get_Manufacturer(manu, 64);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_model_number(char *mode, int len)
{
    user_get_MODULVER(mode, len);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_serial_number(char *num, int len)
{
    user_get_SNumber(num, 64);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_firmware_ver(char *version, int len)
{
    user_get_VERSIONEXT(version, len);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_do_dev_reboot(void)
{
    xy_printf(0,XYAPP, WARN_LOG, "\r\ndevice is rebooting......\r\n");
    xy_Soft_Reset(SOFT_RB_BY_CP_USER);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_do_factory_reset(void)
{
    xy_printf(0,XYAPP, WARN_LOG, "\r\nFACTORY RESET\r\n");
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_power_source(int *arg)
{
    *arg = CTLW_POWER_SOURCE;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_source_voltage(int *voltage)
{
    *voltage = xy_getVbat();
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_power_current(int *arg)
{
    *arg = CTLW_POWER_CURRENT;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_baterry_level(int *voltage)
{
    *voltage = xy_getVbatCapacity();
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_memory_free(int *mem_free)
{
    *mem_free = cloud_get_ResveredMem();
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_dev_err(int *arg)
{
    *arg = CTIOT_NB_SUCCESS;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_do_reset_dev_err(void)
{
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_current_time(int64_t *arg)
{
    *arg = CTLW_TIME_CODE + (int64_t)xy_ctlw_gettime();
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_set_current_time(const int64_t *arg)
{
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_UTC_offset(char *offset, int len)
{
    if(len > strlen(g_UTC_offset) + 1)
    {
        snprintf(offset, len, g_UTC_offset);
    }
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_set_UTC_offset(const char *offset, int len)
{
    (void)snprintf(g_UTC_offset, len + 1, offset);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_timezone(char *timezone, int len)
{
    if(len > strlen(g_timezone) + 1)
    {
        (void)snprintf(timezone, len, g_timezone);
    }
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_set_timezone(const char *timezone, int len)
{
    (void)snprintf(g_timezone, len + 1, timezone);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_bind_mode(char *mode, int len)
{
    xy_printf(0,XYAPP, WARN_LOG, "bind type is UQ......\r\n");
    (void)snprintf(mode, len, "UQ");
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_trig_firmware_update(void)
{
    xy_printf(0,XYAPP, WARN_LOG, "firmware is updating......\r\n");
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_firmware_result(int *result)
{
    *result = 0;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_firmware_state(int *state)
{
    *state = 0;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_network_bearer(int *network_brearer)
{
    *network_brearer = CTLW_NETWORK_BEARER;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_signal_strength(int *singal_strength)
{
    xy_get_RSSI(singal_strength);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_link_quality(int *quality)
{
    *quality = CTLW_LINK_QUALITY;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_link_utilization(int *utilization)
{
    *utilization = CTLW_LINK_UTRILIZATION;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_update_psk(char *psk_id, int len)
{
	(void) psk_id;
	(void) len;

    xy_printf(0,XYAPP, WARN_LOG, "update psk success\r\n");
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_latitude(float *latitude)
{
    *latitude = CTLW_LATITUDE;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_longitude(float *longitude)
{
    *longitude = CTLW_LONGITUDE;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_altitude(float *altitude)
{
    *altitude = CTLW_ALTITUDE;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_radius(float *radius)
{
    *radius = CTLW_RADIUS;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_speed(float *speed)
{
    *speed = CTLW_SPEED;
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_get_timestamp(uint64_t *timestamp)
{
    *timestamp = (uint64_t)xy_ctlw_gettime() + CTLW_TIME_CODE;
    return CTIOT_NB_SUCCESS;
}

//-----  3GPP TS 23.032 V11.0.0(2012-09) ---------
#define HORIZONTAL_VELOCITY                  0
#define HORIZONTAL_VELOCITY_VERTICAL         1
#define HORIZONTAL_VELOCITY_WITH_UNCERTAINTY 2

#define VELOCITY_OCTETS                      5

static void location_get_velocity(ctlw_velocity_s *velocity,
                           uint16_t bearing,
                           uint16_t horizontal_speed,
                           uint8_t speed_uncertainty)
{
    uint8_t tmp[VELOCITY_OCTETS];
    int copy_len;

    tmp[0] = HORIZONTAL_VELOCITY_WITH_UNCERTAINTY << 4;
    tmp[0] |= (bearing & 0x100) >> 8;
    tmp[1] = (bearing & 0xff);
    tmp[2] = horizontal_speed >> 8;
    tmp[3] = horizontal_speed & 0xff;
    tmp[4] = speed_uncertainty;

    copy_len = MAX_VELOCITY_LEN > sizeof(tmp) ? sizeof(tmp) : MAX_VELOCITY_LEN;
    memcpy(velocity->opaque, tmp, copy_len);
    velocity->length = copy_len;
}

int xy_ctlw_get_velocity(ctlw_velocity_s *velocity)
{
    location_get_velocity(velocity, 0, 0, 255);
    return CTIOT_NB_SUCCESS;
}

int xy_ctlw_cmd_ioctl(ctlw_cmd_e cmd, char *arg, int len)
{
    int result = CTIOT_NB_SUCCESS;
    switch(cmd)
    {
    case CTLW_GET_MANUFACTURER:
        result = xy_ctlw_get_manufacturer(arg, len);
        break;
    case CTLW_GET_MODEL_NUMBER:
        result = xy_ctlw_get_model_number(arg, len);
        break;
    case CTLW_GET_SERIAL_NUMBER:
        result = xy_ctlw_get_serial_number(arg, len);
        break;
    case CTLW_GET_FIRMWARE_VER:
        result = xy_ctlw_firmware_ver(arg, len);
        break;
    case CTLW_DO_DEV_REBOOT:
        result = xy_ctlw_do_dev_reboot();
        break;
    case CTLW_DO_FACTORY_RESET:
        result = xy_ctlw_do_factory_reset();
        break;
    case CTLW_GET_POWER_SOURCE:
        result = xy_ctlw_get_power_source((int *)arg);
        break;
    case CTLW_GET_SOURCE_VOLTAGE:
        result = xy_ctlw_get_source_voltage((int *)arg);
        break;
    case CTLW_GET_POWER_CURRENT:
        result = xy_ctlw_get_power_current((int *)arg);
        break;
    case CTLW_GET_BATERRY_LEVEL:
        result = xy_ctlw_get_baterry_level((int *)arg);
        break;
    case CTLW_GET_MEMORY_FREE:
        result = xy_ctlw_get_memory_free((int *)arg);
        break;
    case CTLW_GET_DEV_ERR:
        result = xy_ctlw_get_dev_err((int *)arg);
        break;
    case CTLW_DO_RESET_DEV_ERR:
        result = xy_ctlw_do_reset_dev_err();
        break;
    case CTLW_GET_CURRENT_TIME:
        result = xy_ctlw_get_current_time((int64_t *)arg);
        break;
    case CTLW_SET_CURRENT_TIME:
        result = xy_ctlw_set_current_time((const int64_t *)arg);
        break;
    case CTLW_GET_UTC_OFFSET:
        result = xy_ctlw_get_UTC_offset(arg, len);
        break;
    case CTLW_SET_UTC_OFFSET:
        result = xy_ctlw_set_UTC_offset(arg, len);
        break;
    case CTLW_GET_TIMEZONE:
        result = xy_ctlw_get_timezone(arg, len);
        break;
    case CTLW_SET_TIMEZONE:
        result = xy_ctlw_set_timezone(arg, len);
        break;
    case CTLW_GET_BINDING_MODES:
        result = xy_ctlw_get_bind_mode(arg, len);
        break;
    case CTLW_GET_FIRMWARE_STATE:
        result = xy_ctlw_get_firmware_state((int *)arg);
        break;
    case CTLW_GET_NETWORK_BEARER:
        result = xy_ctlw_get_network_bearer((int *)arg);
        break;
    case CTLW_GET_SIGNAL_STRENGTH:
        result = xy_ctlw_get_signal_strength((int *)arg);
        break;
    case CTLW_GET_CELL_ID:
        result = xy_ctlw_get_cell_id((int *)arg);
        break;
    case CTLW_GET_LINK_QUALITY:
        result = xy_ctlw_get_link_quality((int *)arg);
        break;
    case CTLW_GET_LINK_UTILIZATION:
        result = xy_ctlw_get_link_utilization((int *)arg);
        break;
    /*case CTLW_WRITE_APP_DATA:
        result = atiny_write_app_write((int *)arg, len);
        break;*/
    case CTLW_UPDATE_PSK:
        result = xy_ctlw_update_psk(arg, len);
        break;
    case CTLW_GET_LATITUDE:
        result = xy_ctlw_get_latitude((float *)arg);
        break;
    case CTLW_GET_LONGITUDE:
        result = xy_ctlw_get_longitude((float *)arg);
        break;
    case CTLW_GET_ALTITUDE:
        result = xy_ctlw_get_altitude((float *)arg);
        break;
    case CTLW_GET_RADIUS:
        result = xy_ctlw_get_radius((float *)arg);
        break;
    case CTLW_GET_SPEED:
        result = xy_ctlw_get_speed((float *)arg);
        break;
    case CTLW_GET_TIMESTAMP:
        result = xy_ctlw_get_timestamp((uint64_t *)arg);
        break;
    case CTLW_GET_VELOCITY:
        result = xy_ctlw_get_velocity((ctlw_velocity_s *)arg);
        break;

    default:
        break;
    }
    return result;
}

void ctchip_ip_status_event_callback(PsStateChangeEvent event)
{
	uint16_t ip_type = CHIP_IP_TYPE_FALSE;

    if (event == EVENT_PSNETIF_IPV4_VALID || event == EVENT_PSNETIF_IPV6_VALID  || event == EVENT_PSNETIF_IPV4V6_VALID)
    {
    	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctchip_netif_event_callback, netif up");
		ip_type = ctlw_get_ip_type();
    }
    else 
    {
    	ctiot_log_debug(LOG_SEND_RECV_MODULE,LOG_IP_CLASS,"ctchip_netif_event_callback, netif down");
    }
	//ctchip_event_callback(NB_URC_ID_PS_NETINFO, &ip_type, sizeof(uint16_t));
}

void xy_ctlw_event_ip_status_init(void)
{
	xy_reg_psnetif_callback(EVENT_PSNETIF_CTLW, ctchip_ip_status_event_callback);
}

void xy_ctlw_event_ip_status_destroy(void)
{
	xy_deReg_psnetif_callback(EVENT_PSNETIF_CTLW, ctchip_ip_status_event_callback);
}
static void xy_ctlw_module_init(void)
{
    ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_init start!\r\n");

    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();

    ctiot_init_sdk_notify_cb();//initial notify

    ctiotprv_system_para_init(pContext);

    ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
    ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_init end!\r\n");
}

static void xy_ctlw_get_nvm_address_family(int32_t *addressFamily)
{
	NV_lwm2m_context_t *NV_lwm2m_context = (uint8_t *)xy_malloc(sizeof(NV_lwm2m_context_t));
    if(cloud_read_file(CTLW_SESSION_FILE_UNDER_DIR, (void*)NV_lwm2m_context, sizeof(NV_lwm2m_context_t)) == XY_OK)
    {
        *addressFamily = NV_lwm2m_context->addressFamily;
    }

    xy_free(NV_lwm2m_context);
}


static void xy_ctlw_module_resume(void)
{
    ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_resume enter!");
    
    //是否需要等待业务主线程执行完恢复流程
    bool wait_recoverflag = false;

    ctiot_context_t *pContext = xy_ctlw_ctiot_get_context();
    
    //尝试从session会话文件中获取注册标识位,用于判断最近一次SDK运行时是否为已注册状态
    xy_ctlw_get_nvm_bootflag(pContext);

    ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_resume,bootFlag=%d\r\n", pContext->bootFlag);

    //最近一次运行时是注册状态,需考虑执行恢复流程
    if(pContext->bootFlag == BOOT_LOCAL_BOOTUP)
    {
        //获取当前上电原因
        extern system_boot_reason_e  startReason;
        startReason = ctchip_get_system_boot_reason();

        if(startReason == CTIOT_ACTIVE_REBOOT)//POWER_ON或者NRB,软复位
		{
            //非睡眠唤醒下,未使用CTwing SDK执行FOTA且配置为非支持跨会话Reboot模式，不需要执行恢复流程
            if(pContext->fotaFlag == 0 && pContext->onKeepSession != SYSTEM_ON_REBOOT_KEEP_SESSION)
            {
                pContext->bootFlag = BOOT_NOT_LOAD;
                //删除session会话文件
                cloud_remove_file(CTLW_SESSION_FILE_UNDER_DIR);
                ctiot_update_session_status( NULL,pContext->sessionStatus, UE_NOT_LOGINED);
                ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_resume,active reboot,%d,%d,exit",pContext->fotaFlag, pContext->onKeepSession);
                return ;
            }
        }

        //睡眠唤醒或者非睡眠模式配置为跨reboot会话,需要恢复会话

        wait_recoverflag = true;//等待主线程恢复流程结束

        ctiotprv_init_system_list(pContext);

        xy_ctlw_get_nvm_address_family(&pContext->addressFamily); 

        xy_ctlw_create_sdk_resume_sem();

        ctiot_start_send_recv_thread();//启动主线程进行业务恢复
        
        if(wait_recoverflag == true)//等待业务恢复流程结束
        {
            xy_ctlw_acquire_sdk_resume_sem();
            xy_ctlw_delete_sdk_resume_sem();
        }
    }
    ctiot_log_info(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_resume end\r\n");
}



/**
 * @brief CTwing SDK模块启动入口
 * @warning  使用CTwing SDK模块任何功能前必须调用此接口
 * @note     内部流程仅会执行一次
 * @return int32_t
 */
int32_t xy_ctlw_module_entry()
{
    int32_t ret = CTIOT_NB_SUCCESS;

    static is_module_entryed = false;
    osMutexAcquire(g_ctlw_module_entry_mutex, osWaitForever);
    
    if(is_module_entryed == true)//模块已执行过启动流程
    {
        ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_module_have_entry,exit");
        goto exit;
    }
    if(!xy_tcpip_is_ok())//无网络,退出
    {
        osMutexRelease(g_ctlw_module_entry_mutex);
        return CTIOT_NETWORK_ERROR_BASE;
    }
    
    is_module_entryed = true;

    xy_ctlw_module_init();

    //检查是否存在Session会话文件,判断CTwing业务是否存在
    if(!xy_ctlw_check_if_session_file_exist())
    {
        ctiot_log_debug(LOG_PREINIT_MODULE, LOG_OTHER_CLASS,"xy_ctlw_entry,no session,exit");
        goto exit;//不存在session会话文件,退出
    }

    //session会话文件存在,尝试执行业务恢复
    xy_ctlw_module_resume();
    
exit:
    osMutexRelease(g_ctlw_module_entry_mutex);
    return ret;
}


