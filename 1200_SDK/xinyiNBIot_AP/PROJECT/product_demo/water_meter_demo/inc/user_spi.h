#pragma once

#include "user_gpio.h"
#include "mcu_adapt.h"
#include "hal_spi.h"

#define  AUL_TEST  (0)

HAL_SPI_HandleTypeDef spi_master;

#if AUL_TEST
#define AU20xx_CMD_READ         (0x00)
#define AU20xx_CMD_WRITE        (0x40)
#define AU20xx_CMD_RESET        (0xC0)

#define AU2001_SNS1_OFF0        (0x06)
#define AU2001_SNS1_OFF1        (0x07)
#define AU2001_SNS2_OFF0        (0x08)
#define AU2001_SNS2_OFF1        (0x09)

#define bsp_set_en()            McuGpioWrite(USER_SPI_EN, 1);//AGPIO_Set(USER_SPI_EN)
#define bsp_reset_en()          McuGpioWrite(USER_SPI_EN, 0);//AGPIO_Clear(USER_SPI_EN)

#define USER_SENS_ENABLE        bsp_set_en()
#define USER_SENS_DISABLE       bsp_reset_en()

#define SPI_CS_OUTPUT_HIGH      {bsp_spi_delay(); HAL_SPI_SetCS(HAL_SPI_CS0);} //CS引脚拉高(之前加个延时)
#define SPI_CS_OUTPUT_LOW       HAL_SPI_ResetCS(HAL_SPI_CS0)    

#else

#define USER_SENS_ENABLE        McuGpioWrite(USER_SPI_EN, 1)
#define USER_SENS_DISABLE       McuGpioWrite(USER_SPI_EN, 0)

#define USER_GET_SPI_STATUS     HAL_SPI_GetState(&spi_master)
#define USER_WAIT_SPI_STATUS    while((USER_GET_SPI_STATUS == HAL_SPI_STATE_BUSY))

#define SPI_CS_OUTPUT_HIGH      {USER_WAIT_SPI_STATUS; HAL_SPI_SetCS(HAL_SPI_CS0);} //CS引脚拉高(之前加个延时)
#define SPI_CS_OUTPUT_LOW       HAL_SPI_ResetCS(HAL_SPI_CS0)    
#endif

void User_SPI_Func(void);