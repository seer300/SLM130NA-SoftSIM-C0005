#include "hal_adc.h"


//建议单核情况使用
int16_t Get_Temperature(void)
{
	int16_t temperature = 0;
	HAL_ADC_HandleTypeDef adc_handle = {0};

	adc_handle.Channel= HAL_ADC_TEMPRATURE;
	temperature = HAL_ADC_GetValue(&adc_handle);

	return temperature;
}

int16_t Get_VBAT(void)
{
	int16_t vbat = 0;
	HAL_ADC_HandleTypeDef adc_handle = {0};

	adc_handle.Channel= HAL_ADC_VBAT;
	vbat = HAL_ADC_GetValue(&adc_handle);

	return vbat;
}