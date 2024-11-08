#include "xy_system.h"
#include "prcm.h"
#include "utc_watchdog.h"
#include "xy_fota.h"
#include "xy_at_api.h"
#include "at_process.h"

void xy_OTA_flash_info(uint32_t *addr, uint32_t *len)
{
	
    *addr = HWREG(BAK_MEM_FOTA_FLASH_BASE);

	/*包含工作态时借给DUMP/debug/易变NV使用的空间*/
    *len = (uint32_t)(ARM_FLASH_BASE_LEN - (*addr - ARM_FLASH_BASE_ADDR));

}

/*该函数用于供用户在处理紧急事件时，查询是否正在执行FOTA。AP系统后台发现正在执行FOTA后，会自动启动CP核，以完成FOTA流程*/
bool Is_Fota_Running(void)
{
	if(HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) != 0)
		return true;
	else
		return false;
}

/*供用户在某些特殊事务期间，通知CP核禁止FOTA升级*/
int Do_Fota_onoff(int permit) 
{
	if(permit == 0)
		return at_uart_write(0,(uint8_t *)"AT+FOTACTR=0\r\n", strlen("AT+FOTACTR=0\r\n"));
	else if(permit == 1)
		return at_uart_write(0,(uint8_t *)"AT+FOTACTR=1\r\n", strlen("AT+FOTACTR=1\r\n"));
	else
		xy_assert(0);
    return 0;
}

/**
 * @brief AP核用户注册的执行FOTA期间的私有回调，该回调函数在AP核核间消息中断中处理。
 */
pFunType_u32 p_Cp_FotaProc = NULL;
__FLASH_FUNC void Fota_Proc_Hook_Regist(pFunType_u32 pfun)
{
	p_Cp_FotaProc = pfun;
	mark_dyn_addr(&p_Cp_FotaProc);
}

/*state==1，表示正在FOTA，CP会擦写FLASH且会重启，AP用户需要执行私有动作，防止FOTA期间用户数据丢失。state==0，则表示完成FOTA动作，后续不再有FLASH擦写操作*/
__OPENCPU_FUNC void CP_Fota_proc(uint32_t state)
{
	if(state != 0)
	{	
		xy_printf("FOTA doing!\r\n");	
	
	}
	else if(state==0)
	{
		xy_printf("FOTA  NULL!\r\n");	
	}
	
    if(p_Cp_FotaProc != NULL)
    {
        p_Cp_FotaProc(state);
    }
}

/*公有云触发的FOTA流程，升级完成后需要联云上报升级结果。OPENCPU产品形态在AP初始化后台启动CP核，以完成FOTA结果上报。但考虑到部分客户CP核默认不联网，需要在此函数中进行CP核的联网操控*/
__OPENCPU_FUNC void Read_AT_List_And_Check_Result(char *atstr, int len, const char* compare_str, uint32_t timeout_ms)
{
	uint32_t tickstart = Get_Tick();

	do{
        if(at_uart_read(0, (uint8_t *)atstr, len))
        {
			if(strstr(atstr, compare_str) != NULL)
			{
                memset(atstr, 0, len);
                break;
			}
        }
        
        if(Check_Ms_Timeout(tickstart, timeout_ms))  
		{
#if XY_DUMP
            xy_assert(0);
#else
			break;
#endif
        }
    }while(1);
}

/*当CP核默认上电软关机态才需通过该函数触发attach*/
__OPENCPU_FUNC void Fota_Succ_Report_Ctrl(void)
{
#if XY_AT_CTL	
	char *at_rsp = NULL;
	At_status_type at_ret = -1;

	at_rsp = xy_malloc(648);

	at_ret = AT_Send_And_Get_Rsp("AT+NCONFIG?\r\n",20, "+NCONFIG:", "%a", (char *)at_rsp);//查询客户的联网设置
	
	if(at_ret != XY_OK)
	{
		xy_assert(0);
	}

	if(strstr(at_rsp, "+NCONFIG:AUTOCONNECT,TRUE") != NULL)//客户为自动联网
	{
		xy_free(at_rsp);
		return;
	}
	else if(strstr(at_rsp, "+NCONFIG:AUTOCONNECT,FALSE") != NULL)//客户是手动联网，须触发联网，才能进行公有云的FOTA重启版本确认
	{
		xy_free(at_rsp);

		at_ret = AT_Send_And_Get_Rsp("AT+CFUN=1\r\n",20, NULL, NULL);
		if(at_ret != XY_OK)
		{
			xy_assert(0);
		}
		at_ret = AT_Send_And_Get_Rsp("AT+CGATT=1\r\n",10, NULL, NULL);
		if(at_ret != XY_OK)
		{
			xy_assert(0);
		}
	}
	else
	{
		xy_assert(0);		
	}
#else
	char *at_rsp = NULL;
	uint32_t tickstart = 0;

	at_rsp = xy_malloc(648);

	Read_AT_List_And_Check_Result(at_rsp,256,"REBOOT_CAUSE",10000);//排除上电URC携带的OK对后续AT命令判别的影响

    at_uart_write(0,(uint8_t *)"AT+NCONFIG?\r\n",strlen("AT+NCONFIG?\r\n"));

	tickstart = Get_Tick();

	do{
        if(at_uart_read(0, (uint8_t *)at_rsp, 648))
        {
            break;
        }
        
        if(Check_Ms_Timeout(tickstart, 10000))  
		{
            xy_assert(0);
        }
    }while(1);

	if(strstr(at_rsp, "+NCONFIG:AUTOCONNECT,TRUE") != NULL)//客户为自动联网
	{
		xy_free(at_rsp);
		return;
	}
	else if(strstr(at_rsp, "+NCONFIG:AUTOCONNECT,FALSE") != NULL)//客户是手动联网，须触发联网，才能进行公有云的FOTA重启版本确认
	{
		at_uart_write(0,(uint8_t *)"AT+CFUN=1\r\n",strlen("AT+CFUN=1\r\n"));

		Read_AT_List_And_Check_Result(at_rsp,256,"OK\r\n",10000);
		
		at_uart_write(0,(uint8_t *)"AT+CGATT=1\r\n",strlen("AT+CGATT=1\r\n"));

		Read_AT_List_And_Check_Result(at_rsp,256,"OK\r\n",5000);

		xy_free(at_rsp);
	}
	else
	{
		xy_assert(0);
	}
#endif
}

/*OPENCPU形态，升级完成后，AP初始化阶段，后台启动CP后，执行FOTA的结果云上报，完成后再起AP大版本*/
__OPENCPU_FUNC void Fota_Reboot_Init(void)
{
#if FOTA_SUCC_REPORT  

	uint32_t tickstart = 0;
	/*检测是否为升级完成后的第一次大版本启动*/
	if(Is_Fota_Running() && Get_Boot_Reason()==SOFT_RESET && Get_Boot_Sub_Reason()==SOFT_RB_BY_FOTA)
	{
		UTC_WDT_Init(WAIT_CLOUD_REPORT+5);

		Boot_CP(WAIT_CP_BOOT_MS);

		/*部分客户关闭了CP的自动联网，需通过此函数确认客户设置，使CP正常attach，才能进行云确认*/
		Fota_Succ_Report_Ctrl();
		
		tickstart = Get_Tick();
		/*超时等待CP完成公有云自注册，防止AP核用户程序干扰云上报流程*/
		do{
			if(Check_Ms_Timeout(tickstart, WAIT_CLOUD_REPORT*1000))  
			{
				break;
			}
    	}while(Is_Fota_Running());
			
		HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 0;
	}
	
#endif
}