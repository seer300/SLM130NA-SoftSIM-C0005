#pragma once

#include "xy_utils.h"
#include "xy_lpm.h"
#include "xy_memmap.h"
#include "xy_sys_hook.h"


/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
 
/* 通用函数返回值,建议多返回值场景使用，如网络云API接口；对于简单的bool类型API，建议用bool值 */
typedef enum {
	XY_OK					  =  0, 		
	XY_ERR					  = -1, 		// normal err
	XY_Err_Timeout			  = -2, 		// Operation not completed within the timeout period.
	XY_Err_NoConnected		  = -3, 		// tcpip newwork unusable,such as  PDP not activate
	XY_Err_Parameter		  = -4, 		// input parameter error.
	XY_Err_NoMemory 		  = -5, 		// System is out of memory: it was impossible to allocate or reserve memory for the operation.
	XY_Err_NotAllowed		  = -6, 		// api not allowed,such as SIM card not insert,and do something about 3GPP operation.
	XY_Err_LowVbat			  = -7, 		// low Vbat,can not do flash write
    XY_Err_DnsFail            = -8,         // host unreachalbe or dns parse fail
    XY_Err_SockNoConn         = -9,         // tcp socket not connected
    XY_Err_AddrInUse          = -10,        // socket addr has in used
    XY_Err_InProgress         = -11,        // other progress is doing
	XY_Err_Reserved 		  = 0x7FFFFFFF	
} xy_ret_Status_t;


/* @brief 软重启的细分类型，由xy_Soft_Reset调用设置 */
/* @brief Subdivision type of soft reset, setted by XY_ Soft_ Reset API */
typedef enum
{
    SOFT_RB_BY_NRB,             //  AT+NRB重启
    SOFT_RB_BY_RESET,           //  AT+RESET软重启，恢复出厂设置，类似重新烧录版本
    SOFT_RB_BY_FOTA,            //  FOTA软重启
    SOFT_RB_BY_LOW_VOL,         //  low vBAT软重启，暂未使用
    SOFT_RB_BY_AP_USER,         //  AP用户调用xy_Soft_Reset触发
    SOFT_RB_BY_CP_USER,         //  CP用户调用xy_Soft_Reset触发
} Soft_Reset_Type;

/* @brief 芯片重启细分原因 */
/* @brief Subdivision type of chip reset */
typedef enum
{
    PIN_RESET,                  //  按键复位,整个芯片复位
    SOC_RESET,                  //  CP核看门狗 + xy_Soc_Reset + assert异常断言
    WDT_RESET,                  //  AP_WDT/UTC_WDT复位
    LVD_RESET,                  //  LVD复位
    SVD_RESET,                  //  SVD复位
    DFTGLB_RESET,               //  DFTGLB复位
} Global_Reset_Type;


/* @brief 不建议客户使用！深睡唤醒细分原因，目前仅供模组形态调试使用 */
/* @warning 由于可能存在多个唤醒源并发场景，进而造成唤醒原因不准，不建议客户使用细分原因 */
typedef enum
{
    AT_WAKUP,       //AT的LPUART唤醒
    UTC_WAKUP,      //UTC唤醒,仅CP核使用
} Wake_Deepsleep_Type;


/***************************************************************************************************************************** 
 *@brief  系统开机原因，从SOC寄存器中读取.
 * @warning  SOFT_RESET、WAKEUP_DSLEEP上电情况下，retention memory内存保持不变，由软件进行差异化使用
 *****************************************************************************************************************************/
/***************************************************************************************************************************** 
 *@brief  the reason of system startup，readed from SoC register.
 * @warning  For SOFT_RESET or WAKEUP_DSLEEP, the retention memory remains unchanged and used differentially by software
 *****************************************************************************************************************************/
typedef enum
{
    POWER_ON = 1,            //  正常断电后上电,OPENCPU形态stop_CP复用该状态，子原因为1
    GLOBAL_RESET,            //  断言等软件异常的全局复位，子类型参看Global_Reset_Type
    SOFT_RESET,              //  通过接口xy_soft_reset进行软复位，子类型参看Soft_Reset_Type
    WAKEUP_DSLEEP,           //  深睡唤醒，子类型参看Wake_Deepsleep_Type
} Boot_Reason_Type;


extern uint64_t g_sys_start_time;


/**
 * @brief  使用heap_7进行内存管理，支持离散的堆内存管理
 */
 /*申请不到内存会触发assert断言*/
#define xy_malloc(ulSize)   		XY_MALLOC(ulSize)
/*申请不到内存，返回NULL。通常用于AT命令和数据通信的内存申请，以进行相关容错操作*/
#define xy_malloc2(ulSize)           XY_MALLOC_LOOSE(ulSize)
#define xy_free(mem)				xy_MemFree(mem)

/*返回32字节对齐内存地址*/
#define xy_malloc_Align(ulSize)         XY_MALLOC_ALIGN(ulSize)
#define xy_malloc2_Align(ulSize)  		XY_MALLOC_ALIGN_LOOSE(ulSize)

/**
 * @brief user_printf专供客户使用，用法与printf库函数一致
 * 
 * @note 用户可以通过AT+NV=SET,LOG,<val>命令来修改输出的log内容，以方便客户调试开发。  \n
 * 其中0表示关闭log输出，常用于产线发货；1表示抓取所有log；2表示仅抓取AP核log，不包含3GPP任何log;3表示仅抓取使用该接口输出的用户log
 */
#define user_printf(fmt, ...) PrintUserLog(USER_LOG, WARN_LOG, fmt, ##__VA_ARGS__)

/* 芯翼内部打印使用 */
#define xy_printf(id, src, loglevel, fmt, ...) PrintLog(id, src, loglevel, fmt, ##__VA_ARGS__)


#define xy_assert(a)	Sys_Assert(a)


/*debug调试时断言，以快速定位问题*/
#define debug_assert(a)   \
	if(g_softap_fac_nv->off_debug == 0)  \
		{xy_assert(a);}


/*编译时静态检查断言，通常用于排查数组结构体等大小超设定值异常*/
#define _static_glue2(x, y) x ## y
#define _static_glue(x, y) _static_glue2(x, y)
#define xy_static_assert(exp) \
    typedef char _static_glue(static_assert, __LINE__) [(exp) ? 1 : -1]



/* 基于芯片上电原因的细分子状态，如Global_Reset_Type、Soft_Reset_Type、Wake_Deepsleep_Type。*/
#define  Get_Boot_Sub_Reason()  HWREG(BAK_MEM_CP_UP_SUBREASON)


/* 芯片的上电原因，具体参看Boot_Reason_Type */
#define  Get_Boot_Reason() HWREGB(BAK_MEM_CP_UP_REASON)


/**
 * @brief 指示当前形态是否为OPENCPU形态。若是，则需要执行差异化动作，如不能写flash等
 */
#define  Is_OpenCpu_Ver()  (HWREGB(BAK_MEM_MODULE_VER) == 0)


/**
 * @brief 指示芯片是否从深睡模式唤醒。深睡唤醒后，所有NV皆有效，可以无需attach；否则，需要attach
 */
#define  Is_WakeUp_From_Dsleep()  (Get_Boot_Reason() == WAKEUP_DSLEEP)


/**
 * @brief 仅供模组形态使用，指示芯片是否由于RTC到期触发深睡唤醒，通常用于识别DRX/eDRX的自唤醒
 */
#define  IS_WAKEUP_BY_RTC()      (Is_WakeUp_From_Dsleep() && (Get_Boot_Sub_Reason() & (1 << UTC_WAKUP)))


/**
 * @brief 仅供模组形态debug使用，指示芯片是否自发深睡唤醒（非外部触发，包括RTC超时唤醒、RC32K自校准唤醒）
 */
#define  IS_WAKEUP_BY_SOC()      (Is_WakeUp_From_Dsleep() && (Get_Boot_Sub_Reason() & ((1 << UTC_WAKUP) )))


/**
 * @brief 已废弃！指示CP核深睡唤醒是否执行快速恢复。
 */
bool Is_UP_From_FastRecvry();


/**
* @brief  获取CP核的系统主频时钟，必须用64位进行运算。其值为PLL的368.64M的3.5分频，约105M
*/
#define GetCPClockFreq()    (uint64_t)(BBPLL_CLK_DEFAULT*7/2)


/**
* @brief  软重启，由业务软件调用，aon区域和retention memory内存区域供电保持，内容有效;flash内容不会被损坏
* @warning 最终的软复位动作由AP核执行
*/
void xy_Soft_Reset(Soft_Reset_Type soft_rst_reason);

/**
* @brief  芯片级软复位，所有外设均会断电再上电，等效于硬重启PIN_RESET；flash内容可能被损坏，禁止用户使用
*/
void xy_Soc_Reset(void);



/**
 * @brief 执行3GPP本地软关机后进入深睡，下次上电后3GPP必须执行attach流程。通常用于产品容错
 */
void xy_fast_power_off();


/**
 * @brief  获取SoC的芯片类型
 * @return 0为XY1200L;1为XY1200SL;2为XY2100SL;3为XY1200;4为XY1200S;5为XY2100S;6:1200E;7:1200E+
 */
int get_Soc_ver();


/**
 * @brief  获取VBAT pin电压值，属于芯片内部电压，单位mV
 * @note   最大工作电压范围：2.0V~5.0V，典型值3.6V
 */
uint16_t xy_getVbat();

/**
 * @brief  获取芯片温度，单位摄氏度
 * @note 
 */
int16_t xy_getempera();

/**
 * @brief 获取RTC的count的ms数，不是严格的ms精度.仅限底层使用，不能用xy_gmtime_r进行转换
 * eg：
 * t1 = get_utc_tick();
 * t2 = get_utc_tick();
 * offset = CONVERT_RTCTICK_TO_MS((t2 - t1));
 * @note  软复位或深睡仍保持有效，硬复位无效。
 */
uint64_t get_utc_tick();

/**
 * @brief   仅供云业务开发使用，获取UTC中的毫秒数，只能用于差值计算，不能直接转化为世界时间
 * @warning 时钟源为XTAL32K或RC32K，精度与之保持一致。
 * @note    软复位或深睡仍保持有效，硬复位无效。
 */
uint64_t get_utc_ms();


/**
 * @brief 获取当前世界时间的相对毫秒数
 * @note  使用该接口时，若PDP激活成功，已成功获取到世界时间，则返回相对毫秒数，否则返回0
 * @warning  该接口性能低，仅供业务开发使用，底层对性能敏感的不得使用！
 */
uint64_t xy_get_UT_ms();

/**
 * @brief 操作系统调度前获取互斥量的接口
 * @param mutex_id,
 * @attention 当某函数在main函数初始化阶段会被调用，那么该函数内部获取互斥量时应使用xy_mutex_acquire接口，否则会死机
 */
void xy_mutex_acquire(osMutexId_t mutex_id, uint32_t timeout);

/**
 * @brief 操作系统调度前释放互斥量的接口
 * @param mutex_id,
 * @attention 当某函数在main函数初始化阶段会被调用，那么该函数内部释放互斥量时应使用xy_mutex_release接口，否则会死机
 */
void xy_mutex_release(osMutexId_t mutex_id);

/**
 * @brief  获取RF校准NV主备份两块区域的有效性状态。如果需要软件进行两块NV区域的搬移备份，请参考at_TEST_req中的"NV"分支代码。
 * @return 0为两块都有效;1为存储区有效，备份区无效;2为存储区无效，备份区有效;3为存储区备份区均无效
 */
extern int get_rf_nv_validity();

typedef void (*app_init_f)();
typedef struct
{
    app_init_f app_init_entry;
} appRegItem_t;

#define _appRegTable_attr_ __attribute__((unused, section(".appRegTable")))

/**
 * @brief 特殊业务模块初始化函数，在main入口处调用，此时尚未开调度。通常用于借AP核堆内存的特殊使用，深睡保持供电有效
 */
#define application_init(app_init_entry) const appRegItem_t _regAppItem_##app_init_entry _appRegTable_attr_ = {app_init_entry}



