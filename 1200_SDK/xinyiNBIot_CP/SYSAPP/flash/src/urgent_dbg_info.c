/**
 * @file
 * @brief 用于在RUNINFO_DEBUG_ADDR空间中保存FOTA/IMEI/校准的时间戳信息，以供产品DEBUG时获取背景信息
 * 
 */

#include "xy_flash.h"
#include "xy_utils.h"
#include "xy_at_api.h"
#include "oss_nv.h"
#include "xy_memmap.h"
#include "ipc_msg.h"
#include "mpu_protect.h"
#include "xy_rtc_api.h"


#define CP_DEBUG_ADDR              (RUNINFO_DEBUG_ADDR+0)
#define AP_DEBUG_ADDR              (RUNINFO_DEBUG_ADDR+0X400)
#define VER_DOWNLOAD_DEBUG_ADDR    (RUNINFO_DEBUG_ADDR+0X800)

typedef struct 
{
	uint32_t  fota_sec;     //最近一次FOTA的时刻点
	uint32_t  rf_sum;       //软件进行RF擦写的次数
	uint32_t  rf_sec;       //最近一次RF擦的时刻点
	uint32_t  boot_sum;     //OPENCPU形态，AP核动态加载CP核的次数
	uint32_t  boot_sec;     //OPENCPU形态，最近一次动态加载CP核的时刻点
}cp_dbg_info_t;

#define OFFSET_PARAM(param)     ((uint32_t) & (((cp_dbg_info_t *)0)->param))
#define PARAM_LEN(param)        (sizeof(((cp_dbg_info_t *)0)->param))

#define SAVE_DBG_INFO(param,val)	xy_Flash_Write(CP_DEBUG_ADDR + OFFSET_PARAM(param), (uint8_t *)&val, PARAM_LEN(param))

#define READ_DBG_INFO(type,param) (*(type *)(CP_DEBUG_ADDR + (uint32_t)(&((cp_dbg_info_t *)0)->param)))




/*RF的擦写轨迹记录*/
void save_rf_dbg_into(void *param,int len)
{
	uint32_t sec = 0;
	RTC_TimeTypeDef rtc_time = {0};
	cp_dbg_info_t info = {0};
	UNUSED_ARG(param);
	UNUSED_ARG(len);

	if(Is_OpenCpu_Ver())
		return;
	
	xy_Flash_Read(CP_DEBUG_ADDR,&info,sizeof(cp_dbg_info_t));
	
	if(info.rf_sum == 0XFFFFFFFF)
		info.rf_sum = 1;
	else
		info.rf_sum++;

	if (Get_Current_UT_Time(&rtc_time.wall_clock))
	{
		sec = xy_mktime(&rtc_time) / 1000;
		
		info.rf_sec = sec;
	}
	
	xy_Flash_Write(CP_DEBUG_ADDR,&info,sizeof(cp_dbg_info_t));
}

/*记录FOTA准备升级的时刻点*/
void save_fota_dbg_into(void *param,int len)
{
	uint32_t sec;
	RTC_TimeTypeDef rtc_time = {0};
	UNUSED_ARG(param);
	UNUSED_ARG(len);

	if(Is_OpenCpu_Ver())
		return;
	
	if (Get_Current_UT_Time(&rtc_time.wall_clock))
	{
		sec = xy_mktime(&rtc_time) / 1000;
		
		SAVE_DBG_INFO(fota_sec,sec);
	}
}

void boot_cp_dbg_into()
{
	if(Is_OpenCpu_Ver())
		return;
	
	/*OPENCPU形态，AP核通过stop_CP强行停CP后再动态启停CP，记录时刻点*/
	if(Is_OpenCpu_Ver() && Get_Boot_Reason()==POWER_ON && Get_Boot_Sub_Reason()==1)
	{
		uint32_t sec;
		RTC_TimeTypeDef rtc_time = {0};
		uint32_t sum = READ_DBG_INFO(uint32_t,boot_sum);

		if(sum == 0XFFFFFFFF)
			sum = 1;
		else
			sum++;

		SAVE_DBG_INFO(boot_sum,sum);
		
		if (Get_Current_UT_Time(&rtc_time.wall_clock))
		{
			sec = xy_mktime(&rtc_time) / 1000;
			
			SAVE_DBG_INFO(boot_sec,sec);
		}
	}
}



/****************************************************************************************
* 以下为AP核的debug信息及读取实现
****************************************************************************************/

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



/*复用FOTA区域，FOTA升级会擦除*/
__FLASH_FUNC void print_cp_dbg_info()
{
	RTC_TimeTypeDef rtc_time = {0};
	cp_dbg_info_t info = {0};
	char rsp[80] = {0};
	
	xy_Flash_Read(CP_DEBUG_ADDR,&info,sizeof(cp_dbg_info_t));

	if(info.fota_sec!=0 && info.fota_sec!=0XFFFFFFFF)
	{
		xy_gmtime_r((uint64_t)info.fota_sec*1000,&rtc_time);
		sprintf(rsp,"\r\nFOTA Time=%lu/%lu-%lu:%lu:%lu\r\n",rtc_time.wall_clock.tm_mon,rtc_time.wall_clock.tm_mday,rtc_time.wall_clock.tm_hour,rtc_time.wall_clock.tm_min,rtc_time.wall_clock.tm_sec);
		send_urc_to_ext(rsp,strlen(rsp));	
	}

	if(info.rf_sum!=0 && info.rf_sum!=0XFFFFFFFF)
	{
		if(info.rf_sec!=0 && info.rf_sec!=0XFFFFFFFF)
		{
			xy_gmtime_r((uint64_t)info.rf_sec*1000,&rtc_time);
			sprintf(rsp,"\r\nRF nv opt %lu times!Time=%lu/%lu-%lu:%lu:%lu\r\n",info.rf_sum,rtc_time.wall_clock.tm_mon,rtc_time.wall_clock.tm_mday,rtc_time.wall_clock.tm_hour,rtc_time.wall_clock.tm_min,rtc_time.wall_clock.tm_sec);
		}
		else
			sprintf(rsp,"\r\nRF nv opt %lu times!\r\n",info.rf_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}
	

	/*仅OPENCPU形态才会有这个统计值*/
	if(info.boot_sum != 0XFFFFFFFF)
	{
		memset((void *)&rtc_time,0,sizeof(RTC_TimeTypeDef));
			
		if(info.boot_sec!=0 && info.boot_sec!=0XFFFFFFFF)
			xy_gmtime_r((uint64_t)info.boot_sec*1000,&rtc_time);
		
		sprintf(rsp,"\r\nBoot_cp sum=%lu,Time=%lu/%lu-%lu:%lu:%lu\r\n",info.boot_sum,rtc_time.wall_clock.tm_mon,rtc_time.wall_clock.tm_mday,rtc_time.wall_clock.tm_hour,rtc_time.wall_clock.tm_min,rtc_time.wall_clock.tm_sec);
		send_urc_to_ext(rsp,strlen(rsp));	
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
	send_urc_to_ext(rsp,strlen(rsp));	
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
			xy_gmtime_r((uint64_t)info.soft_reset_sec*1000,&rtc_time);
		
		sprintf(rsp,"\r\nSOFT_RESET=%lu,Time=%lu/%lu-%lu:%lu:%lu\r\n",info.soft_reset_sum,rtc_time.wall_clock.tm_mon,rtc_time.wall_clock.tm_mday,rtc_time.wall_clock.tm_hour,rtc_time.wall_clock.tm_min,rtc_time.wall_clock.tm_sec);
		send_urc_to_ext(rsp,strlen(rsp));	
	}

	if(info.soc_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+SOC_RESET(CP_WDT/assert)=%lu\r\n",info.soc_reset_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}
	if(info.pin_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+PIN_RESET=%lu\r\n",info.pin_reset_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}
	if(info.wdt_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+WDT_RESET(AP_WDT/UTC_WDT)=%lu\r\n",info.wdt_reset_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}
	if(info.other_reset_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nGLOBAL_RESET+other_RESET=%lu\r\n",info.other_reset_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}
	if(info.power_on_sum != 0XFFFFFFFF)
	{
		sprintf(rsp,"\r\nPOWER_ON = %lu\r\n",info.power_on_sum);
		send_urc_to_ext(rsp,strlen(rsp));	
	}

	print_cp_dbg_info();
	print_ver_download_info();
}

