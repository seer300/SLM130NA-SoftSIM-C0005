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

#ifndef __AGENT_TINY_DEMO_H_
#define __AGENT_TINY_DEMO_H_

#include "agenttiny.h"
#include "atiny_osdep.h"
#include "xy_utils.h"
#include "liblwm2m.h"
#include "cc.h"

#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/ssl.h"
#include "dtls_interface.h"
#endif

//cdp sequence 发送状态
typedef enum cdp_sequence_send_status
{
	CDP_SEND_STATUS_UNUSED = -1,
	CDP_SEND_STATUS_FAILED,
	CDP_SEND_STATUS_SUCCESS,
	CDP_SEND_STATUS_SENDING,
} cdp_sequence_send_status_e;
	
#define CDP_SEQUENCE_MAX 255

#define DATALIST_MAX_LEN 8

#define CDP_DOWNSTREAM_MAX_LEN 2560  //下行缓存数据总数

#define CDP_RECOVERY_FLAG_EXIST()    (g_softap_var_nv->cdp_recovery_flag == 1)  //判断CDP是否需要恢复

#define CDP_NEED_RECOVERY(status) (status == ATINY_DATA_SUBSCRIBLE || status == 2 || status == 5 || status == 6 || status == 7) //判断当前cdp状态是否是注册态

#define CDP_SET_RECOVERY_FLAG()  \
        {  \
                g_softap_var_nv->cdp_recovery_flag = 1;  \
        }

#define CDP_CLEAR_RECOVERY_FLAG()  \
        {  \
                g_softap_var_nv->cdp_recovery_flag = 0;  \
        }

typedef struct
{
    char ip_addr_str[16];
    unsigned   short port;
} cdp_server_settings_t;

typedef struct buffer_list_s
{
    struct buffer_list_s *next;
    char *data;
    int data_len;
    cdp_msg_type_e type;
	uint8_t seq_num;
} buffer_list_t;

/*下行数据链表结构体*/
typedef struct recvdata_list_s
{
    struct recvdata_list_s *next;
    char   *data;
    int    data_len;
    void   *connP;//对端接收socket连接表述符
} recvdata_list_t;

typedef struct
{
    

    buffer_list_t *head;
    buffer_list_t *tail;
    int pending_num;
    int sent_num;
    int error_num;
} upstream_info_t;

typedef struct
{
    

    buffer_list_t *head;
    buffer_list_t *tail;
    int buffered_num;
    int received_num;
    int dropped_num;
} downstream_info_t;

typedef struct
{

    recvdata_list_t *head;
    recvdata_list_t *tail;
    int pending_num;
} recvdate_info_t;

bool cdp_handle_exist();
int is_cdp_running();
int cdp_create_lwm2m_task(int lifetime);
void cdp_lwm2m_process(void *param);
int cdp_delete_lwm2m_task();
void app_downdata_recv(void);
int get_upstream_message_pending_num();
int get_upstream_message_sent_num();
int get_upstream_message_error_num();
int get_downstream_message_buffered_num();
int get_downstream_message_received_num();
int get_downstream_message_dropped_num();
char *get_cdp_server_ip_addr_str();
int set_cdp_server_ip_addr_str(char *ip_addr_str);
uint16_t get_cdp_server_port();
int set_cdp_server_port(uint16_t port);
int set_cdp_server_settings(char *ip_addr_str, uint16_t port);
void cdp_clear();
int new_message_indication(char *data, int data_len);
int cdp_update_proc(xy_lwm2m_server_t *targetP);
void cdp_stream_clear(uint32_t *mutex, void *stream);
int send_message_via_lwm2m(char *data, int data_len, cdp_msg_type_e type, uint8_t seq_num);
void cdp_upstream_seq_callback(unsigned long eventId, void *param, int paramLen);
void cdp_upstream_ack_callback(atiny_report_type_e type, int cookie, data_send_status_e status, int mid);
void cdp_set_seq_and_rai(uint8_t sec_num, uint8_t raiflag);
void cdp_get_seq_and_rai(uint8_t* raiflag,uint8_t* seq_num);
void cdp_QLWULDATASTATUS_report(uint8_t seq_num);


bool is_cdp_upstream_ok();
void cdp_set_report_event(xy_module_type_t type,const char *arg, int code);
void cdp_con_flag_init(int type, uint16_t seq_num);
void cdp_set_cur_seq_num(uint8_t seq_num);
uint8_t cdp_get_cur_seq_num();
int cdp_get_con_send_flag();
void cdp_con_flag_deint();
void cdp_default_var_init();

//CDP 模块初始化接口
void cdp_module_init();


#if WITH_MBEDTLS_SUPPORT
int atiny_dtls_shakehand(mbedtls_ssl_context *ssl, const dtls_shakehand_info_s *info);
#endif


#endif 
