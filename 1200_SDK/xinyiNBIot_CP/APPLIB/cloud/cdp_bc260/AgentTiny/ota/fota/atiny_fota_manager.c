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
#include "cdp_backup.h"
#include "atiny_fota_manager.h"
#include "atiny_fota_state.h"
#include <string.h>
#include "firmware_update.h"

#define PULL_ONLY 0

struct atiny_fota_manager_tag_s
{
    char *pkg_uri;                  //差分包路径
    atiny_fota_state_e state;       //升级状态
    atiny_fota_state_e rpt_state;   //上报的升级状态
    atiny_update_result_e update_result;    //升级结果
    atiny_fota_idle_state_s idle_state;     //IDLE态的回调函数
    atiny_fota_downloading_state_s downloading_state;   //DOWNLOADING态的回调函数
    atiny_fota_downloaded_state_s downloaded_state;     //DOWNLOADED态的回调函数
    atiny_fota_updating_state_s updating_state;         //UPDATING态的回调函数
    atiny_fota_state_s *current;        //当前的回调函数
    lwm2m_context_t *lwm2m_context;     
    uint32_t cookie;
    bool wait_ack_flag;                 //上报状态等待回复标志位  
    bool init_flag;                     //全局初始化标志
} ;

atiny_fota_manager_s *g_fota_manager = NULL;

char *atiny_fota_manager_get_pkg_uri(const atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return NULL);
    return thi->pkg_uri;
}

int atiny_fota_manager_set_pkg_uri(atiny_fota_manager_s *thi, char *uri, uint32_t len)
{
    ASSERT_THIS(return ATINY_ERR);

    if(thi->pkg_uri)
    {
        atiny_free(thi->pkg_uri);
        thi->pkg_uri = NULL;
    }

    if(uri == NULL)
        return ATINY_OK;

    thi->pkg_uri = atiny_malloc(len + 1);
    if(NULL == thi->pkg_uri)
    {
        ATINY_LOG(LOG_ERR, "lwm2m_strdup fail");
        return ATINY_ERR;
    }
    memcpy(thi->pkg_uri, uri, len);
    thi->pkg_uri[len] = '\0';

    return ATINY_OK;
}

int atiny_fota_manager_get_state(const atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_FOTA_IDLE);
    return thi->state;
}

int atiny_fota_manager_get_rpt_state(const atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_FOTA_IDLE);
    return thi->rpt_state;
}

int atiny_fota_manager_get_update_result(const atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_FIRMWARE_UPDATE_NULL);
    return thi->update_result;
}

void atiny_fota_manager_set_update_result(atiny_fota_manager_s *thi, atiny_update_result_e result)
{
    ASSERT_THIS(return);
    thi->update_result = result;
}

int atiny_fota_manager_get_deliver_method(const atiny_fota_manager_s *thi)
{
    return PULL_ONLY;
}

int atiny_fota_manager_start_download(atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);
    if(thi->state != thi->rpt_state)
    {
        ATINY_LOG(LOG_ERR, "start download busy state %u rpt state %u",
                  thi->state, thi->rpt_state);
        return ATINY_ERR;
    }

    if(NULL == thi->current ||thi->pkg_uri == NULL)
    {
        ATINY_LOG(LOG_ERR, "null pointer");
        return ATINY_ERR;
    }

    ATINY_LOG(LOG_INFO, "start download");
    return thi->current->start_download(thi->current, thi->pkg_uri);
}

int atiny_fota_manager_execute_update(atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(thi->state != thi->rpt_state)
    {
        ATINY_LOG(LOG_ERR, "execute update busy state %u rpt state %u",
                  thi->state, thi->rpt_state);
        return ATINY_ERR;
    }

    if(NULL == thi->current)
    {
        ATINY_LOG(LOG_ERR, "current null pointer");
        return ATINY_ERR;
    }

    ATINY_LOG(LOG_INFO, "excecute update");
    return thi->current->execute_update(thi->current);
}

int atiny_fota_manager_finish_download(atiny_fota_manager_s *thi, int result)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(thi->state != thi->rpt_state)
    {
        ATINY_LOG(LOG_ERR, "finish download busy state %u rpt state %u",
                  thi->state, thi->rpt_state);
        return ATINY_ERR;
    }

    if(NULL == thi->current)
    {
        ATINY_LOG(LOG_ERR, "current null pointer");
        return ATINY_ERR;
    }

    ATINY_LOG(LOG_INFO, "finish download result %d", result);
    return thi->current->finish_download(thi->current, result);
}

int atiny_fota_manager_repot_result(atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(NULL == thi->current)
    {
        ATINY_LOG(LOG_ERR, "current null pointer");
        return ATINY_ERR;
    }

    return thi->current->repot_result(thi->current);
}

//深睡唤醒后恢复FOTA状态
int atiny_fota_manager_set_state_2(atiny_fota_manager_s *thi, atiny_fota_state_e state)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(state > ATINY_FOTA_UPDATING)
    {
        ATINY_LOG(LOG_ERR, "invalid download state %d", state);
        return ATINY_ARG_INVALID;
    }

    ATINY_LOG(LOG_INFO, "download stat from %d to %d", thi->state, state);
    if(thi->state != state)
    {
        atiny_fota_state_s *states[ATINY_FOTA_UPDATING + 1];
        states[ATINY_FOTA_IDLE] = ATINY_GET_STATE(thi->idle_state);
        states[ATINY_FOTA_DOWNLOADING] = ATINY_GET_STATE(thi->downloading_state);
        states[ATINY_FOTA_DOWNLOADED] = ATINY_GET_STATE(thi->downloaded_state);
        states[ATINY_FOTA_UPDATING] = ATINY_GET_STATE(thi->updating_state);
        thi->current = states[state];
        thi->state = state;
        thi->wait_ack_flag = false;
    }
    thi->rpt_state = state;
    return ATINY_OK;
}

int atiny_fota_manager_set_state(atiny_fota_manager_s *thi, atiny_fota_state_e state)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(state > ATINY_FOTA_UPDATING)
    {
        ATINY_LOG(LOG_ERR, "invalid download state %d", state);
        return ATINY_ARG_INVALID;
    }

    ATINY_LOG(LOG_INFO, "download stat from %d to %d", thi->state, state);
    if(thi->state != state)
    {
        atiny_fota_state_s *states[ATINY_FOTA_UPDATING + 1];
        states[ATINY_FOTA_IDLE] = ATINY_GET_STATE(thi->idle_state);
        states[ATINY_FOTA_DOWNLOADING] = ATINY_GET_STATE(thi->downloading_state);
        states[ATINY_FOTA_DOWNLOADED] = ATINY_GET_STATE(thi->downloaded_state);
        states[ATINY_FOTA_UPDATING] = ATINY_GET_STATE(thi->updating_state);
        thi->current = states[state];
        thi->state = state;
        thi->wait_ack_flag = false;
    }
    thi->rpt_state = state;
    atiny_event_notify(ATINY_FOTA_STATE, (const char *)&thi->state, sizeof(thi->state));
    return ATINY_OK;
}

int atiny_fota_manager_rpt_state(atiny_fota_manager_s *thi, atiny_fota_state_e rpt_state)
{
    lwm2m_uri_t uri;
    const char *uri_str = "/5/0/3";
    ASSERT_THIS(return ATINY_ARG_INVALID);

    atiny_fota_manager_save_rpt_state(thi, rpt_state);
    memset((void *)&uri, 0, sizeof(uri));
    (void)lwm2m_stringToUri(uri_str, strlen(uri_str), &uri);
    lwm2m_resource_value_changed(thi->lwm2m_context, &uri);
    return ATINY_OK;
}

void atiny_fota_manager_save_rpt_state(atiny_fota_manager_s *thi, atiny_fota_state_e rpt_state)
{
    ASSERT_THIS(return);

    ATINY_LOG(LOG_INFO, "rpt download state %d", rpt_state);
    thi->rpt_state = rpt_state;
    thi->wait_ack_flag = true;
    thi->cookie ++;
}

void atiny_fota_manager_update_notify(firmware_update_rst_e rst, void *param)
{
    atiny_fota_manager_s *thi = ( atiny_fota_manager_s *)param;
    (void)atiny_fota_manager_finish_download(thi, rst);
}

void atiny_fota_manager_init(atiny_fota_manager_s *thi)
{
    //初始化fota_manager
    memset(thi, 0, sizeof(*thi));
    atiny_fota_idle_state_init(&thi->idle_state, thi);
    atiny_fota_downloading_state_init(&thi->downloading_state, thi);
    atiny_fota_downloaded_state_init(&thi->downloaded_state, thi);
    atiny_fota_updating_state_init(&thi->updating_state, thi);
    thi->current = ATINY_GET_STATE(thi->idle_state);
    thi->init_flag = true;

    //ota flag结构体初始化
    ota_flag_init();
    //CDP FOTA初始化,包括升级结果、状态上报、和g_cdp_fota_info的初始化
#ifdef CONFIG_FEATURE_FOTA
	cdp_fota_init();
#endif
    //根据上一次保存的flag信息进行FOTA升级结果的上报
    atiny_fota_idle_state_int_report_result(&thi->idle_state);
}

void atiny_fota_manager_destroy(atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return);

    if(thi->pkg_uri)
    {
        atiny_free(thi->pkg_uri);
    }

    memset(thi, 0, sizeof(*thi));

    ota_flag_destroy();
}

int atiny_fota_manager_set_lwm2m_context(atiny_fota_manager_s *thi, lwm2m_context_t  *lwm2m_context)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);
    thi->lwm2m_context = lwm2m_context;
    return ATINY_OK;
}
lwm2m_context_t *atiny_fota_manager_get_lwm2m_context(atiny_fota_manager_s *thi)
{
    ASSERT_THIS(return NULL);
    return thi->lwm2m_context;
}

static int atiny_fota_manager_rcv_notify_ack(atiny_fota_manager_s *thi, data_send_status_e status)
{
    ASSERT_THIS(return ATINY_ARG_INVALID);

    if(NULL == thi->current)
    {
        ATINY_LOG(LOG_ERR, "current null pointer");
        return ATINY_ERR;
    }

    return thi->current->recv_notify_ack(thi->current, status);
}

static void atiny_fota_manager_notify_ack_callback(atiny_report_type_e type, int cookie, data_send_status_e status)
{
    ATINY_LOG(LOG_INFO, "download state ack type %d rev cookie %u expect cookie %u status %d, rpt stat %d", type,  (uint32_t)cookie, atiny_fota_manager_get_instance()->cookie,  status,
              atiny_fota_manager_get_instance()->rpt_state);
    if((atiny_fota_manager_get_instance()->wait_ack_flag) && atiny_fota_manager_get_instance()->cookie == (uint32_t)cookie)
    {
        (void)atiny_fota_manager_rcv_notify_ack(atiny_fota_manager_get_instance(), status);
        atiny_fota_manager_get_instance()->wait_ack_flag = false;
    }

}

void atiny_fota_manager_get_data_cfg(const atiny_fota_manager_s *thi, lwm2m_data_cfg_t *data_cfg)
{
    ASSERT_THIS(return);
    if(NULL == data_cfg)
    {
        ATINY_LOG(LOG_ERR, "current null pointer");
        return;
    }

    data_cfg->callback = (lwm2m_data_process)atiny_fota_manager_notify_ack_callback;
    data_cfg->cookie = thi->cookie;
    data_cfg->type = FIRMWARE_UPDATE_STATE;
}

atiny_fota_manager_s *atiny_fota_manager_get_instance(void)
{
    if(g_fota_manager == NULL)
    {
        g_fota_manager = xy_malloc(sizeof(atiny_fota_manager_s));
        memset(g_fota_manager, 0x00, sizeof(atiny_fota_manager_s));
    }
    
    atiny_fota_manager_s *manager = g_fota_manager;
    if(manager->init_flag)
    {
        return manager;
    }

    atiny_fota_manager_init(manager);
    return manager;
}

#endif

