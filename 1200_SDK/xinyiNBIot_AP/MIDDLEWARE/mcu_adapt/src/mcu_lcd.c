#include "mcu_adapt.h"

//注意：LCD编码请参阅《芯翼XY2100产品LCD使用指导_V0.1》

//==============================================================================================================================================================
//==========以下是LCD类型定义，用户不得更改=======================================================================================================================
//==============================================================================================================================================================
static volatile uint32_t g_UpdateSta = 0; //指示LCD刷新状态是否为LCD使能后的首次，0：首次刷新，1：非首次刷新
static volatile uint32_t g_ComPad = 0, g_SegPad = 0; //记录所用芯翼com号、芯翼seg号
static volatile uint32_t g_LcdRate = 0; //记录LCD速率

/***********************************************************************************************
 * 行示意：McuLcdCom[0]：七段字符区；McuLcdCom[1]：说明区；McuLcdCom[2]：状态区
 * 列示意：列的每个元素表示1个com口，共0-7个com口，其中1bit表示1个seg位，前35bit有效，共0-34个seg位
***********************************************************************************************/
static volatile uint64_t McuLcdCom[3][8] = {0}; 

/*****************************************************************
 * 多点组合显示的com口编码数据类型，需要搭配seg位编码一起使用
 * 1：该com口显示，0：该com口不显示，2：该com口不关心
*****************************************************************/
typedef union
{
    struct
    {
        uint16_t com0 : 2; //com0 bit[0:1]
        uint16_t com1 : 2; //com1 bit[2:3]
        uint16_t com2 : 2; //com2 ……
        uint16_t com3 : 2; //com3
        uint16_t com4 : 2; //com4
        uint16_t com5 : 2; //com5
        uint16_t com6 : 2; //com6
        uint16_t com7 : 2; //com7
    }one_com;
    uint16_t all_com;
} McuLcdPattern_t;

/*****************************************************************
 * 单点显示的com口、seg位编码数据类型
 * com和seg的组合为点的位置，使用Setbit/Reset来实现显示/不显示该点
*****************************************************************/
typedef struct 
{
    uint8_t com;
    uint8_t seg;
} McuLcdBit_t;

/**************************************************************************************
 * 芯翼com0~com7与lcd屏幕7个字符段码的对应关系，注意'X'或'x'表示忽略，
 * 可选：'A'、'B'、'C'、'D'、'E'、'F'、'G'、'X' 或 'a'、'b'、'c'、'd'、'e'、'f'、'g'、'x'
**************************************************************************************/
typedef struct
{
    char char0; //芯翼com0对应的lcd屏幕7个字符段码中的一个段码。
    char char1; //芯翼com1对应的lcd屏幕7个字符段码中的一个段码。
    char char2; //芯翼com2对应的lcd屏幕7个字符段码中的一个段码。
    char char3; //芯翼com3对应的lcd屏幕7个字符段码中的一个段码。
    char char4; //芯翼com4对应的lcd屏幕7个字符段码中的一个段码。
    char char5; //芯翼com5对应的lcd屏幕7个字符段码中的一个段码。
    char char6; //芯翼com6对应的lcd屏幕7个字符段码中的一个段码。
    char char7; //芯翼com7对应的lcd屏幕7个字符段码中的一个段码。
} McuLcdXycomChar_t;

/*********************************************************************************************
 * 七段字符区用户xycom号编码，如数字，大写字母，小写字母，以及符号的编码
 * 注意：该数组值由McuLcdCodeCharTable函数根据McuLcdCharCom和McuLcdCharComConst数组得到
*********************************************************************************************/
#define LCD_CHAR_CODE_NUM (41) //七段字符区总共的芯翼字符编码个数
static McuLcdPattern_t McuLcdCharComUser[LCD_CHAR_CODE_NUM] = {0};

/********************************************************************
 * 七段字符区固定xycom号编码，如数字，大写字母，小写字母，以及符号的编码
 * 1：该xycom口显示，0：该xycom口不显示，2：该xycom口不关心
********************************************************************/
const McuLcdPattern_t McuLcdCharComConst[LCD_CHAR_CODE_NUM] = 
{
    /* a  b  c  d  e  f  g  x */
    {{ 1, 1, 1, 1, 1, 1, 0, 2 }}, //字符'0'
    {{ 0, 1, 1, 0, 0, 0, 0, 2 }}, //字符'1'
    {{ 1, 1, 0, 1, 1, 0, 1, 2 }}, //字符'2'
    {{ 1, 1, 1, 1, 0, 0, 1, 2 }}, //字符'3'
    {{ 0, 1, 1, 0, 0, 1, 1, 2 }}, //字符'4'
    {{ 1, 0, 1, 1, 0, 1, 1, 2 }}, //字符'5'
    {{ 1, 0, 1, 1, 1, 1, 1, 2 }}, //字符'6'
    {{ 1, 1, 1, 0, 0, 0, 0, 2 }}, //字符'7'
    {{ 1, 1, 1, 1, 1, 1, 1, 2 }}, //字符'8'
    {{ 1, 1, 1, 1, 0, 1, 1, 2 }}, //字符'9'
    {{ 0, 0, 0, 0, 0, 0, 0, 2 }}, //字符' ' 
    {{ 1, 1, 1, 0, 1, 1, 1, 2 }}, //字符'A' //11
    {{ 1, 0, 0, 1, 1, 1, 0, 2 }}, //字符'C'
    {{ 1, 0, 0, 1, 1, 1, 1, 2 }}, //字符'E' 
    {{ 1, 0, 0, 0, 1, 1, 1, 2 }}, //字符'F'
    {{ 1, 0, 1, 1, 1, 1, 0, 2 }}, //字符'G'
    {{ 0, 1, 1, 0, 1, 1, 1, 2 }}, //字符'H'
    {{ 0, 1, 1, 1, 0, 0, 0, 2 }}, //字符'J'
    {{ 0, 0, 0, 1, 1, 1, 0, 2 }}, //字符'L'
    {{ 1, 1, 1, 0, 1, 1, 0, 2 }}, //字符'N'
    {{ 1, 1, 1, 1, 1, 1, 0, 2 }}, //字符'O'
    {{ 1, 1, 0, 0, 1, 1, 1, 2 }}, //字符'P'
    {{ 1, 0, 1, 1, 0, 1, 1, 2 }}, //字符'S'
    {{ 0, 1, 1, 1, 1, 1, 0, 2 }}, //字符'U' //23
    {{ 0, 0, 1, 1, 1, 1, 1, 2 }}, //字符'b' //24
    {{ 0, 0, 0, 1, 1, 0, 1, 2 }}, //字符'c'
    {{ 0, 1, 1, 1, 1, 0, 1, 2 }}, //字符'd'
    {{ 1, 1, 1, 1, 0, 1, 1, 2 }}, //字符'g'
    {{ 0, 0, 1, 0, 1, 1, 1, 2 }}, //字符'h'
    {{ 0, 0, 1, 0, 0, 0, 0, 2 }}, //字符'i'
    {{ 0, 0, 0, 0, 1, 1, 0, 2 }}, //字符'l'
    {{ 0, 0, 1, 0, 1, 0, 1, 2 }}, //字符'n'
    {{ 0, 0, 1, 1, 1, 0, 1, 2 }}, //字符'o'
    {{ 1, 1, 1, 0, 0, 1, 1, 2 }}, //字符'q'
    {{ 0, 0, 0, 0, 1, 0, 1, 2 }}, //字符'r'
    {{ 0, 0, 1, 1, 1, 0, 0, 2 }}, //字符'u'
    {{ 0, 1, 1, 1, 0, 1, 1, 2 }}, //字符'y' //36
    {{ 0, 0, 0, 0, 0, 0, 1, 2 }}, //字符'-' //37
    {{ 0, 0, 0, 1, 0, 0, 0, 2 }}, //字符'_' 
    {{ 0, 0, 0, 0, 0, 0, 1, 2 }}, //字符'~' //39
    {{ 0, 0, 0, 1, 1, 1, 1, 2 }}  //字符't' //40
};

//==============================================================================================================================================================
//==========以下是LCD编码，用户需要根据《芯翼XY2100产品LCD使用指导_V0.1》来更改=====================================================================================
//==============================================================================================================================================================
#define USE_JK_4P5V_LCD 0    //1：使用4.5V液晶屏编码
#define USE_JK_3P3V_LCD 0    //1：使用3.3V液晶屏编码
#define USE_WX_3P3V_LCD 0    //1：使用3.3V液晶屏编码
#define USE_JC_3P3V_LCD 1    //1：使用3.3V液晶屏编码

/***************************************************************************
 * LCD屏幕七段字符区的xyseg号编码
 * 注意：完成编码表格调整后，按照LCD屏幕上字符区从左往右显示的顺序排列出的xyseg号，
 * McuLcdCharSeg中第一个元素表示LCD屏幕上第一个七段字符。
***************************************************************************/
#if USE_JK_4P5V_LCD
#define LCD_CHAR_SEG_NUM (8) //七段字符区总共的芯翼SEG号个数
#define LCD_COM_NUM  (8)
#elif USE_JK_3P3V_LCD
#define LCD_CHAR_SEG_NUM (8) //七段字符区总共的芯翼SEG号个数
#define LCD_COM_NUM  (8)
#elif USE_WX_3P3V_LCD
#define LCD_CHAR_SEG_NUM (8) //七段字符区总共的芯翼SEG号个数
#define LCD_COM_NUM  (8)
#elif USE_JC_3P3V_LCD
#define LCD_CHAR_SEG_NUM (20) //七段字符区总共的芯翼SEG号个数
#define LCD_COM_NUM  (4)
#endif

static uint8_t McuLcdCharSeg[LCD_CHAR_SEG_NUM] =
{
#if USE_JK_4P5V_LCD
    3, 26, 20, 9, 14, 25, 27, 18
#endif
#if USE_JK_3P3V_LCD
    16, 15, 19, 17, 20, 25, 26, 27
#endif
#if USE_WX_3P3V_LCD
    0, 27, 26, 25, 20, 9, 14, 3
#endif
#if USE_JC_3P3V_LCD
    19, 21, 22, 23, 24, 30, 17, 18, 16, 15, 8, 4, 0, 27, 26, 25, 20, 9, 14, 3
#endif
};

/*****************************************************************************************
 * LCD屏幕七段字符区的xycom号编码
 * 注意：完成编码表格调整后，按照表格中从上到下的顺序排列出xycom号对应的七段字符区的各个段，
 * McuLcdCharCom中成员变量从左往右，第一个是xycom0对应的字符段码，第二个是xycom1对应的字符段码
*****************************************************************************************/
static McuLcdXycomChar_t McuLcdCharCom = 
{
#if USE_JK_4P5V_LCD
    'C','B','E','A','G','F','X','D'
#endif
#if USE_JK_3P3V_LCD
    'E','A','F','B','G','C','D','X'
#endif
#if USE_WX_3P3V_LCD
    'C','E','D','X','A','B','F','G' 
#endif
#if USE_JC_3P3V_LCD
    'X','E','G','F','D','C','B','A' 
#endif
};

/************************************************************************************
 * LCD屏幕说明区的xycom号xyseg号编码：如剩余、累计量、单价、P5、P6、P7、元、立方米等的编码
 * 注意：McuLcdExp中不使用的成员变量请填为0xFF，
 * 成员变量从左往右，第一个是xycom号，第二个是xyseg号。
************************************************************************************/
static McuLcdBit_t McuLcdExp[32] = 
{
#if USE_JK_4P5V_LCD
    {6,26     },  //s2 剩余
    {6,20     },  //s3 累计量
    {0xFF,0xFF},  //累购(x)
    {0xFF,0xFF},  //表号(x)
    {6,14     },  //p5 小数点
    {6,25     },  //p6 小数点
    {6,27     },  //p7 小数点
    {0xFF,0xFF},  //保留1

    {6,18     },  //s7 立方米
    {6,1      },  //s6 元
    {0xFF,0xFF},  //能量(x)
    {0xFF,0xFF},  //温度(x)
    {0xFF,0xFF},  //压强(x)
    {0xFF,0xFF},  //力(x)
    {0xFF,0xFF},  //保留2
    {0xFF,0xFF},  //保留3

    {0xFF,0xFF},  //冒号1(x)
    {0xFF,0xFF},  //冒号2(x)
    {6,9      },  //s4 单价
    {0xFF,0xFF},  //时间(x)
    {0xFF,0xFF},  //保留4
    {0xFF,0xFF},  //保留5
    {0xFF,0xFF},  //保留6
    {0xFF,0xFF},  //保留7

    {0xFF,0xFF},  //保留8
    {0xFF,0xFF},  //保留9
    {0xFF,0xFF},  //保留10
    {0xFF,0xFF},  //保留11
    {0xFF,0xFF},  //保留12
    {0xFF,0xFF},  //保留13
    {0xFF,0xFF},  //保留14
    {0xFF,0xFF}   //保留15
#endif
#if USE_JK_3P3V_LCD
    {7,15     },  //s2 剩余
    {7,19     },  //s3 累计量
    {0xFF,0xFF},  //累购(x)
    {0xFF,0xFF},  //表号(x)
    {7,20     },  //p5 小数点
    {7,25     },  //p6 小数点
    {7,26     },  //p7 小数点
    {0xFF,0xFF},  //保留1

    {7,27     },  //s7 立方米
    {7,18     },  //s6 元
    {0xFF,0xFF},  //能量(x)
    {0xFF,0xFF},  //温度(x)
    {0xFF,0xFF},  //压强(x)
    {0xFF,0xFF},  //力(x)
    {0xFF,0xFF},  //保留2
    {0xFF,0xFF},  //保留3

    {0xFF,0xFF},  //冒号1(x)
    {0xFF,0xFF},  //冒号2(x)
    {7,17     },  //s4 单价
    {0xFF,0xFF},  //时间(x)
    {0xFF,0xFF},  //保留4
    {0xFF,0xFF},  //保留5
    {0xFF,0xFF},  //保留6
    {0xFF,0xFF},  //保留7

    {0xFF,0xFF},  //保留8
    {0xFF,0xFF},  //保留9
    {0xFF,0xFF},  //保留10
    {0xFF,0xFF},  //保留11
    {0xFF,0xFF},  //保留12
    {0xFF,0xFF},  //保留13
    {0xFF,0xFF},  //保留14
    {0xFF,0xFF}   //保留15
#endif
#if USE_WX_3P3V_LCD
    { 6, 18 }, //剩余
    { 5, 18 }, //累计量
    { 4, 18 }, //累购(购入)
    { 7, 17 }, //表号
    { 3, 26 }, //小数点1
    { 3, 25 }, //小数点2
    { 3, 20 }, //小数点3
    { 3, 9 }, //小数点4

    { 3, 1 }, //立方米(m³)
    { 2, 1 }, //元
    { 0xFF, 0xFF }, //能量
    { 2, 17 }, //温度
    { 3, 17 }, //压强(压力)
    { 0xFF, 0xFF }, //力
    { 3, 14 }, //小数点5
    { 1, 1 }, //摄氏度(℃)

    { 0xFF, 0xFF }, //冒号1
    { 0xFF, 0xFF }, //冒号2
    { 4, 17 }, //单价(价格)
    { 1, 17 }, //时间
    { 5, 17 }, //工况
    { 6, 17 }, //标况
    { 0xFF, 0xFF }, //保留7
    { 3, 27 }, //千帕、兆帕(Kpa\Mpa)

    { 0xFF, 0xFF }, //保留8
    { 0xFF, 0xFF }, //保留9
    { 0xFF, 0xFF }, //保留10
    { 0xFF, 0xFF }, //保留11
    { 0xFF, 0xFF }, //保留12
    { 0xFF, 0xFF }, //保留13
    { 0xFF, 0xFF }, //保留14
    { 0xFF, 0xFF }  //保留15
#endif
#if USE_JC_3P3V_LCD
    { 0, 19 }, //T25
    { 0, 22 }, //T1
    { 0, 24 }, //T2
    { 0, 17 }, //T3
    { 0, 16 }, //T4
    { 0, 8 },  //P1
    { 0, 0 },  //T20
    { 0, 26},  //T21
    { 0, 20},  //T22
    { 0, 14},  //T23
    { 0, 1},   //T11
    { 1, 1},   //T12
    { 2, 1},   //T8
    { 3, 1},   //T24
    { 0, 29},  //T10
    { 1, 29},  //T6
    { 2, 29},  //T7
    { 3, 29},  //T5
    { 0, 28},  //P5
    { 1, 28},  //P4
    { 2, 28},  //P3
    { 3, 28},  //P2
    { 0xFF, 0xFF }, //保留1
    { 0xFF, 0xFF }, //保留2
    { 0xFF, 0xFF }, //保留3
    { 0xFF, 0xFF }, //保留4
    { 0xFF, 0xFF }, //保留5
    { 0xFF, 0xFF }, //保留6
    { 0xFF, 0xFF }, //保留7
    { 0xFF, 0xFF }, //保留8
    { 0xFF, 0xFF }, //保留9
    { 0xFF, 0xFF }  //保留10
#endif
};

/*********************************************************************************
 * LCD屏幕状态区xycom号xyseg号编码：阀开、阀关、电池1、电池2、电池3、电池4、通讯的编码
 * 注意：McuLcdStatus中不使用的成员变量请填为0xFF，
 * 成员变量从左往右，第一个是xycom号，第二个是xyseg号。
*********************************************************************************/
static McuLcdBit_t McuLcdStatus[32] = 
{
#if USE_JK_4P5V_LCD
    {0xFF,0xFF},  //不足(x)
    {0xFF,0xFF},  //异常(x)
    {0xFF,0xFF},  //电池0(x)
    {1,1      },  //t1 电池1
    {4,1      },  //t2 电池2
    {2,1      },  //t3 电池3
    {0,1      },  //t4 电池4
    {0xFF,0xFF},  //电池(x)

    {0xFF,0xFF},  //磁干扰(x)
    {3,1      },  //h2 阀开
    {5,1      },  //h1 阀关
    {0xFF,0xFF},  //命令错(x)
    {0xFF,0xFF},  //读卡中(x)
    {0xFF,0xFF},  //USB(x)
    {0xFF,0xFF},  //wifi(x)
    {0xFF,0xFF},  //透支(x)

    {7,1      },  //s5 通讯
    {0xFF,0xFF},  //开户(x)
    {0xFF,0xFF},  //保留1
    {0xFF,0xFF},  //保留2
    {0xFF,0xFF},  //保留3
    {0xFF,0xFF},  //保留4
    {0xFF,0xFF},  //保留5
    {0xFF,0xFF},  //保留6

    {0xFF,0xFF},  //保留7
    {0xFF,0xFF},  //保留8
    {0xFF,0xFF},  //保留9
    {0xFF,0xFF},  //保留10
    {0xFF,0xFF},  //保留11
    {0xFF,0xFF},  //保留12
    {0xFF,0xFF},  //保留13
    {0xFF,0xFF}   //保留14
#endif
#if USE_JK_3P3V_LCD
    {0xFF,0xFF},  //不足(x)
    {0xFF,0xFF},  //异常(x)
    {0xFF,0xFF},  //电池0(x)
    {1,18     },  //t1 电池1
    {2,18     },  //t2 电池2
    {3,18     },  //t3 电池3
    {4,18     },  //t4 电池4
    {0xFF,0xFF},  //电池(x)

    {0xFF,0xFF},  //磁干扰(x)
    {5,18     },  //h2 阀开
    {6,18     },  //h1 阀关
    {0xFF,0xFF},  //命令错(x)
    {0xFF,0xFF},  //读卡中(x)
    {0xFF,0xFF},  //USB(x)
    {0xFF,0xFF},  //wifi(x)
    {0xFF,0xFF},  //透支(x)

    {0,18     },  //s5 通讯
    {7,16     },  //s1 开户
    {0xFF,0xFF},  //保留1
    {0xFF,0xFF},  //保留2
    {0xFF,0xFF},  //保留3
    {0xFF,0xFF},  //保留4
    {0xFF,0xFF},  //保留5
    {0xFF,0xFF},  //保留6

    {0xFF,0xFF},  //保留7
    {0xFF,0xFF},  //保留8
    {0xFF,0xFF},  //保留9
    {0xFF,0xFF},  //保留10
    {0xFF,0xFF},  //保留11
    {0xFF,0xFF},  //保留12
    {0xFF,0xFF},  //保留13
    {0xFF,0xFF}   //保留14
#endif
#if USE_WX_3P3V_LCD
    { 0xFF, 0xFF }, //不足
    { 0xFF, 0xFF }, //异常
    { 0xFF, 0xFF }, //电池0
    { 0, 18 }, //t1 电池1
    { 1, 18 }, //t2 电池2
    { 7, 18 }, //t3 电池3
    { 2, 18 }, //t4 电池4
    { 3, 0 }, //电池(换电池)

    { 0, 17 }, //磁干扰
    { 4, 1 }, //阀开
    { 3, 3 }, //阀关
    { 0xFF, 0xFF }, //命令错
    { 0xFF, 0xFF }, //读卡中
    { 0xFF, 0xFF }, //USB
    { 0xFF, 0xFF }, //wifi
    { 3, 18 }, //透支(请充值)

    { 0xFF, 0xFF }, //通讯
    { 0xFF, 0xFF }, //开户
    { 5, 1 }, //A 信号强度1
    { 6, 1 }, //B 信号强度2
    { 7, 1 }, //C 信号强度3
    { 0, 1 }, //D 信号强度4
    { 0xFF, 0xFF }, //保留5
    { 0xFF, 0xFF }, //保留6

    { 0xFF, 0xFF }, //保留7
    { 0xFF, 0xFF }, //保留8
    { 0xFF, 0xFF }, //保留9
    { 0xFF, 0xFF }, //保留10
    { 0xFF, 0xFF }, //保留11
    { 0xFF, 0xFF }, //保留12
    { 0xFF, 0xFF }, //保留13
    { 0xFF, 0xFF }  //保留14
#endif
#if USE_JC_3P3V_LCD
    { 0, 22 }, //T1
    { 0, 24 }, //T2
    { 0, 17 }, //T3
    { 0, 16 }, //T4
    { 3, 29},  //T5
    { 1, 29},  //T6
    { 2, 29},  //T7
    { 2, 1},   //T8
    { 0xFF, 0xFF }, //保留1
    { 0, 29},  //T10
    { 0, 1},   //T11
    { 1, 1},   //T12
    { 0xFF, 0xFF }, //保留2
    { 0xFF, 0xFF }, //保留3
    { 0xFF, 0xFF }, //保留4
    { 0xFF, 0xFF }, //保留5
    { 0xFF, 0xFF }, //保留6
    { 0xFF, 0xFF }, //保留7
    { 0xFF, 0xFF }, //保留8
    { 0, 0 },  //T20
    { 0, 26},  //T21
    { 0, 20},  //T22
    { 0, 14},  //T23
    { 3, 1},   //T24
    { 0, 19 }, //T25
    { 0, 8 },  //P1
    { 3, 28},  //P2
    { 2, 28},  //P3
    { 1, 28},  //P4
    { 0, 28},  //P5
    { 0xFF, 0xFF }, //保留9
    { 0xFF, 0xFF }  //保留10
#endif
};

//==============================================================================================================================================================
//==========以下是LCD局部函数定义，用户不得更改和调用==============================================================================================================
//==============================================================================================================================================================
/************************************************************************************************
 * @brief 七段字符区，位显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdSetCharBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[0][ComNum] |= (0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 七段字符区，位不显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdResetCharBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[0][ComNum] &= ~(0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 说明区，位显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdSetExpBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[1][ComNum] |= (0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 说明区，位不显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdResetExpBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[1][ComNum] &= ~(0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 状态区，位显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdSetStatusBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[2][ComNum] |= (0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 状态区，位不显示选择
 * @param ComNum : 0-7，表示选中的com口
 * @param SegNum ：0-34，表示com口上选中的段
 * @retval none
*************************************************************************************************/
static void McuLcdResetStatusBit(uint8_t ComNum, uint8_t SegNum)
{
    if(ComNum > 7 || SegNum > 34)
    {
    	debug_assert(0);
        return;
    }
    McuLcdCom[2][ComNum] &= ~(0x1ULL << SegNum);
}

/************************************************************************************************
 * @brief 显示七段字符区、说明区、状态区的所有位
 * @param none
 * @retval none
*************************************************************************************************/
static void McuLcdWriteAllBit(void)
{
    for(uint8_t num = 0; num < LCD_COM_NUM; num++)
    {
        *((volatile uint32_t *)&(LCDC->COM0LOW)  + (num * 2)) = ( McuLcdCom[0][num] & 0xFFFFFFFF)        | ( McuLcdCom[1][num] & 0xFFFFFFFF)        | ( McuLcdCom[2][num] & 0xFFFFFFFF);
        *((volatile uint32_t *)&(LCDC->COM0HIGH) + (num * 2)) = ((McuLcdCom[0][num] >> 32) & 0xFFFFFFFF) | ((McuLcdCom[1][num] >> 32) & 0xFFFFFFFF) | ((McuLcdCom[2][num] >> 32) & 0xFFFFFFFF);
    }
}

/************************************************************************************************
 * @brief 获取7个字符段对应的芯翼com口号
 * @param Char 字符，注意'X'或'x'表示忽略，
 *        可选：'A'、'B'、'C'、'D'、'E'、'F'、'G'、'X' 或 'a'、'b'、'c'、'd'、'e'、'f'、'g'、'x'
 * @return -1:入参错误
 *         0:'A'或'a'，对应McuLcdCharCom中的com0
 *         1:'B'或'b'，对应McuLcdCharCom中的com1
 *         2:'C'或'c'，对应McuLcdCharCom中的com2
 *         3:'D'或'd'，对应McuLcdCharCom中的com3
 *         4:'E'或'e'，对应McuLcdCharCom中的com4
 *         5:'F'或'f'，对应McuLcdCharCom中的com5
 *         6:'G'或'g'，对应McuLcdCharCom中的com6
 *         7:'X'或'x'，对应McuLcdCharCom中的com7
*************************************************************************************************/
static char McuLcdGetCharCom(char Char)
{
    if(Char == 'A' || Char == 'a')
    {
        return 0;
    }
    if(Char == 'B' || Char == 'b')
    {
        return 1;
    }
    if(Char == 'C' || Char == 'c')
    {
        return 2;
    }
    if(Char == 'D' || Char == 'd')
    {
        return 3;
    }
    if(Char == 'E' || Char == 'e')
    {
        return 4;
    }
    if(Char == 'F' || Char == 'f')
    {
        return 5;
    }
    if(Char == 'G' || Char == 'g')
    {
        return 6;
    }
    if(Char == 'X' || Char == 'x')
    {
        return 7;
    }
   	debug_assert(0);
    return -1;
}

/************************************************************************************************
 * @brief 七段字符区用户xycom号编码
 * @param comx 芯翼com0~com7与lcd屏幕中7个字符段码的对应关系，注意'X'或'x'表示忽略，
 *        可选：'A'、'B'、'C'、'D'、'E'、'F'、'G'、'X' 或 'a'、'b'、'c'、'd'、'e'、'f'、'g'、'x'
 * @retval 0：成功；-1：失败
*************************************************************************************************/
static int8_t McuLcdCodeCharTable(McuLcdXycomChar_t xycom)
{
    //获取7个字符段对应的芯翼com口号
    McuLcdXycomChar_t lcdcom = {0};
    lcdcom.char0 = McuLcdGetCharCom(xycom.char0);
    lcdcom.char1 = McuLcdGetCharCom(xycom.char1);
    lcdcom.char2 = McuLcdGetCharCom(xycom.char2);
    lcdcom.char3 = McuLcdGetCharCom(xycom.char3);
    lcdcom.char4 = McuLcdGetCharCom(xycom.char4);
    lcdcom.char5 = McuLcdGetCharCom(xycom.char5);
    lcdcom.char6 = McuLcdGetCharCom(xycom.char6);
    lcdcom.char7 = McuLcdGetCharCom(xycom.char7);

    //参数检查
    for(uint8_t i = 0; i < 8; i++)
    {
        if(*(&lcdcom.char0+i) == -1)
        {
        	debug_assert(0);
            return -1;
        }
    }

    //按行赋值新表
    for (uint8_t j = 0; j < LCD_CHAR_CODE_NUM; j++) //共LCD_CHAR_CODE_NUM行
    {
        McuLcdCharComUser[j].one_com.com0 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char0 * 2))) >> (lcdcom.char0 * 2);
        McuLcdCharComUser[j].one_com.com1 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char1 * 2))) >> (lcdcom.char1 * 2);
        McuLcdCharComUser[j].one_com.com2 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char2 * 2))) >> (lcdcom.char2 * 2);
        McuLcdCharComUser[j].one_com.com3 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char3 * 2))) >> (lcdcom.char3 * 2);
        McuLcdCharComUser[j].one_com.com4 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char4 * 2))) >> (lcdcom.char4 * 2);
        McuLcdCharComUser[j].one_com.com5 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char5 * 2))) >> (lcdcom.char5 * 2);
        McuLcdCharComUser[j].one_com.com6 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char6 * 2))) >> (lcdcom.char6 * 2);
        McuLcdCharComUser[j].one_com.com7 = (McuLcdCharComConst[j].all_com & (3 << (lcdcom.char7 * 2))) >> (lcdcom.char7 * 2);
    }

    return 0;
}

/************************************************************************************************
 * @brief 七段字符区，显示指定字符
 * @param SegNum ：0-34，表示com口上选中的段
 * @param Num : ASCII字符
 * @retval 0：成功；-1：失败
*************************************************************************************************/
int8_t McuLcdWriteNum(uint8_t SegIndex, uint8_t Num)
{
    uint16_t com_value = 0;
    uint8_t i;

    //遍历指定seg下的所有com口目标状态，并设置七段字符区数组
    for(i = 0; i < LCD_COM_NUM; i++)
    {
        com_value = McuLcdCharComUser[Num].all_com & (uint16_t)(3U << i*2);
        if(com_value == 0)
        {
            McuLcdResetCharBit(i,McuLcdCharSeg[SegIndex]);
        }
        else if(com_value == (1U << i*2))
        {
            McuLcdSetCharBit(i,McuLcdCharSeg[SegIndex]);
        }
    }

#if (LCD_COM_NUM == 4)
    SegIndex++;
    for(i = LCD_COM_NUM; i < 2*LCD_COM_NUM; i++)
    {
        com_value = McuLcdCharComUser[Num].all_com & (uint16_t)(3U << i*2);
        if(com_value == 0)
        {
            McuLcdResetCharBit(i-LCD_COM_NUM,McuLcdCharSeg[SegIndex]);
        }
        else if(com_value == (1U << i*2))
        {
            McuLcdSetCharBit(i-LCD_COM_NUM,McuLcdCharSeg[SegIndex]);
        }
    }
#endif

    return 0;
}

/************************************************************************************************
 * @brief 七段字符区，显示指定字符
 * @param SegNum ：0-34，表示com口上选中的段
 * @param Num : ASCII字符
 * @retval 0：成功；-1：失败
*************************************************************************************************/
int8_t McuLcdWriteChar(uint8_t SegIndex, uint8_t Num)
{
    uint16_t com_value = 0;
    uint8_t i;

    //将字符转换成七段字符区编码下标
    if('0' <= Num && Num <= '9')
    {
        Num -= 0x30;
    }
    else if('A' <= Num && Num <= 'Z')
    {
        switch(Num)
        {
            case 'A': { Num = 11;break; }
            case 'C': { Num = 12;break; }
            case 'E': { Num = 13;break; }
            case 'F': { Num = 14;break; }
            case 'G': { Num = 15;break; }
            case 'H': { Num = 16;break; }
            case 'J': { Num = 17;break; }
            case 'L': { Num = 18;break; }
            case 'N': { Num = 19;break; }
            case 'O': { Num = 20;break; }
            case 'P': { Num = 21;break; }
            case 'S': { Num = 22;break; }
            case 'U': { Num = 23;break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
    }
    else if('a' <= Num && Num <= 'z')
    {
        switch(Num)
        {
            case 'b': { Num = 24;break; }
            case 'c': { Num = 25;break; }
            case 'd': { Num = 26;break; }
            case 'g': { Num = 27;break; }
            case 'h': { Num = 28;break; }
            case 'i': { Num = 29;break; }
            case 'l': { Num = 30;break; }
            case 'n': { Num = 31;break; }
            case 'o': { Num = 32;break; }
            case 'q': { Num = 33;break; }
            case 'r': { Num = 34;break; }
            case 'u': { Num = 35;break; }
            case 'y': { Num = 36;break; }
            case 't': { Num = 40;break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
    }
    else
    {
        switch(Num)
        {
            case ' ': { Num = 10;break; }
            case '-': { Num = 37;break; }
            case '_': { Num = 38;break; }
            case '~': { Num = 39;break; }
            default: 
            {
				debug_assert(0);
				return -1; //入参非法
            }
        }
    }

    //遍历指定seg下的所有com口目标状态，并设置七段字符区数组
    for(i = 0; i < LCD_COM_NUM; i++)
    {
        com_value = McuLcdCharComUser[Num].all_com & (uint16_t)(3U << i*2);
        if(com_value == 0)
        {
            McuLcdResetCharBit(i, McuLcdCharSeg[SegIndex]);
        }
        else if(com_value == (1U << i*2))
        {
            McuLcdSetCharBit(i, McuLcdCharSeg[SegIndex]);
        }
    }

#if (LCD_COM_NUM == 4)
    SegIndex++;
    for (i = LCD_COM_NUM; i < 2 * LCD_COM_NUM; i++) //all_com的后4个com值遍历
    {
        com_value = McuLcdCharComUser[Num].all_com & (uint16_t)(3U << i*2);
        if(com_value == 0)
        {
            McuLcdResetCharBit(i - LCD_COM_NUM, McuLcdCharSeg[SegIndex]);
        }
        else if(com_value == (1U << i*2))
        {
            McuLcdSetCharBit(i - LCD_COM_NUM, McuLcdCharSeg[SegIndex]);
        }
    }
#endif

    return 0;
}

/**
 * @brief 配置LCD所用GPIO无输入输出状态下的浮空或者下拉
 * @param ComPad 芯翼com号，从0开始，bit0表示com0，相应bit设置1即选中该脚为com引脚
 * @param Segpad 芯翼seg号，从0开始，bit0表示seg0，相应bit设置1即选中该脚为seg引脚
 * @param status 指示配置下拉还是浮空 0:浮空 1:下拉
 */
static void McuLcdGpioConfig(uint32_t ComPad, uint32_t SegPad, uint8_t status)
{
    GPIO_PadTypeDef total_lcd_valid_gpio[8 + 35] = {0}; //芯翼LCD所用的GPIO引脚，最多8+35个
    uint8_t total_lcd_valid_gpio_cnt = 0; //芯翼LCD所用的GPIO个数
    
    //对com引脚的bit检查，若为1则找到了对应GPIO引脚
    //并把引脚号存放在xylcd_total_gpio中，元素下标累加
    for(uint8_t i = 0; i < 8; i++)
    {
        if(ComPad & (1UL<<i))
        {
            (i == 0) ? (total_lcd_valid_gpio[total_lcd_valid_gpio_cnt] = 14) : (total_lcd_valid_gpio[total_lcd_valid_gpio_cnt] = 19 + i);
            total_lcd_valid_gpio_cnt++;
        }
    }

    //对seg引脚的bit检查，若为1则找到了对应GPIO引脚
    //并把引脚号存放在xylcd_total_gpio中，元素下标累加
    for(uint8_t i = 0; i < 35; i++)
    {
        if(SegPad & (1ULL<<i))
        {
            (i < 28) ? (total_lcd_valid_gpio[total_lcd_valid_gpio_cnt] = 27 + i) : (total_lcd_valid_gpio[total_lcd_valid_gpio_cnt] = i - (2 * (i - 27)));
            total_lcd_valid_gpio_cnt++;
        }
    }

    //开GPIO时钟
    PRCM_ClockEnable(CORE_CKG_CTL_GPIO_EN);

    //禁能LCD引脚的GPIO输入输出
    Gpio_InputConfig(total_lcd_valid_gpio, total_lcd_valid_gpio_cnt, DISABLE);
    Gpio_OutputConfig(total_lcd_valid_gpio, total_lcd_valid_gpio_cnt, DISABLE);

    //配置GPIO上下拉状态
    Gpio_PullupConfig(total_lcd_valid_gpio, total_lcd_valid_gpio_cnt, DISABLE); //禁能上拉
    if(status == 0) //浮空
    {
        Gpio_PulldownConfig(total_lcd_valid_gpio, total_lcd_valid_gpio_cnt, DISABLE); //禁能下拉
    }
    else //下拉
    {
        Gpio_PulldownConfig(total_lcd_valid_gpio, total_lcd_valid_gpio_cnt, ENABLE); //使能下拉
    }
}

//==============================================================================================================================================================
//==========以下是LCD全局函数定义，用户可以调用，使用前请参阅《芯翼XY2100产品LCD使用指导_V0.1》=======================================================================
//==============================================================================================================================================================
/************************************************************************************************
 * @brief LCD初始化。接口耗时：197.5us
 * @param ComPad 芯翼com号，从0开始，bit0表示com0，相应bit设置1即选中该脚为com引脚
 * @param Segpad 芯翼seg号，从0开始，bit0表示seg0，相应bit设置1即选中该脚为seg引脚
 * @param LcdRate LCD屏幕刷新率，单位Hz，如35Hz，70Hz，140Hz
 * @param SupplySel 由LCD屏幕驱动电压和芯片IO电平决定该入参的值，请参考 @ref McuLcdSupplySelTypedef
*************************************************************************************************/
void McuLcdInit(uint32_t ComPad, uint32_t SegPad, uint32_t LcdRate, McuLcdSupplySelTypedef SupplySel)
{
    //记录所用芯翼com号、芯翼seg号
    g_ComPad = ComPad;
    g_SegPad = SegPad;

    //记录LCD速率
    g_LcdRate = LcdRate;

    //七段字符区编码，用户需要编码McuLcdCharCom，请参阅《芯翼XY2100产品LCD使用指导_V0.1》
    McuLcdCodeCharTable(McuLcdCharCom);

    //计算偏置电压bias
    uint8_t bias = 0;
    if(LCD_COM_NUM <= 2) //com口个数为1、2时取1/2bias
    {
        bias = 1;
    }
    else if(LCD_COM_NUM <= 6) //com口个数为3、4、5、6时取1/3bias
    {
        bias = 3; //Ax取3，Bx取2
    }
    else //com口个数为7、8时取1/4bias
    {
        bias = 3;
    }

    //根据LcdRate(帧刷新率)算出lcd参考时钟（即求出rps、div），公式：2^rps × (div+2) = 32K/(com * 2 * LcdRate)
    uint64_t multiple = (uint64_t)Get32kClockFreq() / (LCD_COM_NUM * 2 * LcdRate);
    uint8_t rps = 0, div = 0;
    for(uint8_t j = 0; j <= 64; j++) //64由rps=7时得到的最大间隔值为128，那么最多加或减64个数就可以找到匹配值
    {
        for(rps = 0; rps < 8; rps++)
        {
            for(div = 0; div < 16; div++)
            {
                //先查找是否有完全相等的值，如果没有完全相等的值，则从8组、每组16个数据中挨个加减1，加减2……去匹配
                for(uint8_t i = 0; i < 2; i++) 
                {
                    char sign = (i == 0) ? 1 : -1; //加一次，减一次
                    if(((1ULL << rps) * (div + 2)) == (multiple + sign * j))
                    {
                        goto lcdconfig;
                    }
                }
            }
        }
    }

    //配置LCD
lcdconfig:
    //默认供电方式为外部供电，即VLCD需要外接电源
    //默认偏置电压为1/4
    LCDC_FCRLcdPuSet(1);
    LCDC_PAD_SetMode(SegPad, ComPad);
#if (LCD_COM_NUM == 4)
    LCDC_Init_Config(0xf, rps, div, (uint8_t)SupplySel, 0, LCD_COM_NUM-1, bias, 0);
#else
    LCDC_Init_Config(0, rps, div, (uint8_t)SupplySel, 0, LCD_COM_NUM-1, bias, 0);
#endif
}

/************************************************************************************************
 * @brief LCD使能（显示）。接口耗时：14.6us
 * @param none
 * @retval none
*************************************************************************************************/
void McuLcdEn(void)
{
    //配置LCD所用GPIO为浮空、无输入输出状态，即高阻态
    McuLcdGpioConfig(g_ComPad, g_SegPad, 0); //目的是为了防止可能的漏电
    LCDC_WORKStart();
}

/************************************************************************************************
 * @brief LCD禁能（熄屏），并将LCD功能引脚去使能为普通GPIO功能。接口耗时：50.1us
 * @warning 调用该接口后LCD所用引脚的外部表现由GPIO寄存器决定，再次使用LCD时需要重新初始化LCD
 * @param none
 * @retval none
*************************************************************************************************/
void McuLcdDis(void)
{
    //配置LCD所用GPIO为下拉、无输入输出状态
    McuLcdGpioConfig(g_ComPad, g_SegPad, 1); //目的是为了防止熄屏乱码
    LCDC_WORKStop();
	GPIO_AllPad_Disable_Rcr();
    g_UpdateSta = 0;
}

/************************************************************************************************
 * @brief LCD显示刷新。lcd使能后的首次接口耗时：68.4us，lcd使能后非首次接口耗时：最差情况16ms
 * @param none
 * @retval none
*************************************************************************************************/
void McuLcdUpdate(void)
{
    McuLcdWriteAllBit();
    if(g_UpdateSta)
    {
        LCDC_WORKUpdateWait(g_LcdRate);
        LCDC_WORKUpdate();
    }
    else
    {
        LCDC_WORKUpdate();
        g_UpdateSta = 1;
    }
}

/************************************************************************************
 * @brief  七段字符区，从屏幕左侧往右侧依次显示指定字符串。接口耗时：373.8us
 * @param str：待显示的字符串
 * @retval 0：成功；-1：失败
************************************************************************************/
int8_t McuLcdWriteStr(char *str)
{
    //从左往右依次写字符数据
#if (LCD_COM_NUM == 4)
    for(uint8_t i = 0; i < LCD_CHAR_SEG_NUM/2; i++)
    {
        if((str[i] == '\0') || (str[i] == ' '))
        {
            if(McuLcdWriteChar(2 * i, ' ') == -1)
            {
            	debug_assert(0);
                return -1;
            }
        }
        else
        {
            if(McuLcdWriteChar(2 * i, str[i]) == -1)
            {
            	debug_assert(0);
                return -1;
            }
        }
    }
#else
    for(uint8_t i = 0; i < LCD_CHAR_SEG_NUM; i++)
    {
        if((str[i] == '\0') || (str[i] == ' '))
        {
            if(McuLcdWriteChar(i, ' ') == -1)
            {
            	debug_assert(0);
                return -1;
            }
        }
        else
        {
            if(McuLcdWriteChar(i, str[i]) == -1)
            {
            	debug_assert(0);
                return -1;
            }
        }
    }
#endif
    return 0;
}

/************************************************************************************************
 * @brief 说明区，显示指定说明符号。接口耗时：114.9us
 * @param Char : 说明区字符，按bit生效
 * @retval 0：成功；-1：失败
*************************************************************************************************/
int8_t McuLcdWriteExp(uint32_t Char)
{
    for(uint8_t i = 0; i < 32; i++)
    {
        //若该位为1则该位显示选择
        if(Char & (uint32_t)(1ULL << i)) 
        {
            McuLcdSetExpBit(McuLcdExp[i].com,McuLcdExp[i].seg);
        }
        //若该位为0则该位显示不选择
        else
        {
            McuLcdResetExpBit(McuLcdExp[i].com,McuLcdExp[i].seg);
        }
    }

    return 0;
}

/************************************************************************************************
 * @brief 状态区，显示指定说明符号。接口耗时：114.9us
 * @param Char : 状态区字符，按bit生效
 * @retval 0：成功；-1：失败
*************************************************************************************************/
int8_t McuLcdWriteStatus(uint32_t Char)
{
    for(uint8_t i = 0; i < 32; i++)
    {
        //若该位为1则该位显示选择
        if(Char & (uint32_t)(1ULL << i)) 
        {
            McuLcdSetStatusBit(McuLcdStatus[i].com,McuLcdStatus[i].seg);
        }
        //若该位为0则该位显示不选择
        else
        {
            McuLcdResetStatusBit(McuLcdStatus[i].com,McuLcdStatus[i].seg);
        }
    }

    return 0;
}

/************************************************************************************************
 * @brief 状态区，显示指定说明符号。
 * @param bit_value : 状态区字符对应的bit位，flag：1显示；0不显示
 * @retval 0：成功；-1：失败
*************************************************************************************************/
int8_t McuLcdWriteOneStatus(uint32_t bit_value, uint32_t flag)
{
    int func_idx = 0;

    if (bit_value == 0) return -1;

    while ((bit_value & 1) == 0)
    {
        func_idx++;
        bit_value >>= 1;
    }

    if (func_idx >= 32) return -1;

    //若该位为1则该位显示选择
    if(flag) 
    {
        McuLcdSetStatusBit(McuLcdStatus[func_idx].com,McuLcdStatus[func_idx].seg);
    }
    //若该位为0则该位显示不选择
    else
    {
        McuLcdResetStatusBit(McuLcdStatus[func_idx].com,McuLcdStatus[func_idx].seg);
    }

    return 0;
}

/************************************************************************************
 * @brief  清除屏幕显示
 * @param 
 * @retval 
************************************************************************************/
void McuLcdClear(void)
{
    for(uint8_t num = 0; num < LCD_COM_NUM; num++)
    {
        McuLcdCom[0][num] = 0;
        McuLcdCom[1][num] = 0;
        McuLcdCom[2][num] = 0;
    }
}

/************************************************************************************
 * @brief  全显
 * @param SegPad
 * @retval 
************************************************************************************/
void McuLcdDisplayAll(uint64_t SegPad)
{
    for(uint8_t num = 0; num < LCD_COM_NUM; num++)
    {
        McuLcdCom[0][num] = SegPad;
        McuLcdCom[1][num] = SegPad;
        McuLcdCom[2][num] = SegPad;
    }
}

/************************************************************************************************
 * @brief 将十进制数字转换成ASCII码，转换后为左对齐。接口耗时：79.6us
 * @param dec : 需要转换为ASCII码的十进制数
 * @param asc : 存放转换后的ASCII码
 * @retval none
*************************************************************************************************/
void McuLcdDecToAscii(uint32_t dec, uint8_t *asc)
{
    uint8_t dec_len = 0, i = 0;
    uint32_t dec_to_ascii[LCD_CHAR_SEG_NUM + 1] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};

    //去掉最高位
    while(dec >= dec_to_ascii[LCD_CHAR_SEG_NUM])
    {
        dec -= dec_to_ascii[LCD_CHAR_SEG_NUM];
    }

    //求出十进制数长度
    for(i = LCD_CHAR_SEG_NUM; i > 0; i--)
    {
        if(dec >= dec_to_ascii[i])
        {
            break;
        }
    }
    dec_len = i + 1;

    //转换除个位外的其他高位数字为ASCII码
    for(i = dec_len - 1; i > 0; i--)
    {
        *asc = 0x30;
        while(dec >= dec_to_ascii[i])
        {
            dec -= dec_to_ascii[i];
            (*asc)++;
        }
        asc++;
    }
    *asc = 0x30 + (uint8_t )dec; //直接转换个位数字为ASCII码
}
