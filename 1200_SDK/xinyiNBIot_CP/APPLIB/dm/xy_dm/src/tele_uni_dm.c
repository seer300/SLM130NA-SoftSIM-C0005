#if XY_DM
#include "factory_nv.h"
#include "oss_nv.h"
#include "lwip/netdb.h"
#include "net_app_resume.h"
#include "xy_ps_api.h"
#include "xy_rtc_api.h"
#include "tele_uni_dm.h"
#include "dm_ctl.h"
#include "cJSON.h"
#include "xy_socket_api.h"
#if CTWING_VER || TELECOM_VER
#include "er-coap-13/er-coap-13.h"
#endif


dm_context_t *g_dm_pContext  = NULL;
osThreadId_t g_dm_TskHandle = NULL;
uint8_t g_dm_timer_flag = 0;

osMutexId_t g_tudm_context_mutex = NULL; 





void dm_print_str(char * buf)
{
	char str[56] = {0};
	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]Print_Len:%d,Print_Context:",strlen(buf));
	for(int len = 0; len <= strlen(buf); len+=55)
	{
		strncpy(str, buf + len, 55);
		xy_printf(0,XYAPP, WARN_LOG, "%s",str);
	}
}

/**
 * @brief 获取本地RTC时间
 */
uint32_t dm_get_world_times()
{
	return (uint32_t)(get_utc_ms()/1000);
}


/**
 * @brief 从xy_get_NCCID接口,获取nccid
 * 
 * @param dm_nccid 
 * @return int32_t -1:失败 0：成功
 */
int32_t dm_get_nccid(uint8_t *dm_nccid)
{
	if(xy_get_NCCID(dm_nccid,UCCID_LEN) == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM] get nccid failed\r\n");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}

int32_t dm_get_imsi(char *imsi)
{
	//get ismi
	if(xy_get_IMSI(imsi,16) == 0 || (imsi[0] == '0' && imsi[1] == '0' && (imsi[2] == '1' || imsi[2] == '2')))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_imsi,,imsi is invalid\n");
		return DM_PROC_FAILED;
	}
	return DM_PROC_SUCCESS;
}


void dm_get_module_ver(char *modul_ver)
{
	//get MODULE_VER_STR
	char i,flag = 0;
	memset(modul_ver, 0x00, 21);

	char *modver = xy_malloc(DM_MOUDLE_VER_LEN);
	dm_get_model(modver);

	memcpy(modul_ver, modver, strlen(modver));
	
	for(i = 0; i < 20; i++)
	{
		if(modul_ver[i] == '-' && flag == 0)
		{
			flag = 1;
			continue;
		}
		if(modul_ver[i] == '-')
			modul_ver[i] = ' ';
	}
}

/**
 * @brief 获取DM全局指针
 * 
 * @return dm_context_t* 
 */
dm_context_t* dm_get_context()
{
	if(g_dm_pContext == NULL)
	{
		g_dm_pContext = xy_malloc(sizeof(dm_context_t));
		memset(g_dm_pContext, 0x00,sizeof(dm_context_t));
	}

	return g_dm_pContext;
}

/**
 * @brief 获取系统默认的电信/联通DM自注册失败的重试次数
 * 
 * @return uint8_t 重试次数
 */
uint8_t dm_get_retry_num_def_val(void)
{
	dm_context_t* dm_pcontext = dm_get_context();
	
	uint8_t retry_num = dm_pcontext->uicc_type == UICC_TELECOM ? DM_TELE_RETRY_NUM_DEFAULT : DM_UNI_RETRY_NUM_DEFAULT;
	
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_retry_num_def_val,%d",retry_num);
	
	return retry_num;
}

/**
 * @brief 获取系统默认的联通DM的注册周期
 * 
 * @return uint32_t 联通DM的注册周期
 */
uint32_t dm_get_uni_reg_time_def_val(void)
{
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_uni_reg_time_def_val,%d",DM_UNI_REG_TIME_DEFAULT);
	return (uint32_t)DM_UNI_REG_TIME_DEFAULT;
}


/**
 * @brief 获取系统默认的电信/联通的DM服务器端口号
 * 
 * @return uint16_t DM服务器端口号
 */
uint16_t dm_get_port_def_val(void)
{
	dm_context_t* dm_pcontext = dm_get_context();
	
	uint16_t port = UICC_TELECOM ==  dm_pcontext->uicc_type ? DM_TELE_PORT_DEFAULT : DM_UNI_PORT_DEFAULT;

	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_port_def_val,%d",port);

	return port;
}


/**
 * @brief 获取系统默认的电信/联通的DM服务器IP地址或域名
 * 
 * @param host DM服务器IP地址或域名
 */
void dm_get_host_def_val(char *host)
{
	dm_context_t* dm_pcontext = dm_get_context();

	if(dm_pcontext->uicc_type == UICC_TELECOM)
	{
		strncpy(host ,DM_TELE_HOST_DEFAULT, strlen(DM_TELE_HOST_DEFAULT));
	}
	else if(dm_pcontext->uicc_type == UICC_UNICOM)
	{
		strncpy(host ,DM_UNI_HOST_DEFAULT, strlen(DM_UNI_HOST_DEFAULT));
	}

	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_host_def_val:%s",host);

}

/**
 * @brief 获取系统默认的电信/联通DM自注册失败后,再次发起注册的时间间隔
 * 
 * @return uint16_t 
 */
uint16_t dm_get_retry_time_def_val()
{
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_retry_time_def_val,%d",DM_RETRY_TIME_DEFAULT);
	
	return (uint16_t)DM_RETRY_TIME_DEFAULT;
}


/**
 * @brief 获取系统默认的regver
 * @note  默认值来源：DM_REGVER_DEFAULT
 * @param regver 
 */
void dm_get_regver_def_str(char * regver)
{
	strcpy(regver, DM_REGVER_DEFAULT_STR);
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]get_dm_regver_default_str: %s",regver);
}

/**
 * @brief 获取系统默认的swver
 * @note  默认值来源：出厂NV
 * @param swver 
 */
void dm_get_swver_def_str(char *swver)
{
	strncpy(swver, g_softap_fac_nv->versionExt, 28); //dm软件版本号
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_swver_def_str: %s",swver);
}

/**
 * @brief 获取系统默认的modver
 * @note  默认值来源：出厂NV
 * @param modver 
 */
void dm_get_modver_def_str(char * modver)
{
	strncpy(modver, g_softap_fac_nv->modul_ver, 20); // 模组型号
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_modver_def_str: %s",modver);
}

/**
 * @brief 获取系统默认的osver,默认为""
 * @note  默认值来源:DM_OSVER_DEFAULT_STR
 * @param osver 
 */
void dm_get_osver_def_str(char *osver)
{
	strcpy(osver,DM_UETYPE_DEFAULT_STR);
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_osver_def_str: %s",osver);
}

/**
 * @brief 获取系统默认的imei,调用xy_get_IMEI接口获取，若接口调用失败,返回空字符串
 * 
 * @param imei 
 */
int32_t dm_get_imei_def_str(char *imei)
{
	if(xy_get_IMEI(imei, 16) == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_imei_def_str,get imei err!\n");
		return DM_PROC_FAILED;
	}
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_imei_def_str: %s",imei);
	return DM_PROC_SUCCESS;
}


/**
 * @brief 获取系统默认的UETYPE
 * @note  默认值来源：DM_UETYPE_DEFAULT_STR
 * @param uetype 
 */
void dm_get_uetype_def_str(char *uetype)
{
	strcpy(uetype,DM_UETYPE_DEFAULT_STR);
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_uetype_def_str,%s!\n",uetype);
}


/**
 * @brief 根据键key从jsonstring字符串中获取对应的键值
 * 
 * @param jsonstring 
 * @param key 
 * @param key_value 
 * @return int32_t 0:成功; -1:失败
 */
static int32_t dm_get_key_value_from_jsonstr(const char *jsonstring, const char *key, char *key_value)
{
	int32_t ret = DM_PROC_FAILED;
	cJSON * json_item = NULL;
	cJSON * json_object = NULL;
	
	json_object = cJSON_Parse(jsonstring);//try translate json string to json object

	if(json_object == NULL)//jsonstring不是json字符串
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_key_value_from_jsonstr err,jsonstring no valid!");
		return DM_PROC_FAILED;
	}

	json_item = cJSON_GetObjectItem(json_object, key);

	if(json_item == NULL)//不存在对应键值对
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_key_value_from_jsonstr,no key-key_value");
		goto exit;
	}

	if(cJSON_GetStringValue(json_item) != NULL)
	{
		strcpy(key_value, cJSON_GetStringValue(json_item));
		xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_key_value_from_jsonstr,key_value:%s",key_value);
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_key_value_from_jsonstr,no key-key_value");
		goto exit;
	}
	
	ret = DM_PROC_SUCCESS;
exit:

	if(json_object)
	{
		cJSON_Delete(json_object);
	}

	return ret;
}


int32_t dm_get_imei(char *imei)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "imei",imei) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_imei,dm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}

int32_t dm_get_osver(char *osver)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "osver",osver) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_osver,dm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}

int32_t dm_get_model(char *model)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "model", model) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_model,dm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}


int32_t dm_get_swver(char *swver)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "swver", swver) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_swver,dm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}

int32_t dm_get_uetype(char *uetype)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "uetype",uetype) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_uetypedm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}

/**
 * @brief 从配置文件中获取regver
 * 
 * @param regver 
 * @return int32_t 
 */
int32_t dm_get_regver(char * regver)
{
	dm_context_t* config = dm_get_context();

	if(dm_get_key_value_from_jsonstr((const char*)config->json_report_str, "regver",regver) == DM_PROC_FAILED)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_get_regver,dm_get_key_value_from_jsonstr,err");
		return DM_PROC_FAILED;
	}

	return DM_PROC_SUCCESS;
}



/**
 * @brief 从DM文件中读取上下文
 * 
 * @return int32_t DM_PROC_SUCCESS:成功，DM_PROC_FAILED：失败
 */
int32_t dm_init_context_from_config_file()
{
	int32_t ret = DM_PROC_FAILED;
	dm_context_nvm_t *context_nvm = xy_malloc(sizeof(dm_context_nvm_t));
	memset(context_nvm, 0x00, sizeof(dm_context_nvm_t));

	if(cloud_read_file(TELECOM_UNICOM_DM_NVM_FILE_NAME,(void*)context_nvm, sizeof(dm_context_nvm_t)) == XY_ERR)
	{
		goto exit;
	}

	dm_context_t* dm_pcontext = dm_get_context();

	if(dm_pcontext->json_report_str)
	{
		xy_free(dm_pcontext->json_report_str);
		dm_pcontext->json_report_str = NULL;
	}

	memcpy(dm_pcontext, context_nvm, sizeof(dm_context_t)-4);

	dm_pcontext->json_report_str = xy_malloc(strlen(context_nvm->json_report_str)+1);

	memset(dm_pcontext->json_report_str, 0x00,strlen(context_nvm->json_report_str)+1);

	memcpy(dm_pcontext->json_report_str,context_nvm->json_report_str, strlen(context_nvm->json_report_str));


	ret = DM_PROC_SUCCESS;

exit:
	if(context_nvm)
	{
		xy_free(context_nvm);
	}
	
	return ret;
}

/**
 * @brief 保存DM配置文件
 * 
 * @return int32_t 
 */
void dm_save_config_file()
{
	dm_context_t* dm_pContext = dm_get_context();

	dm_context_nvm_t *context_nvm = xy_malloc(sizeof(dm_context_nvm_t));
	memset(context_nvm, 0x00, sizeof(dm_context_nvm_t));

	
	memcpy(context_nvm, dm_pContext, sizeof(dm_context_t)-4);

	if(dm_pContext->json_report_str)
	{
		memcpy(context_nvm->json_report_str, dm_pContext->json_report_str, strlen(dm_pContext->json_report_str));
	}

	cloud_save_file(TELECOM_UNICOM_DM_NVM_FILE_NAME,(void*)context_nvm, sizeof(dm_context_nvm_t));

	if(context_nvm)
	{
		xy_free(context_nvm);
	}
}


/**
 * @brief 设置DM参数为默认值
 * 
 * @param dm_pcontext 
 * @return int32_t 成功返回0 失败返回-1
 */
int32_t dm_set_params_to_default(dm_context_t* dm_pcontext, int32_t uicc_type)
{
	int32_t ret = DM_PROC_FAILED;
	
	char regver[DM_REG_VER_LEN] = {0};
	char uetype[DM_UETYPE_LEN]={0};
	char swver[DM_SW_VER_LEN]={0};
	char modver[DM_MOUDLE_VER_LEN]={0};
	char osver[DM_OS_VER_LEN]={0};
	char imei[DM_IMEI_LEN]={0};

	if(dm_pcontext->json_report_str)
	{
		xy_free(dm_pcontext->json_report_str);
		dm_pcontext->json_report_str = NULL;
	}

	memset(dm_pcontext, 0x00, sizeof(dm_context_t));

	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_set_params_to_default,uicc_type:%d!\r\n",uicc_type);

	dm_pcontext->retry_num = uicc_type == UICC_TELECOM ? DM_TELE_RETRY_NUM_DEFAULT : DM_UNI_RETRY_NUM_DEFAULT;
	dm_pcontext->port = UICC_TELECOM ==  uicc_type ? DM_TELE_PORT_DEFAULT : DM_UNI_PORT_DEFAULT;
	dm_pcontext->uni_reg_time = DM_UNI_REG_TIME_DEFAULT;
	dm_pcontext->retry_time = DM_RETRY_TIME_DEFAULT; //重试间隔

	if(uicc_type == UICC_TELECOM)
	{
		strncpy(dm_pcontext->host ,DM_TELE_HOST_DEFAULT, strlen(DM_TELE_HOST_DEFAULT));
	}
	else//uicc_type == UICC_UNICOM
	{
		strncpy(dm_pcontext->host ,DM_UNI_HOST_DEFAULT, strlen(DM_UNI_HOST_DEFAULT));
	}
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_get_def_host:%s\r\n",dm_pcontext->host);

	dm_pcontext->report_mode = XINYI_MESSAGE_FORMAT;
	dm_get_regver_def_str(regver);
	dm_get_uetype_def_str(uetype);
	dm_get_swver_def_str(swver);
	dm_get_modver_def_str(modver);
	dm_get_osver_def_str(osver);

	if(dm_get_imei_def_str(imei)== DM_PROC_FAILED) 
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_set_params_to_default,dm_get_imei_def_str err\r\n");
		goto exit;
	}

	char * buf = xy_malloc(200);
	memset(buf, 0x00, 200);

	sprintf((char *)buf, "{\"regver\":\"%s\",\"uetype\":\"%s\",\"swver\":\"%s\",\"model\":\"%s\",\"osver\":\"%s\",\"imei\":\"%s\"}",
	regver, uetype ,swver, modver, osver, imei);

	dm_pcontext->json_report_str = xy_malloc(strlen(buf)+1);
	strcpy(dm_pcontext->json_report_str, buf);
	ret = DM_PROC_SUCCESS;
exit:
	if(buf)
	{
		xy_free(buf);
	}
	return ret;
}

/**
 * @brief 将regver,uetype,swver,modver,osver,imei参数组成json字符串后设置并保存到FS
 * 
 * @param regver 
 * @param uetype 
 * @param swver 
 * @param modver 
 * @param osver 
 * @param imei 
 */
void dm_set_params_in_jsonstr_format(char *regver, char *uetype, char *swver, char *modver, char *osver, char* imei)
{
	dm_context_t* dm_pcontext = dm_get_context();
	
	char * buf = xy_malloc(300);
	memset(buf, 0x00, 300);

	sprintf((char *)buf, "{\"regver\":\"%s\",\"uetype\":\"%s\",\"swver\":\"%s\",\"model\":\"%s\",\"osver\":\"%s\",\"imei\":\"%s\"}",
		regver, uetype ,swver, modver, osver, imei);

	if(dm_pcontext->json_report_str)
	{
		xy_free(dm_pcontext->json_report_str);
		dm_pcontext->json_report_str = NULL;
	}

	dm_pcontext->json_report_str = xy_malloc(strlen(buf) + 1);

	strcpy(dm_pcontext->json_report_str, buf);

	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_set_params_in_jsonstr_format:\r\n");
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]%s",dm_pcontext->json_report_str);
	dm_pcontext->report_mode = XINYI_MESSAGE_FORMAT;
	dm_save_config_file();

	if(buf)
	{
		xy_free(buf);
	}
	
}

/**
 * @brief 设置和保存用户自定义的,用于上报的json字符串
 * @warning 该字符串,芯翼检查是否为合法json字符串,透传内容
 * @param jsonstring 
 * @return int32_t DM_PROC_SUCCESS:成功 DM_PROC_FAILED:-1
 */
int32_t dm_set_others_json_context(const char * jsonstring)
{
	int32_t ret = DM_PROC_FAILED;

	dm_context_t* dm_pcontext = dm_get_context();

	cJSON * json = cJSON_Parse(jsonstring);//try translate json string to json object
	
	if(json == NULL)//jsonstring不是合法的json字符串
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_set_others_json_context,is not vaild jsonstring");
		goto exit;
	}

	if(dm_pcontext->json_report_str)
	{
		xy_free(dm_pcontext->json_report_str);
		dm_pcontext->json_report_str = NULL;
	}
	

	dm_pcontext->json_report_str = xy_malloc(strlen(jsonstring)+1);

	strcpy(dm_pcontext->json_report_str, jsonstring);
	
	xy_printf(0,XYAPP, INFO_LOG, "[TUDM]dm_set_others_json_context,report context:%s!",dm_pcontext->json_report_str);

	dm_pcontext->report_mode = USER_MESSAGE_FORMAT;
	dm_save_config_file();

	ret = DM_PROC_SUCCESS;

exit:
	if(json)
	{
		cJSON_Delete(json);
	}
	return ret;
}



/**
 * @brief 设置DM配置类参数
 * 
 * @param setting_uicc_owner 
 * @param retry_num 
 * @param retry_time 
 * @param uni_reg_time 
 * @param port 
 * @param host  
 */
void dm_set_setting_cfg_params(uint8_t setting_uicc_owner, uint8_t retry_num, uint16_t retry_time, uint32_t uni_reg_time, uint16_t port,char * host)
{
	dm_context_t* dm_pcontext = dm_get_context();

	dm_set_setting_owner(setting_uicc_owner);

	memset(dm_pcontext->host, 0x00, DM_HOST_NAME_LEN);
	strcpy(dm_pcontext->host ,host);

	dm_pcontext->retry_num = retry_num;

	dm_pcontext->retry_time = retry_time;

	dm_pcontext->uni_reg_time = uni_reg_time;

	dm_pcontext->port = port;

	dm_save_config_file();
}


/**
 * @brief判断当前是否需要执行dm
 * 
 * @param nccid_dm_temp 
 * @param current_time 
 * @return true 需要执行DM
 * @return false 不需要执行DM
 */
bool dm_check_if_need_execute(uint8_t *nccid_dm, uint32_t *current_time)
{
	dm_context_t* dm_pcontext = dm_get_context();

	*current_time = dm_get_world_times();

	xy_printf(0,XYAPP, WARN_LOG, "dm_check_if_need_execute,current_time = %d,last_reg_time = %d,unicom_timer_flag = %d\n", *current_time, dm_pcontext->last_reg_time, g_dm_timer_flag);

	//当前ICCID与存储的ICCID不相等或者注册周期超过30天，均需要dm
	if(strncmp((const char *)nccid_dm, (const char *)dm_pcontext->ue_iccid,20) != 0 || 
		((*current_time - dm_pcontext->last_reg_time > dm_pcontext->uni_reg_time) && (dm_pcontext->uicc_type == UICC_UNICOM)))
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_check_if_need_execute,need start dm\n");
		return true;
	}

	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_check_if_need_execute,do not need start dm");
	
	return false;
}


/**
 * @brief 检查setting_owner类型与当前SIM卡是否匹配
 * 
 * @param setting_uicc_owner 
 * @return true setting_uicc_owner与当前SIM卡匹配
 * @return false setting_uicc_owner与当前SIM卡不匹配
 */
bool dm_check_if_setting_owner_valid(uint8_t setting_uicc_owner)
{
	dm_context_t* dm_pcontext = dm_get_context();

	if((setting_uicc_owner == DM_SETTING_TELECOM && dm_pcontext->uicc_type != UICC_TELECOM) || (setting_uicc_owner == DM_SETTING_UNICOM
	&& dm_pcontext->uicc_type != UICC_UNICOM))//SIM卡类型与用户预设置类型不否，错误
	{
		xy_printf(0,XYAPP, WARN_LOG, "[DMTU]dm_check_if_setting_owner_valid,sim type:%d,setting type:%d,no match err", dm_pcontext->uicc_type, setting_uicc_owner);
		return false;
	}

	return true;
}


/**
 * @brief 检测当前配置文件所属运营商与SIM识别运营商是否一致
 * 
 * @return true  //允许配置DM文件
 * @return false //用户未设置配置文件所属运营商，或者当前配置文件所属运营商与当前SIM卡所属运营商不一致，不允许配置DM文件,返回false
 */
bool dm_check_if_cfg_uicc_owner_match_uicc_type()
{
	dm_context_t* dm_pcontext = dm_get_context();

	if(dm_pcontext->setting_uicc_owner == DM_SETTING_UNKOWN)//用户未设置配置文件所属运营商
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]check_if_setting_uicc_owner_match_uicc_type,setting_uicc_owner unkown!");
		return false;
	}

	if((dm_pcontext->setting_uicc_owner == DM_SETTING_TELECOM && dm_pcontext->uicc_type != UICC_TELECOM)
		|| (dm_pcontext->setting_uicc_owner == DM_SETTING_UNICOM && dm_pcontext->uicc_type != UICC_UNICOM))//当前配置文件所属运营商与当前SIM卡所属运营商不一致，不允许配置DM文件
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]check_if_setting_uicc_owner_match_uicc_type,setting_uicc_owner %d,uicc_type %d no match err!",dm_pcontext->setting_uicc_owner,dm_pcontext->uicc_type);
		return false;
	}

	return true;
}

/**
 * @brief 检测当前DM配置信息是否可用
 * 
 * @return true 
 * @return false 
 */
static bool dm_check_if_cfg_file_avaliable()
{
	dm_context_t* dm_pcontext = dm_get_context();

	//uicc_owner值为DM_SETTING_UNKOWN,用户未通过AT命令进行过参数配置，使用芯翼默认配置，返回当前DM配置信息可用
	if(dm_pcontext->setting_uicc_owner == DM_SETTING_UNKOWN)
	{
		xy_printf(0,XYAPP, INFO_LOG, "[DMTU]dm_check_if_cfg_file_avaliable,user not operation,yes\r\n");
		return true;
	}
	else//用户已使用AT命令对配置文件进行过操作,判断当前配置文件与SIM卡类型是否匹配，匹配则可用,反之不可用
	{
		if((dm_pcontext->setting_uicc_owner == DM_SETTING_TELECOM && dm_pcontext->uicc_type != UICC_TELECOM) || 
		(dm_pcontext->setting_uicc_owner == DM_SETTING_UNICOM && dm_pcontext->uicc_type != UICC_UNICOM))///
		{
			xy_printf(0,XYAPP, WARN_LOG, "[DMTU]dm_check_if_cfg_file_avaliable,uicc_type:%d,setting_uicc_owner:%d,no match!\r\n",dm_pcontext->uicc_type,dm_pcontext->setting_uicc_owner);
			return false;
		}
		return true;
	}
	
}


/**
 * @brief 判断host是否合法
 * 
 * @param host 
 * @return true 合法
 * @return false  不合法
 */
bool dm_check_if_dm_host_valid(uint8_t *host)
{
	ip_addr_t addr = {0};

	if(ipaddr_aton(host, &addr)) //1:ip,0:domain
	{
		uint8_t netif_iptype = xy_get_netif_iptype(); //获取当前网络类型
		//如果当前网络是单栈V4但是入参传入的是V6地址，或者当前网络是单栈v6但是入参传入的是v4地址，或者ip地址非法 则返回错误。
		if((xy_IpAddr_Check(host, addr.type) == 0) || (netif_iptype == IPV4_TYPE && addr.type != IPADDR_TYPE_V4) || (netif_iptype == IPV6_TYPE && addr.type != IPADDR_TYPE_V6))
		{
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_set_config_params, ip:%s,type:%d is invalid",host,addr.type);
			return false;
		}	
	}
	else//域名
	{
		if(xy_domain_is_valid(host) == 0)//域名检测不合法
		{
			xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm_set_config_params,domain is invaild");
			return false;
		}
	}
	return true;
}



//DM数据接收处理
int dm_send_process(int sockfd, int req_len, uint8_t *req_buff, int uicc_type)
{
	int n = 0;
	int ret = -1;
	char *recv_buf = NULL;
	struct timeval tv_out;

	n = send(sockfd, req_buff, req_len, 0);

	if (n < 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]client: server is closed.\n");
		close(sockfd);
		return -1;
	}

	tv_out.tv_sec = 180;
    tv_out.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

	recv_buf = xy_malloc(512);
	memset(recv_buf, 0, 512);



	n = recv(sockfd, recv_buf, 512, 0);
	
	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]recv ret %d", n);
	
	
	if(n > 0)//收到下行回复
	{
		if(uicc_type == UICC_TELECOM)//电信
		{
	#if TELECOM_VER
			coap_packet_t message[1];
			uint8_t error_code = NO_ERROR;
			error_code = xy_coap_parse_message(message, recv_buf, n);
	#elif CTWING_VER
			ctlw_coap_packet_t message[1];
			uint8_t error_code = NO_ERROR;
			error_code = ctlw_coap_parse_message(message, recv_buf, n);
	#endif
			if (error_code == NO_ERROR)
			{
				if(strstr(message->payload,"Success") == NULL)//DM平台下发报文无SUCCESS
				{
					ret = -1;
				}
				else
					ret = 0;///DM平台下发SUCCESS
			}
			else//coap文件解析失败
			{
				ret = -1;
			}
		}
		else
		{
			ret = (strstr(recv_buf,"Success") != NULL) ? 0 : -1;
		}
	}
	else//未收到下行回复
	{
    	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]recv response err,close fd %d", sockfd);
		ret = -1;
	}

	close(sockfd);
	xy_free(recv_buf);
	return ret;
}

void dm_make_json_report_pack(uint8_t *buf, uint8_t *modul_ver ,uint8_t* sim1iccid, uint8_t* sim1lteimsi)
{
	dm_context_t* dm_pcontext = dm_get_context();
	
	char regver[DM_REG_VER_LEN] = {0};
	char uetype[DM_UETYPE_LEN]={0};
	char swver[DM_SW_VER_LEN]={0};
	char osver[DM_OS_VER_LEN]={0};
	char imei[DM_IMEI_LEN]={0};
	
	dm_get_imei(imei);
	dm_get_regver(regver);
	dm_get_uetype(uetype);
	dm_get_swver(swver);
	dm_get_osver(osver);

	if( UICC_TELECOM ==  dm_pcontext->uicc_type)//电信
	{
		if(dm_pcontext->report_mode == XINYI_MESSAGE_FORMAT)
		{
			sprintf((char *)buf, "{\"REGVER\":\"%s\",\"MEID\":\"%s\",\"SWVER\":\"%s\",\"MODEL\":\"%s\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\",\"UETYEP\":\"%s\",\"OSVER\":\"%s\"}",
			regver, imei ,swver, modul_ver, sim1iccid, sim1lteimsi, uetype, osver);

		}
		else if(dm_pcontext->report_mode == USER_MESSAGE_FORMAT)
		{
			strcpy((char *)buf, dm_pcontext->json_report_str);
		}
	}
	else//联通UICC_UNICOM ==  dm_pcontext->uicc_type
	{
		if(dm_pcontext->report_mode == XINYI_MESSAGE_FORMAT)
		{
			sprintf((char *)buf, "{\"REGVER\":\"%s\",\"MEID\":\"%s\",\"SWVER\":\"%s\",\"MODELSMS\":\"%s\",\"SIM1ICCID\":\"%s\",\"SIM1LTEIMSI\":\"%s\"}",
				regver, imei, swver, modul_ver, sim1iccid, sim1lteimsi);
		}
		else if(dm_pcontext->report_mode == USER_MESSAGE_FORMAT)
		{
			strcpy((char *)buf, dm_pcontext->json_report_str);
		}
	}
}


int32_t dm_make_telecom_packet(uint8_t* nccid_dm, uint8_t *req_buff, int32_t * req_len, uint8_t *modul_ver, uint8_t *imsi)
{
	int msg_seri_len = 0;
	uint8_t *temp_buff = NULL;
	uint8_t *coap_buff = NULL;
	uint8_t *dataBuf_base64 = NULL;
	uint16_t coap_msgid = 0;
	time_t tv_sec = 0;
	uint8_t temp_token[6] = {0};
	dm_context_t* dm_pcontext = dm_get_context();
#if TELECOM_VER
		coap_packet_t* pkt = NULL;
#elif CTWING_VER
	    ctlw_coap_packet_t* pkt = NULL;
#endif

	if(dm_pcontext->report_mode == XINYI_MESSAGE_FORMAT)
	{
		temp_buff = xy_malloc(XINYI_MESSAGE_JSONSTR_BUFF_LEN);
		dataBuf_base64 = xy_malloc(XINYI_MESSAGE_BASE64_JSONSTR_LEN);
		memset(dataBuf_base64, 0x00, XINYI_MESSAGE_BASE64_JSONSTR_LEN);
	}
	else if(dm_pcontext->report_mode == USER_MESSAGE_FORMAT)//该模式下，json字符串支持到400字节
	{
		temp_buff = xy_malloc(USER_MESSAGE_JSONSTR_BUFF_LEN);
		dataBuf_base64 = xy_malloc(USER_MESSAGE_BASE64_JSONSTR_BUFF_LEN);
		memset(dataBuf_base64, 0x00, USER_MESSAGE_BASE64_JSONSTR_BUFF_LEN);
	}

	dm_make_json_report_pack(temp_buff, modul_ver, nccid_dm, imsi);

	dm_print_str(temp_buff);

#if TELECOM_VER
		xy_utils_base64Encode(temp_buff,strlen(temp_buff),dataBuf_base64,xy_utils_base64GetSize(strlen(temp_buff)));
#elif CTWING_VER
        ctlw_utils_base64Encode(temp_buff,strlen(temp_buff),dataBuf_base64,ctlw_utils_base64GetSize(strlen(temp_buff)));
#endif

	if(temp_buff)
	{
		xy_free(temp_buff);
	}
	
	//配置请求信息：CON，POST，MID，TOKEN
	coap_msgid = (uint16_t)xy_rand();
	tv_sec = dm_get_world_times();
	//generate a token
	temp_token[0] = coap_msgid;
	temp_token[1] = coap_msgid >> 8;
	temp_token[2] = tv_sec;
	temp_token[3] = tv_sec >> 8;
	temp_token[4] = tv_sec >> 16;
	temp_token[5] = tv_sec >> 24;
		
#if TELECOM_VER
	//申请packet空间
	pkt = (coap_packet_t *)xy_malloc(sizeof(coap_packet_t));
	memset(pkt, 0x00, sizeof(coap_packet_t));

	xy_coap_init_message(pkt, COAP_TYPE_CON, COAP_POST, coap_msgid);
	xy_coap_set_header_token(pkt, temp_token, 6);

	//参照电信白皮书配置规范制定option和base64加密后的payload
	xy_coap_set_header_uri_host(pkt, dm_pcontext->host);
	xy_coap_set_header_uri_path(pkt, DM_HOST_PATH_DEFAULT);
	xy_coap_set_header_content_type(pkt, APPLICATION_JSON);	
	xy_coap_set_payload(pkt, dataBuf_base64, strlen(dataBuf_base64));

	msg_seri_len = xy_coap_serialize_get_size(pkt);
	if (msg_seri_len == 0)
	{
		if(pkt != NULL)
		{
			xy_coap_free_header(pkt);
			xy_free(pkt);
		}
		if(dataBuf_base64)
		{
			xy_free(dataBuf_base64);
		}
		
		return DM_PROC_FAILED;
	}
		
	coap_buff = (uint8_t *)xy_malloc(msg_seri_len);
	memset(coap_buff, 0x00, msg_seri_len);
	msg_seri_len = xy_coap_serialize_message(pkt, coap_buff);
#elif CTWING_VER
	//申请packet空间
	pkt = (ctlw_coap_packet_t *)xy_malloc(sizeof(ctlw_coap_packet_t));
	memset(pkt, 0x00, sizeof(ctlw_coap_packet_t));

	ctlw_coap_init_message(pkt, COAP_TYPE_CON, COAP_POST, coap_msgid);
	ctlw_coap_set_header_token(pkt, temp_token, 6);
	//参照电信白皮书配置规范制定option和base64加密后的payload
	ctlw_coap_set_header_uri_host(pkt, dm_pcontext->host);
	ctlw_coap_set_header_uri_path(pkt, DM_HOST_PATH_DEFAULT);
	ctlw_coap_set_header_content_type(pkt, APPLICATION_JSON);	
	ctlw_coap_set_payload(pkt, dataBuf_base64, strlen(dataBuf_base64));

	msg_seri_len = ctlw_coap_serialize_get_size(pkt);
	if (msg_seri_len == 0)
	{
		if(pkt != NULL)
		{
			ctlw_coap_free_header(pkt);
			xy_free(pkt);
		}
		if(dataBuf_base64)
		{
			xy_free(dataBuf_base64);
		}
		return DM_PROC_FAILED;
	}
	
	coap_buff = (uint8_t *)xy_malloc(msg_seri_len);
	memset(coap_buff, 0x00, msg_seri_len);
	msg_seri_len = ctlw_coap_serialize_message(pkt, coap_buff);
#endif
	if(coap_buff == NULL)
	{
		xy_free(dataBuf_base64);
		return DM_PROC_FAILED;
	}
	memcpy(req_buff, coap_buff, msg_seri_len);
	*req_len = msg_seri_len;

	xy_free(coap_buff);
	if(dataBuf_base64)
	{
		xy_free(dataBuf_base64);
	}
	return DM_PROC_SUCCESS;
}

int32_t dm_make_union_packet(int uicc_type, uint8_t* nccid_dm, uint8_t *req_buff, uint8_t *modul_ver, uint8_t *imsi)
{
	dm_context_t* dm_pcontext = dm_get_context();
	uint8_t *dataBuf_base64 = NULL;

	if(dm_pcontext->report_mode == XINYI_MESSAGE_FORMAT)
	{
		dataBuf_base64 = xy_malloc(XINYI_MESSAGE_BASE64_JSONSTR_LEN);
		memset(dataBuf_base64, 0x00, XINYI_MESSAGE_BASE64_JSONSTR_LEN);
	}
	else if(dm_pcontext->report_mode == USER_MESSAGE_FORMAT)//该模式下，json字符串支持到400字节
	{
		dataBuf_base64 = xy_malloc(USER_MESSAGE_BASE64_JSONSTR_BUFF_LEN);
		memset(dataBuf_base64, 0x00, USER_MESSAGE_BASE64_JSONSTR_BUFF_LEN);
	}

	dm_make_json_report_pack(req_buff, modul_ver, nccid_dm, imsi);

	dm_print_str(req_buff);
#if TELECOM_VER
	xy_utils_base64Encode(req_buff,strlen(req_buff), dataBuf_base64, xy_utils_base64GetSize(strlen(req_buff)));
#endif
	//自注册协议包头格式
	sprintf(req_buff,"POST / HTTP/1.1\r\nContent-type:application/encrypted-json\r\nContent-length:%d\r\nHost:%s:%d\r\n\r\n%s",strlen(dataBuf_base64),dm_pcontext->host,dm_pcontext->port, dataBuf_base64);
	return DM_PROC_SUCCESS;
}


int dm_make_packet(int uicc_type, uint8_t* nccid_dm, uint8_t *req_buff, int32_t *req_len,uint8_t *modul_ver, uint8_t *imsi)
{
	dm_context_t* dm_pcontext = dm_get_context();

	int msg_seri_len = 0;
	uint8_t *temp_buff = NULL;
	uint8_t *coap_buff = NULL;
	
	if(UICC_TELECOM == uicc_type)
	{
		if(dm_make_telecom_packet(nccid_dm, req_buff, req_len, modul_ver, imsi) != DM_PROC_SUCCESS)
		{
			return DM_PROC_FAILED;
		}
	}
	else if(UICC_UNICOM == uicc_type)
	{
		if(dm_make_union_packet(uicc_type, nccid_dm, req_buff, modul_ver, imsi) != DM_PROC_SUCCESS)
		{
			return DM_PROC_FAILED;
		}
		*req_len = strlen(req_buff);
	}
	return DM_PROC_SUCCESS;
}

//电信联通客户端dm数据收发
int dm_client(int req_len, uint8_t *req_buff, int uicc_type)
{
	int fd = -1;
	int rc = -1;
	dm_context_t *dm_pcontext  = dm_get_context();

	if(UICC_TELECOM == uicc_type)
	{
		//电信dm使用UDP进行数据传输
		fd = xy_socket_by_host(dm_pcontext->host, Sock_IPv46, IPPROTO_UDP, 0, dm_pcontext->port, NULL);
	}
	else//UICC_UNICOM == uicc_type
	{
		//联通dm使用TCP进行数据传输
		fd = xy_socket_by_host(dm_pcontext->host, Sock_IPv46, IPPROTO_TCP, 0, dm_pcontext->port, NULL);
	}
	
	if(fd < 0 )
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]create socket error,fd=%d, errno=%d\n",fd,errno);		
	}
	else
	{
		return (dm_send_process(fd, req_len, req_buff, uicc_type));
	}
	close(fd);
	return rc;
}


void dm_timeout_proc(void)
{
	g_dm_timer_flag = 1;
	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]start dm again!!!\n");
	//RTC唤醒后，协议栈可能还处于PSM态，不加锁，业务代码若有delay，则会进入idle，进而可能进入深睡
	app_delay_lock(1000);
	tele_uni_dm_task();
}


/**
 * @brief 设置电信/联通DM的开关
 * 
 * @param dm_switch
 */
void dm_set_switch(uint8_t dm_switch)
{
	g_softap_fac_nv->need_start_dm = dm_switch;
	SAVE_FAC_PARAM(need_start_dm);
}

/**
 * @brief 设置DM配置文件所属运营商是电信还是联通
 * 
 * @param setting_uicc_owner 1：电信 2：联通
 */
void dm_set_setting_owner(uint8_t setting_uicc_owner)
{
	dm_context_t* dm_pcontext = dm_get_context();
	dm_pcontext->setting_uicc_owner = setting_uicc_owner;
	dm_save_config_file();
}



/**
 * @brief DM全局初始化
 * 
 * @return int32_t  0:成功 -1：失败
 */
int32_t dm_context_init()
{
	int32_t ret = DM_PROC_SUCCESS;
    static uint8_t  dm_inited = 0;

	osMutexAcquire(g_tudm_context_mutex, osWaitForever);
    
	if(!dm_inited)
    {
		int uicc_type = UICC_UNKNOWN;

		xy_get_UICC_TYPE(&uicc_type);

		if(uicc_type != UICC_TELECOM && uicc_type != UICC_UNICOM)//当前系统检测SIM不是联通也不是电信
		{
			xy_printf(0,XYAPP, WARN_LOG, "[DMTU]dm_context_init err,uicc type err%d!\r\n", uicc_type);
			ret = DM_PROC_FAILED;
			goto exit;
		}

		dm_context_t *dm_pcontext = dm_get_context();

		if(dm_init_context_from_config_file() != DM_PROC_SUCCESS || ((dm_init_context_from_config_file() == DM_PROC_SUCCESS) 
		&& dm_pcontext->setting_uicc_owner == DM_SETTING_UNKOWN))
		{
			if(dm_set_params_to_default(dm_pcontext,uicc_type) !=DM_PROC_SUCCESS)//从文件系统恢复配置参数失败，用默认值进行初始化
			{
				xy_printf(0,XYAPP, WARN_LOG, "[DMTU]dm_context_init err,dm_set_params_to_default err\r\n");
				ret = DM_PROC_FAILED;
				goto exit;
			}
		}
		dm_pcontext->uicc_type = uicc_type;
        dm_inited = 1;
    }

exit:
	osMutexRelease(g_tudm_context_mutex);
	return ret;
}

void tele_uni_dm_task(void *param)
{
    int uicc_type = (int) param;
	int32_t req_len = 0;
	int ret = -1;
	uint32_t current_time = 0;
	uint8_t * req_buff = NULL;
	uint8_t MODULE_VER_STR[21]={0};
	uint8_t imei[16]={0};
	uint8_t imsi[16]={0};
	uint8_t nccid_dm[UCCID_LEN]={0};

	dm_context_t* dm_pcontext = dm_get_context();
	
    /*网络状态检查*/
	if(!xy_tcpip_is_ok())
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,net is not ok!");
		goto out;
	}
		

	/*DM用户可配置相关参数初始化*/
	if(dm_context_init() != DM_PROC_SUCCESS)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,dm_context_init is err!");
		goto out;
	}
	
	//检测当前DM配置是否可用
	if(dm_check_if_cfg_file_avaliable() == false)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,dm_check_if_cfg_file_avaliable err!");
		goto out;
	}

	if(dm_get_nccid(nccid_dm) != DM_PROC_SUCCESS)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,dm get nccid err");
		goto out;
	}

	//判断当前是否需要进行DM任务
	if(dm_check_if_need_execute(nccid_dm, &current_time) == false)
	{
		goto out;
	}

	/*获取DM自注册上报参数，imsi*/
	if(dm_get_imsi(imsi)!= DM_PROC_SUCCESS)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,dm_get_imsi err!");
		goto out;
	}

	/*获取DM自注册上报参数，module_ver*/
	dm_get_module_ver(MODULE_VER_STR);


    if(dm_pcontext->report_mode == USER_MESSAGE_FORMAT)
	{
		req_buff = xy_malloc(USER_MESSAGE_SEND_BUFF_LEN);
		memset(req_buff, 0x00, USER_MESSAGE_SEND_BUFF_LEN);
	}
	else
	{
		req_buff = xy_malloc(XINYI_MESSAGE_SEND_BUFF_LEN);
		memset(req_buff, 0x00, XINYI_MESSAGE_SEND_BUFF_LEN);
	}

	if(dm_make_packet(uicc_type, nccid_dm ,req_buff, &req_len, MODULE_VER_STR,imsi) != DM_PROC_SUCCESS)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]tele_uni_dm_task,dm_make_packet err!");
		goto out;
	}
		
	ret = dm_client(req_len, req_buff, uicc_type);

	
	if(ret == 0)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm success and store iccid");
		strncpy((char *)dm_pcontext->ue_iccid,(char *)nccid_dm,20);  //store g_ueiccid,20 byte
        dm_pcontext->have_retry_num = 0;
#if !VER_BC25
		send_urc_to_ext("+DM:SUCCESS", strlen("+DM:SUCCESS"));
#endif
		
		if(uicc_type == UICC_UNICOM)
		{
			dm_pcontext->last_reg_time = current_time;
			//联通DM设置下一次注册周期
			xy_rtc_timer_create(RTC_TIMER_CMCCDM, dm_pcontext->uni_reg_time, dm_timeout_proc, (uint8_t)0);
		}
	}
	else
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM]Register Fail\n");
		if (dm_pcontext->have_retry_num < dm_pcontext->retry_num)
		{
			dm_pcontext->have_retry_num++;
			xy_rtc_timer_create(RTC_TIMER_CMCCDM, dm_pcontext->retry_time, dm_timeout_proc, (uint8_t)0);
		}
		else
		{
			dm_pcontext->have_retry_num = 0;
			if(uicc_type == UICC_UNICOM)
			{
				//联通DM设置下一次注册周期
				xy_rtc_timer_create(RTC_TIMER_CMCCDM, dm_pcontext->uni_reg_time, dm_timeout_proc, (uint8_t)0);
			}
		}
	}
	dm_save_config_file();

out:
	if(req_buff)
	{	
		xy_free(req_buff);
	}
	

	if(g_dm_timer_flag == 1)
	{
		g_dm_timer_flag = 0;
	}

	xy_printf(0,XYAPP, WARN_LOG, "[TUDM]dm thread exit\n");
	g_dm_TskHandle = NULL;
	osThreadExit();
}

void tele_uni_dm_start_task(int uicc_type)
{		
	if(g_dm_TskHandle != NULL)
	{
		xy_printf(0,XYAPP, WARN_LOG, "[TUDM] dm aready running\n");
		return;
	}
	
	osThreadAttr_t thread_attr = {0};
	thread_attr.name	   = "tele_uni_dm";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = osStackShared;
	g_dm_TskHandle = osThreadNew((osThreadFunc_t)(tele_uni_dm_task), (void*)uicc_type, &thread_attr);
}
#endif


