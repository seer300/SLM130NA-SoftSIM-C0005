/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_ctl.h"
#include "at_utils.h"
#include "oss_nv.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define ERR_AT_IT(err) \
	{                  \
		err, #err      \
	}

#define POWER_ON_IT(reboot_num) \
    {                           \
        reboot_num, #reboot_num \
    }

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct
{
	int error_number;
	char *errname;
} at_err_msg_t;

typedef struct
{
	int  reboot_num;
	char *reboot_info;
} at_reboot_reason_t;

typedef enum
{
	FMT_INVALID = 0,	//非法解析，%l、%h、%s的个数不合理
	FMT_NORMAL,			//不含%l格式
	FMT_POINT,		//解析的数据为明文字符串，解析格式为%l+%s,需判断input_len == data_len
	FMT_HEX,		//解析的数据为16进制hex码，解析格式为%l+%h,需判断input_len * 2 == data_len
}fmt_type_t;

/*该结构体仅仅是为了解析字符串类型参数后，进行长度合法性检错而设计的，没有功能上的价值*/
typedef struct
{
	int input_len;			//AT输入的数据长度参数解析后的赋值，即%l所对应的那个参数值；
	int data_len;			//AT命令中字符串参数最终的有效内容长度，即对应%s或%h解析后字符串有效长度值
	uint8_t point_flag;		//用于判断解析的参数类型是否为%p，即地址指针
	fmt_type_t data_type;	//用于识别%l的捆绑关系 参考@fmt_type_t
}parse_data_t;

/*整形变量取值范围 - 或 | 的解析结果结构体*/
typedef struct
{	uint8_t range_type;		//0：未定义范围；1：定义了上下限；2：定义了有效值枚举
	uint32_t min_limit;
	uint32_t max_limit;
	char *valid_value_start;
}param_range_t;

/*******************************************************************************
 *						  Local variable definitions						   *
 ******************************************************************************/
static at_reboot_reason_t poweron_reason_list[] = 
{
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_RESET_PIN),
	POWER_ON_IT(REBOOT_CAUSE_APPLICATION_AT),
	POWER_ON_IT(REBOOT_CAUSE_APPLICATION_RTC),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_EXTERNAL_PIN),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_WATCHDOG),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_FOTA_UPGRADE),
	POWER_ON_IT(REBOOT_CAUSE_APPLICATION_SYSRESETREQ),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_GLOBAL_RESET),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_SOFT_RESET),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_WAKEUP_DSLEEP),
	POWER_ON_IT(REBOOT_CAUSE_SECURITY_RESET_UNKNOWN),
};

/*******************************************************************************
 *						   Global variable definitions						   *
 ******************************************************************************/
/*1表示依靠+CTZEU主动上报或NTP服务器方式更新当前世界时间；0表示通过AT+CCLK设置世界时间*/
int g_NITZ_mode = 0;
/* 级联命令处理返回结果码前不做URC缓存 */
uint8_t g_CombCmd_Doing = 0;

/*******************************************************************************
 *                       Local function implementations	                       *
 ******************************************************************************/
void set_echo_mode(uint8_t mode)
{
	HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(1 << 1))) | (mode << 1);
}

bool is_echo_mode()
{
	if(HWREGB(BAK_MEM_ATUART_STATE) & 0x02)
		return true;

	return false;
}

void set_cmee_mode(uint8_t mode)
{
	HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(0x03 << 2))) | (mode << 2);
}

uint8_t get_cmee_mode(void)
{
	return ((HWREGB(BAK_MEM_ATUART_STATE) & 0x0c) >> 2);
}

void set_at_lpuart_state(uint8_t state)
{
	HWREGB(BAK_MEM_ATUART_STATE) = (HWREGB(BAK_MEM_ATUART_STATE) & (~(1 << 4))) | (state << 4);
}

bool is_at_lpuart_doing()
{
	if ((HWREGB(BAK_MEM_ATUART_STATE) & 0x10) >> 4)
		return true;

	return false;
}

char *at_get_power_on_string()
{
    int reboot_num = at_get_power_on();
    int i;
	for (i = 0; i < (int)(sizeof(poweron_reason_list) / sizeof(poweron_reason_list[0])); ++i)
	{
		if (poweron_reason_list[i].reboot_num == reboot_num)
			return poweron_reason_list[i].reboot_info;
	}
	return NULL;
}


int at_get_power_on()
{
	if(Get_Boot_Reason() == POWER_ON)
		return REBOOT_CAUSE_SECURITY_PMU_POWER_ON_RESET;
	else if(Get_Boot_Reason() == GLOBAL_RESET)
	{
		if(Get_Boot_Sub_Reason() == PIN_RESET)
			return REBOOT_CAUSE_SECURITY_RESET_PIN;
		else if(Get_Boot_Sub_Reason() == WDT_RESET)
			return REBOOT_CAUSE_SECURITY_WATCHDOG;
		else
			return REBOOT_CAUSE_SECURITY_GLOBAL_RESET;
	}
	else if(Get_Boot_Reason() == SOFT_RESET)
	{
		if(Get_Boot_Sub_Reason() == SOFT_RB_BY_NRB)
			return REBOOT_CAUSE_APPLICATION_AT;
		else if(Get_Boot_Sub_Reason() == SOFT_RB_BY_RESET)
			return REBOOT_CAUSE_APPLICATION_SYSRESETREQ;
		else if(Get_Boot_Sub_Reason() == SOFT_RB_BY_FOTA)
			return REBOOT_CAUSE_SECURITY_FOTA_UPGRADE;
		else
			return REBOOT_CAUSE_SECURITY_SOFT_RESET;
	}
	else if(Get_Boot_Reason() == WAKEUP_DSLEEP)
	{
		if(Get_Boot_Sub_Reason() & (1 << AT_WAKUP))
			return REBOOT_CAUSE_SECURITY_EXTERNAL_PIN;
		else if(Get_Boot_Sub_Reason() & (1 << UTC_WAKUP))
			return REBOOT_CAUSE_APPLICATION_RTC;
		else
			return REBOOT_CAUSE_SECURITY_WAKEUP_DSLEEP;
	}
	return REBOOT_CAUSE_SECURITY_RESET_UNKNOWN;
}


/*获取特殊字符一共有多少个*/
uint8_t get_chr_num_in_str(char *str, char chr)
{
	uint8_t chr_num = 0;
	char *chr_addr = NULL;
	while(*str != '\0')
	{
		if((chr_addr = strchr(str, chr)) != NULL)
		{
			chr_num++;
			str = chr_addr + 1;
		}
		else
			break;
	}
	return chr_num;
}

static bool is_param_in_valid_range(uint32_t  val_trans, param_range_t *param_range)
{
	bool ret = true;

	if((param_range->range_type == 1 && (int)val_trans < 0) || val_trans < param_range->min_limit || val_trans > param_range->max_limit)
		ret = false;
	else if(param_range->range_type ==2) 
	{
		uint32_t valid_value = 0;
		char *value_head = param_range->valid_value_start;
		char *value_end = strchr(value_head, '|');
		while(1)
		{
			valid_value = (int)strtol(value_head,NULL,0);
			if(val_trans == valid_value)
				break;
			else if(value_end != NULL)
			{
				value_head = value_end + 1; 
				value_end = strchr(value_head, '|');
			}
			else
			{
				ret = false;
				break;
			}
		}
	}
	return ret;
}

/* 获取%d后面() []的int型参数范围 */
static void get_val_valid_range(char *fmt, char *required_flag, char *type, int *size, param_range_t *range)
{
	char len_str[6] = {0};
	char *left_bracket = NULL;
	char *right_bracket = NULL;
	char *left_square_bracket = NULL;
	char *right_square_bracket = NULL;
	char *mid_line = NULL;
	uint8_t break_num = 0;		//括号内参数合法值分隔符'|'的个数

	if(strlen(fmt) > 2)
	{
		left_bracket = strchr(fmt, '(');
		right_bracket = strchr(fmt, ')');
		left_square_bracket = strchr(fmt, '[');
		right_square_bracket = strchr(fmt, ']');
		if((left_bracket == NULL && right_bracket == NULL) && (left_square_bracket == NULL && right_square_bracket== NULL))
		{
			*type = *(fmt + strlen(fmt) - 1);
			strncpy(len_str, fmt + 1, strlen(fmt) - 2);
			*size = (int)strtol(len_str,NULL,10);
		}
		else if(((left_bracket != NULL && right_bracket != NULL) && (left_square_bracket == NULL && right_square_bracket== NULL)) || \
			((left_bracket == NULL && right_bracket == NULL) && (left_square_bracket != NULL && right_square_bracket != NULL)))
		{
			if(left_bracket !=NULL)
			{
				*required_flag = 1;
			}
			else
			{
				left_bracket = left_square_bracket;
				right_bracket = right_square_bracket;
			}
			*left_bracket = '\0';
			*type = *(fmt + strlen(fmt) - 1);
			mid_line = strchr(left_bracket + 1, '-');
			break_num = get_chr_num_in_str(left_bracket + 1, '|');

			xy_assert(left_bracket < right_bracket);
			if(*type != 'd' && *type != 'D' && *type != 'l' && *type != 'L')	//对于%s、%h、%p类型的参数只能输()或[]表示必选和可选，括号中间不能有其他字符
				xy_assert((left_bracket + 1) == right_bracket);	

			if(mid_line != NULL && break_num == 0)			//定义了上下限,例如(0-100),[2-10]
			{
				range->range_type = 1;
				xy_assert(mid_line < right_bracket);
				*right_bracket = '\0';
				range->min_limit = (int)strtol(left_bracket + 1,NULL,0);		//min_limit至少为0
				if(right_bracket - mid_line > 1)
					range->max_limit = (int)strtol(mid_line + 1,NULL,0);		//不输上限时，max_limit为0x0fffffff,-)间必须输有数字，否则max_limit解析为0

				if(range->min_limit > range->max_limit)
				{
					xy_assert(0);
				}
			}
			else if(mid_line == NULL && break_num != 0)		//定义了合法值枚举，例如（0|1|2|4）,[3|5|7]
			{
				range->range_type = 2;
				range->valid_value_start = left_bracket + 1;
			}
			else
			{
				xy_assert((left_bracket + 1) == right_bracket);			//%d()表示必选参数没有范围限制，括号之间不能有任何字符
			}

			if(strlen(fmt) > 2)
			{
				strncpy(len_str, fmt + 1, strlen(fmt) - 2);
				*size = (int)strtol(len_str,NULL,10);
			}
		}
		else
		{
			xy_assert(0);
		}
	}
	else
	{
		*type = *(fmt + strlen(fmt) - 1);
	}
}

/*str:某参数字符串; fmt:参数格式字符串; at_data:记录%p%l特殊参数解析结果，以便检查报错，没有特殊功能用途; arg:无意义;flag:无意义;ap:解析后参数值的空间数组链表*/
static int parse_type_val(char *str, char *fmt, parse_data_t *at_data, int* arg, int flag, va_list *ap)
{
	int ret = AT_OK;
	int size = 0;
	char type = 0;
	char required_flag = 0;
	param_range_t param_range = {0};
	param_range.max_limit = 0xffffffff;
	uint32_t val_trans = 0;

    /*fmt为空，跳过某参数解析，ap指针无需变更*/
    if (strlen(fmt)==0)
    {
		return ret;
	}
	else
	{
		get_val_valid_range(fmt, &required_flag, &type, &size, &param_range);

		/*源AT命令字符串中未携带该参数具体值，则识别是否为必选参数*/
        if (strlen(str) == 0)
		{
			va_arg(*ap, int);     /*参数解析目标空间链表，跳过该参数空间*/	
			if(required_flag == 1)
				ret = ATERR_PARAM_INVALID;
			return ret;
		}
	}

	at_data->point_flag = 0;

	/*一律按无符号进行解析，确保内存值正确。由用户在外部自行强转*/
	if(type == 'd' || type == 'D' || type == 'u' || type == 'U' || type == 'l' || type == 'L')
	{
		if(!is_digit_str(str) && !is_hex_str(str, 1))
		{
			ret = ATERR_PARAM_INVALID;
		}
		else if (size == 0 || size == 4)
		{
			*((unsigned int *)(va_arg(*ap, unsigned int))) = (unsigned int)strtoul(str, NULL, 0);
			val_trans = (unsigned int)strtol(str,NULL,0);
			if (flag == AT_PARAM_PARSE_ESC && arg != NULL)
				*arg += 1;
		}
		else if (size == 1)
		{
			*((char *)(va_arg(*ap, int))) = (char)strtol(str,NULL,0);
			val_trans = (unsigned int)strtol(str,NULL,0);
			if (flag == AT_PARAM_PARSE_ESC && arg != NULL)
				*arg += 1;

			if(param_range.range_type == 1 && param_range.max_limit > (unsigned int)0xff)
			{
				param_range.max_limit = 0xff;
			}
		}
		else if (size == 2)
		{
			*((short *)(va_arg(*ap, int))) = (short)strtol(str,NULL,0);
			val_trans = (unsigned int)strtol(str, NULL, 0);
			if (flag == AT_PARAM_PARSE_ESC && arg != NULL)
				*arg += 1;

			if(param_range.range_type == 1 && param_range.max_limit> (unsigned int)0xffff)
			{
				param_range.max_limit = 0xffff;
			}
		}
		else
		{
			xy_assert(0);
		}

		if(type == 'l' || type == 'L')
		{
			at_data->input_len = val_trans;
		}
		if(is_param_in_valid_range(val_trans, &param_range) == false)
			ret = ATERR_PARAM_INVALID;
	}
	/* %s : 可打印字符串解析*/
	else if (type == 's' || type == 'S')
	{
		if (size == 0)
		{
			if (flag == AT_PARAM_PARSE_ESC)
			{
				format_escape_char(str);
				if (arg != NULL)
					*arg += 1;
			}
			strcpy((char *)(va_arg(*ap, int)), str);
		}
		else
		{
			if (size <= (int)(strlen(str)))
			{
				ret = ATERR_PARAM_INVALID;
			}
			else
			{
				if (flag == AT_PARAM_PARSE_ESC)
				{
					format_escape_char(str);
					if (arg != NULL)
						*arg += 1;
				}
				int cpy_len = (int)(strlen(str));
				char *var_dst = (char *)(va_arg(*ap, int));
				strncpy(var_dst, str, cpy_len);
				*(char *)(var_dst + cpy_len) = '\0';
			}
		}
	}
	/* %p : 可打印的长字符串地址获取，少一次内存拷贝*/
	else if ((type == 'p' || type == 'P'))
	{
		at_data->point_flag = 1;
		if(at_data->data_type == FMT_POINT)
		{
			at_data->data_len = strlen(str);
		}

		if(size != 0 && size <= (int)strlen(str))
		{
			ret = ATERR_PARAM_INVALID;
			return ret;
		}

		*((int *)(va_arg(*ap, int))) = (int)str;
	}
	/* %h: 码流的16进制字符串形式，转换成二进制码流；一般搭配%l使用，需要检查长度合法性*/
	else if(type == 'h' || type == 'H')
	{
		at_data->data_len = strlen(str);
		if(at_data->data_len % 2 != 0)
		{
			ret = ATERR_PARAM_INVALID;
			return ret;
		}

		if(at_data->data_type != FMT_HEX)	//没有%l时，必须为类似%32h带长度限制的格式，不带长度则断言，此时入参长度必须为固定值32，否则报错
		{
			xy_assert(size != 0);
			if(at_data->data_len != size)
			{
				ret = ATERR_PARAM_INVALID;
				return ret;
			}
		}

		if(hexstr2bytes(str, at_data->data_len, (char *)(va_arg(*ap, int)), at_data->data_len/2) == -1)
			ret = ATERR_PARAM_INVALID;
	}
	else
	{
		xy_assert(0);
		return ret;
	}

	return ret;
}

//make sure get correct prefix,for example :AT+CGSN=1;+CIMI;+CGSN?, prefix should be AT+CGSN
static char* at_get_prefix_4_comb_cmd(char* start, char* end, uint8_t* type)
{
	xy_assert(start != NULL && end != NULL && start < end);
	char *tmp = start;
	while(tmp < end)
	{
		if (*tmp == '=')
		{
			if (*(tmp + 1) == '?')
			{
				*type = AT_CMD_TEST;
			}
			else
			{
				*type = AT_CMD_REQ;
			}
			return tmp;
		}
		else if (*tmp == '?')
		{
			*type = AT_CMD_REQ;
			return tmp;
		}
		tmp++;
	};

	if (*tmp == ';')
	{
		//example: AT+CIMI;+CGEQOS=0,9;+CGSN=1;+CGACT?
		*type = AT_CMD_ACTIVE;
		return tmp;
	}
	
	return NULL;
}

static const at_err_msg_t at_err_msg_tbl[] = {
	ERR_AT_IT(ATERR_XY_ERR),
	ERR_AT_IT(ATERR_PARAM_INVALID),
	ERR_AT_IT(ATERR_NOT_ALLOWED),
	ERR_AT_IT(ATERR_DROP_MORE),
	ERR_AT_IT(ATERR_DOING_FOTA),
	ERR_AT_IT(ATERR_MORE_PARAM),
	ERR_AT_IT(ATERR_WAIT_RSP_TIMEOUT),
	ERR_AT_IT(ATERR_CHANNEL_BUSY),
	ERR_AT_IT(ATERR_NO_MEM),
	ERR_AT_IT(ATERR_NOT_NET_CONNECT),
	ERR_AT_IT(ATERR_CONN_NOT_CONNECTED),
	ERR_AT_IT(ATERR_INVALID_PREFIX),

};

/*******************************************************************************
 *                       Global function implementations	                   *
 ******************************************************************************/
char *get_at_err_string(int error_number)
{
    int i;
    for (i = 0; i < (int)(sizeof(at_err_msg_tbl) / sizeof(at_err_msg_tbl[0])); ++i)
    {
        if (at_err_msg_tbl[i].error_number == error_number)
            return at_err_msg_tbl[i].errname;
    }
    return NULL;
}

char *check_atw_req_custom(char *at_cmd, int *flag, uint8_t *type)
{
	char *param = NULL;
	*flag = 0;
	if (at_strncasecmp(at_cmd, "AT&W", 4))
	{
		if (*(at_cmd + 4) == '\r')
		{
			*type = AT_CMD_ACTIVE;
			*flag = 1;
		}
		else if (*(at_cmd + 4) == '?' && *(at_cmd + 5) == '\r')
		{
			*type = AT_CMD_QUERY;
			*flag = 1;
		}
		else if (*(at_cmd + 4) >= '0' && *(at_cmd + 4) <= '9')
		{
			*type = AT_CMD_REQ;
			param = at_cmd + 4;
			*flag = 1;
		}
		else if (*(at_cmd + 4) == '=' && *(at_cmd + 5) == '?' && *(at_cmd + 6) == '\r')
		{
			*type = AT_CMD_TEST;
			*flag = 1;
		}
	}
	return param;
}

//获取的前缀at_prefix里不包含头尾标识，即'+''*''=''?';最终结果为“NRB”"WORKLOCK""ATI""AT"等
char *at_get_prefix_and_param(char *at_cmd, char *at_prefix, uint8_t *type)
{
	char *head;
	char *end;
	char *param = NULL;
	uint8_t atcmd_type = AT_CMD_INVALID;
	int at_prefix_len = 0; 

	
	memset(at_prefix, 0, AT_CMD_PREFIX);
	while (*at_cmd == '\r' || *at_cmd == '\n')
		at_cmd++;

	//deal with custom at&w command:AT&W,AT&W0,AT&W?,AT&W=?
	int is_atw_custom = 0;
	char *atw_param = check_atw_req_custom(at_cmd, &is_atw_custom, type);
	if(is_atw_custom)
	{
		strncpy(at_prefix, "AT&W", strlen("AT&W"));
		return atw_param;
	}

	//include param
	if (at_strncasecmp(at_cmd, "AT",2))
	{
		char *temp = at_cmd + 2;
		if(IS_HEAD_TAG(*temp))
		{
			head = temp + 1;
			if ((end = strchr(head, '=')) != NULL && *(end+1) == '?' && *(end+2) == '\r')
			{
				atcmd_type = AT_CMD_TEST;
				param = end + 1;
			}
			else if((end = strchr(head, '?')) != NULL && *(end+1) == '\r')
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CGEQOS=0,9;+CGSN=1;+CGACT?
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
					g_CombCmd_Doing = 1;
				}
				else
				{
					atcmd_type = AT_CMD_QUERY;
					param = end + 1;
				}
			}
			else if((end = strchr(head, '=')) != NULL)
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CIMI;+CGCMOD=0;
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
					g_CombCmd_Doing = 1;
				}
				else
				{
					atcmd_type = AT_CMD_REQ;
					param = end + 1;
				}
			}
			else if ((end = strchr(head, '\r')) != NULL)
			{
				char *tmp = NULL;
				if ((tmp = strchr(head, ';')) != NULL && tmp < end)
				{
					//example:AT+CGSN;+CIMI
					end = at_get_prefix_4_comb_cmd(at_cmd, tmp, &atcmd_type);
					param = end + 1;
					g_CombCmd_Doing = 1;
				}
				else
				{
					atcmd_type = AT_CMD_ACTIVE;
					param = end;
				}
			}
			else
				xy_assert(0);

			if (type != NULL)
				*type = atcmd_type;

			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
			}
			else
			{
				param = NULL;
				xy_printf(0,PLATFORM, WARN_LOG, "AT_CMD error!!!");
			}
		}
		//ATD
		else if (isalpha((int)(*temp)))
		{
			if (type != NULL)
				*type = AT_CMD_ACTIVE;
			at_prefix_len = 3;
			strncpy(at_prefix, at_cmd, at_prefix_len);
			param = at_cmd + 3;
		}
		else if (*temp == '\r')
		{
			/*eg: AT\r */
			if (type != NULL)
				*type = AT_CMD_ACTIVE;
			at_prefix_len = 2;
			strncpy(at_prefix, at_cmd, at_prefix_len);
			param = at_cmd + 3;
		}
		else
			param = NULL;
	}
	//主动上报
	else if (*at_cmd == '+' || *at_cmd == '^' || *at_cmd == '&')
	{
		head = at_cmd + 1;
		if ((end = strchr(head, ':')) != NULL)
		{
			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end + 1;
			}
			else
			{
				xy_printf(0,PLATFORM, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else if ((end = strchr(head, '\r')) != NULL)
		{
			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end;
			}
			else
			{
				xy_printf(0,PLATFORM, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else
		{
			xy_assert(0);
			param = NULL;
		}
	}
	else
	{
		param = NULL;
	}

	return param;
}

char *at_get_prefix_for_URC(char *at_cmd, char *at_prefix)
{
	char *head;
	char *end;
	char *param = NULL;
	int at_prefix_len = 0; 
	memset(at_prefix, 0, AT_CMD_PREFIX);
	while (*at_cmd == '\r' || *at_cmd == '\n')
		at_cmd++;


 	if (*at_cmd == '+' || *at_cmd == '^' || *at_cmd == '&')
	{
		head = at_cmd + 1;
		if ((end = strchr(head, ':')) != NULL)
		{
			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end + 1;
			}
			else
			{
				xy_printf(0,PLATFORM, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else if ((end = strchr(head, '\r')) != NULL)
		{
			at_prefix_len = end - head;
			if(at_prefix_len < AT_CMD_PREFIX)
			{
				strncpy(at_prefix, head, at_prefix_len);
				param = end;
			}
			else
			{
				xy_printf(0,PLATFORM, WARN_LOG, "AT_CMD error!!!");
			}
		}
		else
		{
			xy_assert(0);
			param = NULL;
		}
	}
	else
	{
		param = NULL;
	}

	return param;
}

bool is_sms_atcmd(char *at_prefix)
{
	char *sem_chr_list[] = {
		"CMGS",
		"CMGC",
		"CNMA",
	};
	unsigned int i = 0;
	for (i = 0; i < sizeof(sem_chr_list) / sizeof(sem_chr_list[0]); i++)
	{
		if (strstr(at_prefix, sem_chr_list[i]))
			return true;
	}
	return false;
}

/*用于对携带%l的格式化字符串进行类别判断*/
fmt_type_t get_fmt_type(char *fmt_param)
{
	uint8_t lenfmt_num = get_chr_num_in_str(fmt_param, 'l') + get_chr_num_in_str(fmt_param, 'L');
	uint8_t hexfmt_num = get_chr_num_in_str(fmt_param, 'h') + get_chr_num_in_str(fmt_param, 'H');
	uint8_t pointfmt_num = get_chr_num_in_str(fmt_param, 'p') + get_chr_num_in_str(fmt_param, 'P');

	if(lenfmt_num == 0)
	{
		return FMT_NORMAL;
	}
	else if(lenfmt_num == 1)
	{
		if(hexfmt_num == 1)
		{
			return FMT_HEX;
		}
		else if(hexfmt_num == 0 && pointfmt_num == 1)
		{
			return FMT_POINT;
		}
	}

	return FMT_INVALID;
}

//参数被“”包围的时候，从'“'的下一个字符开始，找与之匹配的'”'
char *get_quote_end(char *quote_next)
{
	uint8_t quote_num = 1;
	char *data = quote_next;
	char *tail = quote_next;

	while (tail = strchr(data, '"'))
	{
		quote_num++;
		if (quote_num % 2 == 0 && (*(tail + 1) == ',' || *(tail + 1) == '\0'))
			return tail;
		data = tail + 1;
	}
	return NULL;
}

/*该接口仅内部函数调用，参数解析以可变入参方式提供，类似scanf*/
int parse_param(char *fmt_parm, char *buf, int is_strict, int* arg, int flag, va_list *ap)
{
	xy_assert(fmt_parm != NULL && buf != NULL);
	int ret = AT_OK;
	char *param_comma = NULL;
	char *param_quotes_end = NULL;
	char *fmt_comma = NULL;
	char *param_end = NULL;
	//fmt指针指向随时会变化，释放内存时必须从起始释放，zallco首地址也必须有记录(fmt_original变量)
	int fmt_len = strlen(fmt_parm);
	char *fmt_original = xy_malloc(fmt_len + 1);
	*(fmt_original + fmt_len) = '\0';
	char *fmt = fmt_original; 
	strncpy(fmt, fmt_parm, fmt_len);
	parse_data_t at_data = {0};

	at_data.data_type = get_fmt_type(fmt);
	if(at_data.data_type == FMT_INVALID)
	{
		ret = ATERR_PARAM_INVALID;
		goto END;
	}

	while (*buf == ' ')
		buf++;

	param_end = strchr(buf, '\r');
	if (param_end != NULL)
		*param_end = '\0';

	while (*buf != '\0' || *fmt != '\0')
	{
		/*跳过参数头部空格。%s字符串类型若头部有空格，需用""圈定*/
		while(*buf == ' ')
		{
			buf++;
		}
		while(*fmt == ' ')
		{
			fmt++;
		}
		
		param_comma = strchr(buf, ',');
		fmt_comma = strchr(fmt, ',');
		param_quotes_end = NULL;

		//如果参数带引号，则去掉引号
		if (*buf == '"')
		{
			buf++;
			if (flag == AT_PARAM_PARSE_ESC)
				param_quotes_end = find_next_double_quato(buf);
			else
				param_quotes_end = get_quote_end(buf);
			//只有左引号，没有右引号
			if (param_quotes_end == NULL)
			{
				ret = ATERR_PARAM_INVALID;
				break;
			}

			param_comma = strchr(param_quotes_end + 1, ',');
			*param_quotes_end = '\0';
		}

		/* AT命令参数字符串和fmt格式字符串后续都有逗号，则强行将逗号改为'\0'，待处理完当前参数后再恢复 */
		if (param_comma && fmt_comma)
		{
			*param_comma = '\0';
			*fmt_comma = '\0';

			if ((ret = parse_type_val(buf, fmt, &at_data, arg, flag, ap)) != AT_OK)
				break;
			*fmt_comma = ',';
			/*若为%p，则外部用户会直接访问对应字符串，进而不能把尾部的'\0'恢复正常；该行为强行修改了入参AT字符串，考虑到该字符串仅在对应的AT命令解析函数中有生命，问题不大*/
			if(at_data.point_flag == 0)
			{
				*param_comma = ',';
				if (param_quotes_end)
					*param_quotes_end = '"';
			}
			buf = param_comma + 1;
			fmt = fmt_comma + 1;
		}
		/* AT命令参数字符串后续有逗号，而fmt格式字符串已是最后参数时*/
		else if (param_comma)
		{
			*param_comma = '\0';
			ret = parse_type_val(buf, fmt, &at_data, arg, flag, ap);

			//too many param
			if (ret == AT_OK && is_strict)
				ret = ATERR_MORE_PARAM;
			break;
		}
		/* fmt格式字符串后续有逗号，而AT命令参数字符串已是最后参数时*/
		else if (fmt_comma)
		{
			*fmt_comma = '\0';
			ret = parse_type_val(buf, fmt, &at_data, arg, flag, ap);
			if(ret == AT_OK)
			{
				buf = buf + strlen(buf);
				if(param_quotes_end)
				{
					buf++;
					if(at_data.point_flag == 0)
						*param_quotes_end = '"';
				}
				*fmt_comma = ',';
				fmt = fmt_comma + 1;
			}
			else
				break;
		}
		/* AT命令参数字符串和fmt格式字符串皆已解析到最后一个参数时*/
		else
		{
			ret = parse_type_val(buf, fmt, &at_data, arg, flag, ap);
			break;
		}
	}

	//AT字符串携带的长度值与对应的字符串实际传输长度不一致，报错；该类型错误往往是测试人员故意制造的错误AT命令
	if((at_data.data_type == FMT_POINT && at_data.input_len != at_data.data_len && at_data.input_len * 2 != at_data.data_len) || (at_data.data_type == FMT_HEX && at_data.input_len * 2 != at_data.data_len))
	{
		ret = ATERR_PARAM_INVALID;
	}

END:
	if(at_data.point_flag == 0)
	{
		if (param_end)
			*param_end = '\r';
		if (param_comma)
			*param_comma = ',';
		if (param_quotes_end)
			*param_quotes_end = '"';
	}
	xy_free(fmt_original);
	return ret;
}

