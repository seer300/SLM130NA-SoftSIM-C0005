#pragma once

#include "driver_test_common.h"

/**
 * @brief 发送AT+SLAVE?查询是否为从机
 *        发送AT+SLAVE=Test进入外设驱动测试流程，等待主机命令
 */
int at_SLAVE_req(char *param, char **rsp_cmd);

/****************驱动测试的各个外设从机模式入口*******************/
void uart_slave_test(UartArgStruct *pUartArgStruct);
void i2c_slave_test(I2cArgStruct *pI2cArgStruct);
void spi_slave_test(SpiArgStruct *pSpiArgStruct);
void csp_uart_slave_test(CspUartArgStruct *pCspUartArgStruct);
void csp_spi_slave_test(CspSpiArgStruct *pCspSpiArgStruct);
void timer_slave_test(void);
