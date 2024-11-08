/*
 * net_app_mem.h
 *
 *  Created on: 2024年9月26日
 *      Author: luxq
 */

#ifndef _NET_APP_MEM_H_
#define _NET_APP_MEM_H_


/*同时支持UDP和公有云(CDP和ONENET二选一)的保存恢复*/
#define CLOUD_BAKUP_LEN         1900    //按业务所需最大申请：(需要保存到flash的数据长度 + sizeof(cdp_session_info_t) + sizeof(cdp_fota_info_t) + sizeof(socket_udp_info_t))  188+1144+288+280
#define CLOUD_CFG_LEN           180     //取配置文件中最大的长度(sizeof(onenet_config_nvm_t)  180)
#define CLOUD_CHKSUM_LEN        4       //checksum 长度
#define CLOUD_BAKUP_HEAD        (CLOUD_CHKSUM_LEN + 4)  //check_sum + 文件有效标记位(4+4)
#define CLOUD_FLASH_SAVE_LEN    (CLOUD_BAKUP_HEAD + CLOUD_CFG_LEN) // 需要保存到flash的数据长度 (check_sum + flag + 配置文件长度)  188


extern void *g_cloud_mem_p;

#endif /* _NET_APP_MEM_H_ */
