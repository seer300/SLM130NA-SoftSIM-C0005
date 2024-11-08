#pragma once
#include "hal_gpio.h"
#include "mcu_adapt.h"
#include "user_gpio.h"

uint8_t g_hw_init;

#define KEYTIME                 (60)        //开红外时间
#define KEY_VAL_SET             (0xaa)       //按键按下

#define USER_LEDGRE             GPIO_PAD_NUM_2                  //绿灯
#define USER_LEDRED             MCU_WKP3                        //红灯 AGIO
#define USER_CTL_IR             GPIO_PAD_NUM_5                  //IO红外控制脚
#define USER_CHK_MD             GPIO_PAD_NUM_7                  //磁敏检测

/*<-----------------------EEPROM_I2C----------------------*/
#define EEPROM_I2C_SDA          GPIO_PAD_NUM_20
#define EEPROM_I2C_SCL          GPIO_PAD_NUM_14
#define EEPROM_I2C_WP           MCU_WKP1                        //即AGPIO0

/*<-----------------------SPI----------------------------*/
#define USER_SPI_EN             MCU_WKP2                        //即AGPIO1
#define USER_SPI_CS             GPIO_PAD_NUM_6
#define USER_SPI_MISO           GPIO_PAD_NUM_53
#define USER_SPI_CLK            GPIO_PAD_NUM_54
#define USER_SPI_MOSI           GPIO_PAD_NUM_52

/*---------------------------未用管脚-------------------------*/
#define TEST_GPIO_9             GPIO_PAD_NUM_9
#define TEST_GPIO_10            GPIO_PAD_NUM_10
#define TEST_GPIO_21            GPIO_PAD_NUM_21
#define TEST_GPIO_22            GPIO_PAD_NUM_22
#define TEST_GPIO_23            GPIO_PAD_NUM_23
#define TEST_GPIO_24            GPIO_PAD_NUM_24
#define TEST_GPIO_25            GPIO_PAD_NUM_25
#define TEST_GPIO_26            GPIO_PAD_NUM_26

#define HWON()                  HAL_GPIO_WritePin(USER_CTL_IR, GPIO_PIN_RESET)//红外开关
#define HWOFF()                 HAL_GPIO_WritePin(USER_CTL_IR, GPIO_PIN_SET)

#define LEDGON()                HAL_GPIO_WritePin(USER_LEDGRE, GPIO_PIN_SET)//绿灯开关
#define LEDGOFF()               HAL_GPIO_WritePin(USER_LEDGRE, GPIO_PIN_RESET)

#define LEDRON()                McuGpioWrite(USER_LEDRED, 1)//红灯开关
#define LEDROFF()               McuGpioWrite(USER_LEDRED, 0)

void HW_Init(void);
void Key_Check(void);
void Key_Func(void);