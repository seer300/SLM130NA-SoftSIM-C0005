/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_xy_cmd.h"
#include "xy_at_api.h"
#include "at_system_cmd.h"
#include "xy_ps_api.h"
#include "flash_vendor.h"
#include "hw_memmap.h"
#include "ipc_msg.h"
#include "low_power.h"
#include "oss_nv.h"
#include "xy_flash.h"
#include "xy_utils.h"
#include "xy_utils_hook.h"
#include "xy_system.h"
#include "cloud_utils.h"
#include "net_app_resume.h"
#include "at_uart.h"
#include "dump_flash.h"
#include "xy_fs.h"
#include "ps_netif_api.h"
#include "osAssitantUtils.h"
#include "xy_rtc_api.h"
#include "diag_transmit_port.h"
#include "mpu_protect.h"
#include "atc_ps_def.h"
#include "atc_ps.h"

#if TELECOM_VER
#include "cdp_backup.h"
#include "at_cdp.h"
#endif
#if MOBILE_VER && XY_DM
#include "cmcc_dm.h"
#endif

#if XY_DM
#include "tele_uni_dm.h"
#endif

#include "softsim_adapt.h"



/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/


// extern int rate_test;
extern volatile T_LPM_INFO Ps_Lpminfo;
extern volatile T_LPM_INFO Ps_Lpminfo, Phy_Lpminfo;
extern void ps_sleep_state_check(unsigned long *param1,unsigned long *param2);
// extern osThreadId_t g_xylog_TskHandle;
// extern osThreadId_t g_rxlog_TskHandle;

/*******************************************************************************
 *						Local function implementations 					       *
 ******************************************************************************/
extern void diag_filter_set_level(uint32_t val);
extern void diag_filter_set_src(uint32_t val);

extern uint32_t g_xy_mem_alloc_loose_cnt;
extern uint32_t g_xy_mem_alloc_align_loose_cnt;
int simple_set_val(char *param,int val)
{
	if (!strcmp(param, "DEEPSLEEP"))
	{
		g_softap_fac_nv->deepsleep_enable = val;
		SAVE_FAC_PARAM(deepsleep_enable);
	}
	else if(!strcmp(param,"OFFATWAKUP"))  
	{
		g_softap_fac_nv->off_wakeupRXD = (uint8_t)val;
		SAVE_FAC_PARAM(off_wakeupRXD);
	}
	else if (!strcmp(param, "VSIM"))
	{
		g_softap_fac_nv->sim_type = val;
		SAVE_FAC_PARAM(sim_type);
	}
	else if (!strcmp(param, "STANDBY"))
	{
		g_softap_fac_nv->lpm_standby_enable = val;
		SAVE_FAC_PARAM(lpm_standby_enable);
	}
#if VER_BC25
	else if (!strcmp(param, "QSCLK"))
	{
		g_softap_fac_nv->qsclk_mode = val;
		SAVE_FAC_PARAM(qsclk_mode);
	}
#endif
	else if (!strcmp(param, "MPU"))
	{
		if(val == 0)
			flash_interface_protect_disable();
		else
			flash_interface_protect_enable();
	}

	else if (!strcmp(param, "LOG"))
	{
		g_softap_fac_nv->open_log = val;
		SAVE_FAC_PARAM(open_log);
	}
	else if (!strcmp(param, "LOGLEVEL"))
	{					
		diag_filter_set_level(val);
	}
	else if (!strcmp(param, "LOGSRC"))
	{					
		diag_filter_set_src(val);
	}
	else if (!strcmp(param, "LOGSIZE"))
	{					
		g_softap_fac_nv->log_size = val;
		SAVE_FAC_PARAM(log_size);
	}
	else if (!strcmp(param, "MEMDEBUG"))
	{					
		g_softap_fac_nv->mem_debug = val;
		SAVE_FAC_PARAM(mem_debug);
	}
	else if (!strcmp(param, "TEST"))
	{
		g_softap_fac_nv->test = val;
		SAVE_FAC_PARAM(test);
	}
	else if (!strcmp(param, "MTU"))
	{
		g_softap_fac_nv->mtu = val;
		SAVE_FAC_PARAM(mtu);
	}
	else if (!strcmp(param, "IO"))
	{					
		g_softap_fac_nv->io_output_vol = val;
		SAVE_FAC_PARAM(io_output_vol);
	}
	else if (!strcmp(param, "WFI"))
	{
		g_softap_fac_nv->wfi_enable = val;
		SAVE_FAC_PARAM(wfi_enable);
	}
	else if (!strcmp(param, "FRAMECAL"))
	{
		g_softap_fac_nv->frame_cal = val;
		SAVE_FAC_PARAM(frame_cal);
	}
	
	else if (!strcmp(param, "CLOSEDRXSLEEP"))
	{
		g_softap_fac_nv->close_drx_dsleep = val;
		SAVE_FAC_PARAM(close_drx_dsleep);
	}
	else if (!strcmp(param, "RFMODE"))
	{
		HWREGB(BAK_MEM_RF_MODE) = val;
	}
	else if (!strcmp(param, "HTOL"))
	{
		HWREGB(BAK_MEM_TEST) = val;
	}
	else if(!strcmp(param,"DEEPSLEEPTHRESHOLD"))  //重启生效
	{
		g_softap_fac_nv->deepsleep_threshold = val;
		SAVE_FAC_PARAM(deepsleep_threshold);
#if !CP_FAST_RECOVERY_FUNCTION
		g_softap_fac_nv->fast_recovery_threshold = val;
		SAVE_FAC_PARAM(fast_recovery_threshold);  //CP不支持快速恢复，阈值必须相同，同步修改，防止遗忘	
#endif
	}

	else if(!strcmp(param,"FASTTHRESHOLD"))  //重启生效
	{
		g_softap_fac_nv->fast_recovery_threshold = val;
		SAVE_FAC_PARAM(fast_recovery_threshold);
#if !CP_FAST_RECOVERY_FUNCTION
		g_softap_fac_nv->deepsleep_threshold = val;
		SAVE_FAC_PARAM(deepsleep_threshold);
#endif
	}

	else if(!strcmp(param,"THRESHOLD"))  //重启生效
	{
		g_softap_fac_nv->FR_low_temp_threshold = val;
		SAVE_FAC_PARAM(FR_low_temp_threshold);
	}

	else if(!strcmp(param,"DSLEEPHIGHTEMPTHD"))  //重启生效
	{
		g_softap_fac_nv->FR_high_temp_threshold = val;
		SAVE_FAC_PARAM(FR_high_temp_threshold);
	}

	else if(!strcmp(param,"BAKMEM"))  //重启生效
	{
		g_softap_fac_nv->bakmem_threshold = (uint16_t)val;
		SAVE_FAC_PARAM(bakmem_threshold);
	}

	else if(!strcmp(param,"32KCLKMODE"))  //重启生效
	{
		g_softap_fac_nv->_32K_CLK_MODE = (uint8_t)val;
		SAVE_FAC_PARAM(_32K_CLK_MODE);
	}

	else if(!strcmp(param,"RATIME"))
	{
		g_softap_fac_nv->ra_timeout = (uint16_t)val;
		SAVE_FAC_PARAM(ra_timeout);
	}

	else if(!strcmp(param,"LOGRATE"))
	{
		g_softap_fac_nv->log_rate = (uint16_t)val;
		SAVE_FAC_PARAM(log_rate);
	}

	else if(!strcmp(param,"SETCLDO"))
	{
		g_softap_fac_nv->cldo_set_vol = (uint8_t)val;
		SAVE_FAC_PARAM(cldo_set_vol);
	}

	else if(!strcmp(param,"RATETEST"))
	{
		g_rate_test = val;
	}

	else if (!strcmp(param, "ATRECVTIMEOUT"))
	{
		g_softap_fac_nv->at_recv_timeout = val;
		SAVE_FAC_PARAM(at_recv_timeout);
	}

	else if (!strcmp(param, "STANDBYDELAY"))
	{
		g_softap_fac_nv->standby_delay = val;
		SAVE_FAC_PARAM(standby_delay);
	}

#if XY_DM
	else if(!strcmp(param,"DMRETRYTIME"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;

		dm_context_t* dm_pcontext = dm_get_context();
		dm_pcontext->retry_time = val;
		dm_save_config_file();
	}
	else if(!strcmp(param,"DMREGTIME"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;
		dm_context_t* dm_pcontext = dm_get_context();
		dm_pcontext->uni_reg_time = val;
		dm_save_config_file();	
	}
	else if(!strcmp(param,"DMRETRYNUM"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;
		dm_context_t* dm_pcontext = dm_get_context();
		dm_pcontext->retry_num = val;
		dm_save_config_file();
	}
#endif

	else if(!strcmp(param,"DEEPSLEEPDELAY"))
	{
		g_softap_fac_nv->deepsleep_delay = (uint8_t)val;
		SAVE_FAC_PARAM(deepsleep_delay);
	}
	else if(!strcmp(param,"RCAGINGPERIOD"))
	{
		g_softap_fac_nv->rc32k_aging_period = (uint8_t)val;
		SAVE_FAC_PARAM(rc32k_aging_period);
	}
	else if(!strcmp(param,"WDT"))
	{
		if(val != g_softap_fac_nv->watchdog_enable)
		{
			g_softap_fac_nv->watchdog_enable = (uint8_t)val;
			SAVE_FAC_PARAM(watchdog_enable);
			wdt_init();
		}
	}
	else if (!strcmp(param, "FILE"))
    {
#if TELECOM_VER && LITTLEFS_ENABLE
        xy_fremove(CDP_SESSION_NVM_FILE_NAME);
        xy_fremove(TELECOM_UNICOM_DM_NVM_FILE_NAME);
#endif

#if MOBILE_VER && LITTLEFS_ENABLE
        xy_fremove(ONENET_SESSION_NVM_FILE_NAME);
        xy_fremove(CMCC_DM_NVM_FILE_NAME);
#endif
    return XY_OK;
    }
	
	else if (!strcmp(param, "UARTSET"))
	{
		g_softap_fac_nv->at_uart_rate = val;
		SAVE_FAC_PARAM(at_uart_rate);
	}
	
	else if (!strcmp(param, "CLOSEDEBUG"))
	{
		g_softap_fac_nv->off_debug = val;
		SAVE_FAC_PARAM(off_debug);
	}
	else if (!strcmp(param, "OFFCHECK"))
	{
		g_softap_fac_nv->off_check = val;
		SAVE_FAC_PARAM(off_check);
	}
	else if (!strcmp(param, "SOCKASYN"))
	{
		g_softap_fac_nv->sock_async = val;
		SAVE_FAC_PARAM(sock_async);
	}
	else if (!strcmp(param, "NPSMR"))
	{
		g_softap_fac_nv->g_NPSMR_enable = val;
		SAVE_FAC_PARAM(g_NPSMR_enable);
	}
		
	else if (!strcmp(param, "DM"))
	{
		g_softap_fac_nv->need_start_dm = val;
		SAVE_FAC_PARAM(need_start_dm);
	}

	else if (!strcmp(param, "DUMPFLASH"))
	{
		g_softap_fac_nv->dump_mem_into_flash = val;
		SAVE_FAC_PARAM(dump_mem_into_flash);
	}
	//else if(!strcmp(param,"TCMCNT"))
	//g_softap_fac_nv->tcmcnt_enable = val;

	else if (!strcmp(param, "XTAL32K"))
	{
	    Set_32K_Freq(val);
	}
	
	else if (!strcmp(param, "RESETMEASURE"))
	{
        Set_32K_Freq(0xFFFFFFFF);
	}
#if configHEAP_TEST
	else if (!strcmp(param, "LRHEAP"))
	{
	    g_softap_fac_nv->LimitedRemainingHeap = val;
	    SAVE_FAC_PARAM(LimitedRemainingHeap);
	    extern uint32_t LimitedFreeHeap;
	    LimitedFreeHeap = g_softap_fac_nv->LimitedRemainingHeap;
	}
#endif


	else if (!strcmp(param, "MGHW"))
	{
		g_softap_fac_nv->rfSidoSetting_mg = val;
		SAVE_FAC_PARAM(rfSidoSetting_mg);
	}

	else
		return	0;
	return	1;

}

int simple_get_val(char *param,char **prsp_cmd)
{
	if (!strcmp(param, "RATIME"))
		sprintf(*prsp_cmd, "\r\n%u\r\n\r\nOK\r\n", g_softap_fac_nv->ra_timeout);

	else if(!strcmp(param,"OFFATWAKUP"))  
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->off_wakeupRXD);

	else if (!strcmp(param, "LOGRATE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->log_rate);

#if VER_BC25
	else if (!strcmp(param, "QSCLK"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->qsclk_mode);

#endif

	else if (!strcmp(param, "SETCLDO"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->cldo_set_vol);

	else if (!strcmp(param, "MTU"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->mtu);
		
	else if (!strcmp(param, "ATRECVTIMEOUT"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->at_recv_timeout);

	else if (!strcmp(param, "STANDBYDELAY"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->standby_delay);
	else if (!strcmp(param, "MEMDEBUG"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->mem_debug);
	else if (!strcmp(param, "TEST"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->test);

#if XY_DM
	else if(!strcmp(param,"DMRETRYTIME"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;

		dm_context_t* dm_pcontext = dm_get_context();
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n", dm_pcontext->retry_time);	
	}
	else if(!strcmp(param,"DMREGTIME"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;
		dm_context_t* dm_pcontext = dm_get_context();
		sprintf(*prsp_cmd,"\r\n%ld\r\n\r\nOK\r\n", dm_pcontext->uni_reg_time);
	}
	else if(!strcmp(param,"DMRETRYNUM"))
	{
		if(dm_context_init() != DM_PROC_SUCCESS)
			return 0;
		dm_context_t* dm_pcontext = dm_get_context();
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n", dm_pcontext->retry_num);
	}
#endif

	else if(!strcmp(param, "FRAMECAL"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->frame_cal);

	else if(!strcmp(param, "CLOSEDRXSLEEP"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->close_drx_dsleep);

	else if(!strcmp(param, "DEEPSLEEPDELAY"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->deepsleep_delay);

	else if(!strcmp(param, "RCAGINGPERIOD"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->rc32k_aging_period);

	else if(!strcmp(param, "DEEPSLEEPTHRESHOLD"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->deepsleep_threshold);

	else if(!strcmp(param, "FASTTHRESHOLD"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->fast_recovery_threshold);

	else if(!strcmp(param, "THRESHOLD"))
		sprintf(*prsp_cmd,"\r\n%u\r\n\r\nOK\r\n",g_softap_fac_nv->FR_low_temp_threshold);

	else if(!strcmp(param, "DSLEEPHIGHTEMPTHD"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->FR_high_temp_threshold);

	else if(!strcmp(param, "BAKMEM"))
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->bakmem_threshold);

	else if(!strcmp(param,"32KCLKMODE"))  
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",g_softap_fac_nv->_32K_CLK_MODE);

	else if(!strcmp(param,"32KCLKSRC"))  
		sprintf(*prsp_cmd,"\r\n%d\r\n\r\nOK\r\n",HWREGB(BAK_MEM_32K_CLK_SRC));
		
	else if (!strcmp(param, "WDT"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->watchdog_enable);
	
	else if (!strcmp(param, "UARTSET"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->at_uart_rate & 0x1ff);
	
	else if (!strcmp(param, "CLOSEDEBUG"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->off_debug);
	
	else if (!strcmp(param, "OFFCHECK"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->off_check);

	else if (!strcmp(param, "SOCKASYN"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->sock_async);

	else if (!strcmp(param, "NPSMR"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->g_NPSMR_enable);
		
	else if (!strcmp(param, "DEEPSLEEP"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->deepsleep_enable);
	
	else if (!strcmp(param, "STANDBY"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->lpm_standby_enable);
	
	else if (!strcmp(param, "EXTVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->versionExt);
	
	else if (!strcmp(param, "HARDVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->hardver);

	else if (!strcmp(param, "PRODUCTVER"))
		sprintf(*prsp_cmd, "\r\n%s\r\n\r\nOK\r\n", g_softap_fac_nv->product_ver);

	else if (!strcmp(param, "DM"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->need_start_dm);
	
	else if (!strcmp(param, "LOG"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->open_log);

	else if (!strcmp(param, "LOGSIZE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->log_size);
	
	else if (!strcmp(param, "IO"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->io_output_vol);

	else if (!strcmp(param, "WFI"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->wfi_enable);

	else if (!strcmp(param, "FAC"))
        sprintf(*prsp_cmd, "\r\nLOG:%d,CLOSEDEBUG:%d\r\n\r\nOK\r\n",g_softap_fac_nv->open_log, g_softap_fac_nv->off_debug);
#if TELECOM_VER	
	else if (!strcmp(param, "CDPREGMODE"))
	{
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->cdp_register_mode);
	}	
#endif	
	else if (!strcmp(param, "VER"))
		sprintf(*prsp_cmd, "\r\n%s,%s,%s,%s\r\n\r\nOK\r\n", g_softap_fac_nv->product_ver, g_softap_fac_nv->modul_ver, g_softap_fac_nv->hardver, g_softap_fac_nv->versionExt);
	
	else if (!strcmp(param, "XO"))
        sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", 1);

	else if (!strcmp(param, "RFMODE"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", HWREGB(BAK_MEM_RF_MODE));

	else if (!strcmp(param, "HTOL"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n",is_htol_test());

	else if (!strcmp(param, "DUMPFLASH"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->dump_mem_into_flash);
	
	else if (!strcmp(param, "XTAL32K"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", Get_32K_Freq());
	

	else if (!strcmp(param, "MGHW"))
		sprintf(*prsp_cmd, "\r\n%d\r\n\r\nOK\r\n", g_softap_fac_nv->rfSidoSetting_mg);

	else
		return	0;
	return	1;
}


/*******************************************************************************
 *						Global function implementations 					   *
 ******************************************************************************/

extern int g_Rebooting;
int at_NV_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};
		char param[20] = {0};
		char strval[64] = {0};
		char verval[30] = {0};
		int val = 0;
		uint32_t str_len;
		osThreadAttr_t thread_attr = {0};

		if (at_parse_param("%10s,", at_buf, cmd) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		/*该参数仅供射频校准工具软重启时使用*/
		if (!strcmp(cmd, "SAVE"))
		{
			/* 输出REBOOTING字符串时，不能有切换线程的动作！ */
			send_urc_to_ext_NoCache("\r\nREBOOTING\r\n", strlen("\r\nREBOOTING\r\n"));
			g_Rebooting = 1;
			osDelay(100);		//软重启时，延时一段时间以等待AP核把“REBOOTING”、“RESETING”等全部输出到串口
			xy_Soft_Reset(SOFT_RB_BY_NRB);
		}
		else if (!strcmp(cmd, "FAC"))
		{
			flash_interface_protect_disable();
			xy_Flash_Write(NV_MAIN_FACTORY_BASE, (uint8_t *)g_factory_nv, sizeof(factory_nv_t));
			flash_interface_protect_enable();
			return AT_END;
		}
		else if (!strcmp(cmd, "SET"))
		{
			if (at_parse_param(",%20s,", at_buf, param) != AT_OK)
				goto ERR;

			else if (!strcmp(param, "PRODUCTVER"))
			{
				if (at_parse_param(",,%28s", at_buf, verval))
					goto ERR;
				memset(g_softap_fac_nv->product_ver, 0, sizeof(g_softap_fac_nv->product_ver));
				memcpy(g_softap_fac_nv->product_ver, verval, strlen(verval));
				SAVE_FAC_PARAM(product_ver);
			}
			else if (!strcmp(param, "MODULVER"))
			{
				if (at_parse_param(",,%20s", at_buf, verval))
					goto ERR;
				if (strrchr(verval, '-') == NULL)
				{
					return  (ATERR_PARAM_INVALID);
				}
				else
				{
					memset(g_softap_fac_nv->modul_ver, 0, sizeof(g_softap_fac_nv->modul_ver));
					memcpy(g_softap_fac_nv->modul_ver, verval, strlen(verval));
					SAVE_FAC_PARAM(modul_ver);
				}
			}
			else if (!strcmp(param, "HARDVER"))
			{
				if (at_parse_param(",,%20s", at_buf, verval))
					goto ERR;
				memset(g_softap_fac_nv->hardver, 0, sizeof(g_softap_fac_nv->hardver));
				memcpy(g_softap_fac_nv->hardver, verval, strlen(verval));
				SAVE_FAC_PARAM(hardver);
			}
			else if (!strcmp(param, "VERSIONEXT"))
			{
				if (at_parse_param(",,%28s", at_buf, verval))
					goto ERR;
				memset(g_softap_fac_nv->versionExt, 0, sizeof(g_softap_fac_nv->versionExt));
				memcpy(g_softap_fac_nv->versionExt, verval, strlen(verval));
				SAVE_FAC_PARAM(versionExt);
			}
#if TELECOM_VER
			else if (!strcmp(param, "CDPREGMODE"))
			{
				if (at_parse_param(",,%d", at_buf, &val) != AT_OK)
					goto ERR;
				if (val != 0 && val != 1)
					goto ERR;

				g_softap_fac_nv->cdp_register_mode = val;
				SAVE_FAC_PARAM(cdp_register_mode);
			}
#endif
			else if (!strcmp(param, "PIN"))
			{
				int val1, val2, val3, val4;

				if (at_parse_param(",,%64s,%d,%d", at_buf, strval, &val1, &val2, &val3, &val4) != AT_OK)
					goto ERR;
				if (!strcmp(strval, "SWD"))
				{
					g_softap_fac_nv->swd_swclk_pin = val1;
					g_softap_fac_nv->swd_swdio_pin = val2;
					SAVE_FAC_PARAM(swd_swclk_pin);
					SAVE_FAC_PARAM(swd_swdio_pin);
				}

				else if (!strcmp(strval, "AT"))
				{
					g_softap_fac_nv->at_txd_pin = val1;
					g_softap_fac_nv->at_rxd_pin = val2;
					SAVE_FAC_PARAM(at_txd_pin);
					SAVE_FAC_PARAM(at_rxd_pin);
				}
				else if (!strcmp(strval, "LOG"))
				{
					g_softap_fac_nv->log_txd_pin = val1;
					g_softap_fac_nv->log_rxd_pin = val2;
					SAVE_FAC_PARAM(log_txd_pin);
					SAVE_FAC_PARAM(log_rxd_pin);
				}
				else if (!strcmp(strval, "LED"))
				{
					g_softap_fac_nv->led_pin = val1;
					SAVE_FAC_PARAM(led_pin);
				}
				else
					return  (ATERR_PARAM_INVALID);
			}
	
			else
			{

				if (at_parse_param(",%20s,%d", at_buf, param, &val) != AT_OK)
					goto ERR;

				if(simple_set_val(param,val) == 1)
					return AT_END;
				else
					return  (ATERR_PARAM_INVALID);
			}
			return AT_END;
		}
		else if (!strcmp(cmd, "GET"))
		{
			char sub_param[15];

			if (at_parse_param(",%20s,%s", at_buf, param, sub_param) != AT_OK)
				goto ERR;
			*prsp_cmd = xy_malloc(256);

			if(simple_get_val(param,prsp_cmd) == 1)
				return AT_END;
			else if (!strcmp(param, "FLASHID"))
			{
				uint8_t* UniqueID = xy_malloc(16);
				xy_Flash_Read_UniqueID128(UniqueID);
				sprintf(*prsp_cmd, "\r\n%1x%1x%1x%1x"
									   "%1x%1x%1x%1x"
									   "%1x%1x%1x%1x"
									   "%1x%1x%1x%1x",
									   *((uint8_t*)UniqueID), *((uint8_t*)UniqueID + 1), *((uint8_t*)UniqueID + 2), *((uint8_t*)UniqueID + 3),
									   *((uint8_t*)UniqueID + 4), *((uint8_t*)UniqueID + 5), *((uint8_t*)UniqueID + 6), *((uint8_t*)UniqueID + 7),
									   *((uint8_t*)UniqueID + 8), *((uint8_t*)UniqueID + 9), *((uint8_t*)UniqueID + 10), *((uint8_t*)UniqueID + 11),
									   *((uint8_t*)UniqueID + 12), *((uint8_t*)UniqueID + 13), *((uint8_t*)UniqueID + 14), *((uint8_t*)UniqueID + 15));
				xy_free(UniqueID);
			}
			else if (!strcmp(param, "EFTL"))
			{
				uint32_t addr = 0;

				if (!strcmp(sub_param, "FACTORY"))
					addr = NV_FLASH_FACTORY_BASE;
				else if(!strcmp(sub_param, "NONVAR"))
					addr = NV_NON_VOLATILE_BASE;
				else if(!strcmp(sub_param, "VAR1"))
					addr = BAK_MEM_FLASH_BASE;
				else if(!strcmp(sub_param, "VAR2"))
					addr = BAK_MEM_FLASH_BASE2;
				else
					goto ERR;

				sprintf(*prsp_cmd, "\r\n%d", ftl_read_write_num(addr));
			}
			else if(!strcmp(param,"MALLOC"))
			{
				str_len = 0;
				str_len += sprintf(*prsp_cmd,"\r\nxy_malloc2 lose: %d\r\n", g_xy_mem_alloc_loose_cnt);
				str_len += sprintf(*prsp_cmd + str_len,"xy_malloc2_Align lose: %d\r\nOK\r\n", g_xy_mem_alloc_align_loose_cnt);
			}
			else if(!strcmp(param,"EXTMEM"))
			{
				str_len = 0;
				str_len += sprintf(*prsp_cmd,"\r\n+PeakUsed:%d\r\n", g_ps_mem_used_max);
				str_len += sprintf(*prsp_cmd + str_len,"+CurrentUsed:%d\r\nOK\r\n", g_ps_mem_used);
			}
			else if(!strcmp(param,"FOTA"))
			{
				uint32_t addr, len;
				
				xy_OTA_flash_info(&addr,&len);
				
				sprintf(*prsp_cmd,"+FOTA:%X,%d",addr,len);
			}
			else if (!strcmp(param, "PSM"))
			{
				unsigned long  param1;
				unsigned long  param2;
					
				ps_sleep_state_check(&param1,&param2);

		    	sprintf(*prsp_cmd,"STATE:%d,PARAM1:%d,PARAM2:%d\r\nOK\r\n",Ps_Lpminfo.state,param1,param2);
			}
			else if (!strcmp(param, "SLEEP"))
			{
				xySatusList_t *pxSatusList;
				StackStatus_t *tmpNode;
				uint16_t str_len = 0;

				str_len += sprintf(*prsp_cmd, "\r\nEnter Idle %d ms before\r\nIdle Sleep Check:%d, %d\r\nPS Sleep State:%d\r\nReady Task List:", CONVERT_RTCTICK_TO_MS(get_utc_tick() - get_tick_enter_idle()),
								   ((get_sleep_state() & 0xF0) >> 4), (get_sleep_state() & 0x0F), Ps_SleepState_Enquiry());
				pxSatusList = xyTaskGetStackInfo();
				tmpNode = pxSatusList->xyListEnd.pxNext;
				while (tmpNode != &(pxSatusList->xyListEnd))
				{
					if (tmpNode->xyState == eReady)
						str_len += sprintf(*prsp_cmd + str_len, "%s,", tmpNode->xyTaskName);
					tmpNode = tmpNode->pxNext;
				}
				vTaskFreeList(pxSatusList);
				sprintf(*prsp_cmd + str_len - 1, "\r\n\r\nOK\r\n");
			}
			else if (!strcmp(param, "PIN"))
			{
				if (!strcmp(sub_param, "SWD"))
					sprintf(*prsp_cmd, "\r\n%d,%d", g_softap_fac_nv->swd_swclk_pin, g_softap_fac_nv->swd_swdio_pin);

				else if (!strcmp(sub_param, "AT"))
					sprintf(*prsp_cmd, "\r\n%d,%d", g_softap_fac_nv->at_txd_pin, g_softap_fac_nv->at_rxd_pin);

				else if (!strcmp(sub_param, "LOG"))
					sprintf(*prsp_cmd, "\r\n%d,%d", g_softap_fac_nv->log_txd_pin, g_softap_fac_nv->log_rxd_pin);

				else if (!strcmp(sub_param, "LED"))
					sprintf(*prsp_cmd, "\r\n%d", g_softap_fac_nv->led_pin);
				else
				{
					if (*prsp_cmd != NULL)
						xy_free(*prsp_cmd);
					return  (ATERR_PARAM_INVALID);
				}
			}
			else if (!strcmp(param, "TICK"))
			{
				uint32_t tick = osKernelGetTickCount();
				sprintf(*prsp_cmd, "\r\n+CPTICK:%ld\r\n", tick);
			}
			else if (!strcmp(param, "LOGINFO"))
			{
				extern diag_status_info_t g_log_status;
				extern diag_debug_info_t g_diag_debug;

				str_len = 0;
				str_len += sprintf(*prsp_cmd,"\r\n+HEART_TIMEOUT:%d,CUR_TICK:%d,RECV_HEART:%d\r\n", g_log_status.heart_end_count, (uint32_t)DIAG_GET_TICK_COUNT(), g_log_status.heart_recv_tick);
				str_len += sprintf(*prsp_cmd + str_len,"+CSP_RECV:%d,Valid_Packet:%d,CSP_SEND:%d\r\n", g_log_status.data_recv_tick, g_log_status.recv_valid_packet_tick, g_log_status.data_send_tick);
				str_len += sprintf(*prsp_cmd + str_len,"+refresh_heart_flag:%d,alloc_mem_succ_cnt:%d\r\nOK\r\n", g_softap_var_nv->diag_flag, g_diag_debug.alloc_mem_succ_cnt);
			}
			else
			{
				if(*prsp_cmd != NULL)
				{
					xy_free(*prsp_cmd);
					*prsp_cmd = NULL;
				}
				return AT_FORWARD;
			}
			return AT_END;
		}
	ERR:
		return  (ATERR_PARAM_INVALID);
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		char *str = "\r\n+NV:WDT+ARMSTACK+CLOSEDEBUG+NPSMR+DOWNDATA+DEEPSLEEP+STANDBY+EXTVER+HARDVER+VERTYPE+DM+WFI+PSM+VER+UARTSET+PIN(SWD+JTAG+AT+LOG+LED)";
		*prsp_cmd = xy_malloc(strlen(str) + 1);
		strcpy(*prsp_cmd, str);
	}
#endif
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}


int at_mgsleep_req(char *at_buf,char **prsp_cmd)
{
	char cmd[10] = {0};

	if (g_req_type == AT_CMD_REQ)
	{
		if (at_parse_param("%10s,", at_buf, cmd) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		if (!strcmp(cmd, "enable")) {
			g_softap_fac_nv->deepsleep_enable = 1;
			SAVE_FAC_PARAM(deepsleep_enable);
			
			*prsp_cmd = xy_malloc(56);
			sprintf(*prsp_cmd, "\r\n\r\nOK\r\n");

		} else if (!strcmp(cmd, "disable")) {
			g_softap_fac_nv->deepsleep_enable = 0;
			SAVE_FAC_PARAM(deepsleep_enable);
			*prsp_cmd = xy_malloc(56);
			sprintf(*prsp_cmd, "\r\n\r\nOK\r\n");

		} else {
			return  (ATERR_PARAM_INVALID);
		}
	} else 	if (g_req_type == AT_CMD_QUERY) {
		*prsp_cmd = xy_malloc(56);
		if (g_softap_fac_nv->deepsleep_enable == 1) {
			sprintf(*prsp_cmd, "\r\n+MGSLEEP: enable\r\n\r\nOK\r\n");
		} else {
			sprintf(*prsp_cmd, "\r\n+MGSLEEP: disable\r\n\r\nOK\r\n");
		}
	} else 	if (g_req_type == AT_CMD_TEST) {
			*prsp_cmd = xy_malloc(56);
			sprintf(*prsp_cmd, "\r\n+MGSLEEP: <sleepmode>\r\n\r\nOK\r\n");
	} else {
		return  (ATERR_PARAM_INVALID);
	}
	return AT_END;
}

//获取当前CP核内存使用
int at_NUESTATS_req(char* at_buf, char** prsp_cmd)
{
	char cmd[15] = {0};

	if (at_parse_param("%15s,", at_buf, cmd) != AT_OK)
	{
		return  (ATERR_PARAM_INVALID);
	}
	
	if (!strcmp(cmd, "APPSMEM"))
	{
		AppsMemStats_t xHeapStats;
	    vTaskGetAppsMem(&xHeapStats);
	    *prsp_cmd = xy_malloc(400);

		uint32_t str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Heap Size:%d\r\n", cmd, xHeapStats.HeapSize);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Heap Peak Used:%d\r\n", cmd, xHeapStats.HeapSize - xHeapStats.MinimumEverFreeBytesRemaining);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Current Allocated:%d\r\n", cmd, xHeapStats.AllocatedHeapSize);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Total Free:%d\r\n", cmd, xHeapStats.AvailableHeapSize);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Max Free:%d\r\n", cmd, xHeapStats.MaxSizeFreeBlock);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Num Allocs:%d\r\n", cmd, xHeapStats.AllocatedBlockNum);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Num Frees:%d\r\n", cmd, xHeapStats.FreeBlockNum);

		str_len += sprintf(*prsp_cmd + str_len, "\r\nOK\r\n");
	}
	else if(!strcmp(cmd, "ALLMEM"))
	{
		extern uint32_t __Flash_Total;
		extern uint32_t __Flash_Used;
		extern uint32_t __Flash_Remain;
		extern uint32_t __Ram_Total;
		extern uint32_t __Ram_Used;
		extern uint32_t __Ram_Remain;
	    *prsp_cmd = xy_malloc(400);

		uint32_t str_len = 0;
		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Total Flash Space:%#x\r\n", cmd, &__Flash_Total);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Flash Used:%#x\r\n", cmd, &__Flash_Used);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Flash Remaining:%#x\r\n", cmd, &__Flash_Remain);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Total Ram Space:%#x\r\n", cmd, &__Ram_Total);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Ram Used:%#x\r\n", cmd, &__Ram_Used);

		str_len += sprintf(*prsp_cmd + str_len, "\r\n+NUESTATS:%s,Ram Remaining:%#x\r\n", cmd, &__Ram_Remain);

		str_len += sprintf(*prsp_cmd + str_len, "\r\nOK\r\n");
	}
	else
	{
		return AT_FORWARD;
	}
	return AT_END;
}
//请求制造商版本号AT+CGMR
//AT+CGMR=<verval>
int at_CMVER_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char *verval = xy_malloc(28);
		memset(verval, 0, 28);
		xy_assert(verval != NULL);
		
		if(at_parse_param("%28s", at_buf, verval) != AT_OK)
		{
			xy_free(verval);
			return  (ATERR_PARAM_INVALID);
		}
		
		memset(g_softap_fac_nv->versionExt, 0 ,sizeof(g_softap_fac_nv->versionExt));
		memcpy(g_softap_fac_nv->versionExt, verval,strlen(verval));
		//if softap factory NV vary,must set 1
		SAVE_FAC_PARAM(versionExt);
		xy_free(verval);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(60);
#if VER_BC95
		snprintf(*prsp_cmd, 60, "\r\n%s", g_softap_fac_nv->versionExt);
#else
		snprintf(*prsp_cmd, 60, "\r\nSoftware Version:%s", g_softap_fac_nv->versionExt);
#endif
	}
#if (AT_CUT != 1)
	else if (g_req_type == AT_CMD_TEST)
	{
		return AT_END;	
	}
#endif
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;	
}
//请求硬件版本号AT+HVER
//AT+HVER=<verval>
int at_HVER_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		char verval[20] = {0};
		
		if(at_parse_param("%20s", at_buf, verval) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		
		memset(g_softap_fac_nv->hardver, 0 ,sizeof(g_softap_fac_nv->hardver));
		memcpy(g_softap_fac_nv->hardver, verval,strlen(verval));
		SAVE_FAC_PARAM(hardver);
	}
	else if(g_req_type == AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(55);
		snprintf(*prsp_cmd, 55, "\r\nHardware Version:%s", g_softap_fac_nv->hardver);
	}
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;	
}

//请求制造商型号AT+CGMM
//AT+CGMM=<verval>
int at_CGMM_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);

		snprintf(*prsp_cmd, 128, 
			"\r\n%s_%s\r\n\r\nOK\r\n", 
			VENDOR_NAME,
			PRODUCT_NAME 
			);	
	}
	else 
		return ATERR_PARAM_INVALID;
	return AT_END;
}

//请求制造商标识AT+CGMI
//AT+CGMI=<verval>
int at_CGMI_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);

		snprintf(*prsp_cmd, 128, 
			"\r\n%s\r\n\r\nOK\r\n", 
			VENDOR_NAME
			);	
	}
	else 
		return ATERR_PARAM_INVALID;

	return AT_END;
	
}


//AT+NPSMR=<n>         AT+NPSMR? +NPSMR:<n>[,<mode>]
//when  <n>=1,and worklock is 0,SoC will send unsolicited result +NPSMR:<mode>
int at_NPSMR_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type == AT_CMD_REQ)
	{
		int val = 0;

		if (at_parse_param("%d(0-1)", at_buf, &val) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}
		
		g_softap_fac_nv->g_NPSMR_enable = val;
#if 0//VER_BC95
		g_softap_var_nv->g_NPSMR_enable = val;
#else
		SAVE_FAC_PARAM(g_NPSMR_enable);
#endif
	}
	else if(g_req_type == AT_CMD_QUERY)
	{
		*prsp_cmd = xy_malloc(40);

		if (g_softap_fac_nv->g_NPSMR_enable==1)
		{
#if 0//(VER_BC95)
			extern int g_npsmr_status;
		    snprintf(*prsp_cmd, 40, "+NPSMR:1,%d", g_npsmr_status);
#else
			snprintf(*prsp_cmd, 40, "+NPSMR:1,0");
#endif
		}
		else
		{
			snprintf(*prsp_cmd, 40, "+NPSMR:0");
		}
	}
#if (AT_CUT != 1)
	else if(g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(40);
		snprintf(*prsp_cmd,40,"+NPSMR:(0,1)");
	}
#endif
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}
//显示产品标识信息ATI
int at_ATI_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);

		snprintf(*prsp_cmd, 128, 
			"\r\n%s\r\n%s\r\nRevision:%s_T%sS%s\r\n\r\nOK\r\n", 
			VENDOR_NAME,
			PRODUCT_NAME, 
			PRODUCT_NAME,
			VERSION_INFO_NEW,
			BUILD_DATE
			);	
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
	return AT_END;
}

int at_SGSW_req(char *at_buf, char **prsp_cmd)
{
	//SLM130-NA.1.2.0.0T00S0531_M018_XY1200S
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);
		snprintf(*prsp_cmd, 128, "\r\n%s.%sT%sS%s_%s_%s\r\n\r\nOK\r\n",
			PRODUCT_NAME,
			BOARD_PATCH,
			VERSION_INFO_NEW,
			BUILD_DATE,
			VERSION_DEV_TYPE,
			MODULE_CHIP_NAME
			);
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}

	return AT_END;	
}

// SoftSIM密钥配置AT命令
// eg: AT+SOFTSIM=011208294305123023660802149854370000003262865003202BA2E2461C022E31F0AF0735FC4B455904207DFEF49A5BA53AF1290C3945A57F56000520F6A02E230228EFC0A7D5014CBD1399DB0620BC6628E23DD24B502DD8F345AF0DB0B0
int at_SOFTSIM_req(char *at_buf, char **prsp_cmd){
	// 判断用户的动作
	if ( g_req_type == AT_CMD_ACTIVE )
	{
		// AT+XXX 查询密钥
		*prsp_cmd = xy_malloc(360);
		char secretkey1[351] = {0};

		xy_file fp = xy_fopen("ss_SIMKEY.key","r",FS_DEFAULT);
		if ( fp == NULL )
		{
			// 文件不存在
			snprintf(*prsp_cmd,20,"+SOFTSIM:NULL");
			return AT_END;
		}
		int readcount = xy_fread(secretkey1, 351, fp);

		snprintf(*prsp_cmd,360,"+SOFTSIM:%s", secretkey1);
		xy_fclose(fp);

		return AT_END;
	}

	if ( g_req_type == AT_CMD_REQ )
	{
		// AT+XXX=param  设置密钥
		char secretkey[351] = {0};

		if(at_parse_param("%s", at_buf, secretkey) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		xy_printf(0, PLATFORM, FATAL_LOG, "at_SOFTSIM_req-secretkey len:%d", strlen(secretkey));

		// 准备写入文件
		xy_fremove("ss_SIMKEY.key", FS_DEFAULT);
		xy_printf(0, PLATFORM, FATAL_LOG, "at_SOFTSIM_req- xy_fremove end");
		xy_file fp = xy_fopen("ss_SIMKEY.key","w", FS_DEFAULT);
		xy_printf(0, PLATFORM, FATAL_LOG, "at_SOFTSIM_req- xy_fopen end");

		int writeCount = xy_fwrite(secretkey, strlen(secretkey), fp);
		xy_printf(0, PLATFORM, FATAL_LOG, "at_SOFTSIM_req- write end");
		xy_fclose(fp);

		return AT_END;
	}
	
	return ATERR_PARAM_INVALID;
}

// 切换UICC模式
// AT+SWUICC=0 sim
// AT+SWUICC=1 softsim
int at_SWUICC_req(char *at_buf, char **prsp_cmd){
	int uiccmode = 1;
	if ( g_req_type == AT_CMD_ACTIVE )
	{
		*prsp_cmd = xy_malloc(11);

		xy_file fp = xy_fopen("uiccmode.txt","r",FS_DEFAULT);
		if ( fp == NULL )
		{
			// 文件不存在
			snprintf(*prsp_cmd,11,"+SWUICC:1");
			return AT_END;
		}
		int readcount = xy_fread(&uiccmode, sizeof(int), fp);

		snprintf(*prsp_cmd,11,"+SWUICC:%d", uiccmode);
		xy_fclose(fp);

		return AT_END;
	}

	if ( g_req_type == AT_CMD_REQ )
	{
		if(at_parse_param("%d", at_buf, &uiccmode) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		// 准备写入文件
		xy_fremove("uiccmode.txt", FS_DEFAULT);
		xy_file fp = xy_fopen("uiccmode.txt","w", FS_DEFAULT);

		int writeCount = xy_fwrite(&uiccmode, sizeof(int), fp);
		xy_fclose(fp);

		return AT_END;
	}
}

char* my_itoa(unsigned long long int num,char* str,int radix)
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned long long int unum;
    int i=0,j,k;
 
    if(radix==10&&num<0)
    {
        unum=(unsigned long long int)-num;
        str[i++]='-';
    }
    else unum=(unsigned long long int)num;
 

    do
    {
        str[i++]=index[unum%(unsigned long long int)radix];
        unum/=radix;
 
    }while(unum);
 
    str[i]='\0';
 
   
    if(str[0]=='-') k=1;
    else k=0;
 
    char temp;
    for(j=k;j<=(i-1)/2;j++)
    {
        temp=str[j];
        str[j]=str[i-1+k-j];
        str[i-1+k-j]=temp;
    }
 
    return str;
 
}
int at_NGT3412_req(char *at_buf, char **prsp_cmd)
{
	int ret;
	(void) at_buf;
	ATC_MSG_CPSMS_R_CNF_STRU pCpsmsCnf = { 0 };
    unsigned char                  i = 0, p = 4, q = 16;
    unsigned char                   aucTau[9]     = {0};
	char                  str[50]= {0};
	int height_three_bit_val= 0,low_five_bit_val= 0;
	unsigned long long int millisecond = 0;
	
	if(xy_atc_interface_call("AT+CPSMS?\r\n", NULL, (void*)&pCpsmsCnf) == ATC_AP_FALSE)
		return ATC_AP_FALSE;
	
    for ( i = 0; i < 8; i++)
    {
        aucTau[i] = ((pCpsmsCnf.ucReqPeriTAU >> (7 - i))&0x01) + 0x30;
    }		
	i = 0;
	while((i < 8))
	{
		if(i < 3)
		{
			height_three_bit_val += ((aucTau[i]- '0') * p);
			p /= 2;
		}
		else
		{
			low_five_bit_val += ((aucTau[i] - '0') * q);		
			q /= 2;
		}		
		i++;
	}
	switch (height_three_bit_val)
	{
		case 0:
			millisecond = low_five_bit_val * 10 * 60 * 1000;
			break;
		case 1:
			millisecond = low_five_bit_val * 1 * 60 * 60 * 1000;
			break;
		case 2:
			millisecond = low_five_bit_val * 10 * 60 * 60 * 1000;
			break;
		case 3:
			millisecond = low_five_bit_val * 2 * 1000;
			break;			
		case 4:
			millisecond = low_five_bit_val * 30 * 1000;
			break;
		case 5:
			millisecond = low_five_bit_val * 1 * 60 * 1000;
			break;
		case 6:
			millisecond = (unsigned long long int)low_five_bit_val * 320 * 60 * 60 * 1000;
			break;
		case 7:
			break;			
	}
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);
		my_itoa(millisecond, str, 10);			
		snprintf(*prsp_cmd, 128, "\r\n+NGT3412:%s",str);
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}
	return AT_END;	
}

int at_CGMR_req(char *at_buf, char **prsp_cmd)
{
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);

		snprintf(*prsp_cmd, 128, 
			"\r\nRevision:%s_T%sS%s\r\n\r\nOK\r\n", 
			PRODUCT_NAME,
			VERSION_INFO_NEW,
			BUILD_DATE
			);	
	}
	else
	{
		return  (ATERR_PARAM_INVALID);
	}

	return AT_END;
}

//获取产品版本号AT+QGMR
int at_QGMR_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;

	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(128);
		snprintf(*prsp_cmd, 128, "\r\n%s", PRODUCT_NAME);
	}
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}

void GetCompileTime(xy_wall_clock_t *compiletime)
{
	uint8_t mon[4] = {0};
	sscanf(__DATE__, "%3s %d %d", mon, &compiletime->tm_mday, &compiletime->tm_year);
	
	if(!strcmp(mon, "Jan")) compiletime->tm_mon = 1;
	else if(!strcmp(mon, "Feb")) compiletime->tm_mon = 2;
	else if(!strcmp(mon, "Mar")) compiletime->tm_mon = 3;
	else if(!strcmp(mon, "Apr")) compiletime->tm_mon = 4;
	else if(!strcmp(mon, "May")) compiletime->tm_mon = 5;
	else if(!strcmp(mon, "Jun")) compiletime->tm_mon = 6;
	else if(!strcmp(mon, "Jul")) compiletime->tm_mon = 7;
	else if(!strcmp(mon, "Aug")) compiletime->tm_mon = 8;
	else if(!strcmp(mon, "Sep")) compiletime->tm_mon = 9;
	else if(!strcmp(mon, "Oct")) compiletime->tm_mon = 10;
	else if(!strcmp(mon, "Nov")) compiletime->tm_mon = 11;
	else if(!strcmp(mon, "Dec")) compiletime->tm_mon = 12;
	else
		compiletime->tm_mon = 0;
}

//获取版本编译时间 AT+QVERTIME
int at_QVERTIME_req(char *at_buf, char **prsp_cmd)
{
	(void) at_buf;
	xy_wall_clock_t compiletime = {0};
	if(g_req_type==AT_CMD_ACTIVE)
	{
		*prsp_cmd = xy_malloc(60);
		GetCompileTime(&compiletime);
		snprintf(*prsp_cmd, 60, "\r\n%d-%d-%d %s", compiletime.tm_year,compiletime.tm_mon,compiletime.tm_mday, __TIME__);
	}
	else
		return  (ATERR_PARAM_INVALID);
	return AT_END;
}

//导出flash中的dump AT+DUMPFLASH
int at_DUMP_req(char *at_buf, char **prsp_cmd)
{
	vTaskSuspendAll();

	dump_flash_to_file();
 	( void ) xTaskResumeAll();

	return AT_END;
}
