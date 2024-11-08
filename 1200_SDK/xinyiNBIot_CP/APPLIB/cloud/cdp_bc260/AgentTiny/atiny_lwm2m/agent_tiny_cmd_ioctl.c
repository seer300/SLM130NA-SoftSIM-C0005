/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#if TELECOM_VER
#include "xy_utils.h"
#include "xy_at_api.h"
#include "factory_nv.h"
#include "oss_nv.h"
#include "agent_tiny_cmd_ioctl.h"
#include "agent_tiny_demo.h"
#include "agenttiny.h"
#include "atiny_osdep.h"
#include "cloud_utils.h"
#include "cdp_backup.h"
#include "xy_system.h"
#include "xy_ps_api.h"
#include "cdp_backup.h"
#include "net_app_resume.h"

#ifdef CONFIG_FEATURE_FOTA
#include "atiny_fota_manager.h"
#include "atiny_fota_state.h"
#endif

#if defined(WITH_AT_FRAMEWORK) && defined(USE_NB_NEUL95)
#include "at_device/bc95.h"
#endif

#define ATINY_POWER_VOLTAGE     3800
#define ATINY_BATTERY_LEVEL     90
#define ATINY_MEMORY_FREE       50
#define ATINY_NETWORK_BEARER    5
#define ATINY_SIGNAL_STRENGTH   90
#define ATINY_CELL_ID           21103
#define ATINY_LINK_QUALITY      98
#define ATINY_LINK_UTRILIZATION 10
#define ATINY_POWER_SOURCE      1
#define ATINY_POWER_CURRENT     125
#define ATINY_LATITUDE          27.986065f
#define ATINY_LONGITUDE         86.922623f
#define ATINY_ALTITUDE          8495.0000f
#define ATINY_RADIUS            0.0f
#define ATINY_SPEED             0.0f
#define ATINY_TIME_CODE         1367491215

#define UTC_OFFSET_MAX_LEN 7
#define TIMEZONE_MAXLEN 25

//-----  3GPP TS 23.032 V11.0.0(2012-09) ---------
#define HORIZONTAL_VELOCITY                  0
#define HORIZONTAL_VELOCITY_VERTICAL         1
#define HORIZONTAL_VELOCITY_WITH_UNCERTAINTY 2
#define VELOCITY_OCTETS                      5


extern osSemaphoreId_t cdp_wait_sem;
extern int cdp_deregister_flag;
extern int g_send_status;
extern cdp_config_nvm_t *g_cdp_config_data;


static int err_code = ATINY_OK;
static int64_t g_current_time = ATINY_TIME_CODE;
static char g_UTC_offset[UTC_OFFSET_MAX_LEN] = "+01:00";
static char g_timezone[TIMEZONE_MAXLEN] = "Europe/Berlin";
static int g_rsrp, g_sinr,g_txpower,g_CellID = -1;
uint8_t plmn_str[8] = {0};

int atiny_get_manufacturer(char *manu, int len)
{
    char *manufacturer = xy_malloc(64);
    user_get_Manufacturer(manufacturer, 64);
    (void)atiny_snprintf(manu, len, manufacturer);
    xy_free(manufacturer);
    return ATINY_OK;
}

int atiny_get_model_number(char *mode, int len)
{
    (void)atiny_snprintf(mode, len,(const char *)g_softap_fac_nv->modul_ver);
    return ATINY_OK;
}

int atiny_get_serial_number(char *num, int len)
{
    char *serial_number = xy_malloc(64);
    sprintf(serial_number, "345000123");
    (void)atiny_snprintf(num, len, serial_number);
    xy_free(serial_number);
    return ATINY_OK;
}

int atiny_get_firmware_ver(char *version, int len)
{
    (void)atiny_snprintf(version, len,(const char *)g_softap_fac_nv->versionExt);
    return ATINY_OK;
}

int atiny_do_dev_reboot(void)
{
    (void)atiny_printf("\r\ndevice is rebooting......\r\n");
    //LOS_TaskDelay(1000);
    atiny_reboot();
    return ATINY_OK;
}

int atiny_do_factory_reset(void)
{
    (void)atiny_printf("\r\nFACTORY RESET\r\n");
    return ATINY_OK;
}

int atiny_get_power_source(int *arg)
{
    *arg = ATINY_POWER_SOURCE;
    return ATINY_OK;
}

int atiny_get_source_voltage(int *voltage)
{
    *voltage = xy_getVbat();
    return ATINY_OK;
}

int atiny_get_power_current(int *arg)
{
    *arg = ATINY_POWER_CURRENT;
    return ATINY_OK;
}

int atiny_get_baterry_level(int *voltage)
{
   // *voltage = xy_getVbatCapacity();
    return ATINY_OK;
}

int atiny_get_memory_free(int *voltage)
{
   // *voltage = cloud_get_ResveredMem();
    return ATINY_OK;
}

int atiny_get_dev_err(int *arg)
{
    *arg = err_code;
    return ATINY_OK;
}

int atiny_do_reset_dev_err(void)
{
    err_code = ATINY_OK;
    return ATINY_OK;
}

int atiny_get_current_time(int64_t *arg)
{
    *arg = g_current_time + (int64_t)cloud_gettime_s();
    return ATINY_OK;
}

int atiny_set_current_time(const int64_t *arg)
{
    g_current_time = *arg - (int64_t)cloud_gettime_s();
    return ATINY_OK;
}

int atiny_get_UTC_offset(char *offset, int len)
{
    if((size_t)len > strlen(g_UTC_offset) + 1)
    {
        (void)atiny_snprintf(offset, len, g_UTC_offset);
    }
    return ATINY_OK;
}

int atiny_set_UTC_offset(const char *offset, int len)
{
    (void)atiny_snprintf(g_UTC_offset, len + 1, offset);
    return ATINY_OK;
}

int atiny_get_timezone(char *timezone, int len)
{
    if((size_t)len > strlen(g_timezone) + 1)
    {
        (void)atiny_snprintf(timezone, len, g_timezone);
    }
    return ATINY_OK;
}

int atiny_set_timezone(const char *timezone, int len)
{
    (void)atiny_snprintf(g_timezone, len + 1, timezone);
    return ATINY_OK;
}

int atiny_get_bind_mode(char *mode, int len)
{
    (void)atiny_printf("bind type is UQ......\r\n");
	if(g_cdp_config_data->binding_mode == 1)
		atiny_snprintf(mode, len, "U");
	else if(g_cdp_config_data->binding_mode == 2)
		atiny_snprintf(mode, len, "UQ");
    return ATINY_OK;
}

int atiny_trig_firmware_update(void)
{
    (void)atiny_printf("firmware is updating......\r\n");
    return ATINY_OK;
}

int atiny_get_firmware_result(int *result)
{
    *result = 0;
    return ATINY_OK;
}

int atiny_get_firmware_state(int *state)
{
    *state = 0;
    return ATINY_OK;
}

int atiny_get_network_bearer(int *network_brearer)
{
    *network_brearer = ATINY_NETWORK_BEARER;
    return ATINY_OK;
}

int atiny_get_signal_strength(int *singal_strength)
{
    xy_get_RSSI(singal_strength);
    return ATINY_OK;
}
int atiny_get_cell_id(int *cellID)
{
	if(cellID == NULL)
		return ATINY_ERR;
	
	xy_get_CELLID(cellID);

	xy_printf(0,XYAPP, WARN_LOG, "cell_id %d", *cellID);
	return ATINY_OK;
}

int atiny_get_link_quality(int *quality)
{
    *quality = ATINY_LINK_QUALITY;
    return ATINY_OK;
}

int atiny_get_link_utilization(int *utilization)
{
    *utilization = ATINY_LINK_UTRILIZATION;
    return ATINY_OK;
}

int atiny_write_app_write(void *user_data, int len)
{
    (void)atiny_printf("write num19 object success\r\n");
    new_message_indication(user_data, len);
    return ATINY_OK;
}

int atiny_update_psk(char *psk_id, int len)
{
	(void) psk_id;
	(void) len;

    //memcpy_s(g_psk_value,psk_id,len,16);
    (void)atiny_printf("update psk success\r\n");
    return ATINY_OK;
}

int atiny_get_latitude(float *latitude)
{
    *latitude = ATINY_LATITUDE;
    return ATINY_OK;
}

int atiny_get_longitude(float *longitude)
{
    *longitude = ATINY_LONGITUDE;
    return ATINY_OK;
}

int atiny_get_altitude(float *altitude)
{
    *altitude = ATINY_ALTITUDE;
    return ATINY_OK;
}

int atiny_get_radius(float *radius)
{
    *radius = ATINY_RADIUS;
    return ATINY_OK;
}

int atiny_get_speed(float *speed)
{
    *speed = ATINY_SPEED;
    return ATINY_OK;
}

int atiny_get_timestamp(uint64_t *timestamp)
{
    *timestamp = (uint64_t)cloud_gettime_s() + ATINY_TIME_CODE;
    return ATINY_OK;
}

int atiny_get_binding_mode(char *bind)
{
	if(g_cdp_config_data->binding_mode == 1)
		strcpy(bind, "U");
	else if(g_cdp_config_data->binding_mode == 2)
		strcpy(bind, "UQ");
    return ATINY_OK;
}

int atiny_set_binding_mode(char *bind)
{
	if(strcmp(bind,"U") == 0)
		g_cdp_config_data->binding_mode = 1;
	else if(strcmp(bind,"UQ") == 0)
		g_cdp_config_data->binding_mode = 2;
	 cloud_save_file(CDP_CONFIG_NVM_FILE_NAME,(void*)g_cdp_config_data,sizeof(cdp_config_nvm_t));	
    return ATINY_OK;
}


char *atiny_get_registerationQuery()
{
	char modulver[20]={0};
	char *str = xy_malloc(300);
    memset(str, 0x00, 300);
	char *tempstr = str;
	ril_serving_cell_info_t *rcv_servingcell_info = xy_malloc(sizeof(ril_serving_cell_info_t));

	strcat(tempstr, "&imsi=");
	tempstr += (strlen(tempstr));
	if(!xy_get_IMSI(tempstr, IMSI_LEN))
		goto err_proc;

	strcat(tempstr, "&iccid=");
	tempstr += (strlen(tempstr));
	if(!xy_get_NCCID(tempstr,21))
		goto err_proc;

	strcat(tempstr, "&sv=");
	tempstr += (strlen(tempstr));
	if(user_get_VERSIONEXT(tempstr, 28) != 1)
		goto err_proc;

	strcat(tempstr, "&module=");
	tempstr += (strlen(tempstr));
	if(user_get_MODULVER(modulver, 20)!= 1)
		goto err_proc;

	char *tmpstr1 = strstr(modulver, "-");
	if(tmpstr1 != NULL)
	{
		memcpy(tempstr,  tmpstr1+1, strlen(tmpstr1+1));
	}
	
	user_get_chiptype(modulver,20);
	if(g_CellID == -1)
	{
		if(!xy_get_servingcell_info(rcv_servingcell_info))
			goto err_proc;

		if(NULL != rcv_servingcell_info->plmn)
		{
			plmn_str[0] = (uint8_t)(((rcv_servingcell_info->plmn) >> 16) & 0x0F) + 0x30;
	        plmn_str[1] = (uint8_t)(((rcv_servingcell_info->plmn) >> 20) & 0x0F) + 0x30;
	        plmn_str[2] = (uint8_t)(((rcv_servingcell_info->plmn) >> 8) & 0x0F) + 0x30;
	        plmn_str[3] = (uint8_t)((rcv_servingcell_info->plmn) & 0x0F) + 0x30;
	        plmn_str[4] = (uint8_t)(((rcv_servingcell_info->plmn) >> 4) & 0x0F) + 0x30;
	        if (0x0F != (((rcv_servingcell_info->plmn) >> 12) & 0x0F))
	        {
	            plmn_str[5] = (((rcv_servingcell_info->plmn) >> 12) & 0x0F) + 0x30;
	        }
		}

		g_rsrp = rcv_servingcell_info->Signalpower/10;
		g_sinr = rcv_servingcell_info->SNR/10;
		g_txpower = rcv_servingcell_info->TXpower/10;
		g_CellID = rcv_servingcell_info->CellID;
		
		tempstr += (strlen(tempstr));
		sprintf(tempstr,"&chip=%s&rsrp=%d&sinr=%d&txpower=%d&cellid=%s%d",modulver,g_rsrp,g_sinr,g_txpower,plmn_str,g_CellID);
	}
	else
	{
		tempstr += (strlen(tempstr));
		sprintf(tempstr,"&chip=%s&rsrp=%d&sinr=%d&txpower=%d&cellid=%s%d",modulver,g_rsrp,g_sinr,g_txpower,plmn_str,g_CellID);
		g_rsrp=g_sinr=g_txpower=0;
		memset(plmn_str, 0x00, 8);
		g_CellID = -1;
	}
	
	xy_free(rcv_servingcell_info); 
	return str;

err_proc:
	xy_free(str);
	xy_free(rcv_servingcell_info);
	return NULL;
}

void location_get_velocity(atiny_velocity_s *velocity,
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

int atiny_get_velocity(atiny_velocity_s *velocity)
{
    location_get_velocity(velocity, 0, 0, 255);
    return ATINY_OK;
}

int atiny_cmd_ioctl(atiny_cmd_e cmd, char *arg, int len)
{
    int result = ATINY_OK;
    switch(cmd)
    {
    case ATINY_GET_MANUFACTURER:
        result = atiny_get_manufacturer(arg, len);
        break;
    case ATINY_GET_MODEL_NUMBER:
        result = atiny_get_model_number(arg, len);
        break;
    case ATINY_GET_SERIAL_NUMBER:
        result = atiny_get_serial_number(arg, len);
        break;
    case ATINY_GET_FIRMWARE_VER:
        result = atiny_get_firmware_ver(arg, len);
        break;
    case ATINY_DO_DEV_REBOOT:
        result = atiny_do_dev_reboot();
        break;
    case ATINY_DO_FACTORY_RESET:
        result = atiny_do_factory_reset();
        break;
    case ATINY_GET_POWER_SOURCE:
        result = atiny_get_power_source((int *)arg);
        break;
    case ATINY_GET_SOURCE_VOLTAGE:
        result = atiny_get_source_voltage((int *)arg);
        break;
    case ATINY_GET_POWER_CURRENT:
        result = atiny_get_power_current((int *)arg);
        break;
    case ATINY_GET_BATERRY_LEVEL:
        result = atiny_get_baterry_level((int *)arg);
        break;
    case ATINY_GET_MEMORY_FREE:
        result = atiny_get_memory_free((int *)arg);
        break;
    case ATINY_GET_DEV_ERR:
        result = atiny_get_dev_err((int *)arg);
        break;
    case ATINY_DO_RESET_DEV_ERR:
        result = atiny_do_reset_dev_err();
        break;
    case ATINY_GET_CURRENT_TIME:
        result = atiny_get_current_time((int64_t *)arg);
        break;
    case ATINY_SET_CURRENT_TIME:
        result = atiny_set_current_time((const int64_t *)arg);
        break;
    case ATINY_GET_UTC_OFFSET:
        result = atiny_get_UTC_offset(arg, len);
        break;
    case ATINY_SET_UTC_OFFSET:
        result = atiny_set_UTC_offset(arg, len);
        break;
    case ATINY_GET_TIMEZONE:
        result = atiny_get_timezone(arg, len);
        break;
    case ATINY_SET_TIMEZONE:
        result = atiny_set_timezone(arg, len);
        break;
    case ATINY_GET_BINDING_MODES:
        result = atiny_get_bind_mode(arg, len);
        break;
    case ATINY_GET_FIRMWARE_STATE:
        result = atiny_get_firmware_state((int *)arg);
        break;
    case ATINY_GET_NETWORK_BEARER:
        result = atiny_get_network_bearer((int *)arg);
        break;
    case ATINY_GET_SIGNAL_STRENGTH:
        result = atiny_get_signal_strength((int *)arg);
        break;
    case ATINY_GET_CELL_ID:
        result = atiny_get_cell_id((int *)arg);
        break;
    case ATINY_GET_LINK_QUALITY:
        result = atiny_get_link_quality((int *)arg);
        break;
    case ATINY_GET_LINK_UTILIZATION:
        result = atiny_get_link_utilization((int *)arg);
        break;
    case ATINY_WRITE_APP_DATA:
        result = atiny_write_app_write((int *)arg, len);
        break;
    case ATINY_UPDATE_PSK:
        result = atiny_update_psk(arg, len);
        break;
    case ATINY_GET_LATITUDE:
        result = atiny_get_latitude((float *)arg);
        break;
    case ATINY_GET_LONGITUDE:
        result = atiny_get_longitude((float *)arg);
        break;
    case ATINY_GET_ALTITUDE:
        result = atiny_get_altitude((float *)arg);
        break;
    case ATINY_GET_RADIUS:
        result = atiny_get_radius((float *)arg);
        break;
    case ATINY_GET_SPEED:
        result = atiny_get_speed((float *)arg);
        break;
    case ATINY_GET_TIMESTAMP:
        result = atiny_get_timestamp((uint64_t *)arg);
        break;
    case ATINY_GET_VELOCITY:
        result = atiny_get_velocity((atiny_velocity_s *)arg);
        break;
    case ATINY_GET_BINGING_MODE:
        result = atiny_get_binding_mode((char *)arg);
        break;
    case ATINY_SET_BINGING_MODE:
        result = atiny_set_binding_mode((char *)arg);
        break;

#if defined(WITH_AT_FRAMEWORK) && defined(USE_NB_NEUL95)
    case ATINY_TRIGER_SERVER_INITIATED_BS:
        nb_reattach();
        result = ATINY_OK;
        break;
#endif

    default:
        break;
    }
    return result;
}

void atiny_event_notify(atiny_event_e event, const char* arg, int len)
{
    atiny_report_type_e rpt_type;
#ifdef CONFIG_FEATURE_FOTA
	atiny_fota_state_e state;
	atiny_update_result_e update_result;
#endif
	unsigned long buf_len = 48;
    char *rsp_cmd = xy_malloc(buf_len);
	memset(rsp_cmd, 0x00, buf_len);

    (void) len;

    (void)atiny_printf("notify:stat:%d\r\n", event);
    switch (event)
    {
        case ATINY_REG_OK: 
            snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 0");
            break;
        case ATINY_REG_FAIL: 
#if !VER_BC95
		 	snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 1");
#endif
            break;
        case ATINY_DEREG:
            //snprintf(rsp_cmd, buf_len, "+QLWEVTIND:1");
            break;
        case ATINY_REG_UPDATED:
			//BC260Y对标：update上报event 2
            snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 2");
            break;
        case ATINY_DATA_SUBSCRIBLE:
            rpt_type = *((atiny_report_type_e*)arg);
            if (rpt_type == APP_DATA)
            {
                snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 3");
            }
            else if (rpt_type == FIRMWARE_UPDATE_STATE)
            {
                snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 8");
                if(cdp_wait_sem == NULL)
                    cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);
                osSemaphoreRelease(cdp_wait_sem); 
            }
            break;
        case ATINY_DATA_UNSUBSCRIBLE:
			//平台下发取消订阅后,不自动去注册,用户需要主动去注册再注册
            //cdp_deregister_flag = 1;
            rpt_type = *((atiny_report_type_e*)arg);
            if (rpt_type == APP_DATA)
            {
				//if(cdp_get_con_send_flag())
				//	g_send_status = 5;//发送CON包，收到重置或从平台主动取消订阅
            }
            else if (rpt_type == FIRMWARE_UPDATE_STATE) //BC260Y对标：503取消订阅上报event 9
            {
                snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 9");
            }
           
		    //释放信号量，防止主线程阻塞异常
		    if(cdp_wait_sem != NULL)
                osSemaphoreRelease(cdp_wait_sem);
            break;
        case ATINY_DATA_RESET:
			//BC260Y对标 平台下发RESET,对比机查询是去注册状态
			cdp_deregister_flag = 1;
            snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 10");
            //释放信号量，防止主线程阻塞异常
		    if(cdp_wait_sem != NULL)
                osSemaphoreRelease(cdp_wait_sem);
            break;
        case ATINY_RECV_UPDATE_PKG_URL_NEEDED:
            snprintf(rsp_cmd, buf_len, "+QLWEVTIND:6");
            break;
        case ANTIY_DOWNLOAD_COMPLETED:
            snprintf(rsp_cmd, buf_len, "+QLWEVTIND:7");
            break;
        case ATINY_RECOVERY_OK:
            snprintf(rsp_cmd, buf_len, "+CLOUD:1");
            break;
		case ATINY_DTLS_SHAKEHAND_SUCCESS:
			snprintf(rsp_cmd, buf_len, "+QDTLSSTAT:0");
			break;
		case ATINY_DTLS_SHAKEHAND_FAILED:
			snprintf(rsp_cmd, buf_len, "+QDTLSSTAT:3");
			break;
		case ATINY_REGISTER_NOTIFY:
			snprintf(rsp_cmd, buf_len, "REGISTERNOTIFY");
			break;
        case ATINY_BC260_RECOVERY: //BC260Y对标
			snprintf(rsp_cmd, buf_len, "+QLWEVTIND: 6");
			break;
#ifdef CONFIG_FEATURE_FOTA
		case ATINY_FOTA_STATE:
			state = *((atiny_fota_state_e*)arg);
			if(state == ATINY_FOTA_IDLE)
			{
				update_result = atiny_fota_manager_get_update_result(atiny_fota_manager_get_instance());
				if(update_result > 1 ) //下载完成不在这里上报，这里只上报错误码
					snprintf(rsp_cmd, buf_len, "+QIND: \"FOTA\",\"DOWNLOAD\",%d",update_result);
				cdp_ota_state_hook(XY_FOTA_UPGRADE_FAIL);
			}
			else if(state == ATINY_FOTA_DOWNLOADING)
			{				
                if(cdp_wait_sem == NULL)
                    cdp_wait_sem = osSemaphoreNew(0xFFFF, 0, NULL);
                osSemaphoreRelease(cdp_wait_sem); 
				snprintf(rsp_cmd, buf_len, "+QIND: \"FOTA\",\"DOWNLOAD START\"");

                cdp_ota_state_hook(XY_FOTA_DOWNLOADING);
			}
			else if(state == ATINY_FOTA_DOWNLOADED)
			{
				snprintf(rsp_cmd, buf_len, "+QIND: \"FOTA\",\"DOWNLOAD\",0");
                cdp_ota_state_hook(XY_FOTA_DOWNLOADED);
			}
			else if(state == ATINY_FOTA_UPDATING)
			{
				snprintf(rsp_cmd, buf_len, "+QIND: \"FOTA\",\"START\"");
                cdp_ota_state_hook(XY_FOTA_UPGRADING);
			}
			break;
#endif			
        default:
            break;
    }
    if (strcmp(rsp_cmd, "") != 0)
    {
    	if(event >= ATINY_REG_OK && event <= ATINY_REG_FAIL)
		{
			if(!g_cdp_session_info->cdp_event_report_enable)
			{
				xy_free(rsp_cmd);
				return;
			}
		}
		send_urc_to_ext(rsp_cmd, strlen(rsp_cmd));
    }

    xy_free(rsp_cmd);
}


#endif
