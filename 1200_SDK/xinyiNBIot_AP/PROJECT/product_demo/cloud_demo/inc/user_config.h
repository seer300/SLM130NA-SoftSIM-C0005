#pragma once

/*云通信协议类型：0,CDP;1,ONENET;2,socket;3,mqtt;4,http;5,ctwing;*/
#define  CLOUDTYPE               0

#define SAVE_IN_FLASH            0

#define  TIMER_SENDDATA     TIMER_LP_USER1
#define  TIMER_CLOUDUPDATE  TIMER_LP_USER2
#define  PING_TIMER         TIMER_LP_USER3
#define  STOP_CP_TEST_TIMER	TIMER_LP_USER6



typedef struct
{
	uint8_t  data_len;        //8字节的整数倍
	uint8_t  data_period;     //单位为分钟，0表示不触发数据周期发送
	uint8_t  update_period;   //单位为分钟，0表示不触发UPDATE周期发送
	uint8_t  ping_period;     //单位为分钟，0表示不触发PING周期动作;高2位为ping_mode，决定CP核的下电方式
	uint8_t  cp_mode;         //停CP核的模式，0:stop_CP(0);1:Send_Rai；2:AT+CPOF;3:stop_CP随机停；其他值，让CP核自行深睡
}CLOUD_CFG_T;


extern CLOUD_CFG_T  *g_cloud_cfg;