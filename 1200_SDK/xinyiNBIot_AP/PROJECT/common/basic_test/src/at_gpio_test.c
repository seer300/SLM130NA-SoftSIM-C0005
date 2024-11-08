#include <string.h>
#include "at_uart.h"
#include "xy_timer.h"
#include "at_cmd_regist.h"
#include "xy_memmap.h"
#include "system.h"
#include "xy_system.h"
#include "xy_utils.h"


int g_test_type_bitmap = 0;//对应val1，bit0:wakup_pin;bit1-3:AGPI1-3;bit4-12:GPIO0-7
int g_io_mode = -1;//对应val2，0：中断模式；1：普通GPIO模式
int g_api_mode = -1;//对应val3，使用的接口，0：xy_wakeup_pin.h；1：mcu_adapt.h
/*对应val4. bit0~bit1：AGPI1 bit2~bit3：AGPI2 bit4~bit5：AGPI3
			bit6~bit7：GPI0 bit8~bit9：GPI1 bit10~bit11：GPI2 bit12~bit13：GPI3 bit14~bit15：GPI4 bit16~bit17：GPI5 bit18~bit19：GPI6 bit20~bit21：GPI7
			bit22~bit23：WKUP_EN
对应的2个bit为0x00：双边沿触发中断；0x01：上升沿触发中断：0x02：下降沿触发中断*/
int g_edge_trigger_mode = -1;

int gpio_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5,uint32_t val6)
{
	UNUSED_ARG(val5);
	UNUSED_ARG(val6);

	g_test_type_bitmap = val1;
	g_io_mode = val2;
	g_api_mode = val3;
	g_edge_trigger_mode = val4;

	return XY_OK;
}

