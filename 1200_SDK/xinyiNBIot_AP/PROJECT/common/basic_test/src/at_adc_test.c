#include "hal_adc.h"
#include "hal_gpio.h"
#include "xy_event.h"
#include "adc_adapt.h"
#include "xy_at_api.h"
#include "at_uart.h"

uint32_t at_adc_test(uint32_t val1,uint32_t val2,uint32_t val3,uint32_t val4,uint32_t val5)
{
	char rsp[50] = {0};

	/*AT+APTEST=ADC,0,<gpio num>  0:GPIO8; 1:GPIO9; 3:GPIO10;4:GPIO11;5:GPIO12;6:GPIO13;7:TSENSOR;8:VBAT;11:ADC1;12:ADC2*/
	if(val1 == 0)
	{
        HAL_ADC_HandleTypeDef adc_handle = {0};
		if(val2 > 12 || val2 == 2 || val2 == 9 || val2 == 10)
		{
			return XY_ERR_PARAM_INVALID;
		}
		else
		{
			adc_handle.Channel= val2;	
		}
		sprintf(rsp, "\r\n+ADC: %d\r\n",HAL_ADC_GetValue(&adc_handle));
		Send_AT_to_Ext(rsp);
	}
	/*AT+APTEST=ADC,1,<channel num>,[<gpio num>[,<gpio num>[,<gpio num>]]]  0:GPIO8; 1:GPIO9; 3:GPIO10;4:GPIO11;5:GPIO12;6:GPIO13;7:TSENSOR;8:VBAT;11:ADC1;12:ADC2*/
	else if(val1 == 1)
	{
		HAL_ADC_Scan_HandleTypeDef adc_handle = {0};
		HAL_ADC_Scan_ChannelTypeDef ADC_SChannel[ADC_MAX_CHANNELNUM]={0};

		ADC_SChannel[0].Channel = val3;
		ADC_SChannel[1].Channel = val4;
		ADC_SChannel[2].Channel = val5;


		adc_handle.Scan_NbrsChn = val2;
		adc_handle.Scan_channel = ADC_SChannel;

		for(uint8_t i = 0; i < val2; i++)
		{
			uint32_t val = ADC_SChannel[i].Channel;
			if(val > 12 || val == 2 || val == 9 || val == 10)
			{
				return XY_ERR_PARAM_INVALID;
			}
		}
		HAL_ADC_GetValue_2(&adc_handle);

		snprintf(rsp, 48, "\r\n+ADCScan: %d,%d,%d\r\n",\
			ADC_SChannel[0].value,ADC_SChannel[1].value,ADC_SChannel[2].value);

		Send_AT_to_Ext(rsp);
	}
	
	else if(val1 == 3)
	{
		if(g_ADCVref == ADC_VREF_VALUE_2P2V)
		{
			Send_AT_to_Ext("\r\nadc refvol is 2.2V!\r\n");
			Sp_Gpio13_2V_En();
		}
		else
		{
			Send_AT_to_Ext("\r\nadc refvol is not 2.2V!\r\n");
		}
	}

	return XY_OK;
}



