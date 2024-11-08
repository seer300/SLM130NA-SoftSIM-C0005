#ifndef _CTLW_USER_FOTA_H
#define _CTLW_USER_FOTA_H
#include "ctlw_lwm2m_sdk.h"
//#define FOTA_SERVER_IP "139.196.187.107"
#define FOTA_SERVER_IP "192.168.157.212"

#define FOTA_SERVER_PORT 5683
#define FOTA_PATH1 "file"
#define START_NUM 0
#define START_MORE 0
#define BLOCK_SIZE 128
#define MAX_DOWNLOAD_TIMES 5

//#define FOTA_PATH1 ".well-know"
//#define FOTA_PATH2 "core"
#ifdef __cplusplus
extern "C"
{
#endif
	typedef enum
	{
		FOTA_STATE_IDIL,
		FOTA_STATE_DOWNLOADING,
		FOTA_STATE_DOWNLOADED,
		FOTA_STATE_UPDATING,
	} ctiotUserFotaState;
	typedef struct
	{
		char *package;
		char *packageUri;
		int packageLength;
		int packageUriLength;
	} firmwareUserWritePara;

	typedef enum
	{
		FOTA_RESULT_INIT,		 //0 init
		FOTA_RESULT_SUCCESS,	 //1 success
		FOTA_RESULT_NOFREE,		 //2 no space
		FOTA_RESULT_OVERFLOW,	//3 downloading overflow
		FOTA_RESULT_DISCONNECT,  //4 downloading disconnect
		FOTA_RESULT_CHECKFAIL,   //5 validate fail
		FOTA_RESULT_NOSUPPORT,   //6 unsupport package
		FOTA_RESULT_URIINVALID,  //7 invalid uri
		FOTA_RESULT_UPDATEFAIL,  //8 update fail
		FOTA_RESULT_PROTOCOLFAIL //9 unsupport protocol
	} ctiotUserFotaResult;

	typedef struct
	{
		int fotaState;
		int fotaResult;
	} ctiotUserFotaManage;

	int ctlw_user_fota_start(char *url);
	void ctlw_user_fota_state_changed(void);
	uint8_t ctlw_get_app_vote_handler(void);
	void ctlw_init_vote_hander(void);
	uint16_t ctlw_user_fota_start_func(void);

	void ctlw_fota_app_notify(char* uri, ctiot_notify_type notifyType, ctiot_notify_subType subType, uint16_t value, uint32_t data1, uint32_t data2, void *reservedData);
#ifdef __cplusplus
}
#endif
#endif//_CTLW_USER_FOTA_H
