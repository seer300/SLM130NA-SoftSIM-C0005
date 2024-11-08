#include "at_process.h"
#include "at_cmd_regist.h"
#include "at_hardware_cmd.h"
#include "hw_types.h"
#include "xy_system.h"
#include "at_uart.h"
#include "master.h"
#include "slave.h"
#include "xy_flash.h"
#include "xy_cp.h"
#include "sys_mem.h"
#include "xy_fota.h"
#include "sys_ipc.h"


#define AT_DEEPSLEEP_LOCK USER_DSLEEP_LOCK1


__WEAK int at_DEMOCFG_req(char *param,char **rsp_cmd)
{
	UNUSED_ARG(param);
	UNUSED_ARG(rsp_cmd);
	return 0;
}

//AP核AT命令注册链表
const at_cmd_t g_AT_cmd_list[] = {

#if MODULE_VER
	{"NATSPEED", at_NATSPEED_req},
#if !VER_260Y
	{"IPR", at_IPR_req},
#endif
	{"UARTSET", at_UARTSET_req},
	{"RESETCTL", at_RESETCTL_req},
	{"XYCNNT",at_XYCNNT_req},

	{"CMRB", at_RB_req},
	{"NRB", at_RB_req},
	{"RESET", at_RESET_req},


#if VER_BC25
	{"QRST", at_QRST_req},
	{"QPOWD", at_QPOWD_req},
#endif
#if VER_260Y
	{"QRST", at_QRST_req2},
	{"IPR", at_IPR_req2},
#endif

#endif

	{"ASSERT", at_ASSERT_req},
    {"SOCID", at_SOCID_req},
	{"REGTEST", at_REGTEST_req},
	{"APMEMSTATS",at_APMEMSTATUS_req},
	{"LOCK", at_LOCK_req},

	{"DEMOCFG", at_DEMOCFG_req},
	{"BOOTCP", at_BOOTCP_req},

#if XY_DEBUG
	{"FLASHTEST",at_FLASHTEST_req},//AT+FLASHTEST =<擦写总次数>,<擦写大小>,<数据>
#endif

#if OPENCPU_TEST
    // OPENCPU_TEST宏默认为0，如要编译opencpu测试命令，请在define.cmake中手动修改OPENCPU_TEST宏
    {"APTEST", at_APTEST_req},
#endif

#if (DRIVER_TEST == 1)
	// DRIVER_TEST宏默认为0，如要编译驱动测试命令，请在define.cmake中手动修改DRIVER_TEST宏
	{"MASTER", at_MASTER_req},
#elif (DRIVER_TEST == 2)
	{"SLAVE", at_SLAVE_req},
#endif


	/*仅供客户演示使用，未达商用*/
	// {"SVDCFG", at_SVDCFG_req},
	{0, 0}};

uint8_t g_cmd_type = AT_CMD_INVALID;	//at命令类型，参考@at_cmd_type_t



/*AT+NRB/AT+CMRB*/
int at_RB_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(at_buf);
	UNUSED_ARG(prsp_cmd);

	if(g_cmd_type == AT_CMD_REQ || g_cmd_type == AT_CMD_ACTIVE)
	{
		Send_AT_to_Ext("\r\nREBOOTING\r\n");
		xy_Soft_Reset(SOFT_RB_BY_NRB);
	}
	else
		return  (XY_ERR_PARAM_INVALID);
	return XY_OK;
}


/*AT+RESET*/
int at_RESET_req(char *at_buf, char **prsp_cmd)
{
	UNUSED_ARG(at_buf);
	UNUSED_ARG(prsp_cmd);

	if (g_cmd_type == AT_CMD_ACTIVE)
	{
		Send_AT_to_Ext("\r\nRESETING\r\n");
		xy_Soft_Reset(SOFT_RB_BY_RESET);
	}
	else
		return  (XY_ERR_PARAM_INVALID);

	return XY_OK;
}


/*AT+LOCK=<type>,<disable>*/
int at_LOCK_req(char *param,char **rsp_cmd)
{
	UNUSED_ARG(rsp_cmd);
	if (g_cmd_type == AT_CMD_REQ) // 设置类
	{
		int lock_type = 0;
		int disable = 0;

		if (at_parse_param("%d,%d", param, &lock_type,&disable) != XY_OK)
			return XY_ERR_PARAM_INVALID;

		if (lock_type==0 && disable==0)
		{
			LPM_LOCK(AT_DEEPSLEEP_LOCK);
		}
		else if (lock_type==0 && disable==1)
		{
			LPM_UNLOCK(AT_DEEPSLEEP_LOCK);
		}
#if MODULE_VER		
		else if (lock_type==1 && disable==0)
		{
			LPM_LOCK(STANDBY_DEFAULT);
		}
		else if (lock_type==1 && disable==1)
		{
			LPM_UNLOCK(STANDBY_DEFAULT);
		}
		else if (lock_type==2 && disable==0)
		{
			LPM_LOCK(WFI_DEFAULT);
		}
		else if (lock_type==2 && disable==1)
		{
			LPM_UNLOCK(WFI_DEFAULT);
		}
#endif
		else
		{
			return XY_ERR_PARAM_INVALID;
		}

		return XY_OK;
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
}

/*AT+BOOTCP=0 或 AT+BOOTCP=1*/
int at_BOOTCP_req(char *param,char **rsp_cmd)
{
	int cmd = 0;
	UNUSED_ARG(rsp_cmd);
	
	if (g_cmd_type == AT_CMD_REQ)
	{
		at_parse_param("%d,",param,&cmd);

#if MODULE_VER==0
		if(cmd == 1)
			Boot_CP(WAIT_CP_BOOT_MS);
		else
			Stop_CP(0);
#endif

		return XY_OK;
	}
	else
		return XY_ERR_PARAM_INVALID;
}

//AT+ASSERT，辅助定位调试指令
int at_ASSERT_req(char *param,char **rsp_cmd)
{
	int cmd = 0;
	UNUSED_ARG(rsp_cmd);
	
 	if (g_cmd_type == AT_CMD_ACTIVE)
	{
		xy_assert(0);
	}
	else if (g_cmd_type == AT_CMD_REQ)
	{
		at_parse_param("%d,",param,&cmd);
		
		if(cmd == 0)
		{
			xy_assert(0);
		}
		/*人为构造hardfault*/
		else
		{
			*(int *)USER_FLASH_BASE = 1;//非法访问flash地址触发hardfault
		}
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}

extern void GetApHeapInfo(char *info);

int at_APMEMSTATUS_req(char *param, char **rsp_cmd)
{
	char cmd[10] = {0};
	if (at_parse_param("%10s", param, cmd) != XY_OK)
	{
		return XY_ERR_PARAM_INVALID;			
	}

	//AT+APMEMSTATS=APPSMEM
	if (!strcmp(cmd, "APPSMEM"))
	{
		*rsp_cmd = xy_malloc(400);
		GetApHeapInfo(*rsp_cmd);
	}
	else if(!strcmp(cmd, "ALLMEM"))
	{
		extern uint32_t _Flash_Total;
		extern uint32_t _Flash_Remain;
        (void)_Flash_Remain;
        extern uint32_t _Ram_Total;
		extern uint32_t _Ram_Used;
		extern uint32_t _Ram_Remain;
        __attribute__((unused))  extern uint32_t _Heap_Limit;

		*rsp_cmd = xy_malloc(400);
		uint16_t str_len = 0;
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Total Flash Space:%p\r\n", &_Flash_Total);
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Flash Used:%p\r\n", (void *)(HWREG(BAK_MEM_FOTA_FLASH_BASE) - ARM_FLASH_BASE_ADDR));
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Flash Remaining:%p\r\n", (void *)(ARM_FLASH_BASE_LEN + ARM_FLASH_BASE_ADDR - HWREG(BAK_MEM_FOTA_FLASH_BASE)));
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Total Ram Space:%p\r\n", &_Ram_Total);
#if DYN_LOAD
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Ram Used:%p\r\n", (void *)((uint32_t)&_Ram_Used + (uint32_t)&_Heap_Limit - HEAP_END));
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Ram Remaining:%p\r\n", (void *)((uint32_t)&_Ram_Remain + HEAP_END - (uint32_t)&_Heap_Limit));
#else
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Ram Used:%p\r\n", &_Ram_Used);
		str_len += sprintf(*rsp_cmd + str_len, "\r\n+NUESTATS:ALLMEM,Ram Remaining:%p\r\n", &_Ram_Remain);
#endif
		strcat(*rsp_cmd, "\r\nOK\r\n");
	}
	//AT+APMEMSTATS=HEAPINFO
	else if(!strcmp(cmd, "HEAPINFO"))
	{
		unsigned int TotalHeapsize;
		unsigned int FreeRemainHeapsize;
		unsigned int MinimumEverFreeRemaining;
		*rsp_cmd = xy_malloc(300);
		sprintf(*rsp_cmd, "\r\n+AP_HEAP_INFO\r\n");
		TotalHeapsize = xPorGetTotalHeapSize();
		FreeRemainHeapsize = xPortGetFreeHeapSize();
		MinimumEverFreeRemaining = xPortGetMinimumEverFreeHeapSize();
		sprintf(*rsp_cmd + strlen(*rsp_cmd), "\r\n+APHeapTotalSize:%d, \t FreeRemainHeapSize:%d, \t MinimumEverHeapSize:%d\r\n",
		TotalHeapsize, FreeRemainHeapsize, MinimumEverFreeRemaining);

	}
	//AT+APMEMSTATS=FOTA
	else if (!strcmp(cmd, "FOTA"))
	{
		uint32_t addr,len;
		
		*rsp_cmd = xy_malloc(400);
		xy_OTA_flash_info(&addr,&len);
		
		sprintf(*rsp_cmd, "+FOTA: START %lX,LEN %lu",addr,len);
	}

	else
	{
		return XY_ERR_PARAM_INVALID;	
	}
	
	return XY_OK;
}

/*AT+SOCID，辅助调试指令*/
int at_SOCID_req(char *param,char **rsp_cmd)
{
    UNUSED_ARG(param);
    UNUSED_ARG(rsp_cmd);

    char rsp[16];
    sprintf(rsp, "%d\r\n", HWREG(BAK_MEM_OTP_SOC_ID_BASE));
    Send_AT_to_Ext(rsp);

    return XY_OK;
}

/*AT+REGTEST，辅助调试指令，读，写，清楚寄存器*/
int at_REGTEST_req(char *param,char **rsp_cmd)
{
	unsigned char cmd[16] = {0};
	unsigned char regAddrStr[16] = {0};	
	unsigned char hexStr[16] = {0};	
	unsigned int regAddr = 0;
	unsigned int value = 0;

	if (at_parse_param("%15s,%15s,%15s", param, cmd, regAddrStr, hexStr) != XY_OK)
		return XY_ERR_PARAM_INVALID;

	//16进制字符串地址（带0x前缀）转化为整型
	hexstr2int((char *)regAddrStr, (int *)&regAddr);
	hexstr2int((char *)hexStr, (int *)&value);

	/*AT+REGTEST=GET,regAddr*/
	if(!strcmp((const char *)(cmd), "GET"))
	{
		*rsp_cmd = xy_malloc(100);
		sprintf(*rsp_cmd, "\r\n");
		sprintf(*rsp_cmd + strlen(*rsp_cmd), "0x%X %s", (int)HWREG(regAddr),"");

	}
	/*AT+REGTEST=SET,regAddr,value*/
	else if(!strcmp((const char *)(cmd), "SET"))
	{
		HWREG(regAddr) = value;
	}
	/*AT+REGTEST=CLEAR,regAddr*/
	else if(!strcmp((const char *)(cmd), "CLEAR"))
	{
		HWREG(regAddr) = 0x0;
	}
	/*AT+REGTEST=UP*/
	else if(!strcmp((const char *)(cmd), "UP"))
	{
		extern void print_up_dbg_info();
		print_up_dbg_info();
	}
	/*AT+REGTEST=DL*/
	else if(!strcmp((const char *)(cmd), "DL"))
	{
		extern void print_ver_download_info();
		print_ver_download_info();
	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}

	return XY_OK;
}


uint8_t get_at_cmd_type(char *buf)
{
	if(*buf == '\r')
		return AT_CMD_ACTIVE;
	else if(*buf == '?' && *(buf + 1) == '\r')
		return AT_CMD_QUERY;
	else if(*buf == '=')
	{
		if(*(buf + 1) != '?')
			return AT_CMD_REQ;
		else if(*(buf + 2) == '\r')
			return AT_CMD_TEST;
	}
	return AT_CMD_INVALID;
}

char *at_ok_build()
{
	char *at_str;
	at_str = xy_malloc(7);
	snprintf(at_str, 7, "\r\nOK\r\n");
	return at_str;
}

char *at_err_build(uint16_t err_no)
{
	char *at_str = NULL;
	xy_assert(err_no != 0);
	at_str = xy_malloc(64);

	if (get_cmee_mode() == 1)
	{
		sprintf(at_str, "\r\n+CME ERROR:%d\r\n", err_no);
	}
	else
	{
		sprintf(at_str, "\r\nERROR\r\n");
	}
	return at_str;
}

#if MODULE_VER
/**
 * @brief AP的AT命令处理完成后，需要通知CP进行延迟锁的刷新
 */
static void ap_delaywork_active()
{
    IPC_Message pMsg = {0};
    pMsg.id = ICM_APAT_DELAY;
    pMsg.len = sizeof(IPC_MessageID);//无实际作用仅匹配CP流程
    if (IPC_WriteMessage(&pMsg) < 0)//通过核间通知CP刷新延迟锁
    {
        xy_printf("AP AT DELAY LOCK FAIL!!!");
    }
}
#endif

//判断串口输入的字符串是否为AP核注册的AT命令，若是则执行相应的回调函数，否则返回0转发给CP核处理
int Match_AT_Cmd(char *buf)
{
	at_cmd_t *p_at_list = (at_cmd_t *)g_AT_cmd_list;
	char *param = NULL;
	xy_printf("recv at from lpuart:%s", buf);
	while(p_at_list->at_prefix != 0)
	{
		if((param = at_prefix_strstr(buf, p_at_list->at_prefix)) != NULL)
		{
			char *rsp_cmd = NULL;
			int ret = XY_OK;
			g_cmd_type = get_at_cmd_type(param);

			//获取类型失败，转发给CP核处理
			if (g_cmd_type == AT_CMD_INVALID)
			{
				return 0;
			}
			else
			{
#if MODULE_VER
       			ap_delaywork_active();
#endif
				if(g_cmd_type == AT_CMD_REQ)
					param++;
				ret = p_at_list->proc(param, &rsp_cmd);
			}
			if (rsp_cmd == NULL)
			{
				if (ret == XY_OK)
					rsp_cmd = at_ok_build();
				else if (ret == XY_FORWARD) //转给CP处理
                    return 0;
				else
					rsp_cmd = at_err_build(ret);
			}
			/*具体AT应答字符串尾部不加\r\n时，由框架自动添加前缀和OK结果码,AT+QBTWRITE进入透传模式，上报"\r\n>",后面不可带OK*/
			else if(strstr(rsp_cmd+strlen(rsp_cmd)-2, "\r\n") == NULL && *(rsp_cmd+strlen(rsp_cmd)-1) != '>')
			{
				char *temp = xy_malloc(strlen(rsp_cmd)+15+strlen(p_at_list->at_prefix));
				/*有前缀，仅需添加尾部OK。通常用于前缀不规则的URC上报，如REBOOTING上报*/
				if(*rsp_cmd == '\r')
				{
						sprintf(temp,"%s\r\n\r\nOK\r\n",rsp_cmd);
				}
				/*添加头部\r\n和尾部OK，通常用于URC冒号后面不带空格的特殊URC*/
				else if(*rsp_cmd == '+')
				{
					sprintf(temp,"\r\n%s\r\n\r\nOK\r\n",rsp_cmd);
				}
				/*添加前缀和尾部OK，用于标准的URC上报*/
				else 
				{
					sprintf(temp,"\r\n+%s: %s\r\n\r\nOK\r\n",p_at_list->at_prefix,rsp_cmd);
				}
				xy_free(rsp_cmd);
				rsp_cmd = temp;
			}
			
			Send_AT_to_Ext(rsp_cmd);
			xy_free(rsp_cmd);
			if (ret == XY_FORWARD)
            {
                return 0;
            }
			// AT+NATSPEED回复OK后，开始尝试切换到新波特率
#if VER_BC95
			if(g_baudrate_flag == 1)
			{
				extern void natspeed_change_hook();
				natspeed_change_hook();
			}
#elif VER_BC25
			if (g_baudrate_flag == 1)
			{
#if (AT_LPUART == 1)
				extern void set_and_save_baudrate(baud_rate);
				set_and_save_baudrate(g_at_lpuart.Init.BaudRate);
#endif
				g_baudrate_flag = 0;
			}
#elif VER_260Y
			extern uint8_t g_setipr_flag;
			if (g_setipr_flag == 1)
			{
#if (AT_LPUART == 1)
				extern void set_ipr(uint32_t baud_rate);
				set_ipr(g_at_lpuart.Init.BaudRate);
#endif			
				g_setipr_flag = 0;
			}
#endif
			return 1;
		}
		else
			p_at_list++;
	}
	return 0;
}


