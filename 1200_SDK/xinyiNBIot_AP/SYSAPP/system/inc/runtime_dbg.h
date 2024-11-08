#pragma once

/*****************************************************************************************************************************
 * @brief   运行时间统计相关接口
 * 
 * @attention 1、记录运行时间的数组复用了用户数据RAM缓存区（USER_BAK_MEM_BASE），此区域在深睡唤醒与软复位时不清除，其它上电原因时会清0。
 *            
 *             2、Debug_Runtime_Init()在first_excute_in_reset_handler(void)一开始就被调用，其相关初值已经设定，但在之后的BakupMemInit()
 *              中，断电上电或全局复位时，用户数据RAM会清0，导致Debug_Runtime的相关初始被清除，导致Debug_Runtime_Add()等相关值丢失，计
 *              时不准，故建议此接口在深睡唤醒后使用。
 ****************************************************************************************************************************/
#include <stdint.h>
#include "tick.h"
#include "core_cm3.h"

#if(DEBUG_OPENCPU_RUN_TIME == 1)

/*最多支持的统计信息组数*/
#define RUNTIME_INFO_MAXNUM     64

typedef struct 
{
    uint32_t cur_index;      //runtime_info[]的当前下标
    uint32_t total_time;     //总时间消耗，单位us
    struct 
    {
        char *lable_name;     //仅起标识，方便查看，无实质意义
        uint32_t runtime;     //上一次index到当前index的阶段性运行的实际时长，已经减去接口本身误差，单位为us。
    }runtime_info[RUNTIME_INFO_MAXNUM];
    
}RUNTIME_DBG_T;

/*时间戳获取接口*/
#define Debug_Runtime_Get_CurTime()     (SysTick->VAL)

/*此函数无需关注*/
extern void Debug_Runtime_Init(void);

/*调试用，用于分解统计代码流程的函数级耗时，入参字符串仅用于方便阅读查找耗时代码段，无实质含义*/
extern void Debug_Runtime_Add(char *label_name);        

/*用于获取索引对应的函数流程耗时，单位us*/
extern uint32_t Debug_Runtime_Get_us(uint32_t index);
#else
    #define Debug_Runtime_Init()

	/*仅用于调试记录函数级耗时时长，默认关闭*/
    #define Debug_Runtime_Add(str)
    #define Debug_Runtime_Get_us(index)        (0)
#endif


