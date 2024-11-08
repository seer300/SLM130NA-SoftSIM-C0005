/**
* @file   adc_demo.c
* @note   在这个DEMO中，每隔一秒使用ADC先获取芯片温度，VBAT,再获取HAL_ADC_GPIO9脚上的电压值，并将获取到的结果通过串口打印出来。
*
*************************************************************************************************************************/
#include "hal_adc.h"
#include "xy_printf.h"
#include "at_uart.h"
#include "hal_def.h"
#include "runtime_dbg.h"

/**
 * @brief 	demo主函数。
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c。
 */
__RAM_FUNC int main(void)
{
	int16_t voltage9,temprature,vbat,voltageADC2;
	HAL_ADC_HandleTypeDef adc_single = {0};

	SystemInit();

	Debug_Runtime_Init();

	while (1)
	{

		Debug_Runtime_Add("ADC voltage9 start");	
		adc_single.Channel= HAL_ADC_GPIO9;	
		voltage9 = HAL_ADC_GetValue(&adc_single);
		Debug_Runtime_Add("ADC voltage9 stop temprature start");  //根据此标签可以获取GPIO9通道的转换时间
		adc_single.Channel= HAL_ADC_TEMPRATURE;
		temprature = HAL_ADC_GetValue(&adc_single);
		Debug_Runtime_Add("ADC temprature stop ADC vbat start");  //根据此标签可以获取TEMPRATURE通道的转换时间		
		adc_single.Channel= HAL_ADC_VBAT;
		vbat = HAL_ADC_GetValue(&adc_single);
		Debug_Runtime_Add("ADC vbat stop ADC2 start");						  //根据此标签可以获取VBAT通道的转换时间	
		adc_single.Channel= HAL_ADC_AUX_ADC2;
		voltageADC2 = HAL_ADC_GetValue(&adc_single);
		Debug_Runtime_Add("ADC ADC2 stop");						  //根据此标签可以获取VBAT通道的转换时间		

		xy_printf("\r\nTemprature Value: %d\r\n\r\nVbat Value: %d\r\n\
			\r\nADC_GPIO9 Voltage Value: %d\r\n\r\nADC_ADC2 Voltage Value: %d\r\n",\
			temprature,vbat, voltage9,voltageADC2);
	
		HAL_Delay(1000);
	}

	return 0;  
}

