/**
  ******************************************************************************
  * @file    stm32_lcd_driver.h
  * @author  (C)2015, Qindao ieslab Co., Ltd
  * @version V1.0
  * @date    2016-5-4
  * @brief   the function of the entity of system processor
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __U_LCD_DRIVER_H
#define __U_LCD_DRIVER_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "mcu_adapt.h"
#include "type.h"


#define JC_LCD_CHAR_NUM         (10)
#define JC_LCD_REG_SEG1         XY_IO_SEG19 
#define JC_LCD_REG_SEG2         XY_IO_SEG21
#define JC_LCD_REG_SEG3         XY_IO_SEG22
#define JC_LCD_REG_SEG4         XY_IO_SEG23
#define JC_LCD_REG_SEG5         XY_IO_SEG24
#define JC_LCD_REG_SEG6         XY_IO_COM5      //XY_IO_SEG30
#define JC_LCD_REG_SEG7         XY_IO_SEG17
#define JC_LCD_REG_SEG8         XY_IO_SEG18
#define JC_LCD_REG_SEG9         XY_IO_SEG16
#define JC_LCD_REG_SEG10        XY_IO_SEG15
#define JC_LCD_REG_SEG11        XY_IO_SEG8
#define JC_LCD_REG_SEG12        XY_IO_SEG4
#define JC_LCD_REG_SEG13        XY_IO_SEG0
#define JC_LCD_REG_SEG14        XY_IO_SEG27
#define JC_LCD_REG_SEG15        XY_IO_SEG26
#define JC_LCD_REG_SEG16        XY_IO_SEG25
#define JC_LCD_REG_SEG17        XY_IO_SEG20
#define JC_LCD_REG_SEG18        XY_IO_SEG9
#define JC_LCD_REG_SEG19        XY_IO_SEG14
#define JC_LCD_REG_SEG20        XY_IO_SEG3
#define JC_LCD_REG_SEG21        XY_IO_SEG1
#define JC_LCD_REG_SEG22        XY_IO_COM6      //XY_IO_SEG29
#define JC_LCD_REG_SEG23        XY_IO_COM7      //XY_IO_SEG28

#define JC_LCD_REG_COM1         XY_IO_COM0
#define JC_LCD_REG_COM2         XY_IO_COM1
#define JC_LCD_REG_COM3         XY_IO_COM2
#define JC_LCD_REG_COM4         XY_IO_COM3

/* MACRO Define--------------------------------------------------------------------------*/	
#define LCD_TIMEOUT_VALUE               60000 //超时时长，与所选的时钟、分频和死区时间有关
	 
//LCD数字、字母、横杠、空白显示宏定义
#define DIS_0                           0  //数字0
#define DIS_1                           1  //数字1
#define DIS_2                           2  //数字2
#define DIS_3                           3  //数字3
#define DIS_4                           4  //数字4
#define DIS_5                           5  //数字5
#define DIS_6                           6  //数字6
#define DIS_7                           7  //数字7
#define DIS_8                           8  //数字8
#define DIS_9                           9  //数字9
#define DIS_A                           11 //英文A
#define DIS_B                           24 //英文b
#define DIS_C                           12 //英文C
#define DIS_D                           26 //英文d
#define DIS_E                           13 //英文E
#define DIS_F                           14 //英文F
#define DIS_P                           21 //英文P
#define DIS_t                           40 //英文t
#define DIS_HG                          37 //横杠-
#define DIS_BLANK                       10 //空白	
#define DIS_ALL_NUM						20 //数码管显示总数

//#define TIMER_100MS_MAX_SUM             8  //定时器时间

//固化字符显示               
#define FUN_ALARM                       0x00000001//bit0 //T1报警
#define FUN_MALFUNCTION                 0x00000002       //T2故障		
#define FUN_GPRSING                     0x00000004       //T3正在无线通信
#define FUN_FLOW_LOW                    0x00000008       //T4超低流量(滴漏)
#define FUN_BATTERY_LOW                 0x00000010//bit4 //T5电池电压低
#define FUN_TEMPERATURE                 0x00000020       //T6温度
#define FUN_VALVE_CLOSE                 0x00000040       //T7阀门关
#define FUN_TEST                        0x00000080       //T8检定状态
#define FUN_TEST_DISPLAY                0x00000100//bit8 //T9用于检定时显示
#define FUN_M3                          0x00000200       //T10 立方米m3
#define FUN_FH                          0x00000400       //T11 符号
#define FUN_HOUR                        0x00000800       //T12 小时 h
#define FUN_YUAN                        0x00001000//bit12//T13 元
#define FUN_SY                          0x00002000       //T14 剩余
#define FUN_DJ                          0x00004000       //T15 单价
#define FUN_GR                          0x00008000       //T16 购入
#define FUN_JF                        	0x00010000//bit16//T17 缴费
#define FUN_RE1                         0x00020000       //T18 
#define FUN_RE2                         0X00040000       //T19 
#define FUN_P1G                         0x00080000       //T20 
#define FUN_P2G                         0x00100000       //T21 
#define FUN_P3G                         0x00200000       //T22 
#define FUN_P4G                         0x00400000       //T23 
#define FUN_P5G                         0x00800000       //T24 
#define FUN_REVERSE                     0x01000000       //T25 
#define FUN_P1                          0x02000000       //P1 P1小数点 
#define FUN_P2                          0x04000000       //P2 P2小数点 
#define FUN_P3                          0x08000000       //P3 P2小数点 
#define FUN_P4                          0x10000000       //P4 P2小数点 
#define FUN_P5                          0x20000000       //P5 P2小数点 
        

//#define FUN_VALVE_OPEN                  0x00020000       //T18 开阀
//#define FUN_YUAN                        0x00040000       //T19 元
////#define FUN_BK                         	0x00040000       //T19标况BK
////#define FUN_GK                    		0x00080000       //T20工况GK
////#define FUN_DOT                         0x00100000//bit20//T21小数点DOT

////#define FUN_P2                          0x00200000       //T22P2左二
////#define FUN_P3                          0x00400000       //T23P3右二
////#define FUN_P4                          0x00800000       //T24P4右一
////#define FUN_P                           0x00080000       //小数点起始位

//#pragma	pack (1)
typedef	struct { 
		u32 bit0 : 1;
		u32 bit1 : 1;
		u32 bit2 : 1;
		u32 bit3 : 1;
		u32 bit4 : 1;
		u32 bit5 : 1;
		u32 bit6 : 1;
		u32 bit7 : 1;
		u32 bit8 : 1;
		u32 bit9 : 1;
		u32 bit10 : 1;
		u32 bit11 : 1;
		u32 bit12 : 1;
		u32 bit13 : 1;
		u32 bit14 : 1;
		u32 bit15 : 1;
		u32 bit16 : 1;
		u32 bit17 : 1;
		u32 bit18 : 1;
		u32 bit19 : 1;
		u32 bit20 : 1;
		u32 bit21 : 1;
		u32 bit22 : 1;
		u32 bit23 : 1;
		u32 bit24 : 1;
		u32 bit25 : 1;
		u32 bit26 : 1;
		u32 bit27 : 1;
		u32 bit28 : 1;
		u32 bit29 : 1;
		u32 bit30 : 1;
		u32 bit31 : 1; 
}LCD_RAM;


//LCD数据显示寄存器LCD_RAM地址
#define lcd_ram0	                      ((LCD_RAM *)&(M0P_LCD->RAM0))//COM0
#define lcd_ram1	                      ((LCD_RAM *)&(M0P_LCD->RAM8))
#define lcd_ram2	                      ((LCD_RAM *)&(M0P_LCD->RAM1))//COM1
#define lcd_ram3	                      ((LCD_RAM *)&(M0P_LCD->RAM9))
#define lcd_ram4	                      ((LCD_RAM *)&(M0P_LCD->RAM2))//COM2
#define lcd_ram5	                      ((LCD_RAM *)&(M0P_LCD->RAMA))
#define lcd_ram6	                      ((LCD_RAM *)&(M0P_LCD->RAM3))//COM3
#define lcd_ram7	                      ((LCD_RAM *)&(M0P_LCD->RAMB))

//按位定义固化字符：LCD直接符号
//S1-SEG15
//S2-SEG14
//S3-SEG13
//S4-SEG12
//S5-SEG11
//S6-SEG51
//S7-SEG50
//S8-SEG49
//S9-SEG10
//S10-SEG9
//S11-SEG8
//S12-SEG7
//S13-SEG6
//S14-SEG5
//S15-SEG47
//S16-SEG46
//S17-SEG45
//S18-SEG44
//S19-SEG4
//S20-SEG3
//S21-SEG2
//S22-SEG1
//S23-SEG0
/**COM0*/
#define LCD_FUN_T25                        lcd_ram0->bit15  //S1
#define LCD_FUN_T1                         lcd_ram0->bit13  //S3
#define LCD_FUN_T2                         lcd_ram0->bit11  //S5
#define LCD_FUN_T3                         lcd_ram1->bit18  //S7 //高位
#define LCD_FUN_T4                         lcd_ram0->bit10  //S9
#define LCD_FUN_P1                         lcd_ram0->bit8   //S11
#define LCD_FUN_T20                        lcd_ram0->bit6   //S13
#define LCD_FUN_T21                        lcd_ram1->bit15  //S15 //高位
#define LCD_FUN_T22					       lcd_ram1->bit13  //S17 //高位
#define LCD_FUN_T23						   lcd_ram0->bit4   //S19
#define LCD_FUN_T11                        lcd_ram0->bit2   //S21
#define LCD_FUN_T10                        lcd_ram0->bit1   //S22
#define LCD_FUN_P5                         lcd_ram0->bit0   //S23
/**COM1*/
#define LCD_FUN_T12                        lcd_ram2->bit2   //S21
#define LCD_FUN_T6                         lcd_ram2->bit1   //S22
#define LCD_FUN_P4                         lcd_ram2->bit0   //S23
/**COM2*/
#define LCD_FUN_T8                         lcd_ram4->bit2   //S21 
#define LCD_FUN_T7                         lcd_ram4->bit1   //S22
#define LCD_FUN_P3                         lcd_ram4->bit0   //S23
/**COM3*/
#define LCD_FUN_T24                        lcd_ram6->bit2   //S21 
#define LCD_FUN_T5                         lcd_ram6->bit1   //S22
#define LCD_FUN_P2                         lcd_ram6->bit0   //S23

//按位定义固化字符：系统重定义//gaishanxiugai
#define _FUN_T1    LCD_FUN_T1
#define _FUN_T2    LCD_FUN_T2
#define _FUN_T3    LCD_FUN_T3
#define _FUN_T4    LCD_FUN_T4
#define _FUN_T5    LCD_FUN_T5
#define _FUN_T6    LCD_FUN_T6
#define _FUN_T7    LCD_FUN_T7
#define _FUN_T8    LCD_FUN_T8
//#define _FUN_T9
#define _FUN_T10   LCD_FUN_T10
#define _FUN_T11   LCD_FUN_T11
#define _FUN_T12   LCD_FUN_T12
//#define _FUN_T13
//#define _FUN_T14
//#define _FUN_T15
//#define _FUN_T16
//#define _FUN_T17
//小数点
#define _FUN_T18   LCD_FUN_P1
#define _FUN_T19   LCD_FUN_P2
#define _FUN_T20   LCD_FUN_P3
#define _FUN_T21   LCD_FUN_P4
#define _FUN_T22   LCD_FUN_P5
//小数横杠
#define _FUN_T23   LCD_FUN_T20 //P1杠
#define _FUN_T24   LCD_FUN_T21 //P2杠
#define _FUN_T25   LCD_FUN_T22 //P3杠
#define _FUN_T26   LCD_FUN_T23 //P4杠
#define _FUN_T27   LCD_FUN_T24 //P5杠
#define _FUN_T28   LCD_FUN_T25 //逆流符号            

/*关于LCD特性的一些宏,换到华大MCU所需要的宏*/
//LCD占空比
#define LCD_DUTY                        LCD_DUTY_1_4 
#define LCD_DUTY_STATIC                 0x00000000                   //Static占空比 
#define LCD_DUTY_1_2                    (LcdDuty2)                   //1/2占空比    
#define LCD_DUTY_1_3                    (LcdDuty3)                   //1/3占空比  
#define LCD_DUTY_1_4                    (LcdDuty4)                   //1/4占空比    
#define LCD_DUTY_1_8                    (LcdDuty8)                   //1/8占空比 

//LCD偏压比
#define LCD_BIAS                        LcdBias2 
#define LCD_BIAS_1_4                    0x00000000    //1/4偏压比 
#define LCD_BIAS_1_2                    LcdBias2      //1/2偏压比 
#define LCD_BIAS_1_3                    LcdBias3      //1/3偏压比 

//LCD供电电压源
#define LCD_VOLTAGESOURCE               LCD_VOLTAGESOURCE_EXTERNAL
#define LCD_VOLTAGESOURCE_INTERNAL      0x00000000    //LCD使用内部电压源 
#define LCD_VOLTAGESOURCE_EXTERNAL      LcdExtCap     //LCD使用外部电压源  

//LCD对比度
#define LCD_CONTRASTLEVEL               LCD_CONTRASTLEVEL_0
#define LCD_CONTRASTLEVEL_0             (0x00)                //Maximum Voltage = 2.60V    
#define LCD_CONTRASTLEVEL_1             (0x02)                //Maximum Voltage = 2.73V    
#define LCD_CONTRASTLEVEL_2             (0x04)                //Maximum Voltage = 2.86V    
#define LCD_CONTRASTLEVEL_3             (0x06)                //Maximum Voltage = 2.99V    
#define LCD_CONTRASTLEVEL_4             (0x08)                //Maximum Voltage = 3.12V    
#define LCD_CONTRASTLEVEL_5             (0x0a)                //Maximum Voltage = 3.26V    
#define LCD_CONTRASTLEVEL_6             (0x0c)                //Maximum Voltage = 3.40V    
#define LCD_CONTRASTLEVEL_7             (0x0e)                //Maximum Voltage = 3.55V    

//LCD死区时间
#define LCD_DEADTIME                    LCD_DEADTIME_2                     
#define LCD_DEADTIME_0                  0x00000000           //没有死区时间
#define LCD_DEADTIME_1                  (1)                  //不同的两帧之间有一个相位死区时间   
#define LCD_DEADTIME_2                  (2)                  //不同的两帧之间有两个相位死区时间    
#define LCD_DEADTIME_3                  (3)                  //不同的两帧之间有三个相位死区时间  
#define LCD_DEADTIME_4                  (4)                  //不同的两帧之间有四个相位死区时间  
#define LCD_DEADTIME_5                  (5)                  //不同的两帧之间有五个相位死区时间  
#define LCD_DEADTIME_6                  (6)                  //不同的两帧之间有六个相位死区时间  
#define LCD_DEADTIME_7                  (7)                  //不同的两帧之间有七个相位死区时间   

//LCD脉冲持续时间
#define LCD_PULSEONDURATION             LCD_PULSEONDURATION_7
#define LCD_PULSEONDURATION_0           0x00000000          //Pulse ON duration = 0 pulse   
#define LCD_PULSEONDURATION_1           (1)                 //Pulse ON duration = 1/CK_PS  
#define LCD_PULSEONDURATION_2           (2)                 //Pulse ON duration = 2/CK_PS  
#define LCD_PULSEONDURATION_3           (3)                 //Pulse ON duration = 3/CK_PS  
#define LCD_PULSEONDURATION_4           (4)                 //Pulse ON duration = 4/CK_PS  
#define LCD_PULSEONDURATION_5           (5)                 //Pulse ON duration = 5/CK_PS  
#define LCD_PULSEONDURATION_6           (6)                 //Pulse ON duration = 6/CK_PS  
#define LCD_PULSEONDURATION_7           (7)                 //Pulse ON duration = 7/CK_PS  

//LCD复用SEG
#define LCD_MUXSEGMENT                  LCD_MUXSEGMENT_DISABLE 
#define LCD_MUXSEGMENT_DISABLE          0x00000000       //SEG pin multiplexing disabled 
#define LCD_MUXSEGMENT_ENABLE           (LCD_CR_MUX_SEG) //SEG[31:28] are multiplexed with SEG[43:40]  

//LCD闪烁模式
#define LCD_BLINKMODE                   LCD_BLINKMODE_OFF 
#define LCD_BLINKMODE_OFF               0x00000000        //Blink disabled            
#define LCD_BLINKMODE_SEG0_COM0         (LCD_FCR_BLINK_0) //Blink enabled on SEG[0], COM[0] (1 pixel)   
#define LCD_BLINKMODE_SEG0_ALLCOM       (LCD_FCR_BLINK_1) //Blink enabled on SEG[0], all COM   
#define LCD_BLINKMODE_ALLSEG_ALLCOM     (LCD_FCR_BLINK)   //Blink enabled on all SEG and all COM (all pixels)  

//LCD闪烁频率
#define LCD_BLINKFREQUENCY_DIV          LCD_BLINKFREQUENCY_DIV8        
#define LCD_BLINKFREQUENCY_DIV8         0x00000000                            //The Blink frequency = fLCD/8    
#define LCD_BLINKFREQUENCY_DIV16        (LCD_FCR_BLINKF_0)                    //The Blink frequency = fLCD/16   
#define LCD_BLINKFREQUENCY_DIV32        (LCD_FCR_BLINKF_1)                    //The Blink frequency = fLCD/32   
#define LCD_BLINKFREQUENCY_DIV64        (LCD_FCR_BLINKF_1 | LCD_FCR_BLINKF_0) //The Blink frequency = fLCD/64   
#define LCD_BLINKFREQUENCY_DIV128       (LCD_FCR_BLINKF_2)                    //The Blink frequency = fLCD/128  
#define LCD_BLINKFREQUENCY_DIV256       (LCD_FCR_BLINKF_2 |LCD_FCR_BLINKF_0)  //The Blink frequency = fLCD/256  
#define LCD_BLINKFREQUENCY_DIV512       (LCD_FCR_BLINKF_2 |LCD_FCR_BLINKF_1)  //The Blink frequency = fLCD/512  
#define LCD_BLINKFREQUENCY_DIV1024      (LCD_FCR_BLINKF)                      //The Blink frequency = fLCD/1024 

//LCD高驱动
#define LCD_HIGHDRIVE                   LCD_HIGHDRIVE_DISABLE            
#define LCD_HIGHDRIVE_DISABLE           0x00000000   //High drive disabled 
#define LCD_HIGHDRIVE_ENABLE            (LCD_FCR_HD) //High drive enabled   

//LCD标志定义
#define LCD_FLAG_ENS                    LCD_SR_ENS   //LCD使能状态
#define LCD_FLAG_UDR                    LCD_SR_UDR   //刷新显示请求标志 
#define LCD_FLAG_UDD                    LCD_SR_UDD   //刷新完成标志 
#define LCD_FLAG_RDY                    LCD_SR_RDY   //准备完成标志
#define LCD_FLAG_FCRSF                  LCD_SR_FCRSR //液晶帧控制寄存器同步标志 
	 
//LCD主任务机状态号
#define LCD_DISPLAY_S0                  0 
#define LCD_DISPLAY_S1                  1 
#define LCD_DISPLAY_S2                  2 
#define LCD_DISPLAY_S3                  3 

/* variables Define---------------------------------------------------------------*/

typedef struct {
		u32 T1 : 1;
		u32 T2 : 1;
		u32 T3 : 1;
		u32 T4 : 1;
		u32 T5 : 1;
		u32 T6 : 1;
		u32 T7 : 1;
		u32 T8 : 1;
		u32 T9 : 1;
		u32 T10 : 1;
		u32 T11 : 1;
		u32 T12 : 1;
		u32 T13 : 1;
		u32 T14 : 1;
		u32 T15 : 1;
		u32 T16 : 1;
		u32 T17 : 1;
		u32 T18 : 1;
		u32 T19 : 1;
		u32 T20 : 1;
		u32 T21 : 1;
		u32 T22 : 1;
		u32 T23 : 1;
		u32 T24 : 1;
		u32 T25 : 1;
		u32 T26 : 1;
		u32 T27 : 1;
		u32 T28 : 1;
		u32 T29 : 1;
		u32 T30 : 1;
		u32 T31 : 1;
		u32 T32 : 1;						
	}SEG;
	
typedef union {
	SEG seg;
	u32 word;
} FUN_DATA;

/* Function Declare------------------------------------------------------------*/
/* Functions -------------------------------------------------------*/
/*************************************************
Function: void LcdInit(void)
Description: Lcd初始化
Input：无
Return:无
Others:处于MainSpace，第一类接口：上电初始化
*************************************************/
 void LcdInit(void); 
/*************************************************
Function: void LcdMachineDriver(void)
Description:LCD驱动任务机
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
 void LcdMachineDriver(void);
 /*************************************************
Function: void LcdClearAll(void)
Description:LCD清屏
Input:无
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdClearAll(void);
 /*************************************************
Function: void LcdDisplayAll(void)
Description:LCD全显
Input:无
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayAll(void);
 /*************************************************
Function: u8 LcdGetDisplayData(u8 position)
Description:获取LCD当前显示的数字
Input:u8 position：获取数码管显示数字的位置
Return:当前显示的数字
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayData(u8 position, u8 data);
/*************************************************
Function: void LcdDisplayFun(u32 fun)
Description:lcd显示固化字符
Input:u32 fun：按位定义的固化字符
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/ 
 void LcdDisplayFun(u32 fun);
 /*************************************************
Function: u8 LcdGetDisplayData(u8 position)
Description:获取LCD当前显示的数字
Input:u8 position：获取数码管显示数字的位置
Return:当前显示的数字
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 u8 LcdGetDisplayData(u8 position);
 /*************************************************
Function: u32 LcdGetDisplayFun(void)
Description:获取lcd当前显示的固化字符
Input:无
Return:当前显示的字符
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 u32 LcdGetDisplayFun(void);
 /*************************************************
Function: void LcdDisplayValve(u8 flag)
Description:lcd显示阀门状态
Input:u8 flag：1关阀；0开阀 ；其它默认不显示
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayValve(u8 flag);
 /*************************************************
Function: void LcdDisplayVol(u8 flag)
Description:lcd显示换电池
Input:u8 flag：1欠压；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayVol(u8 flag);
 /*************************************************
Function: void LcdDisplayAlarm(u8 flag)
Description:lcd显示报警
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayAlarm(u8 flag);
 /*************************************************
Function: void LcdDisplayActiveGPRS(u8 flag)
Description:lcd显示正在GPRS通信
Input:u8 flag：1正在通信；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayActiveGPRS(u8 flag);
 /*************************************************
Function: void LcdDisplayPleasePay(u8 flag)
Description:lcd显示请缴费
Input:u8 flag：1请缴费；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayPleasePay(u8 flag);
 /*************************************************
Function: void LcdDisplayEnable(void)
Description:使能LCD显示功能
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
 void LcdDisplayEnable(void);
 /*************************************************
Function: void LcdDisplayDisable(void)
Description:关闭LCD显示功能
Input:无
Return:无
Others:处于MainSpace，第二类接口：工作接口
*************************************************/
 void LcdDisplayDisable(void);
/*************************************************
Function: static void LcdClearData(u8 num)
Description:清LCD显示寄存器
Input:u8 num  num取值范围0~9代表数字段10个位置。
Return:无
Others:内部接口
*************************************************/
 void LcdClearData(u8 num);
//新加工作接口
/*************************************************
Function: void LcdDisplayFailure(u8 flag)
Description:lcd 故障检测
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayFailure(u8 flag);
 /*************************************************
Function: void LcdDisplayLowFlowAlarm(u8 flag)
Description:lcd 低流量检测
Input:u8 flag：1报警；0正常
Return:无
Others:处于Mainspace，第二类接口：工作接口
*************************************************/
 void LcdDisplayLowFlowAlarm(u8 flag);


/*************************************************
Function: u8 LcdIfSleep(void)
Description:LCD是否允许休眠
Input:无
Return:TRUE允许；FALSE不允许
Others:处于Mainspace第三类接口：休眠前接口
*************************************************/
 u8 LcdIfSleep(void);
 /*************************************************
Function: void LcdPreSleep(void)
Description:LCD休眠前处理
Input:无
Return:无
Others:处于Mainspace第三类接口：休眠前接口
*************************************************/
 void LcdPreSleep(void);
/*************************************************
Function: void LcdWakeSleep(void)
Description:LCD唤醒后处理
Input:无
Return:无
Others:处于Mainspace，第四类接口：唤醒接口
*************************************************/
 void LcdWakeSleep(void);	

/*宏名解释:
	PINS1 : LCD显示面板的SEG1 ; 
	t25_e1_g1_f1 : SEG1引脚对应不同的COM可以点亮的段 ;
	SEG15 : 对应MUC的SEG15引脚
	宏体 : 对应显示寄存器中的哪一位点亮,数值是2^bitX的十六进制*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define PINS1_T25_E1_G1_F1_SEG15 	0X00008000//2^15
#define PINS2_D1_C1_B1_A1_SEG14	 	0X00004000//2^14	

#define PINS3_T1_E2_G2_F2_SEG13     0X00002000
#define PINS4_D2_C2_B2_A2_SEG12     0X00001000

#define PINS5_T2_E3_G3_F3_SEG11     0X00000800
#define PINS6_D3_C3_B3_A3_SEG51    (0X00080000<<4)//高位

#define PINS7_T3_E4_G4_F4_SEG50    (0X00040000<<4)//高位
#define PINS8_D4_C4_B4_A4_SEG49    (0X00020000<<4)//高位

#define PINS9_T4_5E_5G_5F_SEG10     0X00000400
#define PINS10_D5_C5_B5_A5_SEG9     0X00000200

#define PINS11_P1_E6_G6_F6_SEG8     0X00000100
#define PINS12_D6_C6_B6_A6_SEG7     0X00000080

#define PINS13_T20_E7_G7_F7_SEG6    0X00000040
#define PINS14_D7_C7_B7_A7_SEG5     0X00000020

#define PINS15_T21_E8_G8_F8_SEG47  (0X00008000<<4)//高位
#define PINS16_D8_C8_B8_A8_SEG46   (0X00004000<<4)//高位

#define PINS17_T22_E9_G9_F9_SEG45  (0X00002000<<4)//高位
#define PINS18_D9_C9_B9_A9_SEG44   (0X00001000<<4)//高位

#define PINS19_T23_E10_G10_F10_SEG4 0X00000010
#define PINS20_D10_C10_B10_A10_SEG3 0X00000008


//com是实际接到单片机的COM编号
/*宏名解释:A0_COM4 意义是 A1段是需要MCU COM4配合SEG14来点亮的*/
#define A0_COM4  PINS2_D1_C1_B1_A1_SEG14
#define B0_COM3  PINS2_D1_C1_B1_A1_SEG14
#define C0_COM2  PINS2_D1_C1_B1_A1_SEG14
#define D0_COM1  PINS2_D1_C1_B1_A1_SEG14
#define E0_COM2  PINS1_T25_E1_G1_F1_SEG15
#define F0_COM4  PINS1_T25_E1_G1_F1_SEG15
#define G0_COM3  PINS1_T25_E1_G1_F1_SEG15

#define A1_COM4  PINS4_D2_C2_B2_A2_SEG12
#define B1_COM3  PINS4_D2_C2_B2_A2_SEG12
#define C1_COM2  PINS4_D2_C2_B2_A2_SEG12
#define D1_COM1  PINS4_D2_C2_B2_A2_SEG12
#define E1_COM2  PINS3_T1_E2_G2_F2_SEG13
#define F1_COM4  PINS3_T1_E2_G2_F2_SEG13
#define G1_COM3  PINS3_T1_E2_G2_F2_SEG13

#define A2_COM4  PINS6_D3_C3_B3_A3_SEG51
#define B2_COM3  PINS6_D3_C3_B3_A3_SEG51
#define C2_COM2  PINS6_D3_C3_B3_A3_SEG51
#define D2_COM1  PINS6_D3_C3_B3_A3_SEG51
#define E2_COM2  PINS5_T2_E3_G3_F3_SEG11
#define F2_COM4  PINS5_T2_E3_G3_F3_SEG11
#define G2_COM3  PINS5_T2_E3_G3_F3_SEG11

#define A3_COM4  PINS8_D4_C4_B4_A4_SEG49
#define B3_COM3  PINS8_D4_C4_B4_A4_SEG49
#define C3_COM2  PINS8_D4_C4_B4_A4_SEG49
#define D3_COM1  PINS8_D4_C4_B4_A4_SEG49
#define E3_COM2  PINS7_T3_E4_G4_F4_SEG50
#define F3_COM4  PINS7_T3_E4_G4_F4_SEG50
#define G3_COM3  PINS7_T3_E4_G4_F4_SEG50

#define A4_COM4  PINS10_D5_C5_B5_A5_SEG9
#define B4_COM3  PINS10_D5_C5_B5_A5_SEG9
#define C4_COM2  PINS10_D5_C5_B5_A5_SEG9
#define D4_COM1  PINS10_D5_C5_B5_A5_SEG9
#define E4_COM2  PINS9_T4_5E_5G_5F_SEG10
#define F4_COM4  PINS9_T4_5E_5G_5F_SEG10
#define G4_COM3  PINS9_T4_5E_5G_5F_SEG10

#define A5_COM4  PINS12_D6_C6_B6_A6_SEG7
#define B5_COM3  PINS12_D6_C6_B6_A6_SEG7
#define C5_COM2  PINS12_D6_C6_B6_A6_SEG7
#define D5_COM1  PINS12_D6_C6_B6_A6_SEG7
#define E5_COM2  PINS11_P1_E6_G6_F6_SEG8
#define F5_COM4  PINS11_P1_E6_G6_F6_SEG8
#define G5_COM3  PINS11_P1_E6_G6_F6_SEG8

#define A6_COM4  PINS14_D7_C7_B7_A7_SEG5
#define B6_COM3  PINS14_D7_C7_B7_A7_SEG5
#define C6_COM2  PINS14_D7_C7_B7_A7_SEG5
#define D6_COM1  PINS14_D7_C7_B7_A7_SEG5
#define E6_COM2  PINS13_T20_E7_G7_F7_SEG6
#define F6_COM4  PINS13_T20_E7_G7_F7_SEG6
#define G6_COM3  PINS13_T20_E7_G7_F7_SEG6

#define A7_COM4  PINS16_D8_C8_B8_A8_SEG46
#define B7_COM3  PINS16_D8_C8_B8_A8_SEG46
#define C7_COM2  PINS16_D8_C8_B8_A8_SEG46
#define D7_COM1  PINS16_D8_C8_B8_A8_SEG46
#define E7_COM2  PINS15_T21_E8_G8_F8_SEG47
#define F7_COM4  PINS15_T21_E8_G8_F8_SEG47
#define G7_COM3  PINS15_T21_E8_G8_F8_SEG47

#define A8_COM4  PINS18_D9_C9_B9_A9_SEG44
#define B8_COM3  PINS18_D9_C9_B9_A9_SEG44
#define C8_COM2  PINS18_D9_C9_B9_A9_SEG44
#define D8_COM1  PINS18_D9_C9_B9_A9_SEG44
#define E8_COM2  PINS17_T22_E9_G9_F9_SEG45
#define F8_COM4  PINS17_T22_E9_G9_F9_SEG45
#define G8_COM3  PINS17_T22_E9_G9_F9_SEG45

#define A9_COM4  PINS20_D10_C10_B10_A10_SEG3
#define B9_COM3  PINS20_D10_C10_B10_A10_SEG3
#define C9_COM2  PINS20_D10_C10_B10_A10_SEG3
#define D9_COM1  PINS20_D10_C10_B10_A10_SEG3
#define E9_COM2  PINS19_T23_E10_G10_F10_SEG4
#define F9_COM4  PINS19_T23_E10_G10_F10_SEG4
#define G9_COM3  PINS19_T23_E10_G10_F10_SEG4

#define SEG_NULL  0


#ifdef __cplusplus
}
#endif


#endif /* __U_LCD_DRIVER_H */

