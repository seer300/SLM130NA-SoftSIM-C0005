/** 
* @file        xy_mqtt_api.c
* @brief       This file is the MQTT API that users can call.
* @attention   请参考mqtt_opencpu_demo.c进行二次开发。
* @par  
*/

#if MQTT

/****************************************************************************************************
 *                           Include header files                                                    *
 ***************************************************************************************************/
#include "xy_mqtt_api.h"
#include "MQTTTimer.h"
#include "MQTTClient.h"

/****************************************************************************************************
 *                        Global variable definitions                                                *
 ***************************************************************************************************/
osSemaphoreId_t mqtt_pkt_sem = NULL;

/*****************************************************************************************************
Function    : xy_mqtt_downlink_process
Description : Receive and processing MQTT reply and publish message
Input       : c: MQTT client configure,include network state,timer ,etc
              messageHandler: function for receiving MQTT publish message of subscription topic
              synflag:synchronous mode or asynchronous mode(0:synchronous mode,1:asynchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
*****************************************************************************************************/
int xy_mqtt_downlink_process(MQTTClient* c,messageHandler messageHandler,int synflag)
{
	if (c == NULL)
	{
		return FAILURE;
	}
	
    MQTTHeader header = {0};
    int packet_type =0;
    unsigned short mypacketid =0;
    int len = 0,
        rc = SUCCESS;
    Timer timer;
    TimerInit(&timer);
    MQTTConnackData data;
    memset(&data,0,sizeof(MQTTConnackData));
    if (c == NULL)
        goto exit;

    header.byte = c->readbuf[0];
    packet_type = header.bits.type;

    TimerCountdownMS(&timer, c->command_timeout_ms);

    if ((c->keepAliveInterval > 0) && (packet_type >=1 ))
        TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet

    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] cycle packet_type = %d\n",packet_type);

    switch (packet_type)
    {
        default:
        {
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            goto exit;
        }
        case -1:
        case  0: /* timed out reading packet */
            break;
        case CONNACK:
        {
            if (MQTTDeserialize_connack(&data.sessionPresent, &data.rc, c->readbuf, c->readbuf_size))
            {
                if(data.rc  == 0)
                {
                   c->isconnected =1;
                }
                else
                {
                    rc = FAILURE;
                    MQTTCloseSession(c);
                }
            }
            else if(MQTTDeserialize_connack(&data.sessionPresent, &data.rc, c->readbuf, c->readbuf_size) == 0)
            {
                rc = FAILURE;
            }
            if(synflag && mqtt_pkt_sem != NULL)
                osSemaphoreRelease(mqtt_pkt_sem);
            break;
        }
        case PUBACK:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
            {
                rc = FAILURE;
            }
            if(synflag && mqtt_pkt_sem != NULL)
                osSemaphoreRelease(mqtt_pkt_sem);
            break;
        }
        case SUBACK:
        {
            if (c->subscribetopic == NULL)
            {
                rc = FAILURE;
                break;
            }

            MQTTSubackData subackdata;
            int count = 0;
            subackdata.grantedQoS = QOS0;
            if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, c->readbuf, c->readbuf_size) )
            {
                if (subackdata.grantedQoS != 0x80)
                {
                    rc = MQTTSetMessageHandler(c, c->subscribetopic, messageHandler);
                    if(rc == FAILURE && c != NULL && c->subscribetopic !=NULL) 
                    {
						xy_free(c->subscribetopic);
                    	c->subscribetopic = NULL;
                    }
                }
                else
                {
                    if(c != NULL && c->subscribetopic !=NULL) 
                    {
						xy_free(c->subscribetopic);
                    	c->subscribetopic = NULL;
                    }
                    rc = FAILURE;
                }
            }
            else if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, c->readbuf, c->readbuf_size) == 0)
            {
                rc = FAILURE;
                c->ping_outstanding = 0;
                if (c->cleansession)
                    MQTTCleanSession(c);
            }
            if(synflag && mqtt_pkt_sem != NULL)
                osSemaphoreRelease(mqtt_pkt_sem);
            break;
        }
        case UNSUBACK:
        {
            if (c->subscribetopic == NULL)
            {
                rc = FAILURE;
                break;
            }

            if (MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size))
            {
                /* remove the subscription message handler associated with this topic, if there is one */
                MQTTSetMessageHandler(c, c->subscribetopic, NULL);
                if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }
            }
            else if(MQTTDeserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 0)
            {
                if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }
                rc = FAILURE;
            }
            if(synflag && mqtt_pkt_sem != NULL)
                osSemaphoreRelease(mqtt_pkt_sem);
            break;
        }
        case PUBLISH:
        {
            MQTTString topicName;
            MQTTMessage msg;
            int intQoS;
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1)
            {
                rc = FAILURE;
                break;
            }
            msg.qos = (enum QoS)intQoS;
            deliverMessage(c, &topicName, &msg);

            if (msg.qos != QOS0)
            {
                if (msg.qos == QOS1)
                    len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
                else if (msg.qos == QOS2)
                    len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
                if (len <= 0)
                {
                    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] downlink publish Serialize pubreply fail\n");
                    rc = FAILURE;
                }
                else
                {
                    rc = MQTTSendPacket(c, len, &timer);
                }
            }
            break;
        }
        case PUBREC:
        case PUBREL:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
            else if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
                (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = FAILURE;
            else if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS) // send the PUBREL packet
                rc = FAILURE; // there was a problem

            break;
        }
        case PUBCOMP:
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (MQTTDeserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = FAILURE;
            if(synflag && mqtt_pkt_sem != NULL)
                osSemaphoreRelease(mqtt_pkt_sem);
            break;
        }
        case PINGRESP:
            c->ping_outstanding = 0;
            break;
    }


exit:
    memset(c->readbuf,0,c->readbuf_size);
    return rc;
}

/**************************************************************************************************************
Function    : xy_mqtt_connect
Description : Send MQTT connect message according to parameters in MQTTClient and MQTTPacket_connectData
Input       : c: MQTT client configure,include network state,timer ,etc
              options: MQTT connect message parameter
              synflag:synchronous mode or asynchronous mode(0:asynchronous mode,1:synchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
***************************************************************************************************************/
int xy_mqtt_connect(MQTTClient* c, MQTTPacket_connectData* options,int synflag)
{
	if (c == NULL || options == NULL)
	{
		return FAILURE;
	}
		
    Timer connect_timer;
    int rc = FAILURE;
    MQTTPacket_connectData default_options = MQTTPacket_connectData_initializer;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
     if (c->isconnected) /* don't send connect packet again if we are already connected */
	 {
	     xy_printf(0,XYAPP, WARN_LOG, "[MQTT] connect: now is connected\n");
		 goto exit;
	 }
  
    TimerInit(&connect_timer);
    TimerCountdownMS(&connect_timer, c->command_timeout_ms);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    c->keepAliveInterval = options->keepAliveInterval;
    c->cleansession = options->cleansession;

    if ((len = MQTTSerialize_connect(c->buf, c->buf_size, options)) <= 0)
        goto exit;

    if ((rc = MQTTSendPacket(c, len, &connect_timer)) != SUCCESS)  // send the connect packet
        goto exit;

    if(synflag)
    {
        if(mqtt_pkt_sem == NULL)
            mqtt_pkt_sem = osSemaphoreNew(0xFFFF, 0, NULL);//register semaphore

        if (osSemaphoreAcquire(mqtt_pkt_sem, c->command_timeout_ms) != osOK)
        {
            rc = FAILURE;
            goto exit;
        }
    }


exit:
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif    
	return rc;
}

/**********************************************************************************
Function    : xy_mqtt_disconnect
Description : Send MQTT disconnect message according to parameters in MQTTClient
Input       : c: MQTT client configure,include network state,timer ,etc
              synflag:synchronous mode or asynchronous mode(0:asynchronous mode,1:synchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
**********************************************************************************/
int xy_mqtt_disconnect(MQTTClient* c,int synflag)
{
	if (c == NULL)
	{
		return FAILURE;
	}

    int rc = FAILURE;
    Timer timer;	 // we might wait for incomplete incoming publishes to complete
    int len = 0;
    int i   = 0;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    len = MQTTSerialize_disconnect(c->buf, c->buf_size);
    if (len > 0)
        rc = MQTTSendPacket(c, len, &timer);			// send the disconnect packet

    c->ping_outstanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
    {
        for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        {
            if(c->messageHandlers[i].topicFilter)
            {
                xy_free(c->messageHandlers[i].topicFilter);
                c->messageHandlers[i].topicFilter =NULL;
            }

        }
    }

#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif

      if (synflag && mqtt_pkt_sem != NULL)
      {
          osSemaphoreDelete(mqtt_pkt_sem);
          mqtt_pkt_sem = NULL;
      }
    return rc;
}
/*****************************************************************************
Function    : xy_mqtt_subscribe
Description : Send MQTT subscribe message according to parameters in MQTTClient
Input       : c: MQTT client configure,include network state,timer ,etc
              topicFilter: the topic will unsubscribe
			  qos:quality of service
			  synflag:synchronous mode or asynchronous mode(0:asynchronous mode,1:synchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
*****************************************************************************/
int xy_mqtt_subscribe(MQTTClient* c, const char* topicFilter, int qos,int synflag )
{
	if (c == NULL || topicFilter == NULL)
	{
		return FAILURE;
	}
	
    int rc = FAILURE;
    Timer timer;
    int len = 0;
    MQTTString topic = MQTTString_initializer;
    if(topicFilter == NULL || strlen(topicFilter) == 0)
        goto exit;

    topic.cstring = (char *)xy_malloc(strlen(topicFilter)+1);
	memset(topic.cstring, 0, strlen(topicFilter)+1);
    memcpy(topic.cstring,topicFilter,strlen(topicFilter));
    c->subscribetopic = topic.cstring;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
    if (!c->isconnected)
            goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    len = MQTTSerialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic, &qos);
    if (len <= 0)
        goto exit;

    if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit;

    if(synflag && mqtt_pkt_sem != NULL)
    {
        if (osSemaphoreAcquire(mqtt_pkt_sem, c->command_timeout_ms) != osOK)
        {
            rc = FAILURE;
            goto exit;
        }
    }


exit:
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

/***********************************************************************************************
Function    : xy_mqtt_unsubscribe
Description : Send MQTT unsubscribe message according to parameters in MQTTClient
Input       : c: MQTT client configure,include network state,timer ,etc
              topicFilter: the topic will unsubscribe
              synflag:synchronous mode or asynchronous mode(0:asynchronous mode,1:synchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
***********************************************************************************************/
int xy_mqtt_unsubscribe(MQTTClient* c, const char* topicFilter,int synflag)
{
	if (c == NULL || topicFilter == NULL)
	{
		return FAILURE;
	}

    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    int len = 0;

	topic.cstring = (char *)xy_malloc(strlen(topicFilter)+1);
	memset(topic.cstring, 0, strlen(topicFilter)+1);
    memcpy(topic.cstring,topicFilter,strlen(topicFilter));
	c->subscribetopic = topic.cstring;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
      if (!c->isconnected)
          goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if ((len = MQTTSerialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
        goto exit;
    if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
        goto exit;

    if(synflag && mqtt_pkt_sem != NULL)
    {
        if (osSemaphoreAcquire(mqtt_pkt_sem, c->command_timeout_ms) != osOK)
        {
            rc = FAILURE;
            goto exit;
        }
    }

exit:
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

/*********************************************************************************************
Function    : xy_mqtt_publish
Description : Send MQTT publish message according to parameters in MQTTClient and MQTTMessage
Input       : c: MQTT client configure,include network state,timer ,etc
              topicName:the topic will publish
              message: MQTT publish message parameter
              synflag:synchronous mode or asynchronous mode(0:asynchronous mode,1:synchronous mode)
Output      : None
Return      : 0 -success
             -1 -failure
**********************************************************************************************/
int xy_mqtt_publish(MQTTClient* c, const char* topicName, MQTTMessage* message,int synflag)
{
	if (c == NULL || topicName == NULL || message == NULL)
	{
		return FAILURE;
	}
	
    int rc = FAILURE;
    Timer timer;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

#if defined(MQTT_TASK)
      MutexLock(&c->mutex);
#endif
    if (!c->isconnected)
    {
      xy_printf(0,XYAPP, WARN_LOG, "MQTT is not connected \n");
      goto exit;
    }

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if (message->qos == QOS1 || message->qos == QOS2)
        message->id = getNextPacketId(c);

    len = MQTTSerialize_publish(c->buf, c->buf_size, message->dup, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0)
    {
        xy_printf(0,XYAPP, WARN_LOG, "len <= 0 error len=%d \n",len);
        goto exit;
    }
    if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS) // send the subscribe packet
    {
        xy_printf(0,XYAPP, WARN_LOG, "sendPacket error rc=%d \n",rc);
        goto exit;
    }

    if(synflag && mqtt_pkt_sem != NULL)
    {
        if (osSemaphoreAcquire(mqtt_pkt_sem, c->command_timeout_ms) != osOK)
        {
            rc = FAILURE;
            goto exit;
        }
    }

exit:
#if defined(MQTT_TASK)
      MutexUnlock(&c->mutex);
#endif
    return rc;
}

/*********************************************************************************************
Function    : xy_mqtt_isconnected
Description : Check if MQTT is connected
Input       : c: MQTT client configure,include network state,timer ,etc
Return      : 0:unconnected
              1:connected
**********************************************************************************************/
int xy_mqtt_isconnected(MQTTClient* c)
{
	if (c == NULL)
	{
		return 0;
	}
	
	return c->isconnected;
}

#endif
