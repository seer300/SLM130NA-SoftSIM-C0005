/*****************************************************************************************************************************	 
 * user_cloud.h
 ****************************************************************************************************************************/

#ifndef USER_CLOUD_H__
#define USER_CLOUD_H__

#include "xy_at_api.h"

//云通信状态机
typedef enum
{
	AT_CLOUD_INIT = 0,
	AT_REQ_QLWEVTIND = AT_CLOUD_INIT,
	AT_WAIT_CDPUPDATE,
	AT_WAIT_QLWSREGIND,
	AT_WAIT_UPDATE_RSP,
	AT_WAIT_SEND_RSP,
	WAIT_DOWN_FINISH,
	AT_CLOUD_READY,  
}At_CDP_STATE;

#define AT_CDP_RESPONSE_TIMEOUT (96) //CDP云通信超时时间

At_status_type DO_Send_Data_By_JK(void);

#endif