/**
* @file      fota_by_local.c
* @brief 	 该源文件作为本地差分升级的DEMO，使用芯翼平台提供的本地升级AT命令和差分包配套工具来实现红外等媒介的本地差分升级，详情参阅：《芯翼XY1200&XY2100产品FOTA开发指导》。
* @warning   目前示例使用的是芯翼提供的AT命令和AT收发接口，如果客户变更相关接口或命令，请自行修改！
* @note      在本地FOTA升级期间，由于人为介入，为了防止看门狗超时触发复位，可自行调用Disable_All_WDT来关闭看门狗
*/

#include "xy_printf.h"
#include <stdio.h>
#include "at_process.h"
#include "xy_cp.h"

// 单条AT命令的最大长度，用户根据产品所用的AT命令自行定义
#define RCVED_BUFFER_LEN           (1024 * 3)

// 外部发送来的AT命令接收缓存
uint8_t g_at_RevBuf[RCVED_BUFFER_LEN] = {0};

// 接收缓存的实时长度
uint16_t g_rcved_len  = 0;


/*收到FOTA指示后，AP核用户执行的特别动作，例如关闭外部事务中断等，以防止升级期间产品运行异常。返回true表示容许继续升级，返回false表示禁止此次升级*/
bool proc_for_FOTA()
{

	return true;
}

/*IPC核间通道调用，单字节处理CP核发送来的AT命令。该接口由用户实现*/
void send_char_to_ext(uint8_t single_char)
{
	(void)single_char;
	/*将从CP核收到的AT命令发送给外部，如红外等媒介*/

}

/*将字符串通过红外等媒介发送给外部。该接口由用户实现*/
void send_atstr_to_ext(uint8_t *data)
{
	(void)data;

}

/*接收红外等媒介传递来的数据，识别为完整的一条AT命令后，发送给CP核*/
void proc_atstr_from_ext(char *atstr,int len)
{
	memcpy(g_at_RevBuf+g_rcved_len,atstr,len);
	g_rcved_len += len;

	if(g_rcved_len >= RCVED_BUFFER_LEN)
		xy_assert(0);

	/*接收到AT命令结束标识，发送给CP核*/
	if(g_at_RevBuf[g_rcved_len-1]=='\r' || g_at_RevBuf[g_rcved_len-2]=='\r' || g_at_RevBuf[g_rcved_len-3]=='\r')
	{
		if(strcmp((void *)g_at_RevBuf,"AT+NFWUPD=0")==0)
		{
			if(proc_for_FOTA() != true)
			{
				send_atstr_to_ext((uint8_t *)"ERROR\r\n");
				return;
			}
		}
		
		Boot_CP(WAIT_CP_BOOT_MS);
		at_uart_write(0,g_at_RevBuf,g_rcved_len);
		IPC_Set_User_Handle(send_char_to_ext);
		g_rcved_len = 0;
	}	
}



