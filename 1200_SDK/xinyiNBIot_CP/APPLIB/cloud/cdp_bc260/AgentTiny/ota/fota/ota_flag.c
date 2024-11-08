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

#include "xy_utils.h"
#if TELECOM_VER

#include <stddef.h>
#include <string.h>
#include "atiny_fota_state.h"
#include "xy_flash.h"
#include "net_app_resume.h"
#include "ota_flag.h"
#include "liblwm2m.h"

#define FLASH_UNIT_SIZE (256)
#define FLASH_FLAG_SIZE (512)

static upgrade_flag_s g_flag;
cdp_fota_info_t* g_cdp_fota_info = NULL;  //fota data info

int ota_flag_init()
{
	if(g_cdp_fota_info == NULL)
	{
		g_cdp_fota_info = cloud_malloc(CDP_FOTA_NVM_FILE_NAME);
	    if(cloud_read_file(CDP_FOTA_NVM_FILE_NAME, (void *)g_cdp_fota_info, sizeof(cdp_fota_info_t)) != XY_OK)
    	{
			memset(g_cdp_fota_info, 0x00, sizeof(cdp_fota_info_t));
    	}
	}

    flag_upgrade_init();

    return 0;
}

int ota_flag_destroy()
{
    if(g_cdp_fota_info != NULL && !Is_OpenCpu_Ver())
    {
        xy_free(g_cdp_fota_info);
        g_cdp_fota_info = NULL;
    }

    memset(&g_flag, 0x00, sizeof(g_flag));

    return 0;
}

int ota_flag_read(flag_type_e flag_type, void *buf, int32_t len)
{
	//Read flash data 
    switch (flag_type)
    {
    case FLAG_BOOTLOADER:
        memcpy(buf, &(g_cdp_fota_info->fota_upgrade_info), len);
        break;
    case FLAG_APP:
        memcpy(buf, &(g_cdp_fota_info->fota_observer_info), len);
        break;
    default:
        return -1;
    }

    return 0;
}

int ota_flag_write(flag_type_e flag_type, const void *buf, int32_t len, int code)
{
	//Copy data to flash
	if(flag_type == FLAG_BOOTLOADER)
	{
		 memcpy(&(g_cdp_fota_info->fota_upgrade_info), buf, len);
	}
	else if(flag_type == FLAG_APP)
	{
		memcpy(&(g_cdp_fota_info->fota_observer_info), buf, len);
	}
	else
	{
		//DFOTA 受控模式下保存fota状态信息
		if(code != XY_RECV_UPDATE_PKG_URL_NEEDED && code != XY_DOWNLOAD_COMPLETED)
			return ATINY_ERR;
		
		char *pkt_uri = atiny_fota_manager_get_pkg_uri(atiny_fota_manager_get_instance());
		int fota_state = atiny_fota_manager_get_state(atiny_fota_manager_get_instance());

		//当uri获取失败或者长度溢出, 则不写入
		if(pkt_uri != NULL && (strlen(pkt_uri) <= 255 && strlen(pkt_uri) > 0))
		{
			memcpy(g_cdp_fota_info->uri, pkt_uri, strlen(pkt_uri));
			g_cdp_fota_info->state = fota_state;
		}
		else
			return ATINY_ERR;
	}

    if (cloud_save_file(CDP_FOTA_NVM_FILE_NAME, (void *)g_cdp_fota_info, sizeof(cdp_fota_info_t)) != XY_OK)
        return ATINY_ERR;

    return ATINY_OK;
}

static int save_flag(void)
{
    g_flag.crc_flag = xy_chksum(&g_flag, sizeof(upgrade_flag_s) - sizeof(uint32_t));

    return ota_flag_write(FLAG_BOOTLOADER, &g_flag, sizeof(upgrade_flag_s), -1);
}

int flag_upgrade_init(void)
{
    int ret;
    uint32_t crc;

    ret = ota_flag_read(FLAG_BOOTLOADER, &g_flag, sizeof(upgrade_flag_s));
    if (ret != 0) 
        return ret;

    crc = xy_chksum(&g_flag, sizeof(upgrade_flag_s) - sizeof(uint32_t));
    if (crc != g_flag.crc_flag)
    {
        g_flag.upgrade_state = OTA_IDLE;
        g_flag.upgrade_type = UPGRADE_NONE;
        return save_flag();
    }

    return 0;
}

int flag_set_info(upgrade_type_e upgrade_type, upgrade_state_e *upgrade_state)
{
    g_flag.upgrade_type = upgrade_type;
    g_flag.upgrade_state = OTA_NEED_UPGRADE;

    return save_flag();
}

void flag_get_info(upgrade_type_e *upgrade_type, upgrade_state_e *upgrade_state)
{
    if (NULL != upgrade_type)
        *upgrade_type = g_flag.upgrade_type;

    if (NULL != upgrade_state)
        *upgrade_state = g_flag.upgrade_state;
}

int flag_upgrade_set_result(upgrade_state_e state)
{
    g_flag.upgrade_state = state;

    return save_flag();
}

int flag_upgrade_get_result(upgrade_state_e *state)
{
    if (NULL != state)
        *state = g_flag.upgrade_state;

    g_flag.upgrade_state = OTA_IDLE;
    g_flag.upgrade_type = UPGRADE_NONE;

    return ATINY_OK;
}
#endif
