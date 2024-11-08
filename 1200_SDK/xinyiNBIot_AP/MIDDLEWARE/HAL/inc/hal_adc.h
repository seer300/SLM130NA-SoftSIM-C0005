/**
  ******************************************************************************
  * @file   hal_adc.h
  * @brief  此文件包含ADC外设的变量，枚举，结构体定义，函数声明等.
  * @attention	ADC分辨率为12bit，有效精度请参考芯片规格书。
  * @note ADC通道精度：     
  *   VBAT：±20mV（VBAT=3.6V供电，常温25℃）
  *   ADC2&GPIO8：±10mV（常温25℃）
  *   Tsensor：±5℃（常温25℃）
  * @warning XY1200/2100 只有一个ADC模块，存在双核竟争问题，双核使用时，不建议在AP侧
  *           ADC接口放在中断中使用，这样可能会因为核间硬件锁竞争，导致中断处理时间增加。
  ******************************************************************************
  */

#pragma once

#include "hal_def.h"
#include "adc.h"
#include "prcm.h"
#include "adc_adapt.h"

/*用户使用scan模式同时测量的通道，该值会影响RAM开销，按实际使用设置上限即可*/
#define  ADC_MAX_CHANNELNUM  5	

/**
 * @brief  ADC通道选择枚举，其中:
 * HAL_ADC_GPIO8-13为检测GPIO8-13脚上的电压;
 * HAL_ADC_TEMPRATURE用来检测芯片温度;
 * HAL_ADC_VBAT用来检测片内电压;
 * HAL_AUX_ADC1为检测AUX_ADC1脚上的电压.
 * HAL_AUX_ADC2为检测AUX_ADC2脚上的电压.
 * HAL_AUX_ADC1/HAL_AUX_ADC2在nv参数adcvolrange为2时，量程可到VBAT，但精度较差，其它通道不支持此大量程.
 **/
typedef enum
{
	HAL_ADC_GPIO8 = ADC_CMP_INP,      //channel0
	HAL_ADC_GPIO9 = ADC_CMP_INN,      //channel1  
	HAL_ADC_GPIO10 = ADC_OP1_INP,     //channel3
	HAL_ADC_GPIO11 = ADC_OP1_INN,     //channel4  //仅2100S支持
	HAL_ADC_GPIO12 = ADC_OP1_OUT,     //channel5  //仅2100S支持
	HAL_ADC_GPIO13 = ADC_OP0_OUT,     //channel6
	HAL_ADC_TEMPRATURE = ADC_TSENSOR, //channel7
	HAL_ADC_VBAT = ADC_VBAT,          //channel8
	HAL_ADC_OPA0_INP = ADC_OPA0_INP,	//channel9  //烟感引脚复用为ADC,仅1200S支持
	HAL_ADC_OPA0_INN = ADC_OPA0_INN,	//channel10 //烟感引脚复用为ADC,仅1200S支持
	HAL_ADC_AUX_ADC1 = ADC_AUX_ADC1,  //channel11 //仅2100S支持
	HAL_ADC_AUX_ADC2 = ADC_AUX_ADC2,  //channel12
} HAL_ADC_ChannelTypeDef;

/**
 * @brief  ADC 控制结构体
 **/
typedef struct
{
	HAL_ADC_ChannelTypeDef Channel;		         /*!< ADC single模式通道选择*/
	HAL_LockTypeDef Lock;				               /*!< ADC设备锁*/
} HAL_ADC_HandleTypeDef;

typedef struct 
{
  HAL_ADC_ChannelTypeDef Channel;           /*!< ADC scan模式通道选择*/
  volatile int16_t value;                   /*!< ADC scan模式通道转换值*/
}HAL_ADC_Scan_ChannelTypeDef;

typedef struct
{
	uint8_t Scan_NbrsChn;              /*!< ADC scan模式的通道个数*/
	HAL_ADC_Scan_ChannelTypeDef *Scan_channel;	/*!< ADC scan模式待转换通道*/
	HAL_LockTypeDef Lock;				                /*!< ADC设备锁*/
} HAL_ADC_Scan_HandleTypeDef;

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
int16_t HAL_ADC_GetValue(HAL_ADC_HandleTypeDef *hadc);

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
int16_t HAL_ADC_GetValue_2(HAL_ADC_Scan_HandleTypeDef *hadc);

