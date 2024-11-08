/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_utils.h"
#include "xy_system.h"
#include "xy_utils.h"
#include "cloud_utils.h"
#include "xy_ps_api.h"
#include "xy_net_api.h"
#include "qspi_flash.h"
#include "low_power.h"
#include "oss_nv.h"
#include "lwip/ip_addr.h"
#include "lwip/tcpip.h"
#include "net_app_resume.h"
#if AT_SOCKET
#include "at_socket_context.h"
#endif

#include "xy_fota.h"
#include "xy_fs.h"


#if WITH_MBEDTLS_SUPPORT
#include "mbedtls_init.h"
#endif

#if TELECOM_VER
extern void cdp_netif_event_callback(PsStateChangeEvent event);
extern void cdp_module_mutex_init();
#endif


#if MOBILE_VER
extern void cis_netif_event_callback(PsStateChangeEvent event);
extern void cis_module_mutex_init();
#endif

#if CTWING_VER
extern void ctlw_netif_event_callback(PsStateChangeEvent event);
extern void ctlw_module_mutex_init();
#endif

/*******************************************************************************
 *							   Macro definitions							   *
 ******************************************************************************/

/*******************************************************************************
 *							   Type definitions 							   *
 ******************************************************************************/

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/

/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/
/*******************************************************************************
 *						  Global variable definitions						   *
 ******************************************************************************/

/*******************************************************************************
 *						Inline function implementations 					   *
 ******************************************************************************/

/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
uint64_t cloud_gettime_ms(void)
{
    return get_utc_ms();
}

uint32_t cloud_gettime_s(void)
{
    return (uint32_t)(get_utc_ms()/1000);
}

unsigned int cloud_get_ResveredMem()
{
	unsigned int availableMemory = 0;
    unsigned int availableMemory_addr = 0;
    xy_OTA_flash_info(&availableMemory_addr, &availableMemory);
	return availableMemory;
}
int cloud_mutex_create(osMutexId_t *pMutexId)
{
    int ret = XY_ERR;
    *pMutexId = osMutexNew(NULL);
    if(*pMutexId != NULL)
        ret = XY_OK;
    return ret;
}

int cloud_mutex_destroy(osMutexId_t *pMutexId)
{
    int ret = XY_ERR;
    if (osMutexDelete(*pMutexId) == osOK)
    {
        *pMutexId = NULL;
        ret = XY_OK;
    }
    return ret;
}

int cloud_mutex_lock(osMutexId_t *pMutexId, uint32_t timeout)
{
    int ret = XY_ERR;
    if (osMutexAcquire(*pMutexId, timeout) == osOK)
        ret = XY_OK;
    return ret;
}

int cloud_mutex_unlock(osMutexId_t *pMutexId)
{
    long ret = XY_ERR;
    if (osMutexRelease(*pMutexId) == osOK)
        ret = XY_OK;
    return ret;
}


/*仅限于仅读一次场景，连续读不得使用！返回值-1表示失败，0表示未读到有效信息，正值表示读取的有效长度*/
int app_read_fs(const char * fileName,uint32_t offset,void * buf, uint32_t size)
{
    xy_file* fp = NULL;
    int ret = -1;
	int fs_offset = -1;


    fp = xy_fopen(fileName, "rb",FS_DEFAULT); 

    if (fp != NULL)
    {
    	if(offset > 0)
			fs_offset = xy_fseek(fp,offset, 0);
		
		if (fs_offset > 0 || offset == 0)
		{
			ret = xy_fread((void*)buf, size,fp);

            if(ret < 0)
            {
                ret = -1;
            }
			else if(ret >0 && ret < (int)size )
	        {
				xy_printf(0, XYAPP, WARN_LOG, "[FS]xy_fread %s %d %d FAIL!\n",fileName,size,ret);
			}
		}
		else
		{
			xy_printf(0, XYAPP, WARN_LOG, "[FS]xy_fseek %s %d ERR!\n",fileName,offset);
		}

		xy_fclose(fp);
	}
	else
	{
		xy_printf(0, XYAPP, WARN_LOG, "[FS]xy_fopen %s ERR!\n",fileName);
	}

	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
		xy_assert(ret != -1);
	
	return ret;

}

/*仅限于仅写一次场景，连续写不得使用！返回值XY_ERR表示失败，XY_OK表示成功*/
int app_write_fs(const char * fileName,void *buf,uint32_t size)
{
    xy_file* fp = NULL;
    int ret = XY_ERR;

	xy_assert(fileName!=NULL && size!=0);

   	fp = xy_fopen(fileName,"w+",FS_DEFAULT);
	
    if (fp != NULL)
    {
		ret = xy_fwrite(buf, size,fp);

        if(ret < (int)size )
        {
			xy_printf(0, XYAPP, WARN_LOG, "[FS]xy_fwrite %s %d %d ERR!\n",fileName,size,ret);
			ret = XY_ERR;
		}
        else
        {
            ret = XY_OK;
        }
        

        xy_fclose(fp);
    }
	else
	{
		xy_printf(0, XYAPP, WARN_LOG, "[FS]xy_fopen %s ERR!\n",fileName);
	}

	if (HWREGB(BAK_MEM_XY_DUMP) == 1)
		xy_assert(ret != XY_ERR);
	
    return ret;
}


void cloud_init()
{
    static int cloud_netifup_flag = 0;
    if (!cloud_netifup_flag)
    {
        netif_regist_init();
        tcpip_init(NULL, NULL);
		
#if WITH_MBEDTLS_SUPPORT	
	    dtls_init(); //dtls适配层初始化
#endif

#if AT_SOCKET
        at_socket_init();
#endif

#if VER_BC95
		xy_async_socket_resource_init();
#endif

#if XY_DM
        dm_ctl_init();
#endif // XY_DM

#if TELECOM_VER
		cdp_module_mutex_init();
        xy_reg_psnetif_callback(EVENT_PSNETIF_VALID, cdp_netif_event_callback);
#endif

#if MOBILE_VER
        cis_module_mutex_init();
        xy_reg_psnetif_callback(EVENT_PSNETIF_VALID, cis_netif_event_callback);
#endif

#if CTWING_VER
        xy_ctlw_ctl_init();
#endif

#if MQTT
        at_mqtt_init();
#endif

#if LIBCOAP
        at_coap_init();
#endif
        cloud_netifup_flag = 1;
    }
}

