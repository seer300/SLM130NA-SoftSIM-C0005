#include "user_gpio.h"
#include "hal_i2c.h"
#include "user_init.h"
#include "user_i2c.h"
#include "user_spi.h"

#if AUL_TEST
#define EEPROMMaxSize     (128) 
#else
#define EEPROMMaxSize     (32) 
#endif

#define SlaveAddress      (0xa0)               //E2的器件地址

HAL_StatusTypeDef ret_iic = HAL_ERROR;

HAL_I2C_HandleTypeDef i2c1_master = {0};

__RAM_FUNC void User_I2C_Master_Init(void)
{
    HAL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_I2C1_SCL;
    GPIO_InitStruct.Pin = EEPROM_I2C_SCL;
    HAL_GPIO_Init(&GPIO_InitStruct);

    GPIO_InitStruct.Mode = GPIO_MODE_HW_PER;
    GPIO_InitStruct.PinRemap = GPIO_I2C1_SDA;
    GPIO_InitStruct.Pin = EEPROM_I2C_SDA;
    HAL_GPIO_Init(&GPIO_InitStruct);

    //I2C初始化
    i2c1_master.Instance = HAL_I2C1;
    i2c1_master.Init.Mode = HAL_I2C_MODE_MASTER;
    i2c1_master.Init.ClockSpeed = HAL_I2C_SPEED_400K;
    i2c1_master.Init.AddressingMode = HAL_I2C_ADDRESS_7BITS;
    HAL_I2C_Init(&i2c1_master);
}

__RAM_FUNC void User_I2C_Master_DeInit(void)
{
	HAL_I2C_DeInit(&i2c1_master);
}

void Write_EEPROM_With_Page(uint16_t addr,uint8_t *Buf,uint16_t len)  
{
	/****************硬件I2C模块重新初始化******************/
	User_I2C_Master_DeInit();
    User_I2C_Master_Init();

    uint16_t DataLen = 0;
    uint16_t UsedLen = 0;
    uint16_t UsedAddr = addr;
    uint16_t WriteLen = len;

	while(WriteLen > 0)//是否需要写数据
	{
		if((UsedAddr % EEPROMMaxSize) == 0)//地址是整页的起始地址
		{
			if(WriteLen >= EEPROMMaxSize)
			{
				UsedLen = EEPROMMaxSize;
			}
			else
			{
				UsedLen = WriteLen;
			}
		}
        else//地址非整页的起始地址
		{
			UsedLen = EEPROMMaxSize - (UsedAddr % EEPROMMaxSize);//计算当前区块剩余字节长度

			if(WriteLen >= UsedLen)//地址块写不下
			{
				UsedLen = UsedLen;
			}
			else
			{
				UsedLen = WriteLen;
			}
		}

        Erom_WR_EN();
        ret_iic = HAL_I2C_Mem_Write(&i2c1_master,SlaveAddress,UsedAddr,HAL_I2C_MEMADD_SIZE_16BIT,&Buf[DataLen],UsedLen,400);
        if(ret_iic != HAL_OK)
        {
            xy_assert(0);
        }
        HAL_Delay(10);
        Erom_WR_UN();

		WriteLen = WriteLen - UsedLen;//待写数据长度

		if(WriteLen > 0)
		{
			UsedAddr = UsedAddr +UsedLen;

			DataLen = DataLen +UsedLen;
		}
	}
    LowPower_I2C_Init();
}

void Read_EEPROM_With_Page(uint16_t addr,uint8_t *RamBuf,uint16_t len)
{
	/****************硬件I2C模块重新初始化******************/
	User_I2C_Master_DeInit();
    User_I2C_Master_Init();

    uint16_t DataLen = 0;
    uint16_t UsedLen = 0;
    uint16_t UsedAddr = addr;
    uint16_t ReadLen = len;
 
    while(ReadLen > 0)
    {
		if((UsedAddr % EEPROMMaxSize) == 0)  //地址是整页的起始地址
		{
			if(ReadLen >= EEPROMMaxSize)
			{
				UsedLen = EEPROMMaxSize;
			}
			else
			{
				UsedLen = ReadLen;
			}
		}
        else//地址非整页的起始地址
		{
			UsedLen = EEPROMMaxSize - (UsedAddr % EEPROMMaxSize);//计算当前区块剩余字节长度

			if(ReadLen >= UsedLen)  //地址区块读不完
			{
				UsedLen = UsedLen;
			}
			else
			{
				UsedLen = ReadLen;
			}
		}

        ret_iic = HAL_I2C_Mem_Read(&i2c1_master,SlaveAddress,UsedAddr,HAL_I2C_MEMADD_SIZE_16BIT,&RamBuf[DataLen],UsedLen,400);
        if(ret_iic != HAL_OK)
        {
            xy_assert(0);
        }
		ReadLen = ReadLen - UsedLen;// 待读数据长度

		if(ReadLen > 0)
		{
			UsedAddr = UsedAddr + UsedLen;

			DataLen = DataLen + UsedLen;
		}
	}
    LowPower_I2C_Init();
}