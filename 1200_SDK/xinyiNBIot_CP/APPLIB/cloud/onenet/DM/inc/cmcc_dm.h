#ifndef _XY_CMCCDM_H
#define _XY_CMCCDM_H
#include "net_app_resume.h"
#include "onenet_utils.h"

typedef struct
{
    //用户AT配置信息
    uint8_t dm_app_key[64];     //##"M100000329"## onenet dm appkey
    uint8_t dm_app_pwd[64];     //##"43648As94o1K8Otya74T2719D51cmy58"## onenet dm password for dm appkey
    uint32_t dm_inteval_time;   //##0## 中移DM的注册周期,default 1440 minutes(24 hours)
    uint8_t dm_serv_flag;       //0:"m.fxltsbl.com", 1:"shipei.fxltsbl.com"
    uint8_t retry_num;          //##5## 重试次数
    uint16_t retry_time;         //##5## 重试间隔时间

    //注册后保存的信息
    uint8_t have_retry_num;
    uint8_t dm_reg_ip[128];
    uint8_t serv_location[40];
} cmcc_dm_regInfo_t;

typedef enum
{
    CMDM_INITIAL_STATE = 0,
    CMDM_REG_NEED = 5,
    CMDM_REG_SUCCESS,
    CMDM_REG_FAILED,
    CMDM_REG_TIMEOUT,
    CMDM_LIFETIME_TIMEOUT,
    CMDM_UPDATE_NEED,
    CMDM_UPDATE_SUCCESS = 11,
    CMDM_UPDATE_FAILED,
    CMDM_UPDATE_TIMEOUT
} et_dmp_status_t;

#define DM_TEMPLEATEID_DEFAULT        "TY000123"

void cmccdm_getDevinfo(char* devInfo, unsigned int len);
void cmccdm_getAppinfo(char* appInfo, unsigned int len);
void cmccdm_getMacinfo(char* macInfo, unsigned int len);
void cmccdm_getRominfo(char* romInfo, unsigned int len);
void cmccdm_getRaminfo(char* ramInfo, unsigned int len);
void cmccdm_getCpuinfo(char* CpuInfo, unsigned int len);
void cmccdm_getSysinfo(char* sysInfo, unsigned int len);
void cmccdm_getSoftVer(char* softInfo, unsigned int len);
void cmccdm_getSoftName(char* softname, unsigned int len);
void cmccdm_getVolteinfo(char* volInfo, unsigned int len);
void cmccdm_getNetType(char* netType, unsigned int len);
void cmccdm_getNetAccount(char* netInfo, unsigned int len);
void cmccdm_getPNumber(char* pNumber, unsigned int len);
void cmccdm_getLocinfo(char* locInfo, unsigned int len);
void cmccdm_getRouteMac(char* routeMac, unsigned int len);
void cmccdm_getBrandinfo(char* brandInfo, unsigned int len);
void cmccdm_getGPUinfo(char* GPUInfo, unsigned int len);
void cmccdm_getBoardinfo(char* boardInfo, unsigned int len);
void cmccdm_getModelinfo(char* modelInfo, unsigned int len);
void cmccdm_getResinfo(char* resInfo, unsigned int len);
void cmccdm_getIMEI2info(char* resInfo, unsigned int len);
void cmccdm_getBlethMacinfo(char* btmacInfo, unsigned int len);
void cmccdm_getbatCapinfo(char* batInfo, unsigned int len);
void cmccdm_getscSizeinfo(char* scSizeInfo, unsigned int len);
void cmccdm_getnwStainfo(char* nwStaInfo, unsigned int len);
void cmccdm_getwearStainfo(char* wearStaInfo, unsigned int len);
void cmccdm_getbatCurinfo(char* batCurInfo, unsigned int len);
void cmccdm_getSNinfo(char* SNInfo, unsigned int len);
void cmcc_dm_run(void);
void cmcc_dm_config_init();
void cmcc_dm_init();

//AT+XYDMP=<mode>,<interval_time>,<appkey>,<paswrd>,<test>
int at_XYDMP_req(char *at_buf, char **prsp_cmd);
#endif

