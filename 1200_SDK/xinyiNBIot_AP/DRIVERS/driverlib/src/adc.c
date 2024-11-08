#include "adc.h"
#include "xy_memmap.h"
#include "prcm.h"
#include "timer.h"
#include "common.h"
#include "hal_def.h"
#include "sys_mem.h"
#include "hw_tick.h"

#if(XY_DUMP == 1)
#define ADC_DUBUG_INFO			 0
#define TSENSOR_DELTA_VALUE      63 //63*0.16=10
void Debug_TsensorBefore();
#endif

adc_cali_data_result g_adc_cali_result={0};
adc_cali_nv_t g_adc_cali_nv = {0};

volatile uint8_t g_CalibrationFlag = false;
volatile uint32_t g_trxbg_CaliReg = 0xFFFFFFFF;

const uint8_t DIV[3][4]= {{2,26,53,107},{1,13,26,53},{0,6,12,26}};
//static void adc_delay(unsigned long uldelay)
//{
//    volatile unsigned long i;
//
//    for(i = 0; i < uldelay; i++)
//    {
//    }
//}

/**
  * @brief Set the switch time
  * @param switch_time: Set the channel switching time value,the scope is 0-0xf,the minimum is 3.
  * @retval None
 */
void ADCCTRL_SetSwitchTime(uint8_t switch_time)
{
	ADCCTRL->ADC_SWITCH_TIME = switch_time;
}

/**
  * @brief ADC channel Configure
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param divN: ADC_CLK = MCLK/((div +1)*2)with duty =1(50%),ADC_CLK = MCLK/((div +1)*8)with duty =0(25%).
  * @param ave_mode: the value should be the following :
  *     @arg 0: General average.
  *     @arg 1: Moving average.
  * @param ave_num: The number used to calculate the average.
  * @retval None
*/
void ADCCTRL_ChannelConfig(uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num)
{
	uint8_t avgConfig = 0x0;
    divN -= 1;
	if (channelID >= 8)
		channelID = channelID - 8;

	if (ave_num == 0)
		avgConfig = 0;
	else
		avgConfig = (ave_num << 2) | (ave_mode << 1) | 0x1;

	ADCCTRL->ADCCTRL_CFG[channelID] = (divN >> 8) | (((divN & 0xff)) << 8) | (avgConfig << 16);
}


/**
  * @brief ADC enable
  * @param None
  * @retval None
 */
void ADCCTRL_AdcCtrlEn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_CTRL_EN_Pos);	
}

/**
  * @brief ADC disable
  * @param None
  * @retval None
 */
void ADCCTRL_AdcCtrlDis(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_CTRL_EN_Pos);	
}

/**
  * @brief ADC Div enable
  * @param None
  * @retval None
 */
void ADCCTRL_AdcDivlEn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_AUX_DIV_Pos);	
}

/**
  * @brief ADC Div disable
  * @param None
  * @retval None
 */
void ADCCTRL_AdcDivDis(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_AUX_DIV_Pos);	
}

/**
  * @brief Set duty = 1,the duty rational is 50%.
  * @param None
  * @retval None
 */
void ADCCTRL_DutyEn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_DUTY_EN_Pos);	
}

/**
  * @brief Set duty = 0,the duty rational is 25%.
  * @param None
  * @retval None
 */
void ADCCTRL_DutyDis(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_DUTY_EN_Pos);	
}

/**
  * @brief Set AUX ADC power on.
  * @param None
  * @retval None
 */
void ADCCTRL_AuxADCPowerOn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_AUX_PU_Pos);	
}

/**
  * @brief Set AUX ADC power off.
  * @param None
  * @retval None
 */
void ADCCTRL_AuxADCPowerOff(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_AUX_PU_Pos);	
}

/**
* @brief ADC sample enable.
* @param None
* @retval None
*/
void ADCCTRL_SampleEn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_SAMPLE_EN_Pos);	
}

/**
* @brief ADC sample disable.
* @param None
* @retval None
*/
void ADCCTRL_SampleDis(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_SAMPLE_EN_Pos);	
}

/**
* @brief ADC trigger enable.
* @param None
* @retval None
*/
void ADCCTRL_FuncTriggerEn(void)
{
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_FUNC_TRIGGER_Pos);	
}

/**
* @brief ADC trigger disable.
* @param None
* @retval None
*/
void ADCCTRL_FuncTriggerDis(void)
{
	ADCCTRL->ADC_CTRL0 &= ~(uint8_t)(0x1 << ADC_FUNC_TRIGGER_Pos);	
}

/**
* @brief digital always using rising edge to capture adc_data.
* @param None
* @retval None
*/
void ADCCTRL_AuxADCRdyInvDis(void)
{
	ADCCTRL->ADC_CTRL1 &= ~(uint8_t)(0x1 << ADC_AUX_RDY_Pos);
}

/**
* @brief digital always using falling edge to capture adc_data.
* @param None
* @retval None
*/
void ADCCTRL_AuxADCRdyInvEn(void)
{
	ADCCTRL->ADC_CTRL1 |= (uint8_t)(0x1 << ADC_AUX_RDY_Pos);
}

/**
* @brief The highest bit of the sampled data is not inverted.
* @param None
* @retval None
*/
void ADCCTRL_AuxADCSignSwapDis(void)
{
	ADCCTRL->ADC_CTRL1 &= ~(uint8_t)(0x1 << ADC_AUX_SIGN_SWAP_Pos);	
}

/**
* @brief The highest bit of the sampled data is inverted.
* @param None
* @retval None
*/
void ADCCTRL_AuxADCSignSwapEn(void)
{
	ADCCTRL->ADC_CTRL1 |= (uint8_t)(0x1 << ADC_AUX_SIGN_SWAP_Pos);	
}


/**
* @brief Set the FIFO threshold value .
* @param up_thr
* @retval None
*/
void ADCCTRL_FIFOUpThrConfig(uint8_t up_thr)
{
	ADCCTRL->ADC_CTRL2 = (ADCCTRL->ADC_CTRL2 & 0xF) | (up_thr << ADC_FIFO_THR_Pos);
}

/**
* @brief Set ADC interrupt enable .
* @param adcIntFlag
* @retval None
*/
void ADCCTRL_INTEn(uint8_t adcIntFlag)
{
	ADCCTRL->ADC_CTRL2 |= (uint8_t)adcIntFlag;
}


/**
* @brief Set ADC interrupt enable .
* @param adcIntFlag
* @retval None
*/
void ADCCTRL_INTDis(void)
{
	ADCCTRL->ADC_CTRL2 = 0;
}
/**
* @brief Clear ADC interrupt flag by writing 1 .
* @param adcIntFlag
* @retval None
*/
void ADCCTRL_ClearINTStatus(uint8_t adcIntFlag)
{
	ADCCTRL->ADC_INT_STAT |= (uint8_t)adcIntFlag; 
}

/**
* @brief Read ADC interrupt status.
* @retval None
*/
uint8_t ADCCTRL_ReadINTStatus(void)
{
	return (ADCCTRL->ADC_INT_STAT & 0x0f); 
}

/**
* @brief Enable ADC DMA mode.
* @retval None
*/
void ADCCTRL_DmaModeEn(void)
{
	ADCCTRL->ADC_CTRL3 |= (uint8_t)(0x1 << ADC_DMA_EN_Pos);	
}

/**
* @brief Disable ADC DMA mode.
* @retval None
*/
void ADCCTRL_DmaModeDis(void)
{
	ADCCTRL->ADC_CTRL3 &= ~(uint8_t)(0x1 << ADC_DMA_EN_Pos);	
}

/**
* @brief Enable ADC Scan mode.
* @retval None
*/
void ADCCTRL_ScanModeEn(void)
{
	ADCCTRL->ADC_CTRL3 |= (uint8_t)(0x1 << ADC_SCAN_EN_Pos);	
}

/**
* @brief Disable ADC Scan mode.
* @retval None
*/
void ADCCTRL_ScanModeDis(void)
{
	ADCCTRL->ADC_CTRL3 &= ~(uint8_t)(0x1 << ADC_SCAN_EN_Pos);	
}

/**
* @brief Set ADC channel ID in single mode.
* @param channelID:The scope is 0-12.
* @retval None
*/
void ADCCTRL_SetChannelID(uint8_t channelID)
{
	ADCCTRL->ADC_CTRL3 = (ADCCTRL->ADC_CTRL3 & 0xF) | (channelID << ADC_CHANNEL_NUM);
}

/**
* @brief Clear FIFO by hardware.
* @retval None
*/
void ADCCTRL_FIFOFlush(void)
{
	ADCCTRL->ADC_CTRL3 |= (uint8_t)(0x1 << ADC_FIFO_FLUSH_Pos);	
}

/**
* @brief Check the status of adc_direct_rdy.
* @retval None
*/
uint8_t ADCCTRL_CheckDirectDataRdy(void)
{
	return (ADCCTRL->ADC_DIRECT_RDY & 0x1);	
}

/**
  * @brief ADC scan mode channel Configure
  * @param scanChannelID: Select the ADC scan channel from 0 to 7.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param divN: ADC_CLK = MCLK/((div +1)*2)with duty =1(50%),ADC_CLK = MCLK/((div +1)*8)with duty =0(25%).
  * @param ave_mode: the value should be the following :
  *     @arg 0: General average.
  *     @arg 1: Moving average.
  * @param ave_num: The number used to calculate the average.
  * @param scan_num: ADC scan times .
  * @retval None
*/
int32_t ADCCTRL_AddScanChannel(uint8_t scanChannelID, uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num, uint8_t scan_num)
{
	if (scanChannelID >= 8)
		return 0;
	
	ADCCTRL->scan_channel_select = (ADCCTRL->scan_channel_select & (~(0xF << 4*scanChannelID))) | (channelID << 4*scanChannelID);
	ADCCTRL->scan_channel[scanChannelID] = 0x80 | scan_num;
	ADCCTRL_ChannelConfig(scanChannelID, divN, ave_mode, ave_num);
	
	return 1;
}

/**
  * @brief Disable the ADC scan mode channel Configuration
  * @param scanChannelID: Select the ADC scan channel from 0 to 7.
  * @retval None
*/
void ADCCTRL_DelScanChannel(uint8_t scanChannelID)
{
	uint8_t i = 0;

	if (scanChannelID == 0xFF)
	{	
		for (i = 0; i < 8; i++)	
			ADCCTRL->scan_channel[i] = 0x0;
	}
	else
	{
		ADCCTRL->scan_channel[scanChannelID] = 0x0;
	}
}

/**
  * @brief ADC scan times Configure
  * @param scan_turn_num: ADC scan times .
  * @retval None
*/
void ADCCTRL_SetScanModeTurnNum(uint8_t scan_turn_num)
{
	ADCCTRL->ADC_AVE_NUM = (ADCCTRL->ADC_AVE_NUM & 0x8F) | (scan_turn_num << 4); 
}

/**
  * @brief Enable changing scan channel by Time1 pluse input
  * @retval None
*/
void ADCCTRL_InnerSuspendEnable(void)
{
	ADCCTRL->ADC_AVE_NUM |= SCAN_INNER_SUSPEND_EN_MSK; 
}

/**
  * @brief Disable changing scan channel by Time1 pluse input
  * @retval None
*/
void ADCCTRL_InnerSuspendDisable(void)
{
	ADCCTRL->ADC_AVE_NUM &= ~SCAN_INNER_SUSPEND_EN_MSK; 
}

/**
  * @brief Enable timer turn scan sample.After the periodic scan function is enabled, 
  *        the data of all configuration channels can be sampled for each periodic scan.
  * @retval None
*/
void ADCCTRL_TurnSuspendEnable(void)
{
	ADCCTRL->ADC_AVE_NUM |= SCAN_TURN_SUSPEND_EN_MSK; 
}

/**
  * @brief Disable timer turn scan sample.After the periodic scan function is enabled, 
  *        the data of all configuration channels can be sampled for each periodic scan.
  * @retval None
*/
void ADCCTRL_TurnSuspendDisable(void)
{
	ADCCTRL->ADC_AVE_NUM &= ~SCAN_TURN_SUSPEND_EN_MSK; 
}

/**
  * @brief Reverse the highest bit of ADC sample data by software.
  * @param data: ADC fifo data .
  * @retval None
*/
int16_t adcDataExtend(int16_t data)
{
	if (data & 0x800)
	{
		return ((data | 0xF000) + 2048);
	}
	else
	{
		return (data + 2048 -4);  //真实值大于2048就减4
	}
}

/**
  * @brief Check whether the ADC FIFO is empty.
  * @retval None
*/
uint8_t ADCCTRL_isFIFOEmpty(void)
{
	return ((ADCCTRL->ADC_FIFO_STAT & 0x04));
}


/**
  * @brief Read data from a channel.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @retval ADC data(0-4095).
*/
int16_t ADCCTRL_ReadADCDataBlocking(void)
{
	uint32_t adcData;
	volatile uint8_t dataValidFlg = 0;
	while((ADCCTRL->ADC_FIFO_STAT&0x04)==0x04); //wait no empty
	adcData = ADCCTRL->ANAAUXADC;
	dataValidFlg = ADCCTRL->ADC_DIRECT_RDY;
	if (dataValidFlg & 0x2)
	{
	
		return (adcDataExtend(adcData & 0xFFF));
	}
	if (dataValidFlg & 0x4)
	{		

	    return (adcDataExtend((adcData & 0xFFF0000) >> 16));
	}
    
	return 0;
}


/**
  * @brief Read data from a channel,swap enable.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param AdcData: ADC data(0-4095).
  * @retval The ADC data is true or false.
*/
bool ADCCTRL_ReadADCDataNoBlocking(uint8_t channelID,int16_t* AdcData)
{
	uint32_t adcData;
	uint8_t  adcChID;
	volatile uint8_t dataValidFlg = 0;

	if(ADCCTRL_isFIFOEmpty() == 0)
	{
		adcData = ADCCTRL->ANAAUXADC;
		dataValidFlg = ADCCTRL->ADC_DIRECT_RDY;
		if (dataValidFlg & 0x2)
		{

			adcChID = (adcData & 0xF000) >> 12;
			if (adcChID == channelID)
			{
				
				*AdcData =  adcDataExtend(adcData & 0xFFF);
				return true;
			}

		}
		else if (dataValidFlg & 0x4)
		{
			adcChID = (adcData & 0xF0000000) >> 28;
			if (adcChID == channelID)
			{
				*AdcData = adcDataExtend((adcData & 0xFFF0000) >> 16);
				return true;
			}
		}
		else
		{
		}
		return false;
	}
	else
	{
		return false;
	}
}

/**
  * @brief Read data from ADC_DIRECT_DATA.
  * @retval ADC direct data(0-4095).
*/
uint16_t ADCCTRL_DirectReadADCDataBlocking(void)
{
	uint16_t adcData;

	while(ADCCTRL_CheckDirectDataRdy() == 0);	

	adcData = ADCCTRL->ADC_DIRECT_DATA;

	return adcData;
	
}

bool ADCCTRL_DirectReadADCDataNoBlocking(uint16_t* AdcData)
{

	if(ADCCTRL_CheckDirectDataRdy() == 1)
	{

		*AdcData = ADCCTRL->ADC_DIRECT_DATA;
		return true;
	}
	else
	{
	    return false;
	}

}



/**
  * @brief ADC single mode channel Configure
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param divN: ADC_CLK = MCLK/((div +1)*2)with duty =1(50%),ADC_CLK = MCLK/((div +1)*8)with duty =0(25%).
  * @param ave_mode: the value should be the following :
  *     @arg 0: General average.
  *     @arg 1: Moving average.
  * @param ave_num: The number used to calculate the average.
  * @retval None
*/
void ADCCTRL_SingleModeConfig(uint8_t channelID, uint16_t divN, uint8_t ave_mode, uint8_t ave_num)
{
	ADCCTRL_SetSwitchTime(3);
	ADCCTRL_ChannelConfig(channelID, divN, ave_mode, ave_num);
	ADCCTRL_ScanModeDis();
	ADCCTRL_SetChannelID(channelID);
}

/**
  * @brief ADC scan mode configuration.
  * @param scan_turn_num: ADC scan times(0-7) .
  * @retval None
*/
void ADCCTRL_ScanModeConfig(uint8_t turnNum)
{
	ADCCTRL_SetSwitchTime(3);	
	ADCCTRL_SetScanModeTurnNum(turnNum);   
	ADCCTRL_ScanModeEn();
}


/**
  * @brief ADC start.
  * @retval None
*/
void ADC_Start(void)
{
	delay_func_us(20);            //需要延时10us ,系统时钟为13M时最小延时为14us
	while(!ADC_IsActively_Trxbg());  //等待TRXBG RDY信号
	ADCCTRL->ADC_CTRL3 |= 0x02;  //FLUSH
#if(ADC_DUBUG_INFO == 1)
	if(((READ_REG(ADCCTRL->ADC_CTRL3) & 0xf8) == 0x70) || (READ_REG(ADCCTRL->ADC_CTRL3) & 0x08))    // single mode , channel = Tsensor  or scan mode
	{
		Debug_TsensorBefore();
	}
#endif	
	ADCCTRL->ADC_CTRL0 |= 0x37;  //Sample EN
}
/**
  * @brief ADC stop.
  * @retval None
*/
void ADC_Stop(void)
{
	HWREGB(0x40070008) &= 0x08;
	ADCCTRL->ADC_CTRL3 |= (uint8_t)(0x1 << ADC_FIFO_FLUSH_Pos);	 //FLUSH 

}


/**
  * @brief ADC VREF SET .
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval None
*/
void ADC_Select_Vref(uint8_t Vref)
{

	if(Vref == ADC_VREF_VALUE_1P3V )
	{
		ADC_VREF_Set(0);
	}
	else if(Vref == ADC_VREF_VALUE_1P5V )
	{
		ADC_VREF_Set(1);
	}
	else if(Vref == ADC_VREF_VALUE_2P2V )
	{
		ADC_VREF_Set(2);
	}
	else
	{
		ADC_VREF_Set(3);
	}
}


void ADC_bbldoEnable(void)
{
	//	RF_BBLDO_CNTL_En();
	HWREGB(0x40000825) = HWREGB(0x40000825)|0x01;

	/* bbldo_pu*/
	HWREGB(0x40004860) = HWREGB(0x40004860)| 0x08;
	
	ADC_Set_Ldoana_2p7v();  //Set LDOANA 2.7v

}

/**
 * @brief 通过ADC参考时钟、工作时钟来计算DIVN
 * @retval DIVN的值
 */
uint16_t ADC_CalcDivn_By_AdcClk(uint32_t mclk, ADC_CLK AdcClk)
{
    uint16_t divN = 1;

	if(AdcClk == ADC_CLK_480K)
	{
		HWREGB(0x40004847) |= 0x4;          //ADC clk duty cycle control 1:50%    0:25%
		ADCCTRL->ADC_CTRL0 |= 0x08;	        //Duty Enable 					
		
		if(mclk >= 5280 && mclk <= 6240){divN = 5;}
		else if(mclk > 6240 && mclk <= 6720){divN = 6;}
		else if(mclk > 6720 && mclk <= 8160){divN = 7;}      //6.5M 按照15%的误差，5.525M~7.475M
		else if(mclk >= 11000 && mclk <= 12000){divN = 11;}
		else if(mclk > 12000 && mclk <= 12960) {divN = 12;}  //13M 按照15%的误差，11.05M~14.95M
		else if(mclk > 12960 && mclk <= 15000) {divN = 13;}		
		else if(mclk > 22000 && mclk <= 24480) {divN = 24;}
		else if(mclk > 24480 && mclk <= 25440) {divN = 25;}  //26M 按照15%的误差，22.1M~29.9M
		else if(mclk > 25440 && mclk <= 30000) {divN = 26;}	 	
		else
		{
			divN = 1;
		}
		
	}
	else if(AdcClk == ADC_CLK_960K)
	{
		HWREGB(0x40004847) &= 0xFB;        //ADC clk duty cycle control 1:50%    0:25%
		ADCCTRL->ADC_CTRL0 &= 0xF7;	       //Duty Disable
		
		if(mclk >= 5280 && mclk <= 7500){divN = 1;}             //6.5M 按照15%的误差，5.525M~7.475M
		else if(mclk >= 11000 && mclk <= 13440){divN = 2;}
		else if(mclk > 13440 && mclk <= 17280) {divN = 3;}    //13M 按照15%的误差，11.05M~14.95M
		else if(mclk > 22000 && mclk <= 24960) {divN = 5;}   
		else if(mclk > 24960 && mclk <= 28800) {divN = 6;}    //26M 按照15%的误差，22.1M~29.9M
		else if(mclk > 28800 && mclk <= 30000) {divN = 7;}		
		else
		{
			divN = 1;
		}
	}
	else if(AdcClk == ADC_CLK_240K)
	{
		HWREGB(0x40004847) |= 0x4;          //ADC clk duty cycle control 1:50%    0:25%
		ADCCTRL->ADC_CTRL0 |= 0x08;	        //Duty Enable 	
		
		if(mclk >= 5280 && mclk <= 6240){divN = 11;}
		else if(mclk > 6240 && mclk <= 6720){divN = 12;}
		else if(mclk > 6720 && mclk <= 8000){divN = 13;}      //6.5M 按照15%的误差，5.525M~7.475M
		else if(mclk >= 11000 && mclk <= 12000){divN = 24;}
		else if(mclk > 12000 && mclk <= 13000) {divN = 25;}  //13M 按照15%的误差，11.05M~14.95M
		else if(mclk > 13000 && mclk <= 15000) {divN = 26;}		
		else if(mclk > 22000 && mclk <= 25680) {divN = 52;}
		else if(mclk > 25680 && mclk <= 26000) {divN = 53;}  //26M 按照15%的误差，22.1M~29.9M
		else if(mclk > 26000 && mclk <= 30000) {divN = 54;}	 	
		else
		{
			divN = 5;
		}
	}
	else if(AdcClk == ADC_CLK_120K)
	{
		HWREGB(0x40004847) |= 0x4;          //ADC clk duty cycle control 1:50%    0:25%
		ADCCTRL->ADC_CTRL0 |= 0x08;	        //Duty Enable 	
		
		if(mclk >= 5760 && mclk <= 6360){divN = 25;}
		else if(mclk > 6360 && mclk <= 6600){divN = 26;}      //6.5M 按照15%的误差，5.525M~7.475M
		else if(mclk > 6600 && mclk <= 8000){divN = 27;}
		else if(mclk >= 11000 && mclk <= 12000){divN = 49;}
		else if(mclk > 12000 && mclk <= 12840) {divN = 52;}    //13M 按照15%的误差，11.05M~14.95M
		else if(mclk > 12840 && mclk <= 15000) {divN = 53;}
		else if(mclk > 22000 && mclk <= 25320) {divN = 103;}
		else if(mclk > 25320 && mclk <= 25560) {divN = 105;}
		else if(mclk > 25560 && mclk <= 25800) {divN = 106;}
		else if(mclk > 25800 && mclk <= 26000) {divN = 107;}	//26M 按照15%的误差，22.1M~29.9M
		else if(mclk > 26000 && mclk <= 30000) {divN = 108;}
		else 
		{
			divN = 11;
		}
	}
	else
	{
	
	}

    return divN;
}

/**
  * @brief ADC int.
  * @param channelID: Select the ADC channel from 0 to 12
* @param mclk: select the CLK source of ADC CLK,Unit:K.
  * @param AdcClk: set the ADC CLK,Unit:K.
  * @retval None
*/
void ADC_SigleModeInit(uint8_t channelID,uint32_t mclk,ADC_CLK AdcClk)
{
	uint16_t divN = 1;

    ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_AUX_PU_Pos);	 //ADC PU
    if (channelID == ADC_VBAT)
	{
    	ADC_VbatPowerEN(); // vbat mon enable
	}
	else if (channelID == ADC_TSENSOR)
	{
        //Tsensor power mode control
		if (AdcClk <= ADC_CLK_240K)
		{
			HWREGB(0x40004848) = (HWREGB(0x40004848) & 0xF8) | 0x1; //coreprcn_adif: aux_cntl[58:56]
		}
		else
		{
			HWREGB(0x40004848) = (HWREGB(0x40004848) & 0xF8) | 0x4;
		}
	}
	else if (channelID == ADC_CMP_INP)   //0,GPIO 8
	{
		HWREGB(0x40004851) |= 0x20;
	}
	else if (channelID == ADC_CMP_INN)   //1,GPIO 9
	{
		HWREGB(0x40004851) |= 0x10;
	}
	else if (channelID == ADC_OP1_INP)   //3,GPIO 10
	{
		HWREGB(0x40004851) |= 0x08;
	}
	else if (channelID == ADC_OP1_INN)	 //4,GPIO 11
	{
		HWREGB(0x40004851) |= 0x04;
	}
	else if (channelID == ADC_OP0_OUT)	 //6,GPIO 13
	{
		HWREGB(0x40004851) |= 0x02;
	}
	else if (channelID == ADC_OP1_OUT)	 //5,GPIO 12
	{
		HWREGB(0x40004851) |= 0x01;
	}
	else
	{
	}

    divN = ADC_CalcDivn_By_AdcClk(mclk, AdcClk);

    ADCCTRL_ChannelConfig(channelID, divN, 1, 2);
	ADCCTRL->ADC_CTRL3 = channelID << ADC_CHANNEL_NUM; //clear scan、DMA
	ADCCTRL->ADC_SWITCH_TIME = 3;  
	ADCCTRL->ADC_CTRL1 = 0x40;   //SWAP =0

	if(ADC_VREF_Get() ==2)
	{
		HWREGB(0x40004856) |= 0x40; //OPA0_out connnect to GND when OPA0 power off
	}
}


/**
  * @brief Calculate the calibrated value with NV calibration parameters.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param conv_val: ADC FIFO value.
  * @param Vref: The value can be one of the following:
  *              @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *              @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *              @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *              @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated  ADC sample value
*/
int16_t ADC_ADCDataCaliWtihNV(uint8_t channelID, int16_t conv_val)
{
	int16_t rounding_val = 0;
	
	switch(channelID)
	{	
		case ADC_CMP_INP:  //GPIO 8
		case ADC_CMP_INN:  //GPIO 9
		case ADC_CMP_OUT:	//2
		case ADC_OP1_INP:	//3  GPIO 10  ADC_channel_3   ADC4
		case ADC_OP1_INN:	//4  GPIO 11
		case ADC_OP0_OUT:	//6  GPIO 13  ADC5
		case ADC_OP1_OUT:	//5	 GPIO 12  ADC_channel_6   ADC6
		case ADC_OPA0_INP:	//ADC_channel_9   ADC7
		case ADC_OPA0_INN:	//ADC_channel_10
			rounding_val = (int16_t)(((conv_val * g_adc_cali_result.gpio8_gainerror) + g_adc_cali_result.gpio8_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			 break;
		
		case ADC_VBAT:
			
				rounding_val = (int16_t)(((conv_val * g_adc_cali_result.vbata_gainerror) + g_adc_cali_result.vbata_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			break;
			
		case ADC_TSENSOR:
			rounding_val = (int16_t)(((conv_val * g_adc_cali_result.gpio8_gainerror) + g_adc_cali_result.gpio8_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			break;
		
		case ADC_AUX_ADC1:
		case ADC_AUX_ADC2:
			if(ADC_Get_Attenu())   //ADC1&2 high voltage
			{
				rounding_val = (int16_t)(((conv_val * g_adc_cali_result.adc2_high_gainerror) + g_adc_cali_result.adc2_high_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			}
			else
			{
				rounding_val = (int16_t)(((conv_val * g_adc_cali_result.adc2_gainerror) + g_adc_cali_result.adc2_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			}
			break;
		
		default:
			rounding_val = (int16_t)(((conv_val * g_adc_cali_result.gpio8_gainerror) + g_adc_cali_result.gpio8_offset + (ADC_EXPAND_MULTIPLE >> 1)) / ADC_EXPAND_MULTIPLE);
			break;
	}

	return rounding_val;
}
/**
  * @brief Calculate the calibrated value with NV calibration parameters.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param conv_val: ADC FIFO value.
  * @param adc_cali: 0:smoke mode,output:opa1_out,input:opa0_inp;  1:only OPA1,output:opa1_out,input:opa1_inp;
  * @retval The calibrated  ADC sample value
*/
int16_t ADC_OPACaliWtihNV(uint8_t channelID, int16_t conv_val,ADC_CALI_TYPE adc_cali)
{
	int16_t rounding_val = 0;

	switch(channelID)
	{
		case ADC_OP0_OUT:	//5	 GPIO 12  ADC_channel_6   ADC6
			rounding_val = (int16_t)(((conv_val * OPA0_gainerror) + OPA0_offset + (ADC_EXPAND_MULTIPLE / 2)) / ADC_EXPAND_MULTIPLE);
			break;

		case ADC_OP1_OUT:	//5	 GPIO 12  ADC_channel_6   ADC6
			if(adc_cali == SMOKE_TYPE)
			{
				rounding_val = (int16_t)(((conv_val * OPA0_OPA1_gainerror) + OPA0_OPA1_offset + (ADC_EXPAND_MULTIPLE / 2)) / ADC_EXPAND_MULTIPLE);
			}
			else if(adc_cali == OPA_ONLY_TYPE)
			{
				rounding_val = (int16_t)(((conv_val * OPA1_gainerror) + OPA1_offset + (ADC_EXPAND_MULTIPLE / 2)) / ADC_EXPAND_MULTIPLE);
			}
			break;

		default:
			rounding_val = (int16_t)(((conv_val * OPA0_OPA1_gainerror) + OPA0_OPA1_offset + (ADC_EXPAND_MULTIPLE / 2)) / ADC_EXPAND_MULTIPLE);
			break;
	}
	return rounding_val;
}



/**
  * @brief Convert ADC data to measure value with temperature compensation.
  * @param CurrentTmp:The current temperature( unit :?)
  * @retval Temperature compensation coefficient * 100000
*/
uint32_t ADC_TemperatureCompensation(int16_t CurrentTmp)
{
	uint32_t  multiFactorTC = 1;

	multiFactorTC = (11000  - (CurrentTmp - 25)) * ADC_EXPAND_MULTIPLE / 11000;
	return multiFactorTC;
}

/**
  * @brief Convert ADC data to voltage?
  * @param channelID: Select the ADC channel from 0 to 12
  * @param covert_data: ADC data with NV calibration.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval voltage(unit:mV).
*/
uint16_t ADC_ConverToValtage(uint8_t channelID, int16_t covert_data, uint8_t Vref)
{
	if(((channelID == ADC_AUX_ADC1) || (channelID == ADC_AUX_ADC2)) && (ADC_Get_Attenu()))
	{
		return (uint16_t)(((covert_data * 100) * (Vref *4)+ 2048)/4096);
	}
	else
	{
		return (uint16_t)(((covert_data * 100) * (channelID == ADC_VBAT ? (Vref * 5) : Vref) + 2048)/4096);
	}
}


/**
  * @brief Convert ADC data to temperature?
  * @param covert_data: ADC data. if no temperature compensation ,covert_data need delete 2250 before
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval If the channel is Tsensor,the return is temperature(unit :?),else the return is voltage(unit:mV).
*/
int16_t ADC_ConverToTempeture(int16_t covert_data, uint8_t Vref)    //???????covert_data -= 2250;
{
	int16_t temperature;
	
	if(Vref != ADC_VREF_VALUE_2P2V)
	{
		temperature = (covert_data << 4) / 100;//????????,convert_data -= g_adc_cali_nv.code_tsensor
	}
	else
	{
		temperature = ((int32_t)covert_data) * 2347 / 10000;//????????,convert_data -= g_adc_cali_nv.code_tsensor
	}

	if(temperature > 150)
	{
		temperature = 150;
	}
	else if(temperature < -40)
    {
		temperature = -40;
	}
	else
	{
	}	
	
	return temperature;  
}


/**
  * @brief Disable ADC init
  * @param channelID: Select the ADC channel from 0 to 12
  * @retval None
*/
void ADC_SingleModeDeInit(uint8_t channelID)
{
	if (channelID == ADC_VBAT)
	{
		ADC_VbatPowerDIS(); // vbat mon disable
	}


    if(ADC_VREF_Get() != 2 )
    {
	    HWREGB(0x40004851) = 0;    // 红外收发验证需要屏蔽
    }
    else
    {
	   HWREGB(0x40004851) = 0x02;  // ADC OPA0_OUT connect to the PAD,红外收发验证需要屏蔽
    }
	//ADC Stop
	ADCCTRL->ADC_CTRL0 &= 0x08;
	ADCCTRL->ADC_CTRL3 = 0x02;	 //FLUSH 
}
/**
  * @brief Read the calibrated voltage value or temperature.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param CalibrationFlag: 1:calibration flag valid.
  * @param CurrentTmp: The current temperature.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
 */
#if (ADC_DUBUG_INFO == 1)
#define LOW_ALARM_VBATVOL	1090	  // 检测的下限电压值约为：LOW_ALARM_VBATVOL/4096 *7.5 (V)
#define HIGH_ALARM_VBATVOL	3000	  // 检测的上限电压值约为：HIGH_ALARM_VBATVOL/4096 *7.5 (V)

/* RF pre is off，now is on */
#define TEMP_DELTA_HIGH1      188  //188*0.16 = 30   
#define TEMP_DELTA_HIGH2      188  //124*0.16 = 20   

#define TEMP_DELTA_LOW1        62   //62*0.16=10
#define TEMP_DELTA_LOW2        62   //62*0.16=10

#define TEMP_DELTA_MS         30000

volatile int16_t g_debug_adc_data;
volatile uint8_t g_debug_ChannelID;
volatile uint8_t g_debug_Vref;
volatile uint8_t g_debug_ldoana_2p7 =0x0E;
volatile uint8_t g_debug_tsensorPower = 4;   //480K
volatile int16_t g_debug_tsensorcode_old = 0;
volatile uint8_t first_sample_flag = 0;
volatile uint32_t g_debug_reg[4] = {0};

typedef struct
{
 uint8_t trx_status[2];
 uint8_t tx_pwrctrl[2];
 uint8_t rx_pwrctrl[2];
 int16_t debug_adc_data;

 uint8_t debug_Vref;
 uint8_t debug_ldoana_2p7;
 uint8_t debug_tsensorPower;   //480K
 int16_t debug_tsensorcode;
 uint32_t debug_reg[4];
 uint32_t tickTimer;

}ADC_SampleRecords;

ADC_SampleRecords adc_record[4] = {0};

uint8_t record_cnt = 0;
/**
  * @brief Whether the TRX is active
  * @param  None
  * @retval The status
 */
uint8_t DFE_TxEn_IsActive(void)
{
	return(HWREGB(0x80 + DFE_REG_BASE) & 0x01);
}

uint8_t DFE_RxEn_IsActive(void)
{
	return(HWREGB(0x04 + DFE_REG_BASE) & 0x01);
}


void Debug_TsensorBefore()
{
	if((HWREGB(0x4000400B) & 0x10) &&(HWREGB(0x40004005) & 0x10))
	{
		adc_record[record_cnt].trx_status[0] = DFE_RxEn_IsActive() << 1| DFE_TxEn_IsActive();
		adc_record[record_cnt].rx_pwrctrl[0] = HWREG(0x4001A230);
		adc_record[record_cnt].tx_pwrctrl[0] = HWREG(0x4001A2A0);
	}
}

void Debug_TsensorBehind(int16_t adc_data)
{
     volatile int16_t tsensor_delt =0, pre_Data = 0;
     volatile uint8_t now_RF = 0, pre_RF = 0;

     if((HWREGB(0x4000400B) & 0x10) &&(HWREGB(0x40004005) & 0x10))
     {
		 adc_record[record_cnt].trx_status[1] = DFE_RxEn_IsActive() << 1| DFE_TxEn_IsActive();
		 adc_record[record_cnt].rx_pwrctrl[1] = HWREG(0x4001A230);
		 adc_record[record_cnt].tx_pwrctrl[1] = HWREG(0x4001A2A0);
     }

	 adc_record[record_cnt].debug_Vref = ADC_VREF_Get();
	 adc_record[record_cnt].debug_ldoana_2p7 = HWREGB(0x4000484C);
	 adc_record[record_cnt].debug_tsensorPower = HWREGB(0x40004848) & 0x07;

	 adc_record[record_cnt].debug_adc_data = adc_data;

	 adc_record[record_cnt].debug_reg[0] = HWREG(COREPRCM_ADIF_BASE + 0x44);
	 adc_record[record_cnt].debug_reg[1] = HWREG(COREPRCM_ADIF_BASE + 0x48);
	 adc_record[record_cnt].debug_reg[2] = HWREG(COREPRCM_ADIF_BASE + 0x4c);
	 adc_record[record_cnt].debug_reg[3] = HWREG(COREPRCM_ADIF_BASE + 0x50);

     
	 adc_record[record_cnt].tickTimer = Tick_CounterGet();     //get tick timer

	 if(first_sample_flag==0)//first
	 {
		first_sample_flag =1;
	 }
	 else if(Tick_CounterGet() < adc_record[(record_cnt<1) ? 3 : (record_cnt-1)].tickTimer+TEMP_DELTA_MS)    // 超过30秒，不排查跳变
	 {
		now_RF = adc_record[record_cnt].trx_status[1] & 0x01; //DFE_TxEn
		pre_RF = adc_record[(record_cnt<1) ? 3 : (record_cnt-1)].trx_status[1] & 0x01; //DFE_TxEn

		pre_Data = adc_record[(record_cnt<1) ? 3 : (record_cnt-1)].debug_adc_data;
		tsensor_delt = abs(adc_record[record_cnt].debug_adc_data - pre_Data);

		if(now_RF)
		{
			/*上次未开RF，这次开RF，温度只会变高*/
			if(pre_RF == 0)
				xy_assert(adc_data>pre_Data && (adc_data-pre_Data) < TEMP_DELTA_HIGH1);

			/*上次已开RF，这次也开RF，温度出入不应太大*/
			else
				xy_assert(tsensor_delt < TEMP_DELTA_LOW1);
		}
		else
		{
			/*上次开RF，这次未开RF，温度只会下降*/
			if(pre_RF == 1)
				xy_assert((pre_Data-adc_data) < TEMP_DELTA_HIGH2 && pre_Data > adc_data);

			/*上次未开RF，这次也未开RF，温度出入不应太大*/
			else
				xy_assert(tsensor_delt < TEMP_DELTA_LOW2);
		}
	 }
	 else
	 {

	 }

	 if(++record_cnt>3)
		 record_cnt = 0;
}
#endif
int16_t ADC_ReadData(uint8_t channelID,uint8_t CalibrationFlag,int16_t CurrentTmp, uint8_t Vref)
{
	int16_t adc_data = 0;
	int16_t covert_data = 0,voltage;
	uint16_t temperaCaliNV = 2250;

	/*去除编译warning*/
	(void)CurrentTmp;
	
	adc_data = ADCCTRL_ReadADCDataBlocking();

#if (ADC_DUBUG_INFO == 1)
	if((channelID == ADC_VBAT) && ((adc_data < LOW_ALARM_VBATVOL) ||(adc_data > HIGH_ALARM_VBATVOL)))
		xy_assert(0);
	
	if(channelID == ADC_TSENSOR)
    {
		Debug_TsensorBehind(adc_data);
	}

#endif

	ADC_SingleModeDeInit(channelID);
	covert_data = adc_data;
	
	if (CalibrationFlag == 1)               //
	{
		covert_data = ADC_ADCDataCaliWtihNV(channelID, adc_data);
		if(covert_data < 0)
		{
			covert_data = adc_data;
		}
	}
	
    if(channelID == ADC_TSENSOR)
    {
    	if((g_adc_cali_nv.code_tsensor >= 1200) && (g_adc_cali_nv.code_tsensor <= 2000) && (Vref ==ADC_VREF_VALUE_2P2V))
		{
			temperaCaliNV = g_adc_cali_nv.code_tsensor;
		}
		else if((g_adc_cali_nv.code_tsensor >= 2000) && (g_adc_cali_nv.code_tsensor <= 3000) && (Vref ==ADC_VREF_VALUE_1P5V))
		{
			temperaCaliNV = g_adc_cali_nv.code_tsensor;
		}
		else
		{

			if(Vref == ADC_VREF_VALUE_2P2V)
			{
			    temperaCaliNV = 1534;
			}
			else
			{
				temperaCaliNV = 2250;
			}

		}

    	covert_data -= temperaCaliNV;

    	if(Vref != ADC_VREF_VALUE_2P2V)
		{
			voltage = (covert_data << 4 ) / 100;
		}
		else
		{
			voltage = ((int32_t)covert_data) * 2347 / 10000;
		}

		if(voltage > 150)
		{
			voltage = 150;
		}
		else if(voltage < -40)
		{
			voltage = -40;
		}
		else
		{
		} 
	}
    else if(((channelID == ADC_AUX_ADC1) || (channelID == ADC_AUX_ADC2)) && (ADC_Get_Attenu()))
	{

		 voltage = (uint16_t)(((covert_data * 100) * (Vref *4)+ 2048)/4096);
	}
    else
    {

      	voltage = (uint16_t)(((covert_data * 100) * (channelID == ADC_VBAT ? (Vref *5) : Vref) + 2048) >> 12);
    }
	return voltage;  
}



/**
  * @brief Read the calibrated voltage value or temperature.
  * @param AdcClk: ADC clk.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param CalibrationFlag: 1:calibration flag valid.
  * @param CurrentTmp: The current temperature.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
  * @attention time consuming(HRC DIV4)：93us 				
 */
int16_t get_adc_value_quickly(uint8_t channelID,uint32_t mclk,uint16_t AdcClk,uint8_t CalibrationFlag,int16_t CurrentTmp, uint8_t Vref ) //KHZ
{
	int16_t adcVal; 

	ADC_SigleModeInit(channelID,mclk,AdcClk);
    ADC_Start();
	adcVal = ADC_ReadData(channelID,CalibrationFlag,CurrentTmp,Vref);	//read data and DeInit
	return adcVal;
}
/**
  * @brief Read the calibrated voltage value or temperature.
  * @param AdcClk: ADC clk.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param CalibrationFlag: 1:calibration flag valid.
  * @param CurrentTmp: The current temperature.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
 */
int16_t get_adc_value(uint8_t channelID,uint32_t mclk,uint16_t AdcClk,uint8_t CalibrationFlag,int16_t CurrentTmp, uint8_t Vref) //KHZ
{
	int16_t adcVal; 

	ADC_bbldoEnable();
	delay_func_us(20);//tRES1 > 30us

	ADC_SigleModeInit(channelID,mclk,AdcClk);
    ADC_Start();
	adcVal = ADC_ReadData(channelID,CalibrationFlag,CurrentTmp,Vref);	//read data and DeInit
	return adcVal;
}




/**
  * @brief ADC scan mode init
  * @param AdcClk :ADC clk
  * @param ADC_Channle: The address of ADC channel
  * @param ChannelNum:SCAN channel number
  * @retval None
*/

void ADC_ScanModeInit(uint32_t mclk,uint16_t AdcClk,const uint8_t *ADC_Channle,uint8_t ChannelNum)
{
	uint8_t  channel_num;
	uint8_t  Aux_gpio_ana = 0;
	volatile uint16_t divN = 0;
	uint16_t ChannelIdTotal = 0;
	uint16_t AdcChId = 0;
	
	volatile uint32_t ScanChannelSelectTmp = 0;
	
	volatile uint32_t div_ave = 0;
	ADCCTRL->ADC_CTRL0 |= (uint8_t)(0x1 << ADC_AUX_PU_Pos);	 //ADC PU

    divN = ADC_CalcDivn_By_AdcClk(mclk, AdcClk);
	
	div_ave = (divN >> 8) | ((divN & 0xff) << 8) | 0x0f0000; // set div and Ave(1,3) 
	
	for(channel_num = 0; channel_num < ChannelNum; channel_num++)
	{
		AdcChId = *(ADC_Channle + channel_num);		
		ChannelIdTotal |= ((uint16_t)0x01 << AdcChId);
		
		//ADCCTRL_AddScanChannel(channel_num, *(ADC_Channle + channel_num), divN,1,3,1);
		
		ScanChannelSelectTmp |= AdcChId << (channel_num << 2);
		
		ADCCTRL->scan_channel[channel_num] = 0x81;
		
//		ADCCTRL_ChannelConfig(channel_num, divN, 1, 3);
	    ADCCTRL->ADCCTRL_CFG[channel_num] = div_ave;
	}	
	
	ADCCTRL->scan_channel_select = ScanChannelSelectTmp;

	if(ChannelIdTotal & 0x100) //vbat
	{
		HWREGB(0x40004852) |= 0x10; // vbat mon enable
	}
	if(ChannelIdTotal & 0x80)//tsensor
	{
		if (AdcClk <= ADC_CLK_240K)
		{
			HWREGB(0x40004848) = (HWREGB(0x40004848) & 0xF8) | 0x1; //coreprcn_adif: aux_cntl[58:56]
		}
		else
		{
			HWREGB(0x40004848) = (HWREGB(0x40004848) & 0xF8) | 0x4;
		}
	}
	if(ChannelIdTotal & (0x01))   //GPIO 8
	{
		Aux_gpio_ana |= 0x20;
	}
	if(ChannelIdTotal & 0x02)   //GPIO 9
	{
		Aux_gpio_ana |= 0x10;
	}
	if(ChannelIdTotal & 0x08)   //GPIO 10
	{
		Aux_gpio_ana |= 0x08;
	}
	if(ChannelIdTotal & 0x10)	 //GPIO 11
	{
		Aux_gpio_ana |= 0x04;
	}
	if(ChannelIdTotal & 0x40)	 //GPIO 13
	{
		Aux_gpio_ana |= 0x02;
	}
	if(ChannelIdTotal & 0x20)	 //GPIO 12
	{
		Aux_gpio_ana |= 0x01;
	}
	else
	{
	}
	HWREGB(0x40004851) |= Aux_gpio_ana;
		
	ADCCTRL->ADC_SWITCH_TIME = 0x03;   
	ADCCTRL->ADC_AVE_NUM = 0x10;        //default turn num is 1
	ADCCTRL->ADC_CTRL3 = 0x08;         //enable scan mode
	//ADCCTRL->ADC_CTRL3 &= ~0x01;	    //disable DMA
	ADCCTRL->ADC_CTRL1 = 0x40;          //disable swap

	if(ADC_VREF_Get() == 2)
	{
		HWREGB(0x40004856) |= 0x40; //OPA0_out connnect to GND when OPA0 power off
	}
}

/**
  * @brief Read data into buffer at ADC scan mode,swap enable.
  * @param data: ADC sample data buffer.
  * @param adc_channel: ADC sample channel buffer.
  * @param cnt: The number of data to be read.
  * @retval The number of data that has been read actually.
*/
void ADC_ScanReadBuffDataBlocking(int16_t* result_data,uint8_t *result_channel, uint16_t cnt)
{
	uint32_t timeout = 0;
	volatile uint8_t dataValidFlg = 0;
	uint32_t adcData = 0;

	while(cnt > 0)
	{
		while(ADCCTRL_isFIFOEmpty())
		{
			if(timeout++ > 100000)
			{
				return ;
			}
		}

		adcData = ADCCTRL->ANAAUXADC;
		dataValidFlg = ADCCTRL->ADC_DIRECT_RDY;

		if (dataValidFlg & 0x2)
		{
			*(result_channel++) = (adcData & 0xF000)>>12;
			*(result_data++) = adcDataExtend(adcData & 0x0FFF);
			if(--cnt <= 0){break;}
		}

		if (dataValidFlg & 0x4)
		{
			*(result_channel++) = (adcData & 0xF0000000)>>28;
			*(result_data++) = adcDataExtend((adcData & 0x0FFF0000) >> 16);
			if(--cnt <= 0){break;}
		}
	}
}


/**
  * @brief Disable ADC scan init
  * @retval None
*/
void ADC_ScanModeDeInit(void)
{
	ADC_VbatPowerDIS(); // vbat mon disable

	if(ADC_VREF_Get() != 2 )
	{
		HWREGB(0x40004851) = 0;    // 红外收发验证需要屏蔽
	}
	else
	{
	    HWREGB(0x40004851) = 0x02;  // ADC OPA0_OUT connect to the PAD,红外收发验证需要屏蔽
	}

	//ADCCTRL_DelScanChannel(0xff);  //Delete scan channel        
	HWREG(0x40070000) = 0;
	HWREG(0x40070004) = 0;
	
	ADCCTRL->ADC_AVE_NUM = 0x00;           //clear scan_suspend_en
//	ADCCTRL->ADC_CTRL0 = 0x00;             //ADC Stop
//	ADCCTRL->ADC_CTRL3 = 0x02;	           //FLUSH 
	HWREG(0x40070008) = 0x02000000;        //stop and FLUSH 
}


/**
  * @brief Convert the ADC data to the calibrated voltage value or temperature.
  * @param ChannelNum: The number of channel.
  * @param result_data: The ADC sample result data group.
  * @param result_channel:The ADC sample result channel group.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
 */
void ADC_ScanDataProcessing(uint16_t ChannelNum, int16_t* result_data, const uint8_t *channel, uint8_t Vref)
{
	uint16_t cnt;
	uint16_t temperaCaliNV = 2250;
	//int16_t CurrentTmp = 25;

	uint8_t AdcChID = 0;
	int16_t DataTmp = 0;
	int16_t DataSamp = 0;
	
	for(cnt = 0; cnt < ChannelNum ;  cnt++)
	{
		AdcChID = *(channel + cnt);
	
		if(AdcChID == ADC_TSENSOR)
		{
			DataTmp = *(result_data + cnt);
			
			if (g_CalibrationFlag == 1)
			{
				DataTmp = ADC_ADCDataCaliWtihNV(AdcChID, DataTmp);
			}

				if((g_adc_cali_nv.code_tsensor >= 1200) && (g_adc_cali_nv.code_tsensor <= 2000) && (Vref ==ADC_VREF_VALUE_2P2V))
				{
					temperaCaliNV = g_adc_cali_nv.code_tsensor;
				}
				else if((g_adc_cali_nv.code_tsensor >= 2000) && (g_adc_cali_nv.code_tsensor <= 3000) && (Vref ==ADC_VREF_VALUE_1P5V))
				{
					temperaCaliNV = g_adc_cali_nv.code_tsensor;
				}
				else
				{
					if(Vref == ADC_VREF_VALUE_2P2V)
					{
						temperaCaliNV = 1534;
					}
					else
					{
						temperaCaliNV = 2250;
					}
				}

			DataTmp -= temperaCaliNV;
			*(result_data + cnt) = ADC_ConverToTempeture(DataTmp, Vref);
			//CurrentTmp = *(result_data + cnt);

			break;
		}
		else
		{

		}
	}
	
	for(cnt = 0; cnt <ChannelNum; cnt++)
	{
		AdcChID = *(channel + cnt);

		if(AdcChID != ADC_TSENSOR)
		{
			DataSamp =  *(result_data + cnt);
			DataTmp = DataSamp;

			if (g_CalibrationFlag == 1)
			{
				DataTmp = ADC_ADCDataCaliWtihNV(AdcChID, DataSamp);
				if(DataTmp < 0)
				{
					DataTmp = DataSamp;
				}
			}
			*(result_data + cnt) = (uint16_t)(ADC_ConverToValtage(AdcChID, DataTmp, Vref));

		}
	}
}

#if (ADC_DUBUG_INFO == 1)
#define LOW_ALARM_VOL	1000	  // 濡拷濞村娈戞稉瀣閻㈤潧甯囬崐鑲╁娑撶尨绱癓OW_ALARM_VOL/4096 *2.2(V)
#define HIGH_ALARM_VOL	4200	  // 濡拷濞村娈戞稉濠囨閻㈤潧甯囬崐鑲╁娑撶尨绱癏IGH_ALARM_VOL/4096 *2.2(V)
#define DEBUG_ADC_CHN   ADC_AUX_ADC2	//瀵板懏顥呭ù瀣畱ADC闁岸浜�
volatile int16_t g_debug_data_temp[26]={0};
volatile uint8_t g_debug_channel_temp[26]={0};
volatile uint8_t g_debug_ChannelNum;
volatile int16_t g_debug_result_data[13] = {0};
#endif
/**
  * @brief Read the calibrated voltage value or temperature by scan mode.
  * @param mclk: mclk,unit:K.
  * @param AdcClk: ADC clk.
  * @param ADC_Channle: The ADC channel group.
  * @param ChannelNum: The number of channel.
  * @param result_data: The ADC sample result data group.
  * @param result_channel:The ADC sample result channel group.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
 */
void ADC_ScanReadData(uint32_t mclk,uint16_t AdcClk, const uint8_t *ADC_Channle,uint16_t ChannelNum,int16_t* result_data, uint8_t Vref)
{
    uint8_t i=0, j=0, channel_temp[16]={0};
    int16_t data_temp[16]={0};
    
	ADC_bbldoEnable();
	delay_func_us(20);

	ADC_ScanModeInit(mclk,AdcClk,ADC_Channle,ChannelNum);
	//ADC_Start();
	delay_func_us(20);            //6.5M程序本身的延时是够的，但是提高到26M需要补10us,系统时钟为13M时最小延时为14us
	while(!ADC_IsActively_Trxbg());  //等待TRXBG RDY信号
	ADCCTRL->ADC_CTRL3 = 0x0A;  //FLUSH
#if(ADC_DUBUG_INFO == 1)
		Debug_TsensorBefore();
#endif
	ADCCTRL->ADC_CTRL0 |= 0x37;  //Sample EN

	ADC_ScanReadBuffDataBlocking(data_temp,channel_temp,(ChannelNum*2));

	for(j=0; j<ChannelNum; j++)
	{
		for(i=0; i < ChannelNum*2; i++)
		{
			if(channel_temp[i] == ADC_Channle[j])
			{
				result_data[j] = data_temp[i];
				break;
			}
		}
	}
	
#if (ADC_DUBUG_INFO == 1)

	for(i=0;i<g_debug_ChannelNum*2;i++)
	{
		g_debug_data_temp[i] = data_temp[i];
		g_debug_channel_temp[i] = channel_temp[i];
	}
	for(j=0;j<g_debug_ChannelNum;j++)
	{
		g_debug_result_data[j] = result_data[j];
		if((ADC_Channle[j] == DEBUG_ADC_CHN) &&
			((result_data[j] < LOW_ALARM_VOL) ||(result_data[j] > HIGH_ALARM_VOL)))
			xy_assert(0);

		if(ADC_Channle[j] == ADC_TSENSOR)
        {
			Debug_TsensorBehind(result_data[j]);
        }
	}
#endif

	ADC_ScanModeDeInit();
	ADC_ScanDataProcessing(ChannelNum,result_data,ADC_Channle,Vref);
}

/**
  * @brief Read the calibrated voltage value or temperature by scan mode,need enable BBLDO before reading ADC.
  * @param mclk: mclk,unit:K.
  * @param AdcClk: ADC clk.
  * @param ADC_Channle: The ADC channel group.
  * @param ChannelNum: The number of channel.
  * @param result_data: The ADC sample result data group.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
  * @attention time consuming(HRC DIV4)：// 2023/04/23，pcq 更新
  * 				1channel：146.6us
  *                 2channels：191.5us
  *                 3channels：233.4us
  *                 4channels：278.4us
  *                 5channels：321.1us
  * @note   此函数中包含ADC电压采集异常时的调试代码，当DEBUG_ADC_CHN采集到的电压小于LOW_ALARM_VOL或者大于HIGH_ALARM_VOL时，触发断言,
  * 客户可以根据实际使用情况调整相关的宏的取值，使用此断言时，可收集相关调试变量的值， 以及读取ADC相关寄存器的值，然后联系FAE.
 */ 


void ADC_ScanReadDataQuickly(uint32_t mclk,uint16_t AdcClk,const uint8_t *ADC_Channle,uint16_t ChannelNum,int16_t* result_data,uint8_t Vref)
{
	uint8_t i=0, j=0, channel_temp[26]={0};
	int16_t data_temp[26]={0};

	ADC_ScanModeInit(mclk,AdcClk,ADC_Channle,ChannelNum);
	//ADC_Start();
	delay_func_us(20);            //需要延时10us ,系统时钟为13M时最小延时为14us
	while(!ADC_IsActively_Trxbg());  //等待TRXBG RDY信号
	ADCCTRL->ADC_CTRL3 = 0x0A;  //FLUSH
#if(ADC_DUBUG_INFO == 1)
    Debug_TsensorBefore();
#endif
	ADCCTRL->ADC_CTRL0 |= 0x37;  //Sample EN

	ADC_ScanReadBuffDataBlocking(data_temp,channel_temp,(ChannelNum*2));

	for(j=0; j<ChannelNum; j++)
	{
		for(i=0; i < ChannelNum*2; i++)
		{
			if(channel_temp[i] == ADC_Channle[j])
			{
				result_data[j] = data_temp[i];
				break;
			}
		}
	}

#if (ADC_DUBUG_INFO == 1)

	for(i=0;i<g_debug_ChannelNum*2;i++)
	{
		g_debug_data_temp[i] = data_temp[i];
		g_debug_channel_temp[i] = channel_temp[i];
	}
	for(j=0;j<g_debug_ChannelNum;j++)
	{
		g_debug_result_data[j] = result_data[j];
		if((ADC_Channle[j] == DEBUG_ADC_CHN) &&
			((result_data[j] < LOW_ALARM_VOL) ||(result_data[j] > HIGH_ALARM_VOL)))
			xy_assert(0);

		if(ADC_Channle[j] == ADC_TSENSOR)
        {
			Debug_TsensorBehind(result_data[j]);
        }
	}
#endif
	
	ADC_ScanModeDeInit();
	ADC_ScanDataProcessing(ChannelNum,result_data,ADC_Channle,Vref);
}
/**
  * @brief Get the calibrated voltage value or temperature.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param AdcClk :Set ADC sample clock
  * @retval The ADC sample value with no calibration.
*/
int16_t get_adc_cali_nv(uint8_t channelID,uint32_t mclk,uint16_t AdcClk) //KHZ
{
	int16_t adcVal;

	ADC_SigleModeInit(channelID,mclk,AdcClk);
	ADC_Start();
	adcVal = ADCCTRL_ReadADCDataBlocking();

	ADC_SingleModeDeInit(channelID);

	return adcVal;
}

/**
  * @brief Get the ADC sample value of 0℃.
  * @param temperaVal: The temperature at calibration.
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval None.
*/
void get_tsensor_caliNv(uint32_t mclk,int16_t temperaVal,uint8_t Vref)
{
	int16_t adcVal;
	int16_t conv_val;

	ADC_SigleModeInit(ADC_TSENSOR,mclk,ADC_CLK_480K);
	ADC_Start();

	adcVal = ADCCTRL_ReadADCDataBlocking();

	ADC_SingleModeDeInit(ADC_TSENSOR);

//	conv_val = ADC_ADCDataCaliWtihNV(ADC_TSENSOR, adcVal);
	conv_val = adcVal;
   if(Vref != ADC_VREF_VALUE_2P2V)
   {
	   g_adc_cali_nv.code_tsensor = (uint16_t)(((conv_val<<4) - 100*temperaVal)>>4);
   }
   else
   {
	   g_adc_cali_nv.code_tsensor = (uint16_t)(conv_val -(uint32_t)temperaVal*10000/2347);
   }

}


int16_t ADC_CalibreationPresser(int32_t ADC_channel,int32_t ADC_voltage,uint32_t mclk,adc_cali_nv_t* adc_cali_nv,uint8_t smoke_flag)
{
	uint8_t channelID =0,nvOffset=0;
	int16_t regAdcVal = 0;

	if (ADC_channel == ADC_AUX_ADC1)       // ADC_channel_11   ADC1
	{
		channelID = ADC_AUX_ADC1;
		regAdcVal = get_adc_cali_nv(channelID,mclk,ADC_CLK_480K);
		nvOffset = 0;
	}
	else if (ADC_channel == ADC_AUX_ADC2)  // ADC_channel_12   ADC2
	{
		channelID = ADC_AUX_ADC2;
		regAdcVal = get_adc_cali_nv(channelID,mclk, ADC_CLK_480K);
		nvOffset = 1;
	}
	else if (ADC_channel == ADC_CMP_INP)   //GPIO8 ADC_channel_0   ADC3
	{
		channelID = ADC_CMP_INP;
		regAdcVal = get_adc_cali_nv(channelID,mclk,ADC_CLK_480K);
		nvOffset = 2;
	}
	else if (ADC_channel == ADC_OP0_OUT )   //GPIO13 ADC_channel_6  ADC4
	{
		channelID = ADC_OP0_OUT;

		ADC_SigleModeInit_OPACali(channelID,mclk,ADC_CLK_480K);
		ADC_Start();
		regAdcVal = ADCCTRL_ReadADCDataBlocking();
	
		ADC_SingleModeDeInit(channelID);
		nvOffset = 3;
	}
	else if (smoke_flag == 0 && ADC_channel == ADC_OP1_OUT )   //GPIO12 ADC_channel_5  ADC5
	{
		channelID = ADC_OP1_OUT;
		//fifo_code = get_adc_cali_nv(channelID, ADC_CLK_480K);
		ADC_SigleModeInit_OPACali(channelID,mclk,ADC_CLK_480K);
		ADC_Start();
		regAdcVal = ADCCTRL_ReadADCDataBlocking();

		ADC_SingleModeDeInit(channelID);

		nvOffset = 4;
	}
	else if (smoke_flag == 1 && ADC_channel == ADC_OP1_OUT )   //GPIO12 ADC_channel_6  ADC6
	{
		channelID = ADC_OP1_OUT;

		ADC_SigleModeInit_OPACali(channelID,mclk,ADC_CLK_480K);
		ADC_Start();
		regAdcVal = ADCCTRL_ReadADCDataBlocking();

		ADC_SingleModeDeInit(channelID);
		nvOffset = 5;
	}
	else if (ADC_channel == ADC_VBAT)   //GPIO8 ADC_channel_0   ADC3
	{
		channelID = ADC_VBAT;
		regAdcVal = get_adc_cali_nv(channelID,mclk,ADC_CLK_480K);
		nvOffset = 6;
	}
	else
	{
	}

	if(ADC_channel != ADC_VBAT)
	{
		if (ADC_voltage == 375)
		{
			*(&(adc_cali_nv->code_in_ADC1_375) + 2*nvOffset) = regAdcVal;
		}
		else if (ADC_voltage == 1125)
		{
			*(&(adc_cali_nv->code_in_ADC1_1125) + 2*nvOffset) = regAdcVal;
		}
	}
	else
	{
		if (ADC_voltage == 3020)
		{
			adc_cali_nv->code_Vbat_3020 = regAdcVal;
		}
		else if (ADC_voltage == 4670)
		{
			adc_cali_nv->code_Vbat_4670 = regAdcVal;
		}
	}

	return regAdcVal;
}

/**
  * @brief Get the calibrated voltage value or temperature.
  * @note  The PAD of OPA1_OUT is connected for testing.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param AdcClk :Set ADC sample clock
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
*/
uint16_t ADC_get_Smoke_value(uint8_t channelID, uint32_t mclk,uint16_t AdcClk,uint8_t Vref) //KHZ
{
	int16_t covert_data,adcVal;

	ADC_SigleModeInit(channelID,mclk,AdcClk);
	ADC_Start();
	adcVal = ADCCTRL_ReadADCDataBlocking();
	ADC_SingleModeDeInit(channelID);
	covert_data = adcVal;
    if(g_CalibrationFlag == 1)
    {
    	covert_data = ADC_OPACaliWtihNV(channelID,adcVal,SMOKE_TYPE);
    	if(covert_data < 0)
		{
			covert_data = adcVal;
		}
    }
	return((uint16_t)(ADC_ConverToValtage(channelID, covert_data,Vref)));
}
/**
  * @brief Get the calibrated voltage value or temperature.
  * @note  The PAD of OPA1_OUT is not connected in order to avoiding interference.
  * @param channelID: Select the ADC channel from 0 to 12.
  * @param AdcClk :Set ADC sample clock
  * @param Vref: Can be selected as the following :
  *    @arg ADC_VREF_VALUE_1P3V: 1.3V.
  *    @arg ADC_VREF_VALUE_1P5V: 1.5V(recommended).
  *    @arg ADC_VREF_VALUE_2P2V: 2.2V(for the Golden Card Project)
  *    @arg ADC_VREF_VALUE_3P2V: 3.2V.
  * @retval The calibrated voltage value or temperature.
*/

uint16_t ADC_FT_get_Smoke_value(uint8_t channelID, uint32_t mclk, uint16_t AdcClk, uint8_t Vref)
{
	int16_t covert_data,adcVal;
    ADC_SigleModeInit_OPACali(channelID,mclk,AdcClk);    //The PAD of OPA1_OUT is not connected.
	ADC_Start();
	adcVal = ADCCTRL_ReadADCDataBlocking();
	ADC_SingleModeDeInit(channelID);
	covert_data = adcVal;
    if(g_CalibrationFlag == 1)
    {
    	covert_data = ADC_OPACaliWtihNV(channelID,adcVal,SMOKE_TYPE);
    	if(covert_data < 0)
    	{
    		covert_data = adcVal;
    	}
    }
	return((uint16_t)(ADC_ConverToValtage(channelID, covert_data,Vref)));
}

uint16_t ADC_FT_get_OPAOnly_value(uint8_t channelID, uint32_t mclk,uint16_t AdcClk, uint8_t Vref)
{
	int16_t covert_data, adcVal;

	ADC_SigleModeInit_OPACali(channelID,mclk,AdcClk);
	ADC_Start();
	adcVal = ADCCTRL_ReadADCDataBlocking();
	ADC_SingleModeDeInit(channelID);

	covert_data = adcVal;
	if(g_CalibrationFlag == 1)
	{
		covert_data = ADC_OPACaliWtihNV(channelID,adcVal,OPA_ONLY_TYPE);
		if(covert_data < 0)
		{
			covert_data = adcVal;
		}
	}

	return((uint16_t)(ADC_ConverToValtage(channelID, covert_data, Vref)));
}
/**
  * @brief ADC int,used to OPA0 and OPA1 calibration.
  * @param channelID: Select the ADC channel from 0 to 12
  * @param AdcClk: set the ADC CLK.
  * @retval None
*/
void ADC_SigleModeInit_OPACali(uint8_t channelID,uint32_t mclk,uint16_t AdcClk)
{
	uint16_t divN = 1;
    uint8_t divflag=0,i=0,j=0;
	//VREF 1.5v
//	HWREGB(COREPRCM_ADIF_BASE + 0x46) = (HWREGB(COREPRCM_ADIF_BASE + 0x46) & 0xF3) | (0x01 << 2);
	
	switch(mclk)
	{
		case MCLK_26000K:
			i = 0;
		break;
		
		case MCLK_13000K:
			i = 1;
		break;
		
		case MCLK_6500K:
			i = 2;
		break;
		
		default:
			divflag=1;     
		 break;
	}
	switch(AdcClk)
	{
		case 960:
			j=0;
	    break;
		
		case 480:
			j=1;
	    break;
		
		case 240:
			j=2;
	    break;
		
		case 120:
			j=3;
	    break;
		
		default:
			j=1;
		break;
	}
	divN = DIV[i][j];
	
	if (AdcClk < ADC_CLK_500K)
	{
		HWREGB(COREPRCM_ADIF_BASE + 0x47) |= 0x4;                              //ADC clk duty cycle control 1:50%    0:25%
		ADCCTRL->ADC_CTRL0 = 0x08;	                                           //Duty Enable 	
		if(divflag)
		{
		    divN = (mclk/(AdcClk << 1))+ ((mclk*10/(AdcClk << 1))%10 > 5 ? 1 : 0); //HRC or XTAL as the source of is_iocl	
		} 
	}
	else
	{
		HWREGB(COREPRCM_ADIF_BASE + 0x47) &= 0xFB;                            //ADC clk duty cycle control 0:50%    1:25%
		ADCCTRL->ADC_CTRL0 = 0;	                                              //Duty Disable
		if(divflag)
		{
		    divN = (mclk/(AdcClk << 3)) + ((mclk*10/(AdcClk << 3))%10 > 5 ? 1 : 0);
		}
	}

	if (channelID == ADC_VBAT)
	{
		ADC_VbatPowerEN(); // vbat mon enable
	}
	else if (channelID == ADC_TSENSOR)
	{

		if (AdcClk <= ADC_CLK_240K)//Tsensor power mode control
		{
			HWREGB(COREPRCM_ADIF_BASE + 0x48) = (HWREGB(COREPRCM_ADIF_BASE + 0x48) & 0xF8) | 0x1;
		}
		else if (AdcClk <= ADC_CLK_480K)
		{
			HWREGB(COREPRCM_ADIF_BASE + 0x48) = (HWREGB(COREPRCM_ADIF_BASE + 0x48) & 0xF8) | 0x4;
		}
		else
		{
			HWREGB(COREPRCM_ADIF_BASE + 0x48) = (HWREGB(COREPRCM_ADIF_BASE + 0x48) & 0xF8) | 0x4;
		}
	}
	else
	{
	}	
	
	ADCCTRL_ChannelConfig(channelID, divN, 1, 2);
	ADCCTRL->ADC_CTRL3 = channelID << ADC_CHANNEL_NUM; //clear scan、DMA
	ADCCTRL->ADC_SWITCH_TIME = 3;
	HWREGB(ADC_REG_BASE + 0x09) = 0x40;   //SWAP =0
}

/**
* @brief The function only used to initial for the PlanB V2.2 in GoldCard Project
* @retval None
*/
void GPIO13_2P2_Config(void)
{

	HWREGB(COREPRCM_ADIF_BASE + 0x41)  |= 0x3F;

	//OPA0_INP select RDAC as input: OPA0_VREF_SEL_EN =0 && opa0_inp_chs =1
	HWREGB(COREPRCM_ADIF_BASE + 0x42)  &= ~0x10;
	HWREGB(COREPRCM_ADIF_BASE + 0x55)  = 0x11;     //opa0_inp_chs =1, opa0_rst =0, opa0_en =1

	HWREGB(COREPRCM_ADIF_BASE + 0x56)  = 0xF4;     //opa0_inp_EN =1, opa0_inn_EN =1

	HWREGB(COREPRCM_ADIF_BASE + 0x4B)  = 0x6F;     //SET RDAC_VREF:0x01  and RDAC_IN:0x2F

  //	ADC_VREF_Set(2);                               //ADC_VREF = 2.2v
	HWREGB(COREPRCM_ADIF_BASE + 0x4C) = 0x0E;      //LDOANA = 2.7v

	HWREGB(COREPRCM_ADIF_BASE + 0x54)  |= 0x01;    // Enable CMP

	HWREGB(COREPRCM_ADIF_BASE + 0x51) = 0x02;      //OPA0_OUT connect to PAD
}


/**
* @brief The function only used to get ADC value for the PlanB V2.2 in GoldCard Project
* @retval None
*/
uint16_t ADC_get_GoldCardVref22_value( uint8_t channelID, uint32_t mclk,uint16_t AdcClk,uint8_t CalibrationFlag, int16_t CurrentTmp,uint8_t Vref)
{
	int16_t covert_data,adcVal;

	GPIO13_2P2_Config();

	ADC_SigleModeInit(channelID,mclk,AdcClk);
	ADC_Start();
	adcVal = ADCCTRL_ReadADCDataBlocking();
	ADC_SingleModeDeInit(channelID);

	covert_data = adcVal;
	if(CalibrationFlag == 1)
	{
		covert_data = ADC_ADCDataCaliWtihNV(channelID, adcVal);
	}
	CurrentTmp = (CurrentTmp - 25);

	return((uint16_t)(ADC_ConverToValtage(channelID, covert_data, Vref)));
}


/**
* @brief The function used to calculate the ADC calibration parameters
* @retval None
*/
__FLASH_FUNC void ADC_CaliValueCaculate(adc_cali_data_result* gain_offset_result,uint8_t Vref)
{
	if(ADC_Get_Attenu())  //ADC2 High voltage
	{
		if(Vref == ADC_VREF_VALUE_1P5V)
		{
			if(g_adc_cali_nv.code_in_ADC2H_1125 != g_adc_cali_nv.code_in_ADC2H_3375)
			{

				gain_offset_result->adc2_high_gainerror = 153600000 / (g_adc_cali_nv.code_in_ADC2H_3375 - g_adc_cali_nv.code_in_ADC2H_1125);
				gain_offset_result->adc2_high_offset = 76800000 - (ADC2H15_gainerror * g_adc_cali_nv.code_in_ADC2H_1125);
		    }
			else if(g_adc_cali_nv.code_in_ADC2_1125 != g_adc_cali_nv.code_in_ADC2_375)    // used the adc2 low voltage calibration parameter
			{
				gain_offset_result->adc2_high_gainerror = 204800000 / (g_adc_cali_nv.code_in_ADC2_1125 - g_adc_cali_nv.code_in_ADC2_375);
				gain_offset_result->adc2_high_offset = (102400000 - (ADC2_gainerror * g_adc_cali_nv.code_in_ADC2_375));
			}
			else
			{
				gain_offset_result->adc2_high_gainerror = 1;
				gain_offset_result->adc2_high_offset = 0;
			}
		}
		else if(Vref == ADC_VREF_VALUE_2P2V)
		{
			if(g_adc_cali_nv.code_in_ADC2H_1125 != g_adc_cali_nv.code_in_ADC2H_3375)       // used the adc2 high voltage calibration parameter
			{

				gain_offset_result->adc2_high_gainerror = 104700000 / (g_adc_cali_nv.code_in_ADC2H_3375 - g_adc_cali_nv.code_in_ADC2H_1125);
				gain_offset_result->adc2_high_offset = 52400000 - (ADC2H22_gainerror * g_adc_cali_nv.code_in_ADC2H_1125);
			}
			else if(g_adc_cali_nv.code_in_ADC2_1125 != g_adc_cali_nv.code_in_ADC2_375)    // used the adc2 low voltage calibration parameter
			{
				gain_offset_result->adc2_high_gainerror = 204800000 / (g_adc_cali_nv.code_in_ADC2_1125 - g_adc_cali_nv.code_in_ADC2_375);
				gain_offset_result->adc2_high_offset = (102400000 - (ADC2_gainerror * g_adc_cali_nv.code_in_ADC2_375));
			}
			else
			{
				gain_offset_result->adc2_high_gainerror = 1;
				gain_offset_result->adc2_high_offset = 0;
			}
		}
	}
	else
	{
		if(g_adc_cali_nv.code_in_ADC2_1125 != g_adc_cali_nv.code_in_ADC2_375)
		{
			gain_offset_result->adc2_gainerror = 204800000 / (g_adc_cali_nv.code_in_ADC2_1125 - g_adc_cali_nv.code_in_ADC2_375);
			gain_offset_result->adc2_offset = (102400000 - (ADC2_gainerror * g_adc_cali_nv.code_in_ADC2_375));
		}
		else
		{
			gain_offset_result->adc2_gainerror = 1;
			gain_offset_result->adc2_offset = 0;
		}
	}
	

	if(g_adc_cali_nv.code_in_GPIO8_1125 != g_adc_cali_nv.code_in_GPIO8_375)
	{ 
		gain_offset_result->gpio8_gainerror = 204800000 / (g_adc_cali_nv.code_in_GPIO8_1125 - g_adc_cali_nv.code_in_GPIO8_375);
		gain_offset_result->gpio8_offset = 102400000 - (GPIO8_gainerror * g_adc_cali_nv.code_in_GPIO8_375);
	}
	else 
	{
		gain_offset_result->gpio8_gainerror = 1;
		gain_offset_result->gpio8_offset = 0;
	}
	

	if(g_adc_cali_nv.code_in_OPA0_1125 != g_adc_cali_nv.code_in_OPA0_375)
	{
		gain_offset_result->opa0_gainerror = 204800000 / (g_adc_cali_nv.code_in_OPA0_1125 - g_adc_cali_nv.code_in_OPA0_375);
		gain_offset_result->opa0_offset = 102400000 - (OPA0_gainerror * g_adc_cali_nv.code_in_OPA0_375);
	}
	else 
	{
		gain_offset_result->opa0_gainerror = 1;
		gain_offset_result->opa0_offset = 0;
	}
	

   if(g_adc_cali_nv.code_in_OPA1_1125 != g_adc_cali_nv.code_in_OPA1_375)
	{
		gain_offset_result->opa1_gainerror = 204800000 / (g_adc_cali_nv.code_in_OPA1_1125 - g_adc_cali_nv.code_in_OPA1_375);
		gain_offset_result->opa1_offset = 102400000 - (OPA1_gainerror * g_adc_cali_nv.code_in_OPA1_375);
	}
	else 
	{
		gain_offset_result->opa1_gainerror = 1;
		gain_offset_result->opa1_offset = 0;
	}
	

	if(g_adc_cali_nv.code_in_OPA0_OPA1_1125 != g_adc_cali_nv.code_in_OPA0_OPA1_375)
	{
		gain_offset_result->opa0_opa1_gainerror = 204800000 / (g_adc_cali_nv.code_in_OPA0_OPA1_1125 - g_adc_cali_nv.code_in_OPA0_OPA1_375);
		gain_offset_result->opa0_opa1_offset = 102400000 - (OPA0_OPA1_gainerror * g_adc_cali_nv.code_in_OPA0_OPA1_375);
	}
	else 
	{
		gain_offset_result->opa0_opa1_gainerror = 1;
		gain_offset_result->opa0_opa1_offset = 0;
	}
	

	if(g_adc_cali_nv.code_Vbat_4670 != g_adc_cali_nv.code_Vbat_3020)
	{

		if(Vref == ADC_VREF_VALUE_1P5V)
		{
			gain_offset_result->vbata_gainerror = 90100000 / (g_adc_cali_nv.code_Vbat_4670 - g_adc_cali_nv.code_Vbat_3020);
			gain_offset_result->vbata_offset =    164900000 - (VBAT15_gainerror * g_adc_cali_nv.code_Vbat_3020);
		}
		else if(Vref == ADC_VREF_VALUE_2P2V)
		{

			gain_offset_result->vbata_gainerror = 61500000 / (g_adc_cali_nv.code_Vbat_4670 - g_adc_cali_nv.code_Vbat_3020);
			gain_offset_result->vbata_offset =    112400000 - (VBAT22_gainerror * g_adc_cali_nv.code_Vbat_3020);
		}
		else
		{
		}
	}
	else 
	{
		gain_offset_result->vbata_gainerror = 1;
		gain_offset_result->vbata_offset = 0;
	}
	
}


/**
* @brief This function is used to initialize the necessary configuration of the ADC during the quick recovery process
* @retval None
*/
void ADC_recover_init(void)
{
	if(g_trxbg_CaliReg != 0xFFFFFFFF)   //TRXBG Calibration
	{
		ADC_Trxbg_Set_CaliValue(g_trxbg_CaliReg);
	}
}
