#pragma once

#include <stdint.h>

//use 64 bytes
typedef struct
{
	/*以下几个参数为时间的快照信息,AP单核软重启仍有效，CP核软重启后无效*/
	uint64_t        wall_time_ms;      //世界时间对应ms数。初始化该值时，以当时的世界时间计算出初始ms值，后续更新查询时，都是以ms体现，以保证最新世界时间精度
	uint64_t        rtc_ms;            //RTC时刻数，是tick数，不是毫秒，需要依靠CONVERT_RTCTICK_TO_MS转化
	uint64_t        frame_ms;          //phy的帧信息转化为相对的ms数，不是tick数。该值只能用于获取offset，不能作为真实计时
	uint16_t        cell_id;           //小区ID
	uint32_t        freq_num;          //频点号
	int8_t			g_zone;				//beijing is +32

	uint32_t		next_PS_sec;		//为RTC的tick精度，仅DEBUG调试输出使用；记录下次3GPP唤醒的时间，若在时间内唤醒，则无需attach
#if VER_BC95
	uint8_t         g_NPSMR_enable;     //NPSMR上报开关，若打开，则深睡时上报+NPSMR:
    uint8_t         g_NITZ;             //世界时间的设置模式;1：由3gpp上报的+CTZEU:更新;0：依靠用户输入AT+CCLK=<yy/MM/dd,hh:mm:ss>[<+-zz>]来更新
    uint8_t         cmee_mode;          //AT命令ERR错误上报模式； 0:\r\nERROR\r\n  1:\r\nCME ERROR:错误码\r\n  2:\r\nCME ERROR:ERROR_STRING\r\n
	uint8_t			at_parity;			//AT口校验位，此参数在AP侧修改保存
#elif VER_BC25
	uint8_t			echo_mode;			//回显模式，设置后立即生效，执行AT&W时保存到出厂NV
	uint8_t			cmee_mode;			//AT命令ERR错误上报模式；设置后立即生效，执行AT&W时保存到出厂NV
	uint8_t			wakup_URC;			//睡眠/唤醒时URC上报开关，设置后立即生效，执行AT&W时保存到出厂NV
	uint8_t			qsclk_mode;			//##1## BC25睡眠模式，0：禁止进入深睡和Standby；1：可以进入深睡和Standby；2：只能进Standby
	
	uint8_t         sock_open_time;     //##36## TCP连接的超时时间，单位：秒
	uint8_t         padding0[3];
#else
	uint8_t         padding0[4];
#endif

	uint8_t			ps_deepsleep_state;	//仅DEBUG调试输出使用；PS深睡时的状态机，0，异常深睡； 1,TAU;2,eDRX/DRX;3,CFUN0;4,CFUN1&&CGATT0; 此状态机在睡眠前更新，与ps同步，以便睡眠后查询调试。
	uint8_t			powerdown_flag;		//BC25对标，标记收到AT+QPOWD=0/1后进深睡的场景，该场景下再次唤醒时上报RDY，而非WAKEUP
	uint8_t			cdp_recovery_flag;	//CDP深睡恢复标志位 1：恢复，0：不恢复
	uint8_t         diag_flag;          //记录系统深睡前log输出的状态，以便深睡后继续更新心跳包相关机制
	uint8_t			cmdm_state;         //cmcc dm register state

	uint16_t		onenetNextMid;		//onenet the newest message id; 每次业务数据交互会变更mid，频率较高
	
	uint32_t        ipv4_addr;		    //v4地址深睡唤醒后，若保持不变，无需上报至应用层
	uint8_t			ipv6_addr[20];		//v6地址深睡唤醒后，保持不变，无需PS重新上报，需和lwip的定义保持一致，所以不能改为16字节

#if VER_BC25 || VER_260Y
	uint16_t        cdp_coap_msgid;    //BC25对标，用于平台下行重传包过滤，防止深睡唤醒后有重复包上报
#endif

#if VER_BC25
	uint16_t        cdp_buffered_num;  //BC25对标
	uint16_t        cdp_received_num;  //BC25对标，下行接收在深睡唤醒后有效
    uint16_t        cdp_dropped_num;   //BC25对标，下行丢弃数据在深睡唤醒后有效
#endif

	uint8_t			padding3[6];		//unused.
}softap_var_nv_t;

extern softap_var_nv_t *g_softap_var_nv;

