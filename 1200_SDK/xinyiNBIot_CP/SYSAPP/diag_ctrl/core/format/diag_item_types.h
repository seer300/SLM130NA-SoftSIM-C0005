#ifndef DIAG_ITEM_TYPES_H
#define DIAG_ITEM_TYPES_H

/**/
// 打印log的返回状态枚举，用来标识log打印成功还是失败
typedef enum diag_print_state
{
    DIAG_PRINT_SUCCESS = 0,    // log 打印成功
    DIAG_PRINT_FAILED          // log 打印失败
} diag_print_state_t;

// 返回是否允许打印log的状态
typedef enum diag_send_state
{
    DIAG_SEND_ENABLE  = 0,    // 允许发送log
    DIAG_SEND_DISABLE = 1     // 禁止发送log
} diag_send_state_t;


// 处理ps信令的类型，根据不同的类型做不同处理
typedef enum diag_ps_type
{
    RRC_TYPE      = 0,
    NAS_TYPE      = 1,
    ASN1_BER_TYPE = 2
} diag_ps_type_t;


typedef enum
{
    XY_SUBSYS_CAIL_REQ = 1,
    XY_SUBSYS_TEXT_REQ,
    XY_SUBSYS_CAIL_CNF,
    XY_SUBSYS_TEXT_CNF,
    XY_SYSAPP_REQ,
    XY_SYSAPP_CNF,
    XY_SYSSUB_REQ,
    XY_SYSSUB_CNF,
} XY_BLOCK_e;

/*指示有哪些具体的log数据类型*/
typedef enum
{
    XY_STATIC_LOG = 0x01,  //明文打印
    XY_DYNAMIC_LOG,        //明文打印的简化ID方式
    XY_AT_LOG,             //log口进行AT命令传输
    XY_MESSAGE_LOG,        /*PS与phy等模块间标准消息传递的输出log*/
    XY_PROTOCOL_LOG,       /*PS模块内输出结构体内容使用，不能用于模块间消息*/
    XY_SYSAPPREQ_LOG,      //废弃
    XY_SYSAPPCNF_LOG,      //抓包及logview上行信令的应答
    XY_SUBSYSREQ_LOG,      //废弃
    XY_SUBSYSCNF_LOG,      //物理层与PS使用
    XY_MEMORY_LOG,         //导内存专用
    XY_GNSS_LOG,           //GNSS位置信息码流专用，以便logview提取位置信息
    XY_MAX_LOG = 0xF
} XY_TRANSPORT_e;

typedef enum
{
    START_DUMP = 0,
    REV_PCCMD,
} XY_DUMP_FLAG;


// 发送给PC侧的命令
typedef enum
{
    XY_SUBAPPCNF_BASE = XY_SYSAPP_CNF << 16,
    XY_SYSAPP_DETECT_CNF,
    XY_SYSAPP_READNV_CNF,
    XY_SYSAPP_WRITENV_CNF,
    XY_SYSAPP_MEMREADY_CNF,
    XY_SYSAPP_MEMHAVE_IND,
    XY_SYSAPP_FILTER_CNF,
    XY_SYSAPP_MAXLEN_CNF,
    XY_SYSAPP_WIRESHARK_IND,
    XY_SYSAPP_HEART_CNF,
    XY_SYSAPP_FORCEDL_IND,
    XY_SYSAPP_FORCEDL_CNF,
	XY_SYSAPP_MEMDONE_IND,
} XY_SUBAPP_CNF;


// 从PC侧接收到的命令处理
enum
{
    XY_SUBAPPREQ_BASE = XY_SYSAPP_REQ << 16,
    XY_SYSAPP_DETECT_REQ,
    XY_SYSAPP_READNV_REQ,
    XY_SYSAPP_WRITENV_REQ,
    XY_SYSAPP_MEMREADY_REQ,
    XY_SYSAPP_ASSERT_REQ,
    XY_SYSAPP_FILTER_REQ,
    XY_SYSAPP_MAXLEN_REQ,
    XY_SYSAPP_HEART_REQ,
    XY_SYSAPP_FORCEDL_REQ,
	XY_SYSAPP_MEMDUMP_REQ,
};


typedef enum
{
    DEBUG_LOG,
    INFO_LOG,
    WARN_LOG,
    FATAL_LOG,
    CRITICAL_LOG,
    PREALLOC_LOG,
    MAX_LEV_LOG
} XY_LOG_LEV;

/*!!!新增的ID必须与NB系列保持对齐，不得冲突!!!*/
typedef enum
{
    LAYER2        = 0,      //废弃
    NAS           = 1,
    ADMIN         = 2,      //废弃
    LPHY          = 3,
    LRRC          = 4,
    AT_UART       = 5,      //废弃
    LNBPS         = 6,
    PCTOOL        = 7,      //废弃
    L1C           = 8,      //物理层使用
    WIRESHARCK    = 9,      //废弃
    PLATFORM      = 10,     //基础平台
    XYAPP         = 11,     //APPLIB中的业务模块
    WIRESHARCK_AP = 12,     //废弃
    AP_CORE_LOG   = 13,     //AP核的log
    ATCTRL_AP     = 14,     //废弃
    USER_LOG      = 15,     //CP核用户通过user_printf输出的log
    BIP           = 16,     //废弃
    ATC_AP        = 17,     //PS开发给客户部分的log输出


    WIRESHARK_M3    = 21,  //暂未使用

    // AP侧的src_id
    WIRESHARK_AP2   = 22,  //暂未使用
    XYAPP2          = 23,  //暂未使用
    PLATFORM_AP     = 24,  //暂未使用
    GNSS_LOG        = 25,  //用于抓取AP核的GNSS位置区码流log
    USER_LOG2       = 26,  //暂未使用
    ATC_AP_T        = 27,  //暂未使用
    XYLOG_MAX_BLOCK,
} XY_SRC_E;


#endif  /* DIAG_ITEM_TYPES_H */
