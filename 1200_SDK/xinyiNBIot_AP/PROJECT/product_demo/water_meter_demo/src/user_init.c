#include "user_gpio.h"
#include "hal_gpio.h"
#include "mcu_adapt.h"
#include "user_i2c.h"

void GPIO_Output_Init(void)
{
    HAL_GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = USER_LEDGRE;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_FLOAT;
    HAL_GPIO_Init(&gpio_init);                

    McuGpioModeSet(USER_LEDRED,0x00);   

    McuGpioModeSet(EEPROM_I2C_WP,0x00);  

    gpio_init.Pin = USER_CTL_IR;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_FLOAT;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = USER_CHK_MD;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULL_UP;
    HAL_GPIO_Init(&gpio_init);


    /*-------------------------------未用管脚输出下拉-----------------------------*/
    gpio_init.Pin = TEST_GPIO_21;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_PULL_DOWN;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_22;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_23;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_24;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_25;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_26;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_9;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = TEST_GPIO_10;
    HAL_GPIO_Init(&gpio_init);
}

void LowPower_I2C_Init(void)
{
    HAL_GPIO_InitTypeDef gpio_init = {0};
    
    gpio_init.Pin = EEPROM_I2C_SCL;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_FLOAT;
    HAL_GPIO_Init(&gpio_init);  

    gpio_init.Pin = EEPROM_I2C_SDA;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_FLOAT;
    HAL_GPIO_Init(&gpio_init);  

    HAL_GPIO_WritePin(EEPROM_I2C_SCL, GPIO_PIN_SET);

    HAL_GPIO_WritePin(EEPROM_I2C_SDA, GPIO_PIN_SET);

    Erom_WR_UN(); 
}

void LowPower_SPI_Init(void)
{
    HAL_GPIO_InitTypeDef gpio_init = {0};
    
    gpio_init.Pin = USER_SPI_CS;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_FLOAT;
    HAL_GPIO_Init(&gpio_init);  

    gpio_init.Pin = USER_SPI_MOSI;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = USER_SPI_MISO;
    HAL_GPIO_Init(&gpio_init);

    gpio_init.Pin = USER_SPI_CLK;
    HAL_GPIO_Init(&gpio_init);                                     
    
    McuGpioModeSet(USER_SPI_EN,0x00);

    HAL_GPIO_WritePin(USER_SPI_CS, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(USER_SPI_MOSI, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(USER_SPI_MISO, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(USER_SPI_CLK, GPIO_PIN_RESET);
       
    McuGpioWrite(USER_SPI_EN, 0);                           
}

void User_Gpio_Init(void)//IO初始状态初始化,初始化指示
{
    GPIO_Output_Init();                                            //IO初始化  

    LowPower_I2C_Init();                                           //E2关闭

    LowPower_SPI_Init();                                           //SPI低功耗初始化
    
    LEDGON();

    LEDRON();
    
    HAL_Delay(1000);

    LEDGOFF();

    LEDROFF();

    HWOFF();                                                        //关闭红外                                                    
}
