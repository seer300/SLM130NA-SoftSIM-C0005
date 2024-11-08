#pragma once
#include <stdint.h>

/**
  * @brief  用于获取4字节的随机数种子，为硬件产生的真随机数
  * 获取随机数时，按照以下操作即可：
  * srand(xy_seed());
  * ret = rand();
  * @return 随机数种子
 */
uint32_t xy_seed(void);

/**
  * @brief  获取min到max的随机值
 */
uint32_t get_rand_val(uint32_t min,uint32_t max);

/**
 * @brief 数据校验
 * @param dataptr:数据起始地址；len:数据长度
 * @return 校验的结果
 */
uint32_t xy_chksum(const void *dataptr, int32_t len);


/**
 * @brief BCD码转化成十进制
 * @param bcd:BCD码
 * @return bcd码转化后的十进制数
 */
uint8_t BCD2DEC(uint8_t bcd);

/**
 * @brief 十进制转化成BCD码
 * @param bcd:十进制数
 * @return 十进制数转化后的BCD码
 */
uint8_t DEC2BCD(uint8_t dec); 


/*
 * @brief   将数字转换成ascii码字符串，例如921600-->"921600"
 * @param   big_endian为1时，表示大端在后，例如921600-->"006129"
 */
uint8_t int_to_ascii(int num, char *str, char big_endian);

/**
  * @brief   字符串倒序函数
  */
void str_back_order(char *rst,int rstlen);


/*
*  @brief 软看门狗，深睡后自动无效。超时后会触发芯片硬复位，深睡唤醒后需在User_Init_FastRecovery中重新初始化。
*  @note  硬看门狗解决的是软硬件异常卡死不运行，软看门狗解决的是软硬件能正常运行，但软件流程异常造成的无法进入预期的深睡状态
*  @note  OPEN形态用户设置的看门狗时长建议值由小到大分别为：AP硬看门狗时长(秒级) < UTC全局看门狗时长(分钟级) < 软看门狗时长(小时级)
*  @warning 考虑到FOTA等耗时较长的流程，进而入参时长必须大于30分钟，否则会断言。
*/

void Soft_Watchdog_Init(uint32_t sec);


