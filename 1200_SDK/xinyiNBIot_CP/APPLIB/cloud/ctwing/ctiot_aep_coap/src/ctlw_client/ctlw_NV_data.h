#ifndef __CTIOT_NV_DATA_H__
#define __CTIOT_NV_DATA_H__


#ifdef PLATFORM_XINYI
#include "ctwing_util.h"
#endif
#include "ctlw_config.h"

#include "ctlw_lwm2m_sdk.h"
#include "ctlw_platform.h"
#ifdef PLATFORM_XINYI
#define FLASH_CACHE_SIZE (XY_CTLW_SESSION_FILE_LEN + XY_CTLW_CONFIG_FILE_LEN)
#else
#define FLASH_CACHE_SIZE 2000
#endif

#define NV_OK 0
#define NV_ERR_PARAMS -1
#define NV_ERR_NOT_SAVE -2
#define NV_ERR_MALLOC -3
#define NV_ERR_CACHE_INVALID -4
#define NV_ERR_CACHE_IS_NULL -5
#define NV_VERSION_CUR_MAJOR 1
#define NV_VERSION_CUR_MINOR 0
#define PSK_LEN 32
#define PSKID_LEN 33

typedef enum
{
	block0_params = 0,
	block1_context = 1
} NV_logic_block_t;
typedef struct{
	uint8_t majorV;
	uint8_t minorV;
}NV_version_t;
#pragma pack(1)


/***********************************************************
               NV ctiot params data
***********************************************************/

typedef struct
{
	// params vailid flag
	uint32_t checksum;
	char hello[12];
	NV_version_t verInfo;
	// sdk params
	char serverIPV4[INET_ADDRSTRLEN+1];
	char serverIPV6[INET6_ADDRSTRLEN+1];
	uint32_t portV4;
	uint32_t portV6;

#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	char payloadEncryptPin[MAX_ENCRYPT_PIN_LEN+1];
#endif
	uint32_t lifetime;

#if CTIOT_CHIPSUPPORT_DTLS == 1
	char pskId[PSKID_LEN];
	uint16_t pskLen;
	char psk[PSK_LEN];
	uint8_t pskMode;
#endif
	ctiot_id_auth_mode_e idAuthMode;
	ctiot_id_auth_type_e idAuthType;
	ctiot_keep_session_e onKeepSession;
	mode_connection_type_e connectionType;			   //mode_id=5 平台DTLS协议类型：0 --- 明文 //1 --- DTLS  //2 --- DTLS+
	reg_param_mode_e regParaMode;
	recv_data_mode_e recvDataMode;
	uint8_t recvTimeMode;
	uint8_t recvDataMaxCacheTime;
	uint8_t payloadEncryptAlgorithm;
	/*终端工作模式*/
	client_work_mode_e clientWorkMode;
#ifdef PLATFORM_XINYI
#ifdef WITH_FOTA
	uint16_t fotaFlag;
#endif//endif WITH_FOTA
#endif//endif PLATFORM_XINYI
} NV_params_t;
#pragma pack()


/************************************************************
            lwm2m context data
************************************************************/

#pragma pack(1)
typedef struct
{
	//lwm2m context vailid flag
	uint32_t checksum;
	char hello[12];
	int32_t addressFamily;
	uint32_t contextLifetime;
	uint8_t contextBindMode;
	ctiot_boot_flag_e bootFlag;
	uint8_t resumeFlag;
	ctiot_timemode_e recvTimeMode;
	uint16_t recvDataMaxCacheTime;
	//lwm2m context data
	uint32_t serverCnt;
	uint32_t observedCnt;
#ifdef PLATFORM_XINYI
	char localIP[INET6_ADDRSTRLEN];
#endif
} NV_lwm2m_context_t;
#pragma pack()

/*********************************
  1. lwm2m server list data
*********************************/
#pragma pack(1)
typedef struct
{
	uint16_t shortID;
	time_t lifetime;
	lwm2m_binding_t binding;
	uint16_t secObjInstID;
	time_t registration;
	char location[64];
} NV_lwm2m_server_t;
#pragma pack()

/*********************************
  2. lwm2m observed list data
*********************************/
#pragma pack(1)
typedef struct
{
	lwm2m_uri_t uri;

	uint32_t watcherCnt;
} NV_lwm2m_observed_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	bool active;
	bool update;

	uint8_t attrFlag;

	//lwm2m_attributes_t
	uint8_t toSet;
	uint8_t toClear;
	uint32_t minPeriod;
	uint32_t maxPeriod;
	double greaterThan;
	double lessThan;
	double step;

	lwm2m_media_type_t format;
	uint8_t token[8];
	size_t tokenLen;
	time_t lastTime;
	uint32_t counter;
	uint16_t lastMid;

	union {
		int64_t asInteger;
		double asFloat;
	} lastValue;

	uint32_t serverCnt;
} NV_lwm2m_watcher_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	uint16_t shortID;
} NV_lwm2m_watch_server_t;
#pragma pack()

typedef struct logic_block_entry
{
	NV_logic_block_t logicBlockNo;
	uint8_t *address;
	uint32_t blockSize;
	uint32_t cacheValid;//0:invalid,1:valid,2:No data
	char *name;
} logic_block_entry_t;

typedef enum
{
	NV_CACHE_INVALID = 0,
	NV_CACHE_IS_CORRECT = 1,
	NV_CACHE_IS_NULL= 2
} nv_cache_status_e;
void NV_get_cache(void);
uint32_t check_sum(uint8_t *buffer, uint32_t size);


int32_t c2f_encode_params(ctiot_context_t *pContext);
int32_t c2f_encode_context(ctiot_context_t *pContext,bool isImmdiateWrite);
int32_t f2c_encode_context(ctiot_context_t *pContext);
int32_t f2c_encode_params(ctiot_context_t *pContext);


int32_t cache_get_bootflag(ctiot_boot_flag_e *bootflag);
int32_t cache_get_address_family(int32_t *addressFamily);


void print_cache(void);

#endif
