/**
 * @file at_mqtt_default.c
 * @brief 
 * 
 */
/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "MQTTClient.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"
#include "xy_net_api.h"
#include "ps_netif_api.h"
#include "at_com.h"
#include "xy_mqtt_api.h"
#include "cloud_utils.h"
/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define  MQTT_NUM            2
#define  MQTT_NET_OK         0
#define  MQTT_NET_ERR       -1
#define  MQTT_NET_TIMEOUT   -2
#define  mqtt_client_commandtimeout(cfg_id)   (((pMQTT_client[cfg_id]) == NULL) ?  2000 : pMQTT_client[cfg_id]->command_timeout_ms)
#define  mqtt_client_socket(cfg_id)   (((pMQTT_client[cfg_id]) == NULL || pMQTT_client[cfg_id]->ipstack == NULL) ?  -1 : pMQTT_client[cfg_id]->ipstack->my_socket)

/*******************************************************************************
 *                        Global variable definitions                          *
 ******************************************************************************/
MQTTClient *pMQTT_client[MQTT_NUM]={NULL};
osThreadId_t g_mqttpacket_handle = NULL;

/*******************************************************************************
 *						  Global function declarations 		                   *
 ******************************************************************************/
 int mqtt_cycle(MQTTClient* c,messageHandler messageHandler,int length,int mqtt_id);


/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/

/*****************************************************************************************
 Function    : mqtt_reorganize_message
 Description : server TCP segmentation need local reorganize_message
 Input       : mqtt_id: AT-command input mqtt client id
 Return      : > 0 :mqtt client id matched ; -1:match failed
 *****************************************************************************************/
int mqtt_reorganize_message(MQTTClient* c,int length,int mqtt_id)
{
    if(c->reorganize_recv_len +length > c->reorganize_len)
    {
        c->need_reorganize = 0;
        c->reorganize_len = 0;
        if(c->reorganizeBuf !=NULL)
        {
            xy_free(c->reorganizeBuf);
            c->reorganizeBuf =NULL;
        }
        return 0;
    }
    else if(c->reorganize_recv_len +length < c->reorganize_len)
    {
        memcpy(&c->reorganizeBuf[c->reorganize_recv_len], c->readbuf,length);
        c->reorganize_recv_len += length;
        return 0;
    }
    else if(c->reorganize_recv_len +length == c->reorganize_len)
    {
        memcpy(&c->reorganizeBuf[c->reorganize_recv_len], c->readbuf,length);
        xy_printf(0,XYAPP, WARN_LOG,"[MQTT] publish packet reorganize cycle,readbufsize: %d,reorganize_len: %d",length,c->reorganize_len);

        memcpy(c->readbuf,c->reorganizeBuf,c->readbuf_size);

        mqtt_cycle(c,NULL,c->reorganize_len,mqtt_id);
        if(c->reorganizeBuf != NULL)
        {
            xy_free(c->reorganizeBuf);
            c->reorganizeBuf =NULL;
        }

        c->need_reorganize = 0;
        c->reorganize_len = 0;
        return 0;
    }

    return 0;
}

/*****************************************************************************************
 Function    : mqtt_version_is_right
 Description : check MQTT version information(only support MQTT v4)
 Input       : version: input mqtt version
 Return      : 1:version is right ; -1:version is error
 *****************************************************************************************/
bool mqtt_version_is_right(int version)
{
    return (version == 4);
}

/*****************************************************************************************
 Function    : mqtt_free_client
 Description : free MQTT client memory
 Input       : client_cfg_id: MQTT client id
 Return      : void
 *****************************************************************************************/
void mqtt_free_client(int client_cfg_id)
{
    int i ;

	osMutexAcquire(g_mqtt_mutex, osWaitForever);
	if(pMQTT_client[client_cfg_id] == NULL)
		return;

    if( pMQTT_client[client_cfg_id]->ipstack->my_socket >= 0)
    {    	
		close(pMQTT_client[client_cfg_id]->ipstack->my_socket);
		pMQTT_client[client_cfg_id]->ipstack->my_socket =-1;
    }
        
    //free subscribe topic memory
    if (pMQTT_client[client_cfg_id]->cleansession)
    {
        for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        {
            if(pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter)
            {
                xy_free(pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter);
                pMQTT_client[client_cfg_id]->messageHandlers[i].topicFilter=NULL;
            }

        }
    }

    if(pMQTT_client[client_cfg_id]->ipstack !=NULL )
    {
        xy_free(pMQTT_client[client_cfg_id]->ipstack);
        pMQTT_client[client_cfg_id]->ipstack =NULL;
    }
    if(pMQTT_client[client_cfg_id]->buf !=NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]->buf);
        pMQTT_client[client_cfg_id]->buf =NULL;
    }
    if(pMQTT_client[client_cfg_id]->readbuf !=NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]->readbuf);
        pMQTT_client[client_cfg_id]->readbuf =NULL;
    }
	if(pMQTT_client[client_cfg_id]->subscribetopic != NULL)
	{
		xy_free(pMQTT_client[client_cfg_id]->subscribetopic);
		pMQTT_client[client_cfg_id]->subscribetopic = NULL;
	}
    if(pMQTT_client[client_cfg_id] != NULL)
    {
        xy_free(pMQTT_client[client_cfg_id]);
        pMQTT_client[client_cfg_id] =NULL;
    }
	osMutexRelease(g_mqtt_mutex);

	return;
}

/*****************************************************************************************
 Function    : mqtt_keepalive
 Description : MQTT keepalive function
 Input       : c: MQTT client
 Return      : success:0 ;fail:-1
 *****************************************************************************************/
int mqtt_keepalive(MQTTClient* c)
{
    int rc = SUCCESS;
    Timer  lastsent = {0};
    Timer  lastreceive ={0};
    Timer  var ={0};


    if (c->keepAliveInterval == 0)
        goto exit;

    var.end_time.tv_sec = 10;
    xy_timersub(&c->last_sent.end_time, &var.end_time, &lastsent.end_time);
    xy_timersub(&c->last_received.end_time, &var.end_time, &lastreceive.end_time);

    if (TimerIsExpired(&lastsent) || TimerIsExpired(&lastreceive))
    {
        if (c->ping_outstanding)
            rc = FAILURE; /* PINGRESP not received in keepalive interval */
        else
        {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 200);
            int len = MQTTSerialize_pingreq(c->buf, c->buf_size);
            if (len > 0 && (rc = MQTTSendPacket(c, len, &timer)) == SUCCESS) // send the ping packet
                c->ping_outstanding = 1;

            xy_printf(0,XYAPP, WARN_LOG,"[MQTT]send keeplive pkt len=%d rc=%d",len,rc);
        }
    }

exit:
    return rc;
}

/*****************************************************************************************
 Function    : mqtt_cycle
 Description : process MQTT downlink packet
 Input       : c :MQTT client
               messageHandler: receive MQTT publish packet for specific topics handler
 Return      : success:0 ;fail:-1
 *****************************************************************************************/
int mqtt_cycle(MQTTClient* c,messageHandler messageHandler,int length,int mqtt_id)
{
    MQTTHeader header = {0};
    int packet_type =0;
    unsigned short mypacketid =0;
    unsigned char *recv_data = NULL;
    unsigned char *recv_data_ptr = NULL;
    char *mqtt_rsp    = (char*)xy_malloc(60);
    int len = 0,
        offset=0,
        rem_len=0 , /* read remaining length */
        payload_len=0 , /* read payload length */
        rc = SUCCESS;
    Timer timer;
    TimerInit(&timer);
    MQTTConnackData data={0};

    if (c == NULL)
        goto exit;

    TimerCountdownMS(&timer, c->command_timeout_ms);

    while(offset < length){
        header.byte = c->readbuf[offset];
        packet_type = header.bits.type;

        if ((c->keepAliveInterval > 0) && (packet_type >=1 ))
            TimerCountdown(&c->last_received, c->keepAliveInterval); // Calculate the timeout according to the latest message

        xy_printf(0,XYAPP, WARN_LOG,"[MQTT] cycle packet_type = %d\n",packet_type);

        recv_data = c->readbuf + offset;
        if(c->need_reorganize == 1)
        {
            xy_printf(0,XYAPP, WARN_LOG,"[MQTT] publish packet reorganize,before %d,come %d,total %d",c->reorganize_recv_len,strlen(c->readbuf),c->reorganize_len);
            mqtt_reorganize_message(c,length,mqtt_id);
            goto exit;
        }

        switch (packet_type)
        {
            default:
            {
                /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
                rc = packet_type;
                goto exit;
            }
            case -1:
            case  0: /* timed out reading packet */
                break;
            case CONNACK:
            {
                c->waitflag.waitConAck.flag = 0;
                if (MQTTDeserialize_connack(&data.sessionPresent, &data.rc, recv_data, c->readbuf_size) == 1)
                {
                    if(data.rc  == 0)
                        c->isconnected =1;
                    else
                    {
                        rc = FAILURE;
                        MQTTCloseSession(c);
                    }
                    snprintf(mqtt_rsp,60, "+MQCONNACK:%d,%d",mqtt_id,data.rc);
					send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));

                }
                else if(MQTTDeserialize_connack(&data.sessionPresent, &data.rc, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "+MQCONNACK:%d deserialize error",mqtt_id);
                    send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    rc = FAILURE;
                    goto exit;
                }

                break;
            }
            case PUBACK:
            {
                c->waitflag.waitPubAck.flag = 0;
                unsigned short mypacketid;
                unsigned char dup, type;
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                {
                    snprintf(mqtt_rsp,60, "+MQPUBACK:%d deserialize error",mqtt_id);
                    send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    rc = FAILURE;
                    goto exit;
                }

                snprintf(mqtt_rsp,60, "+MQPUBACK:%d",mqtt_id);
                send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                break;
            }
            case SUBACK:
            {
				if (c->subscribetopic == NULL)
				{
					rc = FAILURE;
					goto suback_exit;
				}

            	char *suback_rsp  = NULL;
            	suback_rsp  = (char*)xy_malloc2(30 + strlen((const void *)(c->subscribetopic)));
				if (suback_rsp == NULL)
				{
					rc = FAILURE;
					goto suback_exit;
				}
                MQTTSubackData subackdata;
                int count = 0;
                subackdata.grantedQoS = QOS0;
                if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, recv_data, c->readbuf_size) == 1)
                {
                    if (subackdata.grantedQoS != 0x80)
                    {
                        rc = MQTTSetMessageHandler(c, c->subscribetopic, messageHandler);
                        snprintf(suback_rsp,(30 + strlen(c->subscribetopic)), "+MQSUBACK:%d,%s,%d",mqtt_id,c->subscribetopic,subackdata.grantedQoS);
                        send_urc_to_ext(suback_rsp, strlen(suback_rsp));
                    }
                    else
                    {
                        snprintf(mqtt_rsp,60, "+MQSUBACK:subscribe fail");
                        send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    }

                }
                else if (MQTTDeserialize_suback(&mypacketid, 1, &count, (int*)&subackdata.grantedQoS, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "+MQSUBACK:%d deserialize error",mqtt_id);
                    send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    rc = FAILURE;
                    c->ping_outstanding = 0;
                    if (c->cleansession)
                        MQTTCleanSession(c);
                    goto suback_exit;
                }
				
			suback_exit:
				if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }
				if(suback_rsp)
        			xy_free(suback_rsp);
				//未接收到服务器的应答，不允许进行重复订阅
				c->waitflag.waitSubAck.flag = 0;
				
				if (rc == FAILURE)
					goto exit;
                break;
            }
            case UNSUBACK:
            {	
				if (c->subscribetopic == NULL)
				{
					rc = FAILURE;
					goto unsuback_exit;
				}
            	char * suback_rsp  = (char*)xy_malloc2(30 + strlen((const void *)(c->subscribetopic)));
				if (suback_rsp == NULL)
				{
					rc = FAILURE;
					goto unsuback_exit;
				}
                if (MQTTDeserialize_unsuback(&mypacketid, recv_data, c->readbuf_size) == 1)
                {
                    /* remove the subscription message handler associated with this topic, if there is one */
                    MQTTSetMessageHandler(c, c->subscribetopic, NULL);
                    snprintf(suback_rsp,(30 + strlen(c->subscribetopic)), "+MQUNSUBACK:%d,%s",mqtt_id,c->subscribetopic);
                    send_urc_to_ext(suback_rsp, strlen(suback_rsp));

                }
                else if(MQTTDeserialize_unsuback(&mypacketid, recv_data, c->readbuf_size) == 0)
                {
                    snprintf(mqtt_rsp,60, "+MQUNSUBACK:%d deserialize error",mqtt_id);
                    send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    rc = FAILURE;
                    goto unsuback_exit;
                }

			unsuback_exit:
			    if((c != NULL) &&(c->subscribetopic !=NULL))
                {
                    xy_free(c->subscribetopic);
                    c->subscribetopic = NULL;
                }
				if(suback_rsp)
        			xy_free(suback_rsp);
				//未接收到服务器的应答，不允许进行重复去订阅
				c->waitflag.waitUnSubAck.flag = 0;
				
				if (rc == FAILURE)
					goto exit;
                break;
            }
            case PUBLISH:
            {
                MQTTString topicName;
                MQTTMessage msg;
                int intQoS;
                msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */

                if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                   (unsigned char**)&msg.payload, (int*)&msg.payloadlen, recv_data, c->readbuf_size) != 1)
                {
                    rc = FAILURE;
                    goto exit;
                }
                else if((unsigned int)(length-offset-((unsigned char *)msg.payload - recv_data)) < msg.payloadlen)
                {
                    /*Reorganize if TCP segments*/
                    c->need_reorganize = 1;
                    c->reorganize_len = msg.payloadlen + ((unsigned char *)msg.payload - recv_data);
                    c->reorganize_recv_len = length;
                    c->reorganizeBuf = (unsigned char*)xy_malloc2(c->reorganize_len + 1);
					if (c->reorganizeBuf == NULL)
                    {
						rc = FAILURE;
						goto exit;
					}
                    memcpy(c->reorganizeBuf, recv_data,c->reorganize_len);
                    snprintf(mqtt_rsp,60, "+MQPUB:TCP segmentation,packet reorganize");
                    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] publish reorganize total:%d,rcvbuf:%d\n",c->reorganize_len,c->reorganize_recv_len);
                    send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                    goto exit;
                }
                else
                {
                	char *publish_rsp = NULL;
    				char *hexpayload_rsp = NULL;
					char *topic_rsp   = NULL;
                    msg.qos = (enum QoS)intQoS;
                    deliverMessage(c, &topicName, &msg);

					if (topicName.lenstring.len == 0)
					{
						rc = FAILURE;
						goto publish_exit;
					}
					
                    topic_rsp = (char*)xy_malloc2(topicName.lenstring.len + 1);
					if (topic_rsp == NULL)
					{
						rc = FAILURE;
						goto publish_exit;
					}
                    memcpy(topic_rsp,topicName.lenstring.data,topicName.lenstring.len);
                    topic_rsp[topicName.lenstring.len]='\0';

                    hexpayload_rsp = (char*)xy_malloc2(msg.payloadlen * 2 +1);
					if (hexpayload_rsp == NULL)
					{
						rc = FAILURE;
						goto publish_exit;
					}
                    bytes2hexstr(msg.payload, msg.payloadlen,  hexpayload_rsp, msg.payloadlen * 2+1);
                    publish_rsp = (char*)xy_malloc2(30 + topicName.lenstring.len +msg.payloadlen*2);
					if (publish_rsp == NULL)
					{
						rc = FAILURE;
						goto publish_exit;
					}
                    snprintf(publish_rsp,(30 + topicName.lenstring.len +msg.payloadlen*2), "+MQPUB:%d,%s,%d,%d,%d,%d,%s",mqtt_id,topic_rsp,msg.qos,msg.retained,msg.dup,msg.payloadlen,hexpayload_rsp);
                    send_urc_to_ext(publish_rsp, strlen(publish_rsp));

                    if (msg.qos != QOS0)
                    {
                        osMutexAcquire(g_mqtt_mutex, osWaitForever);
                        if (msg.qos == QOS1)
                            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
                        else if (msg.qos == QOS2)
                            len = MQTTSerialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
                        if (len <= 0)
                        {
                            xy_printf(0,XYAPP, WARN_LOG,"[MQTT] downlink publish Serialize pubreply fail\n");
                            rc = FAILURE;
							goto publish_exit;
                        }
                        else
                        {
                            rc = MQTTSendPacket(c, len, &timer);
                        }
                        osMutexRelease(g_mqtt_mutex);
                    }

			publish_exit:
					if(publish_rsp)
        				xy_free(publish_rsp);
					if(hexpayload_rsp)
        				xy_free(hexpayload_rsp);
					if(topic_rsp)
        				xy_free(topic_rsp);
					if(rc == FAILURE)
						goto exit;
                }
                break;
            }
            case PUBREC:
            case PUBREL:
            {
                unsigned short mypacketid;
                unsigned char dup, type;
                osMutexAcquire(g_mqtt_mutex, osWaitForever);
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                {
					rc = FAILURE;
					goto exit;
				}    
                else if ((len = MQTTSerialize_ack(c->buf, c->buf_size,
                    (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                {
					rc = FAILURE;
					goto exit;
				}
                    
                else if ((rc = MQTTSendPacket(c, len, &timer)) != SUCCESS)
                {
					rc = FAILURE; // there was a problem
					goto exit;
				}
                    
                osMutexRelease(g_mqtt_mutex);
                snprintf(mqtt_rsp,60, "+%s:%d,%d,%d",(packet_type == PUBREC) ? "MQPUBREC" : "MQPUBREL",mqtt_id,dup,mypacketid);
				send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                break;
            }
            case PUBCOMP:
            {
                c->waitflag.waitPubAck.flag = 0;
                unsigned short mypacketid;
                unsigned char dup, type;
                if (MQTTDeserialize_ack(&type, &dup, &mypacketid, recv_data, c->readbuf_size) != 1)
                {
					rc = FAILURE;
					goto exit;
				}

                snprintf(mqtt_rsp,60, "+MQPUBCOMP:%d,%d,%d",mqtt_id,dup,mypacketid);
                send_urc_to_ext(mqtt_rsp, strlen(mqtt_rsp));
                break;
            }
            case PINGRESP:
                c->ping_outstanding = 0;
                break;
        }

        recv_data_ptr = recv_data + 1; //recvdata 中remain_len位置
        rem_len = MQTTPacket_decodeBuf(recv_data_ptr, &payload_len);
        offset += 1 + rem_len + payload_len;//HEAD+REAMINLEN+PAYLOADLEN
        xy_printf(0,XYAPP, WARN_LOG,"[MQTT] rem_len:%d ,pay_len:%d,offset:%d,length:%d,buf:%p,recv:%p\n",rem_len,payload_len,offset,length,c->readbuf,recv_data);

    }

exit:
    if(mqtt_rsp)
        xy_free(mqtt_rsp);
	if (c != NULL)
    	memset(c->readbuf,0,c->readbuf_size);
    return rc;
}

/*****************************************************************************
 Function    : mqtt_select_timeout_pro
 Description : SELECT function timeout handler
 Input       : linkstate_rsp: report string
 Return      : void
 *****************************************************************************/
void mqtt_select_timeout_pro(char* linkstate_rsp )
{
    int i;

    for (i = 0; i < MQTT_NUM; i++)
    {
        if(pMQTT_client[i] != NULL)
        {
            if(pMQTT_client[i]->waitflag.waitConAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitConAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "+MQDISCON:%d",i);
                send_urc_to_ext(linkstate_rsp, strlen(linkstate_rsp));
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitSubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitSubAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "+MQDISCON:%d",i);
                send_urc_to_ext(linkstate_rsp, strlen(linkstate_rsp));
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitPubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitPubAck.timeout)))
            {
                snprintf(linkstate_rsp,60, "+MQDISCON:%d",i);
                send_urc_to_ext(linkstate_rsp, strlen(linkstate_rsp));
                mqtt_free_client(i);
            }
            else if(pMQTT_client[i]->waitflag.waitUnSubAck.flag && (TimerIsExpired(&pMQTT_client[i]->waitflag.waitUnSubAck.timeout)))
            {
                snprintf(linkstate_rsp,60,"+MQDISCON:%d",i);
                send_urc_to_ext(linkstate_rsp, strlen(linkstate_rsp));
                mqtt_free_client(i);
            }
        }
    }

	return;
}

/*****************************************************************************
 Function    : mqtt_buf_recv
 Description : use SELECT function to receive MQTT downlink packet
 Input       : socketnum:MQTT client id
 Return      : 0:timeout ; < 0:net error ;> 0 success
 *****************************************************************************/
void mqtt_buf_recv(char* printstring)
{
    int i = 0;
    int ret = -1;
    int maxsocket = -1;
    struct timeval tv;
    fd_set read_fds,exceptfds;
	int timeout = 2000;

    xy_printf(0,XYAPP, WARN_LOG, "mqtt_buf_recv s0:%d s1:%d",  mqtt_client_socket(0), mqtt_client_socket(1));
    FD_ZERO(&read_fds);
    FD_ZERO(&exceptfds);

    for (i = 0; i < MQTT_NUM; i++)
    {
        if(mqtt_client_socket(i) < 0)
        {
            continue;
        }
        //AT cmd disconnect tcp link
        else if(pMQTT_client[i]->close_sock)
        {
            close(pMQTT_client[i]->ipstack->my_socket);
            pMQTT_client[i]->ipstack->my_socket= -1;
			snprintf(printstring,60,"+MQDISCON:%d",i);
            send_urc_to_ext(printstring, strlen(printstring));
            mqtt_free_client(i);
            continue;
        }
        else
        {
            if(maxsocket < mqtt_client_socket(i))
            {
                maxsocket = mqtt_client_socket(i);
            }
            FD_SET(mqtt_client_socket(i), &read_fds);
            FD_SET(mqtt_client_socket(i), &exceptfds);
        }
    }

    if (maxsocket < 0)
    {
        return ;
    }


    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    ret = select(maxsocket+1, &read_fds, NULL, &exceptfds, &tv);


   if (ret == 0)
   {
        xy_printf(0,XYAPP, WARN_LOG, "select NET TIMEOUT maxsocket=%d", maxsocket);
        mqtt_select_timeout_pro(printstring);
        return ;
   }
   else if(ret < 0)
   {
        xy_printf(0,XYAPP, WARN_LOG, "select error ret=%d,err %d", ret, errno);
        return ;
   }

   for (i = 0; i < MQTT_NUM; i++)
   {
       if((mqtt_client_socket(i) >= 0) && (FD_ISSET(mqtt_client_socket(i), &exceptfds)))
       {
           snprintf(printstring,60, "+MQDISCON:%d",i);
           send_urc_to_ext(printstring, strlen(printstring));
           mqtt_free_client(i);
           return ;
       }

       if((mqtt_client_socket(i) >= 0) && (FD_ISSET(mqtt_client_socket(i), &read_fds)))
       {
           ret = recv(mqtt_client_socket(i), pMQTT_client[i]->readbuf, pMQTT_client[i]->readbuf_size, 0);

           if (ret < 0)
           {
               if (errno == EWOULDBLOCK) // time out
                   mqtt_select_timeout_pro(printstring);
               return ;
           }
           else if (ret == 0)
           {
               if(pMQTT_client[i]->close_sock != 1 )
               {
                   snprintf(printstring,60,"+MQDISCON:%d",i);
                   send_urc_to_ext(printstring, strlen(printstring));
                   mqtt_free_client(i);
               }
               return;
           }
           else
           {
               //process downlink pkt
               mqtt_cycle(pMQTT_client[i],NULL,ret,i);
           }

       }

   }

    return;
}

/*******************************************************************************************
 Function    : mqtt_downdata_recv
 Description : process MQTT downlink packet according to the SELECT function return value
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_downdata_recv(void)
{
    int i         = 0;
    char *linkstate_rsp = (char*)xy_malloc(60);
    while(1)
    {
		for (i = 0; i < MQTT_NUM; i++)
		{
			if (mqtt_client_socket(i) >= 0)
			{
				break;
			}
		}
		
		if (i == MQTT_NUM)
		{
			xy_printf(0,XYAPP, WARN_LOG, "[MQTT] quit process downlink pkt thread");
			if(linkstate_rsp)
                xy_free(linkstate_rsp);
            return;	
		}

        mqtt_buf_recv(linkstate_rsp);

        for (i = 0; i < MQTT_NUM; i++)
        {
            if(pMQTT_client[i] != NULL && pMQTT_client[i]->isconnected)
            {
                osMutexAcquire(g_mqtt_mutex, osWaitForever);
                mqtt_keepalive(pMQTT_client[i]);
                osMutexRelease(g_mqtt_mutex);
            }
        }
    }

}

/*******************************************************************************************
 Function    : mqtt_deal_packet
 Description : process MQTT downlink packet function
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_deal_packet()
{
    xy_printf(0,XYAPP, WARN_LOG, "[MQTT]process downlink pkt thread start");

    mqtt_downdata_recv();

	g_mqttpacket_handle = NULL;
	osThreadExit();

    xy_printf(0,XYAPP, WARN_LOG, "[MQTT]process downlink pkt thread end");
}

/*******************************************************************************************
 Function    : mqtt_task_create
 Description : create MQTT process downlink packet task
 Input       : void
 Return      : void
 *******************************************************************************************/
void mqtt_task_create()
{
    osThreadAttr_t task_attr = {0};
	
    if (g_mqttpacket_handle != NULL)
        return;
	
    task_attr.name = "mqtt_process_packet";
	task_attr.priority = osPriorityNormal1;
	task_attr.stack_size = osStackShared;
    g_mqttpacket_handle = osThreadNew((osThreadFunc_t)(mqtt_deal_packet), NULL, &task_attr);
}


/*******************************************************************************************
 Function    : at_MQNEW_req
 Description : establish a new mqtt connection to the mqtt server over the TCP protocol
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +EMQNEW=<server>,<port>,<command_timeout_ms>,<bufsize>[,<cid>]
 *******************************************************************************************/
int at_MQNEW_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
	    int  i           = 0;
	    char *remote_ip  = NULL;
	    char *readbuf    = NULL;
	    char *sendbuf    = NULL;
	    Network *ipstack = NULL;
	    int  buf_size    = 0;
	    int  command_timeout = 0;
	    int  client_cfg_id = -1;
	    int  remote_port = 0;
	    remote_ip = xy_malloc(strlen(at_buf));

	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] NEW BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
			xy_free(remote_ip);
	        return  (ATERR_NOT_NET_CONNECT);
	    }
		if(is_Uplink_FlowCtl_Open())
		{
			xy_free(remote_ip);
			return ATERR_NOT_ALLOWED;
		}

	    if (at_parse_param("%s(),%d(1-65535),%d(2000-90000),%d(20-1000)", at_buf, remote_ip,&remote_port,&command_timeout,&buf_size) != AT_OK)
	    {
	    	xy_free(remote_ip);
	        return  (ATERR_PARAM_INVALID);
	    }

	    for (i = 0; i < MQTT_NUM; i++)
	    {
	        if(pMQTT_client[i] == NULL)
	        {
	            client_cfg_id = i;
	            break;
	        }
	    }

	    if(client_cfg_id == -1)
	    {
	    	xy_free(remote_ip);
	        return  (ATERR_NOT_ALLOWED);
	    }

	    ipstack = xy_malloc(sizeof(Network));
	    sendbuf = xy_malloc2(buf_size);
		if (sendbuf == NULL)
		{
			xy_free(remote_ip);
			xy_free(ipstack);
			return (ATERR_NO_MEM);	
		}
		memset(sendbuf, 0, buf_size);
	    readbuf = xy_malloc2(buf_size);
		if (readbuf == NULL)
		{
			xy_free(remote_ip);
			xy_free(ipstack);
			xy_free(sendbuf);
			return (ATERR_NO_MEM);	
		}
		memset(readbuf, 0, buf_size);
	    NetworkInit(ipstack);
	    if( SUCCESS !=NetworkConnect(ipstack, remote_ip, remote_port))
	    {
	        if( ipstack != NULL && ipstack->my_socket >= 0)
	        {
	            xy_printf(0,XYAPP, WARN_LOG, "[MQTT] CLOSE socket %d client id %d\n",ipstack->my_socket,client_cfg_id);
	            close(ipstack->my_socket);
	        }
	        if(ipstack)
	            xy_free(ipstack );

	        if(sendbuf)
	            xy_free(sendbuf);

	        if(readbuf)
	            xy_free(readbuf);

			xy_free(remote_ip);
	        return  (ATERR_NOT_ALLOWED);
	    }
	    pMQTT_client[client_cfg_id] = xy_malloc(sizeof(MQTTClient));
		memset(pMQTT_client[client_cfg_id], 0, sizeof(MQTTClient));
	    MQTTClientInit(pMQTT_client[client_cfg_id], ipstack, command_timeout, sendbuf, buf_size, readbuf, buf_size);
	    mqtt_task_create();

	    *prsp_cmd = xy_malloc(30);
	    snprintf(*prsp_cmd, 30, "+MQNEW:%d", client_cfg_id);
	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] NEW END\n");

	    xy_free(remote_ip);
	    return AT_END;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*********************************************************************************************************************************************
 Function    : at_MQCON_req
 Description : send MQTT connect messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQCON=<mqtt_id>,<version>,<client_id>,<keepalive_interval>,<cleansession>,<will_flag>[,<will_options>][,<username>,<password>]
 *********************************************************************************************************************************************/
int at_MQCON_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
	    int  mqtt_id = -1;
	    int  ret = AT_END;
	    int  version = 0;
	    int  cleansession      = 0;
	    int  will_flag         = 0;
	    int  qos               = 0;
	    int  retained          = 0;
	    int  willlen           = 0;
	    int  keepalive_interval= 0;
		char *tans_data = NULL;
	    char *client_id = xy_malloc(strlen(at_buf));
	    char *username  = xy_malloc(strlen(at_buf));
	    char *password  = xy_malloc(strlen(at_buf));
	    char *topicName = xy_malloc(strlen(at_buf));
	    char *willmessage= xy_malloc(strlen(at_buf));
	    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] CONNECT BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
	        ret = (ATERR_NOT_NET_CONNECT);
	        goto ERR_PROC;
	    }

	    if (at_parse_param("%d(0-),%d,%s,%d(10-65535),%d(0-1),%d(0-1),%s,%d[0-2],%d[0-1],%l[0-],%h,%s,%s", at_buf, &mqtt_id, &version, client_id, &keepalive_interval, &cleansession, &will_flag, topicName,&qos, &retained, &willlen, willmessage, username, password) != AT_OK)
	    {
	        ret = (ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }

	    if(mqtt_id >= MQTT_NUM || pMQTT_client[mqtt_id] == NULL ||!strcmp(client_id,"") || !mqtt_version_is_right(version))
	    {
	        ret = (ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }
					
	    if(pMQTT_client[mqtt_id]->isconnected == 1 || pMQTT_client[mqtt_id]->waitflag.waitConAck.flag == 1)
	    {
			ret = (ATERR_NOT_ALLOWED);
			goto ERR_PROC;
	    }

	    data.willFlag = will_flag;
	    data.MQTTVersion = version;

	    data.clientID.cstring = client_id;
	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT]connect client=%s",data.clientID.cstring);
	    if(will_flag)
	    {
	        data.will.topicName.cstring = topicName;
	        data.will.qos = qos;
	        data.will.retained = retained;
	        data.will.message.lenstring.len = willlen;
	        data.will.message.lenstring.data = willmessage;
	    }

	    data.keepAliveInterval = keepalive_interval;
	    data.cleansession = cleansession;

		if (strlen(username)) {
			data.username.cstring = username;
		} 	
		if (strlen(password)) {
			data.password.cstring = password;
		}
	 
	    osMutexAcquire(g_mqtt_mutex, osWaitForever);
	    ret = xy_mqtt_connect(pMQTT_client[mqtt_id], &data,0);
	    osMutexRelease(g_mqtt_mutex);
	    if (ret != SUCCESS)
	    {
	        ret = (ATERR_NOT_ALLOWED);
	        goto ERR_PROC;
	    }

	    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitConAck.timeout);
	    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitConAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
	    pMQTT_client[mqtt_id]->waitflag.waitConAck.flag = 1;

	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] CONNECT END\n");
	ERR_PROC:
	    if(willmessage)
	        xy_free(willmessage);
	    if(topicName)
	        xy_free(topicName);
	    if(password)
	        xy_free(password);
	    if(username)
	        xy_free(username);
	    if(client_id)
	        xy_free(client_id);
	    return ret;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*****************************************************************************
 Function    : at_MQDISCON_req
 Description : send MQTT disconnect messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQDISCON=<mqtt_id>
 *****************************************************************************/
int at_MQDISCON_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		unsigned int  time = 0;
	    int  ret  = -1;
	    int  mqtt_id = -1;

	    xy_printf(0,XYAPP, WARN_LOG, "MQTT DISCONNECT BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
	        return  (ATERR_NOT_NET_CONNECT);
	    }

	    if (at_parse_param("%d(0-)", at_buf, &mqtt_id) != AT_OK || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
	    {
	        return  (ATERR_PARAM_INVALID);
	    }

	    if(!pMQTT_client[mqtt_id]->isconnected)
	    {
	        return  (ATERR_NOT_ALLOWED);
	    }

	    osMutexAcquire(g_mqtt_mutex, osWaitForever);
	    ret = xy_mqtt_disconnect(pMQTT_client[mqtt_id],0);
	    osMutexRelease(g_mqtt_mutex);
	    if (ret != SUCCESS)
	    {
	        xy_printf(0,XYAPP, WARN_LOG, "MQTT disconnect failed!");
	        return  (ATERR_NOT_ALLOWED);
	    }

	    pMQTT_client[mqtt_id]->close_sock = 1;

	    xy_printf(0,XYAPP, WARN_LOG, "MQTT DISCONNECT END\n");

	    return AT_END;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*****************************************************************************
 Function    : at_MQSUB_req
 Description : send MQTT subscribe messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQSUB=<mqtt_id>,<topic>,<QoS>
 *****************************************************************************/
int at_MQSUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
	    int  ret = -1;
	    int  mqtt_id = -1;
	    int  qos     = 0;
	    char *topic = xy_malloc2(strlen(at_buf));
		if (topic == NULL)
		{
			return (ATERR_NO_MEM);
		}

	    xy_printf(0,XYAPP, WARN_LOG, "[MQTT] subscribe BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
			xy_free(topic);
	        return  (ATERR_NOT_NET_CONNECT);
	    }
		
		if(is_Uplink_FlowCtl_Open())
		{
			xy_free(topic);
			return ATERR_NOT_ALLOWED;
		}

	    if (at_parse_param("%d(0-),%s(),%d(0-2)", at_buf, &mqtt_id, topic, &qos) != AT_OK || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
	    {
	    	xy_free(topic);
	        return  (ATERR_PARAM_INVALID);
	    }

	    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitSubAck.flag == 1)
	    {
	    	xy_free(topic);
	        return  (ATERR_NOT_ALLOWED);
	    }

	    osMutexAcquire(g_mqtt_mutex, osWaitForever);
	    ret = xy_mqtt_subscribe(pMQTT_client[mqtt_id], topic, qos,0);
	    osMutexRelease(g_mqtt_mutex);
	    if (ret !=SUCCESS)
	    {
	    	xy_free(topic);
	        return  (ATERR_NOT_ALLOWED);
	    }
	    else
	    {
	        TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitSubAck.timeout);
	        TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitSubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
	        pMQTT_client[mqtt_id]->waitflag.waitSubAck.flag = 1;
	    }

	    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] subscribe END\n");

		xy_free(topic);
	    return AT_END;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*****************************************************************************
 Function    : at_MQUNSUB_req
 Description : send MQTT unsubscribe messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQUNSUB=<mqtt_id>,<topic>
 *****************************************************************************/
int at_MQUNSUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int  ret = -1;
	    int  mqtt_id = -1;
	    char *topic = xy_malloc2(strlen(at_buf));
		if (topic == NULL)
		{
			return (ATERR_NO_MEM);
		}
		
	    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] unsubscribe BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
			xy_free(topic);
	        return  (ATERR_NOT_NET_CONNECT);
	    }

		if(is_Uplink_FlowCtl_Open())
		{
			xy_free(topic);
			return ATERR_NOT_ALLOWED;
		}

	    if (at_parse_param("%d(0-),%s()", at_buf, &mqtt_id, topic) != AT_OK || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
	    {
	    	xy_free(topic);
	        return  (ATERR_PARAM_INVALID);
	    }

	    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.flag == 1)
	    {
	    	xy_free(topic);
	        return  (ATERR_NOT_ALLOWED);
	    }

	    osMutexAcquire(g_mqtt_mutex, osWaitForever);
	    ret = xy_mqtt_unsubscribe(pMQTT_client[mqtt_id], topic,0);
	    osMutexRelease(g_mqtt_mutex);
	    if (ret != SUCCESS)
	    {
	    	xy_free(topic);
	        return  (ATERR_NOT_ALLOWED);
	    }

	    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.timeout);
	    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
	    pMQTT_client[mqtt_id]->waitflag.waitUnSubAck.flag = 1;

	    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] unsubscribe END\n");

		xy_free(topic);
	    return AT_END;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*****************************************************************************
 Function    : at_MQPUB_req
 Description : send MQTT publish messages
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQPUB=<mqtt_id>,<topic>,<QoS>,<retained>,<dup>,<message_len>,<message>
 *****************************************************************************/
int at_MQPUB_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
	    int  ret = AT_END;
	    int  qos = 0;
	    int  dup = 0;
	    int  retained = 0;
	    int  mqtt_id  = -1;
	    int  message_len = 0;
	    MQTTMessage pubmsg;
	    char *topic = xy_malloc2(strlen(at_buf));
		if (topic == NULL)
		{
			ret = (ATERR_NO_MEM);
			goto ERR_PROC;
		}
	    char *message = xy_malloc2(strlen(at_buf)/2);
		if (message == NULL)
		{
			ret = (ATERR_NO_MEM);
			goto ERR_PROC;
		}

	    memset(&pubmsg, 0, sizeof(pubmsg));
	    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] publish BEGIN\n");

	    if (!xy_tcpip_is_ok()) {
	        ret = (ATERR_NOT_NET_CONNECT);
	        goto ERR_PROC;
	    }
		
		if(is_Uplink_FlowCtl_Open())
		{
			ret = ATERR_NOT_ALLOWED;
			goto ERR_PROC;
		}			

	    if (at_parse_param("%d(0-),%s(),%d(0-2),%d(0-1),%d(0-1),%l(1-900),%h", at_buf, &mqtt_id,topic, &qos, &retained, &dup, &message_len, message) != AT_OK || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
	    {
	        ret = (ATERR_PARAM_INVALID);
	        goto ERR_PROC;
	    }

	    if(!pMQTT_client[mqtt_id]->isconnected || pMQTT_client[mqtt_id]->waitflag.waitPubAck.flag == 1)
	    {
	        ret = (ATERR_NOT_ALLOWED);
	        goto ERR_PROC;
	    }

	    pubmsg.payload = message;
	    pubmsg.payloadlen = message_len;
	    pubmsg.qos = qos;
	    pubmsg.retained = retained;
	    pubmsg.dup = dup;

	    osMutexAcquire(g_mqtt_mutex, osWaitForever);
	    ret = xy_mqtt_publish(pMQTT_client[mqtt_id], topic, &pubmsg,0);
	    osMutexRelease(g_mqtt_mutex);
	    if (ret != SUCCESS)
	    {
	        ret = (ATERR_NOT_ALLOWED);
	        goto ERR_PROC;
	    }

	    TimerInit(&pMQTT_client[mqtt_id]->waitflag.waitPubAck.timeout);
	    TimerCountdownMS(&pMQTT_client[mqtt_id]->waitflag.waitPubAck.timeout, pMQTT_client[mqtt_id]->command_timeout_ms);
	    if(qos > 0)
	        pMQTT_client[mqtt_id]->waitflag.waitPubAck.flag = 1;

	    xy_printf(0,XYAPP, WARN_LOG,"[MQTT] publish END\n");

	ERR_PROC:
	    if(topic)
	        xy_free(topic);
	    if(message)
	        xy_free(message);
	    return ret;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

/*****************************************************************************
 Function    : at_MQSTATE_req
 Description : Check if MQTT is connected
 Input       : at_buf   ---data buf
               prsp_cmd ---response cmd
 Output      : None
 Return      : AT_END
 Eg          : +MQSTATE=<mqtt_id>
 *****************************************************************************/
int at_MQSTATE_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		int  mqtt_id  = -1;
		int  isconnected = 0;

	    if (at_parse_param("%d(0-)", at_buf, &mqtt_id) != AT_OK || mqtt_id >= MQTT_NUM  || pMQTT_client[mqtt_id] == NULL)
	    {
	        return  (ATERR_PARAM_INVALID);
	    }
		
		isconnected = xy_mqtt_isconnected(pMQTT_client[mqtt_id]);

		*prsp_cmd = xy_malloc(30);
	    snprintf(*prsp_cmd, 30, "+MQSTATE:%d", isconnected);
		
	    return AT_END;
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
}

