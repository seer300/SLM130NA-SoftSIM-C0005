#pragma once
#include "user_gpio.h"
#include "mcu_adapt.h"

#define Erom_WR_UN()             McuGpioWrite(EEPROM_I2C_WP, 1)
#define Erom_WR_EN()             McuGpioWrite(EEPROM_I2C_WP, 0)

void User_I2C_Master_Init(void);

void User_I2C_Master_DeInit(void);

void Write_EEPROM_With_Page(uint16_t addr,uint8_t *Buf,uint16_t len);

void Read_EEPROM_With_Page(uint16_t addr,uint8_t *RamBuf,uint16_t len);

void Write_EEPROM_And_Check(uint16_t addr,uint8_t *Buf,uint16_t len);

void Read_EEPROM_And_Check(uint16_t addr,uint8_t *RamBuf,uint16_t len);