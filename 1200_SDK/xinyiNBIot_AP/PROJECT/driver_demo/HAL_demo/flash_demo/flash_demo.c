/**
* @file		flash_demo.c
* @brief    对flash进行读写擦操作，验证其功能是否正常   
***********************************************************************************/
#include "hal_def.h"
#include "xy_flash.h"

//使用USER_FLASH中的最后4K区域测试
#if FLASH_2M
#define FLASH_TEST_ADDR (USER_FLASH_BASE + 0xF000)
#else
#define FLASH_TEST_ADDR (USER_FLASH_BASE + 0x1F000)
#endif
#define FLASH_TEST_LEN  (0x1000)

__RAM_FUNC int main(void)
{
    int i;
    uint8_t * wbuf1 = NULL;
	uint8_t * rbuf = NULL;
	static uint8_t test_times = 0;

    SystemInit();
	
	xy_printf("flash demo start\r\n");

    wbuf1 = xy_malloc(FLASH_TEST_LEN);
	rbuf = xy_malloc(FLASH_TEST_LEN);

    for(i = 0 ; i < FLASH_TEST_LEN; i++)
    {
        wbuf1[i] = (uint8_t)i;
    }

    for(i = 0 ; i < FLASH_TEST_LEN; i++)
    {
        rbuf[i] = 0;
    }

    while(1)
    {
	    //防止该demo长时间循环调用损坏flash
        test_times ++;
        if(test_times > 5)
        {
            xy_printf("flash demo suspennd\r\n");
            while(1);
        }

        //该接口只会改变要写的空间内容，不会对其他flash区域内容产生影响
        if(xy_Flash_Write(FLASH_TEST_ADDR, wbuf1, FLASH_TEST_LEN) == false)
        {
            xy_printf("flash write fail\r\n");
            while(1);
        }
        else
        {
            xy_printf("flash write complete\r\n");
        }

        //调用了Flash读接口，检测flash写是否成功
        if(xy_Flash_Read(FLASH_TEST_ADDR, rbuf, FLASH_TEST_LEN) == false)
        {
            xy_printf("flash read fail\r\n");
            while(1);
        }
        else
        {
            xy_printf("flash read success\r\n");
        }

        //数据检测
        for(i = 0 ; i < FLASH_TEST_LEN; i++)
        {
            if(rbuf[i] != (uint8_t)i)
            {
                xy_printf("flash read fail at addr=%0x, expected value=%0x, actual value=%0x\r\n",  FLASH_TEST_ADDR + i, (uint8_t)i, rbuf[i]);
                while(1);
            }
        }

        xy_printf("flash read success for write flash\r\n");      
    }

    return 0;
}
