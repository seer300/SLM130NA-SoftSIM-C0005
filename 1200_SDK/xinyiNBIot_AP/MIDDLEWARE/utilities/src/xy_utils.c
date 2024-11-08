#include <stdint.h>
#include "xinyi2100.h"
#include "xy_system.h"
#include "hw_sema.h"
#include "sema.h"
#include "trng.h"
#include "prcm.h"
#include "utc.h"
#include "xy_utils.h"

#define XY_RAND_MAX    0x7FFFFFFF

extern bool CP_Is_Alive(void);

__OPENCPU_FUNC uint32_t get_trng_rand_value(uint8_t* readbuff)
{
	uint32_t roscLength = 0;
	uint32_t sampleCount  = 27;
	volatile int err;

    if (CP_Is_Alive() == true)
    {
        SEMA_Request(SEMA_MASTER_AP, SEMA_SLAVE_TRNG, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
    }
	

	PRCM_ClockEnable(CORE_CKG_CTL_TRNG_EN);

	TRNG_InitializingHardware(TRNG_MODE_FAST,roscLength,sampleCount);
	err = TRNG_LoadEHR(readbuff);
	TRNG_StopHardware();

	PRCM_ClockDisable(CORE_CKG_CTL_TRNG_EN);

    if (CP_Is_Alive() == true)
    {
        SEMA_Release(SEMA_SLAVE_TRNG, SEMA_MASK_CP);
    }

	return err;
}

//获取随机数种子,正常的使用是先执行srand(xy_seed());再调用rand()
__OPENCPU_FUNC uint32_t xy_seed(void)
{
	uint32_t trng_buffer[EHR_SIZE_IN_WORDS] = {0};
	while(get_trng_rand_value((uint8_t *)trng_buffer) != 0);

	return trng_buffer[0];
}

/*获取min到max的随机值*/
__OPENCPU_FUNC uint32_t get_rand_val(uint32_t min,uint32_t max)
{
	xy_assert(max > min+1);

	/*种子*/
	srand(xy_seed());
	
	return (min + (uint32_t)(rand() % (max-min+1)));
}

uint32_t xy_chksum(const void *dataptr, int32_t len)
{
    uint32_t acc;
    uint16_t src;
    uint8_t *octetptr;

    acc = 0;
    octetptr = (uint8_t*)dataptr;
    while (len > 1) {
        src = (*octetptr) << 8;
        octetptr++;
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0) {
        src = (*octetptr) << 8;
        acc += src;
    }

    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000UL) != 0) {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
	
	if(acc==0 || acc==0XFFFFFFFF)
		acc = 1;
     return ~acc;
}

__OPENCPU_FUNC uint8_t BCD2DEC(uint8_t bcd)
{  
    return (bcd - (bcd >> 4) * 6);
}  
 
__OPENCPU_FUNC uint8_t DEC2BCD(uint8_t dec) 
{  
    return (dec + (dec / 10) * 6);
}


/*
 * @brief   将数字转换成ascii码字符串，例如921600-->"921600"
 * @param   big_endian为0时，表示小端，例如921600-->"006129"
 */
__OPENCPU_FUNC uint8_t int_to_ascii(int num, char *str, char big_endian)
{
	uint8_t len = 0;
	do
	{
		str[len] = num % 10 + '0';
		len++;
		num /= 10;
	}while(num);

	if(big_endian)
	{
		uint8_t i = 0;
		char *temp_str = xy_malloc(len);
		memcpy(temp_str, str, len);
		for(i =0; i < len; i++)
		{
			str[i] = temp_str[len - 1 - i];
		}
	}
	return len;
}


/**
  * @brief   字符串倒序函数
  */
__OPENCPU_FUNC void str_back_order(char *rst,int rstlen)
{
	char restore_rst = 0;

	if((rst == NULL) || (rstlen == 0))
		xy_assert(0);

	for(int i=0;i<(rstlen/2);i++)
	{
		restore_rst = rst[i];
		rst[i] = rst[rstlen-i-1];
		rst[rstlen-i-1] = restore_rst;
	}
}


/**
 * @brief AP核软看门狗超时事件处理函数
 */
__RAM_FUNC void Soft_Watchdog_Timeout()
{
	/*超过规定时间尚未深睡，用户进行软件容错，保存好关键信息后芯片复位。*/
	
	xy_Soc_Reset();
}


/**
 * @brief 软看门狗，通常用于远程通信相关业务异常的容错。时长不得小于半小时
 */
__RAM_FUNC void Soft_Watchdog_Init(uint32_t sec)
{
	xy_assert(sec >= 30*60);
	
	Timer_AddEvent(TIMER_SOFT_WDT,sec*1000, Soft_Watchdog_Timeout, 0);
}

