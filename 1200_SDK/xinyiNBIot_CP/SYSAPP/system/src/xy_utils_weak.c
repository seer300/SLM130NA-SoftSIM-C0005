/** 
* @brief   芯翼提供的供用户重新实现的模块工具级弱函数声明，根据自身需求实现需要的弱函数即可
* @note    用户需要重开发哪个弱函数，我们就提供哪个弱函数的实现给用户
*/

#include "at_com.h"
#include "factory_nv.h"
#include "low_power.h"
#include "oss_nv.h"
#include "xy_at_api.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "xy_utils.h"

/********************************** 云业务相关 **********************************/
void user_get_Manufacturer(char *manufacturer, int size)
{
	if (size > (int)(strlen("Open Mobile Alliance")))
		sprintf(manufacturer, "Open Mobile Alliance");
    else
		xy_printf(0,PLATFORM, WARN_LOG, "[user_get_Manufacturer]error!");
	return;
}

/*云平台获取设备SN信息函数，SN信息由用户提供*/
void user_get_SNumber(char *serial_number, int size)
{
    if (size > (int)(strlen("345000123")))
        sprintf(serial_number, "345000123");
    else
        xy_printf(0,PLATFORM, WARN_LOG, "[user_get_SNumber]error!");
    return;
}

void  user_get_chiptype(char *chiptype, int size)
{
	if((get_Soc_ver() == 0) || (get_Soc_ver() == 3))
    {
        snprintf(chiptype, size, "XY1200");
    }
    else if((get_Soc_ver() == 1) || (get_Soc_ver() == 4))
    {
        snprintf(chiptype,size, "XY1200S");
    }
    else if((get_Soc_ver() == 2) || (get_Soc_ver() == 5))
    {
        snprintf(chiptype,size, "XY2100S");
    }
}

bool user_get_HARDVER(char *hardver, int len)
{
	#define HARDVER_LEN 20
	if (len < HARDVER_LEN)
		return 0;
	sprintf(hardver, "%s", g_softap_fac_nv->hardver);
	return 1;
}

bool user_get_VERSIONEXT(char *versionExt, int len)
{
	#define VERSIONEXT_LEN 28
	if (len < VERSIONEXT_LEN)
		return 0;
	sprintf(versionExt, "%s", g_softap_fac_nv->versionExt);
	return 1;
}

bool user_get_MODULVER(char *modul_ver, int len)
{
	#define MODULVER_LEN 20
	if (len < MODULVER_LEN)
		return 0;
	sprintf(modul_ver, "%s", g_softap_fac_nv->modul_ver);
	return 1;
}

/********************************** 驱动相关 **********************************/
unsigned int xy_getVbatCapacity()
{
	//Get battery capacity(mAh), user must care and proc
	unsigned int bCapacity = 90;
	return bCapacity;
}

bool xy_is_enough_Capacity(int state)
{
	//compare current battery capacity with minimum threshold,if low than min_mah,return 0,and can not work continuously,such as FOTA
	UNUSED_ARG(state);
	return 1;
}

