#ifndef __CTLW_CONFIG_H__
#define __CTLW_CONFIG_H__



#define PLATFORM_XINYI	     1 //适配芯翼芯片
#define WITH_FOTA  1           //FOTA功能

#define LWM2M_CLIENT_MODE  1	//LWM2M客户端模式，服务器模式，启动服务器模式
#define LWM2M_LITTLE_ENDIAN 1    //LWM2M 字节序大小端模式
#define WITH_MBEDTLS     1


#define CTIOT_TIMER_AUTO_UPDATE 1   //自动update
#define CTIOT_CHIPSUPPORT_RAI 1       //是否支持空口释放 0->不支持，1->支持
#if WITH_MBEDTLS_SUPPORT
#if WITH_MBEDTLS
#define CTIOT_CHIPSUPPORT_DTLS 1       //1->支持DTLS
#else
#define CTIOT_CHIPSUPPORT_DTLS 0       //0->不支持DTLS
#endif
#endif
#define CTIOT_CHIPSUPPORT_NBSLEEPING 1 //是否支持休眠 0->不支持，1->支持
#define CTIOT_RECEIVE_MODE0      1//是否支持AT+CTLWRECV mode 0 设置
#define CTIOT_RECEIVE_MODE1      1//是否支持AT+CTLWRECV mode 1 设置
#define CTIOT_RECEIVE_MODE2      1//是否支持AT+CTLWRECV mode 2 设置
#define CTIOT_QUERY_TYPE0		 1//是否支持AT+CTLWGETSTATUS     query_type 0  查询
#define CTIOT_QUERY_TYPE1		 1//是否支持AT+CTLWGETSTATUS     query_type 1  查询
#define CTIOT_QUERY_TYPE2		 1//是否支持AT+CTLWGETSTATUS     query_type 2  查询
#define CTIOT_QUERY_TYPE3		 1//是否支持AT+CTLWGETSTATUS     query_type 3  查询
#define CTIOT_QUERY_TYPE4		 1//是否支持AT+CTLWGETSTATUS     query_type 4  查询
#define CTIOT_QUERY_TYPE5		 1//是否支持AT+CTLWGETSTATUS     query_type 5  查询
#define CTIOT_QUERY_TYPE6		 1//是否支持AT+CTLWGETSTATUS     query_type 6  查询
#define CTIOT_QUERY_TYPE7		 1//是否支持AT+CTLWGETSTATUS     query_type 7  查询

#define CTIOT_ENABLE_MODE1		 1//是否支持AT+CTLWSETMOD       mode  1  设置
#define CTIOT_ENABLE_MODE2		 1//是否支持AT+CTLWSETMOD       mode  2  设置
#define CTIOT_ENABLE_MODE3		 1//是否支持AT+CTLWSETMOD       mode  3  设置
#define CTIOT_ENABLE_MODE4		 1//是否支持AT+CTLWSETMOD       mode  4  设置
#define CTIOT_ENABLE_MODE5		 1//是否支持AT+CTLWSETMOD       mode  5  设置

#define CTIOT_SIMID_ENABLED	     0//是否启用simid认证方式,0-不启用,1-启用
#define CTIOT_SM9_ENABLED	     0//是否启用sm9认证方式,0-不启用,1-启用,目前程序不支持内部SM9认证方式
#define CTIOT_WITH_PAYLOAD_ENCRYPT 0 //是否支持payload加密，0-否，1-是

#define CTIOT_DEFAULT_LOCAL_PORT "56830"
//默认服务器端口
#define CTIOT_DEFAULT_PORT 5683
#define CTIOT_DEFAULT_DTLS_PORT 5684
#define CTIOT_DEFAULT_ENCRYPT_PORT 5683
#define CTIOT_DEFAULT_DTLSPLUS_PORT 5684
#define CTIOT_DEFAULT_SERVER_ID 1
#define CTIOT_UPDATE_ASYNC_TIMEOUT 0 //0-使用默认超时逻辑
#define CTIOT_DEFAULT_LIFETIME 86400 //默认lifetime
#define CTIOT_MAX_LIFETIME		30*86400 //最大lifetime
#define CTIOT_MIN_LIFETIME      300 //最小lifetime
#define CTIOT_SOCK_READ_TIMEOUT_S 0     //s
#define CTIOT_SOCK_READ_TIMEOUT_MS 1000  //ms
#define CTIOT_SOCK_WRITE_TIMEOUT_S 0    //s
#define CTIOT_SOCK_WRITE_TIMEOUT_MS 200 //ms
#define CTIOT_NETWORK_ERROR_WAITTIME 20 //s
#define CTIOT_MAX_PACKET_SIZE 1024
#define CTIOT_MAX_QUEUE_SIZE 5
#define CTIOT_MIN_LIFETIME 300     //重复定义
#define CTIOT_THREAD_TIMEOUT 100 * 1000
#define CTIOT_THREAD_TIMEOUT_FREE 5*CTIOT_THREAD_TIMEOUT
#define CTIOT_MAX_IP_LEN 256
#define CTIOT_PSK_LEN 32
#define CTIOT_PSKID_LEN 32
#define CTIOT_ONE_CYCLE_RECV_COUNTS 1

#define MAX_RECV_DATA_LEN 512
#define MAX_SEND_DATA_LEN 512

#define MAX_ENCRYPT_PIN_LEN 64
#define MAX_OBSERVE_COUNT 6

#define CTIOT_MANUFACTURE "Tianyi IoT Technology Corporation Limited"
#define CTIOT_FIRMWARE_VERSION "FIRMWAREVERSION_1"
#define CTIOT_SERIAL_NUMBER "SER20210001"

#ifdef PLATFORM_XINYI
//芯片相关数据长度
#define CTIOT_ICCID_LEN 	21
#define CTIOT_APN_LEN   	105
#define CTIOT_SV_LEN		65
#define CTIOT_CHIP_LEN		30
#define CTIOT_MODULE_LEN	30
#define CTIOT_IMSI_LEN		20
#define CTIOT_IMEI_LEN		20
#define CTIOT_CELLID_LEN	20
#else
#define CTIOT_ICCID_LEN 	20
#define CTIOT_APN_LEN   	20
#define CTIOT_SV_LEN		20
#define CTIOT_CHIP_LEN		30
#define CTIOT_MODULE_LEN	30
#define CTIOT_IMSI_LEN		20
#define CTIOT_IMEI_LEN		20
#define CTIOT_CELLID_LEN	20
#endif

#define CTIOT_MAX_SEND_COUNT	5
#define CTIOT_MAX_SIMID_LENGTH 256
#endif //__CTLW_CONFIG_H__
