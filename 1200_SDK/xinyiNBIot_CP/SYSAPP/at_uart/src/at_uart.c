#include "at_uart.h"
#include "cmsis_device.h"
#include "cmsis_os2.h"
#include "prcm.h"
#include "gpio.h"
#include "oss_nv.h"
#include "at_ctl.h"
#include "sema.h"
#include "uart.h"
#include "at_com.h"
#include "softap_nv.h"

osMutexId_t g_at_uart_mux = NULL;
volatile int uart_write_flag = 0;

void set_write_to_uart_flag(void)
{
    uart_write_flag = 1;
}

void clear_write_to_uart_flag(void)
{
    uart_write_flag = 0;
}

/*由于osdelay不参与深睡唤醒，通过该标志位来判断CP是否有输出需求，输出完成前不允许进入深睡*/
int at_uart_is_write_complete(void)
{
    int ret;
    ret = (uart_write_flag == 0) ? 1 : 0;
    return ret;
}

/*直接写LPUART.对于长URC，为了避免AP核缓存内存不足，直接在CP核输出到uart口*/
int write_to_at_uart(char *buf, int size)
{
    if (HWREGB(BAK_MEM_LPUART_USE) == 1)
    {
        /* 模组形态 */
        // 获取内核状态，判断是否处于关键性流程
        osCoreState_t coreState = osCoreGetState();
		uint32_t start = osKernelGetTickCount();
		uint32_t tick;

        /* 非临界区且OS已开始调度需要互斥保护 */
        if ((coreState == osCoreNormal) && (g_at_uart_mux != NULL))
        {
            xy_mutex_acquire(g_at_uart_mux, osWaitForever);
        }
		
         do
        {
            SEMA_RequestNonBlocking(SEMA_ATWR_AUX, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_AP);

            if(SEMA_MASTER_CP != SEMA_MasterGet(SEMA_ATWR_AUX))
            {
            	tick = osKernelGetTickCount();
            	if(tick > start && (tick-start) > (60 * 1000)) /// 等待超过1分钟，直接断言
            	{
            		xy_assert(0);
            	}
            	//ap核正在写lpuart,此时cp核执行flash擦写操作，进而挂起ap核，若后续cp核又有写lpuart的操作，会造成卡死；进而此处释放调度，以继续执行flash擦写操作
            	if(coreState == osCoreNormal && osKernelGetState() >= osKernelRunning && osKernelIsRunningIdle() != osOK)
            	{
                    set_write_to_uart_flag();
            		osDelay(100); 
            	}
            }
            else
            	break;
        } while (1); //阻塞等待获取AP、CP核间的互斥锁


        /**
         * @brief CP侧直接写LPUART，LPUART初始化在AP侧完成，务必保证调用该接口前LPUART已完成初始化
         */
        while (size)
        {
            // 若TXFIFO没满则发送数据
            // if (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_FULL) == 0)
            if (LPUART_IS_TXFIFO_FULL() == 0)
            {
                UARTCharPutNonBlocking(UART1_BASE, *buf);
                size--;
                buf++;
            }
        }
        at_uart_wait_send_done();

        clear_write_to_uart_flag();

        SEMA_Release(SEMA_ATWR_AUX, SEMA_MASK_NULL);

        if ((coreState == osCoreNormal) && (g_at_uart_mux != NULL))
        {
            xy_mutex_release(g_at_uart_mux);
        }
    }
    return 1;
}

/*识别结果码，内部重置通道上下文*/
bool at_send_to_uart(void* at_ctx, void *buf, int size)
{
    if (HWREGB(BAK_MEM_LPUART_USE) == 1)
    {
         /* 模组形态 */
        at_context_t *ctx = search_at_context(((at_context_t *)(at_ctx))->fd);

        if (Is_Result_AT_str((char *)buf))
        {
            ctx->error_no = Get_AT_errno((char *)buf);
            reset_ctx(ctx);
            /* 清除AP接收AT核间标志位 */
            set_at_lpuart_state(0);
        }

        xy_printf(0, PLATFORM, WARN_LOG, "write_to_at_uart:%s", buf);
        write_to_at_uart((char *)buf, size);

        /* 当uart挂在ap时，URC缓存上报 */
#if URC_CACHE
        at_report_urc_cache(at_ctx);
#endif /* URC_CACHE */
    }
    return 1;
}

/*仅供睡眠接口内部URC上报使用*/
int write_to_uart_for_Dslp(char *buf, int size)  
{
    if (HWREGB(BAK_MEM_LPUART_USE) == 1)
    {
        do
        {
            SEMA_RequestNonBlocking(SEMA_ATWR_AUX, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_AP);
        } while (SEMA_MASTER_CP != SEMA_MasterGet(SEMA_ATWR_AUX)); //阻塞等待获取AP、CP核间的互斥锁

        while (size)
        {
            // 若TXFIFO没满则发送数据
            // if (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_FULL) == 0)
            if (LPUART_IS_TXFIFO_FULL() == 0)
            {
                UARTCharPutNonBlocking(UART1_BASE, *buf);
                size--;
                buf++;
            }
        }

        // 等待FIFO_EMPTY的标志位置上，并延时一段时间，等待移位寄存器中的数据也发送完成
        // while (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY) == 0)
        while (LPUART_IS_TXFIFO_EMPTY() == 0)
        {
            if(HWREGB(BAK_MEM_AP_STOP_CP_REQ) != 0)
                break;
        }
        for (volatile uint32_t i = 0; i < 10000; i++);

        SEMA_Release(SEMA_ATWR_AUX, SEMA_MASK_NULL);
    }
    return 1;
}

void at_uart_wait_send_done(void)
{
    if (HWREGB(BAK_MEM_LPUART_USE) == 1)
    {
        // 等待FIFO_EMPTY的标志位置上，并延时一段时间，等待移位寄存器中的数据也发送完成
        // while (UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY) == 0)
        while (LPUART_IS_TXFIFO_EMPTY() == 0)
            ;
        for (volatile uint32_t i = 0; i < 10000; i++)
            ;
    }
}

/*配合BC95版本natspeed指令切换使用*/
#if VER_BC95
static void at_reconfigparity_frmodsleep()
{
    //AT口初始化时，会根据fac_nv配置校验位，动态切换校验位后，会将新校验位存到var_nv中，深睡唤醒后在恢复var_nv后重配AT口校验位
    if(Is_WakeUp_From_Dsleep())
    {
        if(g_softap_var_nv->at_parity != g_softap_fac_nv->at_parity)
            HWREG(UART1_BASE + UART_CTL) = (HWREG(UART1_BASE + UART_CTL) &~ (UART_CTL_PARITY_Msk)) | (g_softap_var_nv->at_parity << UART_CTL_PARITY_Pos);            	
    }
}
#endif

void at_uart_init()
{
#if VER_BC95
    at_reconfigparity_frmodsleep();
#endif
}