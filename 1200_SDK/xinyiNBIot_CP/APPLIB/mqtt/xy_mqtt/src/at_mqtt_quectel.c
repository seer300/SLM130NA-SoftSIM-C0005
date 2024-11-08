#if VER_BC95
/**
 * @file at_mqtt_quectel.c
 * @brief 
 * 
 */
#include "xy_utils.h"
#include "xy_at_api.h" 
#include "at_mqtt.h"
#include "mqtt_backup_proc.h"
#include "xy_passthrough.h"
#include "xy_net_api.h"

publish_params_t *g_publish_data = NULL;

int at_QMTCFG_req(char *at_buf, char **prsp_cmd)
{
	int ret = AT_END;
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		char *cfgType = xy_malloc(strlen(at_buf));
		mqtt_context_t *mqttCurContext = NULL;
			
		if (at_parse_param("%s,%d(0-)", at_buf, cfgType, &tcpconnectID) != AT_OK || tcpconnectID >= MQTT_CONTEXT_NUM_MAX){
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		if (at_strcasecmp(cfgType, "version")) {
			int vsn = -1;
			
			if (at_parse_param(",,%d[3-4]", at_buf, &vsn) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (vsn == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"version\",%d", mqttCurContext->mqtt_conn_data->MQTTVersion);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"version\",%d", MQTT_PROTOCOL_VERSION_DEFAULT);
				}
			}
			else {
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_VERSION, (void *)&vsn);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto exit;
				}
			}
		}
		else if (at_strcasecmp(cfgType, "keepalive")) {
			int keepalivetime = -1;

			if (at_parse_param(",,%d[0-3600]", at_buf, &keepalivetime) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (keepalivetime == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"keepalive\",%d", mqttCurContext->mqtt_conn_data->keepAliveInterval);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"keepalive\",%d", MQTT_KEEPALIVE_DEFAULT);
				}
			}
			else {
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_KEEPALIVE, (void *)&keepalivetime);
				if (res != XY_OK) {
					ret = (ATERR_PARAM_INVALID);
					goto exit;
				}
			}
		}
		else if (at_strcasecmp(cfgType, "session")) {
			int clean_session = -1;

			if (at_parse_param(",,%d[0-1]", at_buf, &clean_session) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (clean_session == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"session\",%d", mqttCurContext->mqtt_conn_data->cleansession);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"session\",%d", MQTT_SESSION_DEFAULT);
				}
			}
			else {
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_SESSION, (void *)&clean_session);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto exit;
				}
			}	
		}
		else if (at_strcasecmp(cfgType, "timeout")) {
			int pkt_timeout = -1;
			int retry_times = -1;
		    int timeout_notice = -1;
			mqtt_timeout_param_t timeout_data = {0};

			if (at_parse_param(",,%d[1-60]", at_buf, &pkt_timeout) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (pkt_timeout == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"timeout\",%d,%d,%d", mqttCurContext->timeout_data->pkt_timeout, mqttCurContext->timeout_data->retry_times, mqttCurContext->timeout_data->timeout_notice);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"timeout\",%d,%d,%d", MQTT_PKT_TIMEOUT_DEFAULT, MQTT_RETRY_TIMES_DEFAULT, MQTT_TIMEOUT_NOTICE_DEFAULT);
				}
			}
			else {
				if (at_parse_param(",,,%d(0-10),%d(0-1)", at_buf, &retry_times, &timeout_notice) != AT_OK) {
					ret = (ATERR_PARAM_INVALID);
					goto exit;	
				}
				
				timeout_data.pkt_timeout = pkt_timeout;
				timeout_data.retry_times = retry_times;
				timeout_data.timeout_notice = timeout_notice;
				
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_TIMEOUT, (void *)&timeout_data);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto exit;
				}
			}	
		}
		else if (at_strcasecmp(cfgType, "will")) { 
			int will_fg = -1;
			int will_qos = -1;
		    int will_retain = -1;
			int will_msg_len = -1;
			char *will_topic = xy_malloc(strlen(at_buf));
			char *will_msg = xy_malloc(strlen(at_buf));
			mqtt_will_param_t will_data = {0};

			memset(will_msg, 0, strlen(at_buf));

			if (at_parse_param(",,%d[0-1]", at_buf, &will_fg) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto will_error;	
			}

			if (will_fg == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					if (mqttCurContext->mqtt_conn_data->willFlag == 1) {
						if (mqttCurContext->mqtt_conn_data != NULL && mqttCurContext->mqtt_conn_data->will.message.cstring != NULL) {
							*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->mqtt_conn_data->will.topicName.cstring) + strlen(mqttCurContext->mqtt_conn_data->will.message.cstring));
                			snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->mqtt_conn_data->will.topicName.cstring) + strlen(mqttCurContext->mqtt_conn_data->will.message.cstring), "\"will\",%d,%d,%d,\"%s\",\"%s\"", mqttCurContext->mqtt_conn_data->willFlag, mqttCurContext->mqtt_conn_data->will.qos, mqttCurContext->mqtt_conn_data->will.retained, mqttCurContext->mqtt_conn_data->will.topicName.cstring, mqttCurContext->mqtt_conn_data->will.message.cstring);
						}
						else {
							*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->mqtt_conn_data->will.topicName.cstring));
                			snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->mqtt_conn_data->will.topicName.cstring), "\"will\",%d,%d,%d,\"%s\",\"\"", mqttCurContext->mqtt_conn_data->willFlag, mqttCurContext->mqtt_conn_data->will.qos, mqttCurContext->mqtt_conn_data->will.retained, mqttCurContext->mqtt_conn_data->will.topicName.cstring);
						}
					}
					else {
						*prsp_cmd = xy_malloc(64);
                		snprintf(*prsp_cmd, 64, "\"will\",%d", 0);
					}
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"will\",%d", MQTT_WILLFLAG_DEFAULT);
				}
			}
			else {
				will_data.willFlag = will_fg;
				if (will_fg == 1) {
					if (at_parse_param(",,,%d(0-2),%d(0-1),%256s(),%256s", at_buf, &will_qos, &will_retain, will_topic, will_msg) != AT_OK) {
						ret = (ATERR_PARAM_INVALID);
						goto will_error;	
					}
					
					will_data.will.qos = will_qos;
					will_data.will.retained = will_retain;
					will_data.will.topicName.cstring = will_topic;

					if (strlen(will_msg)) {
						will_data.will.message.cstring = will_msg;
					}
					else{
						will_data.will.message.cstring = NULL;
					}
					  
				}

				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_WILL, (void *)&will_data);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto will_error;
				}
			}	

		will_error:
			if (will_topic != NULL) {
				xy_free(will_topic);
			}
			if (will_msg != NULL) {
				xy_free(will_msg);
			}
		}
		else if (at_strcasecmp(cfgType, "aliauth")) { 
			char* product_key = xy_malloc(strlen(at_buf));
			char* device_name = xy_malloc(strlen(at_buf));
			char* device_secret = xy_malloc(strlen(at_buf));
			aliauth_t aliauth_data = {0};

			memset(product_key, 0, strlen(at_buf));
			memset(device_name, 0, strlen(at_buf));
			memset(device_secret, 0, strlen(at_buf));
			
			if (at_parse_param(",,%s,%s,%s", at_buf, product_key, device_name, device_secret) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto aliauth_error;	
			}

			if (!strlen(product_key)) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					if (mqttCurContext->aliauth_data != NULL && mqttCurContext->aliauth_data->product_key != NULL) {
						*prsp_cmd = xy_malloc(64 + strlen(mqttCurContext->aliauth_data->product_key) + strlen(mqttCurContext->aliauth_data->device_name) + strlen(mqttCurContext->aliauth_data->device_secret));
                		snprintf(*prsp_cmd, 64 + strlen(mqttCurContext->aliauth_data->product_key) + strlen(mqttCurContext->aliauth_data->device_name) + strlen(mqttCurContext->aliauth_data->device_secret), "\"aliauth\",\"%s\",\"%s\",\"%s\"", mqttCurContext->aliauth_data->product_key, mqttCurContext->aliauth_data->device_name, mqttCurContext->aliauth_data->device_secret);
					}
					else {
						*prsp_cmd = xy_malloc(64);
                		snprintf(*prsp_cmd, 64, "\"aliauth\",\"\",\"\",\"\"");
					}
				}
				else {
						*prsp_cmd = xy_malloc(64);  
                		snprintf(*prsp_cmd, 64, "\"aliauth\",\"\",\"\",\"\"");
				}
			}
			else {
					if (strlen(product_key) == 0 || strlen(device_name) == 0 || strlen(device_secret) == 0) {
						ret = (ATERR_PARAM_INVALID);
						goto aliauth_error;

					}
					aliauth_data.product_key = product_key; 
					aliauth_data.device_name = device_name;
					aliauth_data.device_secret = device_secret;
					
					res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_ALIAUTH, (void *)&aliauth_data);
					if (res != XY_OK) {
						ret = (ATERR_NOT_ALLOWED);
						goto aliauth_error;
					}
			}
	
		aliauth_error:
			if (product_key != NULL) {
				xy_free(product_key);
			}
			if (device_name != NULL) {
				xy_free(device_name);
			}
			if (device_secret != NULL) {
				xy_free(device_secret);
			}
		}
		//是否开启SSL加密链接
		else if (at_strcasecmp(cfgType, "ssl")) {
			int ssl_enable = -1;

			if (at_parse_param(",,%d[0-1]", at_buf, &ssl_enable) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (ssl_enable == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"ssl\",%d", mqttCurContext->ssl_enable);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"ssl\",%d", MQTT_SSLMODE_DEFAULT);
				}
			}
			else {
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_SSLMODE, (void *)&ssl_enable);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto exit;
				}
			}	
		}
		//SSL身份认证模式
		else if (at_strcasecmp(cfgType, "seclevel")) {
			int sec_level = -1;

			if (at_parse_param(",,%d[0-2]", at_buf, &sec_level) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			if (sec_level == -1) {
				mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
				if (mqttCurContext != NULL) {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"seclevel\",%d", mqttCurContext->seclevel);
				}
				else {
					*prsp_cmd = xy_malloc(64);
                	snprintf(*prsp_cmd, 64, "\"seclevel\",%d", MQTT_SSLSECLEVEL_DEFAULT);
				}
			}
			else {
				res = mqtt_client_config(tcpconnectID, MQTT_CONFIG_SSLSECLEVEL, (void *)&sec_level);
				if (res != XY_OK) {
					ret = (ATERR_NOT_ALLOWED);
					goto exit;
				}
			}	
		}
		//设置根证书
		else if (at_strcasecmp(cfgType, "sslcacert")) {
			int ca_len = -1;
		
			if (at_parse_param(",,%d(1-4095)", at_buf, &ca_len) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			g_publish_data = xy_malloc(sizeof(publish_params_t));
			g_publish_data->result = XY_OK;
			g_publish_data->cert_type = MQTTS_CA_CERT;
			
			passthr_fixed_buff_len = ca_len;
			xy_enterPassthroughMode((app_passthrough_proc)mqtt_cakey_passthrough_proc, (app_passthrough_exit)mqtt_cakey_passthr_exit);
			
			send_urc_to_ext_NoCache("\r\nCONNECT\r\n",strlen("\r\nCONNECT\r\n"));
			ret = AT_ASYN;
			goto exit;
		}
		//设置客户端证书
		else if (at_strcasecmp(cfgType, "sslclientcert")) {
			int clientcert_len = -1;
		
			if (at_parse_param(",,%d(1-4095)", at_buf, &clientcert_len) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			g_publish_data = xy_malloc(sizeof(publish_params_t));
			g_publish_data->result = XY_OK;
			g_publish_data->cert_type = MQTTS_CLIENT_CERT;
			
			passthr_fixed_buff_len = clientcert_len;
			xy_enterPassthroughMode((app_passthrough_proc)mqtt_cakey_passthrough_proc, (app_passthrough_exit)mqtt_cakey_passthr_exit);
			
			send_urc_to_ext_NoCache("\r\nCONNECT\r\n",strlen("\r\nCONNECT\r\n"));
			ret = AT_ASYN;
			goto exit;
		}
		//设置客户端密钥
		else if (at_strcasecmp(cfgType, "sslclientkey")) {
			int clientkey_len = -1;
		
			if (at_parse_param(",,%d(1-4095)", at_buf, &clientkey_len) != AT_OK) {
				ret = (ATERR_PARAM_INVALID);
				goto exit;	
			}

			g_publish_data = xy_malloc(sizeof(publish_params_t));
			g_publish_data->result = XY_OK;
			g_publish_data->cert_type = MQTTS_CLIENT_KEY;
			
			passthr_fixed_buff_len = clientkey_len;
			xy_enterPassthroughMode((app_passthrough_proc)mqtt_cakey_passthrough_proc, (app_passthrough_exit)mqtt_cakey_passthr_exit);
			
			send_urc_to_ext_NoCache("\r\nCONNECT\r\n",strlen("\r\nCONNECT\r\n"));
			ret = AT_ASYN;
			goto exit;
		}
		else {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}
		
	exit:
		if (cfgType != NULL) {
			xy_free(cfgType);
		}
		return ret;	
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(300);
		snprintf(*prsp_cmd, 300, "\"version\",(0),(3,4)\r\n+QMTCFG: \"keepalive\",(0),(0-3600)\r\n+QMTCFG: \"session\",(0),(0,1)\r\n+QMTCFG: \"timeout\",(0),(1-60),(0-10),(0,1)\r\n+QMTCFG: \"will\",(0),(0,1),(0-2),(0,1),<will_topic>,<will_msg>\r\n+QMTCFG: \"aliauth\",(0),<productkey>,<devicename>,<devicesecret>");
		return AT_END;	
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTOPEN=<tcpconnectID>,"<host_name>",<port>
int at_QMTOPEN_req(char *at_buf, char **prsp_cmd)
{	
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;
		int port = -1;
		char* host_name = xy_malloc(strlen(at_buf));

		if (!xy_tcpip_is_ok()) {
			xy_free(host_name);
        	return  (ATERR_NOT_NET_CONNECT);
        }
		
		if (at_parse_param("%d(0-),%101s(),%d(0-65535)", at_buf, &tcpconnectID, host_name, &port) != AT_OK || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			xy_free(host_name);
			return  (ATERR_PARAM_INVALID);
		}
		
		res = mqtt_client_open(tcpconnectID, host_name, port);
		if (res != XY_OK) {
			xy_free(host_name);
			return  (ATERR_NOT_ALLOWED);
		}

		xy_free(host_name);
		return AT_END;
	}
	else if (g_req_type == AT_CMD_QUERY) {
		int idx = 0;
		*prsp_cmd = xy_malloc(138 * MQTT_CONTEXT_NUM_MAX);
		memset(*prsp_cmd, 0, 138 * MQTT_CONTEXT_NUM_MAX);
	
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			mqtt_context_t *mqttCurContext = NULL;
	  		mqttCurContext = mqttFindContextBytcpid(idx);
	
      		if (mqttCurContext != NULL) {
				if (mqttCurContext->addrinfo_data != NULL && mqttCurContext->addrinfo_data->host != NULL){		
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTOPEN: %d,\"%s\",%d\r\n", mqttCurContext->tcpconnectID, mqttCurContext->addrinfo_data->host, mqttCurContext->addrinfo_data->port);
				}
        	}
		}
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\nOK\r\n");
		return AT_END;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(48);
		snprintf(*prsp_cmd, 48, "(0),<host_name>,<port>");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

// AT+QMTCLOSE=<tcpconnectID>
int at_QMTCLOSE_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;

		if (!xy_tcpip_is_ok()) {
        	return  (ATERR_NOT_NET_CONNECT);
        }
				
		if (at_parse_param("%d(0-)", at_buf, &tcpconnectID) != AT_OK || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			return  (ATERR_PARAM_INVALID);
		}
		
		res = mqtt_client_close(tcpconnectID);
		if (res != XY_OK) {
			return  (ATERR_NOT_ALLOWED);
		}
		
		return AT_END;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "(0)");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTCONN=<tcpconnectID>,"<clientID>"[,"<username>"[,"<password>"]]
int at_QMTCONN_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int ret = AT_END;
		int res = -1;
		int tcpconnectID = -1;
		int clientID_len = 0;
		int username_len = 0;
		int password_len = 0;
		char* clientID = xy_malloc(strlen(at_buf));
		char* username = xy_malloc(strlen(at_buf));
		char* password = xy_malloc(strlen(at_buf));

		memset(clientID, 0, strlen(at_buf));
		memset(username, 0, strlen(at_buf));
		memset(password, 0, strlen(at_buf));

		if (!xy_tcpip_is_ok()) {
        	ret = (ATERR_NOT_NET_CONNECT);
       		 goto exit;
        }
				
		if (at_parse_param("%d(0-),%s,%s,%s", at_buf, &tcpconnectID, clientID, username, password) != AT_OK || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		clientID_len = strlen(clientID);
		username_len = strlen(username);
		password_len = strlen(password);	
		if (clientID_len == 0) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		if (username_len == 0 && password_len != 0){
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}
		
		res = mqtt_client_connect(tcpconnectID, (clientID_len != 0 ? (char *)clientID : NULL), (username_len != 0 ? (char *)username : NULL), (password_len != 0 ? (char *)password : NULL));
		if (res != XY_OK) {
			ret = (ATERR_NOT_ALLOWED);
		}
	exit:
		if (clientID != NULL) {
			xy_free(clientID);
		}
		if (username != NULL) {
			xy_free(username);
		}
		if (password != NULL) {
			xy_free(password);
		}
		return ret;
	}
	else if (g_req_type == AT_CMD_QUERY) {
		int idx = 0;
		*prsp_cmd = xy_malloc(32 * MQTT_CONTEXT_NUM_MAX);
		memset(*prsp_cmd, 0, 32 * MQTT_CONTEXT_NUM_MAX);
		
		for (idx = 0; idx < MQTT_CONTEXT_NUM_MAX; idx++) {
			mqtt_context_t *mqttCurContext = NULL;
	  		mqttCurContext = mqttFindContextBytcpid(idx);

			if (mqttCurContext != NULL) {
	          	if (((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (mqttCurContext->state & MQTT_STATE_CLOSE)) || ((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (mqttCurContext->state & MQTT_STATE_DISCONNECT)))  {
	              	snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,4\r\n", mqttCurContext->tcpconnectID);
	          	}
				else if ((mqttCurContext->is_used == MQTT_CONTEXT_CONNECTED) && (!(mqttCurContext->state & MQTT_STATE_CLOSE)) && (!(mqttCurContext->state & MQTT_STATE_DISCONNECT))) {
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,3\r\n", mqttCurContext->tcpconnectID);
				}
				else if ((mqttCurContext->is_used == MQTT_CONTEXT_OPENED) && (mqttCurContext->state & MQTT_STATE_CONNECT)) {
					snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,2\r\n", mqttCurContext->tcpconnectID);
				}
	          	else if ((mqttCurContext->is_used == MQTT_CONTEXT_OPENED) && (!(mqttCurContext->state & MQTT_STATE_CONNECT))) {
	              	snprintf(*prsp_cmd + strlen(*prsp_cmd), 32 * MQTT_CONTEXT_NUM_MAX, "\r\n+QMTCONN: %d,1\r\n", mqttCurContext->tcpconnectID);
	          	}
	        }
		}
		snprintf(*prsp_cmd + strlen(*prsp_cmd), 138 * MQTT_CONTEXT_NUM_MAX, "\r\nOK\r\n");
		return AT_END;	
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "(0),<clientID>,<username>,<password>");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTDISC=<tcpconnectID>
int at_QMTDISC_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int res = -1;
		int tcpconnectID = -1;

		if (!xy_tcpip_is_ok()) {
        	return  (ATERR_NOT_NET_CONNECT);
        }
				
		if (at_parse_param("%d(0-)", at_buf, &tcpconnectID) != AT_OK || tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			return  (ATERR_PARAM_INVALID);
		}
		
		res = mqtt_client_disconnect(tcpconnectID);
		if (res != XY_OK) {
			return  (ATERR_NOT_ALLOWED);
		}
		return AT_END;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(24);
		snprintf(*prsp_cmd, 24, "(0)");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTSUB=<TCP_connectID>,<msgID>,<topic>,<QoS>[,<topic>,<QoS>...]
int at_QMTSUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int idx = 0;
		int ret = AT_END;
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int count = 0;
		char *topicFilters[4] = {0};
		int requestedQoSs[4] = {-1, -1, -1, -1};

		for (idx = 0; idx < 4; idx++) {
			topicFilters[idx] = xy_malloc(strlen(at_buf));
			memset(topicFilters[idx], 0, strlen(at_buf));
		}
		
		if (!xy_tcpip_is_ok()) {  
        	ret = (ATERR_NOT_NET_CONNECT);
       		 goto exit;
        }
		
		if (at_parse_param("%d(0-),%d(1-65535),%s,%d,%s,%d,%s,%d,%s,%d", at_buf, &tcpconnectID, &msgID, topicFilters[0], &requestedQoSs[0], topicFilters[1], &requestedQoSs[1], topicFilters[2], &requestedQoSs[2], topicFilters[3], &requestedQoSs[3]) != AT_OK) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
  		}

		if (tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		for (idx = 0; idx < 4; idx++) {
			if (strlen(topicFilters[idx]) != 0) {
				if (strlen(topicFilters[idx]) > 255 || requestedQoSs[idx] < 0 || requestedQoSs[idx] > 2) {
					ret = (ATERR_PARAM_INVALID);   
					goto exit;
				}	
				count += 1;
			}
			else {
				break;
			}
		}
		if (count == 0) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}
		
		res = mqtt_client_subscribe(tcpconnectID, msgID, count, topicFilters, requestedQoSs);
		if (res != XY_OK) {
			ret = (ATERR_NOT_ALLOWED);
		}
	exit:
		for (idx = 0; idx < 4; idx++) {
			if (topicFilters[idx] != NULL) {
				xy_free(topicFilters[idx]);
			}
		}
		return ret;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0),(1-65535),<topic>,(0-2)[,<topic>,(0-2)...]");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTUNS=<TCP_connectID>,<msgID>,<topic>[,<topic>...]
int at_QMTUNS_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int idx = 0;
		int ret = AT_END;
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int count = 0;
		char *topicFilters[4] = {0};

		for (idx = 0; idx < 4; idx++) {
			topicFilters[idx] = xy_malloc(strlen(at_buf));
			memset(topicFilters[idx], 0, strlen(at_buf));
		}

		if (!xy_tcpip_is_ok()) {
        	ret =  (ATERR_NOT_NET_CONNECT);
       		 goto exit;
        }
				
		if (at_parse_param("%d(0-),%d(1-65535),%s,%s,%s,%s", at_buf, &tcpconnectID, &msgID, topicFilters[0], topicFilters[1], topicFilters[2], topicFilters[3]) != AT_OK) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		if (tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		for (idx = 0; idx < 4; idx++) {
			if (strlen(topicFilters[idx]) != 0) {
				if (strlen(topicFilters[idx]) > 255) {
					ret = (ATERR_PARAM_INVALID);   
					goto exit;
				}	  
				count += 1; 
			}
			else {
				break;
			}
		}
		if (count == 0) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}
		
		res = mqtt_client_unsubscribe(tcpconnectID, msgID, count, topicFilters);
		if (res != XY_OK) {
			ret = (ATERR_NOT_ALLOWED);
		}
	exit:
		for (idx = 0; idx < 4; idx++) {
			if (topicFilters[idx] != NULL) {
				xy_free(topicFilters[idx]);
			}
		}
		return ret;
	}
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(80);
		snprintf(*prsp_cmd, 80, "(0),(1-65535),<topic>[,<topic>...]");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}

//AT+QMTPUB=<tcpconnectID>,<msgID>,<qos>,<retain>,"<topic>",<len>
int at_QMTPUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ) {
		int ret =AT_END;
		int res = -1;
		int tcpconnectID = -1;
		int msgID = -1;
		int qos = -1;
		int retain = -1;
		int message_len = 0;
		mqtt_context_t *mqttCurContext = NULL;
		char *topic = xy_malloc(strlen(at_buf));
		
		if (!xy_tcpip_is_ok()) {
        	ret = (ATERR_NOT_NET_CONNECT);
       		 goto exit;
        }
				
		if (at_parse_param("%d(0-),%d(0-65535),%d(0-2),%d(0-1),%256s(),%d[0-1024]", at_buf, &tcpconnectID, &msgID, &qos, &retain, topic, &message_len) != AT_OK) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		if (tcpconnectID >= MQTT_CONTEXT_NUM_MAX) {
			ret = (ATERR_PARAM_INVALID);
			goto exit;
   		}

		if (((qos == 0) && (msgID != 0)) || ((qos != 0) && (msgID == 0))) {
  			ret = (ATERR_PARAM_INVALID);
			goto exit;
		}

		mqttCurContext = mqttFindContextBytcpid(tcpconnectID);
		if (mqttCurContext == NULL) {
			ret = (ATERR_NOT_ALLOWED);
			goto exit;
		}

		if ((mqttCurContext->state & MQTT_STATE_PUBLISH) || (mqttCurContext->is_used != MQTT_CONTEXT_CONNECTED)) {
			ret = (ATERR_NOT_ALLOWED);
			goto exit;
		}
		
		g_publish_data = xy_malloc(sizeof(publish_params_t));
		g_publish_data->tcpconnectID = tcpconnectID;
		g_publish_data->msgID = msgID;
		g_publish_data->qos = qos;
		g_publish_data->retain = retain;
		g_publish_data->topic = topic;
		g_publish_data->pContext = (void *)mqttCurContext;

		if (message_len > 0) {
			passthr_fixed_buff_len = message_len;
			xy_enterPassthroughMode((app_passthrough_proc)mqtt_fixed_length_passthr_proc, (app_passthrough_exit)mqtt_passthr_exit);
		}else {
			xy_enterPassthroughMode((app_passthrough_proc)mqtt_indefinite_length_passthrough_proc, (app_passthrough_exit)mqtt_passthr_exit);
		}
		
		send_urc_to_ext_NoCache("\r\n>",strlen("\r\n>"));
		return AT_ASYN;
		
	exit:   
		if (topic != NULL) {
			xy_free(topic);	
		}
		if (g_publish_data != NULL) {
			xy_free(g_publish_data);
		}
		return ret;	
    }
#if (AT_CUT!=1)
	else if (g_req_type == AT_CMD_TEST) {
		*prsp_cmd = xy_malloc(64);
		snprintf(*prsp_cmd, 64, "(0),(0-65535),(0-2),(0,1),<topic>,(1-1024)");
		return AT_END;
	}
#endif
	else {
		return  (ATERR_PARAM_INVALID);
	}
}
#endif
