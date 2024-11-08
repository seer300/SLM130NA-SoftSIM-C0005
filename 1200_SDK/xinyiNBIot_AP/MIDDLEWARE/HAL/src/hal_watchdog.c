#include "ap_watchdog.h"
#include "hal_watchdog.h"

/**
 * @brief AP看门狗初始化函数
 * @param HAL_WDTMode AP看门狗工作模式。详情参考 @ref HAL_WDTMode_TypeDef.
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如客户使用硬看门狗，main入口和p_User_Init_FastRecovery函数指针所指向函数中都必须调用该API
 * @warning 产品调试阶段若发生不合理的硬看门狗重启，可以设置为HAL_WDT_MODE_Interrupt模式，在中断函数中排查触发的根源。
 *          该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
void HAL_WDT_Init(HAL_WDTMode_TypeDef HAL_WDTMode, int32_t sec)
{
    AP_WDT_Init((AP_WDT_WorkModeTypeDef)HAL_WDTMode, (uint32_t)sec);
}

/**
 * @brief 用于秒级的周期性喂狗，通常在main主函数的while循环里执行喂狗操作。耗时几us
 * @param sec 触发AP看门狗工作的超时时间，单位秒
 * @note 如果用户按照工作容许的最大时长来设置，则无需调用该接口进行喂狗，一旦超时则表明未能正常进入芯片深睡，触发看门狗异常。
 * @warning 该看门狗仅能用于工作态，SoC深睡下无效。如果需要在深睡下有效，请使用UTC看门狗，参见utc_watchdog.h。
 *          考虑到FOTA期间频繁擦写FLASH，单次耗时百毫秒，进而看门狗时长建议设为秒级。
 */
__RAM_FUNC void HAL_WDT_Refresh(int32_t sec)
{
	AP_WDT_Refresh((uint32_t)sec);
}

/**
 * @brief 使能AP看门狗
 */
void HAL_WDT_Enable(void)
{
	AP_WDT_Enable();
}

/**
 * @brief 禁能AP看门狗
 */
void HAL_WDT_Disable(void)
{
	AP_WDT_Disable();
}

/**
 * @brief AP看门狗中断模式(HAL_WDT_MODE_Interrupt)回调函数注册，用户可根据需求重新定义，例如进行产品级容错等。
 * @warning 硬看门狗异常，不保证系统一定能正常处理中断回调！进而建议客户使用UTC全局看门狗来二次监控
 */
void HAL_WDT_Int_Reg(pFunType_void p_fun)
{
	AP_WDT_Int_Reg(p_fun);
}
