#ifndef XY_LOG_H
#define XY_LOG_H

#include "diag_print.h"
#include "diag_transmit_port.h"
#include "diag_item_types.h"
#include "diag_msg_type.h"
#include "diag_at_cmd.h"
#include "diag_cmd_send.h"
#include "diag_filter.h"
#include "diag_config.h"
// 由于协议栈还和log有耦合，这里还需要加入把部分头文件对外
#include "diag_item_struct.h"
#include "diag_mem.h"
#include "diag_packet.h"
#include "web_log.h"
// log 输出的初始化
#if(DIAG_PHY_MEMPOOL_USE)
#define diag_communication_init()    do{diag_port_send_init();diag_phy_mempool_init();}while(0)
#else
#define diag_communication_init()    diag_port_send_init()
#endif

// 动态log，格式话操作在工具侧完成。使用该函数打印的log，需要更新loginfo文件
// dyn_id: 由脚本自动替换，初始要填成0
// src_id: 详见 diag_item_types.h 中的 XY_SRC_E 枚举类型
// lev:    详见 diag_item_types.h 中的 XY_LOG_LEV 枚举类型
// 补充说明:可变参数固定为%d类型时，耗时为:
// 参数个数为0:     耗时范围:3960-6020ticks  平均:5050ticks
// 参数个数为1:     耗时范围:4560-6870ticks 平均:5830ticks
// 参数个数为2:     耗时范围:4320-6650ticks 平均:5690ticks
// 参数个数为3:     耗时范围:4360-6990ticks 平均:5910ticks
// 参数个数为4:     耗时范围:5070-7620ticks 平均:6560ticks
// 参数个数为5:     耗时范围:5520-8060ticks 平均:7070ticks
// 参数个数为6:     耗时范围:5520-8140ticks 平均:7100ticks
// 参数个数为7:     耗时范围:5570-8290ticks 平均:7240ticks
// 参数个数为8:     耗时范围:5740-8760ticks 平均:7740ticks
// 参数个数为9:     耗时范围:6050-9240ticks 平均:8200ticks
// 参数个数为10:    耗时范围:6510-8840ticks 平均:7860ticks
// 参数个数为11:    耗时范围:7000-9540ticks 平均:8270ticks
// 参数个数为12:    耗时范围:8320-10230ticks 平均:9360ticks
// 参数个数为13:    耗时范围:8180-10670ticks 平均:9770ticks
// 参数个数为14:    耗时范围:8450-10620ticks 平均:9710ticks
// 参数个数为15:    耗时范围:8430-10860ticks 平均:9980ticks
// 参数个数为16:    耗时范围:8420-11150ticks 平均:10100ticks
#define PrintLog(dyn_id, src_id, lev, fmt, ...) \
do{ \
    diag_platform_dynamic_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, fmt, ##__VA_ARGS__); \
    log_printf_save_buff(dyn_id, (XY_SRC_E)src_id, fmt, ##__VA_ARGS__); \
}while(0)

// 静态log，格式话操作在工具侧完成。使用该函数打印的log，不需要更新loginfo文件
// src_id: 详见 diag_item_types.h 中的 XY_SRC_E 枚举类型
// lev:    详见 diag_item_types.h 中的 XY_LOG_LEV 枚举类型
#define PrintUserLog(src_id, lev, fmt, ...)        diag_platform_static_log(XY_STATIC_LOG,(XY_SRC_E)src_id, (XY_LOG_LEV)lev, fmt, ##__VA_ARGS__)
#define PrintLogSt(src_id, lev, fmt, ...)          diag_platform_static_log(XY_STATIC_LOG,(XY_SRC_E)src_id, (XY_LOG_LEV)lev, fmt, ##__VA_ARGS__)

/*GNSS特殊信令的抓取解析，暂未使用*/
#define PrintGnssLog(lev, fmt, ...)        diag_platform_static_log(XY_GNSS_LOG,GNSS_LOG, (XY_LOG_LEV)lev, fmt, ##__VA_ARGS__)


// 物理层指定变参个数的log，本质上也是动态log，格式化操作在工具侧完成。使用该函数打印的log，需要更新loginfo文件
// 变参从0个到16个都使用宏替换的方式，理论可以支持无限多个，这里只支持16个，如需要更多，按照格式增加即可，同时需要更改python脚本 loginfo.py 中需要匹配的函数
// dyn_id: 由脚本自动替换，初始要填成0
// src_id: 详见 diag_item_types.h 中的 XY_SRC_E 枚举类型
// lev:    详见 diag_item_types.h 中的 XY_LOG_LEV 枚举类型
// 0个参数 耗时范围:2670-2930ticks 平均:2820ticks
#define PhyPrintf0(dyn_id, src_id, lev, fmt) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 0, fmt)
// 1个参数 耗时范围:2940-3200ticks 平均:3130ticks
#define PhyPrintf1(dyn_id, src_id, lev, fmt, v1) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 1, fmt, v1)
// 2个参数 耗时范围:3320-3580ticks 平均:3470ticks
#define PhyPrintf2(dyn_id, src_id, lev, fmt, v1, v2) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 2, fmt, v1, v2)
// 3个参数 耗时范围:3340-3620ticks 平均:3500ticks
#define PhyPrintf3(dyn_id, src_id, lev, fmt, v1, v2, v3) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 3, fmt, v1, v2, v3)
// 4个参数 耗时范围:3350-3600ticks 平均:3490ticks
#define PhyPrintf4(dyn_id, src_id, lev, fmt, v1, v2, v3, v4) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 4, fmt, v1, v2, v3, v4)
// 5个参数 耗时范围:3550-3950ticks 平均:3780ticks
#define PhyPrintf5(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 5, fmt, v1, v2, v3, v4, v5)
// 6个参数 耗时范围:3340-3880ticks 平均:3620ticks
#define PhyPrintf6(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 6, fmt, v1, v2, v3, v4, v5, v6)
// 7个参数 耗时范围:3750-4220ticks 平均:4020ticks
#define PhyPrintf7(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 7, fmt, v1, v2, v3, v4, v5, v6, v7)
// 8个参数 耗时范围:3620-4050ticks 平均:3880ticks
#define PhyPrintf8(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 8, fmt, v1, v2, v3, v4, v5, v6, v7, v8)
// 9个参数 耗时范围:4260-4420ticks 平均:4320ticks
#define PhyPrintf9(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 9, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9)
// 10个参数 耗时范围:3580-4120ticks 平均:3870ticks
#define PhyPrintf10(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 10, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10)
// 11个参数 耗时范围:3780-4180ticks 平均:3930ticks
#define PhyPrintf11(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 11, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
// 12个参数 耗时范围:4340-4630ticks 平均:4450ticks
#define PhyPrintf12(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 12, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
// 13个参数 耗时范围:4270-4660ticks 平均:4510ticks
#define PhyPrintf13(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 13, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
// 14个参数 耗时范围:4360-4930ticks 平均:4670ticks
#define PhyPrintf14(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 14, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
// 15个参数 耗时范围:4340-4830ticks 平均:4610ticks
#define PhyPrintf15(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 15, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
// 16个参数 耗时范围:4530-5020ticks 平均:4790ticks
#define PhyPrintf16(dyn_id, src_id, lev, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) \
    diag_phy_fixed_arguments_log(dyn_id, (XY_SRC_E)src_id, (XY_LOG_LEV)lev, 16, fmt, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)

// 物理层数据打印
#define PhyRawPring(pMsg, usLen, usHSN, usSFN, ucSubf, ucSN, ucDataType) \
                                                        diag_phy_primitive_log(pMsg, usLen, usHSN, usSFN, ucSubf, ucSN, ucDataType)

// 协议栈的 NAS 信令log，不依赖loginfo文件
#define OutputOtaNasMsg(size, msg, direction)           diag_ps_signalling_log(direction, (char *)msg, size, NAS_TYPE)

// 协议栈的 RRC 信令log，不依赖loginfo文件
#define OutputOtaRrcMsg(msg_id, msg, size)              diag_ps_signalling_log(msg_id, (char *)msg, size, RRC_TYPE)

// 协议栈的 Asn1_Ber 信令log，不依赖loginfo文件
#define OutputAsn1BerMsg(msg_id, msg, size)             diag_ps_signalling_log(msg_id, (char *)msg, size, ASN1_BER_TYPE)

// 协议栈的原语log，不依赖loginfo文件
#define print_primitive_ram(msg)                        diag_ps_primitive_log((char *)msg)

// 协议栈的 wireshark log，不依赖loginfo文件
#define diag_wireshark_dataAP(data, len, type, tick)    diag_ps_wireshark_log(data, len, type)


/*待废弃，直接使用diag_filter_get_send_enable_state*/
#define is_printDsp_log()    diag_filter_get_send_enable_state(LRRC)
#define LOG_TRUE             DIAG_SEND_ENABLE


#endif  /* XY_LOG_H */
