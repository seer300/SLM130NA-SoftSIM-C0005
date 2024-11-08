#pragma once


/*云通信协议类型：0,CDP;1,ONENET;2,socket;3,mqtt;4,http;5,ctwing;*/
#define  CLOUDTYPE                 0


#define SAVE_IN_FLASH 0

/* 保存配置信息 */
void Save_User_Config(void);