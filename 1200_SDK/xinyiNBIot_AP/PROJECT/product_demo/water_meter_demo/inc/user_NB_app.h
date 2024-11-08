#pragma once

#define MODE_PSM                (0)//开启时走PSM，等待CP核正常进入睡眠;关闭时发送成功直接stopcp

#define MAX_RESEND_TIME         (15)
#define CDP_RESPONSE_TIMEOUT    (96) //CDP云通信超时时间


void Trigger_Send_Proc(void);
At_status_type Set_Water_Meter_User_Config(void);
At_status_type Send_Data_By_Cloud_WM(void);
