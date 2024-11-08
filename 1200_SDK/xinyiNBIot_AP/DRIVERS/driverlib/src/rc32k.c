#include "stdlib.h"
#include "uart.h"
#include "gpio.h"
#include "prcm.h"
#include "common.h"
#include "rc32k.h"
#include "mcnt.h"
#include "sys_clk.h"
#include "nvic.h"
#include "xy_list.h"
#include "xy_system.h"

// 重启RC32K Ctrl模块，寄存器恢复初始状态
void RC32K_Ctrl_Reset(void)
{
	PRCM_RC32K_CTRL_PWR_Ctl(RC32K_CTRL_ANY_MODE_OFF);
    utc_cnt_delay(6);   //3个aonsys cycle
	PRCM_RC32K_CTRL_PWR_Ctl(RC32K_CTRL_ANY_MODE_ON);
    utc_cnt_delay(6);   //3个aonsys cycle+2 pclk cycle
}

void PRCM_SelectXtal32k(void)
{
	if (!(AONPRCM->AONCLK_FLAG & 0x2))
	{
		AONPRCM->AONCLK_CTRL |= 0x400;
		while(!(AONPRCM->AONCLK_FLAG & 0x100))
			;
		AONPRCM->AONCLK_CTRL |= 0x4;
		while(!(AONPRCM->AONCLK_FLAG & 0x2))
			;
	}
}

void PRCM_SelectRC32k(void)
{
	if (!(AONPRCM->AONCLK_FLAG & 0x1))
	{
		AONPRCM->PWRCTL_TEST6 |= 0x60;
		AONPRCM->AONCLK_CTRL &= 0xFB;
		while(!(AONPRCM->AONCLK_FLAG & 0x1));
	}

	// 强制将Aon寄存器中RC相关的寄存器清零,避免状态异常!
	HWREG(0x40000070) = 0x200;
	HWREG(0x40000074) = 0x80000A;
	HWREG(0x40000078) = 0;
	HWREG(0x4000007c) = 0x6;
	HWREG(0x40000080) = 0;
	HWREG(0x40000084) = 0;
	HWREG(0x40000088) = 0;
	HWREG(0x4000008c) = 0;
	HWREG(0x40000090) = 0;

	for (int i = 0; i <= 90; i++)
	{
		RC32K_CALIB_LOOKUP_TABLE_Set(i, 0, 0);
	}
}

void RC32K_CALIB_WakeUp_Dis(void)
{
	//REG_Bus_Field_Set(AONPRCM_BASE + 0x71,  1,  1, 0x0);
	HWREGB(AONPRCM_BASE + 0x71) = 0;
}

void RC32K_CALIB_WakeUp_En(void)
{
	//REG_Bus_Field_Set(AONPRCM_BASE + 0x71,  1,  1, 0x1);
	HWREGB(AONPRCM_BASE + 0x71) = 0x02;
}


void RC32K_CALIB_En(void)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x70,  0,  0, 0x1);
}

void RC32K_CALIB_EnNow(void)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x70,  1,  1, 0x1);
}

void RC32K_CALIB_DisNow(void)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x70,  1,  1, 0x0);
}

void RC32K_CALIB_Dis(void)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x70,  0,  0, 0x0);
}

void RC32K_CALIB_SDM_En(void)
{
	REG_Bus_Field_Set(AONPRCM_ADIF_BASE,  106,  106,  0x1);
}

void RC32K_CALIB_Done(void)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x84,  0,  0, 0x1);
}


void RC32K_CALIB_TCOUNT_Set(uint8_t tCountVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x74,  7,  0, tCountVal);
}

void RC32K_CALIB_CAPSEL_INIT_Set(uint8_t capSelInitVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x76,  7,  0, capSelInitVal);
}

void RC32K_CALIB_OSC_REF_Set(uint16_t oscRefVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x78,  11,  0, oscRefVal);
}

void RC32K_CALIB_OSC_CAL_Set(uint16_t oscCalVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x7A,  11,  0, oscCalVal);
}

void RC32K_CALIB_LPTS_RESO_Set(uint8_t lptsResoVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x7C,  7,  0, lptsResoVal);
}

void RC32K_CALIB_NV_Save(uint8_t capSelVal, uint8_t sdmDataVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x80,  7,  0, capSelVal);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x81,  6,  0, sdmDataVal);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x81,  7,  7, 0x1);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,  1,  0, 0x3);    //force_capsel_reg_en   and force_sdm_out_en

}

void RC32K_CALIB_LOOKUP_TABLE_Set(uint8_t tempAddr, uint8_t capSelVal, uint8_t sdmDataVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x7F,	7,	0, tempAddr);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x80,  7,  0, capSelVal);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x81,  7,  0, sdmDataVal);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x88,  0,  0, 0x1);    //force temp we	
}

void RC32K_CALIB_LOOKUP_TABLE_Get(uint8_t tempAddr, uint8_t *capSelVal, uint8_t *sdmDataVal)
{
	REG_Bus_Field_Set(AONPRCM_BASE + 0x7F,	7,	0, tempAddr);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x89,  0,  0, 0x1);    //force temp re
	prcm_delay(10);
	
	*capSelVal  = HWREGB(AONPRCM_BASE + 0x8C);
	*sdmDataVal = HWREGB(AONPRCM_BASE + 0x8D);
}

uint8_t RC32K_CALIB_CAPSEL_GetCur(void)
{
	return HWREGB(AONPRCM_BASE + 0x8C);
}

uint8_t RC32K_CALIB_SDMDATA_GetCur(void)
{
	return HWREGB(AONPRCM_BASE + 0x8D);
}

unsigned short RC32K_CALIB_OSC_PTAT_Get(void)
{
	short oscPtatVal = 0;
	oscPtatVal = HWREGH(AONPRCM_BASE + 0x86) & 0xFFF;
	return oscPtatVal;
}

uint8_t RC32K_CALIB_TEMP_is_VALID(void)
{
	return ((HWREGB(AONPRCM_BASE + 0x8E) & 0x80) >> 7);
}

void RC32K_CALIB_TEMP_VALID_Clr(void)
{
	HWREGB(AONPRCM_BASE + 0x8F) |= 0x1;
}

short RC32K_CALIB_TEMP_VAL_Get(void)
{
	return (HWREGB(AONPRCM_BASE + 0x8E) & 0x7F);
}

void RC32K_CALIB_FORCE_SDM_OUT_DATA(uint8_t sdmOutVal)
{
	HWREGB(AONPRCM_BASE + 0x83) |= 0x1;
	HWREGB(AONPRCM_BASE + 0x83) = (HWREGB(AONPRCM_BASE + 0x83) & 0xFD) | (sdmOutVal << 1) ;
}

uint8_t rc32k_get_last_temperature(void)
{
	volatile int8_t temperaVal ;
	volatile char temp_delay;
	do{
		temperaVal  = RC32K_CALIB_TEMP_VAL_Get();
		for( temp_delay = 0; temp_delay < 10 ; temp_delay++);
	}while(temperaVal != RC32K_CALIB_TEMP_VAL_Get());

	return temperaVal;
}

extern volatile uint32_t g_rc32k_cali_flag;
int8_t rc32k_get_temperature(uint16_t oscCalVal, uint8_t lptsResoVal, uint8_t init_pro_flag)
{
	static uint8_t delay = 0;
	static int8_t temperaVal = 37; 

	RC32K_CALIB_Done();

	RC32K_CALIB_OSC_CAL_Set(oscCalVal);	
	RC32K_CALIB_OSC_REF_Set(2000);
	RC32K_CALIB_LPTS_RESO_Set(lptsResoVal);

	if(init_pro_flag == 1)
	{
		//初始化阶段，需确保得到真实的实时温度（防止dis en之后now模式仍需持续40几个cnt才能结束，造成产生的中断被误清，后续SOC工作态时RC模块处于校准挂起状态）
		//校准后，为了降低校准次数，此处不clear valid flag（校准前后温度检查可能失效）
		RC32K_CALIB_TEMP_VALID_Clr();
	}

retry:
	RC32K_CALIB_EnNow();
	RC32K_CALIB_En();

	utc_cnt_delay(2);   //延时2~3个utc_clk
	RC32K_CALIB_DisNow();

	/*最多延迟4ms，如果查询失败，执行容错*/
	while(!RC32K_CALIB_TEMP_is_VALID())
	{
		if(delay < 400)
		{
			delay++;
			delay_func_us(10);
		}
		else
		{
			delay = 0;

			RC32K_CALIB_Dis();
			// 重启RC32K Ctrl模块
			RC32K_Ctrl_Reset(); 

			// 清除dis en前可能产生的校准信号
			g_rc32k_cali_flag = 0;

			goto retry;
		}
	}

	delay = 0;

	temperaVal = RC32K_CALIB_TEMP_VAL_Get();
	
	RC32K_CALIB_TEMP_VALID_Clr();

	RC32K_CALIB_TCOUNT_Set(1);
	RC32K_CALIB_En();

	return temperaVal;
}




















