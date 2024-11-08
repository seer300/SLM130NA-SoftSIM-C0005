#include "xy_memmap.h"
#include "hw_types.h"
#include "adc.h"
#include "xy_system.h"
#include "sys_proc.h"
#include "xy_flash.h"
#include "dyn_load.h"
#include "utc.h"
#include "rc32k.h"
#include "sys_rc32k.h"
#include "xy_ftl.h"


__FLASH_FUNC void Transfer_Macro_AP_to_CP() // 通过核间共享将一些AP编译宏传递至CP
{
	/*opencpu形态不能实时写FLASH，只能AP单核或SoC深睡时写FLASH*/
	HWREGB(BAK_MEM_BAN_WRITE_FLASH) = BAN_WRITE_FLASH;
	HWREGB(BAK_MEM_MODULE_VER) = MODULE_VER;
	
#if (MODULE_VER == 0x0)
	// open AP必须快速恢复，CP禁止快速恢复，8K长供电
	HWREGB(BAK_MEM_AP_FAST_RECOVERY_FUNC) = 1; // 1表示AP核支持深睡快速恢复，进而整个RAM深睡不下电，唤醒后从WFI退出继续运行
#else
	// 普通模组
	// AP、CP快速恢复功能开启必须保持一致，CP侧采用编译宏的方式：详见define.mk中的CP_FAST_RECOVERY_FUNCTION
	// 默认不支持快速恢复
	HWREGB(BAK_MEM_AP_FAST_RECOVERY_FUNC) = 0;
#endif


#if AT_LPUART
	HWREGB(BAK_MEM_LPUART_USE) = 1;
#else
	HWREGB(BAK_MEM_LPUART_USE) = 0;
#endif
}

// 断电上电或硬reset后执行back内存的初始化，防止脏数据
__FLASH_FUNC void ShmFlagInit(void)
{
#if (DYN_LOAD == 0)
	extern uint32_t _BAK_MEM_FOTA_FLASH_BASE;
	HWREG(BAK_MEM_FOTA_FLASH_BASE) = (uint32_t)&_BAK_MEM_FOTA_FLASH_BASE;
#else
	if (SO_AVAILABLE)
	{
		extern const uint32_t DYN_FOTA_FLASH_BASE;
		HWREG(BAK_MEM_FOTA_FLASH_BASE) = DYN_FOTA_FLASH_BASE;
	}
	else
	{
		extern uint32_t _BAK_MEM_FOTA_FLASH_BASE;
		HWREG(BAK_MEM_FOTA_FLASH_BASE) = (uint32_t)&_BAK_MEM_FOTA_FLASH_BASE;
	}
#endif
	/* 目前仅以下几个跨核状态机需要进行上电初始化，其他状态机都是使用时进行初始化 */
	HWREGB(BAK_MEM_CP_DO_DUMP_FLAG) = 0;
	HWREGB(BAK_MEM_AP_DO_DUMP_FLAG) = 0;
	HWREGB(BAK_MEM_DUMP_LOGVIEW_FLAG) = 0;
	HWREGB(BAK_MEM_BOOT_CP_SYNC) = 0;
	HWREGB(BAK_MEM_ATUART_STATE) = 0;
	HWREG(BAK_MEM_FLASH_NOTICE) = 0;
	HWREGB(BAK_MEM_LPM_FLASH_STATUS) = 0;
	HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 0;
	HWREGB(BAK_MEM_OPEN_TEST) = 0;
	HWREG(BAK_MEM_CLOUDTEST) = 0;
	*((volatile uint64_t *)BAK_MEM_RTC_NEXT_OFFSET) = RTC_NEXT_OFFSET_INVAILD; // BUG8986 规避代码：初始化为无效值，若不起CP单核运行则永远无效
	HWREGB(BAK_MEM_RC32K_CALI_PERMIT) = 1;
	HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0; // 校准中出现异常断电、重启、reset等场景，在此处清标志位，规避无法进入睡眠
	HWREGB(BAK_MEM_AP_LOCK_TYPE) = 0;
    HWREGB(BAK_MEM_FORCE_STOP_CP_HANDSHAKE) = FORCE_STOP_CP_NONE;

	HWREGB(BAK_MEM_32K_CLK_SRC) = PRCM_32KClkSrcGet();
	
	HWREG(BAK_MEM_LAST_WAKEUP_TICK) = 0;
	HWREGB(BAK_MEM_DROP_URC) = DROP_URC;

	HWREGB(BAK_MEM_AP_LOG) = READ_FAC_NV(uint8_t,open_log);

	Transfer_Macro_AP_to_CP(); // 通过核间共享将一些AP编译宏传递至CP

	/*借给CP核用的RAM空间，默认为0，特殊用户可以开启*/
	if(CP_USED_APRAM != 0)
	{
		HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) = CP_USED_APRAM;
	}
	else
	{
		HWREGB(BAK_MEM_CP_USED_APRAM_SIZE) = READ_FAC_NV(uint8_t,log_size);
	}

	/*CP向AP借30K内存*/
	if (1) //(HWREGB(BAK_MEM_CP_LOG_OPEN))
	{
		extern char end asm("end"); /* Defined by the linker.  */
		HWREG(BAK_MEM_CP_USED_APRAM_ADDR) = (uint32_t)&end + 0x1F000000;
	}
	else
	{
		HWREG(BAK_MEM_CP_USED_APRAM_ADDR) = 0;
	}
}

extern Boot_Reason_Type get_sys_up_stat(void);

/*除了快速恢复的深睡唤醒场景外，其他场景皆会调用，包括：断电上电、全局复位、模组形态深睡唤醒等；执行retension mem的初始化，其中模组形态深睡唤醒会下电*/
__FLASH_FUNC void BakupMemInit(void)
{
	Boot_Reason_Type sys_up_reason;
	// AP侧获取上电原因
	sys_up_reason = get_sys_up_stat();

	HWREG(BAK_MEM_CLOCK_TICK) = (TICK->AP_INT_EN << 8) | TICK->AP_INTSTAT;

#if KEEP_CP_ALIVE
	HWREGB(BAK_MEM_CP_SET_RTC) = 1;
#else
	HWREGB(BAK_MEM_CP_SET_RTC) = 0;
#endif

	// 非深睡唤醒场景
	if (sys_up_reason != WAKEUP_DSLEEP)
	{
		/*非深睡唤醒场景，CP核的上电原因与AP一致*/
		HWREGB(BAK_MEM_CP_UP_REASON) = sys_up_reason;
		HWREG(BAK_MEM_CP_UP_SUBREASON) = Get_Boot_Sub_Reason();

		extern void fac_working_nv_check();
		fac_working_nv_check();
		
		// 断电上电或全局复位时，清用户数据和世界时间；软复位一般由用户自行触发，此区域数据会正常保持
		if(sys_up_reason != SOFT_RESET)
		{
			/*CP核相关的全局数据由CP自行清楚*/
			memset((void*)USER_BAK_MEM_BASE, 0, USER_BAK_MEM_LEN);
			memset((void*)RAM_NV_VOLATILE_SOFTAP_START, 0,sizeof(snapshot_t));
			/*BAK_MEM_AP_TIME_BASE在Time_Init里进行上电初始化*/
		}
		// AT+RESET软重启时，执行恢复出厂设置，擦除工作态的出厂NV，非易变NV以及文件系统分区
		else if(Get_Boot_Sub_Reason() == SOFT_RB_BY_RESET)
		{
			xy_Flash_Erase(NV_FLASH_FACTORY_BASE, NV_FLASH_FACTORY_LEN);
			xy_Flash_Erase(NV_NON_VOLATILE_BASE, NV_NON_VOLATILE_LEN);
			xy_Flash_Erase(FS_FLASH_BASE, FS_FLASH_LEN);
		}

		ShmFlagInit();
		
		if(Get_Boot_Reason() != SOFT_RESET || Get_Boot_Sub_Reason() != SOFT_RB_BY_NRB)
			HWREGB(BAK_MEM_RF_MODE) = 0;

		/*FOTA标记位清除*/
		if(Get_Boot_Reason() != SOFT_RESET || Get_Boot_Sub_Reason() != SOFT_RB_BY_FOTA)
			HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 0;
	}
	else
	{
#if MODULE_VER
		// 模组用户深睡时长达到NV参数bakmem_threshold所设置的阈值时，retmem整个区域会在深睡时掉电
		if ((AONPRCM->AONPWR_CTRL & 0x8800) != 0x8800 && (AONPRCM->AONPWR_CTRL & 0x8200) != 0x200)  //ioldo1、sido均下电
		{
			// 此情况下，深睡时ioldo1直接下电，1st boot中完成flash的初始化，此处直接初始化核间共享状态机
			ShmFlagInit();
			HWREGB(BAK_MEM_FOTA_RUNNING_FLAG) = 0;
			HWREGB(BAK_MEM_RF_MODE) = 0;
			// 用于通知CP核执行retmem部分内容恢复，AP核不做恢复
			HWREGB(BAK_MEM_OFF_RETMEM_FLAG) = 2;

		    //可能是脏值，由于CP checksum功能关闭，初始化为0
			HWREG(BAK_MEM_CP_RETMEM_CSM) = 0;
		}
        /*若当前为执行stop_cp后进入模组深睡，则设置CP核为断电上电模式，CP核的nv_restore()内部会清零bakup内存空间*/
		if(!(HWREGB(BAK_MEM_CP_UP_REASON)==POWER_ON && HWREG(BAK_MEM_CP_UP_SUBREASON)==1))
		{
			HWREGB(BAK_MEM_CP_UP_REASON) = sys_up_reason;
			HWREG(BAK_MEM_CP_UP_SUBREASON) = Get_Boot_Sub_Reason();
		}

#endif
		HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0;  //规避深睡唤醒后，无法再次进入睡眠
	}
}
