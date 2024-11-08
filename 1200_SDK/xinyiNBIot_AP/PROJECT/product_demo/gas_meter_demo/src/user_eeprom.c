/*****************************************************************************************************************************	 
 * user_eeprom.c
 ****************************************************************************************************************************/

#include "user_debug.h"
#include "mcu_adapt.h"
#include "user_eeprom.h"
#include "vmcu.h"
#include "type_adapt.h"

/*******************************************************************************************************/
/*******************************************************************************************************/
/****************************************   iic  *******************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

uint8_t s_i2c_tx_buff[I2C_LEN_MAX] = {0};
uint8_t s_i2c_rx_buff[I2C_LEN_MAX] = {0};

__TYPE_IRQ_FUNC void UserEepromTest(void)
{
    if(is_event_set(I2C_EVENT))
    {
        clear_event(I2C_EVENT);

        //按页写1000字节数据
        uint32_t send_cnt_integer = I2C_LEN_MAX / I2C_PAGE_SIZE;
        uint32_t send_cnt_remainder = I2C_LEN_MAX % I2C_PAGE_SIZE;

        VmcuGpioModeSet(I2C_DEVICE_VCC_PIN, 0); //拉高iic CTL
        VmcuGpioWrite(I2C_DEVICE_VCC_PIN, 1);

        //准备发送数据
        for (uint32_t i = 0; i < I2C_LEN_MAX; i++)
        {
            s_i2c_tx_buff[i] = i % 256;
        }

        memset(s_i2c_rx_buff, 0x0, I2C_LEN_MAX);
        DisablePrimask();
        //iic初始化
        McuI2cMasterSet2(I2C_ID, I2C_CLK, MCU_GPIO36, MCU_GPIO35);

        for (uint32_t i = 0; i < send_cnt_integer; i++)
        {
            McuI2cMasterWrite(I2C_ID, I2C_DEVICE_ADDR, I2C_DATA_ADDR_SIZE, i * I2C_PAGE_SIZE, &(s_i2c_tx_buff[i * I2C_PAGE_SIZE]), I2C_PAGE_SIZE, 50);
            delay_func_us(5000); //i2c eeprom写周期为5ms，必要延时
        }
        McuI2cMasterWrite(I2C_ID, I2C_DEVICE_ADDR, I2C_DATA_ADDR_SIZE, send_cnt_integer * I2C_PAGE_SIZE, &(s_i2c_tx_buff[send_cnt_integer * I2C_PAGE_SIZE]), send_cnt_remainder, 50);
        delay_func_us(5000); //i2c eeprom的写周期为5ms，必要延时

        //顺序读100字节数据
        McuI2cMasterRead(I2C_ID, I2C_DEVICE_ADDR, I2C_DATA_ADDR_SIZE, 0, s_i2c_rx_buff, I2C_LEN_MAX, 50);
        EnablePrimask();

        //将eeprom读写结果进行比较，一致则打印串口消息
        if(memcmp(s_i2c_rx_buff, s_i2c_tx_buff, I2C_LEN_MAX) == 0)
        {
            jk_printf(" iic Read-write equality\r\n");
        }
        else
        {
            jk_printf(" !,iic Read-write not equality\r\n");
        }
        VmcuGpioWrite(I2C_DEVICE_VCC_PIN, 0);
    }
}


