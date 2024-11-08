/**
 * @brief  该工程默认为上电就加载CP核的传统数据模式，用户可以参考其他DEMO，实现OPENCPU产品相关代码
 * @warning OPENCPU开发时，注意Boot_CP调用点、低功耗相关外设的使用细节、User_Init_FastRecovery的实现、看门狗的使用、容错策略等，细节参考相关DEMO工程
 * @note  用户编译default工程时，请根据自身产品形态，设置好define.cmake中XY_SOC_VER和VER_BC95等重要宏值
 */
#include "hal_gpio.h"
#include "xy_printf.h"
#include "xy_cp.h"
#include "xy_lpm.h"
#include "at_uart.h"
#include "auto_baudrate.h"
#include "urc_process.h"
#include "xy_system.h"
#include "xy_svd.h"
#include "ap_watchdog.h"
#include "basic_config.h"
#include "sys_rc32k.h"
#include "mcu_adapt.h"

