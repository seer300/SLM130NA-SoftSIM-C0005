/**
 * @file
 * @brief 用于在RUNINFO_DEBUG_ADDR空间中保存FOTA/IMEI/校准的时间戳信息，以供产品DEBUG时获取背景信息
 * 
 */

#include "xy_flash.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "xy_timer.h"
#include "xy_system.h"
#include "at_uart.h"
#include "timestamp.h"

#define CP_DEBUG_ADDR              (RUNINFO_DEBUG_ADDR+0)
#define AP_DEBUG_ADDR              (RUNINFO_DEBUG_ADDR+0X400)
#define VER_DOWNLOAD_DEBUG_ADDR    (RUNINFO_DEBUG_ADDR+0X800)


/*复用FOTA区域，FOTA升级会擦除*/
typedef struct 
{
	uint32_t  soft_reset_sec;    //最近一次软重启的时刻点
	uint32_t  soft_reset_sub;    //最近一次软重启子原因
	uint32_t  soft_reset_sum;    //曾经发生软重启的次数
	uint32_t  soc_reset_sum;     //SOC_RESET的次数
	uint32_t  pin_reset_sum;     //PIN_RESET的次数
	uint32_t  wdt_reset_sum;     //WATCHDOG_RESET的次数
	uint32_t  other_reset_sum;   //其他情况全局复位的次数
	uint32_t  power_on_sum;      //断电上电的次数
}dbg_info_t;



__FLASH_FUNC void save_up_dbg_into(int reason,int subreason)
{
	uint32_t sec=0;
	RTC_TimeTypeDef rtc_time = {0};
	dbg_info_t info = {0};


	if(reason == WAKEUP_DSLEEP)
		return;
	
	/*模组形态断电上电为常规操作*/
#if (MODULE_VER != 0x0)
	if(reason == POWER_ON)
		return;
#endif

	xy_Flash_Read(AP_DEBUG_ADDR,&info,sizeof(dbg_info_t));

	if(reason == SOFT_RESET)
	{
		if(info.soft_reset_sum == 0XFFFFFFFF)
			info.soft_reset_sum = 1;
		else
			info.soft_reset_sum++;

		info.soft_reset_sub = subreason;

		if (Get_Current_UT_Time(&rtc_time))
		{
			sec = xy_mktime(&rtc_time) / 1000;
			
			info.soft_reset_sec = sec;
		}
		else
		{
			info.soft_reset_sec = 0;
		}
	}
	else if(reason == GLOBAL_RESET)
	{
		if(subreason == SOC_RESET)
		{
			if(info.soc_reset_sum == 0XFFFFFFFF)
				info.soc_reset_sum = 1;
			else
				info.soc_reset_sum++;
		}
		else if(subreason == PIN_RESET)
		{
			if(info.pin_reset_sum == 0XFFFFFFFF)
				info.pin_reset_sum = 1;
			else
				info.pin_reset_sum++;
		}
		else if(subreason == WDT_RESET)
		{
			if(info.wdt_reset_sum == 0XFFFFFFFF)
				info.wdt_reset_sum = 1;
			else
				info.wdt_reset_sum++;
		}
		else
		{
			if(info.other_reset_sum == 0XFFFFFFFF)
				info.other_reset_sum = 1;
			else
				info.other_reset_sum++;
		}
	}
	else if(reason == POWER_ON)
	{
		if(info.power_on_sum == 0XFFFFFFFF)
			info.power_on_sum = 1;
		else
			info.power_on_sum++;
	}
	xy_assert(xy_Flash_Write(AP_DEBUG_ADDR,&info,sizeof(dbg_info_t)) != false);
}


typedef struct 
{
	uint32_t  fota_sec;     //最近一次FOTA的时刻点
	uint32_t  rf_sum;       //RF擦写的次数
	uint32_t  rf_sec;       //最近一次RF擦的时刻点
	uint32_t  boot_sum;     //OPENCPU形态，AP核动态加载CP核的次数
	uint32_t  boot_sec;     //OPENCPU形态，最近一次动态加载CP核的时刻点
}cp_dbg_info_t;

/*复用FOTA区域，FOTA升级会擦除*/
__FLASH_FUNC void print_cp_dbg_info()
{
	RTC_TimeTypeDef rtc_time = {0};
	cp_dbg_info_t info = {0};
	char rsp[80] = {0};
	
	xy_Flash_Read(CP_DEBUG_ADDR,&info,sizeof(cp_dbg_info_t));

	if(info.fota_sec!=0 && info.fota_sec!=0XFFFFFFFF)
	{
		xy_gmtime((uint64_t)info.fota_sec*1000,&rtc_time);
		sprintf(rsp,"\r\nFOTA Time=%lu/%lu-%lu:%lu:%lu\r\n",rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		Send_AT_to_Ext(rsp);	
	}

	if(info.rf_sum!=0 && info.rf_sum!=0XFFFFFFFF)
	{
		if(info.rf_sec!=0 && info.rf_sec!=0XFFFFFFFF)
		{
			xy_gmtime((uint64_t)info.rf_sec*1000,&rtc_time);
			sprintf(rsp,"\r\nRF nv opt %lu times!Time=%lu/%lu-%lu:%lu:%lu\r\n",info.rf_sum,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		}
		else
			sprintf(rsp,"\r\nRF nv opt %lu times!\r\n",info.rf_sum);
		Send_AT_to_Ext(rsp);	
	}
	

	/*仅OPENCPU形态才会有这个统计值*/
	if(info.boot_sum != 0XFFFFFFFF)
	{
		memset((void *)&rtc_time,0,sizeof(RTC_TimeTypeDef));
			
		if(info.boot_sec!=0 && info.boot_sec!=0XFFFFFFFF)
			xy_gmtime((uint64_t)info.boot_sec*1000,&rtc_time);
		
		sprintf(rsp,"\r\nBoot_cp sum=%lu,Time=%lu/%lu-%lu:%lu:%lu\r\n",info.boot_sum,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		Send_AT_to_Ext(rsp);	
	}
}

typedef struct 
{
	uint8_t  day;       
	uint8_t  mon;       
	uint16_t year;       
	uint8_t  min;     
	uint8_t  hour;   
	uint16_t padding;   
}ver_dbg_info_t;

/*版本烧录工具写烧录时刻点，通过AT命令读取时刻点*/
__FLASH_FUNC void print_ver_download_info()
{
	ver_dbg_info_t info = {0};
	char rsp[80] = {0};
	
	xy_Flash_Read(VER_DOWNLOAD_DEBUG_ADDR,&info,sizeof(ver_dbg_info_t));

	sprintf(rsp,"\r\nVer download Time=%x/%x/%x-%x:%x\r\n",info.year,info.mon,info.day,info.hour,info.min);
	Send_AT_to_Ext(rsp);	

	sprintf(rsp,"\r\nVer Compile Time = %s\r\n", BUILD_BRIEF_TIMESTAMP);
	Send_AT_to_Ext(rsp);
}


/*AT+APTEST=UP.  复用FOTA区域，FOTA升级会擦除*/
__FLASH_FUNC void print_up_dbg_info()
{
	RTC_TimeTypeDef rtc_time = {0};
	dbg_info_t info = {0};
	char rsp[80] = {0};
	
	xy_Flash_Read(AP_DEBUG_ADDR,&info,sizeof(dbg_info_t));

	if(info.soft_reset_sum != 0XFFFFFFFF)
	{
		memset((void *)&rtc_time,0,sizeof(RTC_TimeTypeDef));
		if(info.soft_reset_sec!=0 && info.soft_reset_sec!=0XFFFFFFFF)
			xy_gmtime((uint64_t)info.soft_reset_sec*1000,&rtc_time);
		
		sprintf(rsp,"\r\nSOFT_RESET=%lu,Time=%lu/%lu-%lu:%lu:%lu\r\n",info.soft_reset_sum,rtc_time.tm_mon,rtc_time.tm_mday,rtc_time.tm_hour,rtc_time.tm_min,rtc_time.tm_sec);
		Send_AT_to_Ext(rsp);	
	}

	if(info.soc_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+SOC_RESET(CP_WDT/assert)=%lu\r\n",info.soc_reset_sum);
		Send_AT_to_Ext(rsp);	
	}
	if(info.pin_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+PIN_RESET=%lu\r\n",info.pin_reset_sum);
		Send_AT_to_Ext(rsp);	
	}
	if(info.wdt_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+WDT_RESET(AP_WDT/UTC_WDT)=%lu\r\n",info.wdt_reset_sum);
		Send_AT_to_Ext(rsp);	
	}
	if(info.other_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+other_RESET=%lu\r\n",info.other_reset_sum);
		Send_AT_to_Ext(rsp);	
	}
	if(info.power_on_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nPOWER_ON = %lu\r\n",info.power_on_sum);
		Send_AT_to_Ext(rsp);	
	}

	print_cp_dbg_info();
	print_ver_download_info();
}


