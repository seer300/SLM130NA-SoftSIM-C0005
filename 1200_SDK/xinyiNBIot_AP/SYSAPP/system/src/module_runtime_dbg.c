#include "xy_memmap.h"
#include "module_runtime_dbg.h"
#include "system.h"
#include "hal_def.h"


#if(DEBUG_MODULE_RUN_TIME == 1)

RUNTIME_DBG_T *p_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);

void Debug_MODULE_Runtime_Init(void)
{
    uint32_t runtime;
    p_runtime_record = (RUNTIME_DBG_T *)(BAK_MEM_RUN_TIME_STATISTICS + 0x100);

    p_runtime_record->cur_index = 0;
    p_runtime_record->total_time = 0;
    p_runtime_record->start_time = 0;
    p_runtime_record->last_time = 0;  
}

void Debug_MODULE_Runtime_Add(char *label_name)
{
    uint32_t cur_time;
    if(p_runtime_record->cur_index >= RUNTIME_INFO_MAXNUM)
        return;

    p_runtime_record->runtime_info[p_runtime_record->cur_index].lable_name = label_name;

    if(p_runtime_record->cur_index == 0)
    {
        p_runtime_record->start_time = Tick_CounterGet();
        p_runtime_record->runtime_info[p_runtime_record->cur_index].runtime = 0;
        p_runtime_record->last_time = p_runtime_record->start_time;
    }
    else
    {
        cur_time = Tick_CounterGet();
        p_runtime_record->runtime_info[p_runtime_record->cur_index].runtime = (cur_time - p_runtime_record->last_time);
        p_runtime_record->last_time = cur_time;
    }

    p_runtime_record->total_time = (p_runtime_record->last_time - p_runtime_record->start_time);
    p_runtime_record->cur_index++;
}

uint32_t Debug_MODULE_Runtime_Get_ms(uint32_t index)
{
    if(index >= p_runtime_record->cur_index)
        return 0;
    return p_runtime_record->runtime_info[index].runtime;
}

#endif