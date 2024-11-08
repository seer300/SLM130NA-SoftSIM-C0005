/**
* @file   adc_demo.c
* @note   
*************************************************************************************************************************/
#include "hal_adc.h"
#include "xy_printf.h"
#include "at_uart.h"
#include "hal_def.h"
#include "runtime_dbg.h"

// ADC_TOTAL_CHANNELNUM参与ADC转换的通道数，用户根据具体使用自行定义
HAL_ADC_Scan_ChannelTypeDef ADC_Scan_Channel[ADC_MAX_CHANNELNUM];

/**
 * @brief 	demo主函数。
 * @note	用户使用xy_printf需要将XY_DEBUG宏置为1,详情参考xy_printf.c。
 */
__RAM_FUNC int main(void)
{
	HAL_ADC_Scan_HandleTypeDef adc_handle = {0};

	SystemInit();

	ADC_Scan_Channel[0].Channel = HAL_ADC_TEMPRATURE;
	ADC_Scan_Channel[1].Channel = HAL_ADC_VBAT;
	ADC_Scan_Channel[2].Channel = HAL_ADC_GPIO9;
	ADC_Scan_Channel[3].Channel = HAL_ADC_GPIO10;
	ADC_Scan_Channel[4].Channel = HAL_ADC_AUX_ADC2;

	while (1)
	{
		adc_handle.Scan_NbrsChn = ADC_MAX_CHANNELNUM;
		adc_handle.Scan_channel = ADC_Scan_Channel;	

		Debug_Runtime_Add("ADC Conversion start");
		HAL_ADC_GetValue_2(&adc_handle);
		Debug_Runtime_Add("ADC Conversion stop");						  	

		xy_printf("\r\nTemprature Value: %d\r\n\r\nVbat Value: %d\r\n\r\nHAL_ADC_GPIO9 Voltage Value: %d\r\n\
							\r\nHAL_ADC_GPIO10 Voltage Value: %d\r\n\r\nHAL_ADC_AUX_ADC2 Voltage Value: %d\r\n",
							adc_handle.Scan_channel[0].value,adc_handle.Scan_channel[1].value,adc_handle.Scan_channel[2].value,\
							adc_handle.Scan_channel[3].value,adc_handle.Scan_channel[4].value);
	
		HAL_Delay(1000);
	}

	return 0;
}
