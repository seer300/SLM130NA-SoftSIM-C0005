/** 
* @file        cmccdm_hook_func.c
* @brief       中移的DM自注册功能二次开发的hook接口,目前所填内容均为示例,客户可按需修改
*/
#include "factory_nv.h"
#include <string.h>


/**
 * Get device info
**/
void cmccdm_getDevinfo(char* devInfo, unsigned int len)
{
	char* dev = "***";
	memcpy(devInfo, dev, strlen(dev)<len?strlen(dev):len);
}

/**
 * Get app info
**/
void cmccdm_getAppinfo(char* appInfo, unsigned int len)
{
	char* app = "unknown|unknown|unknown|unknown\r\n";
	memcpy(appInfo, app, strlen(app)<len?strlen(app):len);
}

/**
 * Get mac info
**/
void cmccdm_getMacinfo(char* macInfo, unsigned int len)
{
	char* mac = "***";
	memcpy(macInfo, mac, strlen(mac)<len?strlen(mac):len);
}

/**
 * Get rom size
**/
void cmccdm_getRominfo(char* romInfo, unsigned int len)
{
	char* rom = "2MB";
	memcpy(romInfo, rom, strlen(rom)<len?strlen(rom):len);
}

/**
 * Get ram size
**/
void cmccdm_getRaminfo(char* ramInfo, unsigned int len)
{
	char* ram = "320KB";
	memcpy(ramInfo, ram, strlen(ram)<len?strlen(ram):len);
}

/**
 * Get CPU info
**/
void cmccdm_getCpuinfo(char* CpuInfo, unsigned int len)
{
	char* cpu = "***";
	memcpy(CpuInfo, cpu, strlen(cpu)<len?strlen(cpu):len);
}

/**
 * Get system version
**/
void cmccdm_getSysinfo(char* sysInfo, unsigned int len)
{
	char* sysVerion = "FreeRTOS_V10.3.1";
	memcpy(sysInfo, sysVerion, strlen(sysVerion)<len?strlen(sysVerion):len);
}

/**
 * Get soft firmware verion
**/
void cmccdm_getSoftVer(char* softInfo, unsigned int len)
{
	char* fmware = (char *)(g_softap_fac_nv->versionExt);
	memcpy(softInfo, fmware, strlen(fmware)<len?strlen(fmware):len);
}

/**
 * Get soft firmware name
**/
void cmccdm_getSoftName(char* softname, unsigned int len)
{
	char* fmname = (char *)(g_softap_fac_nv->hardver);
	memcpy(softname, fmname, strlen(fmname)<len?strlen(fmname):len);
}

/**
 * Get Volte info
**/
void cmccdm_getVolteinfo(char* volInfo, unsigned int len)
{
	char* volte = "0";          //0:开;1:关;-1:不支持
	memcpy(volInfo, volte, strlen(volte)<len?strlen(volte):len);
}

/**
 * Get net type
**/
void cmccdm_getNetType(char* netType, unsigned int len)
{
	char* type = "NB-IoT";
	memcpy(netType, type, strlen(type)<len?strlen(type):len);
}

/**
 * Get net account
**/
void cmccdm_getNetAccount(char* netInfo, unsigned int len)
{
	char* account = "***";
	memcpy(netInfo, account, strlen(account)<len?strlen(account):len);
}

/**
 * Get phone number
**/
void cmccdm_getPNumber(char* pNumber, unsigned int len)
{
	char* number = "***";
	memcpy(pNumber, number, strlen(number)<len?strlen(number):len);
}

/**
 * Get location info
**/
void cmccdm_getLocinfo(char* locInfo, unsigned int len)
{
	char* loction = "***";
	memcpy(locInfo, loction, strlen(loction)<len?strlen(loction):len);
}

/**
 * Get route mac
**/
void cmccdm_getRouteMac(char* routeMac, unsigned int len)
{
	char* route = "***";
	memcpy(routeMac, route, strlen(route)<len?strlen(route):len);
}

/**
 * Get brand info
**/
void cmccdm_getBrandinfo(char* brandInfo, unsigned int len)
{
	char* brand = "XINYI";
	memcpy(brandInfo, brand, strlen(brand)<len?strlen(brand):len);
}

/**
 * Get GPU info
**/
void cmccdm_getGPUinfo(char* GPUInfo, unsigned int len)
{
	char* gpu = "***";
	memcpy(GPUInfo, gpu, strlen(gpu)<len?strlen(gpu):len);
}

/**
 * Get Board info
**/
void cmccdm_getBoardinfo(char* boardInfo, unsigned int len)
{
	char* board = "***";
	memcpy(boardInfo, board, strlen(board)<len?strlen(board):len);
}

/**
 * Get Model info
**/
void cmccdm_getModelinfo(char* modelInfo, unsigned int len)
{
	char* model = (char *)(g_softap_fac_nv->modul_ver);
	memcpy(modelInfo, model, strlen(model)<len?strlen(model):len);
}

/**
 * Get resolution info
**/
void cmccdm_getResinfo(char* resInfo, unsigned int len)
{
	char* resolution = "***";
	memcpy(resInfo, resolution, strlen(resolution)<len?strlen(resolution):len);
}

/**
 * Get IMEI2 info
**/
void cmccdm_getIMEI2info(char* imei2Info, unsigned int len)
{
    char* imei2 = "***";            //没有的话默认填***
    memcpy(imei2Info, imei2, strlen(imei2)<len?strlen(imei2):len);
}

/**
 * Get Blouetooth MAC info
**/
void cmccdm_getBlethMacinfo(char* blethMacInfo, unsigned int len)
{
    char* bluetoothMac = "***";
    memcpy(blethMacInfo, bluetoothMac, strlen(bluetoothMac)<len?strlen(bluetoothMac):len);
}

/**
 * Get batteryCapacity info
**/
void cmccdm_getbatCapinfo(char* batInfo, unsigned int len)
{
    char* bat = "4500mAh";
    memcpy(batInfo, bat, strlen(bat)<len?strlen(bat):len);
}

/**
 * Get screenSize info
**/
void cmccdm_getscSizeinfo(char* scSizeInfo, unsigned int len)
{
    char* screenSize = "***";
    memcpy(scSizeInfo, screenSize, strlen(screenSize)<len?strlen(screenSize):len);
}

/**
 * Get networkStatus info
**/
void cmccdm_getnwStainfo(char* nwStaInfo, unsigned int len)
{
    char* networkStatus = "1";         //1:连接;0:未连接;-1:不支持
    memcpy(nwStaInfo, networkStatus, strlen(networkStatus)<len?strlen(networkStatus):len);
}

/**
 * Get wearingStatus info
**/
void cmccdm_getwearStainfo(char* wearStaInfo, unsigned int len)
{
    char* wearStatus = "-1";            //1:佩戴;0:未佩戴;-1:不支持
    memcpy(wearStaInfo, wearStatus, strlen(wearStatus)<len?strlen(wearStatus):len);
}

/**
 * Get batteryCapacityCurr info
**/
void cmccdm_getbatCurinfo(char* batCurInfo, unsigned int len)
{
    char* batCapCur = "3900mAh";
    memcpy(batCurInfo, batCapCur, strlen(batCapCur)<len?strlen(batCapCur):len);
}

/**
 * Get SN info
**/
extern int xy_get_SN(char *sn, int len);
void cmccdm_getSNinfo(char* SNInfo, unsigned int len)
{
    //char* sn = "NULL";
    //memcpy(SNInfo, sn, strlen(sn)<len?strlen(sn):len);
    xy_get_SN(SNInfo, len);
}

