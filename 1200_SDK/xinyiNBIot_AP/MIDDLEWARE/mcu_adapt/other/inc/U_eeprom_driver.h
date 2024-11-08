/***********************************************************************************
 * @Copyright (c)	:(C)2020, Qindao ieslab Co., Ltd
 * @FileName   	    :new_gsm_sys_processor.h
 * @Author       	:Intelligent gas team
 * @Version      	:V1.0
 * @Date         	:2020-7-1
 * @Description	:Head file definition of system processor
 ************************************************************************************/
 #ifndef __U_EEP_DRIVER_H
#define __U_EEP_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif
//
//#include "sys_config.h"
//#include "ddl.h"
#include <stdint.h>
#include "type.h"

/*------------------------------- Variable Define -----------------------------------*/
//EEPROM
#define READ_LIMITSIZE_MAX                  512                //允许读取的最大数据长度
#define AT24C256                            1
	
#ifdef AT24C32                                                //条件编译
#define EEPageSize                          32                //EEP页大小
#define WRITE_LIMITSIZE_MAX                 80                //允许写入的最大数据长度
#define EE_ADDR_MAX                         0x0FFF
#elif AT24C64
#define EEPageSize                          32
#define WRITE_LIMITSIZE_MAX                 255
#define EE_ADDR_MAX                         0x1FFF
#elif AT24C128
#define EEPageSize                          64
#define WRITE_LIMITSIZE_MAX                 240
#define EE_ADDR_MAX                         0x3FFF
#elif AT24C256
#define EEPageSize                          64
#define WRITE_LIMITSIZE_MAX                 512
#define EE_ADDR_MAX                         0x7FFF
#endif


//配置引脚相关宏
#if (HARDWARE_VERSION==0x14)//方壳表
#define EE_POWER_GPIOx                   GpioPortE
#define EE_POWER_PIN                     GpioPin3


#elif (HARDWARE_VERSION==0x18)//圆壳表
#define EE_POWER_GPIOx                   GpioPortB
#define EE_POWER_PIN                     GpioPin7
#endif
//#define POWER_EE_DOWN                    Gpio_WriteOutputIO(EE_POWER_GPIOx, EE_POWER_PIN,GPIO_PIN_RESET)
//#define POWER_EE_UP                      Gpio_WriteOutputIO(EE_POWER_GPIOx, EE_POWER_PIN,GPIO_PIN_SET)
//
//#define EE_SCL_GPIOx                     GpioPortB
//#define EE_SCL_PIN                       GpioPin8
//#define EE_SCL_AFSEL                     GpioAf1
//
//#define EE_SDA_GPIOx                     GpioPortB
//#define EE_SDA_PIN                       GpioPin9
//#define EE_SDA_AFSEL                     GpioAf1


#define I2C_DATA_ADDR_SIZE  (2)     //i2c eeprom片内数据地址宽度，1或2，如：AT24C02该宏为1，BL24C256A、AT24C64该宏为2


#define EE_POWER_PIN       (MCU_GPIO5)
#define EE_SDA_PIN         (MCU_GPIO11)
#define EE_SCL_PIN         (MCU_GPIO9)

#define EE_USE_IIC         ( 0 )

//I2C选择、操作相关
//#define i2c0 1
//#define i2c1 0
//#define i2c2 0
//#define i2c3 0
//
//#if  i2c0
//#define I2CIE_CLOSE                         SetBit((uint32_t)&M0P_I2C0->CR, I2cModule_En, FALSE) //关闭I2C//M0P_I2C0->CR_f.ENS = 0
//#define I2CIE_OPEN                          SetBit((uint32_t)&M0P_I2C0->CR, I2cModule_En, TRUE)  //开启I2C
//#define MCU_I2C_CLK_ENABLE()                SetBit((uint32_t)(&(M0P_SYSCTRL->PERI_CLKEN0)), SysctrlPeripheralI2c0, TRUE); //开启I2C1时钟
//#define MCU_I2C_CLK_DISABLE()               SetBit((uint32_t)(&(M0P_SYSCTRL->PERI_CLKEN0)), SysctrlPeripheralI2c0, FALSE);//关闭I2C1时钟
//#elif		i2c1
//#define I2CIE_CLOSE                         SetBit((uint32_t)&M0P_I2C1->CR, I2cModule_En, FALSE) //关闭I2C//M0P_I2C0->CR_f.ENS = 0
//#define I2CIE_OPEN                          SetBit((uint32_t)&M0P_I2C1->CR, I2cModule_En, TRUE)  //开启I2C
//#define MCU_I2C_CLK_ENABLE()                SetBit((uint32_t)(&(M0P_SYSCTRL->PERI_CLKEN0)), SysctrlPeripheralI2c1, TRUE); //开启I2C1时钟
//#define MCU_I2C_CLK_DISABLE()               SetBit((uint32_t)(&(M0P_SYSCTRL->PERI_CLKEN0)), SysctrlPeripheralI2c1, FALSE);//关闭I2C1时钟
//#endif
//
//#define I2c_MEM_ADD_MSB(__ADDRESS__)        ((u8)((u16)(((u16)((__ADDRESS__) & (u16)(0xFF00))) >> 8)))//获取__ADDRESS__高字节
//#define I2c_MEM_ADD_LSB(__ADDRESS__)        ((u8)((u16)((__ADDRESS__) & (u16)(0x00FF)))) // 获取__ADDRESS__低字节
//#define I2c_FLAG_MASK                       ((u32)0x0001FFFF)
//#define I2c_GET_FLAG(HANDLE, FLAG)          (HANDLE->STAT == FLAG)//获取寄存器HANDLE中的状态位

//#define I2C_ISR_START                       0x08
//#define I2C_ISR_REPSTART                    0x10
//#define I2C_ISR_READADDR                    0x40
//#define I2C_ISR_READDATA                    0x50
//#define I2C_ISR_READADDR_NOACK              0x48
//#define I2C_ISR_READDATA_NOACK              0x58
//#define I2C_ISR_TXADDR                      0x18
//#define I2C_ISR_TXDATA                      0x28
//
//#define I2C_ISR_WRITEFAIL                   0x38
//#define I2C_ISR_WRITEADDR_NOACK             0x20
//#define I2C_ISR_WRITEDATA_NOACK             0x30
//#define I2C_ISR_RXNE                        0x50
////#define I2C_ISR_NACKF                     0x58
//#define I2C_ISR_IDLE                        0xf8



//选择I2C时钟频率
#define I2C_100K                1
#define I2C_400K                0
#if I2C_100K
#define I2C_CLK                100             //100K
#endif
#if I2C_400K
#define I2C_CLK                400             //400K
#endif


//写地址
#define Write_Addr             0xA2
//读地址
#define Read_Addr              0xA2


#define cst_EE_M_IDLE      ( 0 )
#define cst_EE_M_WRITE_S0  ( 1 )
#define cst_EE_M_WRITE_S1  ( 2 )
#define cst_EE_M_WRITE_S2  ( 3 )
#define cst_EE_M_READ_S0   ( 4 )
#define cst_EE_M_READ_S1   ( 5 )

////主状态机状态号
//#define cst_EE_M_S0     0
//#define cst_EE_M_S1     1
//#define cst_EE_M_S2     2
//#define cst_EE_M_S3     3
//#define cst_EE_M_S4     4
//#define cst_EE_M_S5     5
//#define cst_EE_M_S6     6
//#define cst_EE_M_S7     7
//#define cst_EE_M_S8     8
////中断任务状态机状态号
//#define cst_I2C_S0      0
//#define cst_I2C_SR_S0   10
//#define cst_I2C_SR_S1   11
//#define cst_I2C_SR_S2   12
//#define cst_I2C_SR_S3   13
//#define cst_I2C_SR_S4   14
//#define cst_I2C_SR_S5   15
//#define cst_I2C_SW_S0   20
//#define cst_I2C_SW_S1   21
//#define cst_I2C_SW_S2   22
//#define cst_I2C_SW_S3   23
//#define cst_I2C_SW_S4   24
//#define cst_I2C_SW_S5   25


//数据结构相关
typedef union
{
	struct
	{
		unsigned char lowbyte;
		unsigned char highbyte;
	} byte_form;
	unsigned short word_form;
} TWO_BYTE_TO_WORD;

typedef union
{
	struct
	{
		u8 ErrNo         : 4;
		u8 Completed     : 1;
		u8 EorW          : 1;
		u8 EE_normal     : 1;
		u8 SleepEN       : 1;
	}structs;
	u8 MessageInfo;
} EE_Message;


/*------------------------------ Variable Define -------------------------------------*/
typedef struct
{
	u8  main_state; //主状态机状态
	u16 ee_addr;
	u8* data_ptr;   //指向写buff或读buff
    volatile uint16_t rxcnt; //已接收长度
    volatile uint16_t rxlen; //接收总长度
    volatile uint16_t txlen; //发送总长度
    volatile uint16_t txcnt; //已发送长度
    volatile uint16_t has_txcnt; //需要发送到此长度后，再开启下一次传输，eeprom分页机制
} s_g_EE_t;

/*-------------------------------- Function Declare ---------------------------------*/

//    extern u8 s_g_EE_main_state;
void EepInit(void);
void EepMachineDriver(void);
void EepPreSleep(void);
void EepWakeSleep(void);
void I2cIsrMachine(void);
u8 EepRead(u16 EEaddr, u8 * Data, u16 Num, u8* pMsg);
u8 EepWrite(u8* Data, u16 EEaddr, u16 Num, u8* pMsg);
u8 EepCheckMsg(u8 Msg);
u8 EepCheckState(void);
u8 EepIfIdle(void);
u8 EepIfSleep(void);

#ifdef __cplusplus
}
#endif		

#endif /* __U_EEP_DRIVER_H */	
