#include "xy_memmap.h"
#include "runtime_dbg.h"
#include "system.h"
#include "hal_def.h"
#include "driver_utils.h"

#if(DEBUG_OPENCPU_RUN_TIME == 1)
#define SYSTICK_LOAD_RELOAD		(0xFFFFFF)

RUNTIME_DBG_T *p_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);

void Debug_Runtime_Init(void)
{
    uint32_t runtime;
    p_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);

    p_runtime_record->cur_index = 0;
    p_runtime_record->total_time = 0;


    SysTick->LOAD = (uint32_t)SYSTICK_LOAD_RELOAD;
	SysTick->VAL = (uint32_t)SYSTICK_LOAD_RELOAD;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    //下述四个Add接口主要用于计算Debug_Runtime_Add()接口本身的耗时计算
    Debug_Runtime_Add(NULL);//初始基准参考，index==0
    Debug_Runtime_Add(NULL);//index==1，与index==0，内部执行的代码分支不同
    Debug_Runtime_Add(NULL);//index==2，与index==1执行的分支相同，实测发现最后一个的执行时间比之前的要长
    Debug_Runtime_Add(NULL);//index==3,与index==2执行的分支相同,这个作为最后一个，保证index==2不是最后一个，index==2与index==1的执行时间相同
    runtime = p_runtime_record->runtime_info[2].runtime;
    p_runtime_record->cur_index = 1;
    p_runtime_record->runtime_info[0].runtime = runtime;//以index==2时Debug_Runtime_Add()的耗时作为参考
    p_runtime_record->total_time =  0; 
}

/*仅用于调试记录函数级耗时时长，默认关闭*/
void Debug_Runtime_Add(char *label_name)
{
    uint32_t cur_time;
    uint32_t diff = 0;
    if(p_runtime_record->cur_index >= RUNTIME_INFO_MAXNUM)
        return;

    p_runtime_record->runtime_info[p_runtime_record->cur_index].lable_name = label_name;

   if(p_runtime_record->cur_index != 0)
   {
        cur_time = Debug_Runtime_Get_CurTime();
        diff = SYSTICK_CONVERT_10US(SYSTICK_LOAD_RELOAD - cur_time);
        //p_runtime_record->runtime_info[0].runtime实际为Debug_Runtime_Add接口本身的耗时时间
        p_runtime_record->runtime_info[p_runtime_record->cur_index].runtime = diff - p_runtime_record->runtime_info[0].runtime;
   }
   else
   {
        //此分支只会在Debug_Runtime_Init()初始中第一次调用Debug_Runtime_Add()执行一次，p_runtime_record->runtime_info[0].runtime会在Debug_Runtime_Init()中被更新
        p_runtime_record->runtime_info[p_runtime_record->cur_index].runtime = 0; 
   }
    p_runtime_record->total_time += diff - p_runtime_record->runtime_info[0].runtime;
    p_runtime_record->cur_index++;
    SysTick->VAL = (uint32_t)SYSTICK_LOAD_RELOAD;
}

uint32_t Debug_Runtime_Get_us(uint32_t index)
{
     if(index >= p_runtime_record->cur_index)
        return 0;
    return p_runtime_record->runtime_info[index].runtime;
}

#endif