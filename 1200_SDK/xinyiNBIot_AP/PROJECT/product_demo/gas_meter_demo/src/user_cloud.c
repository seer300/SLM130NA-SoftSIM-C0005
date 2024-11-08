#include "user_debug.h"
#include "type_adapt.h"
#include "user_config.h"
#include "cloud_process.h"
#include "user_cloud.h"
#include "xy_utils.h"
#include "vmcu.h"
#include "user_timer.h"
#include "user_lcd.h"

#if XY_AT_CTL
#include "at_CP_api.h"
#include "urc_process.h"
#endif

#define  TIMER_CP_ASSERT  TIMER_LP_USER1
#define at_cmd_num 3
#define at_cmd_max_length 50 //uint：byte
/*一秒心跳包*/
extern uint32_t g_sec_num ;
uint32_t g_boot_cp ;
int g_powerdown_flag = 0;
int g_assert_cp_flag = 0;
uint32_t g_wait_anwser_tick = 0;

const uint8_t at_cmd[][at_cmd_max_length] =
{
    "AT\r\n",
    "AT+CGMM\r\n",
    //"AT+NTSETID=1,865412058366569\r\n",
    //"AT+NTSETID=1,866687060004766\r\n",
    "AT+CGSN=1\r\n",
    "AT+CFUN=1\r\n",
    "AT+CGMR\r\n",

    "AT+CIMI\r\n",
    "AT+NCCID\r\n",
    "AT+CGATT=1\r\n",
    "AT+CGATT?\r\n",
    "AT+CGATT?\r\n",
    "AT+CGATT?\r\n",
    "AT+CGATT?\r\n",
    "AT+CSQ\r\n",
    "AT+NUESTATS\r\n",
    "AT+NSMI=1\r\n",
    "AT+NCDP=221.229.214.202,5683\r\n",
    //"AT+MCWCREATE=221.229.214.202,5683\r\n",
    //"AT+MCWCFGEX=0,0,1\r\n",
    //"AT+MCWFILTER=1,30\r\n",
    //"AT+MCWOPEN=1,300\r\n",

    //断电
    //"AT+MCWSEND=13,46413030303030303130303146,0\r\n",
    //"AT+MCWREAD\r\n",
    //"AT+MCWCLOSE\r\n",
    //"AT+CPOF=2\r\n",

    //PSM
    //"AT+MCWSEND=13,46413030303030303130303146,0\r\n"
    //"AT+MCWREAD\r\n",
    //"AT+MCWCLOSE\r\n",
    //"AT+CEREG=5\r\n",
    //"AT+CEREG?\r\n",
    //"AT+XYRAI\r\n",
    //"AT+WORKLOCK=0\r\n",
};

__RAM_FUNC void cp_assert_Timeout(void)
{
	jk_printf("cp_assert_Timeout\n");
	g_assert_cp_flag = 1;
}

__RAM_FUNC void cp_assert_proc()
{
	if(g_assert_cp_flag == 1)
	{
		g_assert_cp_flag = 0;
		if(CP_Is_Alive() == true)
		{
			jk_printf("do cp assert CP\n");
			AT_Send_And_Get_Rsp("AT+ASSERTCP\r\n",10, NULL, NULL);
		}
	}
}

void cp_assert_process()
{
	int cp_assert_time = 1000 * get_rand_val(1,60); //设置一个1-60s的随机定时器
	jk_printf("cp_assert_time:%d\n",cp_assert_time);
	Timer_AddEvent(TIMER_CP_ASSERT,cp_assert_time, cp_assert_Timeout, 0);
}

/*处理CP核上报"POWERDOWN" URC*/
__RAM_FUNC void URC_CP_SYSDOWN_Proc(char *paramlist)
{
	UNUSED_ARG(paramlist);
	jk_printf("receive powerdown\n");
	g_powerdown_flag  = 1;
	
}

__RAM_FUNC void powerdown_process()
{
	if(g_powerdown_flag == 1)
	{
		g_powerdown_flag = 0;
		jk_printf("stop_cp\r\n");
		Stop_CP(0);
	}
}

//构造700字节数据发送
At_status_type AT_Send_By_Cdp_jk()
{
	At_status_type  at_ret = XY_OK;
	int SendDataLen = 700;
	uint8_t *SendDataAddr = xy_malloc(700);
	uint8_t *SendDataHex = xy_malloc(SendDataLen * 2 + 1);
	uint8_t *at_cmd = xy_malloc(50 + SendDataLen * 2 + 1);
	
	memset(SendDataHex, 0, SendDataLen * 2 + 1);
	memset(at_cmd, 0, 50 + SendDataLen * 2 + 1);

	//将读取到的所有数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(SendDataAddr), SendDataLen, (char *)SendDataHex, (SendDataLen * 2 + 1));
	xy_free(SendDataAddr);
	//把数据填充到相应的AT命令中
	snprintf((char *)at_cmd, 50 + (SendDataLen) * 2, "AT+NMGS=%d,%s\r\n", (SendDataLen), (char *)SendDataHex);
	xy_free(SendDataHex);
	at_ret = Send_AT_Req((char *)at_cmd, AT_CDP_RESPONSE_TIMEOUT);
	xy_free(at_cmd);
	return at_ret;
}

At_status_type Send_Data_By_Cdp_JK(int *send_ret)
{
	At_status_type  at_ret = -1;

	switch (g_cdp_at_step)
	{
		case AT_CLOUD_INIT:
		{
			//配置命令设置
			for(uint32_t i = 0; i<(sizeof(at_cmd)/sizeof(at_cmd[0]));i++)
			{
				at_ret = AT_Send_And_Get_Rsp((char *)at_cmd[i],10, NULL, NULL);
				if(at_ret != XY_OK)
					break;
			}
			
			at_ret = Send_AT_Req("AT+QLWSREGIND=0\r\n", AT_CDP_RESPONSE_TIMEOUT);
			if(at_ret != XY_OK)
				break;

			g_cdp_at_step = AT_WAIT_QLWSREGIND;
            break;
		}
		case AT_WAIT_QLWSREGIND:
		{
			at_ret = Get_AT_Rsp("+QLWEVTIND:3\r\n", NULL);

			if(at_ret != XY_OK)
				break;

			g_cdp_at_step = AT_CLOUD_READY;	//注册完成进入ready态
			break;
		}
		//能够进行数据发送ready态
		case AT_CLOUD_READY:
		{
			if(is_event_set(EVENT_CLOUD_SEND))
			{
				at_ret = AT_Send_By_Cdp_jk();
				
				if(at_ret != XY_OK)
					break;
				
				g_cdp_at_step = AT_WAIT_SEND_RSP;
			}

			break;
		}
		case AT_WAIT_SEND_RSP:
		{
			char send_result[16] = {0};
			at_ret = Get_AT_Rsp("+NSMI:", "%s", send_result);
			if(at_ret != XY_OK)
				break;

			if(!strcmp(send_result, "SENT"))
			{
				*send_ret = 0;
				//数据发送完成后powdown;
				g_cdp_at_step = WAIT_DOWN_FINISH;	

			}
			else //若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			{
				*send_ret = 1;
				jk_printf("send falied!\n");
			}
			break;
		}

		case WAIT_DOWN_FINISH:
		{
			if(g_wait_anwser_tick == 0)
            {
                g_wait_anwser_tick = Get_Tick();
            }
            else if(Check_Ms_Timeout(g_wait_anwser_tick,(30*1000)))
            {
				g_wait_anwser_tick = 0;
                g_cdp_at_step = AT_CLOUD_INIT;	
				clear_event(EVENT_CLOUD_SEND);
				AT_Send_And_Get_Rsp("AT+CPOF=2\r\n",10, NULL, NULL);
            }
            break;
		}

		default:
			break;
	}
	
	return  at_ret;
}

/*识别远程通信事件后，取数据进行远程通信。若通信异常，需用户自行进行容错策略*/
extern int User_Err_Process(int);
__RAM_FUNC At_status_type DO_Send_Data_By_JK(void)
{
	At_status_type at_ret = XY_OK;
	int send_ret = -1;  //0表示发送成功，其他正值表示发送失败

#if RANDOM_ASSERTCP_ON
    cp_assert_process();
#endif

	cp_assert_proc();

	powerdown_process();

	//没有设置发送事件，则退出不发送数据
	if(!is_event_set(EVENT_CLOUD_SEND))
		return at_ret;

    if (CP_Is_Alive() == false)
	{
#if CHECK_BAT_WHEN_BOOT_CP
        // 打开timer2，2ms定时监测碱电
        UserTimer2Init();
#endif

        UserLcdShowSend();

        VmcuNbBoot(1u);
	}

	/*进行PDP激活成功与否的查询，若未成功，则不能继续云操作*/
	if(g_tcpip_ok == 0)
	{
		//异步方式超时等待PDP激活成功
		at_ret = xy_wait_tcpip_ok_asyc(WAIT_CGATT_TIMEOUT);
		
		if(at_ret == XY_OK)	
		{
			g_tcpip_ok = 1;
		}
		else if(at_ret == XY_ATTACHING)	/* 正在attach，继续等待 */
		{
			return at_ret;
		}
		else/* 其他错误 */
		{
			User_Err_Process(at_ret);
			return at_ret;
		}
	}
	
    at_ret = Send_Data_By_Cdp_JK(&send_ret);

	if(send_ret == 1 || at_ret > XY_OK)
	{
		User_Err_Process(at_ret);
	}

	return at_ret;
}


