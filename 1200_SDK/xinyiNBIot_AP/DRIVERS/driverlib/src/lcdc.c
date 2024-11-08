#include "lcdc.h"
#include "xy_timer.h"

/*****************************************************************************************************************/
/******************************************************************************************************************
  * @brief 这是default初始化的接口，使用LCD默认参数配置，无入参;     .   .
  * @param  None
  * @param  Segpad 芯翼seg编号，从0开始
  * @param  ComPad 芯翼com编号，从0开始
  * 初始化流程：
             1. LCD模式pad使能；
  					 2. LCD电源配置为force on；
  					 3. CTRL0：bypassen 0 bias-0x7 Comnum-8
  					 4. CTRL1：全部配置为缺省值 0
  					 5. UVOL：全部使用缺省值 0
  					 6. FDR：rps-0x2 div 0xF
  					 7. 关闭LCD显示
  * @retval 无
*******************************************************************************************************************/
void LCDC_Init_Default(uint32_t Segpad,uint32_t ComPad)
{
  /*配置LCD端口使能 */
  LCDC_PAD_SetMode(Segpad,ComPad);
  /*配置LCD端口使能 */
  // LCDC_PAD_Enable();
  /*配置LCDForce On*/
  LCDC_PWR_Force_On();
  /*配置CTRL0，Duty、BIAS必选设置 */
  LCDC_CTRL0Set(0,0x7,8,0);
  /*配置CTRL1，LWAVE、Dead、Blink必选设置 */
  LCDC_CTRL1Set(0,0,0,0);
  /*配置UVol，CLK*/
  LCDC_UVolSet(0,0,0,0);
  /*配置FDR，PS和DIV必选设置 */
  LCDC_FDRSet(0x1,0x5); 
  /*停止LCD显示*/
  LCDC_WORKStop(); 
}

/*****************************************************************************************************************/
/******************************************************************************************************************
  * @brief 这是带入参初始化的接口，用户可修改8个常用的LCDC参数，其他配置与缺省初始化保持一致     .   .
  * @param 可配参数：
	           1. mux，这是一个可配置com口数量的控制位，控制器默认只有一个Com，Com0
                该参数用来配置额外七个可选口
                是作为com1-com7或者seg34-seg28使用，0:com 1:seg 只有低7bit有效
						 2、3. rps、div为LCD的分频参数，决定了LCD的工作频率；
						 4. SupplySel，供电电源设置
													1: when VDDIO==3V and VLCD>3V(default)
													0: whenVDDIO==1.8V  or (VDDIO==3V and VLCD<3V)

						 5. VLCDchs  供电方式选择    default  10
													00  External VBAT;
													01  Internal  VBAT;
													10  LCP(starts to power up)
													11  Forbidden
						 6. Duty     占空比，时间片模式选择
												0:static
												1:1/2 duty
												2:1/3 duty
												3:1/4 duty
												4:1/5 duty
												5:1/6 duty
												6:1/7 duty
												7:1/8 duty
						 7. Bias    偏置电压规则
												0:if duty=0
												1:1/2 bias
												2:1/3 bias
												3:1/4 bias

						 8. BypassEn  0/1选择内部分压位或者外部分压 
  * @retval 无
*******************************************************************************************************************/
void LCDC_Init_Config(uint8_t mux,uint8_t rps,uint8_t div,uint8_t SupplySel,uint8_t VLCDchs,uint8_t Duty,uint8_t Bias,uint8_t BypassEn)
{

	 /*配置LCD端口使能 */
   //LCDC_PAD_Enable_All();
	 /*配置LCDForce On*/
   LCDC_PWR_Force_On();
   /*调用接口配置Com0-Com0,全0表示复用口全部配置为Com口*/
   LCDC_ComsegmuxSet(mux); 
   /*配置FDR，PS*/
   LCDC_FCRpsSet(rps);     
   LCDC_FCRdivSet(div); 
	 /*配置CTRL1，LWAVE、Dead、Blink默认值 */
   LCDC_CTRL1Set(0,0,0,0);
   /*配置UVol，CLK必配*/
   LCDC_UVolSet(0,0,SupplySel,VLCDchs);
   /*调试口tmux_sel，仅内部用，缺省值*/
   LCDC_TmuxSelSet(0); 
   /*配置duty*/
   LCDC_DutySet(Duty);
   /*配置BIAS*/
   LCDC_BiasSet(Bias);
   /*选择内部分压或外部分压*/
   LCDC_BypassEnSet(BypassEn);
	 /*停止LCD显示*/
	 LCDC_WORKStop(); 
}
/*****************************************************************************************************
  * @brief  Set LCD pad mode.
  * @param  none
  * @retval none
  ***************************************************************************************************/ 
void LCDC_PAD_SetMode(uint32_t Segpad,uint32_t ComPad)
{
    LCDC->LCD_SEG_PAD_CTRL = (Segpad & 0xFFFFFFFF);
    LCDC->LCD_COM_PAD_CTRL = (ComPad & 0xFF);
}

/*************************************************************************************************
  * @brief 这是写某个COM寄存器x的接口，ComX为COM口的下标，每个com口我们都是对应           
  *  写入其低32bits和高3bits，共35bits的值，对应35个seg口的控制信号     .
  * @param  ComX:        用户使用的COM口下标号
  * @param  ValueLow:    需要写入的控制逻辑,低32位，对应控制Seg0-Seg31；
  * @param  ValueHigh:   需要写入的控制逻辑,高32位，只有低3bits有效，对应控制Seg32-Seg34
  * @retval 这个函数把控制序列分别写入某个特性com寄存器的高低32bits；
**************************************************************************************************/
void LCDC_WriteComXValue(uint8_t ComX,  uint32_t ValueLow, uint32_t ValueHigh)
{
	 switch(ComX)                       
    { 
			case 0:
				LCDC->COM0LOW = ValueLow;
			  LCDC->COM0HIGH = ValueHigh;
                 break;
			case 1:
				LCDC->COM1LOW = ValueLow;
			  LCDC->COM1HIGH = ValueHigh;
                 break;
			case 2:
				LCDC->COM2LOW = ValueLow;
			  LCDC->COM2HIGH = ValueHigh;
                 break;
			case 3:
				LCDC->COM3LOW = ValueLow;
			  LCDC->COM3HIGH = ValueHigh;
                 break;
			case 4:
				LCDC->COM4LOW = ValueLow;
			  LCDC->COM4HIGH = ValueHigh;
                 break;
			case 5:
				LCDC->COM5LOW = ValueLow;
			  LCDC->COM5HIGH = ValueHigh;
                 break;
			case 6:
				LCDC->COM6LOW = ValueLow;
			  LCDC->COM6HIGH = ValueHigh;
                 break;
			case 7:
				LCDC->COM7LOW = ValueLow;
			  LCDC->COM7HIGH = ValueHigh;
                 break;
       default:  
                 break;
		}			
}
	
/*********************************************************************************************************/
/*********************************************************************
**
  * @brief 这是写COM寄存器的总接口，每次显示需要写的com寄存器数量和用户选用的
     COM口总数有关，使用8个COM则每次需依次设置COM0-COM7的值;     .
  * @param  ComNum:      用户使用的COM口总数量
  * @param  SegNum:      用户使用的SEG口总数量
  * @param  SegGroup:    用户选用SEG口下标组合，例如{16，17，18，19，24，25}表示使用Seg16-19、24-25共六个口
  * @param  UserPattern: 用户预期的输入内容，与不同LCD的编码有关，可以看作某个特性SEG的com口输入逻辑序列；
  * @retval 这个函数没有返回值，它会向用户指定数量的Com寄存器写入显示值.
**********************************************************************************************************/
void LCDC_WriteCom(uint8_t ComNum,uint8_t SegNum,uint8_t *SegGroup,uint8_t *UserPattern)
{
   uint8_t i=0;
	 uint32_t ValueL[8]={0},ValueH[8]={0};
   for(i=0;i<ComNum;i++)
   {
			LCDC_GetSeg(i,SegNum,SegGroup,UserPattern,&ValueL[i],&ValueH[i]);
			LCDC_WriteComXValue(i,ValueL[i], ValueH[i]);
	 }
}

/*****************************************************************************************************************/
/******************************************************************************************************************
  * @brief 这是获取SEG逻辑的接口，也就是每个特性Com口下，所选Seg口的配置集合，设置的内容和选择使用的Seg端口有关，
          如果选择低32位的端口显示内容左SegX即可，SegX为SEG口的下标，如果是高32位那么需要用下标减去32;     .   .
  * @param  ComNum:      用户使用的COM口总数量
  * @param  SegNum:      用户使用的SEG口总数量
  * @param  SegGroup:    用户选用SEG口下标组合，例如{16，17，18，19，24，25}表示使用Seg16-19、24-25共六个口
  * @param  UserPattern: 用户预期的输入内容，与不同LCD的编码有关，可以看作某个特性SEG的com口输入逻辑序列；
  * @param  ValueL:      该com口下获取的ValueL值
  * @param  ValueH:      该com口下获取的ValueH值
  * @retval 这个函数把每个com口对应的seg控制信息提取出来，以便写入Com寄存器的高、低部分；
*******************************************************************************************************************/
uint32_t LCDC_GetSeg(uint8_t ComNum,uint8_t SegNum,uint8_t *SegGroup,uint8_t *UserPattern,uint32_t *ValueL,uint32_t *ValueH)
{
	uint8_t i=0;

    for(i=0;i<SegNum;i++)
    {
			if(SegGroup[i] <= 32)
			{
				*ValueL |= LCDC_GetSegX(ComNum,UserPattern[i]) << SegGroup[i];
			}
			else
			{
				*ValueH |= ((LCDC_GetSegX(ComNum,UserPattern[i]) << (SegGroup[i]-32))&0x07); 
			}			
		}

    return 0;
}

/*****************************************************************************************************************
  * @brief 这是设置某个SEG寄存器SegX的接口，每个Com口的控制序列都不同，
  *        所以需要根据Com下标来区分每个确定下标的com口，
  * @param  ComNum:      用户使用的COM口总数量
  * @param  UserPattern: 用户预期的输入内容，与不同LCD的编码有关，可以看作某个特性SEG的com口输入逻辑序列；
  * @retval 这个函数把每个com口对应的特定某个Seg口segx控制信息提取出来，以便移位组合后写入Com寄存器的高、低部分；
*****************************************************************************************************************/ 
uint32_t LCDC_GetSegX(uint8_t ComNum,uint8_t UserPattern)
{
	return (UserPattern & (1<<(7-ComNum))) >> (7-ComNum);
}

//根据com口使用的数量配置Com_seg_mux
/**********************************************************************************/
/***************************************GPIO Port Num******************************/
/****                            Seg_n       27+n    MCU version; 27>= n >=1   ****/
/****                            Seg0        27      NBOnly version:  9>=n>1   ****/
/***                                                                           ****/
/***                Com_seg_mux                                                ****/
/***   Value            0         1                                            ****/
/***   Bit[0]         Com7       Seg28       26                                ****/
/***   Bit[1]         Com6       Seg29       25                                ****/
/***   Bit[2]         Com5       Seg30       24                                ****/
/***   Bit[3]         Com4       Seg31       23                                ****/
/***   Bit[4]         Com3       Seg32       22                                ****/
/***   Bit[5]         Com2       Seg33       21                                ****/
/***   Bit[6]         Com1       Seg34       20                                ****/
/***                                                                           ****/
/***                             Com0        14                                ****/
/**********************************************************************************/
void LCDC_ComBiasSet(uint8_t comNum)
{
    switch(comNum)
    {
        case 1://use 1 com      
            LCDC_ComsegmuxSet(0x7f);
            LCDC_BiasSet(0x0);
        break;       
        case 2://use 2  Com     
            LCDC_ComsegmuxSet(0x3f);
            LCDC_BiasSet(0x1);
        break;        
        case 3://use 3Com     
            LCDC_ComsegmuxSet(0x1f);
            LCDC_BiasSet(0x2);
        break;        
        case 4://use 4  Com     
            LCDC_ComsegmuxSet(0xf);
            LCDC_BiasSet(0x2);
        break;        
        case 6://use 6  Com     
            LCDC_ComsegmuxSet(0x3);
            LCDC_BiasSet(0x3);
        break;       
        case 8://use 8  Com      
            LCDC_ComsegmuxSet(0x0);
            LCDC_BiasSet(0x3);
        break;        
        default:
            break;  
    }
}

/**
  * @brief  Set LCDC CTRL0 regs.
  * @param  tmux、duty、comNum、bypassEn
  * @retval none
  */
void LCDC_CTRL0Set(uint8_t tmuxSel,uint8_t duty,uint8_t comNum,int bypassEn)
{
    /*tmux_sel*/
    LCDC_TmuxSelSet(tmuxSel);  //use for fpga chip debug
    /*duty*/
    LCDC_DutySet(duty);
    /*BIAS*/
    LCDC_ComBiasSet(comNum);
    /*bypass_en*/
    LCDC_BypassEnSet(bypassEn);  // 0： 800nA， 1：  100nA 模拟
}

/**
  * @brief  Set LCDC CTRL1 regs.
  * @param  blink、dead、lwave、Forceclk
  * @retval none
  */
void LCDC_CTRL1Set(uint8_t blinkSet,uint8_t deadSet,int lwaveSet,int Forceclk)
{ 
    /*blink*/
    LCDC_BlinkSet(blinkSet); 
    /*dead periods  cnt*/
    LCDC_DeadSet(deadSet);
    /*LWAVE*/
    LCDC_LwaveSet(lwaveSet);// 0 A Wave， 1：B Wave(duty！=0)
    /*force_clkgate_en*/
    if(Forceclk)
    {
      LCDC_ForceclkEnable();
    }
    else
    {
      LCDC_ForceclkDisable();
    }
}

/**
  * @brief  Set LCDC FDR regs.
  * @param  rps、div
  * @retval none
  */
void LCDC_FDRSet(uint8_t rpsSet, int div)
{
    /*PS*/
    LCDC_FCRpsSet(rpsSet);     
    /*DIV*/
    LCDC_FCRdivSet(div); 
}

/**
  * @brief  Set LCDC Uvol regs.
  * @param  lcpGain、ClkCtrl、supplySel、vlcdCHS
  * @retval none
  */
void LCDC_UVolSet(uint8_t lcpGain, int ClkCtrl, int supplySel,int vlcdCHS)
{
    /*lcp_gain_ctrl 需在lcd_vlcd_chs之前set*/
    LCDC_UvolLCPGainSet(lcpGain); // 0: 3times
    /*clk_ctrl      时钟必选 ，其他设置可选*/
    LCDC_UvolClkCtrl(ClkCtrl);   //0: 32kHz
    /*supply_sel*/
    LCDC_UvolSupplySelSet(supplySel); 
    /*vlcd_chs*/
    LCDC_UvolVlcdCHSSet(vlcdCHS);  //00  External VBAT;
}

/**
  * @brief  Start LCD WORK mode.
  * @param  none
  * @retval none
  */
void LCDC_WORKStart(void)
{
		LCDC_FCRLcdPuSet(1);
    /***工作时 Duty、BIAS、clkgate、LWAVE、FCR、UVol不可更改，需停止 LCDC后才可进行配置**/
    /*配置LCDCen=1*/
    LCDC_ENStart();
}

static uint32_t lcd_lastTickCnt = 0; // 每次update时，更新tick
/**
  * @brief  Update LCD Display.
  * @param  LcdRate LCD屏幕刷新率，单位Hz
  * @retval none
  */
void LCDC_WORKUpdateWait(uint32_t LcdRate)
{
    /* unit：ms  最大更新时间为两帧时间 */
    uint32_t LCD_UPDATE_WAIT_TIMEOUT = 2000UL / LcdRate + 1;

    /*在更新等待的超时时间内，且update done状态没有置1，则一直等待*/
    while((HWREGB(LCD_UPDATE_DONE) != 0x1) && (Check_Ms_Timeout(lcd_lastTickCnt, LCD_UPDATE_WAIT_TIMEOUT) == 0));
}

/**
  * @brief  Update LCD Display.
  * @param  none
  * @retval none
  */
void LCDC_WORKUpdate(void)
{   
    /*等待updatedone读清，以确保前后两次数据稳定切换，避免两次数据间隔太近发生显示重影和乱码*/
    while(HWREGB(LCD_UPDATE_DONE) != 0x0);

    /*如果显示数据需要更改，则配置LCD0—7数据和设置更新_REQ*/
    LCDC_UPDATEReqSet(1);
	
    // bug原因：睡眠时lcd时钟关闭，导致睡眠期间update完成，却无法将update_done置1
    // 软件规避：在LCDC_WORKUpdate接口内增加获取tick操作，
    // 在LCDC_WORKUpdateWait接口内根据tick做了超时判断，如果16ms内还没有将update_done置1则退出等待
    lcd_lastTickCnt = Get_Tick();
}

/**
  * @brief  Stop LCD WORK mode.
  * @param  none
  * @retval none
  */
void LCDC_WORKStop(void)
{
    /*LCDC_en=0*/
    LCDC_ENStop();
    /*等待帧结束(frame_fin =0x1)*/
    /*lcd_pu=0?*/
	  LCDC_FCRLcdPuSet(0);
}

/**********************************************************************************/
/****                       GPIO[27+n]_SEG_n                                   ****/
/****         MCU version; 27>= n >=1 NBOnly version:  9>=n>1                  ****/
/**********************************************************************************/
void LCDC_SegGPIOSet(GPIO_PadTypeDef PadNum)
{
	uint8_t  PinReg  = (PadNum >= 32) ? 0x01 : 0x00;
    uint32_t PinBit =  (PadNum >= 32) ? 
	                      ((uint32_t)0x01 << (PadNum & 31)):
	                      ((uint32_t)0x01 << PadNum);	
    if(PinReg)
    {
		GPIO->INPUT_EN[1] &= ~PinBit; // High 32 IOs disable input
	    GPIO->OUTPUT_EN[1] |= PinBit; // High 32 IOs disable output
        GPIO->PULL_UP[1] |= PinBit;   // High 32 IOs disable pullup
        GPIO->PULL_DOWN[1] |= PinBit; // High 32 IOs disable pulldown
        GPIO->ANAE[1]	&= ~PinBit;	  // High 32 IOs enable  ANAE
    }
    else
    { 	
		GPIO->INPUT_EN[0] &= ~PinBit; //  Low 32 IOs disable input
	    GPIO->OUTPUT_EN[0] |= PinBit; //  Low 32 IOs disable output
		GPIO->PULL_UP[0] |= PinBit;   //  Low 32 IOs disable pullup
		GPIO->PULL_DOWN[0] |= PinBit; //  Low 32 IOs disable pulldown
        GPIO->ANAE[0] &= ~PinBit;	  //  Low 32 IOs enable  ANAE		
    }
}
  
/**
  * @brief  Enable LCD pad mode.
  * @param  none
  * @retval none
  */  
void LCDC_PAD_Enable_All(void)
{
	LCDC->LCD_SEG_PAD_CTRL = 0xFFFFFFFF;
    LCDC->LCD_COM_PAD_CTRL = 0xFF;
}

/**
  * @brief  Disable LCD pad mode.
  * @param  none
  * @retval none
  */  
void LCDC_PAD_Disable(void)
{
	LCDC->LCD_SEG_PAD_CTRL = 0;
    LCDC->LCD_COM_PAD_CTRL = 0;
}

/**
  * @brief  Force on LCD display.
  * @param  none
  * @retval none
  */ 
void LCDC_PWR_Force_On(void)
{
    AONPRCM->AONPWR_CTRL &= ~(0x30); //clear lcd pwr
    AONPRCM->AONPWR_CTRL |= (0x01<<4); //force on lcd pwr
}

/**
  * @brief  Start LCD display，ComX regs must be set done before.
  * @param  none
  * @retval none
  */
void LCDC_ENStart(void)
{
    LCDC->LCDC_EN |= LCD_LCDC_EN_Msk;
}

/**
  * @brief  stop LCD display.
  * @param  none
  * @retval none
  */
void LCDC_ENStop(void)
{
    LCDC->LCDC_EN &= ~LCD_LCDC_EN_Msk;
}

/**
  * @brief  Tmux for debug use.
  * @param  none
  * @retval none
  */
void LCDC_TmuxSelSet(uint8_t ucValue)
{
    LCDC->CTRL0 = (LCDC->CTRL0 & ~(LCDC_CTRL0_TMUX_Msk)) | (ucValue << LCDC_CTRL0_TMUX_Pos);
}

/**
  * @brief  Duty for LCDC controller.
  * @param  duty
  * @retval none
  */
void LCDC_DutySet(uint8_t ucValue)
{
    LCDC->CTRL0 = (LCDC->CTRL0 & ~(LCDC_CTRL0_DUTY_Msk)) | (ucValue << LCDC_CTRL0_DUTY_Pos);
}

/**
  * @brief  Bias for LCDC controller.
  * @param  Bias
  * @retval none
  */
void LCDC_BiasSet(uint8_t ucValue)
{
    LCDC->CTRL0 = (LCDC->CTRL0 & ~(LCDC_CTRL0_BIAS_Msk)) | (ucValue << LCDC_CTRL0_BIAS_Pos);
}

/**
  * @brief  BypassEn for LCDC controller.
  * @param  BypassEn
  * @retval none
  */
void LCDC_BypassEnSet(uint8_t ucValue)
{
    LCDC->CTRL0 = (LCDC->CTRL0 & ~(LCDC_CTRL0_BYPASS_EN_Msk)) | (ucValue << LCDC_CTRL0_BYPASS_EN_Pos);
}

/**
  * @brief  wavemode for LCDC controller.
  * @param  wavemode
  * @retval none
  */
void LCDC_LwaveSet(uint8_t ucValue)
{
    LCDC->CTRL1 = (LCDC->CTRL1 & ~(LCDC_CTRL1_LWAVE_Msk)) | (ucValue << LCDC_CTRL1_LWAVE_Pos);
}


/**
  * @brief  get wavemode for LCDC controller.
  * @param  none
  * @retval wavemode
  */
uint8_t LCDC_LwaveGet(void)
{
    return ((LCDC->CTRL1 & LCDC_CTRL1_LWAVE_Msk)?1:0);
}


/**
  * @brief  Forceclk Enable for LCDC controller.
  * @param  none
  * @retval none
  */
void LCDC_ForceclkEnable(void)
{
    LCDC->CTRL1 |= LCDC_CTRL1_FORCECLK_EN_Msk;
}

/**
  * @brief  Forceclk Disable for LCDC controller.
  * @param  none
  * @retval none
  */
void LCDC_ForceclkDisable(void)
{
    LCDC->CTRL1 &= ~LCDC_CTRL1_FORCECLK_EN_Msk;
}

/**
  * @brief  BlinkSet for LCDC controller.
  * @param  BlinkSet
  * @retval none
  */
void LCDC_BlinkSet(uint8_t ucValue)
{
    LCDC->CTRL1 = (LCDC->CTRL1 & ~(LCDC_CTRL1_BLINK_Msk)) | (ucValue << LCDC_CTRL1_BLINK_Pos);
}

/**
  * @brief  DeadSet for LCDC controller.
  * @param  DeadSet
  * @retval none
  */
void LCDC_DeadSet(uint8_t ucValue)
{
    LCDC->CTRL1 = (LCDC->CTRL1 & ~(LCDC_CTRL1_DEAD_Msk)) | (ucValue << LCDC_CTRL1_DEAD_Pos);
}

/**
  * @brief  VlcdCHS for LCDC controller.
  * @param  VlcdCHS
  * @retval none
  */
void LCDC_UvolVlcdCHSSet(uint8_t ucValue)
{
    LCDC->UVOL = (LCDC->UVOL & ~(LCDC_UVOL_VLCD_CHS_Msk)) | (ucValue << LCDC_UVOL_VLCD_CHS_Pos);
}

/**
  * @brief  LCPGain for LCDC controller.
  * @param  LCPGain
  * @retval none
  */
void LCDC_UvolLCPGainSet(uint8_t ucValue)
{
    LCDC->UVOL = (LCDC->UVOL & ~(LCDC_UVOL_LCP_GAIN_CTRL_Msk)) | (ucValue << LCDC_UVOL_LCP_GAIN_CTRL_Pos);
}

/**
  * @brief  SupplySel for LCDC controller.
  * @param  SupplySel
  * @retval none
  */
void LCDC_UvolSupplySelSet(uint8_t ucValue)
{
    LCDC->UVOL = (LCDC->UVOL & ~(LCDC_UVOL_SUPPLY_SEL_Msk)) | (ucValue << LCDC_UVOL_SUPPLY_SEL_Pos);
}

/**
  * @brief  ClkCtrl for LCDC controller.
  * @param  ClkCtrl
  * @retval none
  */
void LCDC_UvolClkCtrl(uint8_t ucValue)
{
    LCDC->UVOL = (LCDC->UVOL & ~(LCDC_UVOL_CLK_CTRL_Msk)) | (ucValue << LCDC_UVOL_CLK_CTRL_Pos);
}

/**
  * @brief  ps for LCDC controller.
  * @param  ps
  * @retval none
  */
void LCDC_FCRpsSet(uint8_t ucValue)
{
    LCDC->FCR = (LCDC->FCR & ~(LCDC_FCR_PS_Msk)) | (ucValue << LCDC_FCR_PS_Pos);
}

/**
  * @brief  LcdPu for LCDC controller.
  * @param  LcdPu
  * @retval none
  */
void LCDC_FCRLcdPuSet(uint8_t ucValue)
{
    LCDC->FCR = (LCDC->FCR & ~(LCDC_FCR_LCDPU_Msk)) | (ucValue << LCDC_FCR_LCDPU_Pos);
}

/**
  * @brief  div for LCDC controller.
  * @param  div
  * @retval none
  */
void LCDC_FCRdivSet(uint8_t ucValue)
{
    LCDC->FCR = (LCDC->FCR & ~(LCDC_FCR_DIV_Msk)) | (ucValue << LCDC_FCR_DIV_Pos);
}

/**
  * @brief  UPDATEReq for LCDC controller.
  * @param  UPDATEReq
  * @retval none
  */
void LCDC_UPDATEReqSet(uint8_t ucValue)
{
    LCDC->UPDATE_REQ = ucValue;
}

/**
  * @brief  port mux for LCDC controller.
  * @param  port mux
  * @retval none
  */
void LCDC_ComsegmuxSet(uint8_t ucValue)
{
    LCDC->COM_SEG_MUX = ucValue;
}


/**
  * @brief  port mux for LCDC controller.
  * @param  none
  * @retval port mux
  */
uint8_t LCDC_ComsegmuxGet(void)
{
    return LCDC->COM_SEG_MUX;
}

