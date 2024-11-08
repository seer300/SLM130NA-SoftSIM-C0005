/**
 ******************************************************************************
 * @file   hal_adc.c
 * @brief  此文件包含ADC外设的函数定义等.
 ******************************************************************************
 */
#include "hal_adc.h"

/**
 * @brief  获取ADC单通道的测量值，不推荐使用.
 * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_HandleTypeDef 结构体的指针.详情参考 @ref HAL_ADC_HandleTypeDef.
 * @retval 若返回温度信息，单位为摄氏度；若返回电压信息，单位为mV；若出错返回HAL_ADC_RET_INVALID.
 * @attention 1.由于接口内部对不同类型通道处理有所不同，在HRC时钟1分频时，且代码位于RAM上时，实测耗时如下，单位us
 * 				GPIO9      TEMP      VBAT     ADC2
 *              134.3      135.8     86.9     88.0
 * 			  2.由于接口内部对不同类型通道处理有所不同，在HRC时钟2分频时，且代码位于RAM上时，实测耗时如下，单位us
 * 				GPIO9      TEMP      VBAT     ADC2
 *              153.7      161.0    131.0    133.2
 * 
 * 			  3.不同硬件HRC略有差异，故具体时间略有差异
 *			  4.ADC接口耗时长且不能防重入，不能在中断中使用. 
 **/
__RAM_FUNC int16_t HAL_ADC_GetValue(HAL_ADC_HandleTypeDef *hadc)
{
	
	if(hadc == NULL)
	{
		return -32768;
	}

	__HAL_LOCK(hadc);

    //获取指定单通道ADC的采样值
    int16_t value = ADC_Single_GetValue(hadc->Channel);

    __HAL_UNLOCK(hadc);

	return value;
}


/**
 * @brief  使用adc scan 模式获取ADC多通道的转换值，推荐客户使用.
 * @param  hadc 指向一个包含ADC具体配置信息的 HAL_ADC_Scan_HandleTypeDef 结构体的指针.详情参考 @ref HAL_ADC_Scan_HandleTypeDef。
 * @return 0:成功，-32768:失败
 *         返回值可能是以下类型：
 * @note   ADC通道精度：
 *	           VBAT：±20mV（VBAT=3.6V供电，常温25℃）
 *	           ADC2&GPIO8：±10mV（常温25℃）
 *	           Tsensor：±5℃（常温25℃）
 * @attention 1.在HRC时钟1分频时(主频约26M)，且代码位于RAM上时,不同通道数情况下，ADC各个步骤的耗时说明,测试硬件实际主频为6560798Hz，单位（us）：
 * 			通道数		1 				2 				3 						4 								5 						
 * 					(Tempera)  	(Tempera/VBAT) (Tempera/VBAT/GPIO9) (Tempera/VBAT/GPIO9/GPIO10)  (Tempera/VBAT/GPIO9/GPIO10/ADC2)
 *			总计 	  140.6			  174.4			  213.9			   		  254.9						      293.4
 *			
 *			2.在HRC时钟2分频时(主频约13M)，且代码位于RAM上时,不同通道数情况下，ADC各个步骤的耗时说明,测试硬件实际主频为6560798Hz，单位（us）：
 * 			通道数		1 				2 				3 						4 								5 						
 * 					(Tempera)  	(Tempera/VBAT) (Tempera/VBAT/GPIO9) (Tempera/VBAT/GPIO9/GPIO10)  (Tempera/VBAT/GPIO9/GPIO10/ADC2)
 *			总计 	  229.9			  270.9			  319.2			   		  379.4						      425.7
 *
 *			3.不同硬件HRC略有差异，故具体时间略有差异
 *			4.ADC接口耗时长且不能防重入，不能在中断中使用. 
 */
__RAM_FUNC int16_t HAL_ADC_GetValue_2(HAL_ADC_Scan_HandleTypeDef *hadc)
{
	if(hadc == NULL)
	{
		return -32768;
	}

	__HAL_LOCK(hadc);

    uint8_t i;

    //复制通道个数、通道号
    ADC_Scan_HandleTypeDef AdcScan;
    AdcScan.ScanNum = hadc->Scan_NbrsChn; //复制通道个数
    for(i = 0; i < hadc->Scan_NbrsChn; i++) //复制通道号
    {
        AdcScan.Channel[i] = hadc->Scan_channel[i].Channel;
    }

    //获取指定多通道的采样值
    ADC_Scan_GetValue(&AdcScan);

    //赋值采样结果
    for(i = 0; i < hadc->Scan_NbrsChn; i++)
    {
        hadc->Scan_channel[i].value = AdcScan.Value[i];
    }

    __HAL_UNLOCK(hadc);
	
	return 0;
}


