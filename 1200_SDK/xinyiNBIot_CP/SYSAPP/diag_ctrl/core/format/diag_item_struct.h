#ifndef DIAG_ITEM_STRUCT_H
#define DIAG_ITEM_STRUCT_H

#include "diag_options.h"
#include <stdint.h>

#define HEADERSIZE    sizeof(uint32_t)
#define LENSIZE       sizeof(uint16_t)


// 当使能CRC时，尾部占用2字节，1字节CRC校验值和1字节尾0xFA
// 当禁能CRC时，尾部占用1字节，仅1字节尾0xFA
#if (DIAG_CALCULATE_CRC_ENABLE == 1)
#define DIAG_STRUCT_TAIL_LEN    2
#else
#define DIAG_STRUCT_TAIL_LEN    1
#endif


/*所有log都必须有的头部信息，logview解析时需要*/
typedef struct
{
    uint32_t u32Header:28;      // 0x5AA5FEEF
    uint32_t uChipType:4;       // ChipTye_E 类型的枚举
    uint16_t u16Len;            // 此成员之后所有数据的长度，不包含log尾部的crc校验以及尾部标识0xFA
    uint16_t u16SeqNum;
    uint32_t u4TraceId:4;       // XY_TRANSPORT_e 类型的枚举
    uint32_t u28ClassId:28;     // 动态log的动态id，静态log为0
    uint32_t u32Time;
    uint8_t u8Payload[0];
}ItemHeader_t;    //sizeof = 16


typedef struct
{
    ItemHeader_t itemHeader;
    uint8_t u8SrcId;            // XY_SRC_E 类型的枚举
    uint8_t u8MsgLev;           // XY_LOG_LEV 类型的枚举
    uint8_t u8ParamSize;        // 格式化参数的长度
    uint8_t u8CoreType;         // CoreType_E 类型的枚举
    uint16_t u16MsgSize;        // 消息的长度，包含格式化参数的长度和格式化字符串的长度
    uint16_t u16Pad;
    uint8_t u8Payload[0];
} StaticLog_t;    //sizeof = 8


typedef struct
{
    ItemHeader_t itemHeader;
    uint8_t u8SrcId;            // XY_SRC_E 类型的枚举
    uint8_t u8MsgLev;           // XY_LOG_LEV 类型的枚举
    uint16_t pad;
    uint16_t u16DynId;          // 动态log的动态id
    uint16_t u16MsgSize;        // 消息的长度，由于动态log的消息只包含格式化参数，所以也是格式化参数的长度
    uint8_t u8Payload[0];
} DynamicLog_t;

/*logview进行AT命令应答的头信息*/
typedef struct
{
    ItemHeader_t itemHeader;
    uint16_t u16Len;
    uint16_t u16Pad;//
    uint8_t u8Payload[0];//AT
} ATCmd_t; //sizeof 4


/*PS与phy等模块间标准消息传递的输出log*/
typedef struct
{
    ItemHeader_t itemHeader;
    //uint16_t u16Len;
    //uint8_t u8SrcId;
    //uint8_t u8DstId;
    //uint32_t u32MsgClass;
    //uint32_t u32MsgId;
    uint8_t u8Payload[0];
} Message_t;//sizeof 4


/*PS结构体log的统一头部信息*/
typedef struct
{
    ItemHeader_t itemHeader;
    uint8_t u8Type;
    uint8_t u8Ver;
    uint16_t u16Len;
    uint8_t u8Payload[0];
} Protocol_t;    //sizeof 8


typedef struct
{
    uint8_t u8Direction;
    uint8_t pad;
    uint16_t u16Len;
    uint8_t u8Payload[0];
} NasHeader_t;


typedef struct
{
    uint8_t u8MsgId;
    uint8_t pad;
    uint16_t u16Len;
    uint8_t u8Payload[0];//
} RrcHeader_t;


typedef struct
{
    uint16_t u8Seq:15;
    uint16_t u8PEnd:1;
    uint16_t u8Len;
    uint8_t u8Payload[0];
} MemInfo_t;


typedef struct
{
    uint8_t u8Payload[0];
} CommonReq_t;


typedef struct
{
    uint8_t type;
    uint8_t pad;
    uint16_t len;
    uint8_t u8Payload[0];
} WireShark_t;


typedef struct
{
    ItemHeader_t itemHeader;
    uint8_t u8SrcId;
    uint8_t pad;
    uint16_t u16Len;
    uint8_t u8Payload[0];
} CommonCnf_t;


typedef struct
{
    uint32_t freq;
    uint32_t power;
} APC_Control_Id_t;


typedef struct
{
    uint16_t log_lev_bitmap;  //log等级过滤，位图方式表示，XY_LOG_LEV
    uint16_t sig_pri_bitmap;  //PS相关log过滤的位图表示，bit0：结构体log开关；bit1：消息原语log开关；
    uint32_t src_id_bitmap;   //字符串明文输出，根据模块ID进行过滤，XY_SRC_E
} filterInfo;


typedef  CommonReq_t SysAppReq_t;
typedef  CommonReq_t SubSysReq_t;
typedef  CommonCnf_t SysAppCnf_t;
typedef  CommonCnf_t SubSysCnf_t;


typedef enum
{
    Chip_1100 = 0xEF,
    Chip_1100E= 0xEE,
} ChipTye_E;


typedef enum
{
    Core_CP = 0,
    Core_AP = 1
} CoreType_E;


#endif  /* DIAG_ITEM_STRUCT_H */
