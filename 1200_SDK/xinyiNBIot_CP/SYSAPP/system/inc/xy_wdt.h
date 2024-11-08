#pragma once


#include "factory_nv.h"

extern volatile unsigned int g_freq_32k;
extern softap_fac_nv_t *g_softap_fac_nv;

#define WDT_COUNT_CLK  g_freq_32k
#define WDT_TIME_MS    120

typedef enum {
  wdtmode_interrupt = 0,     /*!< 看门狗中断模式，超时触发看门狗中断   */
  wdtmode_reset,              /*!< 看门狗重启模式，超时触发看门狗重启    */
} wdtmode_type;



/*CP单核的看门狗，用户无需关注，平台在调度期间进行喂狗*/
uint32_t wdt_init();
uint32_t wdt_refresh();






/*由于UTC看门狗刷新耗时过久，进而只能在idle或出睡眠处刷新，以免影响调度*/
#define UTC_WDT_TRIGGER_SECOND    (5*60)

/*进入WFI/standby/DEEPSLEEP各种睡眠等级时，将时长设置为超长，以避免睡眠期间误触发*/
#define UTC_WDT_SLEEP_LONG    (30*24*60*60) 


/*用户禁止使用。芯片全局看门狗，仅限CP核造成的整个芯片异常容错用，通常为启停CP核时的芯片异常*/
void cp_utcwdt_refresh(uint32_t timeout_sec);
void cp_utcwdt_deinit(void);
void cp_utcwdt_init(uint32_t timeout_sec);
extern uint32_t g_wdt_refresh;
/*关闭CP WDT和UTC WDT*/
void Disable_All_WDT(void);