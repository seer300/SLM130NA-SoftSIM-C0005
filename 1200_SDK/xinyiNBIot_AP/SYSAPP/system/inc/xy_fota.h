/*****************************************************************************************************************************
 *
 * @brief   OPENCPU产品客户的FOTA升级相关，升级成功后会卡住AP核若干秒(WAIT_CLOUD_REPORT)去执行FOTA结果的云上报。
 *
 * @note    重点关注Fota_Proc_Hook_Regist、Pre_Reset_Hook_Regist两个回调注册接口，以及FOTA_SUCC_REPORT编译宏。
 *****************************************************************************************************************************/
#pragma once
#include "hw_types.h"


/**
 * @brief  opencpu形态，升级完成后会启动AP核，AP核在初始化阶段识别为FOTA流程后会启动CP核，以上报云端FOTA升级完成结果。
 *  该宏值为云上报的最大等待时长，一旦超时后，不管是否上报成功，都将继续AP核流程。
 * @warning  由于AP核采取的是超时等待CP核执行云上报，会阻塞AP核正常执行3分钟。如果用户不能接受此时长，或者不希望立即上报FOTA结果到云端，可关闭编译宏FOTA_SUCC_REPORT
*/
#define WAIT_CLOUD_REPORT  (3*60)



/*****************************************************************************************************************************
 * @brief   获取FOTA备份区的起始地址和长度
 * @warning  FOTA备份区是与AP核的版本共用一块FLASH区域，若AP版本增大，会挤压FOTA备份区的大小。
 * @note     由于retension memory、debug等信息在FOTA升级后无意义，进而借用FOTA备份区尾部的0X8000字节大小来保存，FOTA升级时擦除后作为备份区使用。
 ****************************************************************************************************************************/
void xy_OTA_flash_info(uint32_t *addr, uint32_t *len);


/**
 * @brief 该函数用于供用户在处理紧急事件时，查询是否正在执行FOTA，可调用AT命令"AT+FOTACTR"或Do_Fota_onoff取消当前FOTA任务
 * @return ture:表示正在执行FOTA，false:表示未执行FOTA 
 * @note  由于FOTA升级完成还需上报给云平台，进而新版本重启后，AP核在初始化阶段，识别该函数为真时，会自动加载CP核，以让CP核完成FOTA升级流程。
 * @note  整个FOTA流程，耗时达到分钟级别
 **/
bool Is_Fota_Running(void);

/**
 * @brief  供用户在某些特殊事务期间，禁止FOTA升级
 * @return 返回XY_OK表示成功
 * @note   AP核用户一旦禁止CP核执行FOTA，则CP核会放弃当前的FOTA流程。
 **/
int Do_Fota_onoff(int permit);



/**
 * @brief  供AP核用户注册FOTA差分包下载校验期间的回调，如临时关闭用户业务等。该回调函数在AP核核间消息中断中处理，用户需要识别入参来进行操作。
 * @param state 入参为1，表示正在FOTA，CP会擦写FLASH，AP用户需要执行私有动作，防止FOTA期间用户数据丢失。
                入参为0，则表示FOTA失败，用户可以恢复正常业务流程。
 * @warning 整个FOTA流程，耗时达到分钟级别，期间会频繁执行写FLASH，AP核会较长时间阻塞，单次时长约为百毫秒。
 * @note    由于FOTA升级需要重启后进入二级boot执行，进而平台提供了Pre_Reset_Hook_Regist注册接口，在AP核执行FOTA升级重启之前紧急处理私有事务。
 */
void Fota_Proc_Hook_Regist(pFunType_u32 pfun);








