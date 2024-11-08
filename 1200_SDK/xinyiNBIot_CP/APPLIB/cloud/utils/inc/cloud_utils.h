#ifndef _CLOUD_UTILS__H
#define _CLOUD_UTILS__H
#define MUTEX_LOCK_INFINITY 0xFFFFFFFF

#include "xy_net_api.h"
#include "xy_fota.h"

/*入库NV ucStorageOperType 0:标准版本，非0：入库版本*/
extern factory_nv_t *ptPsFactory_Nv;
#define CHECK_SDK_TYPE(cust_type)   (ptPsFactory_Nv->tNvData.tNasNv.ucStorageOperType & (1 << cust_type))

typedef enum {
    CDP_IP_TYPE = 0U,
    ONENET_IP_TYPE ,
    SOCKET_IP_TYPE,
    CTWING_IP_TYPE,
    CLOUD_IP_TYPE_MAX,
}cloud_ip_type_e;

typedef struct _net_infos_t{
    uint16_t  local_port;
    uint16_t  remote_port;
    ip_addr_t remote_ip;
    ip_addr_t local_ip;
	char is_dm;
}net_infos_t;




/**
 * @brief  仅限于仅读一次场景，连续读不得使用！对open->seek->read->close的文件读取的接口封装
 * @param  fileName [IN] 文件名字符串
 * @param  offset   [IN] 读取的文件内偏移字节
 * @param  buf      [OUT] 读取后内存存放缓存
 * @param  size     [IN] 读取的长度
 * @return -1表示失败，0表示未读到有效信息，正值表示读取的有效长度
 * @note   接口内部open的方式默认为"rb"，如不满足用户需求，请自行调用xy_fs.h中API
 */
int app_read_fs(const char * fileName,uint32_t offset,void * buf, uint32_t size);


/**
 * @brief  仅限于仅写一次场景，连续写不得使用！对open->seek->write->close的写文件的接口封装
 * @param  fileName [IN] 文件名字符串
 * @param  buf      [OUT] 读取后内存存放缓存
 * @param  size     [IN] 读取的长度
 * @return XY_ERR表示失败，XY_OK表示成功
 * @note   用户根据mode取值不同，来决定文件的操作权限，再根据返回值来判断成功与否。
 */
int app_write_fs(const char * fileName,void *buf,uint32_t size);


uint64_t cloud_gettime_ms(void);
unsigned int cloud_get_ResveredMem();
int cloud_mutex_create(osMutexId_t *pMutexId);
int cloud_mutex_destroy(osMutexId_t *pMutexId);
int cloud_mutex_lock(osMutexId_t *pMutexId, uint32_t timeout);
int cloud_mutex_unlock(osMutexId_t *pMutexId);

/**
 * @brief 网络业务初始化处理函数
 */
void cloud_init(void);


#endif /* _CLOUD_UTILS__H */
