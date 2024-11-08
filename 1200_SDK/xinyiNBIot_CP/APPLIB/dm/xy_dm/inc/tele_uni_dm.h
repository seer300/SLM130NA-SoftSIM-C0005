#pragma once

#define DM_REG_VER_LEN	    (2+1) 		
#define DM_SW_VER_LEN	    (30+1)
#define DM_MOUDLE_VER_LEN	(20+1)
#define DM_OS_VER_LEN		(32+1)
#define DM_IMEI_LEN         (15+1)
#define DM_UETYPE_LEN 		(4+1)
#define DM_HOST_NAME_LEN    (40+1)

#define DM_UNI_REG_TIME_MIN		  86400	   		//秒,联通DM的注册周期最小值
#define DM_UNI_REG_TIME_MAX		  5184000  		//秒,联通DM的注册周期最大值
#define DM_RETRY_TIME_MAX			7200   		//秒 电信和联通DM自注册失败后,再次发起重试的间隔时长最大值
#define DM_RETRY_TIME_MIN 			1800 		//秒,0.5小时 电信和联通DM自注册失败后,再次发起重试的间隔时长最小值
#define DM_RETRY_NUM_MAX 			10    		//电信和联通DM自注册失败后，重试次数最大值

#define DM_TELE_RETRY_NUM_DEFAULT 	10    				//电信DM自注册失败后，重试次数的默认值
#define DM_UNI_RETRY_NUM_DEFAULT 	3	  				//联通DM自注册失败后，重试次数的默认值
#define DM_RETRY_TIME_DEFAULT       3600  				//秒，电信和联通DM自注册失败后,再次发起重试的间隔时长的默认值
#define DM_UNI_REG_TIME_DEFAULT     2592000 			//秒(30 days),联通DM的注册周期,默认值		
#define DM_TELE_PORT_DEFAULT        5683        		//电信DM,默认端口号
#define DM_UNI_PORT_DEFAULT         9999        		//联通DM,默认端口号
#define DM_UNI_HOST_DEFAULT         "114.255.193.236" 	//联通DM服务器默认地址
#define DM_TELE_HOST_DEFAULT        "zzhc.vnet.cn"   	//电信DM服务器默认地址
#define DM_HOST_PATH_DEFAULT 	    "nb"
#define DM_REGVER_DEFAULT_STR       "2"					//电信和联通注册版本号默认值，最大2字节
#define DM_OSVER_DEFAULT_STR        ""     				//终端dm参数 软件版本号 默认值，最大30个字节
#define DM_UETYPE_DEFAULT_STR       ""    				//终端dm参数 终端类型 4个字节

#define USER_MESSAGE_JSONSTR_BUFF_LEN 			450		//上报内容由客户自行决定时json_buff的长度
#define USER_MESSAGE_BASE64_JSONSTR_BUFF_LEN 	600		//上报内容由客户自行决定时,base_64编码后json_buff的长度
#define USER_MESSAGE_SEND_BUFF_LEN 				700		//上报内容由客户自行决定时send_buf的长度
#define XINYI_MESSAGE_JSONSTR_BUFF_LEN 			300		//上报内容由芯翼组包时json_buff的长度
#define XINYI_MESSAGE_BASE64_JSONSTR_LEN 		400		//上报内容由芯翼组包时,base_64编码后json_buff的长度
#define XINYI_MESSAGE_SEND_BUFF_LEN 			500 	//上报内容由芯翼组包时send_buf的长度


typedef enum
{
	DM_AT_INITIALIZE_ERR = 800,	        //DM初始化失败
	DM_AT_PARAMS_VALUE_ERR = 801,		//AT命令参数值错误
	DM_AT_SETTING_TYPE_ERR = 802,		//用户未设置DM配置文件所属运营商，或者DM配置文件所属运营商与当前SIM运营商不一致
}DM_AT_PROCESS_ERR;						//AT命令返回错误错误码

typedef enum
{
	DM_SETTING_UNKOWN = 0,	//未知
	DM_SETTING_TELECOM = 1,	//电信
	DM_SETTING_UNICOM = 2,	//联通
}DM_SETTING_TYPE;			//当前配置文件所属运营商类型


typedef enum
{
	DM_SET_SETTING_CFG = 0,	    //设置DM配置文件所属运营商,平台连接参数
	DM_SET_JSON_PARAMS,		    //设置DM注册时json字符串中字段的值
	DM_SET_JSON_STRING,		    //设置用户自定义json字符串内容,用于DM注册,该模式下仅检查是否为合法的json字符串,字符串内容由用户自行保证正确性
}DM_SET_MODE_TYPE;

typedef enum
{
	XINYI_MESSAGE_FORMAT = 0,    //json字符串由芯翼,根据用户AT命令设置的参数值,组建json字符串
	USER_MESSAGE_FORMAT = 1,     //json字符串由用户通过AT命令输入,芯翼仅检查格式的合法性,透传内容
}DM_REPORT_MODE_E;//DM自注册报文中json字符串的来源

typedef enum
{
	DM_PROC_SUCCESS = 0,//成功
	DM_PROC_FAILED = -1,//失败
}DM_PROCESS_RESULT;


typedef struct dm_context_nvm_s
{
	uint8_t  setting_uicc_owner;   			//电信和联通共用同一文件,指示当前配置文件属于电信还是联通,用户未配置时为0，配置过电信参数为1，配置过联通参数为2
	uint8_t  retry_num;     				//电信和联通DM自注册失败的重试次数
	uint16_t retry_time;    				//电信和联通DM自注册失败后间隔若干时间后再进行重启，缺省值为 DM_RETRY_TIME_DEFAULT
	uint32_t uni_reg_time;	   				//联通DM的注册周期,默认值为DM_UNI_REG_TIME_DEFAULT
	uint8_t  host[DM_HOST_NAME_LEN];	   	//dm自注册运营商域名,最大长度40个字节
	uint16_t port;          				//dm 自注册运营商端口号 电信默认DM_TELE_PORT_DEFAULT 联通DM_UNI_PORT_DEFAULT
	
	uint8_t  report_mode;     				//DM自注册报文中的json字符串来源 0:芯翼组json字符串  1:用户通过AT命令输入待上报的json字符串内容，芯翼仅检查格式
	int32_t  uicc_type;		   				//SIM卡运营商类型
	uint8_t  have_retry_num;				//记录DM失败后，已经尝试的总次数
	uint8_t  ue_iccid[23];					//保存已经成功注册的iccid号
	uint32_t last_reg_time;					//最后一次DM成功的时间

	uint8_t  json_report_str[401];			//report_mode为0时,保存用于DM自注册的部分参数内容,report为1时,保存用于DM自注册的json字符串内容
}dm_context_nvm_t;//电信和联通二选一，保存到FS的DM配置参数

typedef struct dm_context_s {
	
	uint8_t  setting_uicc_owner;   			//电信和联通共用同一文件,指示当前配置文件属于电信还是联通,用户未配置时为0，配置过电信参数为1，配置过联通参数为2
	uint8_t  retry_num;     				//电信和联通DM自注册失败的重试次数
	uint16_t retry_time;    				//电信和联通DM自注册失败后间隔若干时间后再进行重启，缺省值为 DM_RETRY_TIME_DEFAULT
	uint32_t uni_reg_time;	   				//联通DM的注册周期,默认值为DM_UNI_REG_TIME_DEFAULT
	uint8_t  host[DM_HOST_NAME_LEN];	   	//dm自注册运营商域名,最大长度40个字节
	uint16_t port;          				//dm 自注册运营商端口号 电信默认DM_TELE_PORT_DEFAULT 联通DM_UNI_PORT_DEFAULT
	
	uint8_t  report_mode;     				//DM自注册报文中的json字符串来源 0:芯翼组json字符串  1:用户通过AT命令输入待上报的json字符串内容，芯翼仅检查格式
	int32_t  uicc_type;		   				//SIM卡运营商类型
	uint8_t  have_retry_num;				//记录DM失败后，已经尝试的总次数
	uint8_t  ue_iccid[23];					//保存已经成功注册的iccid号
	uint32_t last_reg_time;					//最后一次DM成功的时间

	char*    json_report_str;               //report_mode为0时,保存用于DM自注册的部分参数内容,report为1时,保存用于DM自注册的json字符串内容

}dm_context_t;//电信/联通运行时内容


/*
 * @brief DM全局初始化
 * 
 * @return int32_t  0:成功 -1：失败
 */
int32_t dm_context_init();

/**
 * @brief 保存DM文件
 * 
 * @return int32_t 
 */
void dm_save_config_file();

/**
 * @brief 获取DM内容全局
 * 
 * @return dm_context_t* 
 */
dm_context_t* dm_get_context();

/**
 * @brief 获取系统默认的电信/联通DM自注册失败的重试次数
 * 
 * @return uint8_t 重试次数
 */
uint8_t dm_get_retry_num_def_val(void);

/**
 * @brief 获取系统默认的联通DM的注册周期
 * 
 * @return uint32_t 联通DM的注册周期
 */
uint32_t dm_get_uni_reg_time_def_val(void);

/**
 * @brief 获取系统默认的电信/联通的DM服务器端口号
 * 
 * @return uint16_t DM服务器端口号
 */
uint16_t dm_get_port_def_val(void);

/**
 * @brief 获取系统默认的电信/联通的DM服务器IP地址或域名
 * 
 * @param host DM服务器IP地址或域名
 */
void dm_get_host_def_val(char *host);

/**
 * @brief 获取系统默认的电信/联通DM自注册失败后,再次发起注册的时间间隔
 * 
 * @return uint16_t 
 */
uint16_t dm_get_retry_time_def_val();

/**
 * @brief 获取系统默认的regver
 * @note  默认值来源：DM_REGVER_DEFAULT
 * @param regver 
 */
void dm_get_regver_def_str(char * regver);

/**
 * @brief 获取系统默认的swver
 * @note  默认值来源：出厂NV
 * @param swver 
 */
void dm_get_swver_def_str(char *swver);

/**
 * @brief 获取系统默认的modver
 * @note  默认值来源：出厂NV
 * @param modver 
 */
void dm_get_modver_def_str(char * modver);

/**
 * @brief 获取系统默认的osver,默认为""
 * @note  默认值来源:DM_OSVER_DEFAULT_STR
 * @param osver 
 */
void dm_get_osver_def_str(char * osver);

/**
 * @brief 获取系统默认的imei,调用xy_get_IMEI接口获取
 * 
 * @param imei 
 * @return int32_t 0成功；-1：失败
 */
int32_t dm_get_imei_def_str(char *imei);

/**
 * @brief 获取系统默认的UETYPE
 * @note  默认值来源：DM_UETYPE_DEFAULT_STR
 * @param uetype 
 */
void dm_get_uetype_def_str(char *uetype);

/**
 * @brief 检查setting_owner类型与当前SIM卡是否匹配
 * 
 * @param setting_uicc_owner 
 * @return true 与当前SIM卡匹配
 * @return false 与当前SIM卡不匹配
 */
bool dm_check_if_setting_owner_valid(uint8_t setting_uicc_owner);

/**
 * @brief 设置DM配置文件所属运营商是电信还是联通
 * 
 * @param setting_type 1：电信 2：联通
 */
void dm_set_setting_owner(uint8_t setting_uicc_owner);

/**
 * @brief 检测当前配置文件所属运营商与SIM识别运营商是否一致
 * 
 * @return true  //允许配置DM文件
 * @return false //用户未设置配置文件所属运营商，或者当前配置文件所属运营商与当前SIM卡所属运营商不一致，不允许配置DM文件,返回false
 */
bool dm_check_if_cfg_uicc_owner_match_uicc_type();

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
void dm_set_setting_cfg_params(uint8_t setting_uicc_owner, uint8_t retry_num, uint16_t retry_time, uint32_t uni_reg_time, uint16_t port,char * host);

/**
 * @brief 设置电信/联通DM的开关
 * 
 * @param dm_switch
 */
void dm_set_switch(uint8_t dm_switch);

/**
 * @brief 将regver,uetype,swver,modver,osver,imei参数组成json字符串并保存到FS
 * 
 * @param regver 
 * @param uetype 
 * @param swver 
 * @param modver 
 * @param osver 
 * @param imei 
 */
void dm_set_params_in_jsonstr_format(char *regver, char *uetype, char *swver, char *modver, char *osver, char* imei);


/**
 * @brief 设置用户自定义的用于DM自注册时上报的json字符串并保存到FS
 * 
 * @param jsonstring 
 * @return int32_t 
 */
int32_t dm_set_others_json_context(const char * jsonstring);

int32_t dm_get_imei(char *imei);
int32_t dm_get_osver(char *osver);
int32_t dm_get_model(char *model);
int32_t dm_get_swver(char *swver);
int32_t dm_get_uetype(char *uetype);
int32_t dm_get_regver(char * regver);