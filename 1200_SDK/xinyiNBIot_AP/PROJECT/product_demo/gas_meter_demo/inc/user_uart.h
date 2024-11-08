/*****************************************************************************************************************************	 
 * user_uart.h
 ****************************************************************************************************************************/

#ifndef USER_UART_H__
#define USER_UART_H__

#define FAC_UART                    (0)
#define TEST_UART                   (2)
#define TEST_UART_DEEPSLEEP_LOCK    USER_DSLEEP_LOCK1
#define UART_LEN_MAX (512)  //uart最大收发长度，单位字节

typedef struct 
{
    volatile uint16_t rxcnt;
    volatile bool rxdone;
    volatile uint16_t txlen;
    volatile uint16_t txcnt;
} McuUartIt_Typedef;


extern void UserFactUartInit(void);
extern void UserTestUartInit(void);
extern void UserTestUartDis(void);
extern void UserUartFunc(void);

#endif

