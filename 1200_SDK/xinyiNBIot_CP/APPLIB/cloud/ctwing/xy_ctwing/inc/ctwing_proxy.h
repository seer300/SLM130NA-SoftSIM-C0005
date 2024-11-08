/**
 * @file ctwing_proxy.h
 * @brief 云抽象AT
 */


#include "xy_utils.h"
#include "xy_system.h"
#include "cloud_proxy.h"




/**
 * @brief 抽象云AT ctwing云配置及注册
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_config_callback 
 */
proxy_config_callback ctlwProxyConfigProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd);


/**
 * @brief 抽象云AT,ctwing数据发送,update及去注册
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_send_callback 
 */
proxy_send_callback ctlwProxySendProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd);


/**
 * @brief ctwing抽象云AT,获取下行缓存数据
 * 
 * @param req_type 
 * @param paramList 
 * @param prsp_cmd 
 * @return proxy_send_callback 
 */
proxy_send_callback ctlwProxyRecvProc(uint8_t req_type,uint8_t* paramList, uint8_t **prsp_cmd);




/**
 * @brief 平台下行数据加入抽象云AT下行数据缓存链表
 * 
 * @param payloadBuf  下行数据
 */
void ctlw_proxy_cloud_buf_add(uint8_t *payloadBuf);



/**
 * @brief 设置抽象云AT标志位
 * 
 * @param flag  1:当前注册由抽象云AT发起及维持  0:清除当前注册由抽象云AT发起及维持标记
 * @return void 
 */
void ctlw_set_abstract_cloud_flag(uint8_t flag);