/*
 * Copyright (c) 2022 LinJiajun.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __SYS_DEBUG_H__
#define __SYS_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>

/*!< Global Declaration */
extern volatile char     *assert_file;
extern volatile int       assert_line;
extern volatile uint16_t  assert_primask;

/*!< Function Declaration */
void sys_assert_proc(char *file, int line);

// dump memory to file when assert
#define ASSERT_DUMP_MEMORY_ENABLE     1
// assert definition
#define Sys_Assert(x)     do{if(!(x)) sys_assert_proc(__FILE__, __LINE__);}while(0)
// print definition
#define Sys_Print(...)    printf(__VA_ARGS__)
//是否自动导出dump
#define DUMP_TO_FILE           1
#ifdef __cplusplus
}
#endif

#endif  /* end of __SYS_DEBUG_H__ */
