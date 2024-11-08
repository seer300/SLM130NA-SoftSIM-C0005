#include "at_CP_api.h"
#include "at_uart.h"
#include "xy_event.h"
#include "xy_timer.h"
#include "xy_cp.h"
#include "basic_config.h"


/*AT+APTEST=CP,<val>*/
uint32_t CP_TEST(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4)
{
	char rsp[50] = {0};
	UNUSED_ARG(val3);
	UNUSED_ARG(val4);

	/*AT+APTEST=CP,0  加载CP核*/
	if(val1 == 0)
	{
        Boot_CP(WAIT_CP_BOOT_MS);
		Send_AT_to_Ext("\r\nBoot_CP\r\n");
	}
	/*AT+APTEST=CP,1  检测CP核是否加载成功*/
	else if(val1 == 1)
	{
		sprintf(rsp, "\r\nCP_Is_Alive:%d\r\n", CP_Is_Alive());
		Send_AT_to_Ext(rsp);
	}
	/*AT+APTEST=CP,2,<timeout>  强行停CP核，timeout=0表示由AP核立即断电CP核*/
	else if(val1 == 2)
	{
		Stop_CP(val2);
		Send_AT_to_Ext("\r\nStop_CP\r\n");
	}
	
	/*AT+APTEST=CP,3  由AP核本地复位CP核，内部执行下电和加载CP核的动作*/
	else if(val1 == 3)
	{
		xy_CP_Reboot();
	}

	/*AT+APTEST=CP,4  触发RAI，待下次深睡唤醒后，AP核再加载CP核，CP核无需attach，2秒内就可IP通信*/
	else if(val1 == 4)
	{
		Send_Rai();
		Send_AT_to_Ext("\r\nSend_Rai\r\n");
	}
	/*AT+APTEST=CP,5  检测CP核的TCPIP网路是否已通*/
	else if(val1 == 5)
	{
		if(xy_wait_tcpip_ok(120) == XY_OK)
		{
			Send_AT_to_Ext("\r\nxy_wait_tcpip_ok\r\n");
		}
        else
		{
			Send_AT_to_Ext("\r\nxy_wait_tcpip_ok fail\r\n");
		}
	}
	/*AT+APTEST=CP,6  通过AT+BOOTCP=1来启动CP核，内部执行  Boot_CP(WAIT_CP_BOOT_MS)*/
	else if(val1 == 6)
	{
		Send_AT_Req("AT+BOOTCP=1\r\n", 0);

        Send_AT_to_Ext("\r\ndo AT+BOOTCP=1\r\n");
	}
    /*AT+APTEST=CP,7  AP核触发CPOF命令，以下电CP核，内部执行stop_CP(0);可以再通过AT+BOOTCP来启动CP核*/
    else if(val1 == 7)
    {
        Send_AT_Req("AT+CPOF\r\n", 0);

        Send_AT_to_Ext("\r\ndo AT+CPOF\r\n");
    }
	/*AT+APTEST=CP,8  AP核触发NRB命令，以重启CP核*/  
	else if(val1 == 8)
    {
        Send_AT_Req("AT+NRB\r\n", 0);

        Send_AT_to_Ext("\r\ndo AT+NRB\r\n");
    }
	/*AT+APTEST=CP,9  AP核触发AT+XYRAI命令，以快速释放链接。空闲态下也会触发空RAI包的交互*/  
	else if(val1 == 9)
    {
        Send_AT_Req("AT+XYRAI\r\n", 0);

        Send_AT_to_Ext("\r\nDO AT+XYRAI\r\n");
    }
	/*AT+APTEST=CP,10  AP核触发AT+RAI，由PS执行快速链接释放。空闲态下啥事不干*/  
	else if(val1 == 10)
    {
        Send_AT_Req("AT+RAI=1\r\n", 0);

        Send_AT_to_Ext("\r\nDO AT+RAI=1\r\n");
    }
	else
	{
		return XY_ERR_PARAM_INVALID;
	}

	return XY_OK;
}


