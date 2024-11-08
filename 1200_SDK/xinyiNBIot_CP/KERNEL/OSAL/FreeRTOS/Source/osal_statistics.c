#include "cmsis_os2.h"
#include "xy_wdt.h"

#define RECORD_MSG_MAX_NUM    32


typedef struct
{
	const char *tskName;
	uint32_t    tskStartTick;
	uint32_t    tskEndTick;
	const char *tskInShared;
	const char *tskOutShared;
} tskAlive_t;

typedef struct
{
	tskAlive_t  tskMsg[RECORD_MSG_MAX_NUM];
	uint32_t    osRecordCnt;
	uint32_t    osSwitchCnt;
} osRunningTrace_t;

//osRunningTrace_t osThreadTrace = {0};
osRunningTrace_t* osThreadTrace = NULL;

void vTaskSwitchIn(void)
{
//	osRunningTrace_t *pxThreadTrace = &osThreadTrace;
	osRunningTrace_t *pxThreadTrace = osThreadTrace;

	if(pxThreadTrace)
	{
		uint32_t record_cnt = pxThreadTrace->osRecordCnt;

		TaskHandle_t sharedTCB = xTaskGetSharedTaskHandle();

		uint32_t tick_cnt = ( pxThreadTrace->osSwitchCnt == 0 ) ? 0 : osKernelGetTickCount();

		pxThreadTrace->tskMsg[ record_cnt ].tskName = osThreadGetName(NULL);
		pxThreadTrace->tskMsg[ record_cnt ].tskStartTick = tick_cnt;

		pxThreadTrace->tskMsg[ record_cnt ].tskInShared = (sharedTCB == NULL) ? NULL : osThreadGetName(sharedTCB);
	}
}


void vTaskSwitchOut( void )
{
	wdt_refresh();
//	osRunningTrace_t *pxThreadTrace = &osThreadTrace;
	osRunningTrace_t *pxThreadTrace = osThreadTrace;

	if(pxThreadTrace)
	{
		uint32_t record_cnt = pxThreadTrace->osRecordCnt;

		TaskHandle_t sharedTCB = xTaskGetSharedTaskHandle();

		pxThreadTrace->tskMsg[ record_cnt ].tskEndTick = osKernelGetTickCount();

		pxThreadTrace->tskMsg[ record_cnt ].tskOutShared = (sharedTCB == NULL) ? NULL : osThreadGetName(sharedTCB);

		pxThreadTrace->osRecordCnt = ( record_cnt + 1 ) % RECORD_MSG_MAX_NUM;

		( pxThreadTrace->osSwitchCnt )++;
	}
}

void vTaskSwitchInit(void)
{
	osThreadTrace = xy_malloc(sizeof(osRunningTrace_t));
	memset(osThreadTrace, 0, sizeof(osRunningTrace_t));
}
