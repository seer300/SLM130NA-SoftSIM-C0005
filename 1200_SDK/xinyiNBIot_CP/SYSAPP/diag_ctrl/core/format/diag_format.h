#ifndef DIAG_FORMAT_H
#define DIAG_FORMAT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdarg.h>

// 普通动态log使用的参数长度格式化，不写入buffer，返回需要保存的参数长度
int diag_format_get_arguments_length(const char* format, va_list va);

// 普通动态log使用的参数格式化，该函数把所有变参传入写入buffer
int diag_format_arguments_to_buffer(char* buffer, const size_t maxlen, const char* format, va_list va);

// 物理层动态log使用的参数格式化，该函数把指定个数的参数从变参中取出放入buffer
void diag_format_fixed_args(void* buffer, const size_t arg_num, va_list va);


#ifdef __cplusplus
 }
#endif

#endif  /* DIAG_FORMAT_H */
