/**
  ******************************************************************************
  * @file    hc32_uart_driver.h
  * @author  (C)2020, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2020-7-1
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __U_UART_DRIVER_H
#define __U_UART_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

//#include "sys_config.h"
//#include "type.h"
//#include "ddl.h"
#include <stdint.h>
#include "type.h"

#define HDSC_HC32L196

/* MACRO Define---------------------------------------------------------------*/

//所有串口，将要开启的串口置1
#define COM0_OPEN   0
#define COM1_OPEN   0
#define COM2_OPEN   1
#define COM3_OPEN   0
#define LPCOM0_OPEN 1   //红外串口
#define LPCOM1_OPEN 0   //NB串口

//串口数量和串口定义    （总共六个串口）
typedef enum
{ 
	LPCOM0 = 0,//低功耗串口
	COM0,
	COM1,
	COM2,
	COM3,
	LPCOM1,
	COM_MAX_NUM
}UART_ENUM;

#if MODULE_BLE_USED
#define BLE_UART_COM      LPCOM1
#else
#define NB_UART_COM 	  LPCOM1   //NB串口
#endif
#define IR_UART_COM 	  LPCOM0 //红外串口
/*!< LPCOM USART Enable in SLEEP Mode */
#define  LPCOM1_ENABLESLEEPMODE               0 


#define IR_38K_OPEN       1
#if (HARDWARE_VERSION==0x14)//方壳表
#define IR_UART_CTL_PORT  (GpioPortE)
#define IR_UART_CTL_PIN   (GpioPin2)

#elif (HARDWARE_VERSION==0x18)//圆壳表
#define IR_UART_CTL_PORT  (GpioPortF)
#define IR_UART_CTL_PIN   (GpioPin6)
#endif


#if IR_38K_OPEN
//TIM2_CHB
#define IR_38K_PORT  (GpioPortA)
#define IR_38K_PIN   (GpioPin3)
#define IR_38K_AF    (GpioAf4)
#endif

//串口IO配置(IO一定要配置准确)、优先级、时钟及定时器配置
#if COM0_OPEN
//UART0
#define UART0_RX_PORT    (GpioPortA)
#define UART0_RX_PIN     (MCU_GPIO10)
#define UART0_RX_AF      (GpioAf1)

#define UART0_TX_PORT    (GpioPortA)
#define UART0_TX_PIN     (MCU_GPIO12)
#define UART0_TX_AF      (GpioAf1)
#endif

#if COM1_OPEN
//UART1
#define UART1_RX_PORT    (GpioPortA)
#define UART1_RX_PIN     (MCU_GPIO10)
#define UART1_RX_AF      (GpioAf1)

#define UART1_TX_PORT    (GpioPortA)
#define UART1_TX_PIN     (MCU_GPIO12)
#define UART1_TX_AF      (GpioAf1)
#endif

#if COM2_OPEN
//UART2
#define UART2_RX_PORT    (GpioPortC)
#define UART2_RX_PIN     (MCU_GPIO10)
#define UART2_RX_AF      (GpioAf4)

#define UART2_TX_PORT    (GpioPortC)
#define UART2_TX_PIN     (MCU_GPIO12)
#define UART2_TX_AF      (GpioAf5)

#endif

#if COM3_OPEN
//UART3
#define UART3_RX_PORT    (GpioPortA)
#define UART3_RX_PIN     (GpioPin2)
#define UART3_RX_AF      (GpioAf1)

#define UART3_TX_PORT    (GpioPortA)
#define UART3_TX_PIN     (GpioPin3)
#define UART3_TX_AF      (GpioAf1)
#endif

#if LPCOM0_OPEN
//LPUART0
#define LPUART0_RX_PORT  (GpioPortC)
#define LPUART0_RX_PIN   (MCU_GPIO3)
#define LPUART0_RX_AF    (GpioAf2)

#define LPUART0_TX_PORT  (GpioPortC)
#define LPUART0_TX_PIN   (MCU_GPIO4)
#define LPUART0_TX_AF    (GpioAf1)
#endif

#if LPCOM1_OPEN
//LPUART1
#define LPUART1_RX_PORT  (GpioPortA)
#define LPUART1_RX_PIN   (GpioPin1)
#define LPUART1_RX_AF    (GpioAf2)

#define LPUART1_TX_PORT  (GpioPortA)
#define LPUART1_TX_PIN   (GpioPin0)
#define LPUART1_TX_AF    (GpioAf2)
#endif

//缓冲器大小（可修改）
#define UP_BUF_SIZE       512         //上行缓冲区大小

#define UART0_BUF_SIZE    UP_BUF_SIZE
#define UART1_BUF_SIZE    UP_BUF_SIZE
#define UART2_BUF_SIZE    UP_BUF_SIZE
#define UART3_BUF_SIZE    UP_BUF_SIZE
#define LPUART0_BUF_SIZE  UP_BUF_SIZE
#define LPUART1_BUF_SIZE  UP_BUF_SIZE

//波特率
#define BAUD1200   0
#define BAUD2400   1
#define BAUD4800   2
#define BAUD9600   3
#define BAUD19200  4
#define BAUD38400  5
#define BAUD57600  6
#define BAUD115200 7

#define DATA_BIT_8  8
//数据位
#define UART_WORDLEN_8B ((u32)8) //8位

#define STOPBIT_1    0  //1个停止位
#define STOPBIT_1_5  1  //1.5个停止位
#define STOPBIT_2    2  
//停止位
#define UartMsk1bit      2//1个停止位
#define UartMsk1_5bit    2//1.5个停止位
#define UartMsk2bit      2//2个停止位
#define UART_STOPBIT_1     UartMsk1bit   //1个停止位
#define UART_STOPBIT_1_5   UartMsk1_5bit //1.5个停止位
#define UART_STOPBIT_2     UartMsk2bit   //2个停止位
//校验
#define UartMskDataOrAddr  0///<多机模式时，通过读写SBUF[8]决定帧为数据帧或地址帧   ((u32)0x00000000) //无校验
#define UartMskEven        2///<非多机模式偶校验    ((u32)0x00000004) //偶校验
#define UartMskOdd         1///<非多机模式奇校验   ((u32)0x00000008)  //奇校验

#define UART_NONE_PARITY   UartMskDataOrAddr ///<多机模式时，通过读写SBUF[8]决定帧为数据帧或地址帧   ((u32)0x00000000) //无校验
#define UART_EVEN_PARITY   UartMskEven       ///<非多机模式偶校验    ((u32)0x00000004) //偶校验
#define UART_ODD_PARITY    UartMskOdd        ///<非多机模式奇校验   ((u32)0x00000008)  //奇校验
 
//接收状态
#define UART_RCV_OVERTIME_S0 0
#define UART_RCV_OVERTIME_S1 1

//发送状态
#define UART_SEND_OVERTIME_S0 0
#define UART_SEND_OVERTIME_S1 1
//低功耗模式开启
#define UART_LPUART_Enable  0
#define UART_LPUART_Disable 1

//波特率计算
#define UART_BAUD_DIV_LPUART(__PCLK__, __BAUD__)     (((u64)(__PCLK__)*256) / ((__BAUD__)))
#define UART_BAUD_DIV_SAMPLING8(__PCLK__, __BAUD__)  (((__PCLK__)*2) / ((__BAUD__)))
#define UART_BAUD_DIV_SAMPLING16(__PCLK__, __BAUD__) (((__PCLK__)) / ((__BAUD__)))

//串口接收超时
#define FRAMEOVERTIME  20 //超时时间为100ms 20*5 = 100ms

//串口发送超时
#define UART_BUSY_OVERTIME  1800 //超时时间为180S 100ms*1800

/* variables Define---------------------------------------------------------------*/
typedef struct
{
	u32 BaudRate;
	u32 WordLength;
	u32 StopBits;
	u32 Parity;
	u32 Mode;
	u32 OverSampling;
	u8 LPModeFunction;
	u8 UARTMode;
} UART_PARA_STRUCT;

#define M0P_UART_TypeDef    void
#define M0P_LPUART_TypeDef    void

typedef struct
{
	M0P_UART_TypeDef *Instance;
	M0P_LPUART_TypeDef *LP_Instance;
	UART_PARA_STRUCT para;
	u8 uart_status;
	u8 *send_point;
	u16 send_len;
	u8 *send_status_point;   //发送状态指针
	u8 rcv_state;
	u8 send_state;
	u8 start_send_flag;
	u8 finish_send_flag;
	u8 send_over_time;   //发送超时时间
	u8 rcv_flag;
	u16 rcv_len;
	u8 rcv_msg;	
} UART_STRUCT;

/* Function Declare------------------------------------------------------------*/
u8 UartInit(u8 com, u8 baud, u8 databit, u8 stop, u8 check, u8 lpmode);
u8 UartIfIdle(u8 com);
void UartSend(u8 com, u16 len, u8 *data, u8 *out_msg);
void UartRcv(u8 com, u16 *out_len, u8 **out_data);
u8 UartRcvFinish(u8 com);
void UartClearMsg(u8 com);
u8 UartIfSleep(u8 com);
void UartPreSleep(u8 com);
void UartWakeSleep(u8 com, u8 baud, u8 databit, u8 stop, u8 check, u8 lpmode);
void UartMachineDriver(void);
M0P_UART_TypeDef *HAL_GetUart_Hander(u8 com); 
M0P_LPUART_TypeDef *HAL_GetLPUart_Hander(u8 com); 

u8 UartRcvMachineIfIdle(u8 com);
u8 UartSendMachineIfIdle(u8 com);
#ifdef __cplusplus
}
#endif

#endif /* __U_UART_DRIVER_H */

