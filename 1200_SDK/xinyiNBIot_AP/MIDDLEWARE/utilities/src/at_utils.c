#include "xy_list.h"
#include "xy_system.h"
#include "xy_at_api.h"
#include "sys_ipc.h"
#include "xy_timer.h"
#include "system.h"
#include "hal_def.h"
#include "ap_watchdog.h"
#include "hw_types.h"
#include "xy_printf.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct
{
	char ch;          //转义字符 如\a 中的a
	char esc_ch;      //转义字符对应的ASCII码
} esc_item_t;

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
	

typedef struct
{
	int data_len;			//待解参数的长度,已去除空格和引号
	uint8_t point_flag;		//用于判断解析的参数类型是否为%p
}parse_data_t;

//检测入参字符串是否为数字型字符串，跳过头部空格
static int is_digit_str(char *str)
{
    unsigned digit_len = 0;

    while(*str == ' '){
        str++;
    }
	if(*str == '-'){
		str++;
	}
    while(isdigit((int)*str)){
        str++; digit_len++;
    }
    while(*str == ' '){
        str++;
    }
    return *str == '\0' && digit_len > 0;
}

//用于匹配字符串，返回值为匹配成功的下一个有效字符地址
char *at_strstr(char *src,char *substr)
{
	char *ret = NULL;

	ret = strstr(src,substr);

	if(ret == NULL)
	{
		return  NULL;
	}
	else
	{
		char *head = (ret+strlen(substr));

		//跳过空格
		while(*head == ' ')
			head++;

		return  head;
	}
}

/*返回匹配后的下一个地址*/
__OPENCPU_FUNC char * fast_Strstr(char * source, char * substr)
{
	unsigned int n = strlen(substr);
	char *head;
	char temp = 0;


	//为了高效，source大内存时拷贝头部进行比较
	if(strlen(source) > n+30)
	{
		temp = *(source+n+30);
		*(source+n+30) = 0;
	}

	head = strstr(source,substr);	
	
	if(temp != 0)
		*(source+n+30) = temp;

	if(head == NULL)
		return	NULL;
	else
		return (head+n);
}



//用于前缀匹配，返回值为匹配成功的下一个有效字符地址
__OPENCPU_FUNC char *at_prefix_strstr(char *source,char *substr)
{
	unsigned int n = strlen(substr);
	char *head;
	char temp = 0;

	while(*source ==' ')
		source++;

	//为了高效，source大内存时拷贝头部进行比较
	if(strlen(source) > n+15)
	{
		temp = *(source+n+15);
		*(source+n+15) = 0;
	}

	extern char	*strcasestr (const char *, const char *);
	head = strcasestr(source,substr);	//匹配前缀时不区分大小写

	if(temp != 0)
		*(source+n+15) = temp;

	/*子字符串匹配，不作为匹配成功，例如AT+QCCLK与AT+CCLK不是一个前缀*/
	if(head==NULL || *(head-1)!='+' || isalpha(*(head+n)))
		return  NULL;
	else
		return (head+n);
}


/*skip_num值为需要解析的参数个数-1，表示跳过前面的参数个数*/
char* find_special_symbol(char* str,char symbol,int skip_num)
{
	int num = 0;

	if(skip_num == 0)
		return str;

	while(*str != '\0')
	{
		if(*str == symbol)
		{
			num++;
			if(num == skip_num)
				return str+1;
		}
		str++;
	}

	return NULL;
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
			return 1;
		}
	}

	//Hex
	if(index_val == 'x')
	{
		if(isHexChar(*(p + 2)) && isHexChar(*(p + 3)))
		{
			*p = (covertHextoNum(*(p + 2)) << 4) + covertHextoNum(*(p + 3));
			return 3;
		}
		else if(isHexChar(*(p + 2)))
		{
			*p = covertHextoNum(*(p + 2));
			return 2;
		}
	}

	//Octal eg:/101
	if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)) && isOctChar(*(p + 3)))
	{
		*p = (char)(((*(p + 1) - '0') << 6) + ((*(p + 2) - '0') << 3) + (*(p + 3) - '0'));
		return 3;
	}
	else if (isOctChar(*(p + 1)) && isOctChar(*(p + 2)))
	{
		*p = (char)(((*(p + 1) - '0') << 3) + (*(p + 2) - '0'));
		return 2;
	}
	else if (isOctChar(*(p + 1)))
	{
		*p = (char)(*(p + 1) - '0');
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

    while(isxdigit((int)*str))
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


static int parase_type_val(char *str, char *fmt, parse_data_t *at_data, int is_ESC, va_list *ap)
{
	int ret = XY_OK;
	int size = 0;
	char type;

	while (*fmt == ' ')
		fmt++;

	while (*str == ' ')
		str++;

	//如果fmt没有指定具体解析格式，标识无需解析当前这个参数
	if(strlen(fmt) == 0 || strlen(str) == 0)
	{
		return ret;
	}

	at_data->point_flag = 0;
	type = *(fmt + strlen(fmt) - 1);
	size = strtol(fmt + 1, NULL, 10);

	if (type == 'd' || type == 'D')
	{
		if (!is_digit_str(str) && !is_hex_str(str, 1))
		{
			ret = XY_ERR_PARAM_INVALID;
		}
		else if (size == 0 || size == 4)
			*((long *)(va_arg(*ap, int))) = (long)strtol(str,NULL,0);
		else if (size == 1)
			*((char *)(va_arg(*ap, int))) = (char)strtol(str,NULL,0);
		else if (size == 2)
			*((short *)(va_arg(*ap, int))) = (short)strtol(str,NULL,0);
		else
			xy_assert(0);
	}
	else if (type == 's' || type == 'S')
	{
		if (size == 0)
		{
			if(is_ESC == 1)
			{
				format_escape_char(str);
			}
			strcpy((char *)(va_arg(*ap, int)), str);
		}
		else
		{
			if(is_ESC == 1)
			{
				format_escape_char(str);
			}
			int cpy_len = ((int)(strlen(str))) < size ? ((int)(strlen(str))) : (size - 1);
			strncpy((char *)(va_arg(*ap, int)), str, cpy_len);
		}
	}
	else if ((type == 'p' || type == 'P'))
	{
		at_data->point_flag = 1;
		*((int *)(va_arg(*ap, int))) = (int)str;
	}
	else if(type == 'h' || type == 'H')
	{
		at_data->data_len =strlen(str);
		if(at_data->data_len % 2 != 0)
		{
			ret = XY_ERR_PARAM_INVALID;
		}
		else
		{
			if(hexstr2bytes(str, at_data->data_len, (char *)(va_arg(*ap, int)), at_data->data_len/2) == -1)
				ret = XY_ERR_PARAM_INVALID;
		}
	}

	return ret;
}

/*支持0X十六进制的整形解析，is_ESC参数未被使用*/
int parse_param(char *fmt_parm, char *buf, int is_ESC, va_list *vl)
{
	xy_assert(fmt_parm != NULL && buf != NULL);
	int  ret = XY_OK;
	char *param_comma = NULL;
	char *param_quotes_begin = NULL;
	char *param_quotes_end = NULL;
	char *fmt_comma = NULL;
	char *param_end = NULL;
	char *p_maohao = NULL;

	char *fmt = xy_malloc(strlen(fmt_parm) + 1);
	char *fmt_head = fmt;
	parse_data_t at_data = {0};

	//由于fmt_parm格式是RO的，为了能够方便处理，拷贝到RAM中
	strcpy(fmt, fmt_parm);

	//跳过URC的前缀，以:作为标识
	p_maohao = strchr(buf, ':');
	param_comma = strchr(buf, ',');
	if(p_maohao && ((p_maohao<param_comma) || param_comma==NULL))
		buf = p_maohao+1;

	while (*buf == ' ')
		buf++;

	param_end = strchr(buf, '\r');
	if (param_end != NULL)
		*param_end = '\0';

	while (*buf != '\0' || *fmt != '\0')
	{
		param_comma = strchr(buf, ',');
		fmt_comma = strchr(fmt, ',');
		param_quotes_begin = strchr(buf, '"');
		param_quotes_end = NULL;

		//如果参数带引号，则去掉引号
		if (param_quotes_begin && (param_quotes_begin < param_comma || param_comma == NULL))
		{
			if(is_ESC == 1)
			{
				param_quotes_end = find_next_double_quato(param_quotes_begin + 1);
			}
			else
			{
				param_quotes_end = strchr(param_quotes_begin + 1, '"');
			}

			//只有左引号，没有右引号
			if (param_quotes_end == NULL)
			{
				goto CONTINUE;
			}

			param_comma = strchr(param_quotes_end + 1, ',');
			buf = param_quotes_begin + 1;
			*param_quotes_end = '\0';
		}
		
CONTINUE:
		//中间参数的格式化正常解析
		if (param_comma && fmt_comma)
		{
			*param_comma = '\0';
			*fmt_comma = '\0';

			ret = parase_type_val(buf, fmt, &at_data, is_ESC, vl);

			*fmt_comma = ',';
			if(at_data.point_flag == 0)
			{
				*param_comma = ',';
				if (param_quotes_end)
					*param_quotes_end = '"';
			}
			buf = param_comma + 1;
			fmt = fmt_comma + 1;
		}
		//解到fmt格式最后一个参数，但URC后面还有参数；常见于指定解析中间某参数
		else if (param_comma)
		{
			*param_comma = '\0';
			ret = parase_type_val(buf, fmt, &at_data, is_ESC, vl);
			break;
		}
		//解到URC最后一个参数，但fmt格式还有待解参数；常见于URC中未携带可选参数情况，用户入参时需要赋初始值
		else if (fmt_comma)
		{
			*fmt_comma = '\0';
			ret = parase_type_val(buf, fmt, &at_data, is_ESC, vl);
			*fmt_comma = ',';
			break;
		}
		//URC和fmt都到了最后一个参数了
		else
		{
			ret = parase_type_val(buf, fmt, &at_data, is_ESC, vl);
			break;
		}
	}

//END:
	if(at_data.point_flag == 0)
	{
		if (param_end)
			*param_end = '\r';
		if (param_comma)
			*param_comma = ',';
		if (param_quotes_end)
			*param_quotes_end = '"';
	}
	xy_free(fmt_head);
	return ret;
}

int at_parse_param(char *fmt_parm, char *buf, ...)
{
	int ret = XY_OK;
	va_list vl;
	va_start(vl,buf);
	ret = parse_param(fmt_parm, buf, 0, &vl);
	va_end(vl);

	return ret;
}

int at_parse_param_esc(char *fmt_parm, char *buf, ...)
{
	int ret = XY_OK;
	va_list vl;
	va_start(vl,buf);
	ret = parse_param(fmt_parm, buf, 1, &vl);
	va_end(vl);
	return ret;
}


//获取AT命令错误码显示模式
__OPENCPU_FUNC uint8_t get_cmee_mode()
{
	return ((HWREGB(BAK_MEM_ATUART_STATE) & 0x0c) >> 2);
}

/* 检测是否为ERROR,0表示非ERROR结果码*/
int Get_AT_errno(char *str)
{
	int errno = 0;
	int n = strlen(str);
	char *temp = str;

	//ERROR:XXXX\r\n;减少无效匹配
	if(get_cmee_mode() != 2 && n > 15)
	{
		temp = str+n-15;
	}

	if((temp = strstr(temp,"ERROR")) != NULL)
	{
		errno = XY_ERR;
		if(*(temp + strlen("ERROR")) == ':' && get_cmee_mode() == 1 )
			at_parse_param("%d", temp, &errno);

	}
	return errno;
}

/* 检测是否OK/ERROR结果码*/
bool Is_OK_ERR_rsp(char *str)
{
	int n = strlen(str);
	char *temp = str;

	//+:XXXX\r\n\r\nOK\r\n;减少无效匹配
	if(n > 15)
	{
		temp = str+n-15;
	}

	if(strstr(temp,"OK\r\n") != NULL || strstr(temp,"ERROR") != NULL)
	{
		return true;

	}
	return false;
}


/* 检测是否为OK应答结果码;返回 1 表示匹配成功 */
__OPENCPU_FUNC bool Is_AT_Rsp_OK(char *str)
{
	int ret = 0;
	int n = strlen(str);
	char *temp = str;

	//+URC:XXXX\r\n\r\nOK\r\n;减少无效匹配
	if(n > 6)
	{
		temp = str+n-6;
	}

	if(strstr(temp,"OK\r\n") != NULL)
	{
		ret = 1;
	}

	return  ret;
}



/* "AB235E"---->AB235E(3 BYTES) */
int hexstr2bytes(char* src, int src_len, char* dst, int dst_size)
{
	int i;

	if (src ==  NULL || dst == NULL || src_len < 0 || dst_size < (src_len + 1) / 2) {
		xy_assert(0);
	}

	for (i = 0; i < src_len; i += 2) {
		if(*src >= 'a' && *src <='f')
			*dst = ((*src - 'a') + 10) << 4;
		else if (*src >= '0' && *src <= '9') {
			*dst = (*src - '0') << 4;
		} else if (*src >= 'A' && *src <= 'F') {
			*dst = ((*src - 'A') + 10) << 4;
		} else {
			return -1;
		}

		src++;
		if(*src >= 'a' && *src <= 'f')
			*dst |= ((*src - 'a') + 10);
		else if (*src >= '0' && *src <= '9') {
			*dst |= (*src - '0');
		} else if (*src >= 'A' && *src <= 'F') {
			*dst |= ((*src - 'A') + 10);
		} else {
			return -1;
		}

		src++;
		dst++;
	}

	return src_len / 2;
}

/*"0XA358"--->0XA358 */  
int hexstr2int(char *hex,int *value)  
{  
    uint32_t len;   
    uint32_t temp;  
    uint32_t val=0;
    uint32_t i=0;  

	if(strncmp(hex,"0x",2)==0 || strncmp(hex,"0X",2)==0)
		hex+=2;
	else
		return XY_ERR;
	
	len = strlen(hex); 
	while(!(hex[i]<'0' || (hex[i] > '9'&&hex[i] < 'A') || (hex[i] > 'F' && hex[i] < 'a') || hex[i] > 'f'))
		i++;
    if(i < len)
		return XY_ERR;

    for (i=0, temp=0; (i<len && i<8); i++)  
    {  
			if(isdigit((int)hex[i]))  
            	temp = (hex[i] - 48);

    		if(isalpha((int)hex[i]))  
            	temp =  (isupper((int)hex[i]) ? hex[i] - 55 : hex[i] - 87);
    		val = val*16+temp;
    }   
    *value = val;
	return  XY_OK;
}  

/* AB235E(3 BYTES)---->"AB235E" */
void bytes2hexstr(unsigned char* src, signed long src_len, char* dst, signed long dst_size)
{
	const char tab[] = "0123456789ABCDEF";
	signed long i;

	if (src ==  NULL || dst == NULL || src_len < 0 || dst_size <= src_len * 2) {
		xy_assert(0);
	}

	for (i = 0; i < src_len; i++) {
		*dst++ = tab[*src >> 4];
		*dst++ = tab[*src & 0x0f];
		src++;
	}

	*dst = '\0';
}
