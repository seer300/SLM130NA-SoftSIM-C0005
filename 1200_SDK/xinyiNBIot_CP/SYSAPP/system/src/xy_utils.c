#include "xy_utils.h"
#include "sema.h"
#include "trng.h"
#include "prcm.h"
#include "dma.h"
#include "rtc_tmr.h"

#define XY_RAND_MAX (0xffffffff)
uint64_t xy_rand_next = 0;

/* "AB235E"---->AB235E(3 BYTES) */
int hexstr2bytes(char* src, int src_len, char* dst, int dst_size)
{
	int i;

	if (src ==  NULL || dst == NULL || src_len < 0 || dst_size < (src_len + 1) / 2) {
		xy_assert(0);
	}

	for (i = 0; i < src_len; i += 2) {
		if(*src >= 'a' && *src <='f')
			*dst = ((*src - 'a') + 10) << 4;
		else if (*src >= '0' && *src <= '9') {
			*dst = (*src - '0') << 4;
		} else if (*src >= 'A' && *src <= 'F') {
			*dst = ((*src - 'A') + 10) << 4;
		} else {
			return -1;
		}

		src++;
		if(*src >= 'a' && *src <= 'f')
			*dst |= ((*src - 'a') + 10);
		else if (*src >= '0' && *src <= '9') {
			*dst |= (*src - '0');
		} else if (*src >= 'A' && *src <= 'F') {
			*dst |= ((*src - 'A') + 10);
		} else {
			return -1;
		}

		src++;
		dst++;
	}

	return src_len / 2;
}

/* AB235E(3 BYTES)---->"AB235E" */
int bytes2hexstr(unsigned char* src, signed long src_len, char* dst, signed long dst_size)
{
	const char tab[] = "0123456789ABCDEF";
	signed long i;

	if (src ==  NULL || dst == NULL || src_len < 0 || dst_size <= src_len * 2) {
		xy_assert(0);
	}

	for (i = 0; i < src_len; i++) {
		*dst++ = tab[*src >> 4];
		*dst++ = tab[*src & 0x0f];
		src++;
	}

	*dst = '\0';

	return src_len * 2;
}

/*"0XA358"--->0XA358 */  
bool hexstr2int(char *hex,int *value)  
{  
    int len;   
    int temp;  
    int val=0;
    int i=0;  

	if(strncmp(hex,"0x",2)==0 || strncmp(hex,"0X",2)==0)
		hex+=2;
	else
		return 0;
	
	len = strlen(hex); 
	while(!(hex[i]<'0' || (hex[i] > '9'&&hex[i] < 'A') || (hex[i] > 'F' && hex[i] < 'a') || hex[i] > 'f'))
		i++;
    if(i < len)
		return 0;

    for (i=0, temp=0; (i<len && i<8); i++)  
    {  
			if(isdigit((int)(hex[i])))  
            	temp = (hex[i] - 48);

    		if(isalpha((int)(hex[i])))  
            	temp =  (isupper((int)(hex[i])) ? hex[i] - 55 : hex[i] - 87);
    		val = val*16+temp;
    }   
    *value = val;
	return  1;
}

uint32_t get_trng_rand_value(uint8_t* readbuff)
{
	uint32_t roscLength = 0;
	uint32_t sampleCount  = 27;
	volatile int err;

	SEMA_Request(SEMA_MASTER_CP, SEMA_SLAVE_TRNG, SEMA_SEMA_DMAC_NO_REQ, SEMA_SEMA_REQ_PRI_0, SEMA_MASK_AP);

	PRCM_ClockEnable(CORE_CKG_CTL_TRNG_EN);

	TRNG_initializingHardware(TRNG_MODE_FAST,roscLength,sampleCount);
	err = TRNG_loadEHR(readbuff);
	TRNG_stopHardware();

	PRCM_ClockDisable(CORE_CKG_CTL_TRNG_EN);

	SEMA_Release(SEMA_SLAVE_TRNG, SEMA_MASK_AP);

	return err;
}

//获取随机数种子
uint32_t xy_seed(void)
{
	uint32_t trng_buffer[EHR_SIZE_IN_WORDS] = {0};
	while(get_trng_rand_value((uint8_t *)trng_buffer) != 0);

	return trng_buffer[0];
}

//初始化随机数
void xy_srand(uint32_t seed)
{
	xy_rand_next = seed;
}

//获取随机数
uint32_t xy_rand(void)
{
	xy_rand_next = xy_rand_next * 6364136223846793005LL + 1;
	return (uint32_t)((xy_rand_next >> 32) & XY_RAND_MAX);
}

uint32_t xy_chksum(const void *dataptr, int len)
{
    uint32_t acc;
    uint16_t src;
    uint8_t *octetptr;

    acc = 0;
    octetptr = (uint8_t *)dataptr;
    while (len > 1) {
        src = (*octetptr) << 8;
        octetptr++;
        src |= (*octetptr);
        octetptr++;
        acc += src;
        len -= 2;
    }
    if (len > 0) {
        src = (*octetptr) << 8;
        acc += src;
    }

    acc = (acc >> 16) + (acc & 0x0000ffffUL);
    if ((acc & 0xffff0000UL) != 0) {
        acc = (acc >> 16) + (acc & 0x0000ffffUL);
    }
	if(acc==0 || acc==0XFFFFFFFF)
		acc = 1;
     return ~acc;
}

uint32_t DMA_Memcpy_SYNC(uint32_t DstAddress, uint32_t SrcAddress, uint32_t DataLength, uint32_t Timeout)
{
	// 限制使用DMA_CHANNEL_6
	uint32_t ChannelNum = DMA_CHANNEL_6;
	uint64_t tickstart = 0U;
	
	//传输长度检查，单次传输长度需小于0x100000个字节;超时时间检查;
	if((DataLength >= 0x100000) || (Timeout == 0))
	{
		return (1);
	}

	//DMA 可用的通道数紧张，暂时不用DMA做memcpy
	memcpy((void *)DstAddress,(void *)SrcAddress,DataLength);
	return (0);
}

void delay_us(uint32_t us)
{
	if(!(SysTick->CTRL & SysTick_CTRL_ENABLE_Msk))
	{
		SysTick->LOAD  = SysTick_LOAD_RELOAD_Msk - 1;                         /* set reload register */
		SysTick->VAL   = 0UL;                                                           /* Load the SysTick Counter Value */
		SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;          /* Enable SysTick IRQ and SysTick Timer */
	}

	uint32_t t_start = SysTick->VAL;

	//2 / 7是3.5分频，改了分频这里需要更改
#define  SYSTICK_PER_US  ((368640 * 2 / 7 + 1)/1000 + 1)
#define  MAX_SYSTICK     (0XFFFFFFUL/SYSTICK_PER_US)

	if(us <= MAX_SYSTICK)
	{
		uint32_t t_reload = SysTick->LOAD;
		uint32_t t_tick = us * SYSTICK_PER_US;
		uint32_t t_stop, t_needreversal;
		volatile uint32_t cur_tick;
		volatile uint32_t last_tick = SysTick->VAL;
		volatile uint32_t reversal_flag = 0;

		if(t_start >= t_tick)
		{
			t_stop = t_start - t_tick;
			t_needreversal = 0;;
		}
		else
		{
			t_stop = t_reload + t_start - t_tick;
			t_needreversal = 1;
		}

		while(1)
		{
			cur_tick = SysTick->VAL;

			if(cur_tick > last_tick)
			{
				reversal_flag++;
			}

			last_tick = cur_tick;

			if(((reversal_flag == t_needreversal) && (cur_tick <= t_stop)) || (reversal_flag > t_needreversal))
			{
				break;
			}
		}
	}
	else
	{
		extern volatile uint32_t AbsoluteTickOverflowCount;
		extern TickType_t vPortGetAbsoluteTick( void );
		extern volatile unsigned int g_freq_32k;
		xy_assert(g_freq_32k != 0);
		uint64_t curr_tick, over_tick = us / 1000 + vPortGetAbsoluteTick() + (((uint64_t)AbsoluteTickOverflowCount) << 32);
		do
		{
			curr_tick = vPortGetAbsoluteTick() + (((uint64_t)AbsoluteTickOverflowCount) << 32);
		}
		while(over_tick > curr_tick);
	}
}



