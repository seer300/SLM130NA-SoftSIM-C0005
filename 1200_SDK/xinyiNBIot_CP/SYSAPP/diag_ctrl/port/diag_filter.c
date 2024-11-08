#include "diag_options.h"
#include "diag_filter.h"
#include "factory_nv.h"
#include "xy_memmap.h"
#include "cmsis_os2.h"
#include "softap_nv.h"

#if (DIAG_HEART_BEAT_PACKET == 1)
// 由于使用了动态tick，直接获取当前tick count比较耗时
// 这里通过变量获取，虽和实际tick有一定误差，但是效率较高
extern volatile TickType_t xTickCount;
#undef  DIAG_GET_TICK_COUNT
#define DIAG_GET_TICK_COUNT()   xTickCount
#endif

// 判断睡眠唤醒后，当前是否需要刷新心跳包的标志，此标志在睡眠前更新
#define DIAG_FIRST_TIME_TO_RUN          0    //非深睡开机，初始态
#define DIAG_NEED_REFRESH_HEART         1    //之前log在正常输出，需要维持心跳
#define DIAG_NO_NEED_REFRESH_HEART      2    //没有log输出，无需维持心跳


// log 过滤的位图，分别根据 src_id 和 log lev 进行过滤
static filterInfo diag_filter_bitmap = {0};

#if (DIAG_HEART_BEAT_PACKET == 1)
/*根据最近一次心跳握手报文交互的时刻点，记录下一次期望进行心跳交互的时刻点*/
static uint32_t diag_heart_end_count = 0;
#endif


__FLASH_FUNC void diag_filter_init(void)
{
    diag_filter_bitmap.log_lev_bitmap = 0xFFFF;
    diag_filter_bitmap.sig_pri_bitmap = 0xFFFF;
    diag_filter_bitmap.src_id_bitmap = 0xFFFFFFFF;

    #if (DIAG_HEART_BEAT_PACKET == 1)
    {
        // 根据当前标志位，判断刚开机时是否需要更新心跳包
        if ((g_softap_var_nv->diag_flag == DIAG_FIRST_TIME_TO_RUN) || (g_softap_var_nv->diag_flag == DIAG_NEED_REFRESH_HEART))
        {
            // 更新 tick count 的值
            (void) osKernelGetTickCount();
            diag_heart_end_count = (uint32_t) DIAG_GET_TICK_COUNT() + DIAG_HEART_VALID_TIME;
            g_log_status.heart_end_count = diag_heart_end_count;
        }
    }
    #endif
}
/*----------------------------------------------------------------------------------------------------*/

/*所有log输出必须调用该接口进行过滤。src_id为-1表示特殊log，不参与规则匹配，一律输出。平台重点维护*/
__RAM_FUNC diag_send_state_t diag_filter_get_send_enable_state(XY_SRC_E src_id)
{
    if (g_factory_nv->softap_fac_nv.open_log==0 /*||HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) == 0 */|| 
		g_softap_fac_nv->log_txd_pin == 0xFF || g_softap_fac_nv->log_rxd_pin == 0xFF) 
    {
        return DIAG_SEND_DISABLE;
    }

	/*若心跳超时，表明log物理口已断开，无需吐log*/
#if (DIAG_HEART_BEAT_PACKET == 1)
	if (((uint32_t) DIAG_GET_TICK_COUNT()) >= diag_heart_end_count)
	{
		return DIAG_SEND_DISABLE;
	}
#endif

	if(g_factory_nv->softap_fac_nv.open_log == 1)
		return  DIAG_SEND_ENABLE;

	/*phy不输出*/
	if(g_factory_nv->softap_fac_nv.open_log == 2 && (src_id==LPHY || src_id==L1C))
		return DIAG_SEND_DISABLE;

	/*phy+ps不输出*/
	if(g_factory_nv->softap_fac_nv.open_log == 3 && (src_id==LPHY || src_id==L1C || src_id==LRRC || src_id==LNBPS))
		return DIAG_SEND_DISABLE;

	/*phy+ps+nas不输出*/
	if(g_factory_nv->softap_fac_nv.open_log == 4 && (src_id==LPHY || src_id==L1C || src_id==LRRC || src_id==LNBPS || src_id==NAS || src_id==ATC_AP))
		return DIAG_SEND_DISABLE;

	/*仅USER_LOG+AP_CORE_LOG输出*/
	if(g_factory_nv->softap_fac_nv.open_log == 5 && (src_id!=AP_CORE_LOG) && (src_id!=USER_LOG))
		return DIAG_SEND_DISABLE;

	/*CP核所有log皆不输出*/
	if(g_factory_nv->softap_fac_nv.open_log == 6 && (src_id!=AP_CORE_LOG))
		return DIAG_SEND_DISABLE;
	
    return DIAG_SEND_ENABLE;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_filter_set_log_bitmap(filterInfo * bitmap_info)
{
    diag_filter_bitmap.log_lev_bitmap = bitmap_info->log_lev_bitmap;
    diag_filter_bitmap.sig_pri_bitmap = bitmap_info->sig_pri_bitmap;
    diag_filter_bitmap.src_id_bitmap = bitmap_info->src_id_bitmap;
}

__FLASH_FUNC void diag_filter_set_level(uint32_t val)
{
    diag_filter_bitmap.log_lev_bitmap = (uint16_t)val;
}

__FLASH_FUNC void diag_filter_set_src(uint32_t val)
{
    diag_filter_bitmap.src_id_bitmap = val;
}

/*更新期望的下一次心跳交互时刻点*/
__FLASH_FUNC void diag_filter_refresh_heart_beat(void)
{
    #if (DIAG_HEART_BEAT_PACKET == 1)
    {
        g_log_status.heart_recv_tick = (uint32_t)DIAG_GET_TICK_COUNT();
        diag_heart_end_count = (uint32_t) DIAG_GET_TICK_COUNT() + DIAG_HEART_VALID_TIME;
        g_log_status.heart_end_count = diag_heart_end_count;
    }
    #endif
}
/*----------------------------------------------------------------------------------------------------*/

/*睡眠前把log通信是否正常标识保存到易变NV中，以便唤醒后获悉状态*/
__RAM_FUNC void diag_filter_refresh_heart_flag(void)
{
    #if (DIAG_HEART_BEAT_PACKET == 1)
    {
        // 更新 tick count 的值
        (void) osKernelGetTickCount();

        if ((uint32_t) DIAG_GET_TICK_COUNT() >= diag_heart_end_count)
        {
            g_softap_var_nv->diag_flag = DIAG_NO_NEED_REFRESH_HEART;
        }
        else
        {
            g_softap_var_nv->diag_flag = DIAG_NEED_REFRESH_HEART;
        }
    }
    #endif
}
/*----------------------------------------------------------------------------------------------------*/

/*睡眠唤醒后，若发现睡眠前是有log通信的，则唤醒后立即喂狗心跳超时时间*/
__RAM_FUNC void diag_filter_refresh_heart_if_needed(void)
{
    #if (DIAG_HEART_BEAT_PACKET == 1)
    {
        if (g_softap_var_nv->diag_flag == DIAG_NEED_REFRESH_HEART)
        {
            // 更新 tick count 的值
            (void) osKernelGetTickCount();
            diag_heart_end_count = (uint32_t) DIAG_GET_TICK_COUNT() + DIAG_HEART_VALID_TIME;
            g_log_status.heart_end_count = diag_heart_end_count;
        }
    }
    #endif
}
/*----------------------------------------------------------------------------------------------------*/

/*仅用于字符串明文打印的过滤，不适用于PS的原语输出过滤*/
__RAM_FUNC diag_send_state_t diag_filter_normal_log(XY_SRC_E src_id, XY_LOG_LEV lev)
{
    // source id 不能超过最大值
    if (src_id >= XYLOG_MAX_BLOCK)
    {
        return DIAG_SEND_DISABLE;
    }

    // 当前的 source id 的位图要处于使能状态
    if (((diag_filter_bitmap.src_id_bitmap) & (1 << src_id)) == 0)
    {
        return DIAG_SEND_DISABLE;
    }

    // log level 不能超过最大值
    if (lev >= MAX_LEV_LOG)
    {
        return DIAG_SEND_DISABLE;
    }
    // 当前的 log level 的位图要处于使能状态
    if (((diag_filter_bitmap.log_lev_bitmap) & (1 << lev)) == 0)
    {
        return DIAG_SEND_DISABLE;
    }

    return DIAG_SEND_ENABLE;
}
/*----------------------------------------------------------------------------------------------------*/

/*PS模块内输出结构体内容过滤使用，其他团队不适用*/
__FLASH_FUNC diag_send_state_t diag_filter_signalling_log(diag_ps_type_t signalling_type)
{
    (void) signalling_type;

    // 对应信令的位图要处于使能状态
    if (((diag_filter_bitmap.sig_pri_bitmap) & (1 << 0)) == 0)
    {
        return DIAG_SEND_DISABLE;
    }

    return DIAG_SEND_ENABLE;
}
/*----------------------------------------------------------------------------------------------------*/

/*PS与phy等模块间标准消息原语的过滤*/
__FLASH_FUNC diag_send_state_t diag_filter_primitive_log(uint32_t class_id, uint32_t msg_id)
{
    (void) class_id;
    (void) msg_id;

    // 对应原语的位图要处于使能状态
    if (((diag_filter_bitmap.sig_pri_bitmap) & (1 << 1)) == 0)
    {
        return DIAG_SEND_DISABLE;
    }

    return DIAG_SEND_ENABLE;
}
/*----------------------------------------------------------------------------------------------------*/
