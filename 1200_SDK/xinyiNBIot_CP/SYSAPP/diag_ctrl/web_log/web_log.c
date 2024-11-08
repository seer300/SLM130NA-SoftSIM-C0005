#if WEBLOG
#include <stddef.h>
#include <stdarg.h>

#include "web_log.h"
#include "xy_flash.h"
#include "xy_memmap.h"
#include "xy_at_api.h"
#include "ipc_msg.h"
#include "xy_system.h"
#include "xy_rtc_api.h"
#include "diag_item_types.h"

#define WEB_LOG_THREAD_NAME           "web_log_thd" 
#define WEB_LOG_THREAD_STACK_SIZE     osStackShared
#define WEB_LOG_THREAD_PRIO           osPriorityNormal 

#define LOG_CTRL_INFO_LEN       (sizeof(log_cfg_t))                                          // CP侧远程log配置信息内容大小
#define LOG_CTRL_INFO_FLASH_LEN (0x1000)						                             // 远程log配置信息占据的flash空间总大小
#define AP_LOG_CTRL_FLASH_BASE  (OTA_FLASH_BASE())                                      // TODO: 暂定为FOTA备份区的起始地址
#define LOG_DATA_FLASH_BASE     (AP_LOG_CTRL_FLASH_BASE+LOG_CTRL_INFO_FLASH_LEN)             // 可用来保存远程log的flash起始地址，可设为FOTA备份区内的任意地址
#define LOG_DATA_FLASH_LEN_MAX  (DUMP_INFO_BASE_ADDR-LOG_DATA_FLASH_BASE)                    // 可用来保存远程log内容的最大flash空间大小，默认FOTA备份区为300K，固定前4K用来保存LOG配置信息
#define CP_LOG_CTRL_FLASH_BASE  (AP_LOG_CTRL_FLASH_BASE+LOG_CTRL_INFO_LEN)                   // 开启远程log后，为FOTA备份区起始地址, 保存第一条log的世界时间秒偏移量和对应的本地寄存器毫秒偏移量，为读取log时解析每条log的时间戳用
#define CP_LOG_BUFF_LEN_MAX     (0x1000)	                                                 // 暂存log的ram缓冲区大小
#define CP_LOG_BUFF_MARGIN      32

#define CP_POWRON_LOG_FLAG     (0xC5C5C5C5) // 上电第一条log头部标记             
#define CP_UT_LOG_FLAG		   (0xCFCFCFCF) // 携带世界时间的log头部标记，上电第一条log若未携带世界时间，若后续获取的世界时间，会补充一条携带世界时间的log
#define CP_MALLOC_LOG_FLAG	   (0xC0C0C0C0) // 静态log头部标记，静态log指malloc的log内容，其为固定内容；动态log为printf的明文格式的内容，""里的字符串保存在rodata区域

#define FLAGS_ZEROPAD          (1 << 0U)
#define FLAGS_LEFT             (1 << 1U)
#define FLAGS_PLUS             (1 << 2U)
#define FLAGS_SPACE            (1 << 3U)
#define FLAGS_HASH             (1 << 4U)
#define FLAGS_UPPERCASE        (1 << 5U)
#define FLAGS_CHAR             (1 << 6U)
#define FLAGS_SHORT            (1 << 7U)
#define FLAGS_LONG             (1 << 8U)
#define FLAGS_LONG_LONG        (1 << 9U)
#define FLAGS_PRECISION        (1 << 10U)
#define FLAGS_ADAPT_EXP        (1 << 11U)

#define is_digit(c) ((c) >= '0' && (c) <= '9')

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
// 核间log消息
typedef struct
{
	log_cmd_t ap_log_cmd;
	uint32_t ap_log_base_addr;
	uint32_t ap_log_size;
	uint32_t cp_log_base_addr;
	uint32_t cp_log_size;
} log_msg_t;

// 当前log状态
typedef enum
{
	NO_POWERON = 0,			//上电log还未保存，系统打印第一条log时保存上电log
	POWERON_NO_UT,			//已保存上电log，但还未获取到世界时间
	POWERON_AND_UT,			//已保存上电log，且已获取到世界时间
} log_state_t;

// 普通log头部信息
typedef struct
{
	uint32_t fmt_addr;
	uint32_t ms_tick;
}log_header_t;

// 上电log内容
typedef struct
{
	uint32_t log_flag;
	uint8_t poweron_reason;
	uint8_t UT_flag;
	uint8_t unused[2];
	uint64_t ms_tick;
}poweron_log_t;

typedef struct
{
    log_cfg_t cp_log_cfg;
    uint16_t  cp_log_buf_len;
	uint8_t   cp_log_cmd;
	uint8_t   unused;
} log_save_t;

static char *g_log_cache = NULL;   // log缓存地址，快溢出时保存至flash
static uint16_t g_log_buf_len = 0; //已缓存的log数据长度

osMutexId_t g_web_log_mutex = NULL;
osThreadId_t g_web_log_thread_id = NULL;
osMessageQueueId_t g_web_log_msg_q = NULL;

static void skip_digit(const char **s)
{
	while (is_digit(**s))
		(*s)++;
}

bool is_log_config_valid(log_cfg_t *log_cfg)
{
	if (log_cfg->web_log_enable == 1)
		return 1;
	else
		return 0;
}

void web_log_save(void)
{
	log_save_t *msg = NULL;

	while (1)
	{
		osMessageQueueGet(g_web_log_msg_q, (void *)(&msg), NULL, osWaitForever);

		osMutexAcquire(g_web_log_mutex, osWaitForever);
	
		log_cfg_t* cp_log = (log_cfg_t*)xy_malloc(LOG_CTRL_INFO_LEN);
		xy_Flash_Read(CP_LOG_CTRL_FLASH_BASE, (log_cfg_t*)cp_log, LOG_CTRL_INFO_LEN);
		// 保存CP侧log的起始地址，默认AT+LOGENABLE指令传的地址为AP侧访问的flash地址，此处需要转换成CP侧的访问地址
		uint32_t base_addr_temp = Address_Translation_AP_To_CP(cp_log->log_base_addr);

		// 清除原有LOG内容和重新配置LOG信息时，擦除原有LOG内容，并更新配置信息到flash中
		if (msg->cp_log_cmd == LOG_CLEAN || msg->cp_log_cmd == LOG_ENABLE)
		{
			if (cp_log->flash_size != 0 && base_addr_temp >= LOG_DATA_FLASH_BASE)
				xy_Flash_Erase(base_addr_temp, cp_log->flash_size);

			*cp_log = msg->cp_log_cfg;
		}
		else 
		{
			if (msg->cp_log_buf_len != 0)
			{
				// 此处配置信息不应该不合法
				if (cp_log->flash_size == 0 || cp_log->flash_size > LOG_DATA_FLASH_LEN_MAX || base_addr_temp < LOG_DATA_FLASH_BASE)
				{
					// 此条打印不进行保存
					user_printf("CP Web Log Info Error!!!\r\n");			
					xy_assert(0);
				}				
				// flash剩余空间不足，写满剩余空间，多余的log buff从flash头部开始循环写入
				if ((cp_log->save_offset + msg->cp_log_buf_len) > cp_log->flash_size)
				{
					xy_Flash_Write(base_addr_temp + cp_log->save_offset, g_log_cache, cp_log->flash_size - cp_log->save_offset);
					xy_Flash_Write(base_addr_temp, g_log_cache + cp_log->flash_size - cp_log->save_offset, (cp_log->save_offset + msg->cp_log_buf_len - cp_log->flash_size));
					cp_log->save_offset = cp_log->save_offset + msg->cp_log_buf_len - cp_log->flash_size;
					cp_log->flash_rewrite_num++;
				}
				else
				{
					xy_Flash_Write(base_addr_temp + cp_log->save_offset, g_log_cache, msg->cp_log_buf_len);
					cp_log->save_offset += msg->cp_log_buf_len;
				}
			}

			if (msg->cp_log_cmd == LOG_DISABLE)
				cp_log->web_log_enable = 0;
		}
			
		// 更新配置信息，主要是已保存的log长度和flash覆盖写入次数
		xy_Flash_Write(CP_LOG_CTRL_FLASH_BASE, cp_log, LOG_CTRL_INFO_LEN);

		// 此条打印不进行保存
		user_printf("CP Web Log Save Info:%d %d %d %d\r\n", base_addr_temp, cp_log->flash_size, msg->cp_log_buf_len, cp_log->save_offset);		

		// 关闭开关时，释放WEB LOG所占资源
		if (!is_log_config_valid(cp_log))
		{
			xy_free(cp_log);
			xy_free(msg);

			// 此条打印不进行保存	
			user_printf("CP Web Log Close \r\n");

			if (g_log_cache != NULL)
			{
				xy_free(g_log_cache);
				g_log_cache = NULL;
			}	

			osMutexDelete(g_web_log_mutex);
			g_web_log_mutex = NULL;
			osMessageQueueDelete(g_web_log_msg_q);
			g_web_log_msg_q = NULL;			
			g_web_log_thread_id = NULL;
			osThreadExit();
		}

		osMutexRelease(g_web_log_mutex);
		xy_free(cp_log);
		xy_free(msg);	
	}
}

void web_log_task(void)
{
	if (g_log_cache == NULL)
	{
		g_log_cache = xy_malloc(CP_LOG_BUFF_LEN_MAX);

		g_web_log_mutex = osMutexNew(NULL);
		g_web_log_msg_q = osMessageQueueNew(20, sizeof(void *), NULL);

		osThreadAttr_t task_attr = {0};
		task_attr.name = WEB_LOG_THREAD_NAME;
		task_attr.priority = WEB_LOG_THREAD_PRIO;
		task_attr.stack_size = WEB_LOG_THREAD_STACK_SIZE;
		user_printf("CP Web Log task:%d,%d,%d,%d\r\n", g_log_cache, g_web_log_msg_q, g_web_log_mutex, g_web_log_thread_id);
		g_web_log_thread_id = osThreadNew((osThreadFunc_t)(web_log_save), NULL, &task_attr);
	}		
}

// 返回1表示log缓冲区已经在使用并且web_log开关已经打开
bool web_log_config_init(void)
{
	static uint8_t have_init = 0;

	if (!have_init)
	{
		have_init = 1;

		log_cfg_t* cp_log = (log_cfg_t*)xy_malloc(LOG_CTRL_INFO_LEN);
		xy_Flash_Read(CP_LOG_CTRL_FLASH_BASE, (log_cfg_t*)cp_log, LOG_CTRL_INFO_LEN);

		if (is_log_config_valid(cp_log))
			web_log_task();
		xy_free(cp_log);
	}

	if (g_log_cache == NULL)
		return 0;
	else
		return 1;
}

int save_log_into_buff(void *data, uint32_t size)
{
	// 剩余空间若足够则进行保存，若不够，直接丢弃，并触发写FLASH
	if (g_log_buf_len + size <= CP_LOG_BUFF_LEN_MAX)
	{
		memcpy(g_log_cache + g_log_buf_len, data, size);
		g_log_buf_len += size;
		return 1;
	}
	else
		return -1;	
}

bool is_flash_rodata(uint32_t val)
{
	extern uint32_t _flash_rodata_start;
	extern uint32_t _flash_rodata_end;

	if (val >= (uint32_t)&_flash_rodata_start && val < (uint32_t)&_flash_rodata_end)
		return 1;

	return 0;
}

bool is_ram_rodata(uint32_t val)
{
	extern uint32_t _ram_rodata_start;
	extern uint32_t _ram_rodata_end;

	if (val >= (uint32_t)&_ram_rodata_start && val < (uint32_t)&_ram_rodata_end)
		return 1;

	return 0;
}

bool is_rodata(uint32_t val)
{
	if(is_flash_rodata(val) || is_ram_rodata(val))
		return 1;
	else
		return 0;
}

int log_vsprintf(const char *format, va_list va)
{
	char ch;
	char *s;
	int w;
	long l;
	double f;
	long long ll;

	int ret = 0;
  	unsigned int flags, n;

	while (*format)
	{
		if (*format != '%')
		{
			format++;
			continue;
		}

		format++;
		// evaluate flags
		flags = 0U;
		do 
		{
			switch (*format) 
			{
				case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
				case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
				case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
				case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
				case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
				default :                                   n = 0U; break;
			}
		} while (n);

		// evaluate width field
		if (is_digit(*format))
			skip_digit(&format);
		else if (*format == '*') 
		{
			w = va_arg(va, int);
			if (w < 0)
				flags |= FLAGS_LEFT;    // reverse padding
			format++;
			ret = save_log_into_buff(&w, sizeof(int));
		}

		// evaluate precision field
		if (*format == '.') 
		{
			flags |= FLAGS_PRECISION;
			format++;
			if (is_digit(*format))
				skip_digit(&format);
			else if (*format == '*') 
			{
				w = va_arg(va, int);
				format++;
				ret = save_log_into_buff(&w, sizeof(int));
			}
		}

		// evaluate length field
		switch (*format) 
		{
			case 'l' :
				flags |= FLAGS_LONG;
				format++;
				if (*format == 'l') 
				{
					flags |= FLAGS_LONG_LONG;
					format++;
				}
				break;
			case 'h' :
				flags |= FLAGS_SHORT;
				format++;
				if (*format == 'h') 
				{
					flags |= FLAGS_CHAR;
					format++;
				}
				break;
			case 't' :
				flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			case 'j' :
				flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			case 'z' :
				flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			default :
				break;
		}

		// evaluate specifier
		switch (*format)
		{
			case 'd' :
			case 'i' :
			case 'u' :
			case 'x' :
			case 'X' :
			case 'o' :
			case 'b' : 
				if (*format == 'd' || *format == 'i' || *format == 'u') 
					flags &= ~FLAGS_HASH;   // no hash for dec format

				// uppercase
				if (*format == 'X')
					flags |= FLAGS_UPPERCASE;

				// no plus or space flag for u, x, X, o, b
				if ((*format != 'i') && (*format != 'd'))
					flags &= ~(FLAGS_PLUS | FLAGS_SPACE);

				// ignore '0' flag when precision is given
				if (flags & FLAGS_PRECISION)
					flags &= ~FLAGS_ZEROPAD;

				// convert the integer
				if ((*format == 'i') || (*format == 'd')) 
				{
					// signed
					if (flags & FLAGS_LONG_LONG) 
					{
						ll = va_arg(va, long long);
						ret = save_log_into_buff(&ll, sizeof(long long));
					}
					else if (flags & FLAGS_LONG) 
					{
						l = va_arg(va, long);
						ret = save_log_into_buff(&l, sizeof(long));
					}
					else 
					{
						w = ( (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int) );
						ret = save_log_into_buff(&w, sizeof(int));
					}
				}
				else 
				{
					// unsigned
					if (flags & FLAGS_LONG_LONG) 
					{
						unsigned long long ull = va_arg(va, unsigned long long);
						ret = save_log_into_buff(&ull, sizeof(unsigned long long));
					}
					else if (flags & FLAGS_LONG) 
					{
						l = va_arg(va, long);
						ret = save_log_into_buff(&l, sizeof(long));
					}
					else 
					{
						w = ( (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int) );
						ret = save_log_into_buff(&w, sizeof(int));
					}
				}
				format++;
				break;
			case 'f' :
			case 'F' : 
				if (*format == 'F') 
					flags |= FLAGS_UPPERCASE;
				f = va_arg(va, double);
				ret = save_log_into_buff(&f, sizeof(double));
				format++;
				break;
			case 'e' :
			case 'E' :
			case 'g' :
			case 'G' :
				if ((*format == 'g')||(*format == 'G')) 
					flags |= FLAGS_ADAPT_EXP;
				if ((*format == 'E')||(*format == 'G')) 
					flags |= FLAGS_UPPERCASE;
				f = va_arg(va, double);
				ret = save_log_into_buff(&f, sizeof(double));
				format++;
				break;
			case 'c' :
				// char output
				ch = (char) va_arg(va, int);
				ret = save_log_into_buff(&ch, sizeof(char));
				format++;
				break;
			case 's' :
				s = va_arg(va, char*);
				if (!s)
					s = "<NULL>";
				ret = save_log_into_buff(s, strlen(s) + 1);
				format++;
				break;
			case 'p' :
				flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
				const bool is_ll = (sizeof(uintptr_t) == sizeof(long long));
				if (is_ll) 
				{
					ll = (uintptr_t)va_arg(va, void*);
					ret = save_log_into_buff(&ll, sizeof(long long));
				}
				else 
				{
					l = (uintptr_t)va_arg(va, void*);
					ret = save_log_into_buff(&l, sizeof(long));
				}
				format++;
				break;
			case '%' :
				format++;
				break;
			default :
				format++;
				break;
		}
		if (ret == -1)
			return ret;
	}
	return ret;
}

int log_printf_save_buff(int dyn_id, int src_id, const char *fmt, ...)
{
	if ((XYAPP != src_id && PLATFORM != src_id) || !web_log_config_init())
		return -1;
	
	int ret = 0;
	uint16_t fmt_len = 0;
	uint16_t offset_before;
	log_header_t *log_head = NULL;
	RTC_TimeTypeDef rtc_time = {0};
	uint8_t log_buff_full_flag = 0;
	poweron_log_t *power_log = NULL;
	static log_state_t log_state = NO_POWERON;
	log_save_t *msg = NULL;
	
	// 有可能线程调度器还未开启，比如在main函数调用时
	xy_mutex_acquire(g_web_log_mutex, osWaitForever);

	if(log_state == NO_POWERON)
	{
		xy_assert(g_log_buf_len == 0);
		power_log = xy_malloc(sizeof(poweron_log_t));
		power_log->log_flag = CP_POWRON_LOG_FLAG;
		power_log->poweron_reason = Get_Boot_Reason();
		power_log->UT_flag = Get_Current_UT_Time(&rtc_time.wall_clock);
		power_log->ms_tick = osKernelGetTickCount();
		save_log_into_buff(power_log, sizeof(poweron_log_t));

		if (power_log->UT_flag)
		{
			uint32_t poweron_ut_offset = xy_mktime(&rtc_time) / 1000;
			save_log_into_buff(&poweron_ut_offset, sizeof(uint32_t));
			log_state = POWERON_AND_UT;
		}
		else
		{
			log_state = POWERON_NO_UT;
		}
		xy_free(power_log);
		goto write_log;
	}
	else if(log_state == POWERON_NO_UT)
	{
		if (Get_Current_UT_Time(&rtc_time.wall_clock))
		{
			log_header_t *UT_log = xy_malloc(sizeof(log_header_t));
			UT_log->fmt_addr = CP_UT_LOG_FLAG;
			UT_log->ms_tick = xy_mktime(&rtc_time) / 1000;
			save_log_into_buff(UT_log, sizeof(log_header_t));
			uint64_t ut_ms_tick = osKernelGetTickCount();
			save_log_into_buff(&ut_ms_tick, sizeof(uint64_t));

			xy_free(UT_log);
			log_state = POWERON_AND_UT;
		}
		goto write_log;
	}
	else
	{
write_log:
		offset_before = g_log_buf_len;
		// 先跳过log head，待获取参数长度确认buff空间足够时再保存
		log_head = xy_malloc(sizeof(log_header_t));
		g_log_buf_len += sizeof(log_header_t);

		if (is_rodata((uint32_t)fmt))
			log_head->fmt_addr = (uint32_t)dyn_id;
		else
		{
			log_head->fmt_addr = CP_MALLOC_LOG_FLAG;
			fmt_len = strlen(fmt) + 1;
			g_log_buf_len += fmt_len;
		}
		log_head->ms_tick = (uint32_t)osKernelGetTickCount();
	}

	va_list args;
	va_start(args, fmt);
	ret = log_vsprintf(fmt, args);
	va_end(args);

	// 解析成功
	if(ret == 1)	
	{
		// 确认buff空间充足时，将log head保存至buff预留空间
		memcpy(g_log_cache + offset_before, log_head, sizeof(log_header_t));
		if (fmt_len)
			memcpy(g_log_cache + offset_before + sizeof(log_header_t), fmt, fmt_len);
		// 本次log保存到buff后，若buff剩余空间小于32字节，则触发写flash
		if (g_log_buf_len + CP_LOG_BUFF_MARGIN > CP_LOG_BUFF_LEN_MAX)
			log_buff_full_flag = 1;
	}
	else
	{
		// 解析失败，log_buf_len长度不变
		g_log_buf_len = offset_before;
		if (ret == -1)
			log_buff_full_flag = 1;
	}
	
	xy_free(log_head);
	
	if (log_buff_full_flag)
	{
		log_buff_full_flag = 0;
		msg = (log_save_t *)xy_malloc(sizeof(log_save_t));
		msg->cp_log_buf_len = g_log_buf_len;
		msg->cp_log_cmd = LOG_END;
		g_log_buf_len = 0;
		// 此处线程调度器一定已经打开
		xy_mutex_release(g_web_log_mutex);

		// flash相关操作交给WEB LOG线程处理，避免耗时过长，导致AT回复不及时
		if (osMessageQueuePut(g_web_log_msg_q, &msg, 0, osNoWait) != osOK)
			ret = 0;
	}
	else
		xy_mutex_release(g_web_log_mutex);

	return ret;
}

// 配置WEB LOG功能，flash相关操作交给WEB LOG线程处理，避免耗时过长，导致AT回复不及时
bool web_log_enable(log_msg_t *log_msg)
{
	int ret = 1;

	log_save_t *msg = (log_save_t *)xy_malloc(sizeof(log_save_t));
	memset(msg, 0, sizeof(log_save_t));

	// 保存CP侧log的起始地址和空间大小都不为0时，开启WEB LOG功能
	if (log_msg->cp_log_base_addr != 0 && log_msg->cp_log_size != 0)
	{
		// 返回1表示log缓冲区已经在使用并且web_log开关已经打开
		if (!web_log_config_init())
			web_log_task();

		msg->cp_log_cmd = LOG_ENABLE;
		msg->cp_log_cfg.web_log_enable = 1;
		msg->cp_log_cfg.log_buff_size = CP_LOG_BUFF_LEN_MAX;
		// 默认AT指令传的地址都属于AP侧的flash地址
		msg->cp_log_cfg.log_base_addr = log_msg->cp_log_base_addr;
		msg->cp_log_cfg.flash_size = log_msg->cp_log_size;
		
		// 此条打印不进行保存
		user_printf("CP Web Log Enable cmd recv:%d %d\r\n", msg->cp_log_cfg.log_base_addr, msg->cp_log_cfg.flash_size);

		if (osMessageQueuePut(g_web_log_msg_q, &msg, 0, osNoWait) != osOK)
			ret = 0;
	}
	else if (g_log_cache != NULL)
	{
		// 都为0时，清除CP侧所有log内容和控制信息
		if (log_msg->cp_log_base_addr == 0 && log_msg->cp_log_size == 0)
		{
			// 此条打印不进行保存
			user_printf("CP Web Log Clean cmd recv\r\n");

			msg->cp_log_cmd = LOG_CLEAN;
		}
		// 其中之一为0时，只关闭开关，暂停保存之后的所有log，并把ram缓冲区上的log写到flash中
		else
		{
			// 此条打印不进行保存
			user_printf("CP Web Log Disable cmd recv\r\n");		

			msg->cp_log_cmd = LOG_DISABLE;
			msg->cp_log_buf_len = g_log_buf_len;
		}
		g_log_buf_len = 0;

		if (osMessageQueuePut(g_web_log_msg_q, &msg, 0, osNoWait) != osOK)
			ret = 0;	
	}

	return ret;
}

bool web_log_config(int log_read_mode, log_cfg_t *log_config)
{
	if (log_read_mode == LOG_READ_CP)
		xy_Flash_Read(CP_LOG_CTRL_FLASH_BASE, log_config, LOG_CTRL_INFO_LEN);
	else if (log_read_mode == LOG_READ_AP)
		xy_Flash_Read(AP_LOG_CTRL_FLASH_BASE, log_config, LOG_CTRL_INFO_LEN);
	else
		return 0;

	return 1;
}

bool web_log_read(uint32_t base_addr, int size, void *buff)
{
	if (buff != NULL)
	{
		xy_Flash_Read(base_addr, buff, size);
		// 此条打印不进行保存
		user_printf("CP Web Log Read:%d %d %d\r\n", base_addr, size);
		return 1;
	}
	else
	{
		// TODO：暂未确定
		return 0;
	}
}

/*******************************************************************************
 *                       Local function implementations	                       *
 ******************************************************************************/

// 云业务收到下行log指令后开启或关闭相应功能
/* AT+LOGENABLE=<ap_log_base_addr>,<ap_log_size>[,<cp_log_base_addr>][,<cp_log_size>] */
int at_LOGENABLE_req(char *at_buf, char **prsp_cmd)
{
	if (g_req_type == AT_CMD_REQ)
	{
		log_msg_t log_msg = {0};

		if(at_parse_param("%d(0-),%d(0-),%d[0-],%d[0-]", at_buf, &(log_msg.ap_log_base_addr), &(log_msg.ap_log_size), &(log_msg.cp_log_base_addr), &(log_msg.cp_log_size)) != AT_OK)
			return ATERR_PARAM_INVALID;

		if (log_msg.ap_log_base_addr == 0 || log_msg.ap_log_size == 0)
			log_msg.ap_log_cmd = LOG_DISABLE;
		else
			log_msg.ap_log_cmd = LOG_ENABLE;

		// 发送核间消息通知AP开启或关闭log保存模式
		shm_msg_write(&log_msg, sizeof(log_msg_t), ICM_WEB_LOG);
		
		if (!web_log_enable(&log_msg))
			return ATERR_NOT_ALLOWED;
	}
	else if (g_req_type == AT_CMD_TEST)
	{
		*prsp_cmd = xy_malloc(50);
		snprintf(*prsp_cmd, 50, "\r\n+LOGENABLE:%u %ld\r\n\r\nOK\r\n", Address_Translation_CP_To_AP(LOG_DATA_FLASH_BASE), LOG_DATA_FLASH_LEN_MAX);
	}

	return AT_END;
}

// TODO: 此AT命令实现待讨论
int at_LOGREAD_req(char *at_buf, char **prsp_cmd)
{
	int log_size = 0;
	uint32_t log_base_addr = 0;

	if(at_parse_param("%d,%d", at_buf, &log_base_addr, &log_size) != AT_OK)
		return ATERR_PARAM_INVALID;

	web_log_read(log_base_addr, log_size, NULL);

	return AT_END;
}
#endif
