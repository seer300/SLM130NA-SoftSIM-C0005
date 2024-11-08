/*
 * @file  system.h
 * @brief 一些基础系统级的API接口，若用户有平台级接口需求，可以由芯翼持续完善
 * @warning
 */

#pragma once
#include <stdint.h>
#include "runtime_dbg.h"
#include "sys_clk.h"
#include "nvic.h"

extern int g_dump_core;   // 触发死机的核，0为AP 1为CP
extern uint8_t g_nv_ioldo1_mode;   //ioldo1的电压模式
/***********************************宏定义部分*******************************************/

/*!< 弱函数前缀声明的宏定义*/
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/*!< 字节对齐前缀声明的宏定义*/
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

/*!< 函数强制不内联前缀声明的宏定义*/
#ifndef __NOINLINE
#define __NOINLINE __attribute__((noinline))
#endif

#define _ATTRIBURE_STR(x) _ATTRIBURE_VAL(x)
#define _ATTRIBURE_VAL(x) #x


/*!< 将函数放在RAM上的前缀声明的宏定义*/
#ifndef __RAM_FUNC
#define __RAM_FUNC __attribute__((section(".ramtext" "." __FILE__ "." _ATTRIBURE_STR(__LINE__))))
#endif

/*!< 将函数放在FLASH上的前缀声明的宏定义，中断服务程序中的函数不得放flash*/
#ifndef __FLASH_FUNC 
#define __FLASH_FUNC __attribute__((section(".flashtext" "." __FILE__ "." _ATTRIBURE_STR(__LINE__))))
#endif

/*禁止FLASH宏开启，容许中断函数运行在FLASH上，进而可将对BSP底层不依赖的函数放FLASH上运行，以减少OPENCPU的RAM开销。
当前圈定的代码主要为AT通道和命令处理，以及IPC核间通道消息处理。模组形态仍然是RAM上运行，无特别风险*/
// BAN_WRITE_FLASH=1
#ifndef __OPENCPU_FUNC 
#if BAN_WRITE_FLASH
#define __OPENCPU_FUNC  __FLASH_FUNC   
#else
#define __OPENCPU_FUNC  __RAM_FUNC
#endif
#endif

typedef void (*pFunType_void)(void);    //无入参的回调函数类型定义
typedef void (*pFunType_u8)(uint8_t);   //一个uint8_t入参的回调函数类型定义
typedef void (*pFunType_u32)(uint32_t); //一个uint32_t入参的回调函数类型定义

/**************************************常用的功能开关宏******************************************/
/**
 * @brief 配置内存保护单元（MPU）功能宏
 */
#define __MPU_PRESENT 1 /*!< 提供MPU功能 */

/**
 * @brief 用于软件断言，通常为软件判断处于非正常流程或状态时，需要断言。
 */
void sys_assert_proc(void);


// 软重启上电原因获取错误，故采用AON寄存器AONGPREG1的bit4作为软重启标志
#define AONGPREG1_SOFTRESET_Msk  0x10

typedef struct
{
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t R12;
	uint32_t SP;
	uint32_t LR;
	uint32_t PC;
	uint32_t xPSR;
	uint32_t MSP;
	uint32_t PSP;
	uint32_t PRIMASK;
	uint32_t BASEPRI;
	uint32_t FAULTMASK;
	uint32_t CONTROL;
} m3_common_reg_t;


typedef struct
{
	uint8_t MFSR;  /* Addr: 0xE000ED28, Memory Manage Fault Status Register */
	uint8_t BFSR;  /* Addr: 0xE000ED29, Bus Fault Status Register           */
	uint16_t UFSR; /* Addr: 0xE000ED2A, Usage Fault Status Register         */
	uint32_t HFSR; /* Addr: 0xE000ED2C, Hard Fault Status Register          */
	uint32_t DFSR; /* Addr: 0xE000ED30, Debug Fault Status Register         */
	uint32_t MMAR; /* Addr: 0xE000ED34, Memory Manage Address Register      */
	uint32_t BFAR; /* Addr: 0xE000ED38, Bus Fault Address Register          */
	uint32_t AFAR; /* Addr: 0xE000ED3C, Auxiliary Fault Address Register    */
} m3_fault_reg_t;

typedef struct
{
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t LR;
	uint32_t PC;
	uint32_t xPSR;
} m3_int_push_reg_t;

typedef struct {
    unsigned long addr;
    unsigned long value;
    unsigned long delay;
} Flash_OTP_Reg_def;

typedef struct {
    unsigned long otp_reg_num;

	#define OTP_REG_NUM                     0x08

    Flash_OTP_Reg_def reg_set[OTP_REG_NUM];

    unsigned long otp_reg_crc;
} Flash_OTP_Reg_Info_def;

typedef struct {
    unsigned long otp_info_valid;
    unsigned long secondary_boot_flag;
    unsigned long rsa_public_base_addr;		// 0x55AA55AA + (Length of RSA_N in bytes) + RSA_E + RSA_N + 0x55AA55AA + CRC
    unsigned long header_base_addr;			// Secondary Boot Prime (1 Sector) + Backup (1 Sector) + Image Header (1 Sector)
    unsigned long system_flag;
    unsigned long otp_info_crc;

    Flash_OTP_Reg_Info_def otp_reg_info;
} Flash_OTP_Info_Def;

enum OTP_BOOT_FLAG {
    BOOT_FLAG_BYPASS = 1,       // No Secondary Boot
    BOOT_FLAG_PLAIN,            // Keep Secondary Boot Plain Text
    BOOT_FLAG_ONLY_SHA,         // Only SHA Check
    BOOT_FLAG_SHA_RSA,			// SHA + RSA Check
    BOOT_FLAG_MAX,
};


/**
 * @brief flash进入低功耗模式
 * 
 */
void FlashEnterLPMode(void);

/**
 * @brief flash退出低功耗模式
 * 
 */
void FlashExitLPMode(void);

/**
 * @brief flash退出低功耗模式，fastrecovery中使用
 * 
 */
void FlashExitLPMode_NoDelay(void);

/**
 * @brief  main函数入口处调用该接口进行系统初始化
 */
void SystemInit(void);

/**
 * @brief  关闭AP WDT和UTC WDT
 */
void Disable_All_WDT(void);

/**
  * @brief 仅在深睡时sido不开启的场景下，切换aonclk
  * @return None
  */
extern void Switch_VDDAON_32K(void);
/**
  * @brief 睡眠失败/唤醒后将aon切换回HRC
  * @return None
  */
extern void Switch_VDDAON_HRC(void);