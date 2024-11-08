/*****************************************************************************************************************************	 
 * user_eeprom.h
 ****************************************************************************************************************************/

#ifndef USER_EEPROM_H__
#define USER_EEPROM_H__

#include "basic_config.h"

/****************************************define*******************************************************/
#define I2C_ID                  (0)     //i2c no
#define I2C_CLK                 (400)   //i2c clk  单位K
#define I2C_DEVICE_ADDR         (0xA0)  //i2c从机设备地址
#define I2C_DEVICE_VCC_PIN      (41)    //i2c从机设备电源控制GPIO引脚号
#define I2C_LEN_MAX             (100)  //i2c最大收发长度，单位字节
#define I2C_PAGE_SIZE           (32)    //i2c eeprom页长度，单位字节，如：AT24C02该宏为8，AT24C64该宏为32
#define I2C_DATA_ADDR_SIZE      (2)     //i2c eeprom片内数据地址宽度，1或2，如：AT24C02该宏为1，AT24C64该宏为2
#define I2C_EVENT         		EVENT_USER_DEFINE3

/****************************************func*******************************************************/
extern void UserEepromTest(void);

#endif

