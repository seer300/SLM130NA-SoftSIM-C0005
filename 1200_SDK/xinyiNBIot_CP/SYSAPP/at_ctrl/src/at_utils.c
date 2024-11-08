/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_utils.h"
#include "at_ctl.h"
#include "xy_at_api.h"
#include "xy_utils.h"
#include "softap_nv.h"

/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/
/*
 * 转义字符表，用于转义字符转换ASCII码
*/
esc_item_t esc_table[] =
{
	{'a', '\a'},
	{'b', '\b'},
	{'f', '\f'},
	{'n', '\n'},
	{'r', '\r'},
	{'t', '\t'},
	{'v', '\v'},
	{'?', '\?'},
	{'\\', '\\'},
	{'\'', '\''},
	{'\"', '\"'},
};

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/


/*识别source是否为substr对应的AT命令字符串，其中substr必须为字母串，不得添加头尾标识，如“+”/“=”/“？”等，如substr=“NRB”      、“AT”*/
char * at_prefix_strstr(char * source, char * substr)
{
	uint32_t n = strlen(substr);
	char *head;
	char temp = 0;

	//URC会携带头部空格
	while(*source == '\r' || *source == '\n')
		source++;
	
	if(strlen(source) > n+5)
	{
		temp = *(source+n+5);
		*(source+n+5) = 0;
	}

	head = strstr(source,substr);

	if(temp != 0)
		*(source+n+5) = temp;
	
	if(head == NULL)
		return NULL;
	//有两个AT命令的前缀属于包含关系，需要确保完全匹配才行
	else if(((head > source) && (isalpha((int)(*(head-1))) || isdigit((int)(*(head-1))))) || isalpha((int)(*(head+n))) || isdigit((int)(*(head+n))))
		return NULL;
	else
		return head+n;
}

/*仅用于参数解析时的第几个关键字符匹配，如逗号、双引号等，需要关注n取值正确*/
char * at_strnchr(char *s, int c, int n)
{
	char *match = NULL;
	int i = 0;

	do {
		if (*s == (char)c) {
			i++;
			if (i == n) {
				match = (char *)s;
				break;
			}
		}
	} while (*s++);

	return match;
}

bool is_digit_str(char *str)
{
    unsigned digit_len = 0;

	if(*str == '-'){
		str++;
	}
    while(isdigit((int)(*str))){
        str++, digit_len++;
    }
    while(*str == ' '){
        str++;
    }
    return *str == '\0' && digit_len > 0;
}

bool is_hex_str(char *str, uint8_t check_head)
{
    uint16_t digit_len = 0;

	if(check_head)
	{
		if(strncmp(str,"0x",2) == 0 || strncmp(str,"0X",2) == 0)
			str += 2;
		else
			return false;
	}

    while(isxdigit((int)(*str)))
	{
        str++;
		digit_len++;
    }

    while(*str == ' ')
	{
        str++;
    }

    return (*str == '\0' && digit_len > 0);	
}


/* 检测是否为OK应答结果码;返回 1 表示匹配成功 */
bool Is_AT_Rsp_OK(char *str)
{
	int ret = 0;
	int n = strlen(str);
	char *temp = str;

	//+URC:XXXX\r\n\r\nOK\r\n;减少无效匹配
	if(n > 6)
	{
		temp = str+n-6;
	}

	if(strstr(temp,"OK\r") != NULL)
	{
		ret = 1;
	}

	return  ret;
}

int Get_AT_errno(char *str)
{
	int err_no = 0;
	char *err_head = NULL;
	int str_len = strlen(str);

	if(get_cmee_mode() != 2 && str_len > 15)
	{
		str = str + str_len - 15;
	}

	if((err_head = strstr(str, "ERROR")) != NULL)
	{
		err_no = ATERR_XY_ERR;
		if((err_head = strchr(err_head, ':')) != NULL && get_cmee_mode() == 1)
		{
			err_no = (int)strtol(err_head + 1, NULL, 10);
		}
	}

	return err_no;
}

bool Is_Result_AT_str(char *str)
{
	int ret = 0;
	int n = strlen(str);
	char *temp = str;

	if (g_softap_var_nv->powerdown_flag == 1)
	{
		if (strstr(temp, "NORMAL POWER DOWN") != NULL)
		{
			return 1;
		}
	}

	if(!strncmp(str, "\r\nOK\r\n", strlen("\r\nOK\r\n")))
		return 1;

	//ERROR:XXXX\r\n;减少无效匹配
	if(get_cmee_mode() != 2 && n > 15)
	{
		temp = str+n-15;
	}

    if ((strstr(temp, "ERROR") || strstr(temp, "OK\r\n") || strstr(temp, "FAIL\r\n")) && !strstr(temp,"+NPIN:"))
    {
        ret = 1;
    }

    return ret;
}

char *at_ok_build()
{
	char *at_str;

	at_str = xy_malloc(7);
	snprintf(at_str, 7, AT_RSP_OK);
	return at_str;
}

int is_at_char(char c)
{
    return (((((unsigned char)c) >= ' ') && (((unsigned char)c) <= '~')) || c == '\r' || c == '\n');
}

bool isHexChar(char ch)
{
	if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
		return true;
	else
		return false;
}

char covertHextoNum(char ch)
{
	char temp=0;
	if (isdigit(ch))
		temp = (ch - '0');

	if (isalpha(ch))
		temp = (isupper(ch) ? ch - 55 : ch - 87);
	return (char)temp;
}

bool isOctChar(char ch)
{
	if ((ch >= '0' && ch <= '7'))
		return true;
	else 
		return false;
}

int covertEscToAscII(char* p)
{
	int len = -1;
	int index_val = *(p + 1);
	int i = 0;
	for (; i < (int)(sizeof(esc_table) / sizeof(esc_item_t)); i++)
	{
		if (index_val == esc_table[i].ch)
		{
			*p = esc_table[i].esc_ch;
			xy_printf(0,PLATFORM, WARN_LOG, "match esc table and changed:%d", *p);
			return 1;
		}
	}

	//Hex
	if(index_val == 'x')
	{
		if(isHexChar(*(p + 2)) && isHexChar(*(p + 3)))
		{
			*p = (covertHextoNum(*(p + 2)) << 4) + covertHextoNum(*(p + 3));
			xy_printf(0,PLATFORM, WARN_LOG, "hex changed:%d", *p);
			return 3;
		}
		else if(isHexChar(*(p + 2)))
		{
			*p = covertHextoNum(*(p + 2));
			xy_printf(0,PLATFORM, WARN_LOG, "hex changed:%d", *p);
			return 2;			
		}
	}

	//Octal eg:/101
	if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)) && isOctChar(*(p + 3)))
	{
		*p = (char)(((*(p + 1) - '0') << 6) + ((*(p + 2) - '0') << 3) + (*(p + 3) - '0'));
		xy_printf(0,PLATFORM, WARN_LOG, "oct changed:%d", *p);
		return 3;
	}
	else if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)))
	{
		*p = (char)(((*(p + 1) - '0') << 3) + (*(p + 2) - '0'));
		xy_printf(0,PLATFORM, WARN_LOG, "oct changed:%d", *p);
		return 2;
	}
	else if (isOctChar(*(p + 1)))
	{
		*p = (char)(*(p + 1) - '0');
		xy_printf(0,PLATFORM, WARN_LOG, "oct changed:%d", *p);
		return 1;
	}

	return len;
}

void format_escape_char(char *input)
{
	xy_assert(input != NULL);
	char *p = input;
	int offset = 0;
	unsigned int i = 0;

	while ((p = strchr(p, '\\')) != NULL && *(p + 1) != '\0')
	{
		if ((offset = covertEscToAscII(p)) != -1)
		{
			if(strlen(p + offset + 1) + 1 < (unsigned int)(offset)) //eg: \101a --> Aa,  原先1a位置必须置0
			{
				memset(p + 1, '\0', (unsigned int)(offset));
			}	
			for (i = 0; i <= strlen(p + offset + 1); i++)
			{
				*(p + 1 + i) = *(p + offset + 1 + i);
			}
		}
		p = p + 1;
	};
}

char* find_next_double_quato(char* data)
{
	xy_assert(data != NULL);
	char *tmp = NULL;

	while(*data != '\0')
	{
		if(*data == '"' && *(data - 1) != '\\')
		{
			tmp = data;
			break;
		}
		data++;
	}

	return tmp;
}

//不区分大小写的字符串比较，1表示一样，例如“IPV6” “ipv6”
bool at_strcasecmp(const char *s1, const char *s2)
{
    int ch1 = 0;
    int ch2 = 0;

    if(NULL == s1 || NULL == s2)
	{
        return 0;
    }

    do
	{
        if((ch1 = *(unsigned char *)s1++) >= 'a' && (ch1 <= 'z'))
		{
            ch1 -= DIFF_VALUE;
        }
        if((ch2 = *(unsigned char *)s2++) >= 'a' && (ch2 <= 'z'))
		{
            ch2 -= DIFF_VALUE;
        }
    }while(ch1 && (ch1 == ch2));

	if(ch1 == ch2)
		return 1;
	else
		return 0;
}

//不区分大小写的子父字符串比较，其中n表示仅匹配父字符串前n字节
bool at_strncasecmp(const char *s1, const char *s2, int n)
{
    int ch1 = 0;
    int ch2 = 0;

    if(NULL == s1 || NULL == s2 || n<1)
	{
        return 0;
    }

    do
	{
        if((ch1 = *(unsigned char *)s1++) >= 'a' && (ch1 <= 'z'))
		{
            ch1 -= DIFF_VALUE;
        }
        if((ch2 = *(unsigned char *)s2++) >= 'a' && (ch2 <= 'z'))
		{
            ch2 -= DIFF_VALUE;
        }
    }while(--n && ch1 && (ch1 == ch2));

    if(ch1 == ch2)
		return 1;
	else
		return 0;
}

/*为了防止URC风暴对外部MCU的影响，此处供用户添加不需要的URC拦截，返回1表示不发送给外部MCU*/
int urc_filter(char* str)
{
	UNUSED_ARG(str);
	return 0;
}
