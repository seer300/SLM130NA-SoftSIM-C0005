
#pragma once

#include <stdint.h>
#include "basic_config.h"
#include "user_config.h"

typedef enum
{
	LOCAT_NULL = 0,
	LOCAT_FLASH,
	LOCAT_BKMEM,
}Sending_LocatDef;
#if SAVE_IN_FLASH
/**
 * @brief bkmem用于保存用户数据的内存空间，该内容在HAL_UTC_WAKEUP、HAL_EXTPIN_WAKEUP唤醒时保持不变，其他上电情况下被初始化为0
 * @note  bkmem即retention memory内存深睡时不断电，以供保存特殊数据使用
 */
/*retention memory空间中用于保存用户私有采集数据的总字节数*/
#define BKMEM_USABLE_SAVE_SIZE       (USER_BAK_MEM_LEN-sizeof(User_Control_Block_T))
/*retention memory空间中用于保存用户私有采集数据的首地址*/
#define BKMEM_STORE_DATA_ADDR        (USER_BAK_MEM_BASE+sizeof(User_Control_Block_T))

/*用户数据管理信息+用户容错等私有信息，软重启或深睡唤醒后仍有效；用户根据自己需要自行变更该结构体参数*/
typedef struct
{
	uint8_t  softreset_num;          //用户触发的软重启次数，供容错用
    uint16_t flash_saved_size;       //flash有效数据长度
    uint8_t  padding;
	
	uint32_t bkmem_saved_size;        //bkmem有效数据长度，最大不能超过BKMEM_USABLE_SAVE_SZIE
	
	uint32_t flash_write_pos;        //flash中待写入首地址point_offset
	uint32_t flash_read_pos;         //flash中待发送的首地址point_offset，随着g_sending_len更新

	user_config_t user_config;        //用户配置信息，保存在flash，非唤醒初始化时读到ret mem
}User_Control_Block_T;

/*用户数据管理结构体+用户容错等私有信息，软重启后仍有效*/
extern User_Control_Block_T *g_Control_Context;

/*正在通过CP核进行云通信的数据长度，待收到AT应答后，变更bkmem_headdata->flash_send_addr值*/
extern uint32_t g_sending_len;

/**
 * @brief 将bkmem中的数据保存到flash中。由于该接口运行在main函数中，而数据采集外部中断还会继续，需要考虑中断切换的影响
 */
void Save_Bkmem2Flash(void);

/**
 * @brief 获取本地待发送的数据总长度
 */
uint32_t  Get_Send_Data_Len(void);

/**
 * @brief 对采集数据保存到bkmem中，若达到阈值，则触发main主线程执行回写flash动作
 * @warning  由于产品差异，需要用户自行修改数据采集的接口及临界条件
 */
void Save_Data_2_BakMem(void *gather_data,uint32_t gather_len);
#endif

/**
 * @brief 根据云发送结果，更新BKmem_header里的状态值，需要关注发送失败时不能清空
 * @param send_succ: 1表示成功,0表示失败
 */
void Update_Info_By_Send_Result(uint32_t send_succ);

/*供用户定制自己的离散发送数据源，最终只需把地址和长度返回给调用者即可。若待发送的数据尚未准备好，云发送AT流程会一直空跑下去。若用户确定所有待发送的数据源都已发送完成，内部执行释放EVENT_CLOUD_SEND事件。*/
uint32_t Get_Data_To_Send(uint32_t max_len, void **addr, uint32_t *len);
