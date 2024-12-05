/**
 * @file at_test_cmd.c
 * @brief 
 * @version 1.0
 * @date 2021-12-17
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */


#include "at_test_cmd.h"
#include "at_uart.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "osAssitantUtils.h"
#include "xy_memmap.h"
#include "flash_adapt.h"
#include "oss_nv.h"
//#include "rf_mt.h"
#include "at_tcpip_api.h"
#if AT_SOCKET
#include "at_socket_api.h"
#endif /* AT_SOCKET */
//#include "PhyRxAGC.h"
//#include "NBPhyL1cExportInterface.h"

void urc_cache_test(void *arg)
{
    int i = 1;
    char* debug = xy_malloc(32);
    while(i < 1000)
    {
        snprintf(debug, 32, "+XINYI:%d", i++);
        send_urc_to_ext(debug,strlen(debug));
        osDelay(1000);
    }
    xy_free(debug);
    osThreadExit();
}

void urc_cache_test1(void *arg)
{
    int i = 1;
    char* debug = xy_malloc(32);
    while(i < 1000)
    {
        snprintf(debug, 32, "+XINYIPASSTHR:%d", i++);
        send_urc_to_ext(debug, strlen(debug));
        osDelay(1500);
    }
    xy_free(debug);
    osThreadExit();
}

extern HeapRegion_t HeapRegions[HEAP_REGION_MAX + 1];

typedef struct
{
	uint32_t base;
	uint32_t size;
} flash_test_msg_t;

osThreadId_t g_flash_test_TskHandle = NULL;
osMessageQueueId_t g_flash_test_msg_q = NULL;
void flash_test_entry(void)
{
	flash_test_msg_t *msg = NULL;
	char *buff = xy_malloc(USER_BAK_MEM_LEN+5);	

	while (1)
	{
		osMessageQueueGet(g_flash_test_msg_q, (void *)(&msg), NULL, osWaitForever);
		msg->base = Address_Translation_AP_To_CP(msg->base);

		xy_Flash_Erase(msg->base, msg->size);
		xy_Flash_Read(msg->base, buff+USER_BAK_MEM_LEN, 4);
		xy_Flash_Write(msg->base, SHARE_MEM_BASE, SHARE_MEM_LENGTH+BAK_MEM_LENGTH);
		xy_Flash_Read(msg->base+SHARE_MEM_LENGTH+(USER_BAK_MEM_BASE-BAK_MEM_BASE), buff, USER_BAK_MEM_LEN);

		if ((HWREG(buff) == HWREG(USER_BAK_MEM_BASE)) && (HWREG(buff+USER_BAK_MEM_LEN-5) == HWREG(USER_BAK_MEM_BASE+USER_BAK_MEM_LEN-5)) && (HWREG(buff+USER_BAK_MEM_LEN) == 0xFFFFFFFF))
			sprintf(buff, "test flash succ!\r\n");
		else
			sprintf(buff, "test flash err:%X %X %X %X %X\r\n", HWREG(buff), HWREG(buff+USER_BAK_MEM_LEN-5), \
				HWREG(USER_BAK_MEM_BASE), HWREG(USER_BAK_MEM_BASE+USER_BAK_MEM_LEN-5), (HWREG(buff+USER_BAK_MEM_LEN) == 0xFFFFFFFF));
		
		xy_free(msg);
		write_to_at_uart(buff, strlen(buff));
	}
}

/*通过NTP获取世界时间，与3GPP的世界时间做比较，误差超过delta直接断言*/
int check_wall_time(uint32_t delta)
{
	uint64_t sec1,sec2;
	char debug_info[64] = {0};
	ntp_query_param_t arg = {0};

	arg.host = "ntp1.aliyun.com";
	arg.timeout = 20;

	if (0 != query_ntp(&arg))
	{
		xy_printf(0,PLATFORM, WARN_LOG, "error!query_ntp,fail get time!");
		return 0;
	}
	
	if((sec1 = xy_get_UT_ms()/1000) == 0)
	{
		xy_printf(0,PLATFORM, WARN_LOG, "error!xy_get_UT_ms,fail get time!");
		return 0;
	}
	
	sec2 = xy_mktime(&arg.rtctime)/1000 + 8*60*60;

	xy_assert(sec1+delta>sec2 && sec2+delta>sec1);
	
	sprintf(debug_info,"\r\n+DBGINFO:WALLTIME %d %d !\r\n",sec1,sec2);
	send_debug_by_at_uart(debug_info);

	return 1;
}

/**
 * @brief 验证RTC是否能深睡唤醒
 * 
 */
void at_test_rtc_timer_callback(void)
{
	char *urc = xy_malloc(32);
	snprintf(urc, 32, "+WAKEUPBYRTC");
	send_urc_to_ext(urc,strlen(urc));
	xy_free(urc);
}
uint32_t assert_flash = 0;
uint8_t assert_val = 0;

extern uint32_t gMallocFailNum;
/*AT+TEST=*/
int at_TEST_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		char cmd[10] = {0};

		if (at_parse_param("%10s,", at_buf, cmd) != AT_OK)
		{
			return  (ATERR_PARAM_INVALID);
		}

		/*AT+TEST=WDT*/
		if (!strcmp(cmd, "WDT"))
		{
			osCoreEnterCritical();
			while(1);
		}
		/*AT+TEST=WALLTIME,<delta second>*/
		else if (!strcmp(cmd, "WALLTIME"))
		{
			uint32_t delta = 1000;
            at_parse_param(",%d", at_buf,&delta);
			check_wall_time(delta);
		}
		/*AT+TEST=SOCRESET*/
		else if (!strcmp(cmd, "SOCRESET"))
		{
			xy_Soc_Reset();
		}
		/*AT+TEST=SOFTRESET*/
		else if (!strcmp(cmd, "SOFTRESET"))
		{
			xy_Soft_Reset(SOFT_RB_BY_CP_USER);
		}
		else if (!strcmp(cmd, "STACKINFO"))
		{
			/*+Task:线程名，Stat:是否共享栈，Pri:优先级，stack:初始栈大小，usedmax:栈使用峰值*/
			xySatusList_t *pxSatusList;
			StackStatus_t *tmpNode;
			*prsp_cmd = xy_malloc(4000);
			uint16_t str_len = 0;
			uint32_t shareStackPeakValue;

			str_len += sprintf(*prsp_cmd + str_len, "\r\n+2100_STACK_INFO\r\n");
			pxSatusList = xyTaskGetStackInfo();
			tmpNode = pxSatusList->xyListEnd.pxNext;
			while(tmpNode != &(pxSatusList->xyListEnd))
			{
				str_len += sprintf(*prsp_cmd + str_len, "\r\n+taskName:%s,\t isShareStack:%d,\t Pri:%d,\t stackInitialSize:%d,\t stackPeakValue:%d,\t Taskstate:%d\r\n",
						tmpNode->xyTaskName,tmpNode->xyStackInfo.xyStackType,tmpNode->xyPriority,tmpNode->xyStackInfo.xyInitStackSpace,tmpNode->xyStackInfo.xyStackSpace,tmpNode->xyState);
				tmpNode = tmpNode->pxNext;
			}
			vTaskFreeList(pxSatusList);
			shareStackPeakValue = uxTaskGetSharedStackHighWaterMark();
			str_len += sprintf(*prsp_cmd + str_len,"\r\n+shareStackPeakValue:%d\r\n", shareStackPeakValue);
			strcat(*prsp_cmd, "\r\nOK\r\n");
		}

		else if(!strcmp(cmd, "HEAPINFO"))
		{
			HeapInfo_t xyHeapInfo = {0};
			*prsp_cmd = xy_malloc(600);
			uint16_t str_len = 0;

			str_len += sprintf(*prsp_cmd + str_len, "\r\n+2100_HEAP_INFO\r\n");
		    xyTaskGetHeapInfo(&xyHeapInfo);

			str_len += sprintf(*prsp_cmd + str_len,"\r\n Region1HeapSize:%d,\t Region1FreeSize:%d \r\n Region2HeapSize:%d,\t Region2FreeSize:%d \r\n Region3HeapSize:%d,\t Region3FreeSize:%d \r\n\t "
					"Region1AllocatedBlocksNum:%d,\t Region2AllocatedBlocksNum:%d,\t Region3AllocatedBlocksNum:%d,\t Region1StartAdress:%x,\t Region2StartAdress:%x,\t Region3StartAdress:%x,\t "
					"HeapFreeSize:%d,\t HeapMinimumEverFreeSize:%d,\t MallocFail:%d\t\r\n",
					HeapRegions[RegionOne].xSizeInBytes,xyHeapInfo.FreeHeapSize1,HeapRegions[RegionTwo].xSizeInBytes,xyHeapInfo.FreeHeapSize2, HeapRegions[RegionThree].xSizeInBytes,xyHeapInfo.FreeHeapSize3,xyHeapInfo.NumAllocatedBlocks1,xyHeapInfo.NumAllocatedBlocks2,xyHeapInfo.NumAllocatedBlocks3,
				HeapRegions[RegionOne].pucStartAddress, HeapRegions[RegionTwo].pucStartAddress, HeapRegions[RegionThree].pucStartAddress, xyHeapInfo.xyFreeHeapSize, xyHeapInfo.xyMinimumEverFreeBytesRemaining,gMallocFailNum );

			strcat(*prsp_cmd, "\r\nOK\r\n");
		}
#if AT_SOCKET
        else if(!strcmp(cmd, "SOCKINFO"))
        {
            int id = 0;
            if (at_parse_param(",%d", at_buf, &id) != AT_OK)
            {
                return  (ATERR_PARAM_INVALID);
            }
            socket_debug(id);
        }
#endif /* AT_SOCKET */
		else if(!strcmp(cmd, "FOTA"))
        {
			*prsp_cmd = xy_malloc(128);
			int ret = OTA_get_upgrade_result();
			if (ret == XY_OK)
			{
				snprintf(*prsp_cmd, 128, "\r\nUPDATE SUCCESS");
			}
			else 
			{
				snprintf(*prsp_cmd, 128, "\r\nUPDATE FAIL");
			}
		}
        else if(!strcmp(cmd, "ATPARSE"))
        {
            char *p1 = NULL;
            char *p2 = NULL;
            char *p3 = NULL;
            char *p4 = NULL;
            char *p5 = NULL;
            if (at_parse_param(",%p,%p,%p,,%p", at_buf, &p1, &p2, &p3, &p5) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
            *prsp_cmd = xy_malloc(128);
            snprintf(*prsp_cmd, 128, "%s,%s,%s,%s\r\n\r\nOK\r\n", p1,p2,p3,p5);
        }
        else if(!strcmp(cmd, "URC"))
        {
            int mode = 0;
            if (at_parse_param(",%d", at_buf, &mode) != AT_OK)
            {
                return  (ATERR_PARAM_INVALID);
            }
            if (mode == 0)
            {
                osThreadAttr_t attr = {0};
                attr.name = "urc_test";
                attr.priority = osPriorityNormal1;
                osThreadNew((osThreadFunc_t)(urc_cache_test), NULL, &attr);

				osThreadAttr_t attr1 = {0};
                attr1.name = "urc_test1";
                attr1.priority = osPriorityNormal1;
                osThreadNew((osThreadFunc_t)(urc_cache_test1), NULL, &attr1);
            }
            else if (mode == 1)
            {
                osDelay(20000);
            }
        }
		/*具体参阅<芯翼XY1200&2100产品内存使用指导说明>及mpu_protect.c*/
		else if(!strcmp(cmd, "MPU"))
        {
            char sub_cmd[10] = {0};
            if (at_parse_param(",%s", at_buf, sub_cmd) != AT_OK)
            {
                return  (ATERR_PARAM_INVALID);
            }
			/*禁止写RF*/
            if (strstr(sub_cmd, "RF"))
            {
                  xy_Flash_Write(NV_FLASH_RF_BASE + 100,sub_cmd,2);
            }
			/*AT+TEST=MPU,FLASHVAL,<flash addr>,<byte val>    写FLASH时，检查某字节是否为预期值*/
			else if(strstr(sub_cmd, "FLASHVAL"))
			{
				at_parse_param(",,%d,%d", at_buf,&assert_flash,&assert_val);
			}
			/*禁止擦RF*/
			else if(strstr(sub_cmd, "RFE"))
			{
				xy_Flash_Erase(NV_FLASH_RF_BASE + 100, 2);
			}

			/*禁止直接写flash*/
            else if (strstr(sub_cmd, "WFLASH"))
            {
                  *((int *)(NV_FLASH_RF_BASE + 100)) = 2;
            }
			/*禁止写代码段*/
            else if (strstr(sub_cmd, "WCODE"))
            {
            	void *code_addr = (void *)at_TEST_req;
                xy_Flash_Write(code_addr,sub_cmd,2);
            }
			/*CP核的RAM的数据段不可执行，可读可写*/
			else if (strstr(sub_cmd, "RUNDATA"))
            {
            	void *code_addr = xy_malloc(10);
                ((ser_req_func)(code_addr))(NULL,NULL);
            }			
			/*CP核的0地址不可运行*/
			else if (strstr(sub_cmd, "RUNZERO"))
            {
            	void *code_addr = NULL;
                ((ser_req_func)(code_addr))(NULL,NULL);
            }
			/*CP核的0地址不可赋值*/
			else if (strstr(sub_cmd, "WZERO"))
            {
            	int *code_addr = NULL;
                *code_addr = 0;
            }
			/*中断向量表不可执行或写，只读*/
			else if (strstr(sub_cmd, "RUNIVT"))
            {
            	int *code_addr = (int *)0X10000000;
                *code_addr = 2;
            }
            else           
            {
                return  (ATERR_PARAM_INVALID);
            }
        }
		/*AT+TEST=FLASH,<addr>,<len>   后台代理线程执行擦写FLASH，可以构造AP的冲突*/
		else if(!strcmp(cmd, "FLASH"))
		{
			flash_test_msg_t *msg = xy_malloc(sizeof(flash_test_msg_t));
			memset(msg, 0, sizeof(flash_test_msg_t));
			if (at_parse_param(",%d,%d", at_buf, &msg->base, &msg->size) != AT_OK)
				return ATERR_PARAM_INVALID;	

			if (g_flash_test_TskHandle == NULL)
			{
				osThreadAttr_t thread_attr = {0};
				thread_attr.name = "test_flash_task";				
				thread_attr.stack_size = osStackShared;
				thread_attr.priority = osPriorityBelowNormal;
				g_flash_test_msg_q = osMessageQueueNew(10, sizeof(void *), NULL);
				g_flash_test_TskHandle = osThreadNew((osThreadFunc_t)flash_test_entry, NULL, &thread_attr);
			}
			osMessageQueuePut(g_flash_test_msg_q, (void *)(&msg), 0, osNoWait); 
			xy_printf(0, XYAPP, WARN_LOG, "test flash start:%X %X %X", msg->base, msg->size, g_flash_test_TskHandle);
		}
		/*AT+TEST=NV,...用于FLASH中NV区域的FLASH操作测试*/
		else if (!strcmp(cmd, "NV"))
        {
        	uint32_t type = -1;
			uint32_t val1 = -1;
			uint32_t val2 = -1;
			uint32_t val3 = -1;
			
            if (at_parse_param(",%d,%d,%d,%d", at_buf,&type,&val1,&val2,&val3) != AT_OK)
				return ATERR_PARAM_INVALID;	
			/*AT+TEST=NV,0,<address>,<val>   写FLASH 4字节*/
			if(type == 0)
				xy_Flash_Write(val1,(void *)val2,4);
			/*AT+TEST=NV,1,<address>,<len>   擦除*/
			else if(type == 1)
				xy_Flash_Erase(val1,val2);
			/*AT+TEST=NV,2,<address>,<len>,<dest address>   搬移FLASH块*/
			else if(type == 2)
			{
				if(val2 == -1 || val2 == 0)
					val2 = 0X1000;

				void *data = xy_malloc(val2);
				xy_Flash_Read(val1,data,val2);
				xy_Flash_Erase(val3,val2);
				xy_Flash_Write(val3,(void *)data,val2);
				xy_free(data);
			}
			/*AT+TEST=NV,3,<address>,<len>,<dest address>   两个FLASH块内容比较*/
			else if(type == 3)
			{
				if(val2 == -1 || val2 == 0)
					val2 = 0X1000;

				void *data = xy_malloc(val2);
				void *data1 = xy_malloc(val2);
				xy_Flash_Read(val1,data,val2);
				xy_Flash_Read(val3,data,val2);
				if(memcmp(data,data1,val2) != 0)
				{
					xy_free(data);
					xy_free(data1);
					return ATERR_XY_ERR;
				}
				xy_free(data);
				xy_free(data1);
			}
			/*AT+TEST=NV,4,<address>,<len>   读FLASH块内容，以十六进制格式输出*/
			else if(type == 4)
			{
				if(val2 == -1 || val2 == 0)
					val2 = 0X1000;

				void *data = xy_malloc(val2);
				void *data1 = xy_malloc(val2*2+1);
				
				xy_Flash_Read(val1,data,val2);
				bytes2hexstr(data,val2,data1,val2*2+1);
				send_urc_to_ext2(data1);
				xy_free(data);
				xy_free(data1);
			}
        }
        else if (!strcmp(cmd, "PSAPI"))
        {
            if(xy_ps_api_test(at_buf, prsp_cmd) != AT_OK)
            {
                return ATERR_PARAM_INVALID;
            }
        }
		else if (!strcmp(cmd, "RTC"))//设置非周期性RTC
		{
			int32_t time = -1;
			if (at_parse_param(",%d(0-)", at_buf, &time) != AT_OK)
				return ATERR_PARAM_INVALID;
			rtc_event_add_by_offset(RTC_TIMER_CP_USER1, time, at_test_rtc_timer_callback, 0);
		}
	}
	else
        return  (ATERR_PARAM_INVALID);
    return AT_END;
}

/*AT+CPREGTEST，读写寄存器/RAM/FLASH等内存空间，查询系统异常事件*/
int at_REGTEST_req(char *param,char **rsp_cmd)
{
	unsigned char cmd[16] = {0};

	unsigned int regAddr = 0;
	unsigned int value = 0;

	if (at_parse_param("%15s,%d,%d", param, cmd,&regAddr,&value) != AT_OK)
		return ATERR_PARAM_INVALID;


	/*AT+CPREGTEST=GET,<regAddr>,<len>*/
	if(!strcmp((const char *)(cmd), "GET"))
	{
		*rsp_cmd = xy_malloc(150);
		if(value==0 || value==4)
			sprintf(*rsp_cmd, "0x%X\r\n\r\nOK\r\n", (int)HWREG(regAddr));
		else if(value == 1)
			sprintf(*rsp_cmd, "0x%X\r\n\r\nOK\r\n", (int)HWREGB(regAddr));
		else if(value == 2)
			sprintf(*rsp_cmd, "0x%X\r\n\r\nOK\r\n", (int)HWREGH(regAddr));
		else 
		{
			xy_free(*rsp_cmd);
			*rsp_cmd = xy_malloc(value*2+30);
			
			bytes2hexstr((uint8_t *)regAddr,value, *rsp_cmd, value*2+1);
			strcat(*rsp_cmd, "\r\n\r\nOK\r\n");
		}
	}
	
	/*AT+CPREGTEST=SET,<regAddr>,<val>*/
	else if(!strcmp((const char *)(cmd), "SET"))
	{
		HWREG(regAddr) = value;
	}
	/*AT+CPREGTEST=CLEAR,regAddr*/
	else if(!strcmp((const char *)(cmd), "CLEAR"))
	{
		HWREG(regAddr) = 0x0;
	}
	/*AT+CPREGTEST=UP*/
	else if(!strcmp((const char *)(cmd), "UP"))
	{
		extern void print_up_dbg_info();
		print_up_dbg_info();
	}
	/*AT+CPREGTEST=DL*/
	else if(!strcmp((const char *)(cmd), "DL"))
	{
		extern void print_ver_download_info();
		print_ver_download_info();
	}
	/*AT+CPREGTEST=SOC*/
	else if(!strcmp((const char *)(cmd), "SOC"))
	{
		*rsp_cmd = xy_malloc(128);
		char version[2] = {0};//0:模组

		if(Is_OpenCpu_Ver())
		{
			strcpy(version, "O");
		}
		else
		{
			strcpy(version, "M");
		}

		/*0;XY1200L;1:XY1200SL;2:XY2100SL;3:XY1200;4:XY1200S;5:XY2100S*/
		sprintf(*rsp_cmd, "+SOCTYPE:%d(0:XY1200L;1:XY1200SL;2:XY2100SL;3:XY1200;4:XY1200S;5:XY2100S;6:1200E;7:1200E+)\r\n+OPENCPU:%s\r\nOK\r\n",get_Soc_ver(), version);
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}

	return AT_OK;
}


/**********************************RTC测试代码区域**********************************/
#include "xy_rtc_api.h"

#define RTC_TEST_INFO_BASE            (USER_BAK_MEM_BASE+0x100)
#define RTC_TEST_DAYSET_INFO_BASE     RTC_TEST_INFO_BASE
#define RTC_TEST_DAYSET_INFO_LEN      0x10
#define RTC_TEST_WEEKSET_INFO_BASE    (RTC_TEST_DAYSET_INFO_BASE + RTC_TEST_DAYSET_INFO_LEN)

typedef struct
{
	uint32_t sec_offset1;
	uint32_t sec_offset2;
	uint32_t sec_offset3;
	uint32_t sec_offset4;
} rtc_test_time_t;

// 深睡状态下，retmem区域可能断电，内部会做RTC链表恢复处理，但此用户区域内容不做恢复，打印可能会出错，此时只需关注RTC是否准时
rtc_test_time_t *g_rtc_test_time = (rtc_test_time_t *)USER_BAK_MEM_BASE;

// 用随机数与ID5产生随机碰撞
void rtc_test_callback1(void)
{
	xy_printf(0, XYAPP, WARN_LOG, "%d", g_rtc_test_time->sec_offset1);
	g_rtc_test_time->sec_offset1 = (uint16_t)xy_rand() + 11;
	rtc_event_add_by_offset(RTC_TIMER_CP_USER1, g_rtc_test_time->sec_offset1, rtc_test_callback1, 0);
}

// 周期性RTC，固定与ID4产生碰撞
void rtc_test_callback2(void)
{
	xy_printf(0, XYAPP, WARN_LOG, "%d%d%d%d%d%d%d%d", g_rtc_test_time->sec_offset2,  xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER2), \
			g_rtc_test_time->sec_offset1, xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER1), g_rtc_test_time->sec_offset3, xy_rtc_next_offset_by_ID(RTC_TIMER_CDP), \
			g_rtc_test_time->sec_offset4, xy_rtc_next_offset_by_ID(RTC_TIMER_ONENET));

	xy_wall_clock_t time = {0};
	if (Get_Current_UT_Time(&time))
		xy_printf(0, XYAPP, WARN_LOG, "%d%d%d%d%d%d", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);			
	else
		xy_printf(0, XYAPP, WARN_LOG, "%d", osKernelGetTickCount());
}

// 考虑到flash磨损且测flash耗时比较长，此周期性RTC超时时长会设置长一点
void rtc_test_callback3(void)
{
	xy_printf(0, XYAPP, WARN_LOG, "%d", g_rtc_test_time->sec_offset2);
	// test_flash_msg_t msg = {0};
	// msg.base = (g_flash_sync->ap_working) ? TEST_FLASH_BASE1 : TEST_FLASH_BASE2;;
	// msg.size = 4096;
	// msg.num  = 10;
		
	// test_flash_example(&msg);	
}

void rtc_test_callback4(void)
{
	xy_printf(0, XYAPP, WARN_LOG, "%d", g_rtc_test_time->sec_offset4);
	g_rtc_test_time->sec_offset4 = (uint16_t)xy_rand();
	rtc_event_add_by_offset(RTC_TIMER_ONENET, g_rtc_test_time->sec_offset4, rtc_test_callback4, 0);
}

// 添加多个RTC事件, 云业务相关RTC只有做业务时才用，目前先占用一下用于RTC测试
// RTC精度为10ms，超时阈值为11ms
// 暂不测试：目前RTC线程优先级较高，且理论上不会出现比RTC线程优先级更高的线程调用RTC相关的API，若出现，需考虑RTC在超时处理环节里同时被高优先级删的情况
void user_rtc_test_init(int num, int time_ms)
{
	g_rtc_test_time->sec_offset1 = time_ms;
	g_rtc_test_time->sec_offset2 = time_ms + 11;
	g_rtc_test_time->sec_offset3 = (time_ms + 11) * 10;
	g_rtc_test_time->sec_offset4 = time_ms + 20 * 1000;

	rtc_event_add_by_offset(RTC_TIMER_CP_USER1, g_rtc_test_time->sec_offset1, rtc_test_callback1, 0);
	rtc_event_add_by_offset(RTC_TIMER_CP_USER2, g_rtc_test_time->sec_offset2, rtc_test_callback2, 1);
	rtc_event_add_by_offset(RTC_TIMER_CDP, g_rtc_test_time->sec_offset3, rtc_test_callback3, 1);
	rtc_event_add_by_offset(RTC_TIMER_ONENET, g_rtc_test_time->sec_offset4, rtc_test_callback4, 0);
	if (!num)
	{
		xy_rtc_timer_delete(RTC_TIMER_CP_USER1);
		xy_rtc_timer_delete(RTC_TIMER_CP_USER2);
		xy_rtc_timer_delete(RTC_TIMER_CDP);
		xy_rtc_timer_delete(RTC_TIMER_ONENET);
	}
	xy_printf(0, XYAPP, WARN_LOG, "%d%d%d%d%d", num, g_rtc_test_time->sec_offset1, g_rtc_test_time->sec_offset2, g_rtc_test_time->sec_offset3, g_rtc_test_time->sec_offset4);
}

void rtc_test_callback_day(void)
{
	int hour_start = HWREG(RTC_TEST_DAYSET_INFO_BASE);
	int min = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x04);
	int sec = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x08);
	int sec_span = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x0c);

	xy_rtc_set_by_day(RTC_TIMER_CP_USER1, rtc_test_callback_day, hour_start*60*60+min*60+sec, sec_span);
}

void rtc_test_callback_week(void)
{
	int day_week = HWREG(RTC_TEST_DAYSET_INFO_BASE);
	int hour_start = HWREG(RTC_TEST_DAYSET_INFO_BASE +0x04);
	int min = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x08);
	int sec = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x0c);
	int sec_span = HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x10);

	xy_rtc_set_by_week(RTC_TIMER_CP_USER2, rtc_test_callback_week, day_week, hour_start*3600+min*60+sec, sec_span);
}

int at_RTC_req(char *at_buf, char **prsp_cmd)
{
	char cmd[10] = {0};
	*prsp_cmd = xy_malloc(60);

	if (at_parse_param("%10s", at_buf, cmd) != AT_OK)
		return ATERR_PARAM_INVALID;

	if (g_req_type == AT_CMD_REQ)
	{
		if (!strcmp(cmd, "SINGLE"))
		{
			int timer_id = 0;
			int rtc_reload = 0;
			int time_ms = 0;

			if (at_parse_param(",%d,%d,%d", at_buf, &timer_id, &rtc_reload, &time_ms) != AT_OK)
				return ATERR_PARAM_INVALID;

			if(rtc_reload == 0)   
			{
				if(time_ms == 0)  //单个，非周期性，随机时长
				{
					time_ms = (uint16_t)xy_rand() + 11;
					rtc_event_add_by_offset(timer_id, time_ms, NULL, RTC_NOT_RELOAD_FLAG);
					snprintf(*prsp_cmd, 60, "\r\n+RTC:%d, %d\r\n\r\nOK\r\n", timer_id, xy_rtc_next_offset_by_ID(timer_id));
				}
				else   //单个，非周期性，定时
				{
					rtc_event_add_by_offset(timer_id, time_ms, NULL, RTC_NOT_RELOAD_FLAG);
					snprintf(*prsp_cmd, 60, "\r\n+RTC:%d, %d\r\n\r\nOK\r\n", timer_id, xy_rtc_next_offset_by_ID(timer_id));
				}
			}
			if(rtc_reload == 1)   
			{
				if(time_ms == 0)  //单个，周期性，随机时长
				{
					time_ms = (uint16_t)xy_rand() + 11;
					rtc_event_add_by_offset(timer_id, time_ms, NULL, RTC_PERIOD_AUTO_RELOAD_FLAG);
					snprintf(*prsp_cmd, 60, "\r\n+RTC:%d, %d\r\n\r\nOK\r\n", timer_id, xy_rtc_next_offset_by_ID(timer_id));
				}
				else   //单个，周期性，定时
				{
					rtc_event_add_by_offset(timer_id, time_ms, NULL, RTC_PERIOD_AUTO_RELOAD_FLAG);
					snprintf(*prsp_cmd, 60, "\r\n+RTC:%d, %d\r\n\r\nOK\r\n", timer_id, xy_rtc_next_offset_by_ID(timer_id));
				}
			}
		}

		if (!strcmp(cmd, "DAY"))
		{
			int hour_start = 0;  //0-23
			int min = 0;  //0-59
			int sec = 0;  //0-59
			int sec_span = 0;  //always >0

			if (at_parse_param(",%d,%d,%d,%d", at_buf, &hour_start, &min, &sec, &sec_span) != AT_OK)
				return ATERR_PARAM_INVALID;

			HWREG(RTC_TEST_DAYSET_INFO_BASE) = hour_start;
			HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x04) = min;
			HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x08) = sec;
			HWREG(RTC_TEST_DAYSET_INFO_BASE + 0x0c) = sec_span;

			xy_rtc_set_by_day(RTC_TIMER_CP_USER1, rtc_test_callback_day, hour_start*60*60+min*60+sec, sec_span);
			snprintf(*prsp_cmd, 60, "\r\n+RTC:%d\r\n\r\nOK\r\n", xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER1));
		}

		if (!strcmp(cmd, "WEEK"))
		{
			int day_week = 0;   //1-7
			int hour_start = 0;  //0-23
			int min = 0;  //0-59
			int sec = 0;  //0-59
			int sec_span = 0;  //always >0

			if (at_parse_param(",%d,%d,%d,%d,%d", at_buf, &day_week, &hour_start, &min, &sec, &sec_span) != AT_OK)
				return ATERR_PARAM_INVALID;

			HWREG(RTC_TEST_WEEKSET_INFO_BASE) = day_week;
			HWREG(RTC_TEST_WEEKSET_INFO_BASE + 0x04) = hour_start;
			HWREG(RTC_TEST_WEEKSET_INFO_BASE + 0x08) = min;
			HWREG(RTC_TEST_WEEKSET_INFO_BASE + 0x0c) = sec;
			HWREG(RTC_TEST_WEEKSET_INFO_BASE + 0x10) = sec_span;

			xy_rtc_set_by_week(RTC_TIMER_CP_USER2, rtc_test_callback_week, day_week,hour_start*3600+min*60+sec, sec_span);
			snprintf(*prsp_cmd, 60, "\r\n+RTC:%d\r\n\r\nOK\r\n", xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER2));
		}

		if (!strcmp(cmd, "CLEAR"))
		{
			int timer_id = 0;
			if (at_parse_param(",%d", at_buf, &timer_id) != AT_OK)
				return ATERR_PARAM_INVALID;

            xy_rtc_timer_delete(timer_id);
			snprintf(*prsp_cmd, 60, "\r\n+RTC:%d, %d\r\n\r\nOK\r\n", timer_id, xy_rtc_next_offset_by_ID(timer_id));
		}

		if (!strcmp(cmd, "RESET"))
		{
			reset_cp_rtcinfo();
			snprintf(*prsp_cmd, 60, "\r\n+RTC:all clear\r\n\r\nOK\r\n");
		}

		if (!strcmp(cmd, "MIX"))
		{
			// 参数：RTC个数,RTC超时时间
			/* AT+RTC=<test_num>,<time_ms> */
			int num = 0;
			int time_ms = 0;

			if(at_parse_param(",%d,%d", at_buf, &num, &time_ms) != AT_OK)
				return ATERR_PARAM_INVALID;

			user_rtc_test_init(num, time_ms);

			snprintf(*prsp_cmd, 60, "\r\n+RTC:%d,%d,%d,%d\r\n\r\nOK\r\n", xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER1), \
			xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER2), xy_rtc_next_offset_by_ID(RTC_TIMER_ONENET), xy_rtc_next_offset_by_ID(RTC_TIMER_CDP));
		}
	}
	else if (g_req_type == AT_CMD_QUERY)
	{
		snprintf(*prsp_cmd, 60, "\r\n+RTC:%d,%d,%d,%d,%d,%d,%d\r\n\r\nOK\r\n", xy_rtc_next_offset_by_ID(RTC_TIMER_CP_LPM), xy_rtc_next_offset_by_ID(RTC_TIMER_CMCCDM), xy_rtc_next_offset_by_ID(RTC_TIMER_CTWING), \
		xy_rtc_next_offset_by_ID(RTC_TIMER_ONENET), xy_rtc_next_offset_by_ID(RTC_TIMER_CDP), xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER1), xy_rtc_next_offset_by_ID(RTC_TIMER_CP_USER2));
	}
	else
		return ATERR_PARAM_INVALID;
	
	return AT_END;
}

extern void NL1cRLMSetOffset(int16_t sOutOfSyncOffset, int16_t sInSyncOffset);
extern void PHY_RXAGC_FixedGain(int16_t s16Enable, int16_t s16GainIdx);
extern void PhyNL1cGetMeasRslt(int16_t *sSinr, int16_t *sNrsrp, int16_t *sRssi);
extern void DFE_BBPLL_K_Value_Fixed(int flag, int value);
extern void PhyDumpDataFixedRxAGC(uint32_t u32DLFrequency, uint8_t u8FixedAgcIdx);

int at_PHY_req(char *at_buf, char **prsp_cmd)
{
   char module[10] = {0};
   int para1 = 0;
   int para2 = 0;
   int para3 = 0;
   int para4 = 0;
   
   if (at_parse_param("%10s,%d,%d,%d,%d", at_buf, module, &para1, &para2, &para3, &para4) != AT_OK)
   {
       return  (ATERR_PARAM_INVALID);
   }
   
   if (strstr(module, "DFE"))
   {
		DFE_BBPLL_K_Value_Fixed(para1, para2);
   }
   else if (strstr(module, "DUMP"))
   {	//frequency, gainidx
   		PhyDumpDataFixedRxAGC(para1, para2);
   }
   else if (strstr(module, "MGCIDX"))
   {
	   PHY_RXAGC_FixedGain(para1, para2);
   }
   else if (strstr(module, "MGCTAR"))
   {
       if (para1 < 0)
       {
           para1 = 0;
       }
       else if (para1 > 4)
       {
           para1 = 4;
       }
       else
       {
       }

       // para1 is index
       // para2 is Gain in 0.1dB (no error protection)
       //assignLocalGainMGC(para1, para2);
   }
   else if (strstr(module, "CR0499"))
   {
       //NL1cRandomInterferDisable((uint8_t)para1);
   }
   else if (strstr(module, "RLM"))
   {
       NL1cRLMSetOffset(para1, para2);
   }
   else if (strstr(module, "RACHFAIL"))
   {
       //rach_fail = 1;
   }
   else if (strstr(module, "MEASRslt"))
   {
   	   int16_t sRsrp, sRssi, sSinr;
       char * result;
       result = xy_malloc(100);

	   PhyNL1cGetMeasRslt(&sSinr, &sRsrp, &sRssi);
       sprintf(result,"\r\nMEASRslt,%d,%d,%d\r\n", sRsrp, sRssi, sSinr);

       write_to_at_uart(result, strlen(result));
       xy_free(result);
   }
   else if(strstr(module, "DMASEGLEN"))
   {
   	 PHY_SET_DMALEN_INFO(para1);
   }
   return AT_END;
}

