/**
 * @file mcu_i2c_demo.c
 * @author pancq 
 * @brief 该demo为用户演示了i2c eeprom的读写操作，以AT24C64为例。
 *        用户可以根据实际使用的i2c eeprom数据手册修改 I2C_PAGE_SIZE 和 I2C_DATA_ADDR_SIZE 以适配自己的i2c eeprom
 * @version 0.1
 * @date 2023-06-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "mcu_adapt.h"
#include "xy_system.h"
#include "system.h"
#include "string.h"


//该宏定义i2c从机设备地址的最低位为读写控制位，默认为0，
//调用i2c读写接口时，接口内会实现读写控制位的改变，用户无需修改该位
#define DEVICE_ADDR     (0xA0) //i2c从机设备地址
#define I2C_LEN_MAX     (1000) //i2c最大收发长度，单位字节
#define I2C_PAGE_SIZE     (32) //i2c eeprom页长度，单位字节，如：AT24C02该宏为8，AT24C64该宏为32
#define I2C_DATA_ADDR_SIZE (2) //i2c eeprom片内数据地址宽度，1或2，如：AT24C02该宏为1，AT24C64该宏为2

uint8_t i2c_tx_buff[I2C_LEN_MAX] = {0};
uint8_t i2c_rx_buff[I2C_LEN_MAX] = {0};

__RAM_FUNC int main(void)
{
    SystemInit();
    EnablePrimask();

    //准备发送数据
    uint32_t integer = I2C_LEN_MAX / 256;
    uint32_t remainder = I2C_LEN_MAX % 256;
    for (uint32_t i = 0; i < integer; i++)
    {
        for(uint32_t j = 0; j < 256; j++)
        {
            i2c_tx_buff[i * 256 + j] = (uint8_t)j;
        }
    }
    for(uint32_t i = 0; i < remainder; i++)
    {
        i2c_tx_buff[integer * 256 + i] = (uint8_t)i;
    }

    //初始化i2c接口，400KHz
    McuI2cMasterSet2(0, 400, 36, 35);

    //按页写1000字节数据
    uint32_t send_cnt_integer = I2C_LEN_MAX / I2C_PAGE_SIZE;
    uint32_t send_cnt_remainder = I2C_LEN_MAX % I2C_PAGE_SIZE;

    for (uint32_t i = 0; i < send_cnt_integer; i++)
    {
        McuI2cMasterWrite(0, DEVICE_ADDR, I2C_DATA_ADDR_SIZE, i * I2C_PAGE_SIZE, &(i2c_tx_buff[i * I2C_PAGE_SIZE]), I2C_PAGE_SIZE, 50);
        HAL_Delay(5); //i2c eeprom写周期为5ms，必要延时
    }

    McuI2cMasterWrite(0, DEVICE_ADDR, I2C_DATA_ADDR_SIZE, send_cnt_integer * I2C_PAGE_SIZE, &(i2c_tx_buff[send_cnt_integer * I2C_PAGE_SIZE]), send_cnt_remainder, 50);
    HAL_Delay(5); //i2c eeprom的写周期为5ms，必要延时

    //顺序读1000字节数据
    McuI2cMasterRead(0, DEVICE_ADDR, I2C_DATA_ADDR_SIZE, 0, i2c_rx_buff, I2C_LEN_MAX, 100);

    //将i2c eeprom读写结果进行比较，一致则打印串口消息
    if(memcmp(i2c_rx_buff, i2c_tx_buff, I2C_LEN_MAX) == 0)
    {
        const uint8_t uart_message[50] = {0};
        snprintf((char*)uart_message, 50, "CAT24C64 Write and Read %dByte Succeed!\r\n", I2C_LEN_MAX);

        McuUartSet(1, 115200, 8, 2, 0, MCU_GPIO18, MCU_GPIO19);//txd -> gpio18，可观察结果
        McuUartTxEn(1);
        McuUartWriteFram(1, uart_message, strlen((char*)uart_message));
    }

    while (1)
    {
        ;
    }
}

