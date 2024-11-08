/*******************************************************************************
* @Copyright (c)    :(C)2020, Qingdao ieslab Co., Ltd
* @FileName         :hc32_adc_driver.h
* @Author           :Kv-L
* @Version          :V1.0
* @Date             :2020年7月1日 17:57:05
* @Description      :电压转换范围：0~2.5V;  最大连续转换次数：80次
*******************************************************************************/

/*********************************VREFINT***************************************
* typ voltage :1.212V
* Min ADC sampling time : 4us
*
*******************************************************************************/
/*************************external channels*************************************
* 采样电容：CADC = 5pF；
* 允许最大外部输入电阻：RAIN = 50KΩ；
* 内部电阻：RADC = ？；
* 取60RC = 15us经测试足够给采样电容充满电。
* 外部输入通道以最小采样时间15us计算。
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __U_ADC_DRIVER_H
#define __U_ADC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "xy_system.h"
#include "type.h"	
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/*ADCs to use*/

/*Main machine states*/
#define ADC_IDLE            0
#define ADC_CFGADC			1
#define ADC_START			2
#define ADC_CONV			3
#define ADC_END				4

/*states*/
#define ADC_OK              0x00
#define ADC_START_ERROR     0x01
#define ADC_CONV_TIMEOUT	0x02

/*convert result array*/
#define MIN     0
#define SUM     1
#define MAX     2
#define MINCONVNUM	3
/*Max conv num*/
#define MAXCONVNUM  80  //max:86!
/*adc_timeout*/
#define  ADC_TIMEOUT  100  //单位：us
/* Exported functions ------------------------------------------------------- */
void AdcStartConvert( u8 AdcChannel, u32 ConvNum, u32 TimeInterval, u32 *ConvResult, u8 *ConvIfDone);
void AdcMachineDriver(void);
u8 AdcIfIdle(void);
u8 AdcGetStatus(void);
u8 AdcIfSleep(void);
void AdcPreSleep(void);
void AdcWakeSleep(void);
#ifdef __cplusplus
}
#endif

#endif /* __U_ADC_DRIVER_H */

/***************************************************************END OF FILE****/
