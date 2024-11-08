#include "adc_adapt.h"

uint8_t g_ADCVref = 0;
uint8_t g_ADCRange = 0;

/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！获取adc参考电压，并读取相应的校准参数        
* @attention 先读OTP，失败后读RF NV的温度校准信息 
*****************************************************************************************************/
__FLASH_FUNC void get_adc_caliv()
{
	uint8_t adc_reflag = READ_FAC_NV(uint8_t,adc_volrange);
	
	if(adc_reflag == 0)  	//0-1.5V量程,精度好	
		g_ADCVref = ADC_VREF_VALUE_1P5V;	
	
	else if(adc_reflag == 1) //0-2.2V量程,精度较差，此量程下GPIO13不能用作ADC通道，AUX_AD1/AUX_ADC2受限于供电，可能不能达到2.2V量程	
		g_ADCVref = ADC_VREF_VALUE_2P2V;
	
	else if(adc_reflag == 2) //0-VBAT量程,仅AUX_AD1/AUX_ADC2支持，,精度较差
	{
		g_ADCVref = ADC_VREF_VALUE_1P5V; //adc_reflag == 2时开启AUX_ADC1/AUX_ADC2的分压，量程变为0-VBAT,但是参考电压还是1.5v
		g_ADCRange = ADC_RANGE_VBAT; 
		ADC_Enable_Attenu();
	}
	else	
		xy_assert(0);		
    
	//存在有效的校准信息,则读取
	if (HWREG(BAK_MEM_OTP_ADCAL_BASE)  == 1) 
	{
		g_CalibrationFlag = true;
		memcpy((void *)&(g_adc_cali_nv.code_in_ADC1_375), (void *)(BAK_MEM_OTP_ADCAL_BASE + 4), BAK_MEM_OTP_ADCAL_LEN-4);
		ADC_CaliValueCaculate(&g_adc_cali_result, g_ADCVref);

		memcpy((void *)&(g_trxbg_CaliReg), (void *)(BAK_MEM_OTP_TRXBGCAL_BASE), BAK_MEM_OTP_TRXBGCAL_LEN);
		if(g_trxbg_CaliReg != 0xFFFFFFFF)
		{
			ADC_Trxbg_Set_CaliValue(g_trxbg_CaliReg);
		}

	}

	//温度无校准时，其值误差非常大，不可接受，电压无校准时，误差为几十mV，勉强接受
	//无有效的校准信息,则读取射频nv中的校准参数,但此处只有温度的校准参数,以保证后续温度读值可用
	else 
	{
		g_CalibrationFlag = false;
//		assert(!"NON RF NV !");

		if( g_ADCVref == ADC_VREF_VALUE_1P5V )
			g_adc_cali_nv.code_tsensor = HWREGH(NV_FLASH_RF_BASE+ 0x3CC);  //1.5V参考电压的温度校准参数

		else 
			g_adc_cali_nv.code_tsensor = HWREGH(NV_FLASH_RF_BASE + 0x3CE);  //2.2V参考电压的温度校准参数

		if((HWREG(NV_FLASH_RF_BASE )!= RF_MT_NV_VERSION) \
				|| (HWREG(NV_FLASH_RF_BASE + 0x08)!= 0x55aa55aa) || (HWREG(NV_FLASH_RF_BASE + 0x3C8)!= 0xaa55aa55))
		{
			if( g_ADCVref == ADC_VREF_VALUE_1P5V )
				g_adc_cali_nv.code_tsensor = HWREGH(NV_FLASH_RF_BAKUP_BASE + 0x3CC);  //1.5V参考电压的温度校准参数
			else 
				g_adc_cali_nv.code_tsensor = HWREGH(NV_FLASH_RF_BAKUP_BASE + 0x3CE);  //2.2V参考电压的温度校准参数
		}
	}
}

/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！用于开启AD转换时，需要使用的TRXBG与BBLDO时钟  
* @attention 在HRC 4分频时，且相关代码位于RAM上时，耗时 15us   
*****************************************************************************************************/
void ADC_Set_Trxbg_Bbldo()
{
	//低频率使用ADC，当选用HRC时钟源时，需要打开ADC参考电压时钟源
	if(SYSCLK_SRC_HRC == SYS_CLK_SRC)
	{
		COREPRCM->ANATRXBG_CTL = 0x01;		
	}
	ADC_bbldoEnable();
}

/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！关闭TRXBG时钟  
* @attention 在HRC 4分频时，且相关代码位于RAM上时，耗时 8us     
*****************************************************************************************************/
void ADC_Reset_Trxbg(void)
{
	if(SYSCLK_SRC_HRC == SYS_CLK_SRC)
	{
		COREPRCM->ANATRXBG_CTL = 0x00;				
	}
}

/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！供底层快速读取当前芯片电压，不走标准HAL接口      
*****************************************************************************************************/
int16_t get_adc_vbat_quickly()
{
    int16_t temp = 0;

    PRCM_ClockEnable(CORE_CKG_CTL_ADC_EN);

    // 当时钟源使用HRC时，使用ADC时开启adc相关电源
    if(PRCM_SysclkSrcGet() == SYSCLK_SRC_HRC)
    {
        COREPRCM->ANATRXBG_CTL = 0x01;
    }
    ADC_bbldoEnable(); //开ADC时钟
    delay_func_us(70);

    ADC_Select_Vref(g_ADCVref);
    
    ADC_TsensorPowerEN(); 
    temp = get_adc_value_quickly(ADC_VBAT, GetlsioFreq() / 1000, ADC_CLK_480K, g_CalibrationFlag, 0,g_ADCVref);
    ADC_TsensorPowerDIS();

    if(PRCM_SysclkSrcGet() == SYSCLK_SRC_HRC)
    {
        COREPRCM->ANATRXBG_CTL = 0x00;
    }

    return temp;
}

/*****************************************************************************************************
* @brief   配置GPIO13，使其输出2V电压
* @param :  NA
* @return   NA
* @attention 1、此功能相关寄存器会在深睡时掉电，故在深睡唤醒时，需要重新配置
* 			 2、此功能使用时要求VBAT必须大于2.9V
*****************************************************************************************************/
uint8_t g_gpio13_2V = 0; //标识GPIO13是否用于输出2V。1：GPIO13用于输出2V，0：GPIO13不用于输出2V

void Sp_Gpio13_2V_En(void)
{
	ADC_Set_Trxbg_Bbldo();
	delay_func_us(70);
	ADC_Select_Vref(g_ADCVref);

    GPIO13_2P2_Config();
    g_gpio13_2V = 1;
}

/*****************************************************************************************************
* @brief   关闭GPIO13 2V电压输出，输出0V
* @param :  NA
* @return   NA
* 注意：此功能相关寄存器会在深睡时掉电，故在深睡唤醒时，需要重新配置
*****************************************************************************************************/
void Sp_Gpio13_2V_Dis(void)
{
	 ADC_OPA0_Disable(); //gpio13输出0V
	 g_gpio13_2V = 0;
}

/*****************************************************************************************************
 * @brief  获取指定单通道ADC的采样值
 * @param  Channel 详情参考 @ref ADC_CHANNEL_TypeDef.
 * @retval 若返回温度信息，单位为摄氏度；若返回电压信息，单位为mV；若入参错误返回-32768.
*****************************************************************************************************/
inline int16_t ADC_Single_GetValue(ADC_CHANNEL_TypeDef Channel)
{
    if(Channel > ADC_AUX_ADC2)
    {
        return -32768;
    }

    //阻塞等待获取核间的硬件互斥锁
    uint32_t sema_have = 0;
	if(CP_Is_Alive() == true)
	{
		do{
            SEMA_RequestNonBlocking(SEMA_SLAVE_AUXADC, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
        }while(SEMA_MASTER_AP != SEMA_MasterGet(SEMA_SLAVE_AUXADC));
		sema_have = 1;
	}

    //使能TRXBG时钟，使能BBLDO电源
	ADC_Set_Trxbg_Bbldo();

    //使能ADC时钟
	PRCM_ClockEnable(CORE_CKG_CTL_ADC_EN);

    //设置ADC参考电压
	ADC_Select_Vref(g_ADCVref);

    //使能内部温度传感器
	if((CP_Is_Alive() == false) && (Channel == ADC_TSENSOR))
	{
		ADC_TsensorPowerEN(); 
	}

	//复用GPIO的ADC通道需要初始化为模拟引脚
	if(Channel <= ADC_CMP_INN)
	{
		ADC_GPIO_Init(GPIO_PAD_NUM_8 + Channel);
	}
    else if((Channel >= ADC_OP1_INP) && (Channel <= ADC_OP0_OUT))
    {
		ADC_GPIO_Init(GPIO_PAD_NUM_7 + Channel);
	}

	//配置ADC1、ADC2的分压功能
	else if((Channel == ADC_AUX_ADC1) || (Channel == ADC_AUX_ADC2))
	{
        (g_ADCRange == ADC_RANGE_VBAT) ? ADC_Enable_Attenu() : ADC_Disable_Attenu();
	}
	
    //SINGLE模式获取单通道电压值、温度值
	int16_t value = get_adc_value_quickly(Channel, GetlsioFreq() / 1000, ADC_CLK_480K, g_CalibrationFlag, 0, g_ADCVref);

    //禁能内部温度传感器
	if((CP_Is_Alive() == false) && (Channel == ADC_TSENSOR))
	{
		ADC_TsensorPowerDIS();
	}

    //若GPIO13不用于输出2V，则关闭TXRBG时钟
	if(!g_gpio13_2V)
	{
		ADC_Trxbg_SafelyClose();// ADC_Reset_Trxbg();
	}

    //释放核间硬件互斥锁
	if(sema_have == 1)
	{
		SEMA_Release(SEMA_SLAVE_AUXADC, SEMA_MASK_NULL);
	}

	return value;
}

/**
 * @brief  ADC使用扫描模式获取指定多通道的采样值
 * @param  pAdcScan 详情参考 @ref ADC_Scan_HandleTypeDef.
 * @return 0:成功，-32768:失败
 */
inline int16_t ADC_Scan_GetValue(ADC_Scan_HandleTypeDef *pAdcScan)
{
    if((pAdcScan == NULL) || (pAdcScan->ScanNum > ADC_SCAN_MAX_CHANNELS))
    {
        return -32768;
    }

    uint8_t i, subscript = 0xFF;

    //阻塞等待获取核间的硬件互斥锁
	uint32_t sema_have = 0;
	if (CP_Is_Alive() == true)
	{
		do{
			SEMA_RequestNonBlocking(SEMA_SLAVE_AUXADC, SEMA_SEMA_DMAC_NO_REQ,SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
        } while (SEMA_MASTER_AP != SEMA_MasterGet(SEMA_SLAVE_AUXADC));
        sema_have = 1;
	}

    //使能TRXBG时钟，使能BBLDO电源
	ADC_Set_Trxbg_Bbldo();
	
    //使能ADC时钟
	PRCM_ClockEnable(CORE_CKG_CTL_ADC_EN);

    //设置ADC参考电压
    ADC_Select_Vref(g_ADCVref);

    //初始化ADC通道
    for(i = 0; i < pAdcScan->ScanNum; i++)
    {
		if(pAdcScan->Channel[i] == ADC_TSENSOR)
		{
            //SCAN模式下ADC持续工作会导致芯片温度上升，因此TSENSOR通道必须放在待转换通道数组的首个元素中，此时采样所得温度与SINGLE模式一致，若放后面元素中则采样所得温度会高1~2℃
            subscript = i; //记录下标
            pAdcScan->Channel[i] = pAdcScan->Channel[0]; //交换通道号
			pAdcScan->Channel[0] = ADC_TSENSOR;
			
            //使能内部温度传感器
			if(CP_Is_Alive() == false)
			{
				ADC_TsensorPowerEN(); 
			}
		}
		else
		{
	        //复用GPIO的ADC通道需要初始化为模拟引脚
	        if (pAdcScan->Channel[i] <= ADC_CMP_INN)
            {
	            ADC_GPIO_Init(GPIO_PAD_NUM_8 + pAdcScan->Channel[i]);
            }
	        else if((pAdcScan->Channel[i] >= ADC_OP1_INP) && (pAdcScan->Channel[i] <= ADC_OP0_OUT))
            {
                ADC_GPIO_Init(GPIO_PAD_NUM_7 + pAdcScan->Channel[i]);
            }
            
	        //配置ADC1、ADC2的分压功能
			else if((pAdcScan->Channel[i] == ADC_AUX_ADC1) || (pAdcScan->Channel[i] == ADC_AUX_ADC2))
			{
                (g_ADCRange == ADC_RANGE_VBAT) ? ADC_Enable_Attenu() : ADC_Disable_Attenu();
			}
		}
	}
	
    //SCAN模式获取单通道电压值、温度值
    ADC_ScanReadDataQuickly(GetlsioFreq() / 1000, ADC_CLK_480K, (const uint8_t *)pAdcScan->Channel, pAdcScan->ScanNum, (int16_t *)pAdcScan->Value, g_ADCVref);

    //若有温度采集，则按入参顺序排列通道号和采样值
    if(subscript != 0xFF)
    {
        //交换回通道号
        pAdcScan->Channel[0] = pAdcScan->Channel[subscript]; 
        pAdcScan->Channel[subscript] = ADC_TSENSOR;

        //交换采样值
        uint16_t TempValue = pAdcScan->Value[0];
        pAdcScan->Value[0] = pAdcScan->Value[subscript];
        pAdcScan->Value[subscript] = TempValue;

        //禁能内部温度传感器
        if(CP_Is_Alive() == false)
        {
            ADC_TsensorPowerDIS();
        }
    }

    //若GPIO13不用于输出2V，则关闭TXRBG时钟
    if(!g_gpio13_2V)
	{
		ADC_Trxbg_SafelyClose();// ADC_Reset_Trxbg();
	}
	
    //释放核间硬件互斥锁
	if(sema_have == 1)
	{
		SEMA_Release(SEMA_SLAVE_AUXADC, SEMA_MASK_NULL);
	}

    return 0;
}

/*****************************************************************************************************
* @brief  记录trxbg关闭的时刻点   
*****************************************************************************************************/
static uint32_t s_trxbg_last_close_tick = 0;

/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！立即关闭trxbg，目前仅用在standby睡眠前，确保standby期间是关闭的  
*****************************************************************************************************/
void ADC_Trxbg_InstantClose(void)
{
    if(COREPRCM->ANATRXBG_CTL & 0x01)
    {
        s_trxbg_last_close_tick = TICK->COUNTER;
        COREPRCM->ANATRXBG_CTL = 0;
    }
}

/*****************************************************************************************************
* @brief  避免trxbg短时间内的频繁关闭,间隔1ms,依赖aptick计数功能     
*****************************************************************************************************/
void ADC_Trxbg_SafelyClose(void)
{
    DisablePrimask();
    
    uint32_t cur_tick = TICK->COUNTER;
    //trxbg打开状态并且据上次关闭超出1ms
    if( (COREPRCM->ANATRXBG_CTL & 0x01) && ((cur_tick - s_trxbg_last_close_tick) > 1))
    {
        s_trxbg_last_close_tick = cur_tick;
        COREPRCM->ANATRXBG_CTL = 0;  
    }

    EnablePrimask();
}