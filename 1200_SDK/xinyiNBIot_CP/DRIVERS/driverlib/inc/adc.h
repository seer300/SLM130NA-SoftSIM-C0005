#ifndef __ADC_H__
#define __ADC_H__
#include "hw_adc.h"
extern uint8_t g_ADCVref;              /*!< ADC Vref value : g_ADCVref / 10 */
extern volatile uint8_t g_CalibrationFlag;
extern volatile uint32_t g_trxbg_CaliReg;
typedef struct
{
	unsigned int ADCFlagStart;

	uint16_t code_in_ADC1_375;		    //channelID:11
	uint16_t code_in_ADC1_1125;		    //channelID:11
	uint16_t code_in_ADC2_375;		    //channelID:12
	uint16_t code_in_ADC2_1125;		    //channelID:12
	uint16_t code_in_GPIO8_375;		    //GPIO8_CMP_INP ADC3
	uint16_t code_in_GPIO8_1125;	    //GPIO8_CMP_INP ADC3
	uint16_t code_in_OPA0_375;          //only OPA0 enable,input:opa0_inp,output:OPA0_OUT, GPIO13,ADC4,
	uint16_t code_in_OPA0_1125;         //only OPA0 enable,input:opa0_inp,output:OPA0_OUT, GPIO13,ADC4,
	uint16_t code_in_OPA1_375;		    //only OPA1 enable,input:opa1_inp,output:OPA1_OUT, GPIO12,ADC5,
	uint16_t code_in_OPA1_1125;		    //only OPA1 enable,input:opa1_inp,output:OPA1_OUT, GPIO12,ADC5,
	uint16_t code_in_OPA0_OPA1_375; 	//OPA0 + OPA1 enable,input:opa0_inp,output:OPA1_OUT0,ADC6,
	uint16_t code_in_OPA0_OPA1_1125;    //OPA0 + OPA1 enable,input:opa0_inp,output:OPA1_OUT0,ADC6,
	uint16_t code_Vbat_3020;
	uint16_t code_Vbat_4670;
	uint16_t code_tsensor;              //记录0  摄氏度的code
	uint16_t isink_ituren_ctrl;
	uint16_t code_in_ADC2H_1125;		    //channelID:12
	uint16_t code_in_ADC2H_3375;		    //channelID:12
	uint16_t reserved[10];
	unsigned int ADCFlagEnd;
}adc_cali_nv_t;

extern  adc_cali_nv_t g_adc_cali_nv;

typedef struct 
{
	uint32_t  adc2_gainerror;
	uint32_t  adc2_offset;
	uint32_t  gpio8_gainerror;
	uint32_t  gpio8_offset;
	uint32_t  opa0_gainerror;
	uint32_t  opa0_offset;
	uint32_t  opa1_gainerror;
	uint32_t  opa1_offset;
	uint32_t  opa0_opa1_gainerror;
	uint32_t  opa0_opa1_offset;
	uint32_t  vbata_gainerror;
	uint32_t  vbata_offset; 
	uint32_t  adc2_high_gainerror;
	uint32_t  adc2_high_offset;
}adc_cali_data_result;

extern adc_cali_data_result g_adc_cali_result;
#define ADC_IDEAL_CODE_375			1024		// in 50  mV
#define ADC_IDEAL_CODE_1125			3072		// in 200 mV

#define ADC2H15_IDEAL_CODE_1125			768		// in 1.125mV
#define ADC2H15_IDEAL_CODE_3375			2304	// in 3.375mV

#define ADC2H22_IDEAL_CODE_1125			524		// in 1.125mV
#define ADC2H22_IDEAL_CODE_3375			1571	// in 3.375mV

#define ADC_EXPAND_MULTIPLE	100000		// the max num is 10^5

#define ADC1_gainerror		(204800000 / (g_adc_cali_nv.code_in_ADC1_1125 - g_adc_cali_nv.code_in_ADC1_375))
#define ADC1_offset			(102400000 - (ADC1_gainerror * g_adc_cali_nv.code_in_ADC1_375))

//#define ADC2_gainerror		((ADC_IDEAL_CODE_1125 - ADC_IDEAL_CODE_375)  * ADC_EXPAND_MULTIPLE / (g_adc_cali_nv.code_in_ADC2_1125 - g_adc_cali_nv.code_in_ADC2_375))
//#define ADC2_offset			((ADC_IDEAL_CODE_375  * ADC_EXPAND_MULTIPLE) - (ADC2_gainerror * g_adc_cali_nv.code_in_ADC2_375))

#define ADC2_gainerror		(204800000 / (g_adc_cali_nv.code_in_ADC2_1125 - g_adc_cali_nv.code_in_ADC2_375))
#define ADC2_offset			(102400000 - (ADC2_gainerror * g_adc_cali_nv.code_in_ADC2_375))

#define ADC2H15_gainerror	(153600000 / (g_adc_cali_nv.code_in_ADC2H_3375 - g_adc_cali_nv.code_in_ADC2H_1125))
#define ADC2H15_offset		(76800000 - (ADC2H15_gainerror * g_adc_cali_nv.code_in_ADC2H_1125))

#define ADC2H22_gainerror	(104700000 / (g_adc_cali_nv.code_in_ADC2H_3375 - g_adc_cali_nv.code_in_ADC2H_1125))
#define ADC2H22_offset		(52400000 - (ADC2H22_gainerror * g_adc_cali_nv.code_in_ADC2H_1125))

#define GPIO8_gainerror		(204800000 / (g_adc_cali_nv.code_in_GPIO8_1125 - g_adc_cali_nv.code_in_GPIO8_375))
#define GPIO8_offset		(102400000 - (GPIO8_gainerror * g_adc_cali_nv.code_in_GPIO8_375))

#define OPA0_gainerror		(204800000 / (g_adc_cali_nv.code_in_OPA0_1125 - g_adc_cali_nv.code_in_OPA0_375))
#define OPA0_offset			(102400000 - (OPA0_gainerror * g_adc_cali_nv.code_in_OPA0_375))

#define OPA1_gainerror		(204800000 / (g_adc_cali_nv.code_in_OPA1_1125 - g_adc_cali_nv.code_in_OPA1_375))
#define OPA1_offset			(102400000 - (OPA1_gainerror * g_adc_cali_nv.code_in_OPA1_375))

#define OPA0_OPA1_gainerror		(204800000 / (g_adc_cali_nv.code_in_OPA0_OPA1_1125 - g_adc_cali_nv.code_in_OPA0_OPA1_375))
#define OPA0_OPA1_offset			(102400000 - (OPA0_OPA1_gainerror * g_adc_cali_nv.code_in_OPA0_OPA1_375))

#define ADC_IDEAL_VBAT15_3020			1649		// 3.02V
#define ADC_IDEAL_VBAT15_4670			2550		// 4.67V

#define VBAT15_gainerror		(90100000 / (g_adc_cali_nv.code_Vbat_4670 - g_adc_cali_nv.code_Vbat_3020))
#define VBAT15_offset			(164900000 - (VBAT15_gainerror * g_adc_cali_nv.code_Vbat_3020))

#define ADC_IDEAL_VBAT22_3020			1124		// 3.02V   Vref = 2.2V
#define ADC_IDEAL_VBAT22_4670			1739		// 4.67V   Vref = 2.2V

#define VBAT22_gainerror		(61500000 / (g_adc_cali_nv.code_Vbat_4670 - g_adc_cali_nv.code_Vbat_3020))
#define VBAT22_offset			(112400000 - (VBAT22_gainerror * g_adc_cali_nv.code_Vbat_3020))

#define ADC_CLK_960K	960
#define ADC_CLK_500K    500
#define ADC_CLK_480K	480
#define ADC_CLK_240K	240
#define ADC_CLK_120K	120       

#define ADC_VREF_VALUE_1P3V    13UL    //Vref = 1.3V
#define ADC_VREF_VALUE_1P5V    15L     //Vref = 1.5V
#define ADC_VREF_VALUE_2P2V    22UL    //Vref = 2.2V
#define ADC_VREF_VALUE_3P2V    32UL    //Vref = 3.2V
typedef enum
{
	ADC_CMP_INP = 0,
	ADC_CMP_INN,    //1
	ADC_CMP_OUT,	//2	
	ADC_OP1_INP,	//3
	ADC_OP1_INN,	//4
	ADC_OP1_OUT,	//5   GPIO12
	ADC_OP0_OUT,	//6   GPIO13
	ADC_TSENSOR,	//7
	ADC_VBAT,		//8
	ADC_OPA0_INP,	//9
	ADC_OPA0_INN,	//10
	ADC_AUX_ADC1,	//11
	ADC_AUX_ADC2,	//12
}ADC_CHANNEL_TypeDef;

typedef enum
{
	ADC_AVE_NORMAL = 0,
	ADC_AVE_SLIDING,
}ADC_AVE_ModeDef;


typedef enum
{

	IS_IOCLK_32K = 1,
	IS_IOCLK_HRC = 2,
	IS_IOCLK_XTAL = 4
}IS_IOCLK_FLAG;

typedef enum
{
    SMOKE_TYPE = 0,        // used for smoke,input:opa0_inp,output:opa1_out
    OPA_ONLY_TYPE =1,      // only enable OPA0 or OPA1,for OPA0,input:opa0_inp,output:opa0_out,for OPA1,input:opa1_inp,output:opa1_out

}ADC_CALI_TYPE;

typedef enum
{
	MCLK_6500K = 6500,
	MCLK_13000K = 13000, 
	MCLK_26000K = 26000
}MCLK;

typedef enum 
{
	ADC_CLK120 = 120,
	ADC_CLK240 = 240, 
	ADC_CLK480 = 480, 
	ADC_CLK960 = 960, 
}ADC_CLK;



typedef struct 
{
  uint8_t valid;
  int16_t value;
}ADC_Channel_Value;



/**
  * @brief Power up temperature monitor.
  * @retval None
*/
static inline void ADC_TsensorPowerEN(void)
{
	HWREGB(0x40004852) |= 0x08; // tsensor mon enable
}

/**
  * @brief Power off temperature monitor.
  * @retval None
*/
static inline void ADC_TsensorPowerDIS(void)
{
	HWREGB(0x40004852) &= 0xF7; // tsensor mon disable
}

/**
  * @brief Power up battery monitor.
  * @retval None
*/
static inline void ADC_VbatPowerEN(void)
{
	HWREGB(0x40004852) |= 0x10; // vbat mon enable
}

/**
  * @brief Power off battery monitor.
  * @retval None
*/
static inline void ADC_VbatPowerDIS(void)
{
	HWREGB(0x40004852) &= 0xEF; // vbat mon disable
}

/**
  * @brief ADC VREF SET .
  * @param vadj: Can be selected as the following :
  *    @arg 0: 1.3V.
  *    @arg 1: 1.5V(recommended).
  *    @arg 2: 2.2V(for the Golden Card Project)
  *    @arg 3: 3.2V.
  * @retval None
*/
static inline void ADC_VREF_Set(uint8_t vadj)
{
	/*00: 1.3V    01: 1.5V   10: 2.2V   11: 3.2V */
	HWREGB(0x40004846) = (HWREGB(0x40004846) & 0xF3) | (vadj << 2);
}


/**
  * @brief Get ADC VREF .
  * @param None
  * @retval vadj: Can be one of the following :
  *    @arg 0: 1.3V.
  *    @arg 1: 1.5V(recommended).
  *    @arg 2: 2.2V(for the Golden Card Project)
  *    @arg 3: 3.2V.
*/
static inline uint8_t ADC_VREF_Get(void)
{
	/*00: 1.3V    01: 1.5V   10: 2.2V   11: 3.2V */
	return ((HWREGB(0x40004846) & 0x0c)  >> 2);
}

/**
  * @brief Disable OPA0 .
  * @note  GPIO13 will not output 2.2V when OPA0 disabled.
  * @retval None
*/
static inline void ADC_OPA0_Disable(void)
{
	HWREGB(0x40004855 ) &= 0xFE;     //opa0 disable
}

/**
  * @brief Set LDOANA 2.7V.
  * @note  when Vref is 1.5V or Vref is 2.2V, the LDOANA should be set as 2.7V  .
  * @retval None
*/
static inline void ADC_Set_Ldoana_2p7v(void)
{
    HWREGB(0x4000484C) = 0x0E;  //LDOANA = 2.7v

}

/**
  * @brief Set TRXBG calibration value.
  * @param  CaliValue: This value should be read out from OTP.
  * @note   If the calibration value is within the valid range, the TRXBG calibration register needs to be set after power-on.
  * @retval None
*/
static inline void ADC_Trxbg_Set_CaliValue(uint8_t CaliValue)
{
	HWREGB(COREPRCM_ADIF_BASE + 0x4E) = CaliValue & 0x1F;
	HWREGB(COREPRCM_ADIF_BASE + 0x4D)  &= 0xF0;
}

/**
  * @brief Get TRXBG calibration value.
  * @param CaliValue: This value should be read out from OTP.
  * @note  If the calibration value is within the valid range, the TRXBG calibration register needs to be set after power-on.
  * @retval None
*/
static inline uint8_t ADC_Trxbg_Get_CaliValue(void)
{
	return(HWREGB(COREPRCM_ADIF_BASE + 0x4E) & 0x1F);
}



#define ADC_ADC1_ATTENU_EN       1
#define ADC_ADC2_ATTENU_EN       2
#define ADC_ADC1_ADC2_ATTENU_EN  3
/**
  * @brief Enable ADC1 or ADC2 attenuation.
  * @param channel Should be one of the below
  *        @arg  ADC_ADC1_ATTENU_EN
  *        @arg  ADC_ADC2_ATTENU_EN
  *        @arg  ADC_ADC1_ADC2_ATTENU_EN
  * @retval None
*/
static inline void ADC_Enable_Attenu(void )
{
	HWREGB(COREPRCM_ADIF_BASE + 0x46) |= ADC_ADC1_ADC2_ATTENU_EN;
}

/**
  * @brief Disable ADC1 or ADC2 attenuation.
  * @param channel Should be one of the below
  *        @arg  ADC_ADC1_ATTENU_EN
  *        @arg  ADC_ADC2_ATTENU_EN
  *        @arg  ADC_ADC1_ADC2_ATTENU_EN
  * @retval None
*/
static inline void ADC_Disable_Attenu(void)
{
	HWREGB(COREPRCM_ADIF_BASE + 0x46) &= 0xFC;
}

/**
  * @brief Get ADC1 or ADC2 attenuation status.
  * @param channel Should be one of the below
  *        @arg  ADC_ADC1_ATTENU_EN
  *        @arg  ADC_ADC2_ATTENU_EN
  *        @arg  ADC_ADC1_ADC2_ATTENU_EN
  * @retval None
*/
static inline uint8_t ADC_Get_Attenu(void)
{
	return (((HWREGB(COREPRCM_ADIF_BASE + 0x46) & ADC_ADC1_ADC2_ATTENU_EN) == ADC_ADC1_ADC2_ATTENU_EN) ? 1:0);
}

/**
  * @brief Enable TRXBG.
  * @retval None
*/
static inline void ADC_Trxbg_On(void)
{
	COREPRCM->ANATRXBG_CTL = 0x01;
}

/**
  * @brief Enable TRXBG.
  * @retval None
*/
static inline void ADC_Trxbg_Off(void)
{
	COREPRCM->ANATRXBG_CTL = 0;
}

/**
  * @brief Check if TRXBG is ready .
  * @retval None
*/
static inline uint8_t ADC_IsActively_Trxbg(void)
{
	return ((COREPRCM->ANATRXBG_STAT & 0x01 != 0) ? 1 : 0);
}

void ADCCTRL_DmaModeEn(void);
void ADCCTRL_DmaModeDis(void);
int16_t adcDataExtend(int16_t data);
int32_t ADCCTRL_AddScanChannel(uint8_t scanChannelID, uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num, uint8_t scan_num);
void ADCCTRL_ScanModeConfig(uint8_t turnNum);
void ADCCTRL_FIFOUpThrConfig(uint8_t up_thr);
void ADCCTRL_ScanModeSuspendEn(void);
void ADCCTRL_ScanModeSuspendDis(void);
void ADCCTRL_FIFOFlush(void);
void ADC_Start(void);
void ADC_Stop(void);
void ADCCTRL_INTEn(uint8_t adcIntFlag);
void ADCCTRL_ClearINTStatus(uint8_t adcIntFlag);
uint8_t ADCCTRL_ReadINTStatus(void);
void ADCCTRL_SampleEn(void);
void ADCCTRL_SampleDis(void);
uint8_t ADCCTRL_isFIFOEmpty(void);
void ADCCTRL_AuxADCSignSwapEn(void);
void ADCCTRL_AuxADCSignSwapDis(void);
void ADCCTRL_AuxADCPowerOn(void);
void ADCCTRL_AuxADCPowerOff(void);
void ADCCTRL_AdcDivDis(void);
void ADCCTRL_AdcDivlEn(void);
void ADCCTRL_ChannelConfig(uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num);
void ADC_Select_Vref(uint8_t Vref);

void ADC_bbldoEnable(void);	
void ADC_SigleModeInit(uint8_t channelID,uint32_t mclk,ADC_CLK AdcClk);
void ADC_SingleModeDeInit(uint8_t channelID);
int16_t ADCCTRL_ReadADCDataBlocking(void);
bool ADCCTRL_ReadADCDataNoBlocking(uint8_t channelID,int16_t* AdcData);
uint16_t ADCCTRL_DirectReadADCDataBlocking(void);
bool ADCCTRL_DirectReadADCDataNoBlocking(uint16_t* AdcData);
int16_t ADC_ADCDataCaliWtihNV(uint8_t channelID, int16_t conv_val);
uint32_t ADC_TemperatureCompensation(int16_t CurrentTmp);
uint16_t ADC_ConverToValtage(uint8_t channelID, int16_t covert_data, uint8_t Vref);
int16_t ADC_ConverToTempeture(int16_t covert_data,uint8_t Vref) ;

int16_t get_adc_value_quickly(uint8_t channelID,uint32_t mclk,uint16_t AdcClk,uint8_t CalibrationFlag,int16_t CurrentTmp, uint8_t Vref ); //KHZ; //KHZ
int16_t get_adc_value(uint8_t channelID,uint32_t mclk,uint16_t AdcClk,uint8_t CalibrationFlag,int16_t CurrentTmp, uint8_t Vref);//with high precision

void ADC_ScanModeInit(uint32_t mclk,uint16_t AdcClk,const uint8_t *ADC_Channle,uint8_t ChannelNum);
void ADC_ScanModeDeInit(void);
void ADC_ScanDataProcessing(uint16_t ChannelNum, int16_t* result_data, const uint8_t *channel, uint8_t Vref);
void ADC_ScanReadBuffDataBlocking(int16_t* result_data,uint8_t *result_channel, uint16_t cnt);
void ADC_ScanReadData(uint32_t mclk,uint16_t AdcClk,const uint8_t *ADC_Channle,uint16_t ChannelNum,int16_t* result_data, uint8_t Vref);
void ADC_ScanReadDataQuickly(uint32_t mclk,uint16_t AdcClk,const uint8_t *ADC_Channle,uint16_t ChannelNum,int16_t* result_data,uint8_t Vref);

void ADC_SigleModeInit_OPACali(uint8_t channelID,uint32_t mclk,uint16_t AdcClk);
uint16_t ADC_get_Smoke_value(uint8_t channelID, uint32_t mclk,uint16_t AdcClk,uint8_t Vref);
uint16_t ADC_FT_get_Smoke_value(uint8_t channelID, uint32_t mclk, uint16_t AdcClk, uint8_t Vref) ;
uint16_t ADC_FT_get_OPAOnly_value(uint8_t channelID, uint32_t mclk, uint16_t AdcClk, uint8_t Vref);
short get_adc_cali_nv(uint8_t channelID,uint32_t mclk,uint16_t AdcClk); //KHZ
void get_tsensor_caliNv(uint32_t mclk,int16_t temperaVal,uint8_t Vref);
int16_t ADC_CalibreationPresser(int32_t ADC_channel,int32_t ADC_voltage,uint32_t mclk,adc_cali_nv_t* adc_cali_nv,uint8_t smoke_flag);
void GPIO13_2P2_Config(void);
void ADC_CaliValueCaculate(adc_cali_data_result* gain_offset_result,uint8_t Vref);
void ADC_recover_init(void);


//ATE version
#define G_MCLK                  26000
#define ADC_TASK_NAME          "adc_task"
#define ADC_TASK_STACK_SIZE    osStackShared
#define ADC_TASK_PRIORITY      osPriorityHigh
#define ADC_RANGE_VBAT		   55UL

extern int32_t gTempera;
extern uint16_t gVBatVal;
extern uint8_t g_ADCRange;

// uint16_t get_vbat_value();
// int16_t get_tempera_value();
// uint16_t get_adc_extch_value();

int16_t get_adc_value_incpcore(uint8_t channelID);
void get_rfmtsensor_value(int16_t Reftempera);

#endif
