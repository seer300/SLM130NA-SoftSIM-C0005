/*
 * Copyright (c) 2021 LinJiajun.
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

#include <stdio.h>
#include "at_uart.h"

#define PRINT_CHAR(ch)  AT_PRINT_CHAR(ch)

#if __CC_ARM

// 使用MDK编译器时，GCC库的printf实现
// 加入以下代码,支持printf函数,而不需要选择use MicroLIB
#pragma import(__use_no_semihosting)
//标准库需要的支持函数
struct __FILE
{
	int handle;
	/* Whatever you require here. If the only file you are using is */
	/* standard output using printf() for debugging, no file handling */
	/* is required. */
};
/* FILE is typedef in stdio.h. */
FILE __stdout;
// 定义_sys_exit()以避免使用半主机模式
void _sys_exit(int x)
{
	(void)x;
}
// printf implementation of the MDK library
int fputc(int ch, FILE *f)
{
	(void)f;

	PRINT_CHAR(ch);

	return ch;
}

int fputs(char const *__restrict s, FILE *__restrict fp)
{
    (void) fp;

    while(*s != '\0')
    {
    	PRINT_CHAR(*s);
        s++;
    }

    return 0;
}

#endif


#ifdef __GNUC__

int fputc(int ch, FILE *fp)
{
    (void) fp;

    PRINT_CHAR(ch);

    return 0;
}


int fputs(char const *__restrict s, FILE *__restrict fp)
{
    (void) fp;

    while(*s != '\0')
    {
    	PRINT_CHAR(*s);
        s++;
    }

    return 0;
}

// printf implementation of the GCC library
int _write (int fd, char *pBuffer, int size)
{
	int i=0;

	(void)fd;	// Prevents compiler warnings

	for (i = 0; i < size; i++)
	{
		PRINT_CHAR(pBuffer[i]);
	}

	return size;
}

#endif
