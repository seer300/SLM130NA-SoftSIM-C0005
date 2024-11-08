/*****************************************************************************************************************************	 
 * user_config.h
 ****************************************************************************************************************************/

#ifndef USER_CONFIG_H__
#define USER_CONFIG_H__

#define BOOT_CP_PERIOD_ON               (1)      // 1：打开周期性启动CP，0：关闭
#define SWITCHING_VALVE_PERIOD_ON       (1)      // 1：打开模拟周期性关开阀动作，0：关闭
#define CHECK_BAT_WHEN_BOOT_CP          (1)      // 1: 启动cp时检测碱电， 0：启动cp时不检测碱电
#define RANDOM_ASSERTCP_ON              (0)      // 1:启动随机ASSERTCP，0:关闭随机ASSERTCP, 默认关闭

// 控制周期性时间
#define GAS_TIMER_PERIOD                (1000)   // 控制气表LPTIMER唤醒周期         单位：ms
#define BOOT_CP_PERIOD                  (300)    // 控制启动CP进行云通信的周期       单位：秒
#define IIC_E2ROM_TEST_PERIOD           (10)     // 控制EEPROM读写测试的周期        单位：秒
#define FLASH_WRITE_PERIOD              (60)     // 控制flash读写测试的周期         单位：秒
#define BAK_POWER_DETECT_PERIOD         (60)     // 控制备电采集的周期              单位：秒
#define SWITCHING_VALVE_TEST_PERIOD     (10)     // 控制关开阀门的周期              单位：秒
#define RC_CALI_PERIOD                  (600)    // 控制RC校准的周期                单位：秒
#define PRINTF_LPTIMER_PERIOD           (10)     // 单位：秒

#define DEBUG_PRINTF_EN                          // 定义此宏打开jk_printf
#define WDT_TIMEOUT                     (4)      // AP看门狗中断模式超时时间，单位秒

#define XY_TIMER_OFSEC  30      // XY_TIMER_OFSEC秒后clktick_cnt寄存器计满并归零继续计数
#define RTC_NEWYEAR     2002    // 距离新的一年的倒计时，只能取2001或2002
#define RTC_COUNTDOWN   10      // 距离新的一年的倒计时分钟数

#endif
