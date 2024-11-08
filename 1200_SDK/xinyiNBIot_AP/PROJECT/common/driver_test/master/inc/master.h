#pragma once

#include "driver_test_common.h"

/**
 * @brief 发送AT+MASTER?查询是否为主机
 *        发送AT+MASTER=Test<UART><I2C><SPI><CSP_UART><CSP_SPI><TestTimes:2><DebugLevel:2>进入外设驱动测试流程，发送命令给从机
 */
int at_MASTER_req(char *param,char **rsp_cmd);

/****************驱动测试的各个外设主机模式入口*******************/
void uart_master_test(CuTest *tc);
void i2c_master_test(CuTest *tc);
void spi_master_test(CuTest *tc);
void csp_uart_master_test(CuTest *tc);
void csp_spi_master_test(CuTest *tc);
void timer_master_test(CuTest *tc);
void adc_test(CuTest *tc);
