#include "hal_gpio.h"
#include "xy_ftl.h"
#include "urc_process.h"
#include "cloud_process.h"
#include "data_gather.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "at_CP_api.h"
#include "err_process.h"
#include "xy_memmap.h"
#include "user_time.h"
#include "user_config.h"
#include "user_NB_app.h"
#include "at_process.h"
#include "user_adc.h"

const uint8_t at_user_config_cmd[][50] =
{
    "AT+CFUN=0\r\n",
    "ATE0\r\n",
    "ATI\r\n",
    "AT+NCONFIG=AUTOCONNECT,FALSE\r\n",
    "AT+NCONFIG=CR_0354_0338_SCRAMBLING,TRUE\r\n",
    "AT+NCONFIG=CR_0859_SI_AVOID,TRUE\r\n",
    "AT+NCONFIG=CELL_RESELECTION,FALSE\r\n",
    "AT+CEDRXS=0,5,\"0101\"\r\n",
    "AT+CPSMS=1,,,01000011,00000001\r\n",
    "AT+CMEE=1\r\n",
    "AT+CSCON=0\r\n",
    "AT+NCSEARFCN\r\n",
    "AT+CFUN=1\r\n",
    "AT+CGATT=1\r\n",
};

const uint8_t at_cpd_config_cmd[][50] =
{
    "AT+CFUN=0\r\n",
    "AT+QSREGENABLE=0\r\n",
    "AT+QREGSWT=0\r\n",
    "AT+NCDP=221.229.214.202\r\n",
    "AT+QCFG=\"LWM2M/lifetime\",108000\r\n",
    // "AT+NSMI=0\r\n",
    "AT+NNMI=1\r\n",
    "AT+NPSMR=1\r\n",
    "AT+CFUN=1\r\n",
    "AT+CGATT=1\r\n",
};
typedef enum
{
	CDP_PROCESS_INIT = 0,
	AT_CLOUD_REGIST = CDP_PROCESS_INIT,
    AT_WAIT_QLWSREGIND,
	AT_CLOUD_READY,
	AT_WAIT_SEND_RSP,
}METER_SEND_DATA_STATE;

typedef enum
{
	SEND_DATA_INIT = 0,
	CP_BOOT = SEND_DATA_INIT,
    QUERY_CLOUD_STATUES,
    SET_CONFIG,
    WAIT_PDP,
	PREPARE_DATA,
	SEND_DATA_TO_CLOUD,
	WAIT_ANWSER,
    CHECK_CSCON_STATUES,
    CHECK_SLEEP_STATUES,
    WAIT_ONLY,
	HANDLE_CP_STOP,
}METER_SET_CONFIG_STATE;


int meter_data_send_step = 0;
int g_send_at_step = 0;

uint8_t g_resend_time = 0;
uint8_t g_send_buf[512] = {0};
char g_recv_data[128] = {0};
uint16_t g_send_len = 0;
uint8_t g_urc_mark = 0;
uint32_t g_wait_anwser_tick = 0;

__RAM_FUNC void URC_NNMI_Proc(char *paramlist)
{
    //解析NNMI:<data_len>,<recv_data>示例，解析出来的recv_data用户自己处理;
    int data_len = -1;

    at_parse_param("%d,%p", paramlist, &data_len, &g_recv_data[0]);
	xy_printf("datalen=%d,recv_data=%s\n",data_len,&g_recv_data[0]);

	g_urc_mark = 1;
}

__RAM_FUNC void Trigger_Send_Proc(void)
{
    g_send_mark = 0;
    set_event(EVENT_CLOUD_SEND);
    Send_AT_to_Ext("send data\r\n"); 
}

__RAM_FUNC void Clear_Cloud_State(void)
{
    g_resend_time = 0;
    g_send_at_step = 0;
    meter_data_send_step = CP_BOOT;
}
 
__RAM_FUNC At_status_type Set_Water_Meter_User_Config(void)
{
    At_status_type  at_ret = -1;

    //配置命令设置
    for(uint32_t i = 0; i<(sizeof(at_user_config_cmd)/sizeof(at_user_config_cmd[0]));i++)
    {
        at_ret = AT_Send_And_Get_Rsp((char *)at_user_config_cmd[i],10, NULL, NULL);
        if(at_ret != XY_OK)
        {
            xy_assert(0);
        }
    }
    return at_ret;
}

__RAM_FUNC At_status_type Set_Water_Meter_CDP_Config(int *send_ret)
{
    At_status_type  at_ret = -1;

    //配置命令设置
    for(uint32_t i = 0; i<(sizeof(at_cpd_config_cmd)/sizeof(at_cpd_config_cmd[0]));i++)
    {
        at_ret = AT_Send_And_Get_Rsp((char *)at_cpd_config_cmd[i],10, NULL, NULL);
        if(at_ret != XY_OK)
        {
            return at_ret;
        }
    }

    *send_ret = 0;

    return at_ret;
}

//获取待发送的数据，组装为对应的AT命令发送出去;若数据尚未准备好，则继续等待
__RAM_FUNC At_status_type Switch_Data_And_Send(void *SendDataAddr,uint32_t SendDataLen)
{
	At_status_type  at_ret = XY_OK;
	char *SendDataHex = xy_malloc(SendDataLen * 2 + 1);
	char *at_cmd = xy_malloc(50 + SendDataLen * 2 + 1);
	
	memset(SendDataHex, 0, SendDataLen * 2 + 1);
	memset(at_cmd, 0, 50 + SendDataLen * 2 + 1);

	//将读取到的所有数据转换成十六进制形式的字符串
	bytes2hexstr((uint8_t *)(SendDataAddr), SendDataLen, SendDataHex, (SendDataLen * 2 + 1));

	snprintf(at_cmd, 50 + (SendDataLen) * 2, "AT+QLWULDATAEX=%ld,%s,0x0100\r\n", (SendDataLen), (char *)SendDataHex);

	xy_free(SendDataHex);
	//发送AT命令并等待发送结果
	at_ret = Send_AT_Req(at_cmd, CDP_RESPONSE_TIMEOUT);
	
	xy_free(at_cmd);
	
	return at_ret;
}

__RAM_FUNC At_status_type Water_Meter_Send_Data(void *data,int len,int *send_ret, int *send_statues)
{
    At_status_type  at_ret = -1;

    if(*send_statues == 1)
    {
        g_send_at_step = AT_CLOUD_READY;
        *send_statues = 0;
    }

    switch(g_send_at_step)
    {
        case AT_CLOUD_REGIST:
        {
            at_ret = Send_AT_Req("AT+QLWSREGIND=0\r\n", CDP_RESPONSE_TIMEOUT);
            if(at_ret != XY_OK)
            {
                break;
            }

            g_send_at_step = AT_WAIT_QLWSREGIND;
            break;
        }

        case AT_WAIT_QLWSREGIND:
        {
            at_ret = Get_AT_Rsp("+QLWEVTIND:3\r\n", NULL);

            if(at_ret != XY_OK)
            {
                break;
            }

            g_send_at_step = AT_CLOUD_READY;	//注册完成进入ready态
            break;
        }

        //能够进行数据发送ready态
        case AT_CLOUD_READY:
        {
            at_ret = Switch_Data_And_Send(data,len);
                
            if(at_ret != XY_OK)
            {
                break;
            }
                
            g_send_at_step = AT_WAIT_SEND_RSP;

            break;
        }

        case AT_WAIT_SEND_RSP:
        {
			int send_result = -1;
			at_ret = Get_AT_Rsp("+QLWULDATASTATUS:", "%d", &send_result);

			if(at_ret != XY_OK)
				break;

			//如果发送成功则继续下一次发送
			if(send_result == 4)
			{
				*send_ret = 0;
			}
			else //若发送失败，判为网路异常，建议客户容错，例如深睡一段时间后再发送
			{
				xy_printf("send falied!\n");
				*send_ret = 1;
			}
			break;
        }

        default:
            break;
    }

	return  at_ret;
}

__RAM_FUNC At_status_type DO_Send_Data_By_Cloud_WM(void)
{
	At_status_type at_ret = XY_OK;
	int send_mark = -1;
    static uint8_t cp_is_not_abnormal = 0;
    uint8_t mode = 0;
    uint8_t statues = 0;
    static int send_statues = 0;

    switch(meter_data_send_step)
    {
        case CP_BOOT:
        {
            if (CP_Is_Alive() == false)
            {
                if(!Boot_CP(WAIT_CP_BOOT_MS))
                {
                    meter_data_send_step = HANDLE_CP_STOP;
                }
	        }

            meter_data_send_step = QUERY_CLOUD_STATUES;

            break;
        }

        case QUERY_CLOUD_STATUES:
        {
			int event_id = -1;
		    at_ret = AT_Send_And_Get_Rsp("AT+QLWEVTIND?\r\n",10, "+QLWEVTIND:1,", "%d", &event_id);
            if(at_ret > 0)//AT异常，需对CP进行容错处理
            {
                meter_data_send_step = HANDLE_CP_STOP;
            }

			if((event_id != 2) && (event_id != 3) && (event_id != 5))
            {
                meter_data_send_step = SET_CONFIG;
            }
            else
            {
                send_statues = 1;
                at_ret = AT_Send_And_Get_Rsp("AT+CGATT=1\r\n",10, NULL, NULL);
                if(at_ret != XY_OK)
                {
                    return at_ret;
                }
                meter_data_send_step = WAIT_PDP;
            }

            break;            
        }

        case SET_CONFIG:
        {
            at_ret = Set_Water_Meter_CDP_Config(&send_mark);

            if(at_ret > 0)//AT异常，需对CP进行容错处理
            {
                meter_data_send_step = HANDLE_CP_STOP;
            }

            if(send_mark == 0)//设置结束
            {
                send_mark = -1;
                meter_data_send_step = WAIT_PDP; 
            }

            break;
        }
        
        case WAIT_PDP:
        {    
            at_ret = xy_wait_tcpip_ok_asyc(USER_WAIT_SEC);

            if(at_ret == XY_OK)	
            {
                meter_data_send_step = PREPARE_DATA;
            }
            else if(at_ret != XY_ATTACHING)
            {
                meter_data_send_step = HANDLE_CP_STOP;
            }
            break;
        }

        case PREPARE_DATA:
        {
            g_user_temperature = Get_Temperature();

            g_user_vbat = Get_VBAT();

            memset(&g_send_buf[0],0,512);

            at_ret = AT_Send_And_Get_Rsp("AT+CSQ\r\n",10, "+CSQ:","%d,%d", &g_send_buf[0],&g_send_buf[1]);
            if(at_ret != XY_OK)
            {
                meter_data_send_step = HANDLE_CP_STOP;
                break;
            }
            at_ret = AT_Send_And_Get_Rsp("AT+NUESTATS\r\n",10, "Signal power:","%a", &g_send_buf[214]);
            if(at_ret != XY_OK)
            {
                meter_data_send_step = HANDLE_CP_STOP;
                break;
            }

            memcpy(&g_send_buf[2],&Time,40);//填入时间戳

            memcpy(&g_send_buf[42],&g_user_temperature, 2);//填入温度

            memcpy(&g_send_buf[44],&g_user_vbat, 2);//填入VBAT                         

            memcpy(&g_send_buf[46],&g_user_info, 168);//填入用户信息

            g_send_len = 512;

            meter_data_send_step = SEND_DATA_TO_CLOUD;//发送完毕的情况

            break;
        }

        case SEND_DATA_TO_CLOUD:
        {
            at_ret = Water_Meter_Send_Data(&g_send_buf[0], g_send_len, &send_mark, &send_statues);

            if(at_ret > 0)//AT异常，需对CP进行容错处理
            {
                meter_data_send_step = HANDLE_CP_STOP;
            }

            if(send_mark == 0)//发送成功
            {
#if MODE_PSM                
                meter_data_send_step = WAIT_ANWSER;
#else                
                cp_is_not_abnormal = 1;
                meter_data_send_step = HANDLE_CP_STOP;
#endif                
            }
            else if(send_mark == 1)
            {
                meter_data_send_step = HANDLE_CP_STOP; 
            }
            break;	
        }

        case WAIT_ANWSER:
        {
            if(g_wait_anwser_tick == 0)
            {
                g_wait_anwser_tick = Get_Tick();
            }

            if(g_urc_mark == 1)
            {
                g_wait_anwser_tick = 0;
                g_urc_mark = 0; 
                meter_data_send_step = CHECK_CSCON_STATUES;
                Send_AT_to_Ext("\r\nsend success\r\n"); 
            }
            else if(g_urc_mark == 0 && Check_Ms_Timeout(g_wait_anwser_tick,(30*1000)))
            {
                g_wait_anwser_tick = 0;
                g_urc_mark = 0; 
                meter_data_send_step = CHECK_CSCON_STATUES;
                Send_AT_to_Ext("\r\nno reply\r\n"); 
            }

            break;
        }

        case CHECK_CSCON_STATUES:
        {
            at_ret = AT_Send_And_Get_Rsp("AT+CSCON?\r\n",10, "+CSCON:", "%d,%d",&mode,&statues);
            if(at_ret != XY_OK)
            {       
                g_resend_time = 0;
                send_statues = 0;
                meter_data_send_step = HANDLE_CP_STOP;
                break;
            }

            if(statues == 0)
            {
                g_resend_time = 0;
                send_statues = 0;
                meter_data_send_step = CHECK_SLEEP_STATUES;
            }
            else
            {
                g_resend_time++;
                send_statues = CHECK_CSCON_STATUES;
                meter_data_send_step = WAIT_ONLY;
            }

            if(g_resend_time > MAX_RESEND_TIME)
            {
                g_wait_anwser_tick = 0;
                g_resend_time = 0;
                send_statues = 0;
                cp_is_not_abnormal = 1;
                meter_data_send_step = HANDLE_CP_STOP;
                send_statues = 0;
            }

            break;
        }

        case CHECK_SLEEP_STATUES:
        {
            at_ret = AT_Send_And_Get_Rsp("AT+NPSMR?\r\n",10, "+NPSMR:1,", "%d",&mode);
            if(at_ret != XY_OK)
            {
                send_statues = 0;
                g_resend_time = 0;
                meter_data_send_step = HANDLE_CP_STOP;
                break;
            }

            if(mode == 1)
            {
                g_resend_time = 0;
                send_statues = 0;
                Clear_Cloud_State();
                clear_event(EVENT_CLOUD_SEND);
                // Stop_CP(0);
                Send_AT_to_Ext("\r\nsend end\r\n"); 
            }
            else
            {
                g_resend_time++;
                send_statues = CHECK_SLEEP_STATUES;
                meter_data_send_step = WAIT_ONLY;

            }

            if(g_resend_time > MAX_RESEND_TIME)
            { 
                g_wait_anwser_tick = 0;
                g_resend_time = 0;
                send_statues = 0;
                cp_is_not_abnormal = 1;
                meter_data_send_step = HANDLE_CP_STOP;
            }

            break;
        }

        case WAIT_ONLY:
        {            
            if(g_wait_anwser_tick == 0)
            {
                g_wait_anwser_tick = Get_Tick();
            }
            
            if(Check_Ms_Timeout(g_wait_anwser_tick,(3*1000)))
            {
                meter_data_send_step = send_statues;
                g_wait_anwser_tick = 0;
            }

            break;
        }

        case HANDLE_CP_STOP:
        {
            if(cp_is_not_abnormal == 1)
            {
                cp_is_not_abnormal = 0;
                AT_Send_And_Get_Rsp("AT+CFUN=0\r\n",10, NULL, NULL);
            }
            Stop_CP(0);
            Clear_Cloud_State();
            clear_event(EVENT_CLOUD_SEND);
            Send_AT_to_Ext("\r\nsend end,stop cp\r\n"); 
            send_statues = 0;
        }

        default:
            break;
    }

    return 1;		
	
}

/*该函数每次唤醒都执行，需要放在RAM上执行，否则运行时间过长影响功耗*/
__RAM_FUNC At_status_type Send_Data_By_Cloud_WM(void)
{
	At_status_type at_ret = XY_OK;
	
	//识别是否需要远程发送数据，例如RTC超时，或者flash中缓存的数据超过阈值
	if(is_event_set(EVENT_CLOUD_SEND))
	{
		at_ret = DO_Send_Data_By_Cloud_WM();
	}

	return at_ret;
}

