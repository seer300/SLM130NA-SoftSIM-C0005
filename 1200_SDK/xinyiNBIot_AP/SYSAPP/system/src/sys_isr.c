#include <stdint.h>
#include "tick.h"
#include "watchdog.h"
#include "hw_types.h"
#include "xinyi2100.h"
#include "sys_ipc.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include "hw_prcm.h"
#include "hw_utc.h"
#include "xy_cp.h"
#include "hal_def.h"
#include "ap_watchdog.h"
#include "xy_event.h"
#include "xy_svd.h"

extern int Time_ProcessEvent(void);

void CLKTIM_Handler()
{
	/*clear interrupt*/
	Tick_APReadAndClearInt();

	Time_ProcessEvent();
}

/*CP核的UTC的超时唤醒，或CP核触发的核间中断*/
__OPENCPU_FUNC void PendSV_Handler(void)
{
	if(CP_Is_Alive() == true)
    {
    	/*该函数运行在FLASH上，进而AP核写FLASH期间必须关中断*/
		IPC_ProcessEvent();
	}
#if MODULE_VER==0
	else
	{
		set_event(EVENT_BOOT_CP);
	}
#endif
}

/*目前仅支持CP核的UTC或CP核发送的IPC核间中断触发pendsv*/
void Trigger_PendSV(void)
{
	/* Set a PendSV to request  */	
	( * ( ( volatile uint32_t * ) 0xe000ed04 ) ) = ( 1UL << 28UL );

	/* Barriers are normally not required but do ensure the code is completely	\
	within the specified behaviour for the architecture. */
#if defined ( __CC_ARM   )
	__asm volatile( "dsb");
#elif defined ( __GNUC__ )
	__asm volatile( "dsb" ::: "memory" );
#endif
	__asm volatile( "isb" );	
}


extern void DumpRegister_from_Normal(void);

/* 唤醒中断处理函数，非深睡状态下也会触发相应中断 */
void WAKEUP_Handler(void)
{
	//获取WAKEUP中断标志
    uint32_t aon_wakeup_status_reg = AONPRCM->WAKEUP_STATUS;

    //对于gpi唤醒，软件立刻清intStaus可能无效，因为此时gpi_wkp信号硬件可能没拉低，
    //软件清了intStaus之后有可能又被置位，因此至少要等1utc_cnt，硬件才会真正完成清除。
	uint8_t gpi_wakeupsrc = ((aon_wakeup_status_reg >> 16) & 0xFF);
	if(gpi_wakeupsrc)
	{
		utc_cnt_delay(1);
	}

	//写1清除中断位，避免反复进入该中断处理函数
    //等待中断位清除完成，该位置1后可再次进入中断处理函数
    //该标志位有概率出现写1一次清不掉的情况，这里多次写1直到清掉为止
	AONPRCM->WAKUP_INT0 = 0x01;	
    while((AONPRCM->WAKUP_INT0 & 0x01) != 0)
	{
		AONPRCM->WAKUP_INT0 = 0x01;
	}
	
	//CP核触发的核间中断
	if (PRCM_CpApIntWkupGet())
	{
		PRCM_CpApIntWkupClear();

		/*CP死机通知AP核，AP核区分产品形态后进行dump差异化处理*/
		if (HWREGB(BAK_MEM_CP_DO_DUMP_FLAG) == 1)
		{
			HWREGB(BAK_MEM_CP_DO_DUMP_FLAG) = 0;
			DumpRegister_from_Normal();
			extern void proc_cp_abnormal(void);
			proc_cp_abnormal();
			return;
		}
		
		Trigger_PendSV();
	}

	//svd 唤醒
    extern pFunType_void p_Svd_WakeupCallback;
	if(aon_wakeup_status_reg & WAKEUP_STATUS_SVD_WKUP_Msk)
	{
		PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_SVD);
		PRCM_AonWkupSourceEnable(AON_WKUP_SOURCE_SVD);

		//清SVD中断状态位
		while(SVD->INT_STAT)
		{
			SVD->INT_STAT = 0x0f;
		}
		//函数指针非空，则调用回调函数
        if(p_Svd_WakeupCallback != NULL)
        {
            p_Svd_WakeupCallback();
        }
	}

	//wakeup_pin 唤醒
    extern pFunType_void p_Wkupen_WakeupCallback;
	if(aon_wakeup_status_reg & WAKEUP_STATUS_EXTPIN_WKUP_Msk)
	{
		PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_EXTPINT);
		PRCM_AonWkupSourceEnable(AON_WKUP_SOURCE_EXTPINT);
        //函数指针非空，则调用回调函数
		if(p_Wkupen_WakeupCallback != NULL)
        {
            p_Wkupen_WakeupCallback();
        }
	}

	//AGPI0~2（WKP1~3）唤醒
    extern void McuAgpi_Wakeup_Callback(uint8_t num);
    uint8_t agpi_wakeupsrc = ((aon_wakeup_status_reg >> 8) & 0x07);
	if(agpi_wakeupsrc)
	{
		for(uint8_t i = 0; i < 3; i++)
		{
			if(agpi_wakeupsrc & (1 << i)) //判断AGPIO引脚WAKEUP中断标志的有效位
			{
				AGPI_WakeupDisable(i);
				AGPI_WakeupEnable(i);
                McuAgpi_Wakeup_Callback(i); //AGPIO的WAKEUP中断服务函数
			}
		}
	}

	//GPI0-7唤醒
    extern void McuGpi_Wakeup_Callback(uint8_t num);
    if(gpi_wakeupsrc)
	{
        for(uint8_t i = 0; i < 8; i++)
        {
			if(gpi_wakeupsrc & (1 << i)) //判断GPI引脚WAKEUP中断标志的有效位
			{
				GPI_WakeupDisable(i);
				GPI_WakeupEnable(i);
                McuGpi_Wakeup_Callback(i); //GPI的WAKEUP中断服务函数
			}
		}
	}

	// //写1清除中断位，避免反复进入该中断处理函数
	// AONPRCM->WAKUP_INT0 = 0x01;	
	// //等待中断位清除完成，该位置1后可再次进入中断处理函数
    // while((AONPRCM->WAKUP_INT0 & 0x01) != 0)
	// {
	// 	AONPRCM->WAKUP_INT0 = 0x01;//该标志位有概率出现写1一次清不掉的情况，这里多次写1直到清掉为止
	// }
}

/*目前只有CP核会使用UTC，AP核在OPENCPU场景下仅触发pendSv来boot_CP*/
void UTC_Handler(void)
{
	/*清中断*/
	HWREG(UTC_INT_STAT);

	/*目前只有CP核会使用UTC，AP核在OPENCPU场景下仅触发pendSv来boot_CP*/
	if (CP_Is_Alive() != true)
    {
		Trigger_PendSV();
    }
}

extern pFunType_void  p_WDT_Int_func;
void WDT_Handler(void)
{
	WDT_ReadClearInt(AP_WDT);
	if(p_WDT_Int_func != NULL)
    {
		p_WDT_Int_func();
    }
}

extern volatile uint32_t g_rc32k_cali_flag;
extern volatile uint32_t g_rc32k_aging_flag;
volatile uint32_t record_mcnt_handler_tick = 0;
void MCNT_Handler(void)
{
	/*中断函数中禁止运行flash代码，此处添加保护断言*/
	NVIC_ClearPendingIRQ(MCNT_IRQn);

	// 自校准处于暂停模式，等待mcnt中断触发
	if( g_rc32k_cali_flag == 2)
	{
		g_rc32k_cali_flag = 3;
		record_mcnt_handler_tick = Get_Tick();
	}

	// 通知老化流程的mcnt测量已完成
	if( g_rc32k_aging_flag == 1)
	{
		g_rc32k_aging_flag = 2;
	}

}
