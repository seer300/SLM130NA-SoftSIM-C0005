#if VER_BC95
#include "xy_utils.h"
#include "xy_passthrough.h"
#include "sha256.h"
#include "mqtt_backup_proc.h"
#include "MQTTClient.h"
#include "dtls_interface.h"


mqtt_context_t*  g_mqttContext[MQTT_CONTEXT_NUM_MAX] = {NULL};
osMessageQueueId_t g_mqtt_send_msg_q = NULL;
osThreadId_t g_mqtt_at_thread_handle = NULL;
osThreadId_t g_mqtt_recv_thread_handle = NULL;
extern publish_params_t *g_publish_data;

mqtt_context_t *mqttFindContextBytcpid(int tcpconnectID)
{
	int idx = 0;

    for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
       if (g_mqttContext[idx] != NULL && (g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONFIGED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_OPENED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONNECTED)) {
	    	if (tcpconnectID == g_mqttContext[idx]->tcpconnectID) {
            	return g_mqttContext[idx];
	    	}
       }
    }

    return NULL;
}

int mqttFindfreeContext(void)
{
	int idx = 0;
		
	for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
		if (g_mqttContext[idx] == NULL) {
			return idx;
		}
	}

	return -1;
}

mqtt_context_t *mqttCreateContext(int tcpconnectID, int txBufLen, int rxBufLen, int mode)
{
	int idx = 0;
	int mqttUri_len = 0;

	idx = mqttFindfreeContext();
	if (idx < 0) {
		return NULL;
	}

	g_mqttContext[idx] = xy_malloc(sizeof(mqtt_context_t));
	memset(g_mqttContext[idx], 0, sizeof(mqtt_context_t));
	g_mqttContext[idx]->tcpconnectID = tcpconnectID;
	g_mqttContext[idx]->is_used = mode;
	
	g_mqttContext[idx]->sendbuf = xy_malloc(txBufLen);
	memset(g_mqttContext[idx]->sendbuf, 0, txBufLen);
	g_mqttContext[idx]->sendbuf_size = txBufLen;
	g_mqttContext[idx]->readbuf = xy_malloc(rxBufLen);
	memset(g_mqttContext[idx]->readbuf, 0, rxBufLen);
	g_mqttContext[idx]->readbuf_size = rxBufLen;
	
	g_mqttContext[idx]->ipstack = xy_malloc(sizeof(Network));
	memset(g_mqttContext[idx]->ipstack, 0, sizeof(Network));
	g_mqttContext[idx]->mqtt_client = xy_malloc(sizeof(MQTTClient));
	memset(g_mqttContext[idx]->mqtt_client, 0, sizeof(MQTTClient));
	
	g_mqttContext[idx]->cloud_type = CLOUD_TYPE_DEFAULT;
	g_mqttContext[idx]->state = MQTT_STATE_DEFAULT;

	g_mqttContext[idx]->mqtt_conn_data = xy_malloc(sizeof(MQTTPacket_connectData));
	memset(g_mqttContext[idx]->mqtt_conn_data, 0, sizeof(MQTTPacket_connectData));
	g_mqttContext[idx]->mqtt_conn_data->MQTTVersion = MQTT_PROTOCOL_VERSION_DEFAULT;
	g_mqttContext[idx]->mqtt_conn_data->cleansession = MQTT_SESSION_DEFAULT;
	g_mqttContext[idx]->mqtt_conn_data->willFlag = MQTT_WILLFLAG_DEFAULT;
	g_mqttContext[idx]->mqtt_conn_data->keepAliveInterval = MQTT_KEEPALIVE_DEFAULT;

	g_mqttContext[idx]->timeout_data = xy_malloc(sizeof(mqtt_timeout_param_t));
	g_mqttContext[idx]->timeout_data->pkt_timeout = MQTT_PKT_TIMEOUT_DEFAULT;
	g_mqttContext[idx]->timeout_data->retry_times = MQTT_RETRY_TIMES_DEFAULT;
	g_mqttContext[idx]->timeout_data->timeout_notice = MQTT_TIMEOUT_NOTICE_DEFAULT;

	g_mqttContext[idx]->ssl_enable = MQTT_SSLMODE_DEFAULT;
	g_mqttContext[idx]->seclevel = MQTT_SSLSECLEVEL_DEFAULT;	

	return g_mqttContext[idx];
}

int mqttDeleteContext(mqtt_context_t *pctx)
{
	mqtt_context_t* mqttCurContext = pctx;

//删除客户端时，需要保持配置信息

	if (mqttCurContext->addrinfo_data != NULL) {
		if (mqttCurContext->addrinfo_data->host != NULL) {
			xy_free(mqttCurContext->addrinfo_data->host);
			mqttCurContext->addrinfo_data->host = NULL;
		}
		xy_free(mqttCurContext->addrinfo_data);
		mqttCurContext->addrinfo_data = NULL;
	}

	if (mqttCurContext->mqtt_conn_data->clientID.cstring)
	{
		xy_free(mqttCurContext->mqtt_conn_data->clientID.cstring);
		mqttCurContext->mqtt_conn_data->clientID.cstring = NULL;	
	}

	if (mqttCurContext->mqtt_conn_data->username.cstring)
	{
		xy_free(mqttCurContext->mqtt_conn_data->username.cstring);
		mqttCurContext->mqtt_conn_data->username.cstring = NULL;
	}

	if (mqttCurContext->mqtt_conn_data->password.cstring)
	{
		xy_free(mqttCurContext->mqtt_conn_data->password.cstring);
		mqttCurContext->mqtt_conn_data->password.cstring = NULL;
		
	}
	mqttCurContext->is_used = MQTT_CONTEXT_CONFIGED;
    mqttCurContext->state = MQTT_STATE_DEFAULT;
	
	return XY_OK;
}

int mqttConfigContext(int tcpconnectID, int cfgType, void *cfgData)
{
	int idx = 0;
	mqtt_context_t* mqttCurContext = NULL;

	for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
		if (g_mqttContext[idx] != NULL && (g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONFIGED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_OPENED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONNECTED)) {
			if (tcpconnectID == g_mqttContext[idx]->tcpconnectID) {
				mqttCurContext = g_mqttContext[idx];
			}
		}
	}

	if (mqttCurContext->is_used == MQTT_CONTEXT_OPENED || mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
		return XY_ERR; 
	}
	
	if (mqttCurContext != NULL){
		switch(cfgType) {
			case MQTT_CONFIG_VERSION: {
				mqttCurContext->mqtt_conn_data->MQTTVersion = *(int *)cfgData;	
			}
			break;
			case MQTT_CONFIG_KEEPALIVE: {
				mqttCurContext->mqtt_conn_data->keepAliveInterval = *(int *)cfgData;
			}
			break;
			case MQTT_CONFIG_SESSION: {
				mqttCurContext->mqtt_conn_data->cleansession = *(int *)cfgData;
			}
			break;
			case MQTT_CONFIG_TIMEOUT: {
				mqtt_timeout_param_t *timeout_data = (mqtt_timeout_param_t *)cfgData;
				mqttCurContext->timeout_data->pkt_timeout = timeout_data->pkt_timeout;
				mqttCurContext->timeout_data->retry_times = timeout_data->retry_times;
				mqttCurContext->timeout_data->timeout_notice = timeout_data->timeout_notice;
			}
			break;
			case MQTT_CONFIG_WILL: {
				mqtt_will_param_t *will_param = (mqtt_will_param_t *)cfgData;

				if (mqttCurContext->mqtt_conn_data->will.topicName.cstring != NULL) {
					xy_free(mqttCurContext->mqtt_conn_data->will.topicName.cstring);
					mqttCurContext->mqtt_conn_data->will.topicName.cstring = NULL;
				}
 
				if (will_param->will.topicName.cstring != NULL) {
					int topicName_len = strlen(will_param->will.topicName.cstring);
					mqttCurContext->mqtt_conn_data->will.topicName.cstring = xy_malloc(topicName_len + 1);
					memset(mqttCurContext->mqtt_conn_data->will.topicName.cstring, 0, topicName_len + 1);
					memcpy(mqttCurContext->mqtt_conn_data->will.topicName.cstring, will_param->will.topicName.cstring, topicName_len);
				}

				if (mqttCurContext->mqtt_conn_data->will.message.cstring != NULL) {
					xy_free(mqttCurContext->mqtt_conn_data->will.message.cstring);
					mqttCurContext->mqtt_conn_data->will.message.cstring = NULL;
				}

				if (will_param->will.message.cstring != NULL) {
					int message_len = strlen(will_param->will.message.cstring);  
					mqttCurContext->mqtt_conn_data->will.message.cstring = xy_malloc(message_len + 1);
					memset(mqttCurContext->mqtt_conn_data->will.message.cstring, 0, message_len + 1);
					memcpy(mqttCurContext->mqtt_conn_data->will.message.cstring, will_param->will.message.cstring, message_len);
				}

				mqttCurContext->mqtt_conn_data->willFlag = will_param->willFlag;
				mqttCurContext->mqtt_conn_data->will.qos = will_param->will.qos;
				mqttCurContext->mqtt_conn_data->will.retained = will_param->will.retained;	
			}
			break;
			case MQTT_CONFIG_ALIAUTH: {
				aliauth_t* aliauth_data = (aliauth_t *)cfgData;

				int product_key_len = strlen(aliauth_data->product_key);
				int device_name_len = strlen(aliauth_data->device_name);
				int device_secret_len = strlen(aliauth_data->device_secret);

				mqttCurContext->cloud_type = CLOUD_TYPE_ALI;

				if (mqttCurContext->aliauth_data != NULL) {
					if (mqttCurContext->aliauth_data->product_key != NULL) {
						xy_free(mqttCurContext->aliauth_data->product_key);
						mqttCurContext->aliauth_data->product_key = NULL;
					}
					if (mqttCurContext->aliauth_data->device_name != NULL) {
						xy_free(mqttCurContext->aliauth_data->device_name);
						mqttCurContext->aliauth_data->device_name = NULL;
					}
					if (mqttCurContext->aliauth_data->device_secret != NULL) {
						xy_free(mqttCurContext->aliauth_data->device_secret);
						mqttCurContext->aliauth_data->device_secret = NULL;
					}
				}

				if (mqttCurContext->aliauth_data == NULL)
				{
					mqttCurContext->aliauth_data = xy_malloc(sizeof(aliauth_t));
				}
				
				mqttCurContext->aliauth_data->product_key = xy_malloc(product_key_len + 1);
				mqttCurContext->aliauth_data->device_name = xy_malloc(device_name_len + 1);
				mqttCurContext->aliauth_data->device_secret = xy_malloc(device_secret_len + 1);

				memset(mqttCurContext->aliauth_data->product_key, 0, product_key_len + 1);
				memset(mqttCurContext->aliauth_data->device_name, 0, device_name_len + 1);
				memset(mqttCurContext->aliauth_data->device_secret, 0, device_secret_len + 1);
				
				memcpy(mqttCurContext->aliauth_data->product_key, aliauth_data->product_key, product_key_len);
				memcpy(mqttCurContext->aliauth_data->device_name, aliauth_data->device_name, device_name_len);
				memcpy(mqttCurContext->aliauth_data->device_secret, aliauth_data->device_secret, device_secret_len);
			}
			break;
			case MQTT_CONFIG_OPEN: {
				mqtt_addrinfo_param_t* addrinfo_data = (mqtt_addrinfo_param_t *)cfgData;

				int host_len = strlen(addrinfo_data->host);

				if (mqttCurContext->addrinfo_data == NULL)
				{
					mqttCurContext->addrinfo_data = xy_malloc(sizeof(mqtt_addrinfo_param_t));
				}
								
				mqttCurContext->addrinfo_data->host = xy_malloc(host_len + 1);
				memset(mqttCurContext->addrinfo_data->host, 0, host_len + 1);
				memcpy(mqttCurContext->addrinfo_data->host, addrinfo_data->host, host_len);;
				mqttCurContext->addrinfo_data->port = addrinfo_data->port;
			}
			break;
			case MQTT_CONFIG_SSLMODE: {
				mqttCurContext->ssl_enable = *(int *)cfgData;
			}
			break;
			case MQTT_CONFIG_SSLSECLEVEL: {
				mqttCurContext->seclevel = *(int *)cfgData;
			}
			break;
			default:{
				return XY_ERR;
			}
		}
	}

    return XY_OK;
}

int mqttOpenClient(mqtt_context_t *pctx)
{
	mqtt_context_t *mqttCurContext = pctx;
	
	NetworkInit(mqttCurContext->ipstack);  

	//SSL加密链接
	if (mqttCurContext->ssl_enable)
	{
		mbedtls_ssl_context *ssl_ctx = NULL;
		dtls_establish_info_s establish_info = {0};
	
		establish_info.udp_or_tcp  = MBEDTLS_NET_PROTO_TCP;
		establish_info.psk_or_cert = VERIFY_WITH_CERT;

		if (mqttCurContext->seclevel != MQTTS_VERIFY_NULL)
		{
			establish_info.v.c.ca_cert = MQTTS_CA_CERT_ADDR_BASE;
			establish_info.v.c.ca_cert_len = strlen(establish_info.v.c.ca_cert) + 1;
		}

		if (mqttCurContext->seclevel == MQTTS_VERIFY_SERVER_CLIENT)
		{
			establish_info.v.c.client_cert = MQTTS_CLIENT_CERT_ADDR_BASE;
			establish_info.v.c.client_cert_len = strlen(establish_info.v.c.client_cert) + 1;
			establish_info.v.c.client_pk = MQTTS_CLIENT_KEY_ADDR_BASE;
			establish_info.v.c.client_pk_len = strlen(establish_info.v.c.client_pk) + 1;
		}
		
		//初始化SSL句柄
		if ((ssl_ctx = dtls_ssl_new(&establish_info, MBEDTLS_SSL_IS_CLIENT)) == NULL)
		{
			xy_printf(0, XYAPP, WARN_LOG, "dtls new ssl init failed !!!");
			return FAILURE;
		}

		dtls_shakehand_info_s shakehand_info = {0};
		shakehand_info.client_or_server = MBEDTLS_SSL_IS_CLIENT;
		shakehand_info.psk_or_cert      = VERIFY_WITH_CERT;
		shakehand_info.udp_or_tcp       = MBEDTLS_NET_PROTO_TCP;
		shakehand_info.u.c.host         = mqttCurContext->addrinfo_data->host;
		
		shakehand_info.u.c.port = xy_malloc(8);
		snprintf(shakehand_info.u.c.port, 8, "%d", mqttCurContext->addrinfo_data->port);
		
		//进行SSL握手
		if (dtls_shakehand(ssl_ctx, &shakehand_info) != 0)
		{
			xy_free(shakehand_info.u.c.port);
			xy_printf(0, XYAPP, WARN_LOG, "dtls shakehand failed !!!");
			dtls_ssl_destroy(ssl_ctx);
			return FAILURE;
		}
		
		mqttCurContext->ipstack->ssl_ctx = ssl_ctx;
		mqttCurContext->ipstack->my_socket = ((mbedtls_net_context *)(ssl_ctx->MBEDTLS_PRIVATE(p_bio)))->fd;

		xy_free(shakehand_info.u.c.port);
	}
	//非SSL加密链接
	else
	{
		if (SUCCESS != NetworkConnect(mqttCurContext->ipstack, mqttCurContext->addrinfo_data->host, mqttCurContext->addrinfo_data->port)) {
        	if (mqttCurContext->ipstack != NULL && mqttCurContext->ipstack->my_socket >= 0) {
            	xy_printf(0, XYAPP, WARN_LOG, "create socket %d !!!", mqttCurContext->ipstack->my_socket);
            	close(mqttCurContext->ipstack->my_socket);
        	}
			return FAILURE;
    	}
	}

	MQTTClientInit(mqttCurContext->mqtt_client, mqttCurContext->ipstack, mqttCurContext->timeout_data->pkt_timeout * 1000, mqttCurContext->sendbuf, mqttCurContext->sendbuf_size, mqttCurContext->readbuf, mqttCurContext->readbuf_size);

	return SUCCESS;
}

int mqttCloseClient(mqtt_context_t *pctx)
{
	mqtt_context_t *mqttCurContext = pctx;

	if (mqttCurContext->ssl_enable)
	{
		dtls_ssl_destroy(mqttCurContext->ipstack->ssl_ctx);
		mqttCurContext->ipstack->ssl_ctx = NULL;
		mqttCurContext->ipstack->my_socket = -1;
	}
	else
	{
		shutdown(mqttCurContext->ipstack->my_socket, 2);
    	close(mqttCurContext->ipstack->my_socket);
		mqttCurContext->ipstack->my_socket = -1;
	}

    return SUCCESS;
}

int mqttWaitfor(mqtt_context_t *pctx, int packet_type, Timer *timer)
{
    int rc = FAILURE;

    do
    {
        if (TimerIsExpired(timer)) {
	   		break; 
		}
        rc = mqttCycle(pctx, timer);
    }
    while (rc != packet_type && rc >= 0);

    return rc;
}

int mqttClientConnect(mqtt_context_t *pctx, MQTTConnackData *data)
{
	mqtt_context_t *mqttCurContext = pctx;
	
    Timer timer;
	int len = 0;
    int rc = FAILURE;
	MQTTPacket_connectData* options = (MQTTPacket_connectData *)(mqttCurContext->mqtt_conn_data);
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;

	if (mqttCurContext->mqtt_client->isconnected) /* don't send connect packet again if we are already connected */
		  goto exit;
	
    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    mqttCurContext->mqtt_client->keepAliveInterval = options->keepAliveInterval;
    mqttCurContext->mqtt_client->cleansession = options->cleansession;
  
    if ((len = MQTTSerialize_connect(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, options)) <= 0)
        goto exit;
	
    if ((rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) != SUCCESS) {
		char *rsp = xy_malloc(48);
		snprintf(rsp, 48, "\r\n+QMTSTAT: %d,%d\r\n", mqttCurContext->tcpconnectID, 3);
		send_urc_to_ext(rsp, strlen(rsp));
		xy_free(rsp);
		goto exit; 
	}
       
    // this will be a blocking call, wait for the connack
    if (mqttWaitfor(mqttCurContext, CONNACK, &timer) == CONNACK)
    {
        data->rc = 0;
        data->sessionPresent = 0;
        if (MQTTDeserialize_connack(&data->sessionPresent, &data->rc, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) == 1)
            rc = SUCCESS;
        else
            rc = FAILURE;
    }
    else {
		char *rsp = xy_malloc(48);
		snprintf(rsp, 48, "\r\n+QMTSTAT: %d,%d\r\n", mqttCurContext->tcpconnectID, 4);
		send_urc_to_ext(rsp, strlen(rsp));
		xy_free(rsp);
		rc = FAILURE;
	}         
        
exit:
    if (rc == SUCCESS)
    {
        mqttCurContext->mqtt_client->isconnected = 1;
        mqttCurContext->mqtt_client->ping_outstanding = 0;
    }

    return rc;
}


int mqttClientDisconnect(mqtt_context_t *pctx)
{
	mqtt_context_t *mqttCurContext = pctx;
	
	int rc = FAILURE;
    Timer timer;	 // we might wait for incomplete incoming publishes to complete
    int len = 0;
    int i   = 0;

    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

    len = MQTTSerialize_disconnect(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size);
    if (len > 0) 
        rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer);			// send the disconnect packet

    mqttCurContext->mqtt_client->ping_outstanding = 0;
    mqttCurContext->mqtt_client->isconnected = 0;

    return rc;
}

int mqttClientSubscribe(mqtt_context_t *pctx, int msgId, int maxcount, char *topicFilters[], int requestedQoSs[], int grantedQoS[]) 
{
	mqtt_context_t *mqttCurContext = pctx;
	
	int rc = FAILURE;
    Timer timer;
    int len = 0;
	int idx = 0;
	MQTTString topics[MQTT_MAX_SUBSCRIBE_NUM] = {0};
	int retry_counter = 0;
	
	int retry_times = mqttCurContext->timeout_data->retry_times;
	int timeout_notice = mqttCurContext->timeout_data->timeout_notice;
		
	for (idx = 0; idx < maxcount; idx++) {
    	topics[idx].cstring = (char *)topicFilters[idx];
	}
	
	if (!mqttCurContext->mqtt_client->isconnected)
		goto exit;

RETRY_AGAIN:
    TimerInit(&timer);
    TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

    len = MQTTSerialize_subscribe(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, 0, msgId, maxcount, topics, requestedQoSs);
    if (len <= 0)
        goto exit;
	
	if ((rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) != SUCCESS) // send the subscribe packet
    	goto exit;
    
   	if (mqttWaitfor(mqttCurContext, SUBACK, &timer) == SUBACK)      // wait for suback
   	{
    	int count = 0;
    	unsigned short mypacketid;
    	if (MQTTDeserialize_suback(&mypacketid, maxcount, &count, grantedQoS, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) == 1)
    	{
			rc = SUCCESS;
			goto exit;
    	}
    }
	else {
		if (!retry_times)
		{
			rc = FAILURE;
			goto exit;
		}
			
		retry_counter++;
					
		if (retry_counter <= retry_times)
		{
			if (timeout_notice)
			{
				char *rsp = xy_malloc(48);
				snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d,%d\r\n", mqttCurContext->tcpconnectID, msgId, 1, retry_counter);
				send_urc_to_ext(rsp, strlen(rsp));
				xy_free(rsp);
			}
			goto RETRY_AGAIN;
		}
		else 
		{
			rc = FAILURE;
		}
	}
exit:
    return rc;
}
  
int mqttClientUnSubscribe(mqtt_context_t *pctx, int msgId, int maxcount, char *topicFilters[])
{
	mqtt_context_t* mqttCurContext = pctx;
	
	int rc = FAILURE;
	Timer timer;
	int len = 0;
	int idx = 0;
	MQTTString topics[MQTT_MAX_SUBSCRIBE_NUM] = {0};
	int retry_counter = 0;
	
	int retry_times = mqttCurContext->timeout_data->retry_times;
	int timeout_notice = mqttCurContext->timeout_data->timeout_notice;

	for (idx = 0; idx < maxcount; idx++) {
    	topics[idx].cstring = (char *)topicFilters[idx];
	}
	
	if (!mqttCurContext->mqtt_client->isconnected)
		 goto exit;

RETRY_AGAIN:	       
	TimerInit(&timer);
	TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);
	
	if ((len = MQTTSerialize_unsubscribe(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, 0, msgId, maxcount, topics)) <= 0)
		goto exit;

	if ((rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) != SUCCESS) // send the subscribe packet
		goto exit; 
	
	if (mqttWaitfor(mqttCurContext, UNSUBACK, &timer) == UNSUBACK)
	{
		unsigned short mypacketid;	// should be the same as the packetid above
		if (MQTTDeserialize_unsuback(&mypacketid, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) == 1)
		{
			rc = SUCCESS;
			goto exit;
		}
	}
	else {
		if (!retry_times)
		{
			rc = FAILURE;
			goto exit;
		}

		retry_counter++;
					
		if (retry_counter <= retry_times)
		{
			if (timeout_notice)
			{
				char *rsp = xy_malloc(48);
				snprintf(rsp, 48, "\r\n+QMTUNS: %d,%d,%d,%d\r\n", mqttCurContext->tcpconnectID, msgId, 1, retry_counter);
				send_urc_to_ext(rsp, strlen(rsp));
				xy_free(rsp);
			}
			goto RETRY_AGAIN;
		}
		else
		{
			rc = FAILURE;
		}
	}
	
exit:
	return rc;
}

int mqttClientPublish(mqtt_context_t *pctx, int msgId, int qos, int retained, char *mqttPubTopic, int mqttMessage_len, char *mqttMessage)
{
	mqtt_context_t* mqttCurContext = pctx;

	int rc = FAILURE;
	Timer timer;
	MQTTString topic = MQTTString_initializer;
	topic.cstring = (char *)mqttPubTopic;
	int len = 0;
	int retry_counter = 0;
	
	int retry_times = mqttCurContext->timeout_data->retry_times;
	int timeout_notice = mqttCurContext->timeout_data->timeout_notice;
	
	if (!mqttCurContext->mqtt_client->isconnected) {
		goto exit;
	}
	
RETRY_AGAIN1:		  
	TimerInit(&timer);
	TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

	len = MQTTSerialize_publish(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, 0, qos, retained, msgId,
			  topic, (unsigned char*)mqttMessage, mqttMessage_len);
	if (len <= 0)
	{
		xy_printf(0, XYAPP, WARN_LOG,"len <= 0 error len=%d \n",len);
		goto exit;
	}

	if ((rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) != SUCCESS) {
		goto exit; 
	}
		
	if (qos == QOS1) {
		if (mqttWaitfor(mqttCurContext, PUBACK, &timer) == PUBACK) {
			unsigned short mypacketid;
			unsigned char dup, type;
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) != 1) {
				rc = FAILURE;
				goto exit;
			}
		}
		else {
			if (!retry_times)
			{
				rc = FAILURE;
				goto exit;
			}

			retry_counter++;
						
			if (retry_counter <= retry_times)
			{
				if (timeout_notice)
				{
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTPUB: %d,%d,%d,%d\r\n", mqttCurContext->tcpconnectID, msgId, 1, retry_counter);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				goto RETRY_AGAIN1;
			}
			else
			{
				rc = FAILURE;
			}
		}
	}
	else if (qos == QOS2) {
		unsigned short mypacketid;
		unsigned char dup, type;
	
		if (mqttWaitfor(mqttCurContext, PUBREC, &timer) == PUBREC) {

			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) != 1) {
				rc = FAILURE;
				goto exit;
			}	
		}
		else {
			if (!retry_times)
			{
				rc = FAILURE;
				goto exit;
			}

			retry_counter++;
						
			if (retry_counter <= retry_times)
			{
				if (timeout_notice)
				{
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTPUB: %d,%d,%d,%d\r\n", mqttCurContext->tcpconnectID, msgId, 1, retry_counter);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				goto RETRY_AGAIN1;
			}
			else
			{
				rc = FAILURE;
				retry_counter = 0;
				goto exit; 
			}
		}
		
RETRY_AGAIN2:		
		TimerInit(&timer);
		TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

		len = MQTTSerialize_ack(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, PUBREL, 0, mypacketid);

		if (len <= 0)
		{
			xy_printf(0, XYAPP, WARN_LOG,"len <= 0 error len=%d \n",len);
			rc = FAILURE;
			goto exit;
		}
				
		if ((rc = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) != SUCCESS) {
			rc = FAILURE;
			goto exit; 
		}
		
		if (mqttWaitfor(mqttCurContext, PUBCOMP, &timer) == PUBCOMP) {
			if (MQTTDeserialize_ack(&type, &dup, &mypacketid, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) != 1) {
				rc = FAILURE;
				goto exit;
			}	
		}
		else {
			if (!retry_times)
			{
				rc = FAILURE;
				goto exit;
			}

			retry_counter++;
						
			if (retry_counter <= retry_times)
			{
				if (timeout_notice)
				{
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTPUBREL: %d,%d,%d,%d\r\n", mqttCurContext->tcpconnectID, msgId, 1, retry_counter);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				goto RETRY_AGAIN2;
			}
			else
			{
				rc = FAILURE;
			}
		}
	}
	
exit:
	return rc;
}

int mqttDecodePacket(MQTTClient *c, int *value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = c->ipstack->mqttread(c->ipstack, &i, 1, timeout);
        if (rc != 1)
            goto exit;
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
	
exit:
    return len;
}
    
int mqttReadPacket(MQTTClient* c, Timer* timer)
{
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    int rc = c->ipstack->mqttread(c->ipstack, c->readbuf, 1, TimerLeftMS(timer));
    if (rc != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    mqttDecodePacket(c, &rem_len, TimerLeftMS(timer));
    len += MQTTPacket_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    if (rem_len > (c->readbuf_size - len))
    {
        rc = BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (c->ipstack->mqttread(c->ipstack, c->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) {
        rc = 0;
        goto exit;
    }
   
    header.byte = c->readbuf[0];
        rc = header.bits.type;
    if (c->keepAliveInterval > 0)
        TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet
        
exit:
    return rc;
}

int mqttKeepalive(mqtt_context_t *pctx)
{
	mqtt_context_t *mqttCurContext = pctx;
		
	int rc = SUCCESS;

    if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
        return rc;
    }

    if (mqttCurContext->mqtt_client->keepAliveInterval == 0) {
        return rc;
    }

	//阿里云服务器只有在接收到设备发送上行数据时才会更新保活定时器
    if (TimerIsExpired(&mqttCurContext->mqtt_client->last_sent) /*&& TimerIsExpired(&mqttCurContext->mqtt_client->last_received)*/) {
        if (mqttCurContext->mqtt_client->ping_outstanding) {
            xy_printf(0,XYAPP, WARN_LOG, "[MQTT]mqtt keep alive ping resp timeout");
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        }
        else {			
				xy_printf(0,XYAPP, WARN_LOG, "[MQTT]mqtt keep alive send ...");
				mqtt_req_param_t *data = {0};

				data = xy_malloc(sizeof(mqtt_req_param_t));
				data->req_type = MQTT_KEEPALIVE_REQ;
				data->pContext = (void *)mqttCurContext;
			
				if (g_mqtt_send_msg_q != NULL) {
					osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&data), 0, osWaitForever);
				}
        }
    }

    return rc;
}

int mqttCycle(mqtt_context_t *pctx, Timer *timer)
{
	mqtt_context_t *mqttCurContext = pctx;
	
	int len = 0,
		idx = 0,
        rc = SUCCESS;
	
    int packet_type = mqttReadPacket(mqttCurContext->mqtt_client, timer); /* read the socket, see what work is due */

	if (packet_type > 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[MQTT]mqttReadPacket packet_type: %d", packet_type);
	}
		
    switch (packet_type)
    {
        default:
        case 0:
			break;
		case -1:
        {	 
        	/* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
				xy_printf(0,XYAPP, WARN_LOG, "[MQTT]mqttCycle socket read packet_type: %d, errno : %d", packet_type, errno);
				mqtt_req_param_t *data = {0};

				data = xy_malloc(sizeof(mqtt_req_param_t));
				data->req_type = MQTT_TCP_DISCONN_UNEXPECTED_REQ;
				data->tcpconnectID = mqttCurContext->tcpconnectID;
				data->pContext = (void *)mqttCurContext;
	
				if (g_mqtt_send_msg_q != NULL) {
					osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&data), 0, osWaitForever);
				}

				rc = FAILURE;
			    goto exit;
        }
		break;
        case CONNACK:
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
		case PUBREC:
            break;
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg = {0};
            int intQoS;
			char *topic_rsp = NULL;
			char *publish_rsp = NULL;
            char *payload_rsp = NULL;
			
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) != 1) {
              		rc = FAILURE;
               		goto exit;
			}

			topic_rsp = (char*)xy_malloc2(topicName.lenstring.len + 1);
			if (topic_rsp == NULL)
			{
				rc = FAILURE;
               	goto exit;
			}
			memcpy(topic_rsp,topicName.lenstring.data,topicName.lenstring.len);
			topic_rsp[topicName.lenstring.len]='\0';
			
			payload_rsp = (char*)xy_malloc2(msg.payloadlen + 1);
			if (payload_rsp == NULL)
			{
				xy_free(topic_rsp);
				rc = FAILURE;
               	goto exit;
			}
			memcpy(payload_rsp,msg.payload,msg.payloadlen);
			payload_rsp[msg.payloadlen]='\0';
			publish_rsp = (char*)xy_malloc2(30 + topicName.lenstring.len + msg.payloadlen);
			if (publish_rsp == NULL)
			{
				xy_free(topic_rsp);
				xy_free(payload_rsp);
				rc = FAILURE;
               	goto exit;
			}
			snprintf(publish_rsp, (30 + topicName.lenstring.len + msg.payloadlen), "\r\n+QMTRECV: %d,%d,\"%s\",%s\r\n", mqttCurContext->tcpconnectID, msg.id, topic_rsp, payload_rsp);
			send_urc_to_ext(publish_rsp, strlen(publish_rsp));
			xy_free(topic_rsp);
			xy_free(payload_rsp);
			xy_free(publish_rsp);
		
            msg.qos = (enum QoS)intQoS;
            if (msg.qos != QOS0) {
            	if (msg.qos == QOS1) {
					mqtt_req_param_t *data = {0};

					data = xy_malloc(sizeof(mqtt_req_param_t));
					data->req_type = MQTT_PUB_ACK;
					data->tcpconnectID = mqttCurContext->tcpconnectID;
					data->server_ack_mode = PUBACK;
					data->msg_id = msg.id;
					data->pContext = (void *)mqttCurContext;
	
					if (g_mqtt_send_msg_q != NULL) {
						osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&data), 0, osWaitForever);
					}
				}
				else if (msg.qos == QOS2) {
					mqtt_req_param_t *data = {0};

					data = xy_malloc(sizeof(mqtt_req_param_t));
					data->req_type = MQTT_PUB_REC;
					data->tcpconnectID = mqttCurContext->tcpconnectID;
					data->server_ack_mode = PUBREC;
					data->msg_id = msg.id;
					data->pContext = (void *)mqttCurContext;
	
					if (g_mqtt_send_msg_q != NULL) {
						osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&data), 0, osWaitForever);
					}
				}
            }
        }
		break;
        case PUBREL:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
			Timer timer;
			TimerInit(&timer);
            TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, mqttCurContext->mqtt_client->readbuf, mqttCurContext->mqtt_client->readbuf_size) != 1) {
				rc = FAILURE;
				goto exit;
			}

			mqtt_req_param_t *data = {0};

			data = xy_malloc(sizeof(mqtt_req_param_t));
			data->req_type = MQTT_PUB_COMP;
			data->tcpconnectID = mqttCurContext->tcpconnectID;
			data->server_ack_mode = PUBCOMP;
			data->msg_id = mypacketid;
			data->pContext = (void *)mqttCurContext;
	
			if (g_mqtt_send_msg_q != NULL) {
				osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&data), 0, osWaitForever);
			}
        }
		break;
        case PUBCOMP:
		break;
        case PINGRESP: {
			mqttCurContext->mqtt_client->ping_outstanding = 0;
		}
        break;
    }

	rc = mqttKeepalive(mqttCurContext); 
	if (rc == FAILURE) {
		 xy_printf(0,XYAPP, WARN_LOG,"[MQTT] mqttKeepalive failed, mqttCycle socket read errno : %d", errno);
		 mqtt_req_param_t *data = {0};

		 data = xy_malloc(sizeof(mqtt_req_param_t));
	     data->req_type = MQTT_TCP_DISCONN_UNEXPECTED_REQ;
		 data->tcpconnectID = mqttCurContext->tcpconnectID;
		 data->pContext = (void *)mqttCurContext;
	
		 if (g_mqtt_send_msg_q != NULL) {
			osMessageQueuePut(g_mqtt_send_msg_q, (const void *)(&data), 0, osWaitForever);
		 }
	}
	
exit:
    if (rc == SUCCESS)
        rc = packet_type;
    return rc;
}

void mqttYield(mqtt_context_t *pctx, int timeout_ms)
{
	mqtt_context_t* mqttCurContext = pctx;
	
	int rc = SUCCESS;
    Timer timer;

    TimerInit(&timer);
    TimerCountdownMS(&timer, timeout_ms);

	mqttCycle(mqttCurContext, &timer);
}

void mqttTaskRecvProcess(void *argument)
{
   	int res = -1;
	int idx = -1;
	int hasBusyClient = 0;

	xy_printf(0, XYAPP, WARN_LOG, "[MQTT]downlink pkg rcv thread start");
	while(1)
	{	
		hasBusyClient = 0; 
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			osMutexAcquire(g_mqtt_mutex, osWaitForever);
			if (g_mqttContext[idx] != NULL && (g_mqttContext[idx]->is_used == MQTT_CONTEXT_OPENED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONNECTED)) {	
				hasBusyClient = 1;
				mqttYield(g_mqttContext[idx], MQTT_YIELD_TIMEOUT_MS_DEFAULT); 
			}
			osMutexRelease(g_mqtt_mutex);	
		}

		if((hasBusyClient == 0)) {  
            break;
        }
		osDelay(200);
	}

	xy_printf(0, XYAPP, WARN_LOG, "[MQTT]downlink pkg rcv thread end");
	g_mqtt_recv_thread_handle = NULL;
	osThreadExit();
 }

void mqttTaskSendProcess(void* argument)
{
	int res = -1;
	int idx = -1;
	mqtt_req_param_t *msg = {0};
	MQTTConnackData Connackdata = {0};
	unsigned char grantedQoSs[MQTT_MAX_SUBSCRIBE_NUM] = {0};
	mqtt_context_t *mqttCurContext = NULL;

	xy_printf(0,XYAPP, WARN_LOG, "[MQTT]at parse thread start");
	while (1)
	{	
		osMessageQueueGet(g_mqtt_send_msg_q, (void *)&msg, NULL, osWaitForever);
		
		mqttCurContext = (mqtt_context_t *)msg->pContext;
		
		switch (msg->req_type) {
			case MQTT_OPEN_REQ: {
				mqttCurContext->state |= MQTT_STATE_OPEN;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				res = mqttOpenClient(mqttCurContext);

				if (res == SUCCESS) {
					mqttCurContext->is_used = MQTT_CONTEXT_OPENED;
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTOPEN: %d,%d\r\n", msg->tcpconnectID, 0);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);

					if (g_mqtt_recv_thread_handle == NULL) {
						osThreadAttr_t thread_attr = {0};
						thread_attr.name = "mqtt_recv_thread";
						thread_attr.stack_size = osStackShared;
						thread_attr.priority = osPriorityNormal1;
					    g_mqtt_recv_thread_handle = osThreadNew((osThreadFunc_t)mqttTaskRecvProcess, NULL, &thread_attr);
					}
				}
				else {
					mqttDeleteContext(mqttCurContext);
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTOPEN: %d,%d\r\n", msg->tcpconnectID, -1);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
 				}
				
				osMutexRelease(g_mqtt_mutex);
				mqttCurContext->state &= ~MQTT_STATE_OPEN;
			}
			break;
			case MQTT_CLOSE_REQ: {
				mqttCurContext->state |= MQTT_STATE_CLOSE;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				if (mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
					res = mqttClientDisconnect(mqttCurContext);
					if (res == SUCCESS) {
						res = mqttCloseClient(mqttCurContext);
					}
				}
				else {
					res = mqttCloseClient(mqttCurContext);
				}

				if (res == SUCCESS) {
					mqttDeleteContext(mqttCurContext);
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTCLOSE: %d,%d\r\n", msg->tcpconnectID, 0);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				else {
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTCLOSE: %d,%d\r\n", msg->tcpconnectID, -1);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);  
				}
	
				osMutexRelease(g_mqtt_mutex);
				mqttCurContext->state &= ~MQTT_STATE_CLOSE;
			}
			break;
			case MQTT_CONN_REQ: {
				mqttCurContext->state |= MQTT_STATE_CONNECT;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				res = mqttClientConnect(mqttCurContext, &Connackdata);

				if (res != SUCCESS) {
					mqttCloseClient(mqttCurContext);
					mqttDeleteContext(mqttCurContext);
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTCONN: %d,%d\r\n", msg->tcpconnectID, 2);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				else {
					mqttCurContext->is_used = MQTT_CONTEXT_CONNECTED;
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTCONN: %d,%d,%d\r\n", msg->tcpconnectID, 0, Connackdata.rc);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				
				osMutexRelease(g_mqtt_mutex);
				mqttCurContext->state &= ~MQTT_STATE_CONNECT;
			}
			break;
			case MQTT_DISC_REQ: {
				mqttCurContext->state |= MQTT_STATE_DISCONNECT;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				if (mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
					res = mqttClientDisconnect(mqttCurContext);
					if (res == SUCCESS) {
						res = mqttCloseClient(mqttCurContext);
					}
				}
				else {
					res = mqttCloseClient(mqttCurContext);
				}

				if (res == SUCCESS) {
					mqttDeleteContext(mqttCurContext);
					char* rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTDISC: %d,%d\r\n", msg->tcpconnectID, 0);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				else {
					char* rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTDISC: %d,%d\r\n", msg->tcpconnectID, -1);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				
				osMutexRelease(g_mqtt_mutex);
				mqttCurContext->state &= ~MQTT_STATE_DISCONNECT;
			}
			break;
			case MQTT_SUB_REQ: {
				mqttCurContext->state |= MQTT_STATE_SUBSCRIBE;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				res = mqttClientSubscribe(mqttCurContext, msg->msg_id, msg->count, msg->topicFilters, msg->qos, grantedQoSs);

				osMutexRelease(g_mqtt_mutex);

				if (res != SUCCESS) {
					char* rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 2);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				else {
					char *rsp = xy_malloc(48);
					if (msg->count == 4) {
						snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d,%d,%d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0, grantedQoSs[0], grantedQoSs[1], grantedQoSs[2], grantedQoSs[3]);
					}
					else if (msg->count == 3) {
						snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d,%d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0, grantedQoSs[0], grantedQoSs[1], grantedQoSs[2]);
					}
					else if (msg->count == 2) {
						snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0, grantedQoSs[0], grantedQoSs[1]);
					}
					else {
						snprintf(rsp, 48, "\r\n+QMTSUB: %d,%d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0, grantedQoSs[0]);
					}
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}

				for (idx = 0; idx < msg->count; idx++) {
					if (msg->topicFilters[idx] != NULL) {
						xy_free(msg->topicFilters[idx]);
					}
				}

				mqttCurContext->state &= ~MQTT_STATE_SUBSCRIBE;
			}
			break;
			case MQTT_UNSUB_REQ: {
				mqttCurContext->state |= MQTT_STATE_UNSUBSCRIBE;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				res = mqttClientUnSubscribe(mqttCurContext, msg->msg_id, msg->count, msg->topicFilters);

				osMutexRelease(g_mqtt_mutex);

				if (res != SUCCESS) {
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTUNS: %d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 2);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}
				else {
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTUNS: %d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0);
					send_urc_to_ext(rsp, strlen(rsp));
					xy_free(rsp);
				}

				for (idx = 0; idx < msg->count; idx++) {
					if (msg->topicFilters[idx] != NULL) {
						xy_free(msg->topicFilters[idx]);
					}
				}

				mqttCurContext->state &= ~MQTT_STATE_UNSUBSCRIBE;
			}
			break;
			case MQTT_PUB_REQ: {
				mqttCurContext->state |= MQTT_STATE_PUBLISH;
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				
				res = mqttClientPublish(mqttCurContext, msg->msg_id, msg->qos[0], msg->retained, msg->topicFilters[0], msg->message_len, msg->message);

				osMutexRelease(g_mqtt_mutex);

				if (res != SUCCESS) {
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTPUB: %d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 2);
					send_urc_to_ext(rsp, strlen(rsp));	
					xy_free(rsp);
				}
				else {
					char *rsp = xy_malloc(48);
					snprintf(rsp, 48, "\r\n+QMTPUB: %d,%d,%d\r\n", msg->tcpconnectID, msg->msg_id, 0);
					send_urc_to_ext(rsp, strlen(rsp));	
					xy_free(rsp);
				}

				if (msg->topicFilters[0] != NULL) {
					xy_free(msg->topicFilters[0]);
   				} 
				if (msg->message != NULL) {
					xy_free(msg->message);
				}
				
				mqttCurContext->state &= ~MQTT_STATE_PUBLISH;
			}
			break;
			case MQTT_PUB_REC:
            case MQTT_PUB_REL:
            case MQTT_PUB_COMP:
            case MQTT_PUB_ACK: {
				int len = 0;
			
				if (mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
					Timer timer;
                    TimerInit(&timer);
                    TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);

					len = MQTTSerialize_ack(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size, msg->server_ack_mode, 0, msg->msg_id);            
                    res = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer);
					if (res == FAILURE) {
						xy_printf(0,XYAPP, WARN_LOG,"[MQTT]Publish message failed errno : %d, server_ack_mode : %d", errno, msg->server_ack_mode);
						osMutexAcquire(g_mqtt_mutex, osWaitForever);
						mqttCloseClient(mqttCurContext);
						mqttDeleteContext(mqttCurContext);
						osMutexRelease(g_mqtt_mutex);
						char* rsp = xy_malloc(48);
						snprintf(rsp, 48, "\r\n+QMTSTAT: %d,%d\r\n", msg->tcpconnectID, 1);
						send_urc_to_ext(rsp, strlen(rsp));
						xy_free(rsp);
					}
                }
			}
			break;
			case MQTT_KEEPALIVE_REQ: {
				Timer timer;
            	TimerInit(&timer);
            	TimerCountdownMS(&timer, mqttCurContext->mqtt_client->command_timeout_ms);
				
            	int len = MQTTSerialize_pingreq(mqttCurContext->mqtt_client->buf, mqttCurContext->mqtt_client->buf_size);
            	if (len > 0 && (res = MQTTSendPacket(mqttCurContext->mqtt_client, len, &timer)) == SUCCESS) {
					mqttCurContext->mqtt_client->ping_outstanding = 1;
				}
				else {
						xy_printf(0,XYAPP, WARN_LOG,"[MQTT]Ping message failed errno : %d", errno);
						osMutexAcquire(g_mqtt_mutex, osWaitForever);
						mqttCloseClient(mqttCurContext);
						mqttDeleteContext(mqttCurContext);
						osMutexRelease(g_mqtt_mutex);
						char *rsp = xy_malloc(48);
						snprintf(rsp, 48, "\r\n+QMTSTAT: %d,%d\r\n", msg->tcpconnectID, 2);
						send_urc_to_ext(rsp, strlen(rsp));
						xy_free(rsp);
				}
			}
			break;
			case MQTT_TCP_DISCONN_UNEXPECTED_REQ: {
				osMutexAcquire(g_mqtt_mutex, osWaitForever);
				mqttCloseClient(mqttCurContext);
				mqttDeleteContext(mqttCurContext);
				osMutexRelease(g_mqtt_mutex);
				char *rsp = xy_malloc(48);
				snprintf(rsp, 48, "\r\n+QMTSTAT: %d,%d\r\n", msg->tcpconnectID, 1);
				send_urc_to_ext(rsp, strlen(rsp));
				xy_free(rsp);	
			}
			break;
			default: {

			}
		}

		xy_free(msg);
		
		int hasBusyClient = 0;
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			if (g_mqttContext[idx] != NULL && (g_mqttContext[idx]->is_used == MQTT_CONTEXT_OPENED || g_mqttContext[idx]->is_used == MQTT_CONTEXT_CONNECTED)) {
				hasBusyClient = 1;
			} 
		}

		if (hasBusyClient == 0) {
			break;
		}
	}
	xy_printf(0, XYAPP, WARN_LOG, "[MQTT]at parse thread end");

	//删除消息队列
	osMessageQueueDelete(g_mqtt_send_msg_q);
	g_mqtt_send_msg_q = NULL;
	
	g_mqtt_at_thread_handle = NULL;
	osThreadExit();
}

int mqtt_client_config(int tcpconnectID, int cfgType, void *cfgData)
{
	mqtt_context_t *mqttCurContext = NULL;

    mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
    if (mqttCurContext == NULL) {
       mqttCurContext = mqttCreateContext(tcpconnectID, MQTT_TX_BUF_DEFAULT, MQTT_RX_BUF_DEFAULT, MQTT_CONTEXT_CONFIGED);
    }

    if (mqttCurContext != NULL) {
        return mqttConfigContext(tcpconnectID, cfgType, cfgData);
    }
    else {
        return XY_ERR;
    }
}

int mqtt_client_open(int tcpconnectID, char *mqttUri, int mqttPort)
{
	mqtt_context_t *mqttCurContext = NULL;
	mqtt_addrinfo_param_t addinfo_data = {0};
	mqtt_req_param_t *open_data = {0};
	
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
       mqttCurContext = mqttCreateContext(tcpconnectID, MQTT_TX_BUF_DEFAULT, MQTT_RX_BUF_DEFAULT, MQTT_CONTEXT_CONFIGED);
	   if (mqttCurContext == NULL) {
			return XY_ERR;
	   }
    }

	if (mqttCurContext->state & MQTT_STATE_OPEN) {
		return XY_ERR;
	}
			
	if (mqttCurContext->is_used == MQTT_CONTEXT_OPENED || mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
		return XY_ERR; 
	}

	addinfo_data.host = mqttUri;
	addinfo_data.port = mqttPort;
    mqttConfigContext(tcpconnectID, MQTT_CONFIG_OPEN, (void *)&addinfo_data);

	if (g_mqtt_send_msg_q == NULL) {
		g_mqtt_send_msg_q = osMessageQueueNew(16, sizeof(void *), NULL);
	}
	
	if (g_mqtt_at_thread_handle == NULL) {
		osThreadAttr_t thread_attr = {0};
		thread_attr.name = "mqtt_send_thread";
    	thread_attr.stack_size = osStackShared;
   		thread_attr.priority = osPriorityNormal1;

    	g_mqtt_at_thread_handle = osThreadNew((osThreadFunc_t)mqttTaskSendProcess, NULL, &thread_attr);
	}

	open_data = xy_malloc(sizeof(mqtt_req_param_t));
	open_data->req_type = MQTT_OPEN_REQ;
	open_data->tcpconnectID = tcpconnectID;
	open_data->pContext = (void *)mqttCurContext;
	
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&open_data), 0, osWaitForever);
	}

	return XY_OK;
}

int mqtt_client_close(int tcpconnectID)
{
	mqtt_req_param_t *close_data = {0};
	mqtt_context_t *mqttCurContext = NULL;
	
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) { 
		return XY_ERR;
	}

	if (mqttCurContext->state & MQTT_STATE_CLOSE) {
		return XY_ERR;
	}
		
	if ((mqttCurContext->is_used != MQTT_CONTEXT_OPENED) && (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED)) {
		return XY_ERR;	
	}

	close_data = xy_malloc(sizeof(mqtt_req_param_t));
	close_data->req_type = MQTT_CLOSE_REQ;
	close_data->tcpconnectID = tcpconnectID;
	close_data->pContext = (void *)mqttCurContext;
   	      
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&close_data), 0, osWaitForever);
	}
	
	return XY_OK;
}

int mqtt_client_connect(int tcpconnectID, char *clientId, char *userName, char *passWord)
{
	mqtt_req_param_t *conn_data = {0};
	mqtt_context_t *mqttCurContext = NULL;
	
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
		return XY_ERR;
	}

	if (mqttCurContext->state & MQTT_STATE_CONNECT) {
		return XY_ERR;
	}
		
	if (mqttCurContext->is_used != MQTT_CONTEXT_OPENED || mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) {
		return XY_ERR;	
	}
		
	if ((userName == NULL) && (mqttCurContext->cloud_type == CLOUD_TYPE_ALI)) {
		
        if ((mqttCurContext->aliauth_data != NULL) && (mqttCurContext->aliauth_data->device_name != NULL) && (mqttCurContext->aliauth_data->device_secret != NULL) && (mqttCurContext->aliauth_data->product_key != NULL)) {
			
		 	/* setup clientid */
		 	int aliauth_clientid_len = 2 + strlen(clientId) + strlen("|securemode=3,signmethod=hmacsha256|");
		 	char *aliauth_clientid = xy_malloc(aliauth_clientid_len);
		 	snprintf(aliauth_clientid, aliauth_clientid_len, "%s|securemode=3,signmethod=hmacsha256|", clientId);
		 
		 	/* setup username */
		 	int aliauth_username_len = 2 + strlen(mqttCurContext->aliauth_data->device_name) + strlen(mqttCurContext->aliauth_data->product_key);
		 	char *aliauth_username = xy_malloc(aliauth_username_len);
		 	snprintf(aliauth_username, aliauth_username_len, "%s&%s", mqttCurContext->aliauth_data->device_name, mqttCurContext->aliauth_data->product_key);

		 	/* setup password */
			char *sign = xy_malloc(32);
		 	char *aliauth_password = xy_malloc(65);
			
			int hmac_source_len = 30 + strlen(clientId) + strlen(mqttCurContext->aliauth_data->device_name) + strlen(mqttCurContext->aliauth_data->product_key);
			char *hmac_source = xy_malloc(hmac_source_len); 
			
			snprintf(hmac_source, hmac_source_len, "clientId%sdeviceName%sproductKey%s", clientId, mqttCurContext->aliauth_data->device_name, mqttCurContext->aliauth_data->product_key);
			
			utils_hmac_sha256(hmac_source, strlen(hmac_source), mqttCurContext->aliauth_data->device_secret, strlen(mqttCurContext->aliauth_data->device_secret), sign);
			utils_hex2str(sign, 32, aliauth_password, 0);
			*(aliauth_password + 64) = '\0';
			xy_free(sign);

			xy_printf(0,XYAPP, WARN_LOG, "aliauth_clientid:%s, aliauth_username:%s, aliauth_password:%s", aliauth_clientid, aliauth_username, aliauth_password);
			
			mqttCurContext->mqtt_conn_data->clientID.cstring = aliauth_clientid;
			mqttCurContext->mqtt_conn_data->username.cstring = aliauth_username;
			mqttCurContext->mqtt_conn_data->password.cstring = aliauth_password;
			
		}  
		else {
			return XY_ERR;
		}
	}
	else {
		if (clientId != NULL) {
			int clientIDLen = (strlen(clientId)+1);
			mqttCurContext->mqtt_conn_data->clientID.cstring = xy_malloc(clientIDLen);
			memset(mqttCurContext->mqtt_conn_data->clientID.cstring, 0, clientIDLen);
			memcpy(mqttCurContext->mqtt_conn_data->clientID.cstring, clientId, (clientIDLen-1));
		}
		if (userName != NULL) {
			int userNameLen = (strlen(userName)+1);		
			mqttCurContext->mqtt_conn_data->username.cstring = xy_malloc(userNameLen); 
			memset(mqttCurContext->mqtt_conn_data->username.cstring, 0, userNameLen);
		 	memcpy(mqttCurContext->mqtt_conn_data->username.cstring, userName, (userNameLen-1));  
		}
		if (passWord != NULL) {
			int passWordLen = (strlen(passWord)+1);
			mqttCurContext->mqtt_conn_data->password.cstring = xy_malloc(passWordLen);
			memset(mqttCurContext->mqtt_conn_data->password.cstring, 0, passWordLen);
			memcpy(mqttCurContext->mqtt_conn_data->password.cstring, passWord, (passWordLen-1));
		}
	}

	conn_data = xy_malloc(sizeof(mqtt_req_param_t));
	conn_data->req_type = MQTT_CONN_REQ;
	conn_data->tcpconnectID = tcpconnectID;
	conn_data->pContext = (void *)mqttCurContext;
	
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void *)(&conn_data), 0, osWaitForever);
	}
	
	return XY_OK;
}

int mqtt_client_disconnect(int tcpconnectID)
{
	mqtt_req_param_t *disc_data = {0};
	mqtt_context_t *mqttCurContext = NULL;
	
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
		return XY_ERR;
	}

	if (mqttCurContext->state & MQTT_STATE_DISCONNECT) {
		return XY_ERR;
	}
		
	if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
		return XY_ERR;	
	}

	disc_data = xy_malloc(sizeof(mqtt_req_param_t));
	disc_data->req_type = MQTT_DISC_REQ;
	disc_data->tcpconnectID = tcpconnectID;
	disc_data->pContext = (void *)mqttCurContext;
	
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&disc_data), 0, osWaitForever);
	}

	return XY_OK;
}

int mqtt_client_subscribe(int tcpconnectID, int msgId, int count, char *topicFilters[], int requestedQoSs[])
{
	int idx = 0;
	int SubTopic_len = 0;
	mqtt_req_param_t *sub_data = {0};
	mqtt_context_t *mqttCurContext = NULL;

	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
		return XY_ERR;
	}

	if (mqttCurContext->state & MQTT_STATE_SUBSCRIBE) {
		return XY_ERR;
	}
		
	if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
		return XY_ERR;	
	}

	sub_data = xy_malloc(sizeof(mqtt_req_param_t));
	sub_data->req_type = MQTT_SUB_REQ;
	sub_data->tcpconnectID = tcpconnectID;
	sub_data->msg_id = msgId;
	sub_data->pContext = (void *)mqttCurContext;
	sub_data->count = count;

	for (idx = 0; idx < count; idx++ ) {
		sub_data->qos[idx] = requestedQoSs[idx];	
		SubTopic_len = strlen(topicFilters[idx]);
		sub_data->topicFilters[idx] = xy_malloc(SubTopic_len + 1);
		memset(sub_data->topicFilters[idx], 0, SubTopic_len + 1);
		memcpy(sub_data->topicFilters[idx], topicFilters[idx], SubTopic_len);
	}

	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&sub_data), 0, osWaitForever);
	}

	return XY_OK;
}

int mqtt_client_unsubscribe(int tcpconnectID, int msgId, int count, char *topicFilters[])
{
	int idx = 0;
	int   unSubTopic_len = 0;
	mqtt_req_param_t *unsub_data = {0};
	mqtt_context_t *mqttCurContext = NULL;
	
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
		return XY_ERR;
	}
	
	if (mqttCurContext->state & MQTT_STATE_UNSUBSCRIBE) {
		return XY_ERR;
	}
		
	if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
		return XY_ERR;	
	}

	unsub_data = xy_malloc(sizeof(mqtt_req_param_t));
	unsub_data->req_type = MQTT_UNSUB_REQ;
	unsub_data->tcpconnectID = tcpconnectID;
	unsub_data->msg_id = msgId;
	unsub_data->pContext = (void *)mqttCurContext;
	unsub_data->count = count;

	for (idx = 0; idx < count; idx++ ) {
		unSubTopic_len = strlen(topicFilters[idx]);
		unsub_data->topicFilters[idx] = xy_malloc(unSubTopic_len + 1);
		memset(unsub_data->topicFilters[idx], 0, unSubTopic_len + 1);
		memcpy(unsub_data->topicFilters[idx], topicFilters[idx], unSubTopic_len);
	}
	
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&unsub_data), 0, osWaitForever);
	}

	return XY_OK;
}

int mqtt_client_publish(int tcpconnectID, int msgId, int qos, int retained, char *mqttPubTopic, int message_len, char *mqttMessage)
{
	mqtt_req_param_t *pub_data = {0};
	mqtt_context_t *mqttCurContext = NULL;
	int   pubTopic_len = 0;
	char* pubTopic = NULL;
	char* message = NULL;
		
	mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
	if (mqttCurContext == NULL) {
		return XY_ERR;
	}

	if (mqttCurContext->state & MQTT_STATE_PUBLISH) {
		return XY_ERR;
	}
		
	if (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED) {
		return XY_ERR;	
	}

	pub_data = xy_malloc(sizeof(mqtt_req_param_t));
	pub_data->req_type = MQTT_PUB_REQ;
	pub_data->tcpconnectID = tcpconnectID;
	pub_data->msg_id = msgId;
	pub_data->retained = retained;
	pub_data->pContext = (void *)mqttCurContext;
	pub_data->qos[0] = qos;

	pubTopic_len = strlen(mqttPubTopic);
	pubTopic = xy_malloc(pubTopic_len + 1);
	memset(pubTopic, 0, pubTopic_len + 1);
	memcpy(pubTopic, mqttPubTopic, pubTopic_len);
	pub_data->topicFilters[0] = pubTopic;
	pub_data->message_len = message_len;

	message = xy_malloc(message_len + 1);
	memcpy(message, mqttMessage, message_len);
	pub_data->message = message;
	
	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&pub_data), 0, osWaitForever);
	}

	return XY_OK;
}

int mqtt_client_publish_passthr_proc(char* buf, uint32_t len)
{
	int   pubTopic_len = 0;
	char* pubTopic = NULL;
	char* message = NULL;
	mqtt_req_param_t *pub_data = {0};

	pub_data = xy_malloc(sizeof(mqtt_req_param_t));
	pub_data->req_type = MQTT_PUB_REQ;
	pub_data->tcpconnectID = g_publish_data->tcpconnectID;
	pub_data->msg_id = g_publish_data->msgID;
	pub_data->retained = g_publish_data->retain;
	pub_data->pContext = (void *)(g_publish_data->pContext);
	pub_data->qos[0] = g_publish_data->qos;
	pub_data->topicFilters[0] = g_publish_data->topic;

	pub_data->message_len = len;

	message = xy_malloc(len + 1);
	memset(message, 0, len + 1);
	memcpy(message, buf, len);
	pub_data->message = message;

	if (g_mqtt_send_msg_q != NULL) {
		osMessageQueuePut(g_mqtt_send_msg_q, (const void*)(&pub_data), 0, osWaitForever);
	}

	return XY_OK;
}

void mqtt_indefinite_length_passthrough_proc(char *buf, uint32_t data_len)
{
    xy_printf(0, XYAPP, WARN_LOG, "mqtt recv len:%d", data_len);

	passthr_get_unfixedlen_data(buf, data_len);

	if(passthr_rcvd_len > PASSTHR_MQTT_MAX_LEN) 
	{
		mqtt_client_publish_passthr_proc(passthr_rcv_buff, PASSTHR_MQTT_MAX_LEN);
	}
	else if(buf[data_len - 1] == PASSTHR_CTRLZ) /* CtrlZ发送数据 */
	{
		if (passthr_rcvd_len == 1)
		{
			goto END;
		}
		
		mqtt_client_publish_passthr_proc(passthr_rcv_buff, passthr_rcvd_len - 1);
	}
  	else if(buf[data_len - 1] == PASSTHR_ESC)  /* ESC取消发送  */
	{
		goto END;
	}
	else 
	{
		//未收到ctrl+z or esc,继续接收
		return;
	}
END:
	xy_exitPassthroughMode();
}

void mqtt_fixed_length_passthr_proc(char *buf, uint32_t len)
{
	xy_printf(0,XYAPP, WARN_LOG, "mqtt recv len:%d", len);
    if (passthr_fixed_buff_len == 0)
        return;

    if (passthr_rcv_buff == NULL) {
        passthr_rcv_buff = xy_malloc2(passthr_fixed_buff_len);
		if (passthr_rcv_buff == NULL)
		{
			g_publish_data->result = XY_ERR;
			xy_exitPassthroughMode();
		}
    }

    if ((passthr_rcvd_len + len) < passthr_fixed_buff_len) {
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, len);
        passthr_rcvd_len += len;
    }
	else {
		if (passthr_rcvd_len < passthr_fixed_buff_len) { 
			memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, passthr_fixed_buff_len - passthr_rcvd_len);
			passthr_rcvd_len = passthr_fixed_buff_len;
		}

        mqtt_client_publish_passthr_proc(passthr_rcv_buff, passthr_rcvd_len);
        
		xy_exitPassthroughMode();
	}
}

void mqtt_passthr_exit(void)
{
	if (g_publish_data->result == XY_ERR)
	{
		send_rsp_at_to_ext("\r\nERROR\r\n");
	}
	else
	{
		send_rsp_at_to_ext("\r\nOK\r\n");
	}
	
	if (g_publish_data != NULL) {
		xy_free(g_publish_data);
		g_publish_data = NULL;
	}
	
    if (passthr_rcv_buff != NULL) {
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = NULL;
    }
    passthr_rcvd_len = 0;
    passthr_fixed_buff_len = 0;
	passthr_dynmic_buff_len = 0;
}

void mqtt_cakey_passthrough_proc(char *buf, uint32_t len)
{
	xy_printf(0,XYAPP, WARN_LOG, "mqtt recv len:%d", len);
	
    if (passthr_fixed_buff_len == 0)
        return;

    if (passthr_rcv_buff == NULL) {
        passthr_rcv_buff = xy_malloc2(passthr_fixed_buff_len + 1);
		if (passthr_rcv_buff == NULL)
		{
			g_publish_data->result = XY_ERR;
			xy_exitPassthroughMode();
		}
    }

    if ((passthr_rcvd_len + len) < passthr_fixed_buff_len) {
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, len);
        passthr_rcvd_len += len;
    }
	else {
		if (passthr_rcvd_len < passthr_fixed_buff_len) { 
			memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, passthr_fixed_buff_len - passthr_rcvd_len);
			passthr_rcvd_len = passthr_fixed_buff_len;
		}
		*(passthr_rcv_buff + passthr_rcvd_len) = '\0';

		//将透传过来的证书数据写入到flash中
		if (g_publish_data->cert_type == MQTTS_CA_CERT)
		{
        	xy_Flash_Write(MQTTS_CA_CERT_ADDR_BASE, passthr_rcv_buff, passthr_rcvd_len + 1);
		}
		else if (g_publish_data->cert_type == MQTTS_CLIENT_CERT)
		{
			xy_Flash_Write(MQTTS_CLIENT_CERT_ADDR_BASE, passthr_rcv_buff, passthr_rcvd_len + 1);
		}
		else{
			xy_Flash_Write(MQTTS_CLIENT_KEY_ADDR_BASE, passthr_rcv_buff, passthr_rcvd_len + 1);
		}

		xy_exitPassthroughMode();
	}
}

void mqtt_cakey_passthr_exit(void)
{
	if (g_publish_data->result == XY_ERR)
	{
		send_rsp_at_to_ext("\r\nERROR\r\n");
	}
	else
	{
		send_rsp_at_to_ext("\r\nOK\r\n");
	}

	if (g_publish_data != NULL) {
		xy_free(g_publish_data);
		g_publish_data = NULL;
	}

	if (passthr_rcv_buff != NULL) {
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = NULL;
   	}
    passthr_rcvd_len = 0;
	passthr_fixed_buff_len = 0;
}

#endif
