#include "at_uart.h"
#include "xinyi2100.h"
#include "hal_def.h"
#include "auto_baudrate.h"
#include "at_hardware_cmd.h"
#include "at_ipc_process.h"
#include "xy_memmap.h"
#include "sys_mem.h"
#include "at_process.h"
#include "xy_cp.h"
#include "at_cmd_regist.h"
#include "xy_flash.h"
#include "sema.h"
#include "sys_ipc.h"


#if (AT_LPUART == 1)

/*通过AT通道向外部输出的数据链表，通常为AT命令和打印log */
ListHeader_t g_send_list = {0};

HAL_LPUART_HandleTypeDef g_at_lpuart = {0};

//单条AT命令的最大长度，用户根据产品所用的AT命令自行定义
#define AT_UART_BUFFER_LENGTH           (1024 * 3)

//AT命令收到结束标识'\r'
#define Have_Recved_End_Flag()          (g_at_RecvBuffer[g_at_recved_len-1]=='\r' || g_at_RecvBuffer[g_at_recved_len-2]=='\r' || g_at_RecvBuffer[g_at_recved_len-3]=='\r')

//AT串口接收缓存
char g_at_RecvBuffer[AT_UART_BUFFER_LENGTH] = {0};

//AT串口接收到数据的实时长度
volatile uint16_t g_at_recved_len  = 0;

//若干毫秒内没收到新的数据，则认为对方已经发送完毕当前数据
uint16_t g_at_recv_timeout = 0;

//串口波特率大于9600时，睡眠状态下串口发送的第一条AT命令只能唤醒模组，不会正常执行，会关闭浅睡眠模式10s,10s需在NV文件可配置
uint8_t g_at_standby_timeout = 0;

//AT串口最近一次接收到字符的tick点，用来判断接收是否超时，超时则进行数据处理
volatile uint32_t g_at_last_recv_tick = 0;

/**
 * @brief 从NV参数中获取AT通道想配置的波特率，或者进行自动波特率检测
 */
static uint32_t at_uart_get_baudrate(void)
{
    //低9位保存设置波特率，高7位保存波特率自适应结果
    uint16_t baudrate_times = READ_FAC_NV(uint16_t, at_uart_rate);

    //如果是低9位波特率倍数有效则根据低9位NV来设置波特率，否则进行自动波特率检测，高7位为波特率自适应的结果
    if((baudrate_times & 0x1ff) != 0)
    {
        baudrate_times = baudrate_times & 0x1ff;
        // NV值大于384(即波特率大于921600)，默认设置为921600
        if(baudrate_times > 384)
        {
            baudrate_times = 384;
        }
        return (baudrate_times == 3) ? 1200 : (baudrate_times * 2400);
    }
    //如果是NV低9位波特率倍数为0，则进入波特率自适应
    else
    {   
#if MODULE_VER
    	//上电、复位则进行波特率检测，重写NV高7位，使用本次自适应结果来配置AT口
        return AutoBaudDetection();      
#else
		xy_assert(0);
#endif
    }
    return 0;
}

/**
 * @brief 从LPAURT寄存器中获取AT通道当前真实的波特率
 */
static uint32_t get_current_baudrate(HAL_LPUART_HandleTypeDef *hlpuart)
{
    uint32_t baudrate = 9600, pulConfig = 0;
    uint32_t valid_baud[] = {1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 230400, 380400, 460800, 921600};

    //lpuart的AON区使能打开，以及PRCM打开，才能对其寄存器进行操作
    PRCM_LPUA1_PadSel(hlpuart->Init.PadSel);
    PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);   

    //获取波特率
    UARTConfigGetExpClk(UART1_BASE, GetLpuartClockFreq(), &baudrate, &pulConfig);
    for(uint8_t i = 0; i < sizeof(valid_baud) / sizeof(valid_baud[0]); i++)
    {
        if ((baudrate > valid_baud[i] * 9 / 10) && (baudrate < valid_baud[i] * 11 / 10))
        {
            baudrate = valid_baud[i];
            break;
        }
    }

    return baudrate;
}

/**
 * @brief 从NV参数中获取AT通道想配置的奇偶校验位
 */
static HAL_LPUART_ParityModeTypeDef at_uart_get_parity()
{
#if VER_BC95  
    uint16_t parity_bit = READ_FAC_NV(uint8_t,at_parity);
    return parity_bit << UART_CTL_PARITY_Pos ;  
#else
    return UART_CTL_PARITY_NONE << UART_CTL_PARITY_Pos ;  
#endif
}

/**
 * @brief 配置LPUART的波特率和奇偶校验位
 * @param baudrate 1200~921600
 * @param parity   0：无校验位，1：偶校验，2：奇校验
 */
__RAM_FUNC void at_uart_config(uint32_t baudrate, uint32_t parity)
{
    PRCM_ClockEnable(CORE_CKG_CTL_LPUART_EN);
	PRCM_LPUA1_ClkSet_by_BaudRate(baudrate);
    UARTConfigSetExpClk(UART1_BASE, GetLpuartClockFreq(), baudrate, UART_CTL_CHAR_LEN_8 | parity<<UART_CTL_PARITY_Pos);
}

/**
 * @brief 清除AT通道置位的中断标志位
 */
__RAM_FUNC static void at_uart_clear_IT_flag(void)
{
    do {
        UART_IntClear(UART1_BASE, UART_INT_ALL);
    } while (UARTIntRead(UART1_BASE));
}

/**
 * @brief 初始化AT通道
 * @note  1. AT通道初始化调用HAL_LPUART_Init接口实现，LPUART的电源配置也在这个里面实现。
 *        2. AT通道的接收调用HAL_LPUART_Receive_IT接口实现，超时入参非HAL_MAX_DELAY，则表明即使用接收阈值中断又使用接收超时中断。
 *        3. AT通道的发送调用底层的driverlib接口实现。
 */
void at_uart_init(void)
{
    g_at_recv_timeout = READ_FAC_NV(uint16_t,at_recv_timeout);
	g_at_standby_timeout = READ_FAC_NV(uint16_t, standby_delay);

    // 配置LPUART
    g_at_lpuart.Instance = LPUART;
    g_at_lpuart.Init.PadSel = LPUART_PADSEL_RXD_GPIO4;
    g_at_lpuart.Init.WordLength = LPUART_WORDLENGTH_8;
	
    // 获取波特率，用于非深睡唤醒时重配AT口
    if(Get_Boot_Reason() != WAKEUP_DSLEEP) 
    {
        g_at_lpuart.Init.BaudRate = at_uart_get_baudrate();
    }

    //快速恢复则不会走到这里
    //若深睡前进行过波特率切换，深睡时Lpuart不断电，非快速恢复时 g_at_lpuart.Init.BaudRate 与实际波特率不一致，故深睡唤醒时直接从lpuart的寄存器获取实际使用的波特率
    if(Get_Boot_Reason() == WAKEUP_DSLEEP)
    {
        g_at_lpuart.Init.BaudRate = get_current_baudrate(&g_at_lpuart);
#if (MODULE_VER && AT_WAKEUP_SUPPORT)
		/*外部AT唤醒STANDBY，波特率高于9600，延迟若干秒开启STANDBY*/
		if((Get_Boot_Sub_Reason() & 1 << AT_WAKUP))
        {
			at_uart_standby_ctl();
        }
#endif
    }
    g_at_lpuart.Init.Parity = at_uart_get_parity();

    HAL_LPUART_Init(&g_at_lpuart);

    // 若波特率高于9600，芯翼后台会自动关闭STANDBY等级的低功耗，以防止数据脏乱。
	set_standby_by_rate(g_at_lpuart.Init.BaudRate);

    //从自适应流程获取到波特率，需要回复ok
    if(g_auto_rate_nv)
    {
#if(!VER_BC95)//非移远AT命令集时
        Send_AT_to_Ext("\r\nOK\r\n");
#endif
    }

	//复位或上电时，则清除lpuart所有中断标志位，并确保中断标志位清掉
	if(Get_Boot_Reason() != WAKEUP_DSLEEP)
    {
        at_uart_clear_IT_flag();
    }

    //准备中断接收数据
    HAL_LPUART_Receive_IT(&g_at_lpuart, (uint8_t *)g_at_RecvBuffer, AT_UART_BUFFER_LENGTH - 1, 3);
}

/**
 * @brief 中断函数中调用该接口，会挂链表，最终交由main主函数输出
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 * @note  中断服务函数可用、非中断函数也可用，但不保证AT通道输出数据的实时性
 */
__RAM_FUNC void at_lpuart_write(char *buf, int size)
{
	xy_assert(size != 0);

	/*中断中需要挂链表，交由main主函数输出，因为中断函数中不能长时间关中断*/
	if(IS_IRQ_MODE())
	{
		AtCmdList_t *pxlist;

		pxlist = xy_malloc(sizeof(AtCmdList_t) +size + 1);

		pxlist->next = NULL;
		pxlist->len = size;
		memcpy(pxlist->data,buf,size);
		*(pxlist->data + size) = '\0';

		ListInsert((List_t *)pxlist,&g_send_list);
	}
	else
	{
        //AT通道直接写数据至LPUART的TXFIFO，优点是简单高效、带核间互斥锁
        at_uart_write_fifo(buf, size);
	}
}

/**
 * @brief 通过AT通道发送ASCII码的AT命令，需要填写数据长度
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 * @note  (1) 中断服务函数可用、非中断函数也可用，但不保证AT通道输出数据的实时性
 *        (2) 当GNSS_EN开启且为码流模式时，该调用不会使AT通道输出数据
 */
__RAM_FUNC void at_uart_write_data(char *buf, int size)
{
		return at_lpuart_write(buf,size);
}

/**
 * @brief AT通道直接写数据至LPUART的TXFIFO，优点是简单高效、带核间互斥锁，通常用于导DUMP场景，也可被封装为其他AT发送接口
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 * @note  用于AT通道必须立即输出数据的场景，如导DUMP
 */
__RAM_FUNC void at_uart_write_fifo(char *buf, int size)
{
    xy_assert(size != 0);

    uint32_t sema_have = 0;

    //长URC命令，由于在AP核输出耗时过久造成内存耗尽，进而放在CP核直接输出，通过硬件锁保证AP和CP共享LPUART发送
    if(CP_Is_Alive())
    {
        do {
            SEMA_RequestNonBlocking(SEMA_ATWR_AUX, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_CP);
        } while (SEMA_MASTER_AP != SEMA_MasterGet(SEMA_ATWR_AUX));
        sema_have = 1;
    }
    
	//发送数据
	while(size > 0)
	{
        if(!LPUART_IS_TXFIFO_FULL())
        // if(!UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_FULL))
		{
            UARTWriteData(UART1_BASE, *buf);
			size--;
			buf++;
		}
	}

    //等待LPUART数据发送完成，若数据一直发送不完则2秒后自动退出，
    //若2s内发送完成了，则根据波特率增加1个字符的延时以保证数据全部发送至串口总线上。
    at_uart_wait_send_done();

    //释放硬件锁
    if (sema_have == 1)
    {
        SEMA_Release(SEMA_ATWR_AUX, SEMA_MASK_NULL);
    }
}

/**
 * @brief 等待LPUART数据发送完成，若数据一直发送不完则2秒后自动退出，
 *        若2s内发送完成了，则根据波特率增加1个字符的延时以保证数据全部发送至串口总线上。
 */
__RAM_FUNC void at_uart_wait_send_done(void)
{
	uint32_t tickstart = Get_Tick();
    
    //如果LPUART没有发送完成，则等待，等待超时为2秒
    while (!LPUART_IS_TXFIFO_EMPTY())
	// while(!UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY))
	{
		if(Check_Ms_Timeout(tickstart,2000))
        {
            return;
        }
	}

	//针对不同速率，增加1个字符的延时，以保证退出该接口时数据发送完成
	uint32_t onechar_timeout = (uint32_t)((15 * 1000 * 1000 / g_at_lpuart.Init.BaudRate) + 1); //按15bit计算，单位us
    if(onechar_timeout < 100)
    {
        onechar_timeout = 100;
    }
	HAL_Delay_US(onechar_timeout);
}

/**
 * @brief 通过AT通道发送AT命令错误码
 * @param err_no 错误码类型，不得为0，详见 @ref At_status_type.
 */
char *at_err_build(uint16_t err_no);
static void at_send_err_to_ext(uint16_t err_no)
{
	char *err_str = at_err_build(err_no);
	Send_AT_to_Ext(err_str);
	xy_free(err_str);
}

/**
 * @brief 处理AT命令回显，部分客户有特殊定制
 * @param buf  待发送数据的起始地址
 * @param size 待发送数据长度
 */
static void at_echo_process(char *buf, uint32_t size)
{
#if VER_BC95 || VER_BC25
    if (buf[size - 1] == '\r')
    {
        at_uart_write_data(buf, size - 1);
    }
    else if (buf[size - 2] == '\r')
    {
        at_uart_write_data(buf, size - 2);
    }
    else if (buf[size - 3] == '\r')
    {
        at_uart_write_data(buf, size - 3);
    }
    else
    {
        xy_assert(0);
    }
#else
    (void)size;
    Send_AT_to_Ext(buf);
#endif
}

/**
 * @brief 将接收到的AT命令进行处理
 * @param buf  接收到数据的起始地址
 * @param size 接收到数据长度 
 */
static void at_process_cmd(char *buf,uint32_t size)
{
    At_status_type ret = XY_OK;

#if VER_BC95   
    natspeed_succ_hook();    
#endif

    //透传模式直接转发给CP处理
    if(AT_PASSTHR_MODE)
    {
        goto FORWARD;
    }

    //首字母为\n或者\0的AT命令直接丢弃
    if ((buf[0] == '\n') || (buf[0] == '\0'))
    {
        char *invaild_cmd_ack = xy_malloc(40);
        sprintf(invaild_cmd_ack, "\r\nat wait but first recv:0x%x\r\n", buf[0]);
        Send_AT_to_Ext(invaild_cmd_ack);
        xy_free(invaild_cmd_ack);
        return;
    }

#if VER_BC95
    //非透传模式下仅收到\r\n或者\r时，不做任何处理
    if (!AT_PASSTHR_MODE && buf[0] == '\r')
    {
        return;
    }
#endif

	//回显模式开启
	if(AT_ECHO_MODE && HWREGB(BAK_MEM_RF_MODE) == 0)
	{
        at_echo_process(buf, size);
    }

    //先AP核匹配AT命令，若匹配失败则转发给CP核处理
    if(Have_Recved_End_Flag() && Match_AT_Cmd(buf))
    {
        return;
    }
    //通过核间消息发送AT命令到CP核，内部进行零拷贝
    else
    {
FORWARD:
        ret = AT_Send_To_CP(buf, size, AT_FROM_EXT_MCU);
		if(ret != XY_OK)
		{
			at_send_err_to_ext(ret);
		}      
    }
}

/**
 * @brief 清零全局标志并开启LPUART接收中断，为下次AT命令接收做准备
 */
__RAM_FUNC void reset_uart_recv_buf(void)
{
	//清零全局标志
	g_at_last_recv_tick = 0;
	g_at_recved_len = 0;
	g_at_lpuart.RxState = LPUART_STATE_READY;

	//开启LPUART接收中断
	HAL_LPUART_Receive_IT(&g_at_lpuart, (uint8_t *)g_at_RecvBuffer, AT_UART_BUFFER_LENGTH - 1, 3);
}

/**
 * @brief  检测当前是否有待收发处理的AT命令，若未处理则退出睡眠
 * @return false ：AT命令收发处理未结束，AT通道处于非IDLE态
 *         true  ：AT命令收发处理已结束，AT通道处于IDEL态
 */
__RAM_FUNC bool at_uart_idle()
{
    //AT命令处理中，或AT命令接收中
    if(g_at_recved_len || g_at_lpuart.RxXferCount)
    {
        return false;
    }

    //LPUART_RXFIFO非空，或RX总线活跃
    if((!UARTRxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY)) || (!UARTRxIdle(UART1_BASE)))
    {
        return false;
    }

    //LPUART_TXFIFO非空
    //进standby时要保证txfifo为空，若txfifo非空进standby会无法AT唤醒
	if (LPUART_IS_TXFIFO_EMPTY() == 0)
    // if(!UARTTxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY))
    {
        return false;
    }

    //AT通道向外部输出的数据链表节点个数非0
    if(GetListNum(&g_send_list))
    {
        return false;
    }

    return true;
}

/**
 * @brief 解析接收到的AT命令解析和AT命令回复、上报
 * @note  OPENCPU产品不使用，进而放在flash上不影响功耗
 */
__RAM_FUNC void at_uart_recv_and_process(void)
{
	AtCmdList_t *pxlist = NULL;
	
	//先在main主函数中，把待发送给外部的数据发送出去，包括AT和log
	while((pxlist = (AtCmdList_t *)ListRemove(&g_send_list)))
	{
		at_uart_write_data(pxlist->data, pxlist->len);
		xy_free(pxlist);
	}

    //AT通道接收到的数据长度为零时直接退出，此时无AT命令需要处理
    if(g_at_recved_len == 0)
    {
        return;
    }


    //AT接收超时，或者接收到AT结束标记，或者透传模式，则开始处理接收到的内容
	uint8_t at_recv_timeout_flag = Check_Ms_Timeout(g_at_last_recv_tick, g_at_recv_timeout);
    if(AT_PASSTHR_MODE || Have_Recved_End_Flag() || at_recv_timeout_flag)
    {
        //收到AT命令后禁能LPUART的正常接收中断，这里禁能的中断类型需要和HAL_LPUART_Receive_IT里开启的中断对应
        //注意这里关闭LPUART正常接收中断时，外部有可能还在继续发送数据给LPUART，LPUART存在溢出风险，即存在AT命令丢失风险
        //LPUART_RXFIFO深度为32字节，最大关中断且不溢出时间 t(ms) = 1 / AT波特率 × 10 × 1000 × 32，若AT波特率为9600，则t为33.33ms
        //若LPUART接收溢出，则会进入错误回调函数进行容错处理
        UARTIntDisable(UART1_BASE, UART_INT_RXFIFO_TRIGGER | UART_INT_TIMEOUT);

        
#if (MODULE_VER && AT_WAKEUP_SUPPORT)
		//超过9600收到AT命令，延迟若干秒进入STANDBY
        at_uart_standby_ctl();	

        //超过9600的AT唤醒，第一条会脏乱，丢弃不回复ERROR 	 	
        if(!AT_PASSTHR_MODE && drop_dirty_at_from_sleep())
        {
            goto END;	
        }
#endif

        //AT接收缓存数组已满，表明本次接收的AT长度已超出单条AT命令的最大长度，返回错误，当fifo中仍有数据清空
        if ((g_at_recved_len >= AT_UART_BUFFER_LENGTH - 1) || (at_recv_timeout_flag && !Have_Recved_End_Flag()))
        {
            //读空RX_FIFO中多余数据，避免下条AT命令脏
            while(!UARTRxFifoStatusGet(UART1_BASE, UART_FIFO_EMPTY))
            {
                (void)UARTReadData(UART1_BASE);
            }
            at_recv_timeout_flag ? at_send_err_to_ext(XY_ERR_DROP_MORE) : at_send_err_to_ext(XY_ERR_NOT_ALLOWED);
            goto END;
        }

        //增加字符串结束符方便进行字符串操作
        g_at_RecvBuffer[g_at_recved_len] = '\0';

        //处理收到的AT命令
        at_process_cmd(g_at_RecvBuffer, g_at_recved_len);

 END:
        //清零全局标志并开启LPUART接收中断，为下次AT命令接收做准备
        reset_uart_recv_buf();
    }
}


/**
 * @brief AT通道有效接收完成回调函数，有效是指没有帧错误、校验错误、接收溢出
 * @note  AT通道只有在接收超时或者接收到指定长度的数据时(由RxXferSize指定)才会调用该函数
 *        进入该函数后，接收阈值中断和超时中断都会关闭，调用HAL_LPUART_Receive_IT接口可以开启这两个中断
 */
__RAM_FUNC void HAL_LPUART_RxCpltCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
    (void)hlpuart;
    uint16_t remainder_bytes = 0;
    uint8_t *recvbuffer_offset = 0;

    at_uart_clear_IT_flag(); //解决在高波特率下，当读取AT命令的执行时间大于LPUART超时时间(31.25us)时，进入回调函数会带有一个接收超时中断标志位，当回调内再次开启超时中断时就会直接断言。

    xy_assert(g_at_lpuart.RxXferCount);

    g_at_recved_len += g_at_lpuart.RxXferCount;

    //更新下次AT缓存偏移地址
    if(g_at_recved_len < AT_UART_BUFFER_LENGTH - 1)
    {
        remainder_bytes = AT_UART_BUFFER_LENGTH - g_at_recved_len - 1;
        recvbuffer_offset = (uint8_t *)(g_at_RecvBuffer + g_at_recved_len);

        //开启LPUART接收中断
		HAL_LPUART_Receive_IT(&g_at_lpuart, (uint8_t *)(recvbuffer_offset), remainder_bytes, 3);
    }
    else
    {
        UARTIntDisable((uint32_t)g_at_lpuart.Instance, UART_INT_RXFIFO_TRIGGER | UART_INT_TIMEOUT);
    }

    //更新最近一次接收到数据的时刻点
    if(g_at_lpuart.RxXferCount!=0)
    {
        g_at_last_recv_tick = Get_Tick();
    }
}

/**
 * @brief AT串口错误回调函数，发生相关接收错误中断时，要读取接收FIFO中的数据，并进行处理
 */
__RAM_FUNC void HAL_LPUART_ErrorCallback(HAL_LPUART_HandleTypeDef *hlpuart)
{
    (void)hlpuart;
    uint16_t remainder_bytes;
    uint8_t *recvbuffer_offset;

    //发生错误时也要讲数据全部读走，并放入AT接收缓存中，AT命令长度也要增加
    if(g_at_lpuart.ErrorCode & (LPUART_ERROR_FRAME_ERR|LPUART_ERROR_PARITY_ERR|LPUART_ERROR_RX_FIFO_OVF))
    {
        if(g_at_lpuart.ErrorCode & LPUART_ERROR_RX_FIFO_OVF)
        {
            xy_printf("\r\n+DBGINFO:AT OVERFLOW!\r\n");
        }

        while(!UARTRxFifoStatusGet((uint32_t)g_at_lpuart.Instance, UART_FIFO_EMPTY))
        {
            if(g_at_lpuart.RxXferSize != 0)
            {
                *(g_at_lpuart.pRxBuffPtr) = UARTReadData((uint32_t)g_at_lpuart.Instance);
                g_at_lpuart.pRxBuffPtr++;
                g_at_lpuart.RxXferCount++;
                g_at_lpuart.RxXferSize--;
            }
            else
            {
                (void)UARTReadData((uint32_t)g_at_lpuart.Instance);
            }
        }

        g_at_recved_len += g_at_lpuart.RxXferCount;
        g_at_lpuart.ErrorCode = LPUART_ERROR_NONE;
    }

    //更新下次AT缓存偏移地址
    if(g_at_recved_len < AT_UART_BUFFER_LENGTH - 1)
    {
        remainder_bytes = AT_UART_BUFFER_LENGTH - g_at_recved_len - 1;
        recvbuffer_offset = (uint8_t *)(g_at_RecvBuffer + g_at_recved_len);
    }
    else
    {
        remainder_bytes = AT_UART_BUFFER_LENGTH - 1;
        recvbuffer_offset = (uint8_t *)g_at_RecvBuffer;
    }
    
    //更新最近一次接收到数据的时刻点
    if(g_at_lpuart.RxXferCount != 0)
    {
        g_at_last_recv_tick = Get_Tick();
    }

    //当产生AT通道接收错误时，要保证开启下次接收时中断标志位为0
    at_uart_clear_IT_flag();

    //发生错误并收完数据后，需要释放RxState为READY状态
    g_at_lpuart.RxState = LPUART_STATE_READY;
    
    //开启LPUART接收中断
    HAL_LPUART_Receive_IT(&g_at_lpuart, (uint8_t *)(recvbuffer_offset), remainder_bytes, 3);
}

#endif
