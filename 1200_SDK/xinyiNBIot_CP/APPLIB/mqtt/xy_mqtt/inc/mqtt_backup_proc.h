#pragma once

#include "MQTTClient.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define MQTT_CONTEXT_NUM_MAX           (1)

/* default configuration */
#define MQTT_PROTOCOL_VERSION_DEFAULT    (3)
#define MQTT_KEEPALIVE_DEFAULT           (120)
#define MQTT_SESSION_DEFAULT             (1)
#define MQTT_WILLFLAG_DEFAULT            (0)
#define MQTT_SSLMODE_DEFAULT             (0)
#define MQTT_SSLSECLEVEL_DEFAULT         (0)

#define MQTT_PKT_TIMEOUT_DEFAULT      (10)
#define MQTT_RETRY_TIMES_DEFAULT      (3)
#define MQTT_TIMEOUT_NOTICE_DEFAULT   (0)

#define MQTT_TX_BUF_DEFAULT           (4+16+256+1024)    /*header+variable header+topic+payload*/
#define MQTT_RX_BUF_DEFAULT           (4+16+256+1024)    /*header+variable header+topic+payload*/
#define MQTT_YIELD_TIMEOUT_MS_DEFAULT (500)
#define MQTT_TCP_CONNECT_ID_DEFAULT   (0xff)
#define MQTT_CLOUD_TYPE_DEFAULT       (0xff)

#define MQTT_MAX_SUBSCRIBE_NUM        (4)                /*最大可订阅取消订阅的数目*/

#define PASSTHR_MQTT_MAX_LEN          (1024)             /*不定长透传数据最大长度*/


//定制化需求，该地址用于存储CA证书,客户端证书，客户端密钥，由用户进行维护
#define MQTTS_CA_CERT_ADDR_BASE      (USER_FLASH_BASE + USER_FLASH_LEN_MAX - 0x1000)
#define MQTTS_CLIENT_CERT_ADDR_BASE  (USER_FLASH_BASE + USER_FLASH_LEN_MAX - 0x2000)
#define MQTTS_CLIENT_KEY_ADDR_BASE   (USER_FLASH_BASE + USER_FLASH_LEN_MAX - 0x3000)


/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/	
typedef enum {
	MQTTS_VERIFY_NULL = 0,
	MQTTS_VERIFY_SERVER,
	MQTTS_VERIFY_SERVER_CLIENT
}mqtts_verify_mode_e;

typedef enum {
	MQTTS_CA_CERT = 0,
	MQTTS_CLIENT_CERT,
	MQTTS_CLIENT_KEY
}mqtts_cert_type_e;

typedef struct {
	int tcpconnectID;
	int msgID;
	int qos;
	int retain;
	char *topic;
	void *pContext;
	int result;
	int cert_type;
}publish_params_t;

typedef struct {
	char *product_key; 
    char *device_name; 
    char *device_secret; 
}aliauth_t;  

typedef enum
{	
	CLOUD_TYPE_DEFAULT, /*need client id, user name, passwd*/
   	CLOUD_TYPE_ALI,
    CLOUD_TYPE_TENCENT,		
}mqtt_cloud_type_e;

typedef enum {
	MQTT_CONFIG_BASE,
		
	/*Configure the MQTT protocol version*/
	MQTT_CONFIG_VERSION,
	
	/*Configure the keep-alive time*/
	MQTT_CONFIG_KEEPALIVE,
	
	/*Configure the session type*/
	MQTT_CONFIG_SESSION,
	
	/*Configure timeout of message delivery*/
	MQTT_CONFIG_TIMEOUT,
	
	/*Configure Will Information*/
	MQTT_CONFIG_WILL,
	
	/*Configure Alibaba device information for Alibaba Cloud*/
	MQTT_CONFIG_ALIAUTH,

	MQTT_CONFIG_OPEN,

	MQTT_CONFIG_SSLMODE,

	MQTT_CONFIG_SSLCAKEY,

	MQTT_CONFIG_SSLSECLEVEL,
	
	MQTT_CONFIG_MAX,
}mqtt_cfg_type_e;

typedef enum {
    MQTT_CONTEXT_NOT_USED = 0,
	MQTT_CONTEXT_CONFIGED,
	MQTT_CONTEXT_OPENED,
	MQTT_CONTEXT_CONNECTED
}mqtt_context_e;

typedef enum {
	MQTT_STATE_DEFAULT      = 0,
	MQTT_STATE_OPEN		    = 1 << 0,
	MQTT_STATE_CLOSE	    = 1 << 1,
	MQTT_STATE_CONNECT		= 1 << 2,
	MQTT_STATE_DISCONNECT   = 1 << 3,
	MQTT_STATE_SUBSCRIBE	= 1 << 4,
	MQTT_STATE_UNSUBSCRIBE  = 1 << 5,
	MQTT_STATE_PUBLISH      = 1 << 6,
}mqtt_event_state_e;

typedef enum {
	MQTT_OPEN_REQ,
	MQTT_CLOSE_REQ,
	MQTT_CONN_REQ,
	MQTT_DISC_REQ,
	MQTT_SUB_REQ,
	MQTT_UNSUB_REQ,
	MQTT_PUB_REQ,  
	MQTT_PUB_REC, 
    MQTT_PUB_REL, 
    MQTT_PUB_ACK,
    MQTT_PUB_COMP, 
	MQTT_KEEPALIVE_REQ,
	MQTT_TCP_DISCONN_UNEXPECTED_REQ,
	MQTT_REQ_MAX,		
}mqtt_req_type_e;

typedef struct {
	int   req_type;
	int   tcpconnectID;
	int   msg_id;
	int   server_ack_mode;
	int   retained;
	int   message_len;
	void *pContext;
    char *message;
	int   count;   //用于记录实际订阅/取消订阅的主题的数目
	int   qos[MQTT_MAX_SUBSCRIBE_NUM];
	char *topicFilters[MQTT_MAX_SUBSCRIBE_NUM];
}mqtt_req_param_t;

typedef struct {
	char* host;
	int port;
}mqtt_addrinfo_param_t;

typedef struct {
	int pkt_timeout;
	int retry_times;
	int timeout_notice;
}mqtt_timeout_param_t;

typedef struct {
	unsigned char willFlag;
	MQTTPacket_willOptions will;
}mqtt_will_param_t;

typedef struct {
	uint8_t tcpconnectID;
	uint8_t is_used; 
	uint8_t state;  
	uint8_t cloud_type;
	uint8_t ssl_enable;
	uint8_t seclevel;
	
	Network* ipstack;

	int sendbuf_size;
    int readbuf_size;
    char *sendbuf;
    char *readbuf;
	
	mqtt_addrinfo_param_t* addrinfo_data;
	mqtt_timeout_param_t* timeout_data;
	MQTTPacket_connectData* mqtt_conn_data;
	aliauth_t* aliauth_data;

	MQTTClient *mqtt_client;
}mqtt_context_t;   

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
mqtt_context_t *mqttFindContextBytcpid(int tcpconnectID);
mqtt_context_t *mqttCreateContext(int tcpconnectID, int txBufLen, int rxBufLen, int mode);
int mqttDeleteContext(mqtt_context_t *mqttContext);
int mqttConfigContext(int tcpconnectID, int cfgType, void* cfgData);

int mqtt_client_config(int tcpconnectID, int cfgType, void* cfgData);
int mqtt_client_open(int tcpconnectID, char *mqttUri, int mqttPort);
int mqtt_client_close(int tcpconnectID);
int mqtt_client_connect(int tcpconnectID, char *clientId, char* userName, char* passWord);
int mqtt_client_disconnect(int tcpconnectID);
int mqtt_client_subscribe(int tcpconnectID, int msgId, int count, char *topicFilters[], int requestedQoSs[]);
int mqtt_client_unsubscribe(int tcpconnectID, int msgId, int count, char* topicFilters[]);
int mqtt_client_publish(int tcpconnectID, int msgId, int qos, int retained, char* mqttPubTopic, int message_len, char* mqttMessage);
int mqtt_client_publish_passthr_proc(char* buf, uint32_t len);
void mqtt_indefinite_length_passthrough_proc(char *buf, uint32_t data_len);
void mqtt_fixed_length_passthr_proc(char *buf, uint32_t len);
void mqtt_passthr_exit(void);
void mqtt_cakey_passthrough_proc(char *buf, uint32_t len);
void mqtt_cakey_passthr_exit(void);


