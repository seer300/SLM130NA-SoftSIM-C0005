#ifndef _CTLW_FOTA_H
#define _CTLW_FOTA_H


#include "lwip/sockets.h"
#include "ctlw_parseuri.h"


//#define FOTA_SERVER_IP "139.196.187.107"
#define FOTA_SERVER_IP "192.168.157.212"

#define FOTA_SERVER_PORT 5683
#define FOTA_PATH1 "file"
#define START_NUM 0
#define START_MORE 0
#define BLOCK_SIZE 512
#define MAX_DOWNLOAD_TIMES 5

//#define FOTA_PATH1 ".well-know"
//#define FOTA_PATH2 "core"

typedef enum
{
	FOTA_STATE_IDIL,
	FOTA_STATE_DOWNLOADING,
	FOTA_STATE_DOWNLOADED,
	FOTA_STATE_UPDATING,
} ctiotFotaState;
typedef struct
{
	char *package;
	char *packageUri;
	int packageLength;
	int packageUriLength;
} firmwareWritePara;

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
	FOTA_RESULT_PROTOCOLFAIL, //9 unsupport protocol
	FOTA_RESULT_OVER		  //10 update over,this state just used in URC
} ctiotFotaResult;

typedef struct
{
	int32_t fotaState;
	int32_t fotaResult;
	int32_t notifyUpdatingMsgId;//记录平台put5/0/1(URL)后的第一条notify上报消息的Id
	int32_t sockfd;
	int32_t messageID;
	uint8_t *uri;
	uint8_t *query;
	CTIOT_URI fotaUrl;
	struct sockaddr_storage addr;
	int32_t addr_len;
} ctiotFotaManage;


int fota_start(char *url);
void ctiot_fota_state_changed(void);
int ctlw_fota_check();
void ctlw_fota_state_clear();

/**
 * @brief 判断本次ACK是否为平台下方5/0/1URL后，主动上报5/0/3的ACK回复,若是开启fota下载线程
 * 
 * @param msgId 
 */
void fota_notification_ack_handle(int msgId);

#endif//_CTLW_FOTA_H
