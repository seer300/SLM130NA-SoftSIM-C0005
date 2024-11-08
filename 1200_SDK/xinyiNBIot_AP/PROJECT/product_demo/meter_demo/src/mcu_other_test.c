#include <string.h>
#include "xy_system.h"
#include "xy_timer.h"
#include "ap_watchdog.h"
#include "hal_adc.h"
#include "xy_printf.h"
#include "basic_config.h"
#include "at_uart.h"
#include "user_config.h"
#include "xy_event.h"
#include "mcu_adapt.h"

/************************************************************************************************************
***  重点测试mcu_gpio.c/hal_adc.h/xy_wakeup_pin.h中的接口耗时、功耗开销、稳定性、启停CP核的健壮性等
*************************************************************************************************************/

__RAM_FUNC void LPTIMER_ADC_Callback(void)
{
    HAL_ADC_Scan_ChannelTypeDef ADC_Scan_Channel[1];
	ADC_Scan_Channel[0].Channel = HAL_ADC_VBAT;

    HAL_ADC_Scan_HandleTypeDef adc_handle;
    adc_handle.Scan_NbrsChn = 1;
    adc_handle.Scan_channel = ADC_Scan_Channel;	
    HAL_ADC_GetValue_2(&adc_handle);
	
    char debug_info[50];
    snprintf(debug_info, 50, "\r\n(lptim_irq) VBATvalue: %d\r\n", adc_handle.Scan_channel[0].value);
	Send_AT_to_Ext(debug_info);
}


void ADC_test_set(uint32_t val1,uint32_t val2,uint32_t val3)
{
    //AT+DEMOCFG = 1 //scan与single模式ADC采集，采集VBAT与Tsensor
    if(val1 == 1)
    {
        HAL_ADC_Scan_HandleTypeDef adc_handle2 = {0};
        HAL_ADC_Scan_ChannelTypeDef ADC_Scan_Channel[ADC_MAX_CHANNELNUM];

        ADC_Scan_Channel[0].Channel = HAL_ADC_TEMPRATURE;
        ADC_Scan_Channel[1].Channel = HAL_ADC_VBAT;
        
        adc_handle2.Scan_NbrsChn = 2;
        adc_handle2.Scan_channel = ADC_Scan_Channel;	

        HAL_ADC_GetValue_2(&adc_handle2); 	

        xy_printf("\r\nHAL_ADC_GetValue_2 vol= %d,Tem= %d\r\n",
                            adc_handle2.Scan_channel[1].value,adc_handle2.Scan_channel[0].value);     

		int32_t tempratureinAP = 0;
	    int32_t voltageinAP = 0;
	    HAL_ADC_HandleTypeDef adc_handle = {0};
	        
	    adc_handle.Channel = HAL_ADC_TEMPRATURE;
	    tempratureinAP = HAL_ADC_GetValue(&adc_handle);

	    adc_handle.Channel = HAL_ADC_VBAT;
	    voltageinAP = HAL_ADC_GetValue(&adc_handle);
		
	    xy_printf("\r\nHAL_ADC_GetValue vol= %d,Tem= %d\r\n", voltageinAP,tempratureinAP);
    }   

    //AT+DEMOCFG = 2,val2,val3 //定时进行ADC采集。val2，定时采集开关，1：开启定时采集，0：关闭定时采集；val3,定时时长（ms）.
    else if(val1 == 2)
    {          
        if(val2 == 0)
        {
            McuLptimerDis(); 
        } 
        else 
        {
            McuLptimerSetPeriod(val3);
            McuLptimerIrqReg(LPTIMER_ADC_Callback);
            McuLptimerEn();
        } 
    }

     //AT+DEMOCFG = 3 // 双核ADC采集.
    else if(val1 == 3)
    {       
        int32_t tempratureinAP = 0;
        int32_t tempraturefromCP = 0;
        int32_t voltageinAP = 0;
        int32_t voltagefromCP = 0;
        HAL_ADC_HandleTypeDef adc_handle = {0};
        At_status_type at_ret1 = XY_OK,at_ret2 = XY_OK;
            
        adc_handle.Channel = HAL_ADC_TEMPRATURE;
        tempratureinAP = HAL_ADC_GetValue(&adc_handle);

        adc_handle.Channel = HAL_ADC_VBAT;
        voltageinAP = HAL_ADC_GetValue(&adc_handle);
        xy_printf("\r\nVBATValue in AP = %d,TempValue in AP= %d\r\n", voltageinAP,tempratureinAP);

        if (CP_Is_Alive() == true) 
        {
            //注意使用此指令时，需要CP侧为对标移远的BC95版本
            at_ret1 = AT_Send_And_Get_Rsp("AT+QCHIPINFO=VBAT\r\n",10, "+QCHIPINFO:", ",%d", &voltagefromCP);
            at_ret2 = AT_Send_And_Get_Rsp("AT+QCHIPINFO=TEMP\r\n",10, "+QCHIPINFO:", ",%d", &tempraturefromCP);
            
            if(at_ret1 == XY_OK && at_ret2 == XY_OK)	
            {
                if( (abs(voltageinAP - voltagefromCP) < 50) && (abs(tempratureinAP - tempraturefromCP) < 5))
                    xy_printf("\r\nVBATValue from CP: %d,TempValue from CP: %d\r\n",voltagefromCP,tempraturefromCP); 
                else 
                    xy_printf("\r\nExcessive difference in ADC collection values\r\n"); //理论上AP与CP获取的采样值是差不多的，如果误差过大，则打出调试信息
            }         
        }          
    }
}


