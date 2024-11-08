
/**************************************************************

***************************************************************/
#include <stdint.h>
#include <string.h>

#include "ctlw_config.h"
#ifdef PLATFORM_XINYI
#include "ctwing_util.h"
#endif


#include "ctlw_NV_data.h"
#include "ctlw_lwm2m_sdk.h"
#ifdef PLATFORM_LINUX
#include "../../sample/linux/cache_manage.h"
#endif


/****************************** logic block in the cache **********************/

uint8_t *flashCache = NULL;


#define LOGIC_BLOCK_ENTRY_CNT 2
logic_block_entry_t logicBlockEntryTable[LOGIC_BLOCK_ENTRY_CNT] =
	{
		{.logicBlockNo = block0_params,  .address = NULL, .blockSize = sizeof(NV_params_t),                    .cacheValid = 0, .name = "param"},
		{.logicBlockNo = block1_context, .address = NULL, .blockSize = FLASH_CACHE_SIZE - sizeof(NV_params_t), .cacheValid = 0, .name = "context"},
};

logic_block_entry_t *NV_get_logic_block(NV_logic_block_t logicBlock)
{
	uint8_t i = 0;
	for (i; i < LOGIC_BLOCK_ENTRY_CNT; i++)
	{
		if (logicBlockEntryTable[i].logicBlockNo == logicBlock)
			return &logicBlockEntryTable[i];
	}
	return NULL;
}

/**********************************  utils check sum ****************************************/

uint32_t check_sum(uint8_t *buffer, uint32_t size)
{
	uint32_t i   = 0;
    uint32_t sum = 0;
	for( i=0; i<size; i++ )
	{
		sum += buffer[i];
	}
	return sum;
}

/******************************** list *********************************/

struct c2f_list_t
{
	struct c2f_list_t *next;
};

static int c2f_get_list_count(void *c2fList)
{
	uint32_t count = 0;
	struct c2f_list_t *plist = c2fList;
	while (plist)
	{
		count++;
		plist = plist->next;
	}
	return count;
}

/**********************************  api ****************************************/
int32_t c2f_encode_params(ctiot_context_t *pContext)
{
	NV_params_t *pNVParams;
	uint8_t *pCache;
	logic_block_entry_t *pEntry;
	uint32_t sum;

	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"c2f_encode_params...\r\n");

	NV_get_cache();
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"get cache...\r\n");
	pEntry = (logic_block_entry_t *)NV_get_logic_block(block0_params);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"get logic cache...\r\n");
	pCache = pEntry->address;
	pNVParams = (NV_params_t *)pCache;

	//fill top
	strcpy(pNVParams->hello, "ctiotctiot");
	pNVParams->verInfo.majorV = NV_VERSION_CUR_MAJOR ;
	pNVParams->verInfo.minorV = NV_VERSION_CUR_MINOR ;
	if (pContext->serverIPV4)
	{
		memset(pNVParams->serverIPV4,0,INET_ADDRSTRLEN);
		strcpy(pNVParams->serverIPV4, pContext->serverIPV4);
	}
	else
	{
		pNVParams->serverIPV4[0] = 0;
	}
	if (pContext->serverIPV6)
	{
		memset(pNVParams->serverIPV6,0,INET6_ADDRSTRLEN);
		strcpy(pNVParams->serverIPV6, pContext->serverIPV6);
	}
	else
	{
		pNVParams->serverIPV6[0] = 0;
	}
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	if(pContext->payloadEncryptPin)
	{
		memset(pNVParams->payloadEncryptPin,0,MAX_ENCRYPT_PIN_LEN+1);
		strcpy(pNVParams->payloadEncryptPin,(char *)pContext->payloadEncryptPin);
	}
	else
	{
		pNVParams->payloadEncryptPin[0]=0;
	}
#endif
	//fill sdk params
	pNVParams->portV4 = pContext->portV4;
	pNVParams->portV6 = pContext->portV6;
	pNVParams->lifetime = pContext->lifetime;
	pNVParams->idAuthMode = pContext->idAuthMode;
	pNVParams->idAuthType = pContext->idAuthType;
	pNVParams->onKeepSession = pContext->onKeepSession;
	pNVParams->connectionType = pContext->connectionType;
	pNVParams->recvDataMode = pContext->recvDataMode;
	pNVParams->regParaMode = pContext->regParaMode;
	pNVParams->recvTimeMode = pContext->recvTimeMode;
	pNVParams->recvDataMaxCacheTime = pContext->recvDataMaxCacheTime;
	pNVParams->clientWorkMode = pContext->clientWorkMode;
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	pNVParams->payloadEncryptAlgorithm = pContext->payloadEncryptAlgorithm;
#endif

#ifdef PLATFORM_XINYI
#ifdef WITH_FOTA
    pNVParams->fotaFlag = pContext->fotaFlag;
#endif//endif WITH_FOTA
#endif//endif PLATFORM_XINYI

#if CTIOT_CHIPSUPPORT_DTLS == 1
	pNVParams->pskMode = pContext->pskMode;
	//ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pContext->pskid:%s\r\n",pContext->pskID);
	if ((pContext->pskID != NULL && strlen(pContext->pskID) + 1 > PSKID_LEN) ||(pContext->psk != NULL && strlen(pContext->psk) + 1 > PSK_LEN))
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"psk information err!!!!!\r\n");
		pNVParams->pskId[0] = 0;
		pNVParams->psk[0] = 0;
		pNVParams->pskLen = 0;
#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_PARAMS;
	}
	if(pContext->pskID != NULL)
	{
		memcpy(pNVParams->pskId, pContext->pskID, strlen(pContext->pskID) + 1);
	}
	else
	{
		pNVParams->pskId[0] = 0;
	}
	if(pContext->psk!=NULL)
	{
		memcpy(pNVParams->psk, pContext->psk, strlen(pContext->psk) + 1);
		pNVParams->pskLen = pContext->pskLen;
	}
	else
	{
		pNVParams->psk[0] = 0;
		pNVParams->pskLen = 0;
	}
#endif
	sum = check_sum(pEntry->address+sizeof(pNVParams->checksum), pEntry->blockSize-sizeof(pNVParams->checksum));
#ifndef PLATFORM_XINYI
	if( sum != pNVParams->checksum )
	{

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"params new/old sum:%u,%u \r\n", sum,pNVParams->checksum);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"params change, then updata flash immediately\r\n");
#endif
		pNVParams->checksum = sum;
    	/* Flush the User none volatile memory immediately to File System.  */
#ifdef PLATFORM_XINYI
		if(ctchip_flush_nv(CONFIG_FILE)!=0)
#else
		if(ctchip_flush_nv()!=0)
#endif
		{
#ifdef PLATFORM_XINYI
			NV_free_cache();
#endif//endif PLATFORM_XINYI
			return NV_ERR_NOT_SAVE;
		}
#ifndef PLATFORM_XINYI
	}

	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"params not change\r\n");
	}

	print_cache();//去除无效打印
#endif
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"c2f_encode_params end...\r\n");

#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}

int32_t c2f_encode_context(ctiot_context_t *pContext,bool isImmdiateWrite)
{
	NV_lwm2m_context_t *pNVLwContext;
	NV_lwm2m_server_t *pNVLwServerData;
	NV_lwm2m_observed_t *pNVLwObservedData;
	NV_lwm2m_watcher_t *pNVLwWatcherData;
	NV_lwm2m_watch_server_t *pNVLwWatchServer;
	lwm2m_server_t *pLwServer;
	lwm2m_observed_t *pLwObserved;
	lwm2m_watcher_t *pLwWatcher;
	uint8_t *pCache;
	logic_block_entry_t *pEntry;
	uint32_t i, ii, iii;
	uint32_t sum = 0;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"c2f_encode_context...\r\n");

	NV_get_cache();

	pEntry = (logic_block_entry_t *)NV_get_logic_block(block1_context);

	pCache = pEntry->address;
	pNVLwContext = (NV_lwm2m_context_t *)pCache;

	//fill valid flag
	memset(pNVLwContext->hello,0,12);
	strcpy(pNVLwContext->hello, "ctiotctiot");

	//fill local ip
	//memset(pNVLwContext->localIP,0,40);
	//memcpy(pNVLwContext->localIP, pContext->localIP, sizeof(pNVLwContext->localIP));
	//fill bootflag
#ifdef PLATFORM_XINYI
	memset(pNVLwContext->localIP,0,IP6ADDR_STRLEN_MAX);
	memcpy(pNVLwContext->localIP, pContext->localIP, sizeof(pNVLwContext->localIP));
#endif 
	pNVLwContext->bootFlag = pContext->bootFlag;
	pNVLwContext->contextBindMode = pContext->contextBindMode;
	pNVLwContext->contextLifetime = pContext->contextLifetime;
	pNVLwContext->recvTimeMode = pContext->recvTimeMode;
	pNVLwContext->recvDataMaxCacheTime = pContext->recvDataMaxCacheTime;
	pNVLwContext->addressFamily = pContext->addressFamily;
	//fill lwm2m context
	if (pContext->lwm2mContext == NULL)
	{
		pNVLwContext->serverCnt = 0;
		pNVLwContext->observedCnt = 0;
	}
	else
	{
		pNVLwContext->serverCnt = c2f_get_list_count(pContext->lwm2mContext->serverList);
		pNVLwContext->observedCnt = c2f_get_list_count(pContext->lwm2mContext->observedList);

#ifdef PLATFORM_XINYI 
		//session 文件大小限制，最多只保存6个
		pNVLwContext->observedCnt = pNVLwContext->observedCnt > MAX_OBSERVE_COUNT ? MAX_OBSERVE_COUNT: pNVLwContext->observedCnt;
#endif

		//fill server data
		pLwServer = pContext->lwm2mContext->serverList;
		pCache += sizeof(NV_lwm2m_context_t);
		for (i = 0; i < pNVLwContext->serverCnt; i++)
		{
			pNVLwServerData = (NV_lwm2m_server_t *)pCache;

			pNVLwServerData->binding = pLwServer->binding;
			pNVLwServerData->lifetime = pLwServer->lifetime;
			pNVLwServerData->shortID = pLwServer->shortID;
			pNVLwServerData->registration = pLwServer->registration;
			pNVLwServerData->secObjInstID = pLwServer->secObjInstID;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->lifetime=%d\r\n", pNVLwServerData->lifetime);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->registration=%d\r\n", pNVLwServerData->registration);
			if (pLwServer->location != NULL)
			{
				strcpy(pNVLwServerData->location, pLwServer->location);
			}
			else
			{
				pNVLwServerData->location[0] = 0;
			}

			//goto next serverlist
			pCache += sizeof(NV_lwm2m_server_t);
			pLwServer = pLwServer->next;
		}
		//goto next block
		//fill observed data
		pLwObserved = pContext->lwm2mContext->observedList;
		for (i = 0; i < pNVLwContext->observedCnt; i++)
		{
			pNVLwObservedData = (NV_lwm2m_observed_t *)pCache;

			pNVLwObservedData->uri.flag = pLwObserved->uri.flag;
			pNVLwObservedData->uri.instanceId = pLwObserved->uri.instanceId;
			pNVLwObservedData->uri.objectId = pLwObserved->uri.objectId;
			pNVLwObservedData->uri.resourceId = pLwObserved->uri.resourceId;
			pNVLwObservedData->watcherCnt = c2f_get_list_count(pLwObserved->watcherList);
			//goto next block
			pCache += sizeof(NV_lwm2m_observed_t);

			//fill watcher data
			pLwWatcher = pLwObserved->watcherList;
			for (ii = 0; ii < pNVLwObservedData->watcherCnt; ii++)
			{
				pNVLwWatcherData = (NV_lwm2m_watcher_t *)pCache;

				//params 0
				pNVLwWatcherData->active = pLwWatcher->active;
				pNVLwWatcherData->update = pLwWatcher->update;

				//params 1
				if (pLwWatcher->parameters != NULL)
				{
					pNVLwWatcherData->attrFlag = 1;
					pNVLwWatcherData->toClear = pLwWatcher->parameters->toClear;
					pNVLwWatcherData->toSet = pLwWatcher->parameters->toSet;
					pNVLwWatcherData->minPeriod = pLwWatcher->parameters->minPeriod;
					pNVLwWatcherData->maxPeriod = pLwWatcher->parameters->maxPeriod;
					pNVLwWatcherData->greaterThan = pLwWatcher->parameters->greaterThan;
					pNVLwWatcherData->lessThan = pLwWatcher->parameters->lessThan;
					pNVLwWatcherData->step = pLwWatcher->parameters->step;
				}
				else
				{
					pNVLwWatcherData->attrFlag = 0;
				}

				//params 2
				pNVLwWatcherData->format = pLwWatcher->format;
				memcpy(pNVLwWatcherData->token, pLwWatcher->token, pLwWatcher->tokenLen);
				pNVLwWatcherData->tokenLen = pLwWatcher->tokenLen;
				pNVLwWatcherData->lastTime = pLwWatcher->lastTime;
				pNVLwWatcherData->counter = pLwWatcher->counter;
				//params 3
				pNVLwWatcherData->lastMid = pLwWatcher->lastMid;
				pNVLwWatcherData->lastValue.asInteger = pLwWatcher->lastValue.asInteger;
				pNVLwWatcherData->serverCnt = c2f_get_list_count(pLwWatcher->server);

				//goto next block
				pCache += sizeof(NV_lwm2m_watcher_t);

				//fill watcher server data
				pLwServer = pLwWatcher->server;
				for (iii = 0; iii < pNVLwWatcherData->serverCnt; iii++)
				{
					pNVLwWatchServer = (NV_lwm2m_watch_server_t *)pCache;
					pNVLwWatchServer->shortID = pLwServer->shortID;
					//goto next serverlist
					pCache += sizeof(NV_lwm2m_watch_server_t);
					pLwServer = pLwServer->next;
				}
				//goto next watcherlist
				pLwWatcher = pLwWatcher->next;
			}
			pLwObserved = pLwObserved->next;
		}
	}
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->serverCnt:%d\r\n", pNVLwContext->serverCnt);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->observedCnt:%d\r\n", pNVLwContext->observedCnt);
	//goto next block
	pCache += sizeof(NV_lwm2m_context_t);

	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"context size:%d\r\n",pEntry->blockSize);
	sum = check_sum(pEntry->address+sizeof(pNVLwContext->checksum), pEntry->blockSize-sizeof(pNVLwContext->checksum));
#ifndef PLATFORM_XINYI
	if( sum != pNVLwContext->checksum )
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext new/old sum:%u,%u \r\n", sum,pNVLwContext->checksum);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext change, then updata flash\r\n");
#endif
		pNVLwContext->checksum = sum;

		if(!isImmdiateWrite)
		{
			if(ctchip_update_nv()!=0)
			{
#ifdef PLATFORM_XINYI
				NV_free_cache();
#endif//endif PLATFORM_XINYI
				return NV_ERR_NOT_SAVE;
			}
		}
		else
		{
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"start flush context cache(immediate write)... \r\n");
#ifdef PLATFORM_XINYI
			if(ctchip_flush_nv(SESSION_FILE)!=0)
#else
			if(ctchip_flush_nv()!=0)
#endif
			{
#ifdef PLATFORM_XINYI
				NV_free_cache();
#endif//endif PLATFORM_XINYI
				return NV_ERR_NOT_SAVE;
			}
		}
#ifndef PLATFORM_XINYI
	}
	else
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext not change\r\n");
	}
	//为减少代码段，去掉该部分的debug打印信息
	print_cache();
#endif
#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}


int32_t f2c_encode_params(ctiot_context_t *pContext)
{
	NV_params_t *pNVParams;
	uint8_t *pCache;
	logic_block_entry_t *pEntry;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"f2c_encode_params...\r\n");

	NV_get_cache();
	pEntry = (logic_block_entry_t *)NV_get_logic_block(block0_params);
	if (pEntry->cacheValid == NV_CACHE_INVALID)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context data invalid\r\n");
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_INVALID;
	}
	else if (pEntry->cacheValid == NV_CACHE_IS_NULL)
	{
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_IS_NULL;
	}
	pCache = pEntry->address;
	pNVParams = (NV_params_t *)pCache;

	if (pContext->serverIPV6 != NULL)
	{
		ctlw_lwm2m_free(pContext->serverIPV6);
	}
	if (pContext->serverIPV4 != NULL)
	{
		ctlw_lwm2m_free(pContext->serverIPV4);
	}

	if (pNVParams->serverIPV4[0] != 0)
	{
		pContext->serverIPV4 = ctlw_lwm2m_strdup(pNVParams->serverIPV4);
	}
	else
	{
		pContext->serverIPV4 = NULL;
	}

	if (pNVParams->serverIPV6[0] != 0)
	{
		pContext->serverIPV6 = ctlw_lwm2m_strdup(pNVParams->serverIPV6);
	}
	else
	{
		pContext->serverIPV6 = NULL;
	}
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	if(pNVParams->payloadEncryptPin[0] != 0)
	{
		pContext->payloadEncryptPin = (uint8_t *)ctlw_lwm2m_strdup(pNVParams->payloadEncryptPin);
	}
	else
	{
		pContext->payloadEncryptPin = NULL;
	}
#endif
	pContext->portV4 = pNVParams->portV4;
	pContext->portV6 = pNVParams->portV6;
	pContext->lifetime = pNVParams->lifetime;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pContext->lifetime:%u",pContext->lifetime);
	pContext->idAuthMode = pNVParams->idAuthMode;
	pContext->idAuthType = pNVParams->idAuthType;
	pContext->onKeepSession = pNVParams->onKeepSession;
	pContext->recvDataMode = pNVParams->recvDataMode;
	pContext->connectionType = pNVParams->connectionType;
	pContext->regParaMode = pNVParams->regParaMode;
	pContext->recvTimeMode = (ctiot_timemode_e)pNVParams->recvTimeMode;
	pContext->recvDataMaxCacheTime = pNVParams->recvDataMaxCacheTime;
	pContext->clientWorkMode = pNVParams->clientWorkMode;
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	pContext->payloadEncryptAlgorithm = (ctiot_payload_encrypt_algorithm_e)pNVParams->payloadEncryptAlgorithm;
#endif

#ifdef PLATFORM_XINYI
#ifdef WITH_FOTA
    pContext->fotaFlag = pNVParams->fotaFlag;
#endif
#endif
#if CTIOT_CHIPSUPPORT_DTLS == 1
	pContext->pskMode = pNVParams->pskMode;
	if (pNVParams->pskId[0] != 0 && pNVParams->psk[0] != 0)
	{
		pContext->pskID = ctlw_lwm2m_strdup(pNVParams->pskId);
		pContext->psk = ctlw_lwm2m_malloc(pNVParams->pskLen + 1);//此处malloc长度 从原psklen变为psklen+1 修复电信字符串结尾无'\0' bug
		if(pContext->psk!=NULL)
		{
			memcpy(pContext->psk,pNVParams->psk,pNVParams->pskLen + 1);//此处malloc长度从原psklen变为 psklen+1 修复电信字符串结尾无'\0' bug
		}
		pContext->pskLen = pNVParams->pskLen;
	}
	else
	{
		pContext->pskID = NULL;
		pContext->psk = NULL;
		pContext->pskLen = 0;
	}
#endif
#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}


int32_t cache_get_bootflag(ctiot_boot_flag_e *bootflag)
{
	NV_lwm2m_context_t *pNVLwContext;
	logic_block_entry_t *pEntry;
	uint8_t *pCache;
	NV_get_cache();
	pEntry = (logic_block_entry_t *)NV_get_logic_block(block1_context);
	if (pEntry->cacheValid == NV_CACHE_INVALID)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context data invalid\r\n");
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_INVALID;
	}
	else if (pEntry->cacheValid == NV_CACHE_IS_NULL)
	{
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_IS_NULL;
	}
	pCache = pEntry->address;
	pNVLwContext = (NV_lwm2m_context_t *)pCache;
	*bootflag = pNVLwContext->bootFlag;
#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}



int32_t cache_get_address_family(int32_t *addressFamily)
{
	NV_lwm2m_context_t *pNVLwContext;
	logic_block_entry_t *pEntry;
	uint8_t *pCache;
	NV_get_cache();
	pEntry = (logic_block_entry_t *)NV_get_logic_block(block1_context);
	if (pEntry->cacheValid == NV_CACHE_INVALID)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context data invalid\r\n");
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_INVALID;
	}
	else if (pEntry->cacheValid == NV_CACHE_IS_NULL)
	{
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_IS_NULL;
	}
	pCache = pEntry->address;
	pNVLwContext = (NV_lwm2m_context_t *)pCache;
	*addressFamily = pNVLwContext->addressFamily;
#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}

static lwm2m_server_t *ctlw_utils_findServer(lwm2m_context_t *contextP,
										uint16_t shortID)
{
	lwm2m_server_t *targetP;
	targetP = contextP->serverList;
	while (targetP != NULL && shortID != targetP->shortID)
	{
		targetP = targetP->next;
	}
	return targetP;
}

int32_t f2c_encode_context(ctiot_context_t *pContext)
{
	NV_lwm2m_context_t *pNVLwContext;
	NV_lwm2m_server_t *pNVLwServerData;
	NV_lwm2m_observed_t *pNVLwObservedData;
	NV_lwm2m_watcher_t *pNVLwWatcherData;
	NV_lwm2m_watch_server_t *pNVLwWatchServer;
	uint8_t *pCache;
	lwm2m_server_t *pLwServer;
	lwm2m_observed_t *pLwObserved;
	lwm2m_watcher_t *pLwWatcher;
	logic_block_entry_t *pEntry;
	uint32_t i, ii, iii;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"f2c_encode_context...\r\n");

	NV_get_cache();

	pEntry = (logic_block_entry_t *)NV_get_logic_block(block1_context);
	if (pEntry->cacheValid != NV_CACHE_IS_CORRECT)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context data invalid\r\n");
#ifdef PLATFORM_XINYI
		NV_free_cache();
#endif//endif PLATFORM_XINYI
		return NV_ERR_CACHE_INVALID;
	}

	pCache = pEntry->address;
	pNVLwContext = (NV_lwm2m_context_t *)pCache;
	//memset(pContext->localIP,0,40);
	//memcpy(pContext->localIP, pNVLwContext->localIP, sizeof(pNVLwContext->localIP));
#ifdef PLATFORM_XINYI
	memset(pContext->localIP,0,40);
	memcpy(pContext->localIP, pNVLwContext->localIP, sizeof(pNVLwContext->localIP));
#endif 

	pContext->bootFlag = pNVLwContext->bootFlag;
	pContext->contextBindMode = (ctiot_bind_mode_e)pNVLwContext->contextBindMode;
	pContext->contextLifetime = pNVLwContext->contextLifetime;
	pContext->recvDataMaxCacheTime = pNVLwContext->recvDataMaxCacheTime;
	pContext->recvTimeMode = pNVLwContext->recvTimeMode;
	pContext->addressFamily = pNVLwContext->addressFamily;

	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->serverCnt=%u\r\n", pNVLwContext->serverCnt);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->observedCnt=%u\r\n", pNVLwContext->observedCnt);

	//goto next block
	pCache += sizeof(NV_lwm2m_context_t);

	//fill server list
	for (i = 0; i < pNVLwContext->serverCnt; i++)
	{
		pLwServer = ctlw_lwm2m_malloc(sizeof(lwm2m_server_t));
		if (pLwServer == NULL)
		{
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"malloc err\r\n");
#ifdef PLATFORM_XINYI
			NV_free_cache();
#endif//endif PLATFORM_XINYI
			return NV_ERR_MALLOC;
		}
		memset(pLwServer, 0, sizeof(lwm2m_server_t));
		pNVLwServerData = (NV_lwm2m_server_t *)pCache;

		pLwServer->next = NULL;
		pLwServer->secObjInstID = pNVLwServerData->secObjInstID;
		pLwServer->binding = pNVLwServerData->binding;
		pLwServer->lifetime = pNVLwServerData->lifetime;
		pLwServer->shortID = pNVLwServerData->shortID;
		pLwServer->registration = pNVLwServerData->registration;
		pLwServer->sessionH = NULL;

		if (pNVLwServerData->location[0] != 0)
		{
			pLwServer->location = ctlw_lwm2m_strdup(pNVLwServerData->location);
			if (pLwServer->location == NULL)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"malloc err\r\n");
#ifdef PLATFORM_XINYI
				NV_free_cache();
#endif//endif PLATFORM_XINYI
				return NV_ERR_MALLOC;
			}
		}
		else
		{
			pLwServer->location = NULL;
		}

		pContext->lwm2mContext->serverList = (lwm2m_server_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->serverList, pLwServer);

		pCache += sizeof(NV_lwm2m_server_t);
	}

	//fill observed list
	for (i = 0; i < pNVLwContext->observedCnt; i++)
	{
		pNVLwObservedData = (NV_lwm2m_observed_t *)pCache;
		pLwObserved = ctlw_lwm2m_malloc(sizeof(lwm2m_observed_t));
		if (pLwObserved == NULL)
		{
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"malloc err\r\n");
#ifdef PLATFORM_XINYI
			NV_free_cache();
#endif//endif PLATFORM_XINYI
			return NV_ERR_MALLOC;
		}
		memset(pLwObserved, 0, sizeof(lwm2m_observed_t));

		pLwObserved->uri.flag = pNVLwObservedData->uri.flag;
		pLwObserved->uri.objectId = pNVLwObservedData->uri.objectId;
		pLwObserved->uri.instanceId = pNVLwObservedData->uri.instanceId;
		pLwObserved->uri.resourceId = pNVLwObservedData->uri.resourceId;
		pLwObserved->next = NULL;
		pLwObserved->watcherList = NULL;

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->watcherCnt=%u\r\n", pNVLwObservedData->watcherCnt);

		//goto next block
		pCache += sizeof(NV_lwm2m_observed_t);

		//fill watcher list
		for (ii = 0; ii < pNVLwObservedData->watcherCnt; ii++)
		{
			pNVLwWatcherData = (NV_lwm2m_watcher_t *)pCache;

			pLwWatcher = ctlw_lwm2m_malloc(sizeof(lwm2m_watcher_t));
			if (pLwWatcher == NULL)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"malloc err\r\n");
#ifdef PLATFORM_XINYI
				NV_free_cache();
#endif//endif PLATFORM_XINYI
				return NV_ERR_MALLOC;
			}
			memset(pLwWatcher, 0, sizeof(lwm2m_watcher_t));

			//param 0
			pLwWatcher->active = pNVLwWatcherData->active;
			pLwWatcher->update = pNVLwWatcherData->update;

			//param 1
			if (pNVLwWatcherData->attrFlag == 1)
			{
				pLwWatcher->parameters = ctlw_lwm2m_malloc(sizeof(lwm2m_attributes_t));
				if (pLwWatcher->parameters == NULL)
				{
					ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"malloc err\r\n");
#ifdef PLATFORM_XINYI
					NV_free_cache();
#endif//endif PLATFORM_XINYI
					return NV_ERR_MALLOC;
				}
				memset(pLwWatcher->parameters, 0, sizeof(lwm2m_attributes_t));

				pLwWatcher->parameters->toClear = pNVLwWatcherData->toClear;
				pLwWatcher->parameters->toSet = pNVLwWatcherData->toSet;
				pLwWatcher->parameters->minPeriod = pNVLwWatcherData->minPeriod;
				pLwWatcher->parameters->maxPeriod = pNVLwWatcherData->maxPeriod;
				pLwWatcher->parameters->greaterThan = pNVLwWatcherData->greaterThan;
				pLwWatcher->parameters->lessThan = pNVLwWatcherData->lessThan;
				pLwWatcher->parameters->step = pNVLwWatcherData->step;
			}

			//param 2
			pLwWatcher->format = pNVLwWatcherData->format;
			pLwWatcher->tokenLen = pNVLwWatcherData->tokenLen;
			pLwWatcher->lastTime = pNVLwWatcherData->lastTime;
			pLwWatcher->counter = pNVLwWatcherData->counter;
			memcpy(pLwWatcher->token, pNVLwWatcherData->token, pNVLwWatcherData->tokenLen);
			//param 3
			pLwWatcher->lastMid = pNVLwWatcherData->lastMid;
			pLwWatcher->lastValue.asInteger = pNVLwWatcherData->lastValue.asInteger;

			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->serverCnt=%u\r\n", pNVLwWatcherData->serverCnt);

			//goto next block
			pCache += sizeof(NV_lwm2m_watcher_t);

			//fill watcher server list
			for (iii = 0; iii < pNVLwWatcherData->serverCnt; iii++)
			{
				lwm2m_server_t *serverP;
				pNVLwWatchServer = (NV_lwm2m_watch_server_t *)pCache;
				serverP = ctlw_utils_findServer(pContext->lwm2mContext, pNVLwWatchServer->shortID);
				if (serverP != NULL)
				{
					pLwWatcher->server = (lwm2m_server_t *)LWM2M_LIST_ADD(pLwWatcher->server, serverP);
				}
				//goto next block
				pCache += sizeof(NV_lwm2m_watch_server_t);
			}
			pLwObserved->watcherList = (lwm2m_watcher_t *)LWM2M_LIST_ADD(pLwObserved->watcherList, pLwWatcher);
		}
		pContext->lwm2mContext->observedList = (lwm2m_observed_t *)LWM2M_LIST_ADD(pContext->lwm2mContext->observedList, pLwObserved);
	}

#ifdef PLATFORM_XINYI
	NV_free_cache();
#endif//endif PLATFORM_XINYI
	return NV_OK;
}

/******************************** c2f api *********************************/
#ifndef PLATFORM_XINYI
void print_cache(void)
{

	NV_params_t *pNVParams;
	NV_lwm2m_context_t *pNVLwContext;
	NV_lwm2m_server_t *pNVLwServerData;
	NV_lwm2m_observed_t *pNVLwObservedData;
	NV_lwm2m_watcher_t *pNVLwWatcherData;
	NV_lwm2m_watch_server_t *pNVLwWatchServer;
	uint8_t *pCache;
	logic_block_entry_t *pEntry;

	uint32_t i = 0, ii = 0, iii;

	NV_get_cache();

	pEntry = (logic_block_entry_t *)NV_get_logic_block(block0_params);
	if (pEntry->cacheValid != NV_CACHE_IS_CORRECT)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block0_params data,%u\r\n",pEntry->cacheValid);
		return;
	}

	pCache = pEntry->address;
	pNVParams = (NV_params_t *)pCache;
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"******************* ctiot NV params and context *************\r\n");
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"ctiot_nv param ver:%u.%u\r\n",pNVParams->verInfo.majorV,pNVParams->verInfo.minorV);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->lifetime=%u\r\n", pNVParams->lifetime);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->serverIPV4=%s\r\n", pNVParams->serverIPV4);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->serverIPV6=%s\r\n", pNVParams->serverIPV6);
#if CTIOT_WITH_PAYLOAD_ENCRYPT == 1
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->payloadEncryptPin=%s\r\n", pNVParams->payloadEncryptPin);
#endif
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->portV4=%u\r\n", pNVParams->portV4);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->portV6=%u\r\n", pNVParams->portV6);
	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->bindMode=%u\r\n", pNVParams->bindMode);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->idAuthMode:%u\r\n", pNVParams->idAuthMode);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->idAuthType(1:simid,2:sm9):%u\r\n", pNVParams->idAuthType);

	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->bsAuthType(1:simid,2:sm9):%u\r\n", pNVParams->bsAuthType);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->onKeepSession:%u\r\n", pNVParams->onKeepSession);
	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->onCeLevel2Policy:%u\r\n",  pNVParams->onCeLevel2Policy);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->connectionType:%u\r\n", pNVParams->connectionType);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->recvDataMode:%u\r\n", pNVParams->recvDataMode);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->regParaMode:%u\r\n", pNVParams->regParaMode);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->payloadEncryptAlgorithm:%u\r\n", pNVParams->payloadEncryptAlgorithm);

	pEntry = (logic_block_entry_t *)NV_get_logic_block(block1_context);

	if (pEntry->cacheValid != NV_CACHE_IS_CORRECT)
	{
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context data ,%u\r\n",pEntry->cacheValid);
		return;
	}
	pCache = pEntry->address;
	pNVLwContext = (NV_lwm2m_context_t *)pCache;
#ifdef PLATFORM_XINYI
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams->LocalIP=%s\r\n",pNVLwContext->localIP);
#endif
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->bootFlag=%u\r\n", pNVLwContext->bootFlag);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->addressFamily=%d\r\n", pNVLwContext->addressFamily);
	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->localIP=%s\r\n", pNVLwContext->localIP);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->observedCnt=%u\r\n", pNVLwContext->observedCnt);
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->serverCnt=%u\r\n", pNVLwContext->serverCnt);
	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->instanceUpdateList:%s\r\n", pNVLwContext->instanceUpdateList);
	//ctiot_log_info(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext->ipMode:%u\r\n", pNVLwContext->ipMode);
	//goto next block
	pCache += sizeof(NV_lwm2m_context_t);

	for (i = 0; i < pNVLwContext->serverCnt; i++)
	{
		pNVLwServerData = (NV_lwm2m_server_t *)pCache;
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->binding=%d\r\n", pNVLwServerData->binding);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->lifetime=%d\r\n", pNVLwServerData->lifetime);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->location=%s\r\n", pNVLwServerData->location);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->registration=%d\r\n", pNVLwServerData->registration);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->secObjInstID=%d\r\n", pNVLwServerData->secObjInstID);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwServerData->shortID=%d\r\n", pNVLwServerData->shortID);
		pCache += sizeof(NV_lwm2m_server_t);
	}

	for (i = 0; i < pNVLwContext->observedCnt; i++)
	{
		pNVLwObservedData = (NV_lwm2m_observed_t *)pCache;

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->uri.flag:%u\r\n", pNVLwObservedData->uri.flag);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->uri.objectId:%u\r\n", pNVLwObservedData->uri.objectId);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->uri.instanceId:%u\r\n", pNVLwObservedData->uri.instanceId);
		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->uri.resourceId:%u\r\n", pNVLwObservedData->uri.resourceId);

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwObservedData->watcherCnt:%d\r\n", pNVLwObservedData->watcherCnt);
		pCache += sizeof(NV_lwm2m_observed_t);

		for (ii = 0; ii < pNVLwObservedData->watcherCnt; ii++)
		{
			pNVLwWatcherData = (NV_lwm2m_watcher_t *)pCache;

			//param 0
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->active:%u\r\n", pNVLwWatcherData->active);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->update:%u\r\n", pNVLwWatcherData->update);

			//param 1
			if (pNVLwWatcherData->attrFlag == 1)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->toClear:%u\r\n", pNVLwWatcherData->toClear);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->toSet:%u\r\n", pNVLwWatcherData->toSet);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->minPeriod:%u\r\n", pNVLwWatcherData->minPeriod);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->maxPeriod:%u\r\n", pNVLwWatcherData->maxPeriod);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->greaterThan:%lf\r\n", pNVLwWatcherData->greaterThan);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->lessThan:%lf\r\n", pNVLwWatcherData->lessThan);
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters->step:%lf\r\n", pNVLwWatcherData->step);
			}
			else
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->parameters is NULL\r\n");
			}

			//param 2
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->format:%u\r\n", pNVLwWatcherData->format);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->token:");
			for (iii = 0; iii < pNVLwWatcherData->tokenLen; iii++)
			{
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"%02x ", pNVLwWatcherData->token[iii]);
			}
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"\r\n");
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->tokenLen:%u\r\n", pNVLwWatcherData->tokenLen);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->lastTime:%u\r\n", pNVLwWatcherData->lastTime);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->counter:%u\r\n", pNVLwWatcherData->counter);
			//param 3
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->lastMid:%u\r\n", pNVLwWatcherData->lastMid);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatcherData->lastValue:%lld\r\n", pNVLwWatcherData->lastValue.asInteger);

			pCache += sizeof(NV_lwm2m_watcher_t);
			for (iii = 0; iii < pNVLwWatcherData->serverCnt; iii++)
			{
				pNVLwWatchServer = (NV_lwm2m_watch_server_t *)pCache;

				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwWatchServer->shortID:%u\r\n", pNVLwWatchServer->shortID);

				pCache += sizeof(NV_lwm2m_watch_server_t);
			}
		}
	}
	ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"*********************** end *******************\r\n");
}

#endif


void NV_get_cache(void)
{
	int ret;
	uint32_t 			 sum;
	NV_params_t *pNVParams;
	NV_lwm2m_context_t *pNVLwContext;
	logic_block_entry_t *pEntry;
	uint8_t *pCache = NULL;

	if (flashCache == NULL)
	{
		flashCache = ctchip_get_nv();
		logicBlockEntryTable[0].address = flashCache;
		logicBlockEntryTable[1].address = flashCache + logicBlockEntryTable[0].blockSize;
	}

	if (flashCache != NULL)
	{
		//check entry 1
		pEntry = NV_get_logic_block(block0_params);
		pCache = pEntry->address;
		pNVParams = (NV_params_t *)pCache;

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"check params\r\n");
		ret = memcmp(pNVParams->hello, "ctiotctiot", 10);
		if (ret == 0)
		{
			sum = check_sum(pEntry->address+sizeof(pNVParams->checksum), pEntry->blockSize-sizeof(pNVParams->checksum));
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block0_params sum:%d\r\n",sum);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block0_params pNVParams->checksum:%d\r\n",pNVParams->checksum);

			if( sum == pNVParams->checksum )
			{
			    pEntry->cacheValid = NV_CACHE_IS_CORRECT;
			    ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams_NV_CACHE_IS_CORRECT:header correct,sum correct,%s\r\n", pNVParams->hello);
			}
			else
			{
				pEntry->cacheValid = NV_CACHE_INVALID;
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams_NV_CACHE_INVALID:header correct,sum incorrect,%s\r\n", pNVParams->hello);
			}
		}
		else
		{
			pEntry->cacheValid = NV_CACHE_IS_NULL;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVParams_NV_CACHE_IS_NULL:header incorrect,%s\r\n", pNVParams->hello);

		}

		//check entry 2
		pEntry = NV_get_logic_block(block1_context);
		pCache = pEntry->address;
		pNVLwContext = (NV_lwm2m_context_t *)pCache;

		ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"check context\r\n");
		ret = memcmp(pNVLwContext->hello, "ctiotctiot", 10);
		if (ret == 0)
		{
			sum = check_sum(pEntry->address+sizeof(pNVLwContext->checksum), pEntry->blockSize-sizeof(pNVLwContext->checksum));
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context sum:%d\r\n",sum);
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"block1_context pNVLwContext->checksum:%d\r\n",pNVLwContext->checksum);

		    if( sum == pNVLwContext->checksum )
		    {
				pEntry->cacheValid = NV_CACHE_IS_CORRECT;
			    ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext_NV_CACHE_IS_CORRECT:header correct,sum correct,%s\r\n", pNVLwContext->hello);
		    }
			else
			{
				pEntry->cacheValid = NV_CACHE_INVALID;
				ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext_NV_CACHE_INVALID:header correct,sum incorrect,%s\r\n", pNVLwContext->hello);
			}
		}
		else
		{
			pEntry->cacheValid = NV_CACHE_IS_NULL;
			ctiot_log_debug(LOG_COMMON_MODULE,LOG_OTHER_CLASS,"pNVLwContext_NV_CACHE_IS_NULL:header incorrect,%s\r\n", pNVLwContext->hello);
		}
	}
}

