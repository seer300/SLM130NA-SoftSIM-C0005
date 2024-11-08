/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/

#if TELECOM_VER
#include "net_app_resume.h"
#include "cdp_backup.h"
#include "cloud_utils.h"
#include "oss_nv.h"
#include "xy_ps_api.h"


/*******************************************************************************
 *                      Local function implementations                         *
 ******************************************************************************/
void cdp_user_config_init()
{
	//配置类数据初始化，初始值不为0的配置需在该处完成初始化
#if VER_BC95
	g_cdp_config_data->cdp_lifetime = 86400; //cdp lifetime 默认24小时
#endif
}

void cdp_storage_nv_init()
{
	cdp_cloud_setting("lwm2m.ctwing.cn",5683);
	
	g_softap_fac_nv->cdp_register_mode = 1;
	g_softap_fac_nv->need_start_dm = 1;
	SAVE_SOFTAP_FAC();
}

//入库模式下需检测卡类型是否有效
bool cdp_storage_uicc_isvalid()
{
	if(!CHECK_SDK_TYPE(OPERATION_VER))
		return 1;

	int uicc_type = 0;
	xy_get_UICC_TYPE(&uicc_type);
	xy_printf(0,XYAPP, WARN_LOG, "uicc_type:%d", uicc_type);

	if((0xFFFF+1) == uicc_type)
		return 0;
	else
		return 1;
	
}
#endif
