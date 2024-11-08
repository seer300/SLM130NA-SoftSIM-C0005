#pragma once

/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "xy_utils.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define DIFF_VALUE ('a'-'A')

/**
* @brief  AT错误码构建宏定义，内部组装为“+CME ERROR: XXX”
* @param  a 错误码，参见@ref AT_ERRNO_E
* @return  返回字符串堆指针，由芯翼平台内部负责释放空间
* @warning 不建议客户使用！
*/
#define AT_ERR_BUILD(a) at_err_build_info(a, __FILE__, __LINE__)

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct
{
	char ch;          //转义字符 如\a 中的a
	char esc_ch;      //转义字符对应的ASCII码
} esc_item_t;



/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
bool is_digit_str(char *str);

bool is_hex_str(char *str, uint8_t check_head);


/*识别source是否为substr对应的AT命令字符串，其中substr必须为字母串，不得添加头尾标识，如“+”/“=”/“？”等，如substr=“NRB”      、“AT”*/
char *at_prefix_strstr(char *source, char * substr);

char *at_strnchr(char *s, int c, int n);

/**
 * @brief  内部使用！不区分大小写的子父字符串比较，其中n表示仅匹配父字符串前n字节。常用于字符串头部的匹配
 */
bool at_strncasecmp(const char *s1, const char *s2, int n);

/**
 * @brief  获取错误显示模式
 */
uint8_t get_cmee_mode(void);

/* 检测是否为OK应答结果码;返回 1 表示匹配成功 */
bool Is_AT_Rsp_OK(char *str);

/*  识别是否为“ERROR”应答报文，并解析出对应的错误码；返回0表示非ERROR结果码 */
int Get_AT_errno(char *str);

/**
 * @brief  识别字符串是否含应答结果，即是否含“OK”或“ERROR”
 * @param  str 待处理的at数据
 * @note
 */
bool Is_Result_AT_str(char *str);

/* 构建“OK”应答字符串，使用者需释放内存空间 */
char *at_ok_build();

/**
 * @brief 解析字符串中的转义字符，并转换为对应的ASCII码
 * @param input [IN]/[OUT]
 * @note
 */
void format_escape_char(char *input);


/**
 * @brief 获取字符串中下一个双引号位置，排除\"转义字符干扰
 * @param data [IN] 输入字符串
 * @return 返回下一个双引号位置，若未找到，返回NULL
 * @note
 */
char *find_next_double_quato(char *data);

/**
 * @brief AT错误码构建函数，客户禁用，内部调试用
 * @param err_no [IN] 错误码，参见AT_XY_ERR
 * @param file [IN] 调用函数所在文件名
 * @param line [IN] 调用函数所在行
 * @return 返回字符串堆指针
 * @attention  客户禁用！
 */
char *at_err_build_info(int err_no, char *file, int line);


/**
 * @brief 拦截外部MCU不感兴趣的URC，以防止URC风暴对外部MCU流程和内存产生干扰
 * @param   str  [IN] 输入URC字符串
 * @return 1表示匹配成功，不发送给外部MCU
 * @note
 */
int urc_filter(char* str);


