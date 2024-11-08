/**
 * @file xy_fota.c
 * @brief 芯翼自研的FOTA方案相关API接口，该源文件客户无权修改，只能调用相应的API接口。如果有额外需求，可与芯翼FAE联系
 * @version 1.0
 * @date 2022-06-22
 * @copyright Copyright (c) 2022  芯翼信息科技有限公司
 * 
 */
#include "xy_system.h"
#include "xy_memmap.h"
#include "xy_fota.h"
#include "dma.h"
#include "softap_nv.h"
#include "oss_nv.h"
#include "rsa.h"
#include "sha.h"
#include "sys_init.h"
#include "net_app_resume.h"
#include "xy_at_api.h"
#include "xy_flash.h"
#include "mpu_protect.h"

#if WITH_MBEDTLS_SUPPORT
#include "mbedtls/rsa.h"
#include "mbedtls/bignum.h"
#endif



#define FOTA_UPROUND(raw, base)      ((((raw) + (base)-1) / (base)) * (base))
#define FOTA_DOWNROUND(raw, base)    (((raw) / (base)) * (base))



/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/

#define	OTP_SHA_CIPHER_WORD_LEN			32				// RSA 1024 bits == 32  words

/**
* @brief Flash_Secondary_Boot_Fota_Def成员位置偏移
*/
#define OFFSET_FOTA_HEADER_PARAM(param)     ((uint32_t) & (((Flash_Secondary_Boot_Fota_Def *)0)->param))

/**
* @brief delta_head_info_t成员位置偏移
*/
#define OFFSET_DELTA_HEADER_PARAM(param)     ((uint32_t) & (((delta_head_info_t *)0)->param))

/**
* @brief FOTA差分文件头部魔术字
*/
#define FOTA_DELTA_MAGIC_NUM    3514

/**
* @brief 二级boot头部中主FOTA信息段地址
*/
#define FOTA_PRIME_INFO_ADDR        (SECONDBOOT_PRIME_HEADER_BASE + sizeof(Flash_Secondary_Boot_Image_Def) + sizeof(Flash_Secondary_Boot_Flash_Image_Def))

/**
* @brief 二级boot头部中辅FOTA信息段地址
*/
#define FOTA_BACKUP_INFO_ADDR       (SECONDBOOT_BACKUP_HEADER_BASE + sizeof(Flash_Secondary_Boot_Image_Def) + sizeof(Flash_Secondary_Boot_Flash_Image_Def))

/**
* @brief 主FOTA头部信息中MING SHA字段地址
*/
#define FOTA_VER_MING_SHA           (FOTA_PRIME_INFO_ADDR + OFFSET_FOTA_HEADER_PARAM(ming_sha)) 

/**
* @brief fota package hmac key
*/
#define HMAC_KEY "XY1100"

/**
* @brief 差分包中RSA加密SHA校验值后的最大长度
*/ 
#define FOTA_DELTA_RSA_SHA_MAX_LEN   128

/**
* @brief 差分升级状态标志，二级boot升级时使用，大版本仅赋值，以满足断电续升
*/ 
typedef enum
{
    FOTA_UPGRADE_BYPASS     = 0x0,
    FOTA_UPGRADE_NEEDED     = 0x1,
    FOTA_UPGRADE_SUCCESS    = 0x2,
    FOTA_UPGRADE_FAILED     = 0x3,
} XY_FOTA_FLAG;

/**
 * @brief 差分包完整性校验时校验类型
 */
typedef enum
{
    NO_CHECK_OLD = 0,
    SHA_CHECK_OLD,
    RSA_SHA_CHECK_OLD,
} CHECK_MODE_OLD_E;

/**
 * @brief 版本校验时Image镜像校验类型
 */
typedef enum
{
    RSA_SHA_CHECK = 0,
    SHA_CHECK,
} CHECK_MODE_E;

/**
 * @brief Image checksum信息中Image core类型
 */
typedef enum
{
    IMAGE_AP = 0,
    IMAGE_CP,
    IMAGE_AP_BOOT,
    IMAGE_CP_BOOT,
    IMAGE_FAC_NV,
    IMAGE_TYPE_MAX,
} IMAGE_TYPE_E;

/**
 * @brief 差分包中extra信息块类型
 */
typedef enum
{
    EXTRA_SECONDARY_BOOTLOADER_RSA_SHA = 0,
    EXTRA_SECONDARY_BOOTLOADER_SHA,
    EXTRA_AP_BOOT,
    EXTRA_AP,
    EXTRA_CP,
    EXTRA_CP_BOOT,
    EXTRA_FACNV,
    EXTRA_SECONDARY_BOOTLOADER_USERNV,
    EXTRA_AP_INFO,
    EXTRA_CP_INFO,
    EXTRA_WORKNV_UPG_MODE,
    EXTRA_AP_BOOT_OLD = 20,
    EXTRA_AP_OLD,
    EXTRA_CP_OLD,
    EXTRA_CP_BOOT_OLD,
    EXTRA_FACNV_OLD,
    EXTRA_TYPE_MAX
} extra_type_t;

/**
 * @brief 差分包中extra信息块数据
 */
typedef struct
{
	uint32_t extra_type;        //@see @ref extra_type_t
	uint32_t extra_data_len;
	uint32_t extra_reserve0;
	uint32_t extra_reserve1;
	uint32_t extra_reserve2;
    uint8_t *extra_data;
} extra_info_t;

/**
 * @brief 单Image Checksum信息
 */
typedef struct
{
    uint32_t image_core; // ap, cp, ap_boot, cp_boot, nv
    uint32_t load_addr_num;
    uint32_t load_addr[5];
    uint32_t len_byte[5];
	uint32_t check_switch;        // 0 关闭，1 开启
    uint32_t check_mode;          // 0 sha+rsa， 1 sha
    uint32_t image_crypto_policy; // 抽样，跳过几个字节取数据，0表示所有数据参与校验
    uint32_t image_content_crypto_value[32];
    uint32_t image_info_crc; // 该结构体这个字段之前所有字段按4字节异或
} Image_CheckSum_Def;

#define IMAGE_CHECKSUM_HEADER_MAGIC_NUM 0xACACACAC

/**
 * @brief 0x30004000地址存放的所有Image Checksum信息
 */
typedef struct
{
	uint32_t magic_num;         // 0xACACACAC
	uint32_t version;           // 安全启动内部版本
    uint32_t image_num;         // 当前默认为5
    uint32_t header_info_crc;   // 该结构体这个字段之前所有字段按4字节异或
    Image_CheckSum_Def image_info[7];
} Image_CheckSum_Header_Def;

/**
* @brief 二级boot 启动信息头
*/
typedef struct
{
    uint32_t prime_or_backup;
    uint32_t prime_load_addr;
    uint32_t prime_exec_addr;
    uint32_t prime_len_byte;
    uint32_t prime_image_content_crc;
    uint32_t prime_crypto_policy;
    uint32_t prime_crypto_value[OTP_SHA_CIPHER_WORD_LEN];
    uint32_t backup_load_addr;
    uint32_t backup_exec_addr;
    uint32_t backup_len_byte;
    uint32_t backup_image_content_crc;
    uint32_t backup_crypto_policy;
    uint32_t backup_crypto_value[OTP_SHA_CIPHER_WORD_LEN];
    uint32_t wakeup_ap_exec_addr;
    uint32_t wakeup_cp_enable; // bit 0, Release CP; bit 1, Enable CP Memory Remap;
    uint32_t image_info_crc;
} Flash_Secondary_Boot_Image_Def;

/**
* @brief 二级boot flash信息
*/
typedef struct
{
    uint32_t prime_load_addr;
    uint32_t prime_exec_addr;
    uint32_t prime_len_byte;
    uint32_t prime_image_content_crc;
    uint32_t backup_load_addr;
    uint32_t backup_exec_addr;
    uint32_t backup_len_byte;
    uint32_t backup_image_content_crc;
    uint32_t image_info_crc;
} Flash_Secondary_Boot_Flash_Image_Def;

/**
* @brief 二级BOOT FOTA HEADER信息
*/
typedef struct
{
    uint32_t fota_flag;
    uint32_t fota_base_addr;
    uint8_t ming_sha[SHA1HashSize];
    uint32_t fota_end_addr;
    uint32_t fota_backup_end_addr;
	uint8_t fota_off_debug;
    uint8_t reserve[11];
    uint32_t fota_info_crc;
} Flash_Secondary_Boot_Fota_Def;

/**
* @brief 差分包数据头部信息结构体,用户不需要关注
*/
typedef struct
{
    uint32_t magic_num;
	uint16_t sign_type;
	uint16_t sign_len;
    uint8_t total_sha[FOTA_DELTA_RSA_SHA_MAX_LEN];
	uint32_t flash_bytes_m;
    uint32_t version;
    uint8_t old_image_version_sha[SHA1HashSize];	///< old_image_version_sha
    uint8_t new_image_version_sha[SHA1HashSize];	///< new_image_version_sha
    uint32_t image_num;
    uint32_t extra_num;
    uint32_t user_delta_info_num;
    uint32_t backup_start_addr;
    uint32_t backup_len;
    uint32_t fota_packet_base;              ///< 差分包存放地址
    uint32_t lzma_model_properties;
    uint32_t lzma_dict_size;
    uint32_t user_data_len;
} delta_head_info_t;

/**
* @brief 差分包信息块数据，image对应bin文件
*/
typedef struct
{
	uint32_t image_core;
	uint32_t image_load_addr;
	uint32_t image_exec_addr;
	uint32_t image_delta_type;
	uint32_t image_delta_len;  /* 1到多个block的长度总和 */
	uint32_t image_delta_block_num;
	uint8_t image_delta_sha[SHA1HashSize];
} image_delta_info_t;

/**
* @brief 差分包block信息块数据
*/
typedef struct
{
	uint32_t image_delta_uncompressed_old_len;
	uint32_t image_delta_uncompressed_new_len;
	uint32_t image_delta_block_ctrl_len;
	uint32_t image_delta_block_diff_len;
	uint32_t image_delta_block_extra_len;
} image_delta_block_info_t;

/**
* @brief 获取存放差分包的FLASH的信息：FLASH区域起始地址和大小
*/
typedef struct
{
    uint32_t image_id[3];
    uint32_t load_addr;
    uint32_t exec_addr;
    uint32_t len_byte;
    uint32_t image_content_crc;
    uint32_t image_info_crc;
} Flash_Image_Def;

// External Flash First 256 bytes
typedef struct {
	// Header Info
	uint32_t image_num;
	uint32_t image_info_addr_base;
	uint32_t user_code;
	uint32_t nv_addr_base;
	uint32_t nv_len_byte;
	uint32_t exchange_addr_base;
	uint32_t exchange_len_byte;
	uint32_t header_info_crc;

	// Image Info
    Flash_Image_Def image_info[7];
} Flash_Header_Def;

/**
* @brief RSA公钥数据结构体
*/
typedef struct
{
    uint32_t magic;
    uint32_t rsa_N_len;
    uint32_t rsa_E;
    uint8_t rsa_N[128];
} Fota_Pub_Key_Def;


/**
* @brief 差分包存放的首地址和FLASH的大小
*/
static uint32_t g_flash_base;
static uint32_t g_flash_maxlen;


/*FOTA终端用户容许与否的开关。一旦不容许，则不得重启升级，以防止终端重要事件丢失*/
int g_fota_permit = 1;

/**
* @brief 接收到的差分包数据的大小
*/ 
static uint32_t g_recv_size = 0;

/**
* @brief 差分包信息结构体
*/
static delta_head_info_t *s_fota_delta_header = NULL;

/**
* @brief 断点续传数据结构体
*/
static Fota_State_Info_T *g_fota_breakpoint_info = NULL;


/*******************************************************************************
 *                          FOTA API 接口                                      *
 ******************************************************************************/

/*断电续传，保存进度状态信息*/
static bool OTA_save_breakpoint_info(Fota_State_Info_T *pBreakInfo)
{
	if (pBreakInfo == NULL) // || NOT_ALLOWED_SAVE_FLASH()
	{
		return false;
	}
	
	flash_interface_protect_disable();
	xy_Flash_Erase(FOTA_BREAKPOINT_INFO_ADDR, FOTA_BREAKPOINT_INFO_LEN);
	xy_Flash_Write_No_Erase(FOTA_BREAKPOINT_INFO_ADDR, pBreakInfo, sizeof(Fota_State_Info_T));
	flash_interface_protect_enable();

	return true;
}

static void OTA_get_breakpoint_info()
{
	static int have_read = 0;

	if(have_read == 0)
	{
		have_read = 1;
		
		if (g_fota_breakpoint_info == NULL)
		{
			g_fota_breakpoint_info = (Fota_State_Info_T *)xy_malloc(sizeof(Fota_State_Info_T));
		}

	    xy_Flash_Read(FOTA_BREAKPOINT_INFO_ADDR, g_fota_breakpoint_info, sizeof(Fota_State_Info_T));

        if(g_fota_breakpoint_info->recv_size!=0xFFFFFFFF && g_fota_breakpoint_info->recv_size!=0)
		{
			g_recv_size = g_fota_breakpoint_info->recv_size;
			g_flash_base = g_fota_breakpoint_info->flash_base;
			g_flash_maxlen = g_fota_breakpoint_info->flash_maxlen;
		}
	}
}

/*防止CP的FOTA流程异常，造成AP核长时间挂起*/
#define  FOTA_MAX_TIMEOUT  (30*60)
osTimerId_t g_fota_timer = NULL; 

/*开始差分包下载时，通知AP核执行一些紧急事务，同时开启软定时，防止差分包下载异常造成AP核业务长时间挂起*/
void FOTA_timeout_callback()
{
	OTA_upgrade_init();
}

extern void send_Fota_stat_msg(uint32_t fota_state);

static void OTA_update_state(XY_OTA_STAT_E stats)
{
	/*FOTA写FLASH是特殊场景，OPENCPU形态需要触发核间消息通知AP核执行特殊处理，如关看门狗/屏蔽中断事务等*/
	if(Is_OpenCpu_Ver())
	{	
		static uint32_t old_state = XY_FOTA_IDLE;

		/*准备升级，CP核将频繁擦写FLASH，通知AP核执行私有动作*/
		if(stats == XY_FOTA_DOWNLOADING)
		{
			/*开始差分包下载时，通知AP核执行一些紧急事务，同时开启软定时，防止差分包下载异常造成AP核业务长时间挂起*/
			send_Fota_stat_msg(1);

			osTimerAttr_t timer_attr = {0};
			timer_attr.name = "fota_timer";
			g_fota_timer = osTimerNew((osTimerFunc_t)FOTA_timeout_callback,osTimerOnce, NULL, &timer_attr);
			osTimerStart(g_fota_timer,FOTA_MAX_TIMEOUT* 1000);
		}
		/*差分包下载或校验失败，通知AP核恢复正常工作；成功不通知，因为AP核初始化阶段会死等云上报结果完成*/
		else if((stats == XY_FOTA_IDLE && old_state != XY_FOTA_IDLE) || stats == XY_FOTA_UPGRADE_FAIL)
		{
			send_Fota_stat_msg(0);
			if (g_fota_timer != NULL)
			{
				osTimerDelete(g_fota_timer);
				g_fota_timer = NULL;
			}	
		}
		old_state = stats;
	}

	OTA_get_breakpoint_info();
	g_fota_breakpoint_info->ota_stat = stats;
	
	if(stats == XY_FOTA_DOWNLOADING || stats == XY_FOTA_DOWNLOADED || stats == XY_FOTA_UPGRADING)
	{
		/*软重启时能够保持该状态值，进而二级boot升级完成后仍然为1*/
		HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 1;
		
		/*FOTA期间关闭FLASH写保护，以防止频繁操作寄存器影响phy性能*/
		flash_hardware_protect_set_always(0);
	}
	else if(stats == XY_FOTA_IDLE)
	{
		HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 0;
		g_fota_breakpoint_info->recv_size = 0xFFFFFFFF;
	}
	else
	{
		g_fota_breakpoint_info->recv_size = 0xFFFFFFFF;
	}
	
	OTA_save_breakpoint_info(g_fota_breakpoint_info);
}



/* 检查差分包合法性 */
static int mbedtls_rsa_pubkey_verify(uint8_t *prsa_n, size_t rsa_n_byte_len,
							uint8_t *prsa_e, size_t rsa_e_byte_len, 
							uint8_t *pCiphertext, uint8_t *pPlainText)
{
	int ret = XY_OK;
#if WITH_MBEDTLS_SUPPORT
	mbedtls_mpi K;
	mbedtls_rsa_context rsa;

    mbedtls_mpi_init(&K);
    mbedtls_rsa_init(&rsa);
	
	mbedtls_mpi_read_binary_le(&K, prsa_n, rsa_n_byte_len);
    mbedtls_rsa_import(&rsa, &K, NULL, NULL, NULL, NULL);
    
    mbedtls_mpi_read_binary_le(&K, prsa_e, rsa_e_byte_len);
    mbedtls_rsa_import(&rsa, NULL, NULL, NULL, NULL, &K);
    
	mbedtls_rsa_complete(&rsa);

	if (mbedtls_rsa_check_pubkey(&rsa) != 0)
    {
		ret = XY_ERR;
		goto cleanup;
    }
							   
   	if (mbedtls_rsa_pkcs1_verify( &rsa, MBEDTLS_MD_NONE, SHA1HashSize,
   								pPlainText, pCiphertext ) != 0)
 	{
 		ret = XY_ERR;
     	goto cleanup;
	}

cleanup:
    mbedtls_mpi_free( &K );
    mbedtls_rsa_free( &rsa );
#endif
	return ret;
}

static int get_extra_type(uint32_t image_core)
{
    switch (image_core)
    {
    case IMAGE_AP:
        return EXTRA_AP_OLD;
    case IMAGE_CP:
        return EXTRA_CP_OLD;
    case IMAGE_AP_BOOT:
        return EXTRA_AP_BOOT_OLD;
    case IMAGE_CP_BOOT:
        return EXTRA_CP_BOOT_OLD;
    case IMAGE_FAC_NV:
        return EXTRA_FACNV_OLD;  
    default:
        break;
    }
    return EXTRA_TYPE_MAX;
}

/* 检验差分包是否适用于该版本，根据差分包中携带的SHA校验值判断 */
static int OTA_version_check(delta_head_info_t *head)
{
    if (head == NULL)
        return XY_ERR;

    int i = 0;
    int ret = XY_OK;
    image_delta_info_t *delta_infos = NULL;
	extra_info_t *extra_infos = NULL;
    uint32_t header_info_crc = 0;
	
    Image_CheckSum_Header_Def* imgChk_infos = (Image_CheckSum_Header_Def*)xy_malloc2(sizeof(Image_CheckSum_Header_Def));
    if (imgChk_infos == NULL)
    {
        ret = XY_ERR;
        goto END;
    }
    xy_Flash_Read(IMAGE_CHECKSUM_ADDR, imgChk_infos, sizeof(Image_CheckSum_Header_Def));

    if (imgChk_infos->magic_num != IMAGE_CHECKSUM_HEADER_MAGIC_NUM || imgChk_infos->version != 1 || imgChk_infos->image_num > 7)
    {
        ret = XY_ERR;
        goto END;
    }

    header_info_crc = imgChk_infos->magic_num ^ imgChk_infos->version ^ imgChk_infos->image_num;
    if (header_info_crc != imgChk_infos->header_info_crc)
    {
        ret = XY_ERR;
        goto END;
    }

    int offset = 0;
    offset += sizeof(delta_head_info_t) + head->user_data_len;
    if (head->image_num > 0)
    {
        delta_infos = (image_delta_info_t *)xy_malloc2(sizeof(image_delta_info_t) * head->image_num);
        if (delta_infos == NULL)
        {
            ret = XY_ERR;
            goto END;
        }
        memset(delta_infos, 0x00, sizeof(image_delta_info_t) * head->image_num);
        for (i = 0; i < head->image_num; i++)
        {
            xy_Flash_Read(g_flash_base + offset, delta_infos + i, sizeof(image_delta_info_t));
            offset += sizeof(image_delta_info_t);
            offset += sizeof(image_delta_block_info_t) * (delta_infos[i].image_delta_block_num);
        }

        for (i = 0; i < head->image_num; i++)
        {
            offset += delta_infos[i].image_delta_len;
        }
    }

    /* 工具侧extra信息块记录了各个镜像的校验信息，不存在为0的情况 */
    extra_infos = (extra_info_t*)xy_malloc2(sizeof(extra_info_t) * head->extra_num);
    if (extra_infos == NULL)
    {
        ret = XY_ERR;
        goto END;
    }
    memset(extra_infos, 0x00, sizeof(extra_info_t) * head->extra_num);
    /* 读取额外信息块 */
    for (i = 0; i < head->extra_num; i++)
    {
        xy_Flash_Read(g_flash_base + offset, extra_infos + i, sizeof(extra_info_t) - 4);
        offset += sizeof(extra_info_t) - 4;
        if (extra_infos[i].extra_data_len != 0)
        {
            extra_infos[i].extra_data = (uint8_t *)xy_malloc2(extra_infos[i].extra_data_len);
            if (extra_infos[i].extra_data == NULL)
            {
                ret = XY_ERR;
                goto END;
            }
            xy_Flash_Read(g_flash_base + offset, extra_infos[i].extra_data, extra_infos[i].extra_data_len);
			offset += extra_infos[i].extra_data_len;
        }
    }
	
	/* IMAGE版本校验 */
    int j = 0;
    for (i = 0; i < imgChk_infos->image_num; i++)
    {
       int extra_type = get_extra_type(imgChk_infos->image_info[i].image_core);
       if (extra_type == EXTRA_TYPE_MAX)
       {
           	ret = XY_ERR;
        	goto END;
       }

       for (j = 0; j < head->extra_num; j++)
       {
           if (extra_infos[j].extra_type == extra_type)
           {
               if (extra_infos[j].extra_reserve2 != imgChk_infos->image_info[i].check_switch ||
			   		extra_infos[j].extra_reserve1 != imgChk_infos->image_info[i].check_mode || 
			   		extra_infos[j].extra_reserve0 != imgChk_infos->image_info[i].image_crypto_policy)
               {
                    ret = XY_ERR;
        			goto END;

               }
               if (imgChk_infos->image_info[i].check_mode == SHA_CHECK || imgChk_infos->image_info[i].check_mode == RSA_SHA_CHECK)
               {
                   if (memcmp(imgChk_infos->image_info[i].image_content_crypto_value, extra_infos[j].extra_data, extra_infos[j].extra_data_len) != 0)
                   {
                        ret = XY_ERR;
        				goto END;
                   }
                   else
                   {
                        break;
                   }
               }
           }
       }
    }
	
END:
    for (i = 0; extra_infos != NULL && i < head->extra_num; i++)
    {
        if (extra_infos[i].extra_data != NULL)
            xy_free(extra_infos[i].extra_data);
    }
    if (imgChk_infos != NULL)
        xy_free(imgChk_infos);
    if (extra_infos != NULL)
        xy_free(extra_infos);
    if (delta_infos != NULL)
        xy_free(delta_infos);
    return ret;
}

/*对差分包数据进行处理，检查差分包头的完整性，版本检测，最后将差分包数据写入flash*/
static int OTA_downlink_packet_proc(char* data, uint32_t size)
{
    uint32_t offset = 0;
    char *input_data = data;
    uint32_t input_len = size;

	if(g_softap_fac_nv->fota_close == 1 || g_fota_permit==0)
	{
        xy_printf(0, XYAPP, WARN_LOG, "OTA_downlink_packet_proc fail,not permit!");
        return XY_ERR;
    }

    if (g_recv_size + input_len > g_flash_maxlen)
    {
        xy_printf(0, XYAPP, WARN_LOG, "error:Dtle file size too large!");
        return XY_ERR;
    }

    if (g_recv_size + input_len < (sizeof(delta_head_info_t)))
    {
        DMA_Memcpy_SYNC((char *)s_fota_delta_header + g_recv_size, input_data, input_len, 1000);
        g_recv_size += input_len;
        return XY_OK;
    }

    if (g_recv_size < (sizeof(delta_head_info_t)))
    {
        DMA_Memcpy_SYNC((char *)s_fota_delta_header + g_recv_size, input_data, (sizeof(delta_head_info_t)) - g_recv_size, 1000);
        input_data += (sizeof(delta_head_info_t)) - g_recv_size;
        input_len -= (sizeof(delta_head_info_t)) - g_recv_size;
        g_recv_size = (sizeof(delta_head_info_t));

        /* 差分包魔术字检测 */
        if (s_fota_delta_header->magic_num != FOTA_DELTA_MAGIC_NUM)
        {
            xy_printf(0, XYAPP, WARN_LOG, "error: magic_num(%d) != %d", s_fota_delta_header->magic_num, FOTA_DELTA_MAGIC_NUM);
            return XY_ERR;
        }

        /* 差分包过大flash不足 */
        if ((Address_Translation_AP_To_CP(s_fota_delta_header->fota_packet_base)) < g_flash_base)
        {
            xy_printf(0,XYAPP, WARN_LOG, "error: delta is too big");
            return XY_ERR;
        }

        xy_printf(0, XYAPP, WARN_LOG, "user_data_len: %x ", s_fota_delta_header->user_data_len);

        g_flash_maxlen = g_flash_maxlen - (Address_Translation_AP_To_CP(s_fota_delta_header->fota_packet_base) - g_flash_base);
        g_flash_base = Address_Translation_AP_To_CP(s_fota_delta_header->fota_packet_base);

		//写入真正的差分包存放地址
		g_fota_breakpoint_info->flash_base = g_flash_base;
		g_fota_breakpoint_info->flash_maxlen = g_flash_maxlen;		
        xy_printf(0, XYAPP, WARN_LOG, "delta_save_addr: %x %x", s_fota_delta_header->fota_packet_base, g_flash_maxlen);
    }

    if (g_recv_size == (sizeof(delta_head_info_t)))
    {
		if(Is_OpenCpu_Ver())
		{
        	xy_Flash_Write(g_flash_base, s_fota_delta_header, (sizeof(delta_head_info_t)));
        	if (input_len > 0)
        	{
            	xy_Flash_Write(g_flash_base + (sizeof(delta_head_info_t)), input_data, input_len);
        	}
		}
		else
		{
			xy_Flash_Erase(g_flash_base, g_flash_maxlen);
        	xy_Flash_Write_No_Erase(g_flash_base, s_fota_delta_header, (sizeof(delta_head_info_t)));
        	if (input_len > 0)
        	{
            	xy_Flash_Write_No_Erase(g_flash_base + (sizeof(delta_head_info_t)), input_data, input_len);
        	}
		}
        g_recv_size += input_len;
    }
    else
    {
        if(Is_OpenCpu_Ver())
    	{
			xy_Flash_Write(g_flash_base + g_recv_size, input_data, input_len);
		}
		else
		{
			xy_Flash_Write_No_Erase(g_flash_base + g_recv_size, input_data, input_len);
		}
        g_recv_size += input_len;
    }

    return XY_OK;
}



/*FOTA初始化动作，成功或失败后，也会执行该接口*/
void OTA_upgrade_init()
{
    g_flash_base = 0;
    g_flash_maxlen = 0;
    g_recv_size = 0;
	
	OTA_update_state(XY_FOTA_IDLE);

    if (s_fota_delta_header != NULL)
    {
        xy_free(s_fota_delta_header);
        s_fota_delta_header = NULL;
    }
	
    xy_printf(0, XYAPP, WARN_LOG, "[OTA_upgrade_init]Reset to complete!");
}

/*保存单个差分包*/
int OTA_save_one_packet(char* data, uint32_t size)
{
    if(g_softap_fac_nv->fota_close == 1 || g_fota_permit==0)
	    goto error;

	//开始升级或者断电续升，需执行init操作
    if (g_recv_size == 0)
    {	
        //获取差分区域的起始地址和长度
        xy_OTA_flash_info(&(g_flash_base), &(g_flash_maxlen));

		OTA_get_breakpoint_info();
			
        //下载出错或续升后，已下载数据没有包含全部的差分包信息，需重新开始下载
        if (g_recv_size > 0 && g_recv_size < sizeof(delta_head_info_t))
        {
            xy_printf(0,XYAPP, WARN_LOG, "[OTA_save_one_packet]please restart fota!");
            return XY_Err_NotAllowed;
        }

        //断电续升后需从flash中获取差分包头部信息
        s_fota_delta_header = (delta_head_info_t *)xy_malloc2(sizeof(delta_head_info_t));
        if (s_fota_delta_header == NULL)
        {
            goto error;
        }
        if (g_recv_size >= sizeof(delta_head_info_t))
            xy_Flash_Read(g_flash_base, s_fota_delta_header, sizeof(delta_head_info_t));

		OTA_update_state(XY_FOTA_DOWNLOADING);

        xy_printf(0,XYAPP, WARN_LOG, "[OTA_save_one_packet] Init end");
    }

    if (OTA_downlink_packet_proc(data, size) != XY_OK)
        goto error;
    
    //差分包数据写入FLASH后，需更新已保存的数据长度
    g_fota_breakpoint_info->recv_size = g_recv_size;
	OTA_save_breakpoint_info(g_fota_breakpoint_info);

    return XY_OK;
    
error: 
    OTA_upgrade_init();
    return XY_ERR;
}

/*对整个差分包进行校验*/
int OTA_delta_check()
{
    int ret = XY_OK;
    uint8_t digest[SHA1HashSize];
    uint8_t *buf= NULL;
    int read_size = 0;
    delta_head_info_t *head_info = NULL;
	HMACContext *hmac_context = NULL;

	if(g_softap_fac_nv->fota_close == 1 || g_fota_permit==0)
	{
        xy_printf(0, XYAPP, WARN_LOG, "OTA_delta_check fail,not permit!");
        return XY_ERR;
    }

    if (g_flash_base == 0 || g_recv_size <= sizeof(delta_head_info_t))
    {
        xy_printf(0, XYAPP, WARN_LOG, "[OTA]:check no recv data\n");
        return XY_ERR;
    }

    xy_printf(0,XYAPP, WARN_LOG, "[OTA]:flash_base:%x, recv_size:%d\n", g_flash_base, g_recv_size);

    buf = (uint8_t *)xy_malloc2(1024);
    if (buf == NULL)
    {
        ret = XY_ERR;
        goto exit;
    }
    head_info = (delta_head_info_t *)xy_malloc2(sizeof(delta_head_info_t));
    if (head_info == NULL)
    {
        ret = XY_ERR;
        goto exit;
    }
    memset(head_info, 0x00, sizeof(delta_head_info_t));
    xy_Flash_Read(g_flash_base + read_size, head_info, sizeof(delta_head_info_t));

    xy_printf(0, XYAPP, WARN_LOG, "[OTA]:delta check mode:%d\n", head_info->sign_type);

    if (head_info->sign_type != NO_CHECK_OLD && head_info->sign_type != SHA_CHECK_OLD && head_info->sign_type != RSA_SHA_CHECK_OLD)
    {
    	ret = XY_ERR;
        goto exit;
    }
    if (head_info->sign_type == NO_CHECK_OLD)
    {
        ret = XY_OK;
		goto exit;
    }

    //初始化HMAC校验结构体
    hmac_context = (HMACContext *)xy_malloc2(sizeof(HMACContext));
    if (hmac_context == NULL)
    {
        ret = XY_ERR;
        goto exit;
    }
    memset(hmac_context, 0x00, sizeof(HMACContext));
    hmacReset(hmac_context, SHA1, (const uint8_t*)HMAC_KEY, strlen(HMAC_KEY));

    //差分包RSA加密SHA字段前所有内容不参与SHA计算
    hmacInput(hmac_context, (const uint8_t *)&(head_info->flash_bytes_m), sizeof(delta_head_info_t) - OFFSET_DELTA_HEADER_PARAM(flash_bytes_m));
    read_size += sizeof(delta_head_info_t);
    
    while((read_size + 1024) < g_recv_size)
    {
        memset(buf, 0x00, 1024);
        xy_Flash_Read(g_flash_base + read_size, buf, 1024);
        read_size += 1024;
        if(hmacInput(hmac_context, (const uint8_t *)buf, 1024))
        {
        	ret = XY_ERR;
			goto exit;
		}
    }

    memset(buf, 0x00, 1024);
    xy_Flash_Read(g_flash_base + read_size, buf, g_recv_size - read_size);
    if(hmacInput(hmac_context, (const uint8_t *)(buf), (uint32_t)(g_recv_size - read_size)))
    {
    	ret = XY_ERR;
		goto exit;
    }

    hmacResult(hmac_context, digest);

    if (head_info->sign_type == SHA_CHECK)
    {

    }
    else
    {
		Fota_Pub_Key_Def *pub_key = (Fota_Pub_Key_Def *)BAK_MEM_OTP_RSA_BASE;
	
		if(mbedtls_rsa_pubkey_verify((uint8_t *)pub_key->rsa_N, pub_key->rsa_N_len, (uint8_t *)&(pub_key->rsa_E), 4, head_info->total_sha, digest) == XY_OK)
		{
			/*如果用户需要携带私有数据，可在此定制*/
			//ota_private_data_hook(head_info->fota_packet_base + sizeof(delta_head_info_t), head_info->user_data_len);
		}
		else
		{
			xy_printf(0,XYAPP, WARN_LOG, "[OTA]: FW integrity check failed");
			ret = XY_ERR;
			goto exit;
		}
    }

    if(OTA_version_check(head_info) != XY_OK)
    {
    	xy_printf(0,XYAPP, WARN_LOG, "[OTA]: FW version check failed");
		ret = XY_ERR;
		goto exit;
	}

	OTA_update_state(XY_FOTA_DOWNLOADED);
	xy_printf(0, XYAPP, WARN_LOG, "[OTA]: FW validate success");
	
exit:
	if(buf)
		xy_free(buf);
	if(head_info)
		xy_free(head_info);
	if(hmac_context)
		xy_free(hmac_context);
	if(ret != XY_OK)
     	OTA_upgrade_init();
    return ret;
}


/* 设置二级BOOT的FOTA标志位和相关信息，并重启后开始由二级boot升级 */
int OTA_upgrade_start()
{
    Flash_Secondary_Boot_Fota_Def fota_def;

	if(g_softap_fac_nv->fota_close == 1 || g_fota_permit==0)
	{
        xy_printf(0, XYAPP, WARN_LOG, "OTA_upgrade_start fail,not permit!");
        goto error;
    }

	if (OTA_get_state() != XY_FOTA_DOWNLOADED)
	{
		goto error;
	}

	g_flash_base = g_fota_breakpoint_info->flash_base;
	g_flash_maxlen = g_fota_breakpoint_info->flash_maxlen;

	//断电续升后需重新从flash中获取差分包头部信息
	if (s_fota_delta_header == NULL)
	{        
   		s_fota_delta_header = (delta_head_info_t *)xy_malloc2(sizeof(delta_head_info_t));
        if (s_fota_delta_header == NULL)
        {
            goto error;
        }
    	xy_Flash_Read(g_flash_base, s_fota_delta_header, sizeof(delta_head_info_t));
	}

		
	OTA_update_state(XY_FOTA_UPGRADING);

    xy_Flash_Read(FOTA_PRIME_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));

    //设置二级boot相关信息并保存到flash中
    fota_def.fota_flag = FOTA_UPGRADE_NEEDED;
    fota_def.fota_base_addr = Address_Translation_CP_To_AP(g_flash_base);
    fota_def.fota_end_addr = Address_Translation_CP_To_AP(g_flash_base) + g_flash_maxlen;
	fota_def.fota_off_debug = g_softap_fac_nv->off_debug;

    //记录备份flash结束地址，用于二级boot升级时备份flash info header
    fota_def.fota_backup_end_addr = FOTA_UPROUND(s_fota_delta_header->backup_start_addr + s_fota_delta_header->backup_len, FLASH_SECTOR_LENGTH);
    fota_def.fota_info_crc = (unsigned short)xy_chksum((void *)&fota_def, sizeof(Flash_Secondary_Boot_Fota_Def) - 4);

	/*前1M空间正常禁止写入，FOTA时需临时关闭保护才可写*/
    flash_interface_protect_disable();
    xy_Flash_Write(FOTA_PRIME_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));
    flash_interface_protect_enable();

    xy_printf(0,XYAPP, WARN_LOG, "[OTA_upgrade_start] updating");

    if (g_softap_fac_nv->fota_close == 1 || g_fota_permit==0)
		goto error;


    //重启升级
	xy_Soft_Reset(SOFT_RB_BY_FOTA);
    return XY_OK;

error:
    OTA_upgrade_init();
    return XY_ERR;
}

/*由云业务调用，调用之前需云模块自行识别当前是在FOTA流程中*/
int OTA_get_upgrade_result()
{
    if (OTA_get_state() == XY_FOTA_UPGRADE_SUCCESS) 
		return XY_OK;
	else
		return XY_ERR;
}

/*FOTA升级成功后，由主控函数调用，更新升级最终结果。OPENCPU形态下，此时AP核在系统初始化里死等升级完成*/
void OTA_update_upgrade_result()
{
    Flash_Secondary_Boot_Fota_Def fota_def, tmp_fota_def;
    
    xy_Flash_Read(FOTA_PRIME_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));

    tmp_fota_def = fota_def;

    //重新设置二级boot fota相关信息并保存到flash中，清除升级结果标志位(0xFFFFFFF)
    fota_def.fota_flag = -1;
    
    fota_def.fota_info_crc = (unsigned short)xy_chksum((void *)&fota_def, sizeof(Flash_Secondary_Boot_Fota_Def) - 4);

	/*前1M空间正常禁止写入，FOTA时需临时关闭保护才可写*/
    flash_interface_protect_disable();
    xy_Flash_Write(FOTA_PRIME_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));
    xy_Flash_Write(FOTA_BACKUP_INFO_ADDR, &fota_def, sizeof(Flash_Secondary_Boot_Fota_Def));
    flash_interface_protect_enable();

    /*OPENCPU形态下，更新升级最终结果后，不允许写FLASH*/
    if (tmp_fota_def.fota_flag == FOTA_UPGRADE_SUCCESS)
    {
    	OTA_update_state(XY_FOTA_UPGRADE_SUCCESS);
    }
    else
    {
    	OTA_update_state(XY_FOTA_UPGRADE_FAIL);
    }
}

XY_OTA_STAT_E OTA_get_state()
{
    OTA_get_breakpoint_info();
	
	return g_fota_breakpoint_info->ota_stat;
}

/*识别当前是否正在进行FOTA流程，包括升级结果的云上报过程*/
bool OTA_is_doing()
{
    return (HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) != 0);
}

void OTA_set_permit(int permit)
{
	g_fota_permit = permit;
}

int OTA_get_permit()
{
	return g_fota_permit;
}

/*FOTA尾部临时保存debug、易变NV等，执行FOTA时会被擦除*/
void xy_OTA_flash_info(uint32_t *addr, uint32_t *len)
{
	*addr = OTA_FLASH_BASE();
    *len = ARM_FLASH_BASE_LEN - (*addr - ARM_FLASH_BASE_ADDR);
}


