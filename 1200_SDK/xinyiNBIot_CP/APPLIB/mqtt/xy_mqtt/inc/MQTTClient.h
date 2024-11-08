#pragma once

#include "MQTTPacket.h"
#include "MQTTTimer.h"

#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */

#if !defined(MAX_MESSAGE_HANDLERS)
#define MAX_MESSAGE_HANDLERS 5 /* redefinable - how many subscriptions do you want? */
#endif

enum QoS { QOS0, QOS1, QOS2, SUBFAIL=0x80 };

/* all failure return codes must be negative */
enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

typedef struct MessageData
{
    MQTTMessage* message;
    MQTTString* topicName;
} MessageData;

typedef struct MQTTConnackData
{
    unsigned char rc;
    unsigned char sessionPresent;
} MQTTConnackData;

typedef struct MQTTSubackData
{
    enum QoS grantedQoS;
} MQTTSubackData;

typedef struct MQTTTimeoutFlag
{
    unsigned char flag;
    Timer timeout;
} MQTTTimeoutFlag;

typedef struct MQTTWaitPktFlag
{
    MQTTTimeoutFlag waitSubAck;
    MQTTTimeoutFlag waitConAck;
    MQTTTimeoutFlag waitUnSubAck;
    MQTTTimeoutFlag waitPubAck;
} MQTTWaitPktFlag;

typedef void (*messageHandler)(MessageData*);

typedef struct MQTTClient
{
    unsigned int next_packetid,
      command_timeout_ms;
    size_t buf_size,
      readbuf_size;
    unsigned char *buf,
      *readbuf;
    unsigned char *subscribetopic;
    unsigned int keepAliveInterval;
    char ping_outstanding;
    MQTTWaitPktFlag waitflag;
    int isconnected;
    int cleansession;
    int close_sock;
    int need_reorganize;
    int reorganize_len;
    int reorganize_recv_len;
    unsigned char *reorganizeBuf;

    struct MessageHandlers
    {
        const char* topicFilter;
        void (*fp) (MessageData*);
    } messageHandlers[MAX_MESSAGE_HANDLERS];      /* Message handlers are indexed by subscription topic */

    void (*defaultMessageHandler) (MessageData*);

    Network* ipstack;
    Timer last_sent, last_received;
#if defined(MQTT_TASK)
    Mutex mutex;
    Thread thread;
#endif
} MQTTClient;

extern osMutexId_t g_mqtt_mutex;

int MQTTSendPacket(MQTTClient* c, int length, Timer* timer);

void MQTTCloseSession(MQTTClient* c);

void MQTTCleanSession(MQTTClient* c);

int deliverMessage(MQTTClient* c, MQTTString* topicName, MQTTMessage* message);

int getNextPacketId(MQTTClient *c);


/**
 * Create an MQTT client object
 * @param client
 * @param network
 * @param command_timeout_ms
 * @param
 */
void MQTTClientInit(MQTTClient* client, Network* network, unsigned int command_timeout_ms,
		unsigned char* sendbuf, size_t sendbuf_size, unsigned char* readbuf, size_t readbuf_size);

