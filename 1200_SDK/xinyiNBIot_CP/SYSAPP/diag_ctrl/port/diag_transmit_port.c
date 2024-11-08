#include "diag_options.h"
#include "diag_transmit_port.h"
#include "diag_list.h"
#include "diag_mem.h"
#include "diag_filter.h"
#include "diag_recv_msg.h"

#include "xinyi_hardware.h"
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "csp.h"
#include "dma.h"
#include "prcm.h"
#include "gpio.h"
#include "sys_clk.h"
#include "sys_config.h"


// 选择LOG通信使用的串口是UART还是CSP
// 1: UART  0: CSP
#define DIAG_PORT_USE_UART          0


#if (DIAG_PORT_USE_UART == 1)

#define DIAG_PORT_MODULE_CLOCK      PRCM_CKG_CTL_UART2_EN
#define DIAG_PORT_UART_BASE         UART2_BASE
#define DIAG_PORT_UART_INT_IRQ      UART2_IRQn
#define DIAG_PORT_UART_CLOCK        (GetlsioFreq())
#define DIAG_PORT_REMAP_TXD         GPIO_UART2_TXD
#define DIAG_PORT_REMAP_RXD         GPIO_UART2_RXD

#define DIAG_PORT_DMA_REQNUM        DMA_REQNUM_UART2_WR
#define DIAG_PORT_DMA_DEST_ADDR     (DIAG_PORT_UART_BASE + UART_FIFO_WRITE)
#define DIAG_PORT_DMA_CTRLTYPE      DMAC_CTRL_TYPE_MEM_TO_MEM

#else   /* DIAG_PORT_USE_UART == 0 */

#define DIAG_PORT_MODULE_CLOCK      CORE_CKG_CTL_CSP3_EN
#define DIAG_PORT_UART_BASE         CSP3_BASE
#define DIAG_PORT_UART_INT_IRQ      CSP3_IRQn
#define DIAG_PORT_UART_CLOCK        (BBPLL_CLK_DEFAULT / Get_Sys_Div() / Get_Peri2_Div())   // BBPLL_CLK_DEFAULT/5
#define DIAG_PORT_REMAP_TXD         GPIO_CSP3_TXD
#define DIAG_PORT_REMAP_RXD         GPIO_CSP3_RXD

#define DIAG_PORT_DMA_REQNUM        DMA_REQNUM_CSP3_TX
#define DIAG_PORT_DMA_DEST_ADDR     (DIAG_PORT_UART_BASE + CSP_TX_FIFO_DATA)
#define DIAG_PORT_DMA_CTRLTYPE      DMAC_CTRL_TYPE_MEM_TO_IO

#endif  /* DIAG_PORT_USE_UART == 1 */

#define DIAG_PORT_UART_BAUDRATE     921600
#define DIAG_PORT_UART_TX_PIN       (uint8_t)(g_softap_fac_nv->log_txd_pin)
#define DIAG_PORT_UART_RX_PIN       (uint8_t)(g_softap_fac_nv->log_rxd_pin)
#define DIAG_PORT_DMA_CHANNEL       DMA_CHANNEL_1
#define DIAG_PORT_DMA_INT_IRQ       DMAC1_IRQn
#define DIAG_PORT_DMA_MEM_TYPE      MEMORY_TYPE_CP


// 根据是否使用DMA，需要给log发送线程设置不同的优先级
#if (DIAG_TRANSMIT_WITH_DMA == 1)
#define DIAG_PORT_SEND_THREAD_PRIORITY    osPriorityAboveNormal1
#else
#define DIAG_PORT_SEND_THREAD_PRIORITY    osPriorityIdle
#endif


#if (DIAG_TRANSMIT_WITH_DMA == 1)
#define DIAG_PORT_DMA_TRANS_COMPLETE      0
#define DIAG_PORT_DMA_TRANS_UNCOMPLETE    1
static  int diag_dma_trans_complete = DIAG_PORT_DMA_TRANS_COMPLETE;
#endif

static TaskHandle_t diag_send_thread_handle = NULL;
static TaskHandle_t diag_recv_thread_handle = NULL;

/*
 * DMA以及UART的硬件初始化
 */
void diag_port_hardware_init(void);

/*
 * 通过DMA写入数据到UART中的控制线程，负责设置DMA和释放已经输出的log的内存
 */
static void diag_port_log_send_thread(void *args);

/*
 * 发送有效链表的头节点数据，该函数内存会有判断，如果不满足条件，调用该函数需要保证已经进
 * 入临界区，该函数内部会访问全局变量，该全局变量同时会被多个线程或中断中被访问
 */
static void diag_port_send_head_valid_list(void);

/*
 * 释放待删除链表内所有的内存，清空待释放链表。调用该函数需要保证已经进入临界区，该函数内
 * 部会访问全局变量，该全局变量同时会被多个线程或中断中被访问
 */
static void diag_port_free_all_free_list(void);

#if (DIAG_TRANSMIT_WITH_DMA == 1)
/* 
 * DMA的中断处理函数
 */
static void diag_port_dma_isr(void);
#endif

/*
 * log的接收中断，通知log接收线程接收数据并处理
 */
static void diag_port_uart_isr(void);

/*
 * log接收线程，接收log数据并处理
 */
static void diag_port_uart_rcv_thread(void *args);
/*----------------------------------------------------------------------------------------------------*/

/*模组客户要求log必须随时可以响应*/
__FLASH_FUNC void diag_port_send_init(void)
{
	/*不开LOG或者仅AP核明文LOG，CP核无需开启LOG相关资源*/
	if(g_softap_fac_nv->open_log==0 || g_softap_fac_nv->open_log==7 || HWREGB(BAK_MEM_AP_LOG)==7 || HWREGB(BAK_MEM_AP_LOG)==0)
		return;
	
    if((g_softap_fac_nv->log_txd_pin != 0xFF) && (g_softap_fac_nv->log_rxd_pin != 0xFF))  //硬件初始化与nv:open_log无关，只要pin配置正确，必执行log硬件的初始化
    {
        diag_port_hardware_init();
    }

    xTaskCreate((TaskFunction_t)diag_port_uart_rcv_thread, "diag_recv", osStackShared / sizeof(StackType_t), NULL, 1, &diag_recv_thread_handle);

    if (1)  //(diag_port_get_send_enable_state() == DIAG_SEND_ENABLE)
    {
        xTaskCreate((TaskFunction_t)diag_port_log_send_thread, "diag_send", 0x200 / sizeof(StackType_t), NULL, DIAG_PORT_SEND_THREAD_PRIORITY, &diag_send_thread_handle);

        diag_filter_init();
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_port_send_log(void *log_str, uint32_t length)
{
    diag_list_t * log_head;

    DIAG_CRITICAL_DEF(isr);

    DIAG_ASSERT(log_str != NULL);
    DIAG_ASSERT(length != 0);

    log_head = (diag_list_t *) ((char *)log_str - sizeof(diag_list_t));

    // 使用DMA的list模式才会有的结构体成员
    #if ((DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1))
    {
        // 设置结构体成员中，DMA相关的配置
        // 源地址需要转换成DMA可以访问的地址，由于DMA的list模式会自动从内存中读取并设置进DMA，不会做地址转换，需要提前做好地址转换
        log_head->src_addr = (void *) CORE_ADDR_TO_DMA_ADDR(DIAG_LIST_GET_SEND_DATA(log_head));
        log_head->dst_addr = (void *) (DIAG_PORT_UART_BASE + UART_FIFO_WRITE);
        log_head->dma_ctrl = DMAC_CTRL_SINC_SET | DMAC_CTRL_DDEC_DIS | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | \
                            DMAC_CTRL_BURST_SIZE_2W | DMAC_CTRL_STRT_Msk | DMAC_CTRL_CHG_Msk;
    }
    #endif
    
    // 进入临界区，保证下列操作的连续性，防止多线程的抢占
    DIAG_ENTER_CRITICAL(isr);

    // 设置要发送的字节数，用于后续从链表中取出数据时，可以知道要发送的长度
    DIAG_LIST_SET_SEND_SIZE(log_head, length);
    diag_list_insert_valid(log_head);

    DIAG_EXIT_CRITICAL(isr);

    // 并不是一定会发送有效链表的头节点，内部有判断，满足条件才会发送
    diag_port_send_head_valid_list();
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_port_send_log_directly(void *log_str, uint32_t length)
{
    uint32_t i;
    uint8_t *data = (uint8_t *) log_str;

    #if (DIAG_PORT_USE_UART == 1)
    {
        for(i = 0; i < length; i++)
        {
            UARTCharPut(DIAG_PORT_UART_BASE, data[i]);
        }
    }
    #else   /* DIAG_PORT_USE_UART == 0 */
    {
        for(i = 0; i < length; i++)
        {
            CSPCharPut(DIAG_PORT_UART_BASE, data[i]);
        }
    }
    #endif  /* DIAG_PORT_USE_UART == 1 */
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_port_clear_unsent_log(void)
{
    DIAG_CRITICAL_DEF(isr);

    // 下面会判断全局变量，需要进入临界区
    DIAG_ENTER_CRITICAL(isr);

    diag_list_move_valid_to_free();

    DIAG_EXIT_CRITICAL(isr);
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC int diag_port_is_send_complete(void)
{
    int ret;

    #if (DIAG_TRANSMIT_WITH_DMA == 1)
    {
        DIAG_CRITICAL_DEF(isr);

        DIAG_ENTER_CRITICAL(isr);
        // 如果使用DMA，根据该标志位判断是否发送完成
        ret = (diag_dma_trans_complete == DIAG_PORT_DMA_TRANS_COMPLETE) ? 1 : 0;
        DIAG_EXIT_CRITICAL(isr);
    }
    #else
    {
        // 默认认为该函数只会被idel线程调用，执行到idle线程时，log已经发送完毕
        ret = 1;
    }
    #endif

    return ret;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_port_dump_environment_init(void)
{
    #if (DIAG_TRANSMIT_WITH_DMA == 1)
    {
        // 等待正常流程的log数据发送完成
        if(DMAChannelTransferRemainCNT(DIAG_PORT_DMA_CHANNEL) != 0)
        {
            DMAChannelWaitIdle(DIAG_PORT_DMA_CHANNEL);
        }

        // 注意LOG口的TX控制模式在初始化时被配置成了DMA控制模式，
        // 如果需要使用软件直接写TX_DATA寄存器完成数据发送，
        // 这里建议先切换TX控制模式为IO控制模式，完成数据传输后再切回DMA控制模式。
        // 切换成IO控制模式
        #if (DIAG_PORT_USE_UART == 1)
        {
            UARTDmaTransferDisable(DIAG_PORT_UART_BASE);
        }
        #else   /* DIAG_PORT_USE_UART == 0 */
        {
            HWREG(DIAG_PORT_UART_BASE + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
            HWREG(DIAG_PORT_UART_BASE + CSP_TX_DMA_IO_LEN) = 0;
            HWREG(DIAG_PORT_UART_BASE + CSP_TX_FIFO_CTRL) = CSP_TX_FIFO_CTRL_WIDTH_BYTE;
        }
        #endif  /* DIAG_PORT_USE_UART == 1 */
    }
    #endif  /* DIAG_TRANSMIT_WITH_DMA == 1 */
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC void diag_port_wait_send_done(void)
{
    #if (DIAG_PORT_USE_UART == 1)
    {
        // 等待发送完成
        UARTWaitTxDone(DIAG_PORT_UART_BASE);
        for (volatile int i = 0; i < 100; i++);
    }
    #else   /* DIAG_PORT_USE_UART == 0 */
    {
        while (!(CSPTxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_EMPTY)));
        for (volatile int i = 0; i < 100; i++);
    }
    #endif  /* DIAG_PORT_USE_UART == 1 */
    
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC uint32_t diag_port_recv_after_dump(uint8_t *rcv_buf, uint32_t max_len)
{
    uint32_t  fifo_level;
    uint32_t  i;

    #if (DIAG_PORT_USE_UART == 1)
    {
        // 等待接收到数据
        // while (UARTRxFifoStatusGet(DIAG_PORT_UART_BASE, UART_FIFO_EMPTY));

        // FIFO 满的时候，读出得数值是0，需要处理这种情况
        fifo_level = UARTRxFifoStatusGet(DIAG_PORT_UART_BASE, UART_FIFO_DATA_LEN);
        fifo_level |= UARTRxFifoStatusGet(DIAG_PORT_UART_BASE, UART_FIFO_FULL) << 6;
        // 每次最多只能读取传入buffer的最大长度，防止内存越界
        fifo_level = (fifo_level > max_len) ? max_len : fifo_level;

        // 读取指定长度的数据
        for (i = 0; i < fifo_level; i++)
        {
            rcv_buf[i] = UARTCharGet(DIAG_PORT_UART_BASE);
        }
    }
    #else   /* DIAG_PORT_USE_UART == 0 */
    {
        // 等待接收到数据
        // while (CSPRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_EMPTY));

        // FIFO 满的时候，读出得数值是0，需要处理这种情况
        fifo_level = CSPRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_LEVEL);
        fifo_level |= CSPRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_FULL) << 7;
        // 每次最多只能读取传入buffer的最大长度，防止内存越界
        fifo_level = (fifo_level > max_len) ? max_len : fifo_level;

        // 读取指定长度的数据
        for (i = 0; i < fifo_level; i++)
        {
            rcv_buf[i] = CSPCharGet(DIAG_PORT_UART_BASE);
        }
    }
    #endif  /* DIAG_PORT_USE_UART == 1 */

    return fifo_level;
}
/*----------------------------------------------------------------------------------------------------*/


__FLASH_FUNC void diag_port_hardware_init(void)
{
	GPIO_InitTypeDef gpio_init = {0};
    uint32_t bound_rate = 921600;

    uint32_t log_rate_bund[] = {921600,9600,19200,38400,57600,115200,380400,460800};

    /* enable uart2/csp3/gpio/dmac clock */
    PRCM_ClockEnable(DIAG_PORT_MODULE_CLOCK | CORE_CKG_CTL_GPIO_EN | CORE_CKG_CTL_DMAC_EN);

    /* clear conflict */
	GPIO_AllocateRemove(DIAG_PORT_REMAP_TXD);
	GPIO_AllocateRemove(DIAG_PORT_REMAP_RXD);

	gpio_init.Pin = DIAG_PORT_UART_TX_PIN;
	gpio_init.PinRemap = DIAG_PORT_REMAP_TXD;
	gpio_init.Mode = GPIO_MODE_HW_PER;
	GPIO_Init(&gpio_init);

	gpio_init.Pin = DIAG_PORT_UART_RX_PIN;
	gpio_init.PinRemap = DIAG_PORT_REMAP_RXD;

    #if (DIAG_PORT_USE_UART == 1)
	gpio_init.Mode = GPIO_MODE_HW_PER;
    #else
    gpio_init.Mode = GPIO_MODE_SW_PER_INPUT;
    gpio_init.Pull = GPIO_PULL_UP;
    #endif
	GPIO_Init(&gpio_init);
	GPIO_InputPeriSelect(DIAG_PORT_UART_RX_PIN, DIAG_PORT_REMAP_RXD);
	GPIO_InputPeriSelectCmd(DIAG_PORT_REMAP_RXD, ENABLE);


    if(g_softap_fac_nv->log_rate < sizeof(log_rate_bund)/sizeof(uint32_t))
    {
        bound_rate = log_rate_bund[g_softap_fac_nv->log_rate];
    }

    #if (DIAG_PORT_USE_UART == 1)
    {
        UARTConfigSetExpClk(DIAG_PORT_UART_BASE, DIAG_PORT_UART_CLOCK, bound_rate, UART_CTL_CHAR_LEN_8 | UART_CTL_PARITY_NONE);
        UARTIntDisable(DIAG_PORT_UART_BASE, UART_INT_ALL);
        UART_RXFIFO_LevelSet(DIAG_PORT_UART_BASE, UART_FIFO_LEVEL_RX2_4);
        UART_TXFIFO_LevelSet(DIAG_PORT_UART_BASE, UART_FIFO_LEVEL_TX1_4);
        UARTTimeOutConfig(DIAG_PORT_UART_BASE, UART_RX_TIMEOUT_START_FIFO_NEMPTY, 10);
        UARTTimeOutEnable(DIAG_PORT_UART_BASE);

        // 使用LOG口的中断接收功能，则需要注册相应串口中断并使能有关接收中断源
        NVIC_IntRegister(DIAG_PORT_UART_INT_IRQ, diag_port_uart_isr, 2);
        UARTIntEnable(DIAG_PORT_UART_BASE, UART_INT_RXFIFO_TRIGGER | UART_INT_TIMEOUT);

        // 使能硬件的DMA传输
        #if (DIAG_TRANSMIT_WITH_DMA == 1)
        {
            //使能硬件与nv:open_log无关，只要pin配置正确，必执行log硬件的初始化
            if(1)     //(diag_port_get_send_enable_state() == DIAG_SEND_ENABLE)
                UARTDmaTransferEnable(DIAG_PORT_UART_BASE);
        }
        #endif
    }
    #else   /* DIAG_PORT_USE_UART == 0 */
    {
        /* csp3 config */
        CSPUARTModeSet(DIAG_PORT_UART_BASE, DIAG_PORT_UART_CLOCK, bound_rate, 8, CSP_UART_PARITYCHECK_None, 1);
        CSPIntDisable(DIAG_PORT_UART_BASE, CSP_INT_ALL);
        CSPSetRxFifoThreshold(DIAG_PORT_UART_BASE, 32 - 1);
        CSPUARTRxTimeoutConfig(DIAG_PORT_UART_BASE, 1, 255);

        // 使用LOG口的中断接收功能，则需要注册相应串口中断并使能有关接收中断源
        NVIC_IntRegister(DIAG_PORT_UART_INT_IRQ, diag_port_uart_isr, 2);
        CSPIntEnable(DIAG_PORT_UART_BASE, CSP_INT_RXFIFO_THD_REACH | CSP_INT_RX_TIMEOUT);

        // 使能硬件的DMA传输
        #if (DIAG_TRANSMIT_WITH_DMA == 1)
        {
            // 第二个参数是CSP发起DMA请求的level，单位是word
            // 第三个参数是CSP发送字节数，这里有硬件bug，必须设置到足够大，才能保证传输正常
            // 这里第三个参数设置成0xFFFFFFFF理论上也存在发完的时候，假设一条log 128字节数据，大概有 0xFFFFFFFF / 128 = 2^25 条，理论上不会超过
            if(1)     //(diag_port_get_send_enable_state() == DIAG_SEND_ENABLE)
                CSPDMAConfigTX(DIAG_PORT_UART_BASE, 64 / sizeof(uint32_t), 0xFFFFFFFF);
        }
        #endif
    }
    #endif  /* DIAG_PORT_USE_UART == 1 */

    // 使用DMA时，需要注册DMA的中断处理函数
    #if (DIAG_TRANSMIT_WITH_DMA == 1)
    {
        if(1)     //(diag_port_get_send_enable_state() == DIAG_SEND_ENABLE)
        {
            DMAIntClear(DIAG_PORT_DMA_CHANNEL);
            NVIC_IntRegister(DIAG_PORT_DMA_INT_IRQ, diag_port_dma_isr, 1);
            DMAChannelPeriphReq(DIAG_PORT_DMA_CHANNEL, DIAG_PORT_DMA_REQNUM);
            DMAPeriphReqEn(DIAG_PORT_DMA_CHANNEL);
            //设置dma：dma源地址增加，目标地址不变，设置TC中断，TC set，相应dma控制类型，burst size 32字节
            //注意:csp在设置dma时要满足burst*4 < fifo_depth(128)-SC*4,且TX_DMA_IO_LEN == transferCnt
            DMAChannelConfigure(DIAG_PORT_DMA_CHANNEL, DMAC_CTRL_SINC_SET | DMAC_CTRL_DINC_DIS | DMAC_CTRL_DDEC_DIS | \
                            DMAC_CTRL_INT_SET | DMAC_CTRL_TC_SET | DIAG_PORT_DMA_CTRLTYPE | DMAC_CTRL_BURST_SIZE_8W);
        }
    }
    #endif
}
/*----------------------------------------------------------------------------------------------------*/

#if (DIAG_TRANSMIT_WITH_DMA == 1)

__RAM_FUNC static void diag_port_dma_isr(void)
{
#if RUNTIME_DEBUG
	extern uint32_t xy_runtime_get_enter(void);
	uint32_t time_enter = xy_runtime_get_enter();
#endif

    DIAG_CRITICAL_DEF(isr);

    DMAIntClear(DIAG_PORT_DMA_CHANNEL);
    //用于确保DMA中断已清除
    while(DMAIntStatus(DIAG_PORT_DMA_CHANNEL));

    // 当前只有该中断会访问链表接口，其他会访问链表的都在线程中
    // 线程中的访问都加了临界区保护，中断中不会被线程打断，所以中断中不需要加临界区保护
    // 这里为了保险还是加上
    DIAG_ENTER_CRITICAL(isr);

    // 把发送列表的成员加入到待释放链表
    diag_list_insert_send_to_free();

    // 设置标记位，表示当前可以继续设置DMA
    diag_dma_trans_complete = DIAG_PORT_DMA_TRANS_COMPLETE;

    DIAG_EXIT_CRITICAL(isr);

    // 通知log线程设置下一个要发送的log
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(diag_send_thread_handle, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR (pxHigherPriorityTaskWoken);

#if RUNTIME_DEBUG
	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
	xy_runtime_get_exit(DMAC1_IRQn, time_enter);
#endif
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static void diag_port_log_send_thread(void *args)
{
    (void) args;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 并不是一定会发送有效链表的头节点，内部有判断，满足条件才会发送
        diag_port_send_head_valid_list();

        // 清空待释放链表，释放内存
        diag_port_free_all_free_list();
    }
}
/*----------------------------------------------------------------------------------------------------*/

__RAM_FUNC static void diag_port_send_head_valid_list(void)
{
    diag_list_t * log_head;
    void * send_str;
    uint32_t send_len;
    uint32_t time_interval;
    ItemHeader_t *pItemHeader;

    DIAG_CRITICAL_DEF(isr);

    // 下面会判断全局变量，需要进入临界区
    DIAG_ENTER_CRITICAL(isr);

    // 该全局的置位在中断中，为防止多线程抢占，调用该函数的位置需要处于临界区中
    if(diag_dma_trans_complete == DIAG_PORT_DMA_TRANS_COMPLETE)
    {
        // 获取待发送的链表，根据是否使用DMA的list模式，会得到一个链表成员或整个链表
        log_head = diag_list_get_send_list();

        // 能够获取到链表，则设置DMA进行发送
        if(log_head != NULL)
        {
            // 设置标志位，表示当前DMA已经在发送
            diag_dma_trans_complete = DIAG_PORT_DMA_TRANS_UNCOMPLETE;
            
            // 获取需要DMA发送的数据的起始地址和长度
            send_str = DIAG_LIST_GET_SEND_DATA(log_head);
            send_len = DIAG_LIST_GET_SEND_SIZE(log_head);

            pItemHeader = (ItemHeader_t *)send_str;
            time_interval = DIAG_GET_TICK_COUNT() - pItemHeader->u32Time;
            if (time_interval > g_diag_debug.send_time_interval) 
                g_diag_debug.send_time_interval = time_interval;

            g_log_status.data_send_tick = (uint32_t)DIAG_GET_TICK_COUNT();

#if (DIAG_PORT_USE_UART == 0) //logDMA卡住
            HWREG(DIAG_PORT_UART_BASE + CSP_TX_DMA_IO_LEN) = 0xFFFFFFFF;
#endif

            // 设置DMA发送单条数据，即使是DMA的list模式，也需要设置第一条开启传输
            DMAChannelTransferSet(DIAG_PORT_DMA_CHANNEL, send_str, (void *)DIAG_PORT_DMA_DEST_ADDR, send_len, DIAG_PORT_DMA_MEM_TYPE);

            // DMA的list模式，设置下一节点到到DMA
            #if (DIAG_TRANSMIT_IN_DMA_LIST_MODE == 1)
            {
                DMAChannelNextPointerSet(DIAG_PORT_DMA_CHANNEL, DIAG_LIST_GET_NEXT_ITEM(log_head));
            }
            #endif

            // 开启传输
            DMAChannelTransferStart(DIAG_PORT_DMA_CHANNEL);
        }
    }
    
    // 这里保证所有流程都在临界区中处理，实际上在获取到头节点后，就可以退出临界区
    // 这样修改的目的是为了尽快把log设置进DMA，否则退出临界区后，如果切换到其他线程，本来可以打印的log被推迟了
    DIAG_EXIT_CRITICAL(isr);
}
/*----------------------------------------------------------------------------------------------------*/

#else   /* DIAG_TRANSMIT_WITH_DMA == 0 */

__RAM_FUNC static void diag_port_log_send_thread(void *args)
{
    diag_list_t * log_head;
    void   * send_str;
    uint32_t send_len;

    DIAG_CRITICAL_DEF(isr);

    (void) args;

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while(1)
        {
            // 下面会判断全局变量，需要进入临界区
            DIAG_ENTER_CRITICAL(isr);

            log_head = diag_list_get_send_list();
            
            // 这里保证所有流程都在临界区中处理，实际上在获取到头节点后，就可以退出临界区
            // 这样修改的目的是为了尽快把log设置进DMA，否则退出临界区后，如果切换到其他线程，本来可以打印的log被推迟了
            DIAG_EXIT_CRITICAL(isr);

            if(log_head != NULL)
            {
                send_str = DIAG_LIST_GET_SEND_DATA(log_head);
                send_len = DIAG_LIST_GET_SEND_SIZE(log_head);
                
                diag_port_send_log_directly(send_str, send_len);
                
                DIAG_ENTER_CRITICAL(isr);

                // 进入中断，认为头节点已经打印完成，把该头节点加入到待释放的链表
                diag_list_insert_send_to_free();

                DIAG_EXIT_CRITICAL(isr);

                // 清空待释放链表，释放内存
                diag_port_free_all_free_list();
            }
            else
            {
                break;
            }
        }
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static void diag_port_send_head_valid_list(void)
{
    xTaskNotifyGive(diag_send_thread_handle);
}
/*----------------------------------------------------------------------------------------------------*/

#endif  /* DIAG_TRANSMIT_WITH_DMA == 1 */


__RAM_FUNC static void diag_port_free_all_free_list(void)
{
    diag_list_t * diag_head;
    void * free_mem;

    DIAG_CRITICAL_DEF(isr);

    // 死循环，直到释放完所有待释放链表的内存
    while(1)
    {
        // 下面会判断全局变量，需要进入临界区
        DIAG_ENTER_CRITICAL(isr);

        // 获取头节点，直到获取到NULL认为链表为空，已经释放所有内存，然后退出
        diag_head = diag_list_get_free_head();

        if(diag_head == NULL)
        {
            DIAG_EXIT_CRITICAL(isr);
            break;
        }

        // 移除待释放链表的头节点，释放该节点内存
        diag_list_remove_free_head();

        // 退出临界区，到这里全局变量已经使用完毕，下面的释放内存会占用较多时间，先退出临界区
        DIAG_EXIT_CRITICAL(isr);

        // 释放的内存，必须是调用者实际可用的内存，需要增加 diag_list_t 结构体的大小
        free_mem = (char *)diag_head + sizeof(diag_list_t);
        diag_mem_free(free_mem);
    }
}
/*----------------------------------------------------------------------------------------------------*/


__RAM_FUNC static void diag_port_uart_isr(void)
{
    #if (DIAG_PORT_USE_UART == 1)
    {
        uint16_t IntStatus;

        /* read and clear interrupt status, and use these status to do something */
        IntStatus = UARTIntReadAndClear(DIAG_PORT_UART_BASE);

        if(IntStatus & UART_INT_TIMEOUT)
        {
            // UARTTimeOutDisable(DIAG_PORT_UART_BASE);
            // UARTTimeOutEnable(DIAG_PORT_UART_BASE);
        }
    }
    #else   /* DIAG_PORT_USE_UART == 0 */
    {
        uint32_t IntStatus;

        IntStatus = CSPIntStatus(DIAG_PORT_UART_BASE);
        
        /* clear rxfifo_thd_reach_int_flag */
        if(IntStatus & CSP_INT_RXFIFO_THD_REACH)
        {
            CSPIntClear(DIAG_PORT_UART_BASE, CSP_INT_RXFIFO_THD_REACH);
            CSPIntDisable(DIAG_PORT_UART_BASE, CSP_INT_RXFIFO_THD_REACH);//必须关闭!!!
        }

        /* clear timeout_int_flag */
        if(IntStatus & CSP_INT_RX_TIMEOUT)
        {
            CSPIntClear(DIAG_PORT_UART_BASE, CSP_INT_RX_TIMEOUT);
        }
    }
    #endif  /* DIAG_PORT_USE_UART == 1 */

    // 通知log接收线程接收数据并处理
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(diag_recv_thread_handle, &pxHigherPriorityTaskWoken);
    portYIELD_FROM_ISR (pxHigherPriorityTaskWoken);
}
/*----------------------------------------------------------------------------------------------------*/

__FLASH_FUNC static void diag_port_uart_rcv_thread(void *args)
{
    uint8_t   uart_rx_fifo[32];
    uint32_t  uart_rcv_len;
    uint32_t  i;

    (void) args;

    // 流程开始，清空接收buffer
    diag_recv_reset_buffer();

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (1)
        {
            #if (DIAG_PORT_USE_UART == 1)
            {
                // FIFO 满的时候，读出得数值是0，需要处理这种情况
                uart_rcv_len = UARTRxFifoStatusGet(DIAG_PORT_UART_BASE, UART_FIFO_DATA_LEN);
                uart_rcv_len |= UARTRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_FULL) << 6;
                // 每次最多只能读取传入buffer的最大长度，防止内存越界
                uart_rcv_len = (uart_rcv_len > sizeof(uart_rx_fifo)) ? sizeof(uart_rx_fifo) : uart_rcv_len;

                for (i = 0; i < uart_rcv_len; i++)
                {
                    uart_rx_fifo[i] = UARTCharGet(DIAG_PORT_UART_BASE);
                }
            }
            #else   /* DIAG_PORT_USE_UART == 0 */
            {
                // FIFO 满的时候，读出得数值是0，需要处理这种情况
                uart_rcv_len = CSPRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_LEVEL);
                uart_rcv_len |= CSPRxFifoStatusGet(DIAG_PORT_UART_BASE, CSP_FIFO_FULL) << 7;
                // 每次最多只能读取传入buffer的最大长度，防止内存越界
                uart_rcv_len = (uart_rcv_len > sizeof(uart_rx_fifo)) ? sizeof(uart_rx_fifo) : uart_rcv_len;

                // 读取指定长度的数据
                for (i = 0; i < uart_rcv_len; i++)
                {
                    uart_rx_fifo[i] = CSPCharGet(DIAG_PORT_UART_BASE);
                }
            }
            #endif  /* DIAG_PORT_USE_UART == 1 */

            // 接收数据不为0时，处理数据，并继续循环，否则退出循环
            if(uart_rcv_len != 0)
            {
                g_log_status.data_recv_tick = (uint32_t)DIAG_GET_TICK_COUNT();
                diag_recv_write_data_to_buffer(uart_rx_fifo, uart_rcv_len);
                diag_recv_check_buffer_and_process_command();
            }
            else
            {
                break;
            }
        }
    }
}

__FLASH_FUNC void diag_dma_to_io(void)
{
#if (DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_PORT_USE_UART == 0)
	HWREG(DIAG_PORT_UART_BASE + CSP_TX_DMA_IO_CTRL) = CSP_TX_DMA_IO_CTRL_IO_MODE;
	HWREG(DIAG_PORT_UART_BASE + CSP_TX_DMA_IO_LEN) = 0;
	HWREG(DIAG_PORT_UART_BASE + CSP_TX_FIFO_CTRL) = CSP_TX_FIFO_CTRL_WIDTH_BYTE;
#endif
}

__FLASH_FUNC void diag_io_to_dma(void)
{
    // 使能硬件的DMA传输
#if (DIAG_TRANSMIT_WITH_DMA == 1) && (DIAG_PORT_USE_UART == 0)
	if(1)     //(diag_port_get_send_enable_state() == DIAG_SEND_ENABLE)
		CSPDMAConfigTX(DIAG_PORT_UART_BASE, 64 / sizeof(uint32_t), 0xFFFFFFFF);
#endif
}

/*----------------------------------------------------------------------------------------------------*/
