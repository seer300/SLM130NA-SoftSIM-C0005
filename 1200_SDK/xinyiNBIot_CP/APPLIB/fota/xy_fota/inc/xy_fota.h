/** 
* @file        xy_fota.h
* @brief       芯翼自研的FOTA算法的用户二次开发接口，主要为差分包的下载和校验相关
* @attention   芯翼自研的FOTA方案仅供参考
*/
#pragma once

#include "xy_utils.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/



typedef enum
{
    XY_FOTA_IDLE = 0,         //升级尚未执行或者已复位
    XY_FOTA_DOWNLOADING,      //数据包下载中，频繁擦写FLASH
    XY_FOTA_DOWNLOADED,       //所有数据包下载完成且校验通过，有一次擦写FLASH
    XY_FOTA_UPGRADING,        //准备进行重启升级，在二级boot中执行差分升级
    XY_FOTA_UPGRADE_SUCCESS,  //二级boot升级成功后，启动新版本后，设置该状态，以供云上报。有一次擦写FLASH
    XY_FOTA_UPGRADE_FAIL,     //二级boot升级失败，在大版本中设置该状态，以供云上报。有一次擦写FLASH
    XY_FOTA_STATE_MAX
} XY_OTA_STAT_E;

/**
* @brief FOTA运行过程中的信息，用于异常掉电后恢复
*/
typedef struct
{
	/*断点续传数据结构体*/
    uint32_t flash_base;
    uint32_t flash_maxlen;
    uint32_t recv_size;
	uint32_t ota_stat;
} Fota_State_Info_T;



/**
 * @brief  获取当前OTA的状态
 */
XY_OTA_STAT_E OTA_get_state();


/**
 * @brief OTA的(去)初始化
 * @note  断电续升时，不需要调用OTA_upgrade_init，直接调用OTA_save_packet即可
 * @warning 云模块在FOTA差分包下载流程中因故放弃升级，必须调用该接口去初始化，否则会造成状态机处于中间态，误导AP核用户认为一直在FOTA
 */
void OTA_upgrade_init();


/**
 * @brief 将获取到的单个差分包数据块，保存到本地缓存，如flash。该接口会被调用多次
 * @param data [IN] 差分包数据块
 * @param size [IN] 单个数据块长度
 * @return XY_ERR: 写入失败; XY_OK: 写入成功
 * @note  断电续升时，不需要调用OTA_upgrade_init，直接该接口即可
 * @warning  当用户有紧急事件处理，通过OTA_set_permit取消升级时，该接口可能返回XY_ERR
 */
int OTA_save_one_packet(char* data, uint32_t size);


/**
 * @brief   待差分包完全下载完毕后，通过该接口进行差分包校验
 * @return  XY_ERR: 校验失败; XY_OK: 校验成功
 * @warning 芯翼差分算法会执行差分包完整性和来源性校验及image版本校验。先对差分包数据做RSA+SHA校验，再对参与FOTA升级的image做版本检验
 */
int OTA_delta_check();


/**
 * @brief 更新二级boot的升级信息，重启开始差分升级 
 * @return XY_ERR:取消升级,否则正常重启升级
 * @note  芯翼的差分升级是在二级boot中执行，调用该接口后会自行进行重启升级。
 */
int OTA_upgrade_start();


/**
 * @brief  二级boot升级完成后，进入大版本。部分云业务需要通过该接口查询升级结果并上报，调用之前需云模块自行识别当前是在FOTA流程中。
 * @return XY_ERR:升级失败; XY_OK:升级成功;
 * @note   该接口指示升级最终结果,可供部分云业务用户上报云端升级结果，但不会清空升级状态。需要云模块自行保证不会多次读取上报结果。
 */
int OTA_get_upgrade_result();



/**
 * @brief   识别当前是否正在进行FOTA流程，包括升级结果的云上报过程
 * @warning 用户不得轻易使用！如需获取更细节状态，使用OTA_get_state()
 */
bool OTA_is_doing();

/**
 * @brief OTA升级的控制，对于需紧急处理的突发事件，用户可以调用该接口取消fota升级
 */
void OTA_set_permit(int permit);


/**
 * @brief 获取FOTA升级区域FLASH起始地址及FOTA升级可用的flash空间大小
 * @param addr[OUT]: FLASH起始地址
 * @param len [OUT]: FLASH空间大小
 * @warning  FOTA备份区与AP大版本复用一块FLASH区域，AP版本增大，会挤压FOTA备份区的大小
 * @note     由于retention memory/debug等信息在FOTA升级后无意义，进而放在FOTA备份区尾部占用0X8000字节大小来保存，FOTA升级是擦除后作为备份区使用。
 */
void xy_OTA_flash_info(uint32_t *addr, uint32_t *len);


/**
 * @brief 自研NFWUPD AT命令
 */
int at_NFWUPD_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 自研AT命令,用于取消fota升级
 */
int at_FOTACTR_req(char *at_buf, char **prsp_cmd);



/**
 * @brief    仅限内部使用！升级完成后，由后台调用该接口删除二级boot中升级的信息，并更新最终升级结果
 */
void OTA_update_upgrade_result();

