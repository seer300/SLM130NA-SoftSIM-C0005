#include "net_app_resume.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/udp.h"
#include "oss_nv.h"

#include "xy_fs.h"
#include "net_app_mem.h"
#include "main_proxy.h"
#include "xy_system.h"
#if TELECOM_VER
#include "atiny_context.h"
#include "cdp_backup.h"
#include "ota_flag.h"

#endif

#if MOBILE_VER
#include "onenet_utils.h"
#endif

#if CTWING_VER
#include "ctwing_resume.h"
#endif

/*深睡或软复位仍然有效的云状态机全局内存*/
void *g_cloud_mem_p = NULL;


/*同时支持UDP和公有云(CDP和ONENET二选一)的保存恢复*/
//#define CLOUD_BAKUP_LEN         1900    //按业务所需最大申请：(需要保存到flash的数据长度 + sizeof(cdp_session_info_t) + sizeof(cdp_fota_info_t) + sizeof(socket_udp_info_t))  188+1144+288+280
//#define CLOUD_CFG_LEN           180     //取配置文件中最大的长度(sizeof(onenet_config_nvm_t)  180)
//#define CLOUD_CHKSUM_LEN        4       //checksum 长度
//#define CLOUD_BAKUP_HEAD        (CLOUD_CHKSUM_LEN + 4)  //check_sum + 文件有效标记位(4+4)
//#define CLOUD_FLASH_SAVE_LEN    (CLOUD_BAKUP_HEAD + CLOUD_CFG_LEN) // 需要保存到flash的数据长度 (check_sum + flag + 配置文件长度)  188

void cloud_bakup_mem_init()
{
    if(Is_OpenCpu_Ver())		
    {
        g_cloud_mem_p = NV_MALLOC(CLOUD_BAKUP_LEN);		//开机申请固定不掉电内存

        /*深睡唤醒，或者FOTA软重启，或stop_cp后，AP核内存内容仍然有效，无需从FLASH读取*/
        if(Get_Boot_Reason()==WAKEUP_DSLEEP || (Get_Boot_Reason()==SOFT_RESET && Get_Boot_Sub_Reason()==SOFT_RB_BY_FOTA) || ((Get_Boot_Reason()==POWER_ON)&&(Get_Boot_Sub_Reason()==1)))
        {
            return;
        }
        else
        {
            //读取flash空间的内存并校验，校验成功则沿用flash读取内容，否则将内存清零
            xy_Flash_Read(FS_FLASH_BASE, g_cloud_mem_p, CLOUD_FLASH_SAVE_LEN);
            if(*((uint32_t *)g_cloud_mem_p) == xy_chksum(g_cloud_mem_p + CLOUD_BAKUP_HEAD, CLOUD_CFG_LEN))
            {
                //校验通过，内存内容有效，正常使用
                xy_printf(0,PLATFORM,WARN_LOG, "[BAN_WRITE_FLASH]cloud data check success");
				memset((g_cloud_mem_p + CLOUD_CFG_LEN),0,(CLOUD_BAKUP_LEN - CLOUD_CFG_LEN));
				return;
            }
            else
            {
                //校验失败，内存内容无效
                xy_printf(0,PLATFORM,WARN_LOG, "[BAN_WRITE_FLASH]cloud data error,clean all");
                memset(g_cloud_mem_p,0,CLOUD_BAKUP_LEN);
            }
        }
    }
}

void SaveCloudCfgByAP(void)//一旦配置文件发生过变化，就需要进行一次存储，防止客户配置文件掉电遗失
{
    static uint8_t whether_in_list = 0;

    if(whether_in_list == 0)
    {
        void *tmp = xy_malloc(CLOUD_CFG_LEN);
        xy_Flash_Read(FS_FLASH_BASE + CLOUD_BAKUP_HEAD, tmp, CLOUD_CFG_LEN);
        if(memcmp(tmp, g_cloud_mem_p + CLOUD_BAKUP_HEAD, CLOUD_CFG_LEN) == 0)       //配置文件内容无变化不需要写flash
        {
            xy_free(tmp);
            xy_printf(0,PLATFORM,WARN_LOG, "[BAN_WRITE_FLASH]cloud cfg data no change");
            return;
        }

        xy_free(tmp);
        *((uint32_t *)g_cloud_mem_p) = xy_chksum(g_cloud_mem_p + CLOUD_BAKUP_HEAD, CLOUD_CFG_LEN);

        whether_in_list = 1;
        lpm_nv_write_buff_add((uint32_t)Address_Translation_CP_To_AP(FS_FLASH_BASE), g_cloud_mem_p, CLOUD_FLASH_SAVE_LEN);
        xy_printf(0,PLATFORM,WARN_LOG, "[BAN_WRITE_FLASH]insert cloud data for AP flash write");
    }
}

/*OPENCPU形态从深睡不下电RAM区域申请，模组形态从普通堆申请*/
void *cloud_malloc(const char * fileName)
{
    void *mem = NULL;
    if(strcmp(fileName, CDP_CONFIG_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        if(Is_OpenCpu_Ver())
        {
            mem =  g_cloud_mem_p + CLOUD_BAKUP_HEAD;
        }
        else
        {
            mem =  xy_malloc(sizeof(cdp_config_nvm_t));
            memset(mem, 0,sizeof(cdp_config_nvm_t));
        }
#endif
    }
    else if(strcmp(fileName, ONENET_CONFIG_NVM_FILE_NAME) == 0)
    {
#if MOBILE_VER
        if(Is_OpenCpu_Ver())
        {
            mem = g_cloud_mem_p + CLOUD_BAKUP_HEAD;
        }
        else
        {
            mem =  xy_malloc(sizeof(onenet_config_nvm_t));
            memset(mem, 0,sizeof(onenet_config_nvm_t));
        }
#endif
    }
    else if(strcmp(fileName, CDP_SESSION_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        if(Is_OpenCpu_Ver())
        {
            mem = g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN;
        }
        else
        {
            mem = xy_malloc(sizeof(cdp_session_info_t));
            memset(mem, 0,sizeof(cdp_session_info_t));
        }
#endif
    }
    else if(strcmp(fileName, ONENET_SESSION_NVM_FILE_NAME) == 0)
    {
#if MOBILE_VER
        if(Is_OpenCpu_Ver())
        {
            mem = g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN;
        }
        else
        {
            mem = xy_malloc(sizeof(onenet_session_info_t));
            memset(mem, 0,sizeof(onenet_session_info_t));
        }
#endif
    }
    else if(strcmp(fileName, CDP_FOTA_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        if(Is_OpenCpu_Ver())
        {
            mem = g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN + sizeof(cdp_session_info_t);
        }
        else
        {
            mem = xy_malloc(sizeof(cdp_fota_info_t));
            memset(mem, 0,sizeof(cdp_fota_info_t));
        }
#endif
    }
	else if(strcmp(fileName, SOCKET_SESSION_NVM_FILE_NAME) == 0)
    {
#if AT_SOCKET
        if(Is_OpenCpu_Ver())
        {
            mem = g_cloud_mem_p + CLOUD_BAKUP_LEN - sizeof(socket_udp_info_t);
        }
        else
        {
            mem = xy_malloc(sizeof(socket_udp_info_t));
            memset(mem, 0,sizeof(socket_udp_info_t));
        }
#endif
    }
    return mem;
}

/*使有效某标记位，等效于写文件*/
int Save_File_By_BakMem(const char * fileName)
{
    char *tmp = (char *)(g_cloud_mem_p + CLOUD_CHKSUM_LEN);
    if(strcmp(fileName, CDP_CONFIG_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<0);     //cdp config 有效
        *tmp &= ~(0x3<<2);  //置onenet文件系统无效

        SaveCloudCfgByAP();        //仅配置类文件,需要深睡前让AP核保存flash
    }
    else if(strcmp(fileName, CDP_SESSION_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<1);     //cdp session 有效
        *tmp &= ~(0x3<<2);  //置onenet文件系统无效
    }
    else if(strcmp(fileName, ONENET_CONFIG_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<2);   //onenet config 有效
        *tmp &= ~(0x13<<0);  //置cdp文件系统无效

        SaveCloudCfgByAP();        //仅配置类文件,需要深睡前让AP核保存flash
    }
    else if(strcmp(fileName, ONENET_SESSION_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<3);     //onenet session 有效
        *tmp &= ~(0x13<<0);  //置cdp文件系统无效
    }
    else if(strcmp(fileName, CDP_FOTA_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<4);     //cdp fota 有效
    }
	else if(strcmp(fileName, SOCKET_SESSION_NVM_FILE_NAME) == 0)
    {
        *tmp |= (1<<5);     //udp socket 有效
    }
    else
        return 0;       //非需要关注的文件

    return 1;
}

/*依靠头部位图标识来指示内容有效性。内部memcpy当源和目的地址一致时不会执行拷贝动作*/
int Read_File_By_BakMem(const char * fileName, void *buf, uint32_t size)
{
    char *tmp = (char *)(g_cloud_mem_p + CLOUD_CHKSUM_LEN);
	
    if(strcmp(fileName, CDP_CONFIG_NVM_FILE_NAME) == 0)
    {
        if((*tmp & (1<<0)) == 0)
            return XY_ERR;

        memcpy(buf, g_cloud_mem_p + CLOUD_BAKUP_HEAD, size);
    }
    else if(strcmp(fileName, CDP_SESSION_NVM_FILE_NAME) == 0)
    {
        if((*tmp & (1<<1)) == 0)
            return XY_ERR;
#if TELECOM_VER
        memcpy(buf, g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN, size);
#endif
    }
    else if(strcmp(fileName, ONENET_CONFIG_NVM_FILE_NAME) == 0)
    {
        if((*tmp & (1<<2)) == 0)
            return XY_ERR;

        memcpy(buf, g_cloud_mem_p + CLOUD_BAKUP_HEAD, size);
    }
    else if(strcmp(fileName, ONENET_SESSION_NVM_FILE_NAME) == 0)
    {
        if((*tmp & (1<<3)) == 0)
            return XY_ERR;
#if MOBILE_VER
        memcpy(buf, g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN, size);
#endif
    }
    else if(strcmp(fileName, CDP_FOTA_NVM_FILE_NAME) == 0)
    {
        //cdp fota file
        if((*tmp & (1<<4)) == 0)
            return XY_ERR;
#if TELECOM_VER
        memcpy(buf, g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN + sizeof(cdp_session_info_t), size);
#endif
    }
	else if(strcmp(fileName, SOCKET_SESSION_NVM_FILE_NAME) == 0)
    {
        //cdp fota file
        if((*tmp & (1<<5)) == 0)
            return XY_ERR;
    }
    else
        return XY_ERR;

    return XY_OK;
}

/*使无效某标记位，等效于删除文件*/
int Remove_File_By_BakMem(const char * fileName)
{
    //清除文件有效标记位
    char *tmp = (char *)(g_cloud_mem_p + CLOUD_CHKSUM_LEN);      //偏移checksum的长度
    if(strcmp(fileName, CDP_CONFIG_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        *tmp &= ~(1<<0);
        memset(g_cloud_mem_p + CLOUD_BAKUP_HEAD, 0, CLOUD_CFG_LEN);
        SaveCloudCfgByAP();			//仅配置类文件,需要深睡前让AP核保存flash
#endif
    }
    else if(strcmp(fileName, CDP_SESSION_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        *tmp &= ~(1<<1);
        memset(g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN, 0, sizeof(cdp_session_info_t));
#endif
    }
    else if(strcmp(fileName, ONENET_CONFIG_NVM_FILE_NAME) == 0)
    {
#if MOBILE_VER
        *tmp &= ~(1<<2);
        memset(g_cloud_mem_p + CLOUD_BAKUP_HEAD, 0, CLOUD_CFG_LEN);
        SaveCloudCfgByAP();			//仅配置类文件,需要深睡前让AP核保存flash
#endif
    }
    else if(strcmp(fileName, ONENET_SESSION_NVM_FILE_NAME) == 0)
    {
#if MOBILE_VER
        *tmp &= ~(1<<3);
        memset(g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN, 0, sizeof(onenet_session_info_t));
#endif
    }
    else if(strcmp(fileName, CDP_FOTA_NVM_FILE_NAME) == 0)
    {
#if TELECOM_VER
        *tmp &= ~(1<<4);
        memset(g_cloud_mem_p + CLOUD_FLASH_SAVE_LEN + sizeof(cdp_session_info_t), 0, sizeof(cdp_fota_info_t));
#endif
    }
	else if(strcmp(fileName, SOCKET_SESSION_NVM_FILE_NAME) == 0)
    {
#if AT_SOCKET
        *tmp &= ~(1<<5);
        memset(g_cloud_mem_p + CLOUD_BAKUP_LEN - sizeof(socket_udp_info_t), 0, sizeof(socket_udp_info_t));
#endif
    }
    else
        return XY_ERR;

    return XY_OK;
}


/*开机初始化时，内部识别为OPENCPU形态，申请永不下电内存空间*/
application_init(cloud_bakup_mem_init);


