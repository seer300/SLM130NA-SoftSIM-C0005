/***********************************************************************************
* @Copyright (c)	:(C)2013, Qindao ieslab Co., Ltd
* @FileName             :type.h
* @Author       	:wf200 team
* @Version      	:V1.0
* @Date         	:2013-11-29
* @Description	:Data type definition
************************************************************************************/
#ifndef  TYPE_H
#define  TYPE_H
#ifdef   __cplusplus
extern "C" 
{
#endif   /* #ifdef __cplusplus */
/*---------------------------- Variable Define -------------------------------------*/

#pragma pack(1) //1字节对齐

typedef signed int         s32;
typedef	signed short int   s16;
typedef signed char  s8;

typedef signed int const   sc32;  /* Read Only */
typedef signed short const sc16;  /* Read Only */
typedef signed char  const sc8;   /* Read Only */

typedef volatile signed long  vs32;
typedef volatile signed short vs16;
typedef volatile signed char  vs8;

typedef unsigned int          u32;
typedef	unsigned short int    u16;
typedef unsigned char  u8, *PU8;;

typedef unsigned char  u8; 
typedef unsigned char  bcd;

typedef unsigned int const    uc32;	/* Read Only */
typedef unsigned short const uc16;	/* Read Only */
typedef unsigned char  const uc8;	/* Read Only */

typedef volatile unsigned int	  vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char	vu8;

typedef unsigned long long u64;
typedef long long s64;

typedef unsigned char    BIT;
typedef unsigned char    BOOL;

typedef void    VOID;
typedef void*    PVOID;

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)

#define SUCCESS_IES		0x01
#define ERROR_IES		0x00

#define JC_ENABLE			0x01
#define JC_DISABLE			0x00

//GPIO Bit SET and Bit RESET enumeration
 typedef enum
{
    JC_GPIO_PIN_RESET  = 0,
    JC_GPIO_PIN_SET    = 1,
 }JC_GPIO_PinState;


#define TASK_IDLE	1		//任务机处于空闲状态
#define TASK_BUSY	0		//任务机处于忙碌状态
#define TASK_CONTINUE  2 //任务机本步骤完成需要继续
#define TASK_COMPLETE  3 //任务机完成

#define BIT0		0x0001	//一个字节中取bit0 
#define BIT1		0x0002	//一个字节中取bit1 
#define BIT2		0x0004	//一个字节中取bit2 
#define BIT3		0x0008	//一个字节中取bit3 
#define BIT4		0x0010	//一个字节中取bit4 
#define BIT5		0x0020	//一个字节中取bit5 
#define BIT6		0x0040	//一个字节中取bit6 
#define BIT7		0x0080	//一个字节中取bit7
#define BIT8		0x0100	//一个字节中取bit0 
#define BIT9		0x0200	//一个字节中取bit1 
#define BIT10		0x0400	//一个字节中取bit2 
#define BIT11		0x0800	//一个字节中取bit3 
#define BIT12		0x1000	//一个字节中取bit4 
#define BIT13		0x2000	//一个字节中取bit5 
#define BIT14		0x4000	//一个字节中取bit6 
#define BIT15		0x8000	//一个字节中取bit7

typedef union {
	u16	to_uint16;
	s16	to_sint16;
	u8	to_uint8[2];
	s8	to_sint8[2];
} mtype16_;

#define BYTE_L 0
#define BYTE_H 1
typedef union {
	float	to_float;
	u32	to_uint32;
	s32	to_sint32;
	u16	to_uint16[2];
	s16	to_sint16[2];
	u8	to_uint8[4];
	s8	to_sint8[4];
} mtype32_;	
#define WORD_L 0
#define WORD_M 1
#define WORD_H 2
#define WORD_P 3

typedef union 
{
	struct 
	{
			u8 data0;
			u8 data1;
			u8 data2;
			u8 data3;
	} datas_map;
	u8 data_buf[4];
	u32 datas;
} UNION_4U8TOU32;

typedef union 
{
	struct 
	{
    u8 data0;
    u8 data1;
	} datas_map;
	u8 data_buf[2];
	u16 datas;
} UNION_2U8TOU16;
/*---------------------------- Function Declare -----------------------------------*/


#ifdef __cplusplus
	}
#endif

#endif	//#ifndef  __TYPE_H


