#pragma once

/*****************************************************************************************************************************
 * @brief   模组运行时间统计相关接口
 ****************************************************************************************************************************/
#include <stdint.h>
#include "tick.h"
#include "core_cm3.h"

#if(DEBUG_MODULE_RUN_TIME == 1)

/*最多支持的统计信息组数*/
#define RUNTIME_INFO_MAXNUM     32

/*clocktick作为时钟源，Tick_CounterGet()作为时间戳获取接口，精度为ms*/
typedef struct 
{
    uint32_t cur_index;      //runtime_info[]的当前下标
    uint32_t start_time;     //第一个时间戳，仅clocktick作为时钟源时有意义
    uint32_t last_time;      //最后一个时间戳，仅clocktick作为时钟源时有意义
    uint32_t total_time;     //总时间消耗，单位ms
    struct 
    {
        char *lable_name;     //仅起标识，方便查看，无实质意义
        uint32_t runtime;     //上一次index到当前index的阶段性运行的实际时长，单位为ms。
    }runtime_info[RUNTIME_INFO_MAXNUM];
}RUNTIME_DBG_T;

/*此函数无需关注*/
extern void Debug_MODULE_Runtime_Init(void);

/*用于分解统计代码流程的函数级耗时，入参字符串仅用于方便阅读查找耗时代码段，无实质含义*/
extern void Debug_MODULE_Runtime_Add(char *label_name);        

/*用于获取索引对应的函数流程耗时，单位ms*/
extern uint32_t Debug_MODULE_Runtime_Get_ms(uint32_t index);
#else
    #define Debug_MODULE_Runtime_Init()
    #define Debug_MODULE_Runtime_Add(str)
    #define Debug_MODULE_Runtime_Get_ms(index)        (0)
#endif


