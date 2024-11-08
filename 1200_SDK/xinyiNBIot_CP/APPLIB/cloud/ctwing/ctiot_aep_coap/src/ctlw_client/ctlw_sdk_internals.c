/*******************************************************************************
 *
 * Copyright (c) 2019-2021 天翼物联科技有限公司. All rights reserved.
 *
 *******************************************************************************/

 #include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "ctlw_abstract_os.h"
#include "ctlw_sdk_internals.h"
#ifdef CTLW_APP_DEMO
#include "ctlw_user_objects.h"
#endif

static thread_mutex_t voteSlpMutex; //收发线程投票锁
static thread_mutex_t signalStatusMutex;//IP通知处理互斥锁
static thread_mutex_t sessionStatusMutex; //修改会话状态锁
static thread_mutex_t observeMutex; //lwm2m observe锁
static thread_mutex_t getMsgIdMutex; //获取消息ID互斥锁


int32_t ctiot_vote_slp_mutex_init(void)
{
	return	thread_mutex_init(&voteSlpMutex, "voteSlpMutex");
}
int32_t ctiot_vote_slp_mutex_lock(void)
{
	return thread_mutex_lock(&voteSlpMutex);
}
int32_t ctiot_vote_slp_mutex_unlock(void)
{
	return thread_mutex_unlock(&voteSlpMutex);
}

int32_t ctiot_vote_slp_mutex_destroy(void)
{
	return thread_mutex_destroy(&voteSlpMutex);
}
int32_t ctiot_signal_mutex_init(void)
{
	return	thread_mutex_init(&signalStatusMutex, "signalStatusMutex");
}
int32_t ctiot_signal_mutex_lock(void)
{
	return thread_mutex_lock(&signalStatusMutex);
}
int32_t ctiot_signal_mutex_unlock(void)
{
	return thread_mutex_unlock(&signalStatusMutex);
}
int32_t ctiot_signal_mutex_destroy(void)
{
	return thread_mutex_destroy(&signalStatusMutex);
}
int32_t ctiot_session_status_mutex_init(void)
{
	return thread_mutex_init(&sessionStatusMutex, "sessionStatusMutex");
}
int32_t ctiot_session_status_mutex_lock(void)
{
	return thread_mutex_lock(&sessionStatusMutex);
}
int32_t ctiot_session_status_mutex_unlock(void)
{
	return thread_mutex_unlock(&sessionStatusMutex);
}
int32_t ctiot_session_status_mutex_destroy(void)
{
	return thread_mutex_destroy(&sessionStatusMutex);
}

int32_t ctiot_observe_mutex_init(void)
{
	return thread_mutex_init(&observeMutex,"observeMutex");
}
int32_t ctiot_observe_mutex_lock(void)
{
	return thread_mutex_lock(&observeMutex);
}
int32_t ctiot_observe_mutex_unlock(void)
{
	return thread_mutex_unlock(&observeMutex);
}
int32_t ctiot_observe_mutex_destroy(void)
{
	return thread_mutex_destroy(&observeMutex);
}
int32_t ctiot_get_msg_id_mutex_init(void)
{
	return thread_mutex_init(&getMsgIdMutex, "getMsgIdMutex");
}
int32_t ctiot_get_msg_id_mutex_lock(void)
{
	return thread_mutex_lock(&getMsgIdMutex);
}
int32_t ctiot_get_msg_id_mutex_unlock(void)
{
	return thread_mutex_unlock(&getMsgIdMutex);
}

int32_t ctiot_get_msg_id_mutex_destroy(void)
{
	return thread_mutex_destroy(&getMsgIdMutex);
}
#ifdef CTLW_APP_FUNCTION
void ctiot_add_user_objects(void)
{
	/*add user objects*/
#ifdef CTLW_APP_DEMO
#endif
}
#endif

