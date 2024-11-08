/**
 * @file        data_gather.c
 * @brief       该DEMO模拟表计类客户的OPENCPU场景，即当有用量时，外部传感器通过wakeup_pin唤醒AP核采集数据，AP核完成本次采集后将数据保存到BACKUP_MEM中（深睡不掉电）后，进入深睡；\n
 ***            若满足某条件时，如半天超时，再如数据长度超过阈值等，则由AP核动态加载CP，通过AT命令通知CP核进行远程云通信。 	\n
 ***            此处的DEMO是当retention memory达到阈值后写flash，发送数据时从flash取数据；客户也可以自行修改，直接将retention memory内存拷贝出来后直接通过CP核发送。\n
 ***            方案设计中，数据采集部分在中断函数中执行，数据回写flash和AT通信在main函数中执行，进而就保证了耗时长的部分不会影响数据的及时采集。
 * @attention   芯翼芯片在深睡或软重启时，只有BACKUP_MEM(retention memory)内存是不断电，内容始终有效，其他上电情况下，BACKUP_MEM是清零的。总的大小为USER_BAK_MEM_LEN，切不可越界！！！
 * @warning     对于flash内容的跨核传递，必须通过retention memory头部8个字节告知CP核的flash首地址及尾地址，以便环形读取内容，用户不得占用retention memory头部8个字节！！！
 */

#include "xy_timer.h"
#include "sys_ipc.h"
#include "xy_flash.h"
#include "hal_def.h"
#include "hal_gpio.h"
#include "hw_types.h"
#include "xy_memmap.h"
#include "xy_system.h"
#include <string.h>
#include "xy_printf.h"
#include "user_config.h"
#include "data_gather.h"
#include "cloud_process.h"
#include "hal_def.h"


#if SAVE_IN_FLASH
/*正在发送的数据长度及缓存，最终根据发送结果更新bkmem和flash中状态和偏移*/
uint32_t g_sending_len = 0;

/*用于记录动态申请的地址，发送结束后内部释放内存*/
uint8_t *g_sending_data = NULL;

/* 正在发送的g_sending_data对应的副本所在位置，以方便发送成功或失败时对原缓存进行操作 */
Sending_LocatDef  g_LocationOfSendingData = LOCAT_NULL;


/*用户数据管理结构体+用户容错等私有信息，保存在retention memory头部*/
/* @warning  HAL_SOFT_RESET、APL_WDT_RESET、HAL_UTC_WAKEUP、HAL_EXTPIN_WAKEUP四种上电情况下，retention memory内存保持不变，仍然有效，客户使用时需要注意*/
User_Control_Block_T *g_Control_Context = (User_Control_Block_T *)USER_BAK_MEM_BASE;

/**
 * @brief 对采集数据保存到bkmem中，若达到阈值，则触发main主线程执行回写flash动作
 * @warning  由于产品差异，需要用户自行修改数据采集的接口及临界条件
 */
__RAM_FUNC void Save_Data_2_BakMem(void *gather_data,uint32_t gather_len)
{

#if (!FLASH_OPTEST)
	//xy_printf("Save Gathered Data into bkmem\n");
#endif

	//若bkmem已满，则需要用户自行修改回写flash的触发条件，如将触发阈值(gather_len*5)改大，以加快回写flash
	if(g_Control_Context->bkmem_saved_size + gather_len > BKMEM_USABLE_SAVE_SIZE)
	{
		xy_assert(0);
	}
	//bkmem中数据已达到阈值，通知main线程回写flash
#if FLASH_OPTEST
	else if(g_Control_Context->bkmem_saved_size + gather_len*10 > BKMEM_USABLE_SAVE_SIZE)
	{
		Send_AT_to_Ext("Save Gathered Data into flash\n");
#else
	else if(g_Control_Context->bkmem_saved_size + gather_len*5 > BKMEM_USABLE_SAVE_SIZE)
	{
#endif
		memcpy(((uint8_t *)(BKMEM_STORE_DATA_ADDR+g_Control_Context->bkmem_saved_size)),gather_data,gather_len);
		g_Control_Context->bkmem_saved_size += gather_len;

		set_event(EVENT_SAVE_DATA);
	}

	//缓存采集到的数据到bkmem中
	else
	{
		memcpy(((uint8_t *)(BKMEM_STORE_DATA_ADDR+g_Control_Context->bkmem_saved_size)),gather_data,gather_len);
		g_Control_Context->bkmem_saved_size += gather_len;
	}
}

/**
 * @brief 将RAM中的数据保存到flash环形空间中，若存满则会强行覆盖最旧数据。\n
 * 考虑到用户数据可能因为断电、写满覆盖等原因造成的部分脏数据，建议客户每次采集的数据通过魔术数字进行隔离，在云端服务器层面进行数据的有效性筛选。
 * @warning 由于发送耗时可能达到分钟级，而期间可能仍然有大量采集数据进行保存，进而可能造成数据写满覆盖，该问题无法根本解决，即使加大环形缓存大小也不行。
 */
void Write_To_Flash(void *write_src, uint32_t write_size)
{
	uint32_t Base_Addr = RINGBUF_FLASH_BASE;

    //如果要写入的长度需要从头覆盖，则环形写入，否则顺序写；若所写长度超过剩余可写长度，则会造成最旧的数据被部分覆盖！！！
    if(write_size > (RINGBUF_FLASH_SIZE - g_Control_Context->flash_write_pos))
    {
		xy_assert(xy_Flash_Write(Base_Addr + g_Control_Context->flash_write_pos, write_src, RINGBUF_FLASH_SIZE - g_Control_Context->flash_write_pos) != false);
        xy_assert(xy_Flash_Write(Base_Addr, (void *)((uint32_t)write_src + RINGBUF_FLASH_SIZE - g_Control_Context->flash_write_pos), write_size - RINGBUF_FLASH_SIZE + g_Control_Context->flash_write_pos) != false);
        g_Control_Context->flash_write_pos = write_size - (RINGBUF_FLASH_SIZE - g_Control_Context->flash_write_pos);
    }
    else
    {
        xy_assert(xy_Flash_Write(Base_Addr + g_Control_Context->flash_write_pos, write_src, write_size) != false);
        g_Control_Context->flash_write_pos = g_Control_Context->flash_write_pos + write_size;
    }

	//如果要写入的长度小于剩余还能写入的长度,计数累加
    if((uint16_t)write_size <= RINGBUF_FLASH_SIZE - g_Control_Context->flash_saved_size )
	{
		g_Control_Context->flash_saved_size +=write_size;
    }

	//若写溢出，则覆盖最旧数据，保留最新数据；同时read指针指向最旧数据头部
    else
    {
        g_Control_Context->flash_saved_size = RINGBUF_FLASH_SIZE ;
        g_Control_Context->flash_read_pos = g_Control_Context->flash_write_pos ;
    }
}


/**
 * @brief 根据云发送结果，更新BKmem_header里的状态值，失败时不能清空缓存数据
 * @param send_succ: 1，发送成功，需要清空已发送数据；0，发送失败，需要缓存住数据
 */
__WEAK void Update_Info_By_Send_Result(uint32_t send_succ)
{
	/* 发送bkmem数据成功 */
	if(g_LocationOfSendingData==LOCAT_BKMEM && send_succ==1)
	{
		if(g_sending_len != g_Control_Context->bkmem_saved_size)
		{
			memcpy((void *)(BKMEM_STORE_DATA_ADDR),(void *)(BKMEM_STORE_DATA_ADDR+g_sending_len),g_Control_Context->bkmem_saved_size-g_sending_len);
			g_Control_Context->bkmem_saved_size = g_Control_Context->bkmem_saved_size - g_sending_len;
		}
		else
			g_Control_Context->bkmem_saved_size = 0;
	}

	//flash中缓存发送成功，需要更新flash发送偏移
	else if(send_succ==1 && g_LocationOfSendingData==LOCAT_FLASH)
	{
		g_Control_Context->flash_read_pos = (g_Control_Context->flash_read_pos + g_sending_len) % RINGBUF_FLASH_SIZE;
        g_Control_Context->flash_saved_size -= g_sending_len;
	}

	g_LocationOfSendingData = LOCAT_NULL;
	g_sending_len = 0;
	if(g_sending_data != NULL)
		xy_free(g_sending_data);
	g_sending_data = NULL;
}

/**
 * @brief 按照指定上限长度，读取待发送的数据缓存。数据缓存读取顺序为：上次未发送成功的数据换、flash中缓存的旧数据、bakmem中缓存的数据。
 * @param max_len: 读取的数据最大长度，一般为云发送AT命令的最大长度
 * @param addr: 数据的目标缓存，接口内部动态申请，使用者需要主动释放该堆内存
 * @param len:  实际读取的数据长度，与addr配对
 * @warning 该函数的目的就是读取待发送的数据，当存在多个数据源时，如何决策哪个优先读取发送，由客户自行编程实现。
 * @attention 返回0表示当前没有待发送的缓存。如果用户确定所有待发送的数据源都已发送完成，内部执行释放EVENT_CLOUD_SEND事件。
 */
__WEAK uint32_t Get_Data_To_Send(uint32_t max_len, void **addr, uint32_t *len)
{
	uint32_t  flash_head = 0;

	//上次发送失败的缓存
	if(g_sending_data!=NULL && g_sending_len!=0)
	{
		xy_assert(g_sending_len <= max_len);
		
		*addr = g_sending_data;
		*len = g_sending_len;

		return  g_sending_len;
	}
	
	/* 优先发送flash中旧数据 */
	else if(g_Control_Context->flash_saved_size)
	{
		if(max_len >= g_Control_Context->flash_saved_size)
		{
			flash_head = g_Control_Context->flash_read_pos + RINGBUF_FLASH_BASE;
			*len = g_Control_Context->flash_saved_size;
		}
		else
		{
			flash_head = g_Control_Context->flash_read_pos + RINGBUF_FLASH_BASE;
			*len = max_len;
		}
		
		*addr = xy_malloc(*len + 1);
		g_LocationOfSendingData = LOCAT_FLASH;
		
		//读取的数据没有回到环形队列的开头
		if(flash_head + *len <= RINGBUF_FLASH_BASE + RINGBUF_FLASH_SIZE)
		{
			xy_Flash_Read(flash_head, *addr, *len);
		}
		//读取的数据回到环形队列的开头
		else
		{
			xy_Flash_Read(flash_head, *addr, (RINGBUF_FLASH_BASE + RINGBUF_FLASH_SIZE - flash_head));
			xy_Flash_Read(RINGBUF_FLASH_BASE, ((char*)*addr + RINGBUF_FLASH_BASE + RINGBUF_FLASH_SIZE - flash_head), *len + flash_head - RINGBUF_FLASH_BASE - RINGBUF_FLASH_SIZE);
		}
	}
	/* flash中无缓存数据，再发送BKmem中缓存数据 */
	else
	{
		//BKmem中有缓存数据
		if(g_Control_Context->bkmem_saved_size != 0)
		{
			uint32_t  saved_len = 0;

			saved_len = g_Control_Context->bkmem_saved_size>max_len ? max_len:g_Control_Context->bkmem_saved_size;

			*addr = (uint8_t *)xy_malloc(saved_len);

			//将该段数据拷贝至sram上，以尽快开中断，防止外部中断丢失过多
			memcpy((void *)(*addr), (void *)(BKMEM_STORE_DATA_ADDR), saved_len);

			*len = saved_len;
			g_LocationOfSendingData = LOCAT_BKMEM;
		}
		else
		{
			*addr = NULL;
			*len = 0;
			xy_assert(g_LocationOfSendingData==LOCAT_NULL && g_sending_len==0 && g_sending_data==NULL);
		}
	}

	g_sending_data = *addr;
	g_sending_len = *len;

	return  g_sending_len;
}

/**
 * @brief 将bkmem中的数据保存到flash中。由于该接口运行在main函数中，而数据采集外部中断还会继续，需要考虑中断切换的影响
 */
__WEAK void Save_Bkmem2Flash(void)
{
	if(g_Control_Context->bkmem_saved_size == 0)
	{
		return ;
	}
	/* 若从bkmem取数据发送过程中出现bkmem满了,则写入flash，此时flash一定是空的，且正在发送的数据一定位于flash头部 */
	if(g_LocationOfSendingData == LOCAT_BKMEM)
		g_LocationOfSendingData = LOCAT_FLASH;

	Write_To_Flash((void *)(BKMEM_STORE_DATA_ADDR),g_Control_Context->bkmem_saved_size);
	g_Control_Context->bkmem_saved_size = 0;
}

#else


/*供用户定制自己的离散发送数据源，最终只需把地址和长度返回给调用者即可。如果用户确定所有待发送的数据源都已发送完成，内部执行释放EVENT_CLOUD_SEND事件。*/
 __WEAK uint32_t Get_Data_To_Send(uint32_t max_len, void **addr, uint32_t *len)
{
	UNUSED_ARG(max_len);
	UNUSED_ARG(addr);
	UNUSED_ARG(len);
	return 0;
}

__WEAK void Update_Info_By_Send_Result(uint32_t send_succ)
{
	UNUSED_ARG(send_succ);
}
#endif
