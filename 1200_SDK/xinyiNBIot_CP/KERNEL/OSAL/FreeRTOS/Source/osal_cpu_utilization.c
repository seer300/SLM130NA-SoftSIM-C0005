#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_device.h"
#include "hw_types.h"
#include "hw_timer.h"
#include "prcm.h"
#include "hw_memmap.h"

#if (configGENERATE_RUN_TIME_STATS == 1)

#define osUtilTimerBase           TIMER3_BASE
#define OS_UTIL_TIMER_RELOAD     configCPU_CLOCK_HZ / configTICK_RATE_HZ / 20
static uint32_t timer_interrupt_cnt = 0;

void TIM3_Handler(void)
{
    timer_interrupt_cnt++;
}

void osUtilTimerEnable(unsigned long ulBase)
{
    HWREG(ulBase + TIMER_CTL) |= TIMER_CTL_TEN_EN;
}
void osUtilTimerDisable(unsigned long ulBase)
{
    HWREG(ulBase + TIMER_CTL) &= ~(TIMER_CTL_TEN_EN);
}
void osUtilTimerInitValueSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_CNT) = ulValue;
}
void osUtilTimerReloadValueSet(unsigned long ulBase, unsigned long ulValue)
{
    HWREG(ulBase + TIMER_RLD) = ulValue;
}
void osUtilTimerConfigure(unsigned long ulBase)
{
    HWREG(ulBase + TIMER_CTL) = (HWREG(ulBase + TIMER_CTL)&
            ~(TIMER_CTL_TMODE_Msk|TIMER_CTL_PRES_Msk|TIMER_CTL_RSTCNT_Msk))|(TIMER_CTL_TMODE_CONTINUOUS|TIMER_CTL_PRES_DIVIDE_1);
}
void osTimer_init(void)
{
    PRCM_ClockEnable(CORE_CKG_CTL_TMR3_EN);
    osUtilTimerDisable(osUtilTimerBase);
    osUtilTimerReloadValueSet(osUtilTimerBase, OS_UTIL_TIMER_RELOAD);
    osUtilTimerInitValueSet(osUtilTimerBase, 0);
    osUtilTimerConfigure(osUtilTimerBase);
    NVIC_IntRegister(TIM3_IRQn, TIM3_Handler, 0);


}



void osThreadCPUUtilizationStart(void)
{
    osUtilTimerEnable(osUtilTimerBase);

    uxTaskResetRunTimeCounter();
    timer_interrupt_cnt = 0;
}


void osThreadCPUUtilizationPrint(void)
{
    char *pcWriteBuffer = pvPortMalloc(1024);
    char *pxCheck = NULL;
    char *tempbuffer;
    char *pcwriteOutBuffer = pvPortMalloc(1024);
    size_t x;
    size_t i;
    if (pcWriteBuffer != NULL)
    {

        vTaskGetRunTimeStats(pcWriteBuffer);
        tempbuffer = pcWriteBuffer;
        while(strstr(tempbuffer,"\r\n") != NULL)
        {
        pxCheck = strstr(tempbuffer,"\r\n");
        x = strlen(tempbuffer)- strlen(pxCheck);
        strncpy(pcwriteOutBuffer, tempbuffer, x);
        pcwriteOutBuffer[ x ] = ( char ) 0x00;
        for(i = 0; i<strlen("\r\n"); i++)
        {
        pxCheck[i]  =  ' ';

        }
        tempbuffer = pxCheck;
        //*(pcWriteBuffer +10) = 0;
        user_printf("%s\r\n", pcwriteOutBuffer);
        pxCheck = NULL;
        }

        user_printf("%s\r\n", tempbuffer);
        vPortFree(pcWriteBuffer);
        pcWriteBuffer = NULL;
        vPortFree(pcwriteOutBuffer);
        pcwriteOutBuffer = NULL;
    }
}


void osThreadCPUUtilizationStop(void)
{
    osUtilTimerDisable(osUtilTimerBase);
    osUtilTimerInitValueSet(osUtilTimerBase, 0);
}


uint32_t osThreadTimeBaseGetTime(void)
{
    return timer_interrupt_cnt;
}

#endif  /* configGENERATE_RUN_TIME_STATS == 1 */
