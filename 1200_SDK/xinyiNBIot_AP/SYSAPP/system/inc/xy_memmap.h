/**
 *
 * @brief  芯片的内存和flash映射表，以指导用户更清晰的使用内存
 *
 */
#pragma once

/***************************************SRAM CP***********************************************************
           _ _ _ _ _ _ _ ____________0x10000000
            |           |
        160K|  0x20000  | text/data/bss/heap/stack
           _|_ _ _ _ _ _|____________0x10028000

***************************************************************************************************/

/*****************************************SRAM AP*********************************************
                    _ _ _ _ __ _ _ _ _ _ ____________0x0100_0000
                    |   |            |
                    |   |  0x1FF80   | ARM  text / data / bss / heap / stack    1st boot(0x0101a000 - 0x01020000)
                 128K   | _ _ _ _ _ _|____________0x0101_FF80                   2st boot(0x01010000 - 0x0101a000)
                    |   |            |
                    |   |    0x80    | 1st boot(stack)
                    |_ _|_ _ _ _ _ _ |____________0x0102_0000

*****************************************SRAM AP***********************************************/
#define  SRAM_BASE                            (0x01000000)
#define  SRAM_LENGTH                          (0x20000)


/********************************shared memory*************************************************************
             _ _ _ _ _ _ ____________0x60000000
            |           |
            |  0x10000   | heap2 for cp                  // cp核的第二块heap
            |_ _ _ _ _ _|____________0x60010000

***************************************************************************************************/
#define  SHARE_MEM_BASE        0x60000000
#define  SHARE_MEM_LENGTH      0x10000

#define  CP_HEAP_START         SHARE_MEM_BASE    //0x60000000，OS的heap
#define  CP_HEAP_LEN           SHARE_MEM_LENGTH

/*****************************************RET RAM AP*********************************************
                    _ _ __ _ _ _ _ _ ____________0x6001_0000
                    |   |            |
                    |   |            |   shared_variables(0x120)
                   4K   |  0x1000    |   ps_volatile(0xEE0)
                    |__ |_ _ _ _ _ _ |_ ____________0x6001_1000
                    |   |            |   reserved for user(1112)
                    |   |            |   ipc(0x400)
                    |   |            |   phy_volatile(0x400)
                    |   |            |   lpm_volatile(56)
                   4K   |  0x1000    |   platform_volatile(128)
                    |   |            |   CP_RTC+CP_CCLK+AT_CLOCKTICK+OTP(大小已确认，760)
                    |__ |_ _ _ _ _ _ |_ ____________0x6001_1FFF
*****************************************RET RAM AP***********************************************/
#define  BAK_MEM_BASE                           0x60010000UL
#define  BAK_MEM_LENGTH                         0x2000UL

#define  BAK_MEM_SHM_BASE                       (BAK_MEM_BASE)                                                 // 0x60010000
#define  BAK_MEM_SHM_LEN                        0x120                                                          // 0X120,核间共享状态机信息长度,截止地址为0x60010120

#define  RAM_NV_VOLATILE_PS_START               (BAK_MEM_BASE + BAK_MEM_SHM_LEN)                               // read and write by AP,not CP
#define  RAM_NV_VOLATILE_PS_LEN                 0XEE0 //3808


#define  RESERVED_USER_BAK_MEM_BASE             (RAM_NV_VOLATILE_PS_START + RAM_NV_VOLATILE_PS_LEN)            //0x60011000
#define  RESERVED_USER_BAK_MEM_LEN              0X458 //1112

#define  IPCMSG_BUF_BASE_ADDR                   (RESERVED_USER_BAK_MEM_BASE + RESERVED_USER_BAK_MEM_LEN)       //0x60011458
#define  IPCMSG_BUF_LEN                         0x400 //1024

#define  RAM_NV_VOLATILE_PHY_START              (IPCMSG_BUF_BASE_ADDR + IPCMSG_BUF_LEN)                        //0x60011858
#define  RAM_NV_VOLATILE_PHY_LEN                0x400 //1024

#define  RAM_NV_VOLATILE_LPM_START              (RAM_NV_VOLATILE_PHY_START+RAM_NV_VOLATILE_PHY_LEN)            //0x60011C58
#define  RAM_NV_VOLATILE_LPM_LEN                0x38  //56

#define  RAM_NV_VOLATILE_SOFTAP_START           (RAM_NV_VOLATILE_LPM_START+RAM_NV_VOLATILE_LPM_LEN)            //0x60011C90
#define  RAM_NV_VOLATILE_SOFTAP_LEN             0x80  //128

/* CP侧RTC 以下部分占用地址空间已确认，占用大小0x2F8,起始地址为0x60011D10*/
#define  BAK_MEM_CP_RTC_BASE                    0x60011D10                                                     // backup mem中CP侧RTC起始地址，必须是实际的retension memory地址
#define  BAK_MEM_CP_RTC_LEN                     148                                                            // backup mem中CP侧RTC总长度

#define  BAK_MEM_CP_RTC_ALARM                   (BAK_MEM_CP_RTC_BASE)                                          // 0x60011D10,CP侧 RTC alarm值
#define  BAK_MEM_CP_RTC_ALARM_LEN               8                                                              // CP侧 RTC alarm值的长度
#define  BAK_MEM_CP_RTCLIST_BASE                (BAK_MEM_CP_RTC_ALARM + BAK_MEM_CP_RTC_ALARM_LEN)              // 0x60011D18,CP侧 RTC 链表起始地址        （0x60011BC0）
#define  BAK_MEM_CP_RTCLIST_LEN                 140                                                            // CP侧 RTC 链表长度           （可存放7个RTC事件）

/**/
#define  BAK_MEM_PADDING1                       (BAK_MEM_CP_RTCLIST_BASE + BAK_MEM_CP_RTCLIST_LEN)             // 0x60011DA4
#define  BAK_MEM_PADDING1_LEN                    8                                                          
                      

/* AP侧TIME，需深睡保持,在Time_Init中进行初始化 */
#define  BAK_MEM_AP_TIME_BASE                   (BAK_MEM_PADDING1 + BAK_MEM_PADDING1_LEN)                      // 0x60011DAC,backup mem中AP侧TIME起始地址  （0x60011C54）
#define  BAK_MEM_AP_TIME_LEN                    128                                                          // AP侧 TIME 链表长度     （可存放6个CLK_TIMER事件）

/* OTP预留452字节 */
#define  BAK_MEM_OTP_BASE                       (BAK_MEM_AP_TIME_BASE + BAK_MEM_AP_TIME_LEN)                   // 0x60011E2C,backup mem中OTP起始地址      （0x60011FF0）
#define  BAK_MEM_OTP_LEN                        452

#define  BAK_MEM_OTP_RSA_BASE                   (BAK_MEM_OTP_BASE)                                             // 0x60011E2C,安全启动相关，头部添加55AA55AA，需要偏移4字节使用
#define  BAK_MEM_OTP_RSA_LEN                    140

#define  BAK_MEM_OTP_ADCAL_BASE                 (BAK_MEM_OTP_RSA_BASE + BAK_MEM_OTP_RSA_LEN)                   // 0x60011EB8,ADC校准数据，头部添加校准信息有效标志位，需要偏移4字节使用
#define  BAK_MEM_OTP_ADCAL_LEN                  60

#define  BAK_MEM_OTP_TRXBGCAL_BASE              (BAK_MEM_OTP_ADCAL_BASE + BAK_MEM_OTP_ADCAL_LEN )              // 0x60011EF4
#define  BAK_MEM_OTP_TRXBGCAL_LEN               4

#define  BAK_MEM_OTP_SVDCAL_BASE                (BAK_MEM_OTP_TRXBGCAL_BASE + BAK_MEM_OTP_TRXBGCAL_LEN )        // 0x60011EF8,SVD校准数据
#define  BAK_MEM_OTP_SVDCAL_LEN                 4

#define  BAK_MEM_OTP_HRCCAL_BASE                (BAK_MEM_OTP_SVDCAL_BASE + BAK_MEM_OTP_SVDCAL_LEN )            // 0x60011EFC,HRC26M校准数据,写入0x40000812, bit[7:0]
#define  BAK_MEM_OTP_HRCCAL_LEN                 4

#define  BAK_MEM_OTP_RC32KCAL_BASE              (BAK_MEM_OTP_HRCCAL_BASE + BAK_MEM_OTP_HRCCAL_LEN )            // 0x60011F00,RC32K校准数据
#define  BAK_MEM_OTP_RC32KCAL_LEN               4

#define  BAK_MEM_OTP_HPBGCAL_BASE               (BAK_MEM_OTP_RC32KCAL_BASE + BAK_MEM_OTP_RC32KCAL_LEN )        // 0x60011F04,HPBG校准数据,写入0x40000806, bit[6:2]
#define  BAK_MEM_OTP_HPBGCAL_LEN                4

#define  BAK_MEM_OTP_RCCAL_BASE                 (BAK_MEM_OTP_HPBGCAL_BASE + BAK_MEM_OTP_HPBGCAL_LEN )        // 0x60011F08,RC校准数据
#define  BAK_MEM_OTP_RCCAL_LEN                  4

#define  BAK_MEM_OTP_SOC_ID_BASE                (BAK_MEM_OTP_RCCAL_BASE + BAK_MEM_OTP_RCCAL_LEN)           // 0x60011F0c, 0x0：old，0x1：new1
#define  BAK_MEM_OTP_SOC_ID_LEN                 4  

#define  BAK_MEM_OTP_PADDING                    (BAK_MEM_OTP_SOC_ID_BASE + BAK_MEM_OTP_SOC_ID_LEN)         // 0x60011F10,OTP预留空间剩余部分，截止地址为0x60011FF0
#define  BAK_MEM_OTP_PADDING_LEN                224   


/* AP和CP核的握手状态机信息,所有的变量必须考虑retension mem深睡断电的影响，如果必须初始化，请在ShmFlagInit中执行！
***如果是CP核用的状态机，还需要在boot_CP接口内部执行动态初始化！
*/
#define  BAK_MEM_AP_UP_REASON             (BAK_MEM_SHM_BASE)                    // 0x60010000,单字节，AP核的上电原因，从寄存器获取，具体参看Boot_Reason_Type
#define  BAK_MEM_AP_UP_SUBREASON          (BAK_MEM_SHM_BASE + 0x04)             // 0x60010004,4字节长，AP核上电细分子状态，即AP_UP_REASON对应的细分状态
#define  BAK_MEM_FLASH_DYN_SYNC           (BAK_MEM_SHM_BASE + 0x08)             // 0x60010008,sizeof(flash_op_sync)8字节长，双核flash同步使用
#define  BAK_MEM_EXT_AT_CMD_ADDR          (BAK_MEM_SHM_BASE + 0x10)             // 0x60010010,4字节长，待废弃！AP定制AT命令首地址，AP核加载CP时初始化，CP核读取，串口收到外部AT命令时如果匹配到该列表，则转发至AP处理
#define  BAK_MEM_CP_USED_APRAM_ADDR       (BAK_MEM_SHM_BASE + 0x14)             // 0x60010014,4字节长，未使用！CP向AP借的heap首地址，供CP核的log和heap堆使用
#define  BAK_MEM_CP_USED_APRAM_SIZE       (BAK_MEM_SHM_BASE + 0x18)             // 0x60010018,单字节，无使用！CP向AP借的heap大小
#define  BAK_MEM_AP_LOG                   (BAK_MEM_SHM_BASE + 0x19)             // 0x60010019,单字节，动态设置LOG输出类别，与出厂NV参数open_log功能保持一致
#define  BAK_MEM_OFF_RETMEM_FLAG          (BAK_MEM_SHM_BASE + 0x1a)             // 0x6001001a,单字节，retmem下电状态机。0:retmem保持长供电；1:深睡下电前，CP通知AP核将retmem的内容保存至flash；2:深睡唤醒后，AP通知CP核将retmem恢复为深睡前的内容
#define  BAK_MEM_TEST                     (BAK_MEM_SHM_BASE + 0x1b)             // 0x6001001b,单字节，内部测试状态。1：HTOL测试模式；
#define  BAK_MEM_ATUART_STATE             (BAK_MEM_SHM_BASE + 0x1c)             // 0x6001001c,单字节，CP核赋值；ATUART位图状态机，bit0：透传模式；bit1：回显模式；bit3-bit2：AT命令错误码显示模式；AP核赋值，bit5:表示是否有AT命令正在接收处理中
#define  BAK_MEM_RF_MODE                  (BAK_MEM_SHM_BASE + 0x20)             // 0x60010020,单字节，RF模式，仅NRB软重启保持有效，其他上电/重启/唤醒皆需清零
#define  BAK_MEM_CP_DO_DUMP_FLAG          (BAK_MEM_SHM_BASE + 0x21)             // 0x60010021,单字节，CP死机与AP的握手，1用于CP死机时通过核间中断通知AP；
#define  BAK_MEM_BOOT_CP_SYNC             (BAK_MEM_SHM_BASE + 0x22)             // 0x60010022,单字节，AP动态启动CP握手状态机，以及CP低功耗状态机
#define  BAK_MEM_AP_STOP_CP_REQ           (BAK_MEM_SHM_BASE + 0x23)             // 0x60010023,单字节，AP强行停CP的握手状态机，1表示CP核目前软件正常运行；2：表示CP已执行完CFUN=5
#define  BAK_MEM_CP_UP_REASON             (BAK_MEM_SHM_BASE + 0x24)             // 0x60010024,单字节，CP核的上电原因，从寄存器获取，具体参看Boot_Reason_Type
#define  BAK_MEM_CP_SET_RTC               (BAK_MEM_SHM_BASE + 0x25)             // 0x60010025,单字节，仅供OPENCPU产品使用，指示是否支持CP核的RTC唤醒，默认不支持，与出厂NV参数set_CP_rtc功能一致
#define  BAK_MEM_CP_UP_SUBREASON          (BAK_MEM_SHM_BASE + 0x28)             // 0x60010028,4字节长，CP核上电细分子状态位图，即CP_UP_REASON对应的细分状态
#define  BAK_MEM_FOTA_RUNNING_FLAG        (BAK_MEM_SHM_BASE + 0x2c)             // 0x6001002c,单字节， 1表示正在FOTA流程。OTA过程状态机，参看XY_OTA_STAT_E,CP核置位，AP核仅读
#define  BAK_MEM_DROP_URC                 (BAK_MEM_SHM_BASE + 0x2d)             // 0x6001002d,单字节，OPENCPU部分客户要求关闭所有的URC上报给AP，1表示不上报任何URC
#define  BAK_MEM_AP_DO_DUMP_FLAG          (BAK_MEM_SHM_BASE + 0x2e)             // 0x6001002e,单字节长，AP死机与CP的握手，1用于AP死机时通过核间中断通知CP
#define  BAK_MEM_DUMP_LOGVIEW_FLAG        (BAK_MEM_SHM_BASE + 0x2f)             // 0x6001002F,单字节长，双核dump同步标志位,ap,cp各使用四位,
#define  BAK_MEM_CP_RETMEM_CSM            (BAK_MEM_SHM_BASE + 0x30)             // 0x60010030,4字节长，CP核retmem校验值，仅调试用
#define  BAK_MEM_AP_RETMEM_CSM            (BAK_MEM_SHM_BASE + 0x34)             // 0x60010034,4字节长，AP核retmem校验值，仅调试用
#define  BAK_MEM_RC32K_FREQ               (BAK_MEM_SHM_BASE + 0x38)             // 0x60010038,4字节长，RC或xtal 32K的上电测量时钟值，AP核设置，CP核读取使用
#define  BAK_MEM_FOTA_FLASH_BASE          (BAK_MEM_SHM_BASE + 0x3c)             // 0x6001003c,4字节长，FOTA区域起始地址，受AP核版本影响；由AP赋值，CP和二级boot的FOTA使用
#define  BAK_MEM_AP_FAST_RECOVERY_FUNC    (BAK_MEM_SHM_BASE + 0x40)             // 0x60010040,单字节, AP传递给CP, 1:表示AP支持快速恢复,不允许自行修改
#define  BAK_MEM_BAN_WRITE_FLASH          (BAK_MEM_SHM_BASE + 0x41)             // 0x60010041,单字节，opencpu形态，禁止实时写FLASH，仅AP单核或SoC深睡时容许写，FOTA除外
#define  BAK_MEM_LPUART_USE               (BAK_MEM_SHM_BASE + 0x42)             // 0x60010042,单字节, AP传递给CP,0:表示未使用LPUART,1:表示使用LPUART
#define  BAK_MEM_MODULE_VER               (BAK_MEM_SHM_BASE + 0x43)             // 0x60010043,单字节, AP传递给CP, 1:表示模组形态, 注意取值由define.cmake中对应宏决定,不允许另行修改
#define  BAK_MEM_CLOUDTEST                (BAK_MEM_SHM_BASE + 0x44)             // 0x60010044,4字节长，云测试工程使用，通过AT命令配置工程参数
#define  BAK_MEM_OPEN_TEST                (BAK_MEM_SHM_BASE + 0x48)             // 0x60010048,单字节，用于OPENCPU随机测试的阈值控制，0表示不启用
#define  BAK_MEM_LPM_FLASH_STATUS         (BAK_MEM_SHM_BASE + 0x49)             // 0x60010049,单字节，AP在standby前后操作flash供电状态，供CP查询,STANDBY_FLASH_STATUS_ENUM
#define  BAK_MEM_SOC_VER                  (BAK_MEM_SHM_BASE + 0x4a)             // 0x6001004a,单字节，二级boot通过OTP识别芯片类型，AP核软件平台使用。0:XY1200L;1:XY1200SL;2:XY2100SL;3:XY1200;4:XY1200S;5:XY2100S
#define  BAK_MEM_XY_DUMP                  (BAK_MEM_SHM_BASE + 0x4b)             // 0x6001004b,单字节，SoC软件异常主动断言，且关闭看门狗，保持现场。CP核调试相关代码也会生效，会输出一些异常log。
#define  BAK_MEM_FLASH_NOTICE             (BAK_MEM_SHM_BASE + 0x4c)             // 0x6001004c,4字节长，cp写flash时通知ap挂起
#define  BAK_MEM_RTC_NEXT_OFFSET          (BAK_MEM_SHM_BASE + 0x50)             // 0x60010050,8字节长，CP侧最快超时的RTC ALARM事件的剩余毫秒偏移，供AP睡眠前裁决
#define  BAK_MEM_TICK_CAL_BASE            (BAK_MEM_SHM_BASE + 0x58)             // 0x60010058,4字节长，计算CP侧最快超时的RTC ALARM事件剩余毫秒偏移的TICK时刻点，供AP睡眠前裁决
#define  BAK_MEM_AP_LOCK_TYPE             (BAK_MEM_SHM_BASE + 0x5c)             // 0x6001005c,单字节，AP的睡眠锁位图传递给CP。bit0为DEEPSLEEP锁；bit1为STANDBY锁；bit2为WFI锁
#define  BAK_MEM_RC32K_CALI_FLAG          (BAK_MEM_SHM_BASE + 0x5d)             // 0x6001005d,单字节，AP传递给CP，1:表示AP核此时处于RC32K校准流程，CP核不允许进入睡眠，规避唤醒后补偿phytimer误差太大，校准流程完成后标志位由AP置0
#define  BAK_MEM_RC32K_CALI_PERMIT        (BAK_MEM_SHM_BASE + 0x5e)             // 0x6001005e,单字节，CP传递给AP，0:表示CP进入睡眠，此时不允许AP进行RC32K校准流程，CP退出睡眠时置1
#define  BAK_MEM_RC32K_DIAGNOSE           (BAK_MEM_SHM_BASE + 0x5f)             // 0x6001005f,单字节，协议栈ppm值异常后置1，AP检测该负反馈标志进行一组共10次的mcnt测量
#define  BAK_MEM_MCNT_PROCESS_FLAG        (BAK_MEM_SHM_BASE + 0x60)             // 0x60010060,单字节，1:表示AP收到协议栈负反馈标志位后正在进行一组共10次的mcnt测量，完成后清0
#define  BAK_MEM_PADDING5          	  	   (BAK_MEM_SHM_BASE + 0x61)             // 0x60010061,2字节，未使用
#define  BAK_MEM_32K_CLK_SRC              (BAK_MEM_SHM_BASE + 0x63)             // 0x60010063,单字节，32K时钟源，1：xtal32k，0：rc32k
#define  BAK_MEM_CLOCK_TICK               (BAK_MEM_SHM_BASE + 0x64)             // 0x60010064,4字节长
#define  BAK_MEM_LAST_WAKEUP_TICK         (BAK_MEM_SHM_BASE + 0x68)             // 0x60010068,4字节长,上次深睡唤醒的clk tick时刻点
#define  BAK_MEM_CP_ASSERT_INFO           (BAK_MEM_SHM_BASE + 0x6C)             // 0x6001006c,4字节长,CP死机信息
#define  BAK_MEM_FORCE_STOP_CP_HANDSHAKE  (BAK_MEM_SHM_BASE + 0x70)             // 0x60010070,1字节长,停CP的握手信号
#define  BAK_MEM_PADDING4                 (BAK_MEM_SHM_BASE + 0x71)             // 0x60010071，预留
#define  BAK_MEM_PADDING8                 (BAK_MEM_SHM_BASE + 0x72)             // 0x60010072，
#define  BAK_MEM_LPM_NV_WRITE_BUFF        (BAK_MEM_SHM_BASE + 0x74)             // 0x60010074，64个字节长

#define  BAK_MEM_PADDING7                 (BAK_MEM_SHM_BASE + 0xB4)             // 0x600100B4，预留,截至地址为0x60010120
#define  BAK_MEM_PADDING7_LEN              108

//AP一级boot在快速恢复场景下读取，AP大版本赋值，地址不得调整
#define DEEPSLEEP_CP_FASTRECOVERY_FLAG       0x60011FF0        // CP核是否执行了快速恢复的标记，CP的boot会访问
#define POWERUP_AP_JUMP_ADDR                 0x60011FF4        //由于AP_WDT复位不彻底，进而在一级boot中强行跳转执行UTC WDT，执行芯片全局复位
#define DEEPSLEEP_WAKEUP_AP_JUMP_ADDR        0x60011FF8        // SoC Wakeup from Deepsleep
#define DEEPSLEEP_WAKEUP_CP_ACTION           0x60011FFC        // SoC Wakeup from Deepsleep

/* 用户数据RAM缓存区，深睡或软重启维持数据有效,通常用来缓存用户的配置或采集类数据*/
#define  USER_BAK_MEM_BASE                   RESERVED_USER_BAK_MEM_BASE   //0x60011000
#define  USER_BAK_MEM_LEN                    RESERVED_USER_BAK_MEM_LEN    //1112 BYTES


#if(DEBUG_OPENCPU_RUN_TIME == 1 || DEBUG_MODULE_RUN_TIME == 1)
/*复用用户私有空间，保存函数级时间统计信息，具体参见module_runtime_dbg.h或opencpu_runtime_dbg.h*/
#define BAK_MEM_RUN_TIME_STATISTICS             USER_BAK_MEM_BASE    
#endif

#if FLASH_2M
/**************************************FLASH 2M********************************************

             _ _ _ _ _ _ _ __________0x3000_0000
            |           |
            |   0x1000  |  CP boot
            |_ _ _ _ _ _|____________0x3000_1000
            |           |                                 //secondary bootloader prime header
            |   0x3000  |  data for AP 1st boot           //secondary bootloader backup header
            |_ _ _ _ _ _|____________0x3000_4000          //image_info header
            |           |                                 
            |   0x1000  |  security boot header
            |_ _ _ _ _ _|____________0x3000_5000          
            |           |                                 
            |   0x1000  |  FOTA_BREAKPOINT_INFO           //FOTA差分包断点续传和相关状态机信息                     
            |_ _ _ _ _ _|____________0x3000_6000          
            |           |
            |   0x7000  |  AP secondary boot prime
            |_ _ _ _ _ _|____________0x3000_D000
            |           |
            |   0x7000  |  AP secondary boot backup
            |_ _ _ _ _ _|____________0x3001_4000
            |           |
            |   0x2000  |  RF calibration NV              //射频校准NV，升级时不擦除；烧录时根据工具配置是否重新擦写
            |_ _ _ _ _ _|____________0x3001_6000
            |           |
            |   0x1000  |  factory NV                     //原始出厂NV，版本烧录时写入，永不修改；FOTA升级根据需要可重写入
            |_ _ _ _ _ _|____________0x3001_7000
            |           |
            |  0x15D000 |  CP(text/rodata)               //CP版本
            |_ _ _ _ _ _|____________0x3017_4000
            |           |
            |  0x6F000  |  AP(text/rodata/FOTA packet)   //AP版本与FOTA备份区。FOTA差分包保存的首地址决定于AP版本大小；参见BAK_MEM_FOTA_FLASH_BASE
            |_ _ _ _ _ _|____________0x301E_3000
            |           |
            |   0x2000  |  CP NON-volatile NV            //非易变NV,升级时擦除
            |_ _ _ _ _ _|____________0x301E_5000
            |           |
            |   0x1000  |  working factory NV            //工作态的出厂NV，运行时可修改;FOTA升级时，若需要升级原始出厂NV，则会擦除此处；否则不擦除
            |_ _ _ _ _ _|____________0x301E_6000
            |           |
            |   0x1000  |  32k calibration                 //32K校准数据、xy_timer链表、BLE运行态NV
            |_ _ _ _ _ _|____________0x301E_7000
            |           |
            |   0x9000  |  LittleFS Region                //文件系统分区
            |_ _ _ _ _ _|____________0x301F_0000
            |           |
            |   0x10000 |  user NV                       //该区域供用户保存私有数据，FOTA升级时不擦除，不升级
            |_ _ _ _ _ _|____________0x3020_0000


**********************************************************************************************/
#define  FLASH_BASE                             (0x30000000)
#define  FLASH_LENGTH                           (0x200000)

#define  IMAGE_ADDR                             (0x30003000)
#define  IMAGE_INFO_SECTION_LEN                  0x1000

#define  IMAGE_CHECKSUM_ADDR                    (0x30004000)
#define  IMAGE_CHECKSUM_SECTION_LEN              0x1000

#define  NV_FLASH_RF_BASE                       (0x30014000)               //射频校准NV基地址
#define  NV_FLASH_RF_BAKUP_BASE                 (0x30015000)
#define  NV_FLASH_RF_SIZE                       (0x1000)
#define  RF_MT_NV_VERSION                       (0x07)

#define  NV_FLASH_MAIN_FACTORY_BASE             (0x30016000)                //原始出厂NV，版本烧录时写入，永不修改；FOTA升级根据需要可重写入
#define  NV_FLASH_FACTORY_BASE                  (0x301E5000)                //工作态的出厂NV，运行时可修改;FOTA升级时，若需要升级原始出厂NV，则会擦除此处；否则不擦除
#define  NV_FLASH_FACTORY_LEN                   0x1000

#define  ARM_FLASH_BASE_ADDR                    (0x30174000)                // AP核使用的flash开始地址，该空间与FOTA差分包复用，参见BAK_MEM_FOTA_FLASH_BASE
#define  ARM_FLASH_BASE_LEN                     0x6F000                     // AP核中text/data/fota共用的flash总长度

/*死机回写flash的信息内容，与FOTA区域复用。FOTA升级会擦除*/
#define  DUMP_INFO_BASE_ADDR                    (ARM_FLASH_BASE_ADDR + ARM_FLASH_BASE_LEN - 0X8000)  //0x301DB000
#define  DUMP_INFO_BASE_LEN                     0X1000 

/*借用FOTA尾部4K,存放运行的debug信息，如fota、IMEI擦除等时刻点。复用FOTA区域，FOTA升级会擦除*/
#define  RUNINFO_DEBUG_ADDR                     (DUMP_INFO_BASE_ADDR + DUMP_INFO_BASE_LEN)   //0x301DC000
#define  RUNINFO_DEBUG_LEN                      0X1000

/* 模组形态下，若8K retension内存下电，则回写到FOTA区域的尾部6K flash中，内容通常为易变NV。复用FOTA区域，FOTA升级会擦除 */
#define  BAK_MEM_FLASH_TOTAL_LEN                0x6000UL
#define  BAK_MEM_FLASH_BASE                     (RUNINFO_DEBUG_ADDR + RUNINFO_DEBUG_LEN)  //0x301DD000
#define  BAK_MEM_FLASH_LEN                      0x3000UL 
#define  BAK_MEM_FLASH_BASE2                    (BAK_MEM_FLASH_BASE+BAK_MEM_FLASH_LEN)    //0x301E0000
#define  BAK_MEM_FLASH_LEN2                     0x3000UL  


#define  NV_NON_VOLATILE_BASE                   (0x301E3000)
#define  NV_NON_VOLATILE_LEN                    0x2000

#define  CALIB_FREQ_BASE                        (0x301E6000)              //32K校准数据、xy_timer链表、BLE运行态NV
#define  FLASH_AP_TIME_LEN                      128                     //sizeof(Cali_Ftl_t)+128+BLE_NVRAM_LENGTH
#define  CALIB_FREQ_LEN                         0x1000

#define  FS_FLASH_BASE                          (0x301E7000)                 //芯翼文件系统分区
#define  FS_FLASH_LEN                           0x9000


#define  FLASH_SECTOR_LENGTH                    0x1000


/* 该区域供用户保存私有数据，FOTA升级时不擦除；用户不得私自更改该区域大小范围，若有需要，请联系芯翼研发!
   若需要磨损功能及断电异常保护能力，可使用hal_ftl.h相关接口 */
#define  USER_FLASH_BASE                        (0x301F0000)
#define  USER_FLASH_LEN_MAX                     0x10000
#else
/**************************************FLASH 4M********************************************

             _ _ _ _ _ _ _ __________0x3000_0000
            |           |
            |   0x1000  |  CP boot
            |_ _ _ _ _ _|____________0x3000_1000
            |           |                                 //secondary bootloader prime header
            |   0x3000  |  data for AP 1st boot           //secondary bootloader backup header
            |_ _ _ _ _ _|____________0x3000_4000          //image_info header
            |           |                                 
            |   0x1000  |  security boot header
            |_ _ _ _ _ _|____________0x3000_5000       
            |           |                                 
            |   0x1000  |  FOTA_BREAKPOINT_INFO           //FOTA差分包断点续传和相关状态机信息
            |_ _ _ _ _ _|____________0x3000_6000
            |           |
            |   0x7000  |  AP secondary boot prime
            |_ _ _ _ _ _|____________0x3000_D000
            |           |
            |   0x7000  |  AP secondary boot backup
            |_ _ _ _ _ _|____________0x3001_4000
            |           |
            |   0x2000  |  RF calibration NV              //射频校准NV，升级时不擦除；烧录时根据工具配置是否重新擦写
            |_ _ _ _ _ _|____________0x3001_6000
            |           |
            |   0x1000  |  factory NV                     //原始出厂NV，版本烧录时写入，永不修改；FOTA升级根据需要可重写入
            |_ _ _ _ _ _|____________0x3001_7000
            |           |
            |  0x200000 |  CP(text/rodata)               //暂时不知道1级boot校验值的计算方式，等知道后将Reserved合并
            |_ _ _ _ _ _|____________0x3021_7000
            |           |
            |  0x1BC000 |  AP(text/rodata/FOTA packet)   //FOTA差分包保存的首地址决定于AP版本大小；参见BAK_MEM_FOTA_FLASH_BASE
            |_ _ _ _ _ _|____________0x303D_3000
            |           |
            |   0x2000  |  CP NON-volatile NV            //非易变NV,升级时擦除
            |_ _ _ _ _ _|____________0x303D_5000
            |           |
            |   0x1000  |  working factory NV             //工作态的出厂NV，运行时可修改;FOTA升级时，若需要升级原始出厂NV，则会擦除此处；否则不擦除
            |_ _ _ _ _ _|____________0x303D_6000
            |           |
            |   0x1000  |  32k calibration                 //32K校准数据、xy_timer链表、BLE运行态NV
            |_ _ _ _ _ _|____________0x303D_7000
            |           |
            |   0x9000  |  LittleFS Region                //文件系统分区
            |_ _ _ _ _ _|____________0x303E_0000
            |           |
            |   0x20000 |  user NV                       //该区域供用户保存私有数据，FOTA升级时不擦除，不升级
            |_ _ _ _ _ _|____________0x3040_0000
**********************************************************************************************/
#define  FLASH_BASE                             (0x30000000)
#define  FLASH_LENGTH                           (0x400000)

#define  IMAGE_ADDR                             (0x30003000)
#define  IMAGE_INFO_SECTION_LEN                  0x1000

#define  IMAGE_CHECKSUM_ADDR                    (0x30004000)
#define  IMAGE_CHECKSUM_SECTION_LEN              0x1000

#define  NV_FLASH_RF_BASE                       (0x30014000)               //射频校准NV基地址
#define  NV_FLASH_RF_BAKUP_BASE                 (0x30015000)
#define  NV_FLASH_RF_SIZE                       (0x1000)
#define  RF_MT_NV_VERSION                       (0x07)


#define  NV_FLASH_MAIN_FACTORY_BASE             (0x30016000)            //原始出厂NV，版本烧录时写入，永不修改；FOTA升级根据需要可重写入
#define  NV_FLASH_FACTORY_BASE                  (0x303D5000)            //工作态的出厂NV，运行时可修改;FOTA升级时，若需要升级原始出厂NV，则会擦除此处；否则不擦除
#define  NV_FLASH_FACTORY_LEN                    0x1000    


#define  ARM_FLASH_BASE_ADDR                    (0x30217000)            // AP核使用的flash开始地址，该空间与FOTA差分包复用；参见BAK_MEM_FOTA_FLASH_BASE
#define  ARM_FLASH_BASE_LEN                     0x1BC000                // AP核中text/data/fota共用的flash总长度


/*死机回写flash的信息内容，与FOTA区域复用。FOTA升级会擦除*/
#define  DUMP_INFO_BASE_ADDR                    (ARM_FLASH_BASE_ADDR + ARM_FLASH_BASE_LEN - 0X8000)  //0x303CB000
#define  DUMP_INFO_BASE_LEN                     0X1000 

/*借用FOTA尾部4K,存放运行的debug信息，如fota、IMEI擦除等时刻点。复用FOTA区域，FOTA升级会擦除*/
#define  RUNINFO_DEBUG_ADDR                     (DUMP_INFO_BASE_ADDR + DUMP_INFO_BASE_LEN)   //0x303CC000
#define  RUNINFO_DEBUG_LEN                      0X1000

/* 模组形态下，若8K retension内存下电，则回写到FOTA区域的尾部6K flash中，内容通常为易变NV。复用FOTA区域，FOTA升级会擦除 */
#define  BAK_MEM_FLASH_TOTAL_LEN                0x6000UL
#define  BAK_MEM_FLASH_BASE                     (RUNINFO_DEBUG_ADDR + RUNINFO_DEBUG_LEN)  //0x303CD000
#define  BAK_MEM_FLASH_LEN                      0x3000UL 
#define  BAK_MEM_FLASH_BASE2                    (BAK_MEM_FLASH_BASE+BAK_MEM_FLASH_LEN)    //0x303D0000
#define  BAK_MEM_FLASH_LEN2                     0x3000UL 


#define  NV_NON_VOLATILE_BASE                   (0x303D3000)
#define  NV_NON_VOLATILE_LEN                    0x2000

#define  CALIB_FREQ_BASE                        (0x303D6000)            //32K校准数据、xy_timer链表、BLE运行态NV
#define  FLASH_AP_TIME_LEN                      128                     //sizeof(Cali_Ftl_t)+128+BLE_NVRAM_LENGTH
#define  CALIB_FREQ_LEN                         0x1000

#define  FS_FLASH_BASE                          (0x303D7000)              //芯翼文件系统分区
#define  FS_FLASH_LEN                           0x9000

#define  FLASH_SECTOR_LENGTH                    0x1000

/* 该区域供用户保存私有数据，FOTA升级时不擦除；用户不得私自更改该区域大小范围，若有需要，请联系芯翼研发!
   若需要磨损功能及断电异常保护能力，可使用hal_ftl.h相关接口 */
#define  USER_FLASH_BASE                        (0x303E0000)
#define  USER_FLASH_LEN_MAX                      0x20000
#endif
