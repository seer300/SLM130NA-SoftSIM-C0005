#pragma once

#include <ctype.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "at_utils.h"
#include "cmsis_os2.h"
#include "factory_nv.h"
#include "hw_cache.h"
#include "sys_debug.h"
#include "xy_log.h"
#include "mem_adapt.h"
#include "xy_system.h"

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#ifndef UNUSED_ARG
#define UNUSED_ARG(x) 				(void)x
#endif

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
  * @brief 将16进制码流字符串，转换为码流，有效长度将减半
  * @param src		 16进制码流字符串，如"AB23"
  * @param src_len	 16进制码流字符串长度，应该为2*dst_size
  * @param dst		 转换后的二进制码流首地址，如0XAB23
  * @param dst_size	 转换后的二进制码流长度
  * @return -1表示16进展码流字符串不合法
  * @note  通常用于跨核的内存码流传递，例如"AB23"---->AB23(2 BYTES)
  */
int hexstr2bytes(char* src, int src_len, char* dst, int dst_size);

/**
  * @brief  将码流转换为16进制码流字符串，长度会翻倍
  * @param src		 二进制码流首地址，如0XAB23
  * @param src_len	 二进制码流长度
  * @param dst		 转换后的16进制码流字符串，如"AB23"
  * @param dst_size	 转换后的16进制码流字符串长度，为src_len*2
  * @return 转换后的长度
  * @note  通常用于跨核的内存码流传递，例如0XAB23(2 BYTES)---->"AB23" 
  */
int bytes2hexstr(unsigned char* src, signed long src_len, char* dst, signed long dst_size);

/**
 * @brief  将16进制ASCII整形字符串，转换为int型，例如“D800”输出结果为55296
 */
bool hexstr2int(char *hex, int *value);

/**
 * @brief  用于获取4字节的随机数种子，为硬件产生的真随机数
 * 获取随机数时，按照以下操作即可：
 * srand(xy_seed());	//初始化种子值，一般只需执行一次
 * int ret = rand();
 * @return  随机数种子
 */
uint32_t xy_seed(void);

/**
 * @brief 初始化种子值，一般只需执行一次
 * @param seed 随机数种子值
 */
void xy_srand(uint32_t seed);

/**
 * @brief  用于获取4字节的随机数
 * @return  随机数
 */
uint32_t xy_rand(void);

/**
 * @brief checksum计算，返回的是32位值
 * @param dataptr 待校验的数据首地址
 * @param len 待校验的数据长度
 * @return  32位结果
 * @note 
 */
uint32_t xy_chksum(const void *dataptr, int len);


/*供底层驱动执行微妙级别延时*/
void delay_us(uint32_t us);

/**
  * @brief  内存DMA拷贝的同步接口，若等到传输时间超过设定时间，则报超时错误
  * @param  DstAddress 	目标内存缓冲区地址
  * @param  SrcAddress 	源内存缓冲区地址
  * @param  DataLength 	待传输数据的长度
  * @param	Timeout		用户业务层接受的最大延迟，单位ms
  * @retval 成功返回0，其他错误返回1
  */
uint32_t DMA_Memcpy_SYNC(uint32_t DstAddress, uint32_t SrcAddress, uint32_t DataLength, uint32_t Timeout);


#define CLK32K_XTAL_FREQ        32768
#define CLK32K_RC_FREQ          32000

#define CLK32K_FREQ            (((AONPRCM->AONCLK_CTRL & 0x4) >> 2) == 1 ?  CLK32K_XTAL_FREQ : CLK32K_RC_FREQ)

/*
 *根据不同的32k频率，转换tick和ms
 */
extern volatile unsigned int g_freq_32k;

#define port_MS_TO_TICKS(xTimeInMs)             (((uint64_t)(xTimeInMs) * g_freq_32k + CLK32K_FREQ / 2) / CLK32K_FREQ)
#define port_TICKS_TO_MS(xTicks)                (((uint64_t)(xTicks) * CLK32K_FREQ + g_freq_32k / 2) / g_freq_32k)
	