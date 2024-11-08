#pragma once

/*****************************************************************************************************
 * @brief   如用户复用LCD相关IO口，且关心深睡倒灌，则深睡之前，需将LCD对应的IO脚配置为LCD模式，以达到深睡期间防倒灌
            1200(s)支持的LCD引脚为GPIO14，GPIO20~26，GPIO52~54。
            2100s支持的LCD引脚为GPIO14，GPIO20~54。
 * @note    优先推荐客户使用GPIO3~4和GPIO_WKP1-3这5个引脚来支持防倒灌，若不够用，再复用LCD引脚来做到深睡防倒灌
 * @attention 由客户自行在函数Before_Deepsleep_Hook里调用该接口
 * @warning   该函数会造成CP核无法在线接jlink，需注掉才行
*****************************************************************************************************/
void RCR_LCD_IO_All_Enable(void);

/*****************************************************************************************************
 * @brief   如用户复用LCD相关IO口，且关心深睡倒灌。则工作期间在使用相关外设之前，执行该接口，以去初始化LCD的配置，达到复用。
            1200(s)支持的LCD引脚为GPIO14，GPIO20~26，GPIO52~54。
            2100s支持的LCD引脚为GPIO14，GPIO20~54。
 * @note      优先推荐客户使用GPIO3~4和GPIO_WKP1-3这5个引脚来支持防倒灌，若不够用，再复用LCD引脚来做到深睡防倒灌
 * @attention 由用户在外设初始化(如HAL_I2C_Init/HAL_SPI_Init)之前调用该接口，以达到去初始化LCD配置，与RCR_LCD_IO_All_Enable配对使用。
 * @warning   如果用户不关心通信外设的深睡倒灌，则无需关注该接口
*****************************************************************************************************/
void RCR_LCD_IO_All_Disable(void);
