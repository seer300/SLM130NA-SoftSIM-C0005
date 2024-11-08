
#include "oss_nv.h"
#include "factory_nv.h"
#include "xy_memmap.h"
#include "xy_utils.h"
#include "nbiot_ps_export_interface.h"
#include "NBPhyL1cExportInterface.h"
#include "factory_nv.h"
#include "flash_adapt.h"
#include "low_power.h"
#include "xy_rtc_api.h"
#include "sys_config.h"
#include "net_app_resume.h"
#include "at_com.h"
#include "sys_clk.h"
#include "rc32k_cali.h"


/*供RF驱动使用，识别当前是否正在进行HTOL。信令模式测试除外*/
bool is_htol_test()
{
	return (g_softap_fac_nv->test==2 && HWREGB(BAK_MEM_TEST)==1);
}


void check_HTOL_test()
{	
	if(g_softap_fac_nv->test != 2)
		return;

	/*断电上电后默认为HTOL模式*/
	if(Get_Boot_Reason() == POWER_ON)
	{
		HWREGB(BAK_MEM_TEST) = 1;
	}

}
