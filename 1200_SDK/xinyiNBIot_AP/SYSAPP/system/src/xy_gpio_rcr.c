#include "gpio.h"
#include "system.h"


#if (MODULE_VER == 0x0)// opencpu、表计
static uint8_t g_gpio_rcr = 0;
#endif

/*****************************************************************************************************
* @brief   使能pad为防倒灌模式，全部防倒灌脚为GPIO14，GPIO20~54。
            1200(s)支持的引脚为GPIO14，GPIO20~26，GPIO52~54。
            2100s支持的引脚为GPIO14，GPIO20~54。
* @warning  该函数会造成CP核无法在线接jlink，需注掉才行
*****************************************************************************************************/
__FLASH_FUNC void RCR_LCD_IO_All_Enable(void)
{
#if (MODULE_VER == 0x0) // opencpu、表计
    if(!g_gpio_rcr)
    {
        GPIO_AllPad_Enable_Rcr();
        g_gpio_rcr = 1;
    }
#endif
}

/*****************************************************************************************************
* @brief   失能pad为防倒灌模式，全部防倒灌脚为GPIO14，GPIO20~54。
            1200(s)支持的引脚为GPIO14，GPIO20~26，GPIO52~54。
            2100s支持的引脚为GPIO14，GPIO20~54。
*****************************************************************************************************/
__FLASH_FUNC void RCR_LCD_IO_All_Disable(void)
{
#if (MODULE_VER == 0x0) // opencpu、表计
    if(g_gpio_rcr)
    {
        GPIO_AllPad_Disable_Rcr();
        g_gpio_rcr = 0;
    }
#endif
}



