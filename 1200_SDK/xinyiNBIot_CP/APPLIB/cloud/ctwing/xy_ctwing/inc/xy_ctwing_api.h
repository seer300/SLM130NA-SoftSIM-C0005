/**
 * @file xy_ctwing_api.h
 * @brief Ctwing API
 * 
 */

typedef enum
{
    CTLW_API_REG_SUCCESS_SEM,
    CTLW_API_UPDATE_SUCCESS_SEM,
    CTLW_API_DATA_SENT_SUCCESS_SEM,
    CTLW_API_DEREG_SUCCESS_SEM,
}ctlw_api_sem_type_e;//api信号量类型


/**
 * @brief 设置云连接参数
 * 
 * @param server_ip 合法IP字符串，支持IPV4，IPV6，点分十进制，例"221.229.214.202"
 * @param server_port 端口，合法范围:大于0,小于65535
 * @param lifetime 合法范围:大于等于300，小于等于30*86400
 * @param auth_mode 终端认证协议,ctiot_id_auth_mode_e类型,建议入参AUTHMODE_SIMPLIFIED
 * @return int 
 */
int32_t ctlw_cloud_setting(uint8_t *server_ip, int32_t server_port, int32_t lifetime, int32_t auth_mode);


/**
 * @brief 发送数据到AEP平台
 * @param data 待发送数据
 * @param sendMode  SENDMODE_NON:NON data ,SENDMODE_CON:CON data
 * @param timeout 等待时长,单位:秒
 * @return int32_t CTIOT_NB_SUCCESS:成功    CTIOT_NB_FAILED:失败
 * @note 同步接口
 */
int32_t ctlw_cloud_send_data(uint8_t *data, ctiot_send_mode_e sendMode, int32_t timeout);


/**
 * @brief 向AEP平台发起注册,注册成功后可进行数据收发，注册失败则处于未登录状态
 * 
 * @param timeout 等待时长,单位:秒
 * @return int32_t CTIOT_NB_SUCCESS:成功    CTIOT_NB_FAILED:失败
 * @note  同步接口
 */
int32_t ctlw_cloud_register(int32_t timeout);



/**
 * @brief 向AEP云平台发起去注册请求，去注册完成后本地处于未登录状态
 * 
 * @param timeout 等待时长,单位:秒
 * @return int32_t CTIOT_NB_SUCCESS:成功    CTIOT_NB_FAILED:失败
 * @note  同步接口
 */
int32_t ctlw_cloud_deregister(int32_t timeout);



/**
 * @brief 根据对应notify事件，执行api接口相关处理流程
 * 
 * @param notifyType 
 * @param subType 
 * @param value 
 * @param data1 
 */
void ctlw_notify_api_event_process(uint8_t notifyType, uint8_t subType, uint16_t value, uint32_t data1);



/**
 * @brief api信号量释放
 * 
 * @param sem_type   ctlw_api_sem_type_e类型
 */
void ctlw_cloud_api_sem_give(ctlw_api_sem_type_e sem_type);


