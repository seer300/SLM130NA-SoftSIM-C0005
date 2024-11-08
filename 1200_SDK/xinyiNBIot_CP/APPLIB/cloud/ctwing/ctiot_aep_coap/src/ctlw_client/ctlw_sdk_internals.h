/****************************************************************************

            (c) Copyright 2019 by 天翼物联科技有限公司. All rights reserved.

****************************************************************************/

#ifndef _CTLW_SDK_INTERNALS_H
#define _CTLW_SDK_INTERNALS_H

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_internals.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct ctiot_instance_ptr_t
{
    lwm2m_list_t * instanceP;
	struct ctiot_instance_ptr_t * next;
} CTIOT_INSTANCE_PTR_NODE;
#ifdef CTLW_APP_FUNCTION
	typedef ctlw_lwm2m_object_t* (*ADDOBJFUNC)(void);
#endif
    void ctiot_sdk_recover(ctiot_context_t *pContext);
    system_boot_reason_e ctiot_get_start_reason(void);
    uint16_t ctiot_get_random(void);
    void ctiot_update_voted(uint8_t status);
    uint8_t ctiot_get_vote_status(void);
    int ctiot_coap_extend_query_len(void);
    char *ctiot_coap_extend_query(int querylen);
	uint16_t ctiot_close_socket(void);
	void ctiot_exit_on_error(ctiot_context_t* pContext);
	void ctiot_eps_event_async(void);
	void ctiot_ip_event_async(void);
	void ctiot_detach_event_async(void);
	void ctiot_vote_recv_send_busy(void);
	int ctiot_network_restore(bool recoverConnect, int addressFamily);
	bool ctiot_update_session_status( ctiot_session_status_e* outStatus,ctiot_session_status_e curStatus, ctiot_session_status_e destStatus);
	void ctiot_notify_nb_info(CTIOT_NB_ERRORS errInfo, ctiot_at_to_mcu_type_e infoType, void *params, uint16_t paramLen);
	int ctiot_location_path_validation(char *location);
	bool ctiot_change_client_status(ctiot_client_status_e srcStatus,ctiot_client_status_e destStatus);
#ifdef CTLW_APP_FUNCTION
	int ctlw_add_user_object(ADDOBJFUNC addfunc);
#endif
	int ctiot_uri_to_string(lwm2m_uri_t * uriP, uint8_t * buffer, size_t bufferLen, uri_depth_t * depthP);
	bool ctiot_compare_uri(lwm2m_uri_t *uri1, lwm2m_uri_t *uri2, uint8_t type);
	int16_t ctiot_lwm2m_obj_notify_data_serialize(char* uri, uint16_t size, lwm2m_data_t * dataP, ctiot_send_format_e sendFormat, char** buff);
	uint8_t ctiot_trace_ip_by_bindmode(void);

	//发送接收线程投票锁
	int32_t ctiot_vote_slp_mutex_init(void);
	int32_t ctiot_vote_slp_mutex_lock(void);
	int32_t ctiot_vote_slp_mutex_unlock(void);
	int32_t ctiot_vote_slp_mutex_destroy(void);

	//处理芯片事件锁
	int32_t ctiot_signal_mutex_init(void);
	int32_t ctiot_signal_mutex_lock(void);
	int32_t ctiot_signal_mutex_unlock(void);
	int32_t ctiot_signal_mutex_destroy(void);

	//会话状态修改锁
	int32_t ctiot_session_status_mutex_init(void);
	int32_t ctiot_session_status_mutex_lock(void);
	int32_t ctiot_session_status_mutex_unlock(void);
	int32_t ctiot_session_status_mutex_destroy(void);

	//observe锁
	int32_t ctiot_observe_mutex_init(void);
	int32_t ctiot_observe_mutex_lock(void);
	int32_t ctiot_observe_mutex_unlock(void);
	int32_t ctiot_observe_mutex_destroy(void);

	//获取msgId操作锁
	int32_t ctiot_get_msg_id_mutex_init(void);
	int32_t ctiot_get_msg_id_mutex_lock(void);
	int32_t ctiot_get_msg_id_mutex_unlock(void);
	int32_t ctiot_get_msg_id_mutex_destroy(void);
#if CTIOT_TIMER_AUTO_UPDATE == 1
	void ctiot_set_auto_update_flag(uint8_t flag);
	uint8_t ctiot_get_auto_update_flag(void);
	void ctiot_start_auto_update_timer(void);
	uint16_t ctiot_auto_update_step(void);
	void timer_callback(uint8_t id);
#endif
#ifdef CTLW_APP_FUNCTION
	void ctiot_add_user_objects(void);
#endif
#ifdef __cplusplus
}
#endif
#endif //_CTLW_LWM2M_INTERNALS_H
