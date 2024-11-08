#include "net_app_resume.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/udp.h"
#include "oss_nv.h"

#include "xy_fs.h"

#include "main_proxy.h"
#include "xy_system.h"
#if TELECOM_VER
#include "atiny_context.h"
#include "cdp_backup.h"
#include "ota_flag.h"

#endif

#if MOBILE_VER
#include "onenet_utils.h"
#endif


osMutexId_t g_cloud_fs_mutex= NULL;
osSemaphoreId_t cdp_recovery_sem = NULL;

#if CTWING_VER
#include "ctwing_resume.h"
#endif

extern int Remove_File_By_BakMem(const char * fileName);
extern int Read_File_By_BakMem(const char * fileName, void *buf, uint32_t size);
extern int Save_File_By_BakMem(const char * fileName);




/*该函数用于文件系统空间不够的情况下，清空所有云的会话文件*/
static void cloud_file_clear()
{
    xy_fremove(CDP_SESSION_NVM_FILE_NAME, FS_DEFAULT);
    xy_fremove(ONENET_SESSION_NVM_FILE_NAME, FS_DEFAULT);
    xy_fremove(SOCKET_SESSION_NVM_FILE_NAME, FS_DEFAULT);
    xy_fremove(CTLW_SESSION_FILE_UNDER_DIR, FS_DEFAULT);
}

int cloud_write_file(const char * fileName,void * buf, uint32_t size)
{
    xy_file fp = NULL;
    int writeCount = XY_ERR;

	xy_mutex_acquire(g_cloud_fs_mutex, osWaitForever);

    fp = xy_fopen(fileName, "w+", FS_DEFAULT );   //read & write & creat
    if (fp != NULL)
    {
        /* write the file */
		writeCount = xy_fwrite_safe(buf, size, fp);

        /*文件系统空间不够导致写文件失败，需要清除文件系统中的一些文件*/
        if(writeCount == LFS_ERR_NOSPC)
            cloud_file_clear();

        xy_fclose(fp);
    }

	xy_mutex_release(g_cloud_fs_mutex);
    return ((writeCount < 0)?XY_ERR:XY_OK);
}

int cloud_save_file(const char * fileName,void * buf, uint32_t size)
{
    /*OPENCPU产品使用AP侧RAM存储云业务全局，深睡唤醒始终有效*/
    if(Is_OpenCpu_Ver())
    {
        if(Save_File_By_BakMem(fileName) == 1)
            return XY_OK;
        else
            return XY_ERR;
    }
    else
        return cloud_write_file(fileName,buf,size);
}

int cloud_read_file(const char * fileName,void * buf, uint32_t size)
{
    xy_file fp = NULL;
    int readCount = XY_ERR;

	if(g_cloud_fs_mutex == NULL)
		g_cloud_fs_mutex = osMutexNew(NULL);

	if(Is_OpenCpu_Ver())
	{
	    return Read_File_By_BakMem(fileName, buf, size);
	}

	xy_mutex_acquire(g_cloud_fs_mutex, osWaitForever);
    
    /* open the file */
    fp = xy_fopen(fileName, "rb", FS_DEFAULT);   //read & write & creat
    if (fp != NULL)
    {
        /* read the file  */
		readCount = xy_fread(buf, size, fp);
        xy_fclose(fp);
    }

	xy_mutex_release(g_cloud_fs_mutex);
    return ((readCount < 0)?XY_ERR:XY_OK);
}

int cloud_remove_file(const char * fileName)
{
    /*OPENCPU产品直接使用AP存RAM存储云业务全局变量*/
    if(Is_OpenCpu_Ver())
    {
        return Remove_File_By_BakMem(fileName);
    }
	else
		return xy_fremove(fileName, FS_DEFAULT);
}

//该接口内部禁止调用xy_printf，可调用send_debug_by_at_uart
void save_net_app_infos(void)
{
#if VER_BC25
    //BC25对标：深睡之前如果有下行缓存，则缓存丢弃；对比机恢复功能关闭深睡恢复也有效，所以提到此处
	if(g_softap_var_nv->cdp_buffered_num != 0)
	{
		g_softap_var_nv->cdp_dropped_num += g_softap_var_nv->cdp_buffered_num;
		g_softap_var_nv->cdp_buffered_num = 0;
	}	
#endif
    return;
}

int is_IP_changed(net_app_type_t type)
{
    ip_addr_t pre_local_ip = {0};
    ip_addr_t new_local_ip = {0};

    if(pre_local_ip.type !=IPADDR_TYPE_V4 && pre_local_ip.type !=IPADDR_TYPE_V6)
        return IP_RECEIVE_ERROR;

    if(xy_get_ipaddr(pre_local_ip.type,&new_local_ip) == 0)
        return IP_RECEIVE_ERROR;

    if(ip_addr_cmp(&pre_local_ip,&new_local_ip))
        return IP_NO_CHANGED;
    else
        return IP_IS_CHANGED;
}

/**下行数据触发的网络业务恢复总入口*/
void net_resume()
{
    static bool have_resumed = false;

    if(have_resumed)
        return;
    have_resumed = true;

#if AT_SOCKET
    socket_resume();
#endif //AT_SOCKET

#if CTWING_VER
    xy_ctlw_resume();
#endif

#if TELECOM_VER
    cdp_resume();
#endif

#if MOBILE_VER
    onenet_resume();
#endif
}


