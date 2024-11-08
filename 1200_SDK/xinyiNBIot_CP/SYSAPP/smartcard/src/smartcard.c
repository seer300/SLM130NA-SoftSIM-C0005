#include "smartcard.h"
#include "cmsis_os2.h"
#include "xy_utils.h"
#include "gpio.h"
#include "iso7816.h"
#include "xy_log.h"
#include "sys_config.h"
#include "softsim_adapt.h"




SimCard_Item SC7816_Item;

uint8_t ATR_Rsp[MAX_ATR_CHARACTER_LEN];

volatile uint8_t smartcard_config=0;
osMutexId_t	g_smartcard_mutex = NULL;

osSemaphoreId_t g_smartcard_sem = NULL;
extern volatile unsigned char g_uicc_focus;


#define SM_GPIO_CONFIG	0x01
#define SM_INT_CONFIG	0x02


static uint8_t s_smartcard_sleep_enable = 1;

//debug code
#if SIM_DEBUG_CODE
volatile uint32_t tickcnt_before_iso7816_recv = 0;
volatile uint32_t tickcnt_after_iso7816_recv = 0;

volatile uint32_t tickcnt_before_iso7816_send = 0;
volatile uint32_t tickcnt_after_iso7816_send = 0;

volatile uint32_t g_iso7816_debug_timeout = 0;
volatile uint8_t g_iso7816_debug_plllock = 0;
volatile uint8_t g_iso7816_debug_sysclk = 0;
extern unsigned int TickCounterGet(void);

void iso7816_debug_record_info(void)
{
    g_iso7816_debug_plllock = COREPRCM->ANAXTALRDY;
    g_iso7816_debug_sysclk = (uint8_t)COREPRCM->SYSCLK_FLAG;
}
#endif

void Smartcard_Sleep_Disable()
{
	s_smartcard_sleep_enable = 0;
}

void Smartcard_Sleep_Enable()
{
	s_smartcard_sleep_enable = 1;
}

uint8_t Smartcard_Sleep_Allow_Get()
{
	if ( Is_softsim_type() )
		return 1; // SoftSIM
	else 
		return s_smartcard_sleep_enable;
}


//void SIM_SmartCard_WT(unsigned char aa)
//{
//	return ;
//}

void SIM_TxAllOut(void)
{
	/*
	while(ISO7816_GetFifoEmptyFlag() == 0);
	while(ISO7816_GetFifoByteNum() != 0);
	while(HWREGB(SM_DBGSTAT) & 0x3);
	*/
	ISO7816_SwitchToRxFromTx();
}



uint32_t Etu_2_Clk(uint32_t etu)
{
	//return (uint32_t)((SC7816_Item.profile.Fi*etu)/SC7816_Item.profile.Di);
    return (uint32_t)((SC7816_Item.profile.F*etu)/SC7816_Item.profile.D);
}

uint32_t SMClk_2_Etu(uint32_t sm_clk_num)
{
	uint32_t etu_num;
	etu_num = (uint64_t)sm_clk_num*SC7816_Item.profile.D/SC7816_Item.profile.F + 1;
	return etu_num;
}

uint32_t SMClk_2_Tick(uint32_t sm_clk_num)
{
	uint32_t tick;
	tick = (uint64_t)sm_clk_num*SC7816_Item.profile.clk_div*1000/SIM_APB_CLK + 1;
	return tick;
}

void SimCard_ClkDelay(uint16_t clk)
{
    /*delay by idle etu*/
	ISO7816_IntStatClr(SM_INTENA_IDLE_Msk);
    ISO7816_IdleETUSet(SMClk_2_Etu(clk));
    while(!ISO7816_IntStatGet(SM_INTENA_IDLE_Msk));
    ISO7816_IntStatClr(SM_INTENA_IDLE_Msk);
    ISO7816_IdleETUSet(0);
	
}
void __RAM_FUNC SimCard_IntHandler(void)
{
#if RUNTIME_DEBUG
	extern uint32_t xy_runtime_get_enter(void);
	uint32_t time_enter = xy_runtime_get_enter();
#endif
	uint8_t sim_int_enable;

	sim_int_enable = ISO7816_GetIntEna_ISR();
	ISO7816_IntDisAll_ISR();				//disable all
	ISO7816_IntClr_ISR(sim_int_enable);//clr int which is enabled

	osSemaphoreRelease(g_smartcard_sem);

#if RUNTIME_DEBUG
	extern void xy_runtime_get_exit(uint32_t id, uint32_t time_enter);
	xy_runtime_get_exit(ISO7816_IRQn, time_enter);
#endif
}

void SimCard_Init()
{	
	GPIO_InitTypeDef gpio_init = {0};

	//unsigned char sim_clk_pin,sim_rst_pin,sim_data_pin;

	/*
		sim_rst_pin = g_factory_nv->softap_fac_nv.sim_rst_pin;
		sim_clk_pin = g_factory_nv->softap_fac_nv.sim_clk_pin;
		sim_data_pin = g_factory_nv->softap_fac_nv.sim_data_pin;
	*/
	//if(0 == (smartcard_config&SM_GPIO_CONFIG))
	{
		GPIO_AllocateRemove(GPIO_SM_CLK);
		GPIO_AllocateRemove(GPIO_SM_RST);
		GPIO_AllocateRemove(GPIO_SM_SIO);

		gpio_init.Pin = GPIO_PAD_NUM_15;
		gpio_init.PinRemap = GPIO_SM_CLK;
		gpio_init.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&gpio_init);
		GPIO_InputPeriSelect(GPIO_PAD_NUM_15, GPIO_SM_CLK);
		GPIO_InputPeriSelectCmd(GPIO_SM_CLK, ENABLE);

		gpio_init.Pin = GPIO_PAD_NUM_16;
		gpio_init.PinRemap = GPIO_SM_RST;
		gpio_init.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&gpio_init);
		GPIO_InputPeriSelect(GPIO_PAD_NUM_16, GPIO_SM_RST);
		GPIO_InputPeriSelectCmd(GPIO_SM_RST, ENABLE);

		gpio_init.Pin = GPIO_PAD_NUM_17;
		gpio_init.PinRemap = GPIO_SM_SIO;
		gpio_init.Mode = GPIO_MODE_HW_PER;
		GPIO_Init(&gpio_init);
		GPIO_InputPeriSelect(GPIO_PAD_NUM_17, GPIO_SM_SIO);
		GPIO_InputPeriSelectCmd(GPIO_SM_SIO, ENABLE);

		//smartcard_config |= SM_GPIO_CONFIG;
		
		
	}
	//HWREGB(SM_ACTIVATE) |= SM_ACTIVATE_ISO7816RST_Msk;
	ISO7816_Reset();
	//HWREGH(SM_IDLETU) = 0;//default 255
	ISO7816_IdleETUSet(0);
	//HWREGB(SM_INTENA) = 0;
	ISO7816_IntDisable(SM_INTENA_ALL_Msk);
	NVIC_IntRegister(ISO7816_IRQn,SimCard_IntHandler,1);
	//SimCard_Deactivation();
}

uint16_t sim_rx_num 		=	0;


//#define SIM_FIFO_DEEP	64

T_Response_APDU RxAPDU	=	{0};
uint8_t rcv_in_interrupt=0;
uint8_t sim_le_ins;
uint16_t sim_Le_sw=0;

void SIM_SetGuardTime( uint16_t extra_guard_bits)
{
	//stopbits num only support 1-4

	if(extra_guard_bits <= 4)
	{
		ISO7816_StopbitsSet(extra_guard_bits);
		ISO7816_TxETUWaitSet(0);
		ISO7816_TxRetryETUWaitDis();
	}
	else
	{
		ISO7816_StopbitsSet(4);
		ISO7816_TxETUWaitSet(extra_guard_bits-4);
		ISO7816_TxRetryETUWaitEn();
	}
	
}

void SIM_SetRetryNum( uint8_t retry_num)
{
	/*
	if(retry_num)
		HWREGB(SM_TRXRETRY) = (1<<3) | retry_num;
	else
		HWREGB(SM_TRXRETRY) = 0;
	*/
	ISO7816_TRxRetrySet(retry_num);
}

uint32_t SimCard_GetClkDiv()
{

	uint32_t clk_div;
	
	clk_div = ISO7816_ClkDivGet();
	if(clk_div == 0)
		clk_div = 1;
	else
		clk_div *= 2; 
	return clk_div;
}



uint8_t Character_Receive(uint8_t *Data, uint32_t TimeOut)
{
#if (SIM_INT_EN==1)
	osStatus_t retv;
	if(ISO7816_GetFifoEmptyFlag())
	{
		ISO7816_FifoLevelSet(0x01);
		ISO7816_IntStatClr(SM_INTENA_FIFO_LEVEL_Msk);
		ISO7816_IntEnable(SM_INTENA_FIFO_LEVEL_Msk);

		//二次判断fifo是防止在一次判断和清除intstat之间来了数据,导致int被清除
		if(ISO7816_GetFifoEmptyFlag())
		{
			TimeOut = SMClk_2_Tick(TimeOut);
            
#if SIM_DEBUG_CODE
            tickcnt_before_iso7816_recv = TickCounterGet();
#endif	       

			retv = osSemaphoreAcquire(g_smartcard_sem, TimeOut);
			//maybe already being disabled in handler
			ISO7816_IntDisable(SM_INTENA_FIFO_LEVEL_Msk);
            
			if(retv == osOK)
			{
				//interrupt occurs,cleared in inthandler
				*Data = ISO7816_ByteGet();
        		return SC_SUCCESS; 
			}
			else
			{
#if SIM_DEBUG_CODE
            tickcnt_after_iso7816_recv = TickCounterGet();
            g_iso7816_debug_timeout = TimeOut;
            iso7816_debug_record_info();
#endif	
				//临界处理
				while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
				return SC_FAILURE; 
			}
			
		}
		else
		{
			ISO7816_IntDisable(SM_INTENA_FIFO_LEVEL_Msk);
			//临界处理
			while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
			
			*Data = ISO7816_ByteGet();
        	return SC_SUCCESS; 
		}
	}
	else
	{
		*Data = ISO7816_ByteGet();
        return SC_SUCCESS; 
	}
	
#else

	uint32_t Counter = 0;
//	TimeOut=(TimeOut/WHILE_DELAY_CLK)*SC7816_Item.profile.clk_div*XY_HCLK_DIV*XY_PCLK_DIV/XY_CPCLK_DIV;
	
	TimeOut = 100000000;

    while((ISO7816_GetFifoEmptyFlag()) == 1 && (Counter < TimeOut))
    {
    		Counter++;
    }
	
    if(Counter < TimeOut)
    {
        *Data = ISO7816_ByteGet();
		
        return SC_SUCCESS;  
    }
    else
    {   	
        return SC_FAILURE; 
    }
	
#endif	
}

#if (SIM_INT_EN==1)
static uint8_t Character_Send_Blocking(uint8_t *txbuff, uint32_t txlen)
{
	uint32_t txnum = 0;
	osStatus_t retv;
	//发送TimeOut 5倍余量
	uint32_t TimeOut = SMClk_2_Tick(Etu_2_Clk(SIM_FIFO_MAXNUM+2)*12) * 5;
	while(1)
	{
		osCoreEnterCritical();
		
		while(!ISO7816_GetFifoFullFlag() && (txnum < txlen))
		{
			ISO7816_BytePut(txbuff[txnum]);
			txnum++;
		}

		if(txnum < txlen)
		{
			//fifo full

			ISO7816_FifoLevelSet(SIM_FIFO_MAXNUM/8);//level = 16
			ISO7816_IntStatClr(SM_INTENA_FIFO_LEVEL_Msk);
			ISO7816_IntEnable(SM_INTENA_FIFO_LEVEL_Msk);

			if(ISO7816_GetFifoByteNum() <= SIM_FIFO_MAXNUM/8)
			{
				ISO7816_IntDisable(SM_INTENA_FIFO_LEVEL_Msk);
				osCoreExitCritical();
				//临界处理
				while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
				
			}
			else
			{
				osCoreExitCritical();
#if SIM_DEBUG_CODE
                    tickcnt_before_iso7816_send = TickCounterGet();
#endif	
				retv = osSemaphoreAcquire(g_smartcard_sem, TimeOut);
				ISO7816_IntDisable(SM_INTENA_FIFO_LEVEL_Msk);
				
				if(retv != osOK)
				{
#if SIM_DEBUG_CODE
                    tickcnt_after_iso7816_send = TickCounterGet();
                    g_iso7816_debug_timeout = TimeOut;
                    iso7816_debug_record_info();
#endif	
					//xy_assert(0);
					while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
					PrintLog(0, PLATFORM, WARN_LOG, "uSim tx fail pos1,fifo_num=%x,SM_DBGSTAT=%x",ISO7816_GetFifoByteNum(),HWREGB(SM_DBGSTAT));
					return SC_FAILURE;
				}
			}
		}
		else
		{
			break;//exit while1
		}
	}

	if(ISO7816_GetFifoEmptyFlag())
	{
		osCoreExitCritical();
		return SC_SUCCESS;
	}
	else
	{
		ISO7816_IntStatClr(SM_INTENA_FIFO_EMPTY_Msk);
		ISO7816_IntEnable(SM_INTENA_FIFO_EMPTY_Msk);

		if(ISO7816_GetFifoEmptyFlag())
		{
			ISO7816_IntDisable(SM_INTENA_FIFO_EMPTY_Msk);
			osCoreExitCritical();
			//临界处理
			while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
		}
		else
		{
			osCoreExitCritical();
#if SIM_DEBUG_CODE
            tickcnt_before_iso7816_send = TickCounterGet();
#endif	
			retv = osSemaphoreAcquire(g_smartcard_sem, TimeOut);
			ISO7816_IntDisable(SM_INTENA_FIFO_EMPTY_Msk);
			
			if(retv != osOK)
			{
#if SIM_DEBUG_CODE
                tickcnt_after_iso7816_send = TickCounterGet();
                g_iso7816_debug_timeout = TimeOut;
                iso7816_debug_record_info();
#endif	
				//xy_assert(0);
				while(osSemaphoreAcquire(g_smartcard_sem, osNoWait) == osOK);
				PrintLog(0, PLATFORM, WARN_LOG, "uSim tx fail pos2,fifo_num=%x,SM_DBGSTAT=%x",ISO7816_GetFifoByteNum(),HWREGB(SM_DBGSTAT));
				return SC_FAILURE;
			}
		}

		return SC_SUCCESS;
	}
	
				

}
#else
static uint8_t Character_Send(uint8_t Data, uint32_t TimeOut)
{

	uint32_t Counter = 0;
	
	while((ISO7816_GetFifoFullFlag() == 1) && (Counter < TimeOut))
	{
		Counter++;
	}
	
	if(Counter != TimeOut)
	{
		
		ISO7816_BytePut(Data);
		return SC_SUCCESS;			
	}
	else
	{
		return SC_FAILURE;			
	}

}
#endif





uint8_t switch_next_state(uint8_t current_state, uint8_t T_bitmap)
{
	uint8_t current_bit = (current_state-STATE_ATR_PARSE_T0)%4;
	uint8_t bit_offset 	= 0;
	while((current_bit+bit_offset) < 4)
	{
		if(T_bitmap&(1<<(current_bit+bit_offset)))
		{
			return current_state+bit_offset+1;
		}
		bit_offset++;
	}
	return STATE_ATR_PARSE_HISTORY_BYTES;
}
#ifdef UNUSED_FUNCTION
uint8_t Get_Max_Div(uint16_t Max_frequency)
{

	//XY_PCLK/1000=92 160
	uint8_t clk_div=1;
	for (clk_div=1;clk_div<255;clk_div++)
	{
		if(Max_frequency*clk_div>=XY_PCLK/1000)
		{
			break;
		}
		
	}
	return clk_div;

}
#endif
uint32_t ATR_Character_Parse(uint8_t ATR_character, uint8_t *T_bitmap, uint8_t * history_bytes)
{
	uint8_t T_indicate = 0;
	const uint16_t Fi_Table[16] =
	{
		372, 	372, 	558, 	744, 
		1116, 	1488, 	1860, 	0,
		0, 		512, 	768, 	1024, 
		1536, 	2048, 	0, 		0
	};
	const uint8_t Di_Table[] = 
	{
		0,		1,		2,		4, 
		8,		16,		32,		64,
		12,		20,		0,		0,
		0,		0,		0,		0
	};
//	uint16_t F_Max_Div1000[]=
//	{
//		4000,		5000,		6000,		8000,
//		12000,		16000,		20000,		0,
//		0,			5000,		7500,		10000,
//		15000,		20000,		0,			0
//	};
		
	switch(SC7816_Item.atr_state)
	{
		case STATE_ATR_PARSE_TS:
			
			SC7816_Item.atr_state	=	STATE_ATR_PARSE_T0;
			break;
		
		case STATE_ATR_PARSE_T0:
			
			*T_bitmap				=	(ATR_character&0xF0)>>4;
			*history_bytes			=	ATR_character&0x0F;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);					
			break;
			
		case STATE_ATR_PARSE_TA1:

			SC7816_Item.profile.global_atrpara_present	|=	1<<ATR_GLOBAL_PARA_TA1_POS;
			
			SC7816_Item.profile.TA1	=	ATR_character;
			SC7816_Item.profile.Fi	=	Fi_Table[(ATR_character&0xF0)>>4];
			SC7816_Item.profile.Di	=	Di_Table[ATR_character&0x0F];

			//F,D may be modified in ATR_process in specific mode,and PPS shall use default values
			//SC7816_Item.profile.F	=	SC7816_Item.profile.Fi;
			//SC7816_Item.profile.D	=	SC7816_Item.profile.Di;

			if(SC7816_Item.profile.Fi == 0 || SC7816_Item.profile.Di == 0)
			{
				SC7816_Item.atr_state   =   STATE_ATR_FAILURE;
			}
			else
			{
				SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			}
			
			break;
			
		case STATE_ATR_PARSE_TB1:
			/* deprecated */
			SC7816_Item.profile.global_atrpara_present	|=	1<<ATR_GLOBAL_PARA_TB1_POS;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TC1:

			SC7816_Item.profile.global_atrpara_present	|=	1<<ATR_GLOBAL_PARA_TC1_POS;
			SC7816_Item.profile.N	=	ATR_character;

			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TD1:
			
			T_indicate	=	ATR_character&0x0F;
			*T_bitmap 	= 	(ATR_character&0xF0)>>4;
			if(T_indicate == PROTOCOL_T15)
			{
				/* T=15 is invalid in TD1. */
				T_indicate=0;
			}
			else
			{
				/*
				The “first offered transmission protocol” is defined as follows.
				If TD1 is present, then it encodes the first offered protocol T.
				If TD1 is absent, then the only offer is T=0.
				*/
				/*SC7816_Item.profile.T_indicated is initialized to be PROTOCOL_T0 in init_SimCard_Item()*/
				SC7816_Item.profile.T_indicated		&=	~(1<<PROTOCOL_T0);
				SC7816_Item.profile.T_indicated		|=	1<<T_indicate;
			}
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TA2:
			
			SC7816_Item.profile.global_atrpara_present	|=	1<<ATR_GLOBAL_PARA_TA2_POS;
			SC7816_Item.profile.TA2_SpecificMode		=	ATR_character;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TB2:
			/* deprecated */
			SC7816_Item.profile.global_atrpara_present	|=	1<<ATR_GLOBAL_PARA_TB2_POS;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TC2:
			/* specific to T0 */
			SC7816_Item.profile.WI	=	ATR_character;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TD2:
			
			T_indicate	=	ATR_character&0x0F;
			*T_bitmap 	= 	(ATR_character&0xF0)>>4;
			
			SC7816_Item.profile.T_indicated		|=		1<<T_indicate;
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);

			break;
			
		case STATE_ATR_PARSE_TAi:
			
			/*If TD1, TD2 and so on are present, the encoded types T shall be in ascending numerical order. If present, T=0
			shall be first, T=15 shall be last. T=15 is invalid in TD1.*/
			
			if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T15))
			{
				/*
				The first TA for T=15 encodes the clock stop indicator (X) and the class indicator (Y). The default values are
				X = “clock stop not supported” and Y = “only class A supported”.
				*/
				if( !(SC7816_Item.profile.T15_atrpara_present & (1<<ATR_SPECIFIC_PARA_TA_POS)))
				{
					SC7816_Item.profile.class_clock		=	ATR_character;
				}
				SC7816_Item.profile.T15_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TA_POS);
			}
			else if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T1))
			{
				if( !(SC7816_Item.profile.T1_atrpara_present & (1<<ATR_SPECIFIC_PARA_TA_POS)))
				{
					if(ATR_character >= 0x01 && ATR_character<= 0xFE)
					{
						SC7816_Item.profile.IFSC			=	ATR_character;
					}
					else
					{
						xy_assert(0);
					}
				}
				SC7816_Item.profile.T1_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TA_POS);
			}
			
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_TBi:
			if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T15))
			{
				/*
				The first TB for T=15 indicates the use of SPU by the card (see 5.2.4). The default value is “SPU not used”.
				*/
				if( !(SC7816_Item.profile.T15_atrpara_present & (1<<ATR_SPECIFIC_PARA_TB_POS)))
				{
					
				}
				SC7816_Item.profile.T15_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TB_POS);
			}
			else if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T1))
			{
				if( !(SC7816_Item.profile.T1_atrpara_present & (1<<ATR_SPECIFIC_PARA_TB_POS)))
				{
					SC7816_Item.profile.CWI			=	ATR_character&0x0F;	
					SC7816_Item.profile.BWI			=	(ATR_character&0xF0)>>4;
				}
				SC7816_Item.profile.T1_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TB_POS);
					
			}
			
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		case STATE_ATR_PARSE_TCi:
			if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T15))
			{
				/*
				The first TC for T=15.
				*/
				if( !(SC7816_Item.profile.T15_atrpara_present & (1<<ATR_SPECIFIC_PARA_TC_POS)))
				{
					
				}
				SC7816_Item.profile.T15_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TC_POS);
			}
			else if(SC7816_Item.profile.T_indicated&(1<<PROTOCOL_T1))
			{
				if( !(SC7816_Item.profile.T1_atrpara_present & (1<<ATR_SPECIFIC_PARA_TC_POS)))
				{
					if(ATR_character&0x01)
					{
						SC7816_Item.profile.edc			=	T1_EDC_CRC;
						xy_assert(0);
					}
					else
					{
						SC7816_Item.profile.edc			=	T1_EDC_LRC;
					}
				}
				
				SC7816_Item.profile.T1_atrpara_present |= (1<<ATR_SPECIFIC_PARA_TC_POS);
			}
			SC7816_Item.atr_state	=	switch_next_state(SC7816_Item.atr_state,*T_bitmap);
			break;
		
		case STATE_ATR_PARSE_TDi:
			
			T_indicate				=	ATR_character&0x0F;
			*T_bitmap 				= 	(ATR_character&0xF0)>>4;
			SC7816_Item.profile.T_indicated|=1<<T_indicate;
			SC7816_Item.atr_state	=	switch_next_state(STATE_ATR_PARSE_TD2,*T_bitmap);
			break;
			
		case STATE_ATR_PARSE_HISTORY_BYTES:
			
			if(*history_bytes)
			{
				*history_bytes=(*history_bytes)-1;
			}
			if(0 == *history_bytes)
			{
				if(SC7816_Item.profile.T_indicated&(~(1<<PROTOCOL_T0)))
				{
					SC7816_Item.atr_state	=	STATE_ATR_PARSE_TCK;//only T=0 is indicated,TCK shall be absent,else TCK shall be present
				}
				else
				{
					SC7816_Item.atr_state	=	STATE_ATR_SUCCESS;
				}
			}
			break;
			
		case STATE_ATR_PARSE_TCK:
		default :
			break;
		
	}

	return SC7816_Item.atr_state;
}


//get the guardtime in T0,T1(CGT)
void SimCard_GetGuardTime(void)
{
	if(SC7816_Item.profile.N == 255)
	{
		//TC1(N) = 255
		if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T1)
		{
			SC7816_Item.profile.guard_time = 1;
		}
		else
		{
			SC7816_Item.profile.guard_time = 2;
		}
	}
	else
	{
		/*
		• If T=15 is absent in the Answer-to-Reset, then R = F / D, i.e., the integers used for computing the etu.
		• If T=15 is present in the Answer-to-Reset, then R = Fi / Di, i.e., the integers defined above by TA1.
		No extra guard time is used to transmit characters from the card: GT = 12 etu.
		*/
		//guard_time update here,can not be used in PPS,it should be used in CMD
		if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T15))
		{
			//T=15 present,(Fi/Di)*N/f=(Fi/Di)*N/(F/d)*etu
			SC7816_Item.profile.guard_time = 2 + SC7816_Item.profile.N * (SC7816_Item.profile.Fi/SC7816_Item.profile.Di)/(SC7816_Item.profile.F/SC7816_Item.profile.D);
		}
		else
		{
			SC7816_Item.profile.guard_time = 2 + SC7816_Item.profile.N;
		}
	}
}

//get the CWT(clk) in T1
uint32_t SimCard_GetCWT(void)
{
	uint32_t CGT_etu_num = 1;
	
	for(uint8_t i=0;i<SC7816_Item.profile.CWI;i++)
		CGT_etu_num *= 2;
	
	CGT_etu_num += 11;
	
	return Etu_2_Clk(CGT_etu_num);
}

//get the BWT(clk) in T1
uint32_t SimCard_GetBWT(void)
{
	uint32_t BWT_num = 1;
	
	for(uint8_t i=0;i<SC7816_Item.profile.BWI;i++)
		BWT_num *= 2;
	
	BWT_num *= 960*DEFAULT_Fd;
	
	return Etu_2_Clk(11) + BWT_num;
}


//对ATR数据作进一步的处理
void ATR_process(void)
{

	if(SC7816_Item.profile.global_atrpara_present & (1<<ATR_GLOBAL_PARA_TA2_POS) )
	{
		//TA2 is present,specific mode
		SC7816_Item.profile.T_protocol_used = SC7816_Item.profile.TA2_SpecificMode & 0x0f;
		
		if(SC7816_Item.profile.TA2_SpecificMode & 0x10)
		{
			//If bit 5 is set to 1, then implicit values (not defined by the interface bytes) shall apply
			SC7816_Item.profile.Fp = DEFAULT_Fd;
			SC7816_Item.profile.Dp = DEFAULT_Dd;
		}
		else
		{
			SC7816_Item.profile.Fp = SC7816_Item.profile.Fi;
			SC7816_Item.profile.Dp = SC7816_Item.profile.Di;
		}
	}
	else
	{
		//negotiate mode
		SC7816_Item.profile.Fp = SC7816_Item.profile.Fi;
		SC7816_Item.profile.Dp = SC7816_Item.profile.Di;
		
		if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T0))
		{
			SC7816_Item.profile.T_protocol_used = PROTOCOL_T0;
		}
		else if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T1))
		{
			SC7816_Item.profile.T_protocol_used = PROTOCOL_T1;
		}
	}
		
}
/*
uint8_t SIM_WaitTS(uint8_t *Data, uint32_t TimeOut)
{

	uint32_t Counter = 0;
//	TimeOut=(TimeOut/WHILE_DELAY_CLK)*SC7816_Item.profile.clk_div*XY_HCLK_DIV*XY_PCLK_DIV/XY_CPCLK_DIV;
	
	TimeOut = 100000000;

    while((ISO7816_GetFifoEmptyFlag()) == 1 && (Counter < TimeOut))
    {
    		Counter++;
    }
	
    if(Counter < TimeOut)
    {
        *Data = ISO7816_ByteGet();
		
        return SC_SUCCESS;  
    }
    else
    {   	

        return SC_FAILURE; 
    }
}
*/

uint8_t ATR_Receive(uint8_t *characters)
{

	uint8_t 	result			=	SC_FAILURE;
	uint8_t 	data			=	0;
	uint8_t 	checksum 		= 	0;
	uint8_t 	T_bitmap		=	0;
	
	uint8_t		rcv_num		 	= 	0;
	uint8_t 	history_bytes	=	0;
	uint32_t 	waiting_time 	= 	40000+5000;//margin 5000
	uint8_t 	null_data_num	=	0;
	uint8_t 	debug_atr_data[30] = {0};
	
	memset(debug_atr_data,0,sizeof(debug_atr_data));
    
	while(data!= 0x3F && data != 0x3B)
	{
		result = Character_Receive(&data,waiting_time);
		
		if(SC_FAILURE ==result)
		{
			PrintLog(0, PLATFORM, WARN_LOG, "uSim TS timeout!debug_atr_data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
						debug_atr_data[0],debug_atr_data[1],debug_atr_data[2],debug_atr_data[3],debug_atr_data[4],\
						debug_atr_data[5],debug_atr_data[6],debug_atr_data[7],debug_atr_data[8],debug_atr_data[9],\
						debug_atr_data[10],debug_atr_data[11],debug_atr_data[12],debug_atr_data[13],debug_atr_data[14] );	
			
			SC7816_Item.atr_state=STATE_ATR_FAILURE;
			return SC_FAILURE;
		}
		else if(data!= 0x3F && data != 0x3B)
		{
			debug_atr_data[null_data_num] = data;
			null_data_num++;
			
			PrintLog(0, PLATFORM, WARN_LOG, "uSim TS dirty %d",null_data_num);
			// by default,1 byte = 12 etu,1 etu = DEFAULT_Fd/DEFAULT_Dd,40000/372/12=9
			if(null_data_num > 15)
			{
				PrintLog(0, PLATFORM, WARN_LOG, "uSim TS dirty exit!debug_atr_data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,", \
						debug_atr_data[0],debug_atr_data[1],debug_atr_data[2],debug_atr_data[3],debug_atr_data[4],\
						debug_atr_data[5],debug_atr_data[6],debug_atr_data[7],debug_atr_data[8],debug_atr_data[9],\
						debug_atr_data[10],debug_atr_data[11],debug_atr_data[12],debug_atr_data[13],debug_atr_data[14] );
				SC7816_Item.atr_state=STATE_ATR_FAILURE;
				return SC_FAILURE;
			}
		}
	}
	characters[rcv_num]		=	data;
	
	waiting_time			=	Etu_2_Clk(DEFAULT_WT_ETU)*5/4;//margin
	
	SC7816_Item.atr_state	=	STATE_ATR_PARSE_TS;
    
	SIM_SetRetryNum(3);//debug use,for gcf test
	
	while(1)
	{

		ATR_Character_Parse(characters[rcv_num], &T_bitmap, &history_bytes);
		
		if(SC7816_Item.atr_state==STATE_ATR_SUCCESS)
		{
			return SC_SUCCESS;
		}
		else if(SC7816_Item.atr_state==STATE_ATR_FAILURE)
		{
			return SC_FAILURE;
		}
		
		rcv_num++;

		if(SC_FAILURE == Character_Receive(&characters[rcv_num],waiting_time))
		{
			SC7816_Item.atr_state		=	STATE_ATR_FAILURE;
			return SC_FAILURE;
		}

		checksum^=characters[rcv_num];
		
		if(SC7816_Item.atr_state==STATE_ATR_PARSE_TCK)
		{
			if(0 == checksum)
			{
				SC7816_Item.atr_state	=	STATE_ATR_SUCCESS;
				return SC_SUCCESS;
			}
			else
			{
				SC7816_Item.atr_state	=	STATE_ATR_FAILURE;
				return SC_FAILURE;
			}
		}
	}
		
}


void init_SimCard_Item()
{

	SC7816_Item.atr_state				=	STATE_ATR_NONE;
	SC7816_Item.pps_state				=	STATE_PPS_NONE;
	SC7816_Item.T0_state				=	STATE_T0_CMD_NONE;
	SC7816_Item.current_procedure		=	PROCEDURE_NONE;

	memset(&SC7816_Item.profile,0x00,sizeof(SC7816_Item.profile));
		
	SC7816_Item.profile.Fi				=	DEFAULT_Fd;
	SC7816_Item.profile.Di				=	DEFAULT_Dd;
	SC7816_Item.profile.Fp				=	DEFAULT_Fd;
	SC7816_Item.profile.Dp				=	DEFAULT_Dd;
	SC7816_Item.profile.F				=	DEFAULT_Fd;
	SC7816_Item.profile.D				=	DEFAULT_Dd;
	
	SC7816_Item.profile.R				=	DEFAULT_Fd/DEFAULT_Dd;
	
	//SC7816_Item.profile.clk_div			=	CLK_CSP_SM_DIV;
	SC7816_Item.profile.clk_div			=	SimCard_GetClkDiv();

	SC7816_Item.profile.TA1				=	0x11;
	SC7816_Item.profile.N				=	0;
	SC7816_Item.profile.WI				=	10;
	SC7816_Item.profile.edc				=	T1_EDC_LRC;

	SC7816_Item.profile.Ns				=	0;
					
	SC7816_Item.profile.IFSC			=	32;
	SC7816_Item.profile.IFSD			=	32;
	SC7816_Item.profile.CWI				=	13;
	SC7816_Item.profile.BWI				=	4;

	SC7816_Item.profile.guard_time		=	2;

	/*
	If TD1, TD2 and so on are present, the encoded types T shall be in ascending numerical order. If present, T=0
	shall be first, T=15 shall be last. T=15 is invalid in TD1.
	The “first offered transmission protocol” is defined as follows.
	If TD1 is present, then it encodes the first offered protocol T.
	If TD1 is absent, then the only offer is T=0.
	*/

	/*
	For example, clause 11.4 specifies three interface bytes specific to T=1, namely the first TA, TB and TC for
	T=1. If needed, such a byte shall be transmitted respectively as TA3 TB3 and TC3 after TD2 indicating T=1.
	Depending on whether the card also offers T=0 or not, TD1 shall indicate either T=0 or T=1.
	*/
	SC7816_Item.profile.T_indicated		=	1<<PROTOCOL_T0;
	SC7816_Item.profile.T_protocol_used	=	PROTOCOL_T0;

	SC7816_Item.profile.T1_atrpara_present		=	0;

}

void SimCard_SwitchPPSOrCmd()
{
	uint16_t 	F_div_D	=	0;
	uint16_t 	PPS_guard_time = 0;
	if(    (SC7816_Item.profile.global_atrpara_present & (1<<ATR_GLOBAL_PARA_TA2_POS))
		|| (   (SC7816_Item.profile.T_indicated & ((1<<PROTOCOL_T0)|(1<<PROTOCOL_T1))) != ((1<<PROTOCOL_T0)|(1<<PROTOCOL_T1)) 
		     &&(SC7816_Item.profile.Fi == DEFAULT_Fd && SC7816_Item.profile.Di == DEFAULT_Dd) )
	)
	{

		
		
		//Change the baudrate of sending data based on ATR req from smartcard
		SC7816_Item.profile.F = SC7816_Item.profile.Fp;
		SC7816_Item.profile.D = SC7816_Item.profile.Dp;
		
	  	F_div_D = SC7816_Item.profile.F/SC7816_Item.profile.D;

		SimCard_GetGuardTime();
		SIM_SetGuardTime(SC7816_Item.profile.guard_time);

		ISO7816_ETUCycleSet(F_div_D);

		if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T0)
		{
			SIM_SetRetryNum(3);
			SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
		}
		else
		{
			SIM_SetRetryNum(0);//disable retry in T1
			ISO7816_RxStopbitsSet(1);//rx mininum 1 stopbit
			SC7816_Item.current_procedure	=	PROCEDURE_T1_CMD;
		}
			
		
		//SIM_SmartCard_WT(10);
		
	}
	else
	{

		
		
		/*
		The use of N = 255 is protocol dependent: GT = 12 etu in PPS (see 9) and in T=0 (see 10). For the use of
		N = 255 in T=1, see 11.2.
		*/
		//set guardtime in PPS
		if(SC7816_Item.profile.N == 255)
		{
			SIM_SetGuardTime(2);
		}
		else
		{

			/*
			• If T=15 is absent in the Answer-to-Reset, then R = F / D, i.e., the integers used for computing the etu.
			• If T=15 is present in the Answer-to-Reset, then R = Fi / Di, i.e., the integers defined above by TA1.
			No extra guard time is used to transmit characters from the card: GT = 12 etu.
			*/
			//in PPS,F=Fd,D=Dd
			if(SC7816_Item.profile.T_indicated & (1<<PROTOCOL_T15))
			{
				//T=15 present,(Fi/Di)*N/f=(Fi/Di)*N/(F/d)*etu,in PPS,F=Fd,D=Dd
				PPS_guard_time = 2 + SC7816_Item.profile.N * (SC7816_Item.profile.Fi/SC7816_Item.profile.Di)/(SC7816_Item.profile.F/SC7816_Item.profile.D);
			}
			else
			{
				PPS_guard_time = 2 + SC7816_Item.profile.N;
			}
			//
			//PPS_guard_time = 2 + SC7816_Item.profile.N * (SC7816_Item.profile.Fi/SC7816_Item.profile.Di)/(DEFAULT_Fd/DEFAULT_Dd);
			SIM_SetGuardTime(PPS_guard_time);
		}
        SIM_SetRetryNum(3);//enable before pps
		SC7816_Item.current_procedure	=	PROCEDURE_PPS;
		
	}
}

uint8_t SimCard_SimVccSelect(char vol_class)
{
    volatile uint32_t delay;
    switch (vol_class)
	{
		//case VOLTAGE_CLASS_A_SUPPORT://5V not supported
		//	break;
		case VOLTAGE_CLASS_B_SUPPORT://3V
		    if( !(AONPRCM->RST_CTRL1 & 0x80))
		    {
		        //if ioldo is 1.8v,vddio2 use ioldo2

				//not support,ioldo2 should always be 1.8v
				return 0;

                // AONPRCM->PWRMUX1_TRIG = 0;
                // AONPRCM->PWRMUX1_BYP |= 0x02;//pwrmux1_bypsupp_ctl = 1
                // AONPRCM->PWRMUX1_CFG &= 0xcf;//auto supply
                // AONPRCM->PWRMUX1_TRIG = 1;

                //AONPRCM->RST_CTRL1 |= 0x40;//ioldo2 3v
                
                AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x07)|0x03;//pmux2 select ioldo2,sw on
                AONPRCM->PWRMUX2_TRIG = 1;

                AONPRCM->ISO7816_VDD_CTL = 0;//register control

                //AONPRCM->CORE_PWR_CTRL3 &= 0x0F;//clear ioldo2 cfg
                //AONPRCM->PWRCTL_TEST3 &= 0xF0;//bypassmode = 0
                
		    }
		    else
		    {
		        //if ioldo is 3v,vddio2 use ioldo1
		        AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x07)|0x02;//pmux2 select ioldo1,sw on
                AONPRCM->PWRMUX2_TRIG = 1;

                AONPRCM->ISO7816_VDD_CTL = 0;//register control
		    }
			break;
		
		case VOLTAGE_CLASS_C_SUPPORT://1.8V
		    if( !(AONPRCM->RST_CTRL1 & 0x80))
		    {
                //if ioldo is 1.8v,vddio2 use ioldo1
		        AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x07)|0x02;//pmux2 select ioldo1,sw on
                AONPRCM->PWRMUX2_TRIG = 1;

                AONPRCM->ISO7816_VDD_CTL = 0;//register control
		    }
		    else
		    {
		        //if ioldo is 3v,vddio2 use ioldo2

                // AONPRCM->PWRMUX1_TRIG = 0;
                // AONPRCM->PWRMUX1_BYP |= 0x02;//pwrmux1_bypsupp_ctl = 1
                // AONPRCM->PWRMUX1_CFG &= 0xcf;//auto supply
                // AONPRCM->PWRMUX1_TRIG = 1;

                
                //HWREGB(AONPRCM_ADIF_BASE+0x05) &= 0x9F;//iaon_ctrl 46:45,ioldo2 inrush current
                AONPRCM->RST_CTRL1 &= ~0x40;//ioldo2 1.8v,ioldo2 should always be 1.8v
                
                AONPRCM->PWRMUX2_TRIG = 0;
                AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x07)|0x03;//pmux2 select ioldo2,sw on
                AONPRCM->PWRMUX2_TRIG = 1;

                for(delay=0;delay<1000;delay++);

                AONPRCM->ISO7816_VDD_CTL = 0;//register control

                //AONPRCM->CORE_PWR_CTRL3 &= 0x0F;//clear ioldo2 cfg
                //AONPRCM->PWRCTL_TEST3 &= 0xF0;//bypassmode = 0
		    }
			break;
        default:
            xy_assert(0);
            break;
	}
	return 1;
}

void SimCard_SimVccPoweroff(void)
{

    if( (AONPRCM->PWRMUX2_CFG & 0x03) == 0x03)
    {
		//B0 flash use vddio2,should not poweroff
        //if vddio2 use ioldo2.power off ioldo2 first
        //AONPRCM->CORE_PWR_CTRL3 &= 0x0F;//clear ioldo2 cfg
        //AONPRCM->PWRCTL_TEST3 = (AONPRCM->PWRCTL_TEST3 & 0xF0)|0x01;//bypassmode = 1
    }

    if(AONPRCM->ISO7816_VDD_CTL == 0)
    {
        AONPRCM->PWRMUX2_TRIG = 0;
        AONPRCM->PWRMUX2_CFG = (AONPRCM->PWRMUX2_CFG & ~0x03)|0x04;//pmux2 off
        AONPRCM->PWRMUX2_TRIG = 1;
    }
    
}

//If the answer does not begin within 40000 clockcycles with RST at state H,the interface device shall perform a deactivation
void SimCard_PowerReset(unsigned char mode, char vol_class)
{

	uint8_t		result	=	SC_FAILURE;

	uint16_t 	F_div_D	=	0;

	ISO7816_ClockDiVSet(SIM_APB_CLK, SIM_CLK);
	
	init_SimCard_Item();

	if(mode == Pw_on)
		SC7816_Item.current_procedure		  =	PROCEDURE_COLD_RST_ACTIVATION;
	else if(mode == Pw_warmreset)
		SC7816_Item.current_procedure		  =	PROCEDURE_WARM_RST_ACTIVATION;

	if(SimCard_SimVccSelect(vol_class) == 0)
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim VoltageSelect fail");
		return;
	}
	
	F_div_D=SC7816_Item.profile.F/SC7816_Item.profile.D;
	
	ISO7816_ETUCycleSet(F_div_D);
	
	if(Pw_on == mode)
	{
		ISO7816_ColdReset();
	}
	else if(Pw_warmreset == mode) 
	{
		ISO7816_WarmReset();
	}

	memset(ATR_Rsp,0x00,sizeof(ATR_Rsp));
	result = ATR_Receive(ATR_Rsp);							//wait atr from 400clk to 40000clk
	

	if(SC_FAILURE==result)
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim ATR Failed: %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,SM_DBGSTAT=%x,SM_ATRSTAT=%x,F/D=%d,clkdiv=%d", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9],HWREGB(SM_DBGSTAT),HWREGB(SM_ATRSTAT),F_div_D,SC7816_Item.profile.clk_div);
#if SIM_DEBUG_CODE
        PrintLog(0, PLATFORM, WARN_LOG, "uSim ATR debug: tick_before=%x,tick_after=%x,timeout=%x,ANAXTALRDY=%x,syclkflag=%x",tickcnt_before_iso7816_recv,tickcnt_after_iso7816_recv,g_iso7816_debug_timeout,g_iso7816_debug_plllock,g_iso7816_debug_sysclk);
#endif	        
		return;
	}
	else if(SC7816_Item.profile.class_clock !=0 && (SC7816_Item.profile.class_clock & vol_class) == 0)
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim VoltageClass: %d Failed",vol_class);
		return;
	}
	else
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim ATR = %x,%x,%x,%x,%x,%x,%x,%x,%x,%x", ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9]);
	}
	
	ATR_process();
	if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T1)
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim T=1!");
	}

	SimCard_SwitchPPSOrCmd();
}


/*
In the following three cases:
	overrun of WI 
	erroneous PPS response
	unsuccessful PPS exchange
the interface device shall perform a deactivation
*/
volatile PPS_element PPS_response_debug	=	{0};
void PPS_Exchange()
{

	uint8_t result 				= 	SC_FAILURE;
	uint8_t checksum 			= 	0;
	uint16_t F_div_D			=	0;
	//uint8_t Negotiation_result 	= 	1;
	
	//uint32_t waiting_time 		=	Etu_2_Clk(DEFAULT_WT_ETU);
	uint32_t waiting_time 		=	(DEFAULT_Fd*DEFAULT_WT_ETU/DEFAULT_Dd)*5/4;//margin
	PPS_element PPS_request		=	{0};
	PPS_element PPS_response	=	{0};


	if(PROCEDURE_PPS != SC7816_Item.current_procedure)
	{
		xy_assert(0);
		return;
	}

	SC7816_Item.pps_state		=	STATE_PPS_NONE;

	/*
	PPS1 allows the interface device to propose values of F and D to the card. Encoded in the same way as
	in TA1, these values shall be from Fd to Fi and from Dd to Di respectively. If an interface device does not
	transmit PPS1, it proposes to continue with Fd and Dd. The card either acknowledges both values by
	echoing PPS1 (then these values become Fn and Dn) or does not transmit PPS1 to continue with Fd and
	Dd (then Fn = 372 and Dn = 1).
	*/
	//delay?
	ISO7816_SwitchToTxFromRx();
	if(!(HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_EMPTY_Msk))
	{
        PrintLog(0, PLATFORM, WARN_LOG, "uSim debug:switch tx error,FIFONUM = %x,SM_DBGSTAT = %x", HWREGB(SM_FIFONUM), HWREGB(SM_DBGSTAT));
        ISO7816_FifoFlush();
	}
//    SIM_SmartCard_WT(8);
	PPS_request.PPSS	=	0xFF;
	PPS_request.PPS0	=	0x10 | SC7816_Item.profile.T_protocol_used;
	PPS_request.PPS1	=	SC7816_Item.profile.TA1;
	PPS_request.PCK		=	PPS_request.PPSS^PPS_request.PPS0^PPS_request.PPS1;

#if (SIM_INT_EN == 1)

	uint8_t txdata[4];
	txdata[0] = PPS_request.PPSS;
	txdata[1] = PPS_request.PPS0;
	txdata[2] = PPS_request.PPS1;
	txdata[3] = PPS_request.PCK;
	if(SC_FAILURE == Character_Send_Blocking(txdata,4))
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim PPS tx fail");
		SC7816_Item.pps_state=STATE_PPS_FAILURE;
		return;
	}
	
#else
	
	Character_Send(PPS_request.PPSS,waiting_time);	//
	Character_Send(PPS_request.PPS0,waiting_time);	//
	Character_Send(PPS_request.PPS1,waiting_time);	//
	Character_Send(PPS_request.PCK,waiting_time);	//
	
#endif	
	//ISO7816_SwitchToRxFromTx();

	result	=	Character_Receive(&PPS_response.PPSS,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	
	result	=	Character_Receive(&PPS_response.PPS0,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	
	if(PPS_response.PPS0&0x10)
	{
		result	=	Character_Receive(&PPS_response.PPS1,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	if(PPS_response.PPS0&0x20)
	{
		result	=	Character_Receive(&PPS_response.PPS2,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	if(PPS_response.PPS0&0x40)
	{
		result	=	Character_Receive(&PPS_response.PPS3,waiting_time);
		if(result==SC_FAILURE)
		{
			
			SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
	}
	
	result	=	Character_Receive(&PPS_response.PCK,waiting_time);
	if(result==SC_FAILURE)
	{
	
		SC7816_Item.pps_state		=	STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}

	checksum = 	PPS_response.PCK^PPS_response.PPSS^PPS_response.PPS0^PPS_response.PPS1^PPS_response.PPS2^PPS_response.PPS3;

	if(checksum ||((PPS_response.PPS0&0x0F)	!= (PPS_request.PPS0&0x0F)))
	{
		//Negotiation_result=0;
		SC7816_Item.pps_state=STATE_PPS_FAILURE;
		PPS_response_debug = PPS_response;
		//xy_assert(0);
		return;
	}
	if(PPS_response.PPS0&0x10)
	{
		if(PPS_request.PPS1 != PPS_response.PPS1)
		{
			//Negotiation_result=0;
			SC7816_Item.pps_state=STATE_PPS_FAILURE;
			PPS_response_debug = PPS_response;
			//xy_assert(0);
			return;
		}
		else
		{
			SC7816_Item.profile.F	=	SC7816_Item.profile.Fp;
			SC7816_Item.profile.D	=	SC7816_Item.profile.Dp;
		}

	}
	else
	{
		SC7816_Item.profile.F	=	DEFAULT_Fd;
		SC7816_Item.profile.D	=	DEFAULT_Dd;
	}

	SimCard_GetGuardTime();
	SIM_SetGuardTime(SC7816_Item.profile.guard_time);
	
	F_div_D = SC7816_Item.profile.F/SC7816_Item.profile.D;
	
	ISO7816_ETUCycleSet(F_div_D);
	
	//Change the baudrate of sending data based on ATR req from smartcard
	
	SC7816_Item.pps_state			=	STATE_PPS_SUCCESS;

	//disable retry in T1
	
	if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T0)
	{
		SIM_SetRetryNum(3);
		SC7816_Item.current_procedure	=	PROCEDURE_T0_CMD;
	}
	else
	{
		SIM_SetRetryNum(0);
		ISO7816_RxStopbitsSet(1);//rx mininum 1 stopbit
		SC7816_Item.current_procedure	=	PROCEDURE_T1_CMD;
	}
	
	SimCard_ClkDelay(Etu_2_Clk(240));//bug13539 特定仪表卡切换速率后需要delay一段时间，才能正确接收后续cmd，实测90etu
	
    //SIM_SmartCard_WT(10);

	PrintLog(0, PLATFORM, WARN_LOG, "uSim PPS config:F_div_D = %d, CLK_DIV = %d", F_div_D, SC7816_Item.profile.clk_div);


}
void SimCard_Deactivation()
{
	GPIO_InitTypeDef gpio_init = {0};
    volatile unsigned int delay;
	if(SC7816_Item.current_procedure==PROCEDURE_DEACTIVATION)
	{
		return;
	}

	ISO7816_Deactivation();
    
    SimCard_SimVccPoweroff();

	SC7816_Item.current_procedure=PROCEDURE_DEACTIVATION;
	g_uicc_focus=1;
}



uint8_t SIM_ClkCtrl(uint8_t ctrl_mode, uint32_t timeout)
{

	uint16_t clk_stop_state = SC7816_Item.profile.class_clock&0xC0;
	if(clk_stop_state == CLK_STOP_NOT_SUPPORT)
	{
		return SC_FAILURE;
	}
	if (ctrl_mode == SIM_CLK_STOP)
    {
    	//SimCard_ClkDelay(timeout);
    	ISO7816_IdleETUSet(0);
        /*fsm ensures 1860 clks*/
		if(CLK_STOP_STATE_HIGH_LOW != clk_stop_state)
		{
			if(CLK_STOP_STATE_LOW_ONLY == clk_stop_state)
			{
				ISO7816_ClockStopEn(CLOCK_LOW);
			}
			else //if(CLK_STOP_STATE_HIGH_ONLY == clk_stop_state)
			{
				ISO7816_ClockStopEn(CLOCK_HIGH);
			}
		}
        else
        {
            ISO7816_ClockStopEn(CLOCK_LOW);
        }
		SC7816_Item.current_procedure			 =	PROCEDURE_CLK_STOP;
    }
    else
    {
		//ISO7816_ClockStopDis();
		//ISO7816_IdleETUSet(0xFFF);//4095 etu max,not enough for 9600etu,so when tx data sending,ISO7816_ClockStopDis()
		//2022-08-19 not use clk stop dis:is disable clkstop,iso7816 will send imediately after txdata into fifo;enable clk stop,when tx,fsm will ensure 744 clks before ending
		//2022-08-22 manual clk stop dis,and delay before sending;fsm auto delay before sending,only 700 clock cycles,but GCF test requires 744 clks
        ISO7816_ClockStopDis();
        SimCard_ClkDelay(timeout);

		SC7816_Item.current_procedure			 =	PROCEDURE_T0_CMD;
    }

    return SC_SUCCESS;
}

uint8_t dirty_character[30];
uint8_t dirty_num=0;

//uint8_t sim_csp_int_en=0;



uint8_t T0_Cmd_Handler(uint8_t *Txbuff, uint8_t *Rxbuff, uint32_t* len)
{
	uint8_t result			=	SC_FAILURE;
	uint8_t character		=	0;
	uint8_t xor_character	=	0;
	uint8_t p3 				= 	Txbuff[4];
	uint8_t tx_num			=	0;
	
	uint8_t is_rcv_more		=	0;

	T_Command_APDU TxAPDU	=	{0};

	//uint32_t waiting_time 	= 	SC7816_Item.profile.WI*960*SC7816_Item.profile.Fi+480*SC7816_Item.profile.Fi;
    uint32_t waiting_time 	= 	(SC7816_Item.profile.WI*960*SC7816_Item.profile.Fi)*5/4;//margin
	if((PROCEDURE_T0_CMD != SC7816_Item.current_procedure) && (PROCEDURE_CLK_STOP != SC7816_Item.current_procedure))
	{
		SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;

		PrintLog(0, PLATFORM, WARN_LOG, "uSim Not in T0 or T1 state");		

		return SC_FAILURE;
	}

    if(!(HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_EMPTY_Msk))
	{
        PrintLog(0, PLATFORM, WARN_LOG, "uSim debug:Fifo not empty before T0_CMD,FIFONUM = %x,SM_DBGSTAT = %x", HWREGB(SM_FIFONUM), HWREGB(SM_DBGSTAT));
        ISO7816_FifoFlush();
        SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
        return SC_FAILURE;
	}
	
	TxAPDU.cla				=	Txbuff[0];
	TxAPDU.ins				=	Txbuff[1];
	TxAPDU.p1				=	Txbuff[2];
	TxAPDU.p2				=	Txbuff[3];
	
	if(*len==5)
	{
		TxAPDU.Lc		=	0;
		if(0==Txbuff[4])
		{
			TxAPDU.Le	=	256;
		}
		else
		{
			TxAPDU.Le	=	(uint16_t)Txbuff[4];
		}
	}
	else
	{
		TxAPDU.Lc		=	Txbuff[4];
		TxAPDU.Le		=	0;
		TxAPDU.data		=	(uint8_t *)(Txbuff+5);
	}


	RxAPDU.data			=	Rxbuff;
	RxAPDU.sw1			=	0;
	RxAPDU.sw2			=	0;
	sim_rx_num			=	0;
	dirty_num			=	0;

	SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);


	
	SC7816_Item.T0_state=STATE_T0_CMD_NONE;
	is_rcv_more 		=	1;


	ISO7816_SwitchToTxFromRx();
    
#if (SIM_INT_EN == 1)

	uint8_t txdata[5];
	txdata[0] = TxAPDU.cla;
	txdata[1] = TxAPDU.ins;
	txdata[2] = TxAPDU.p1;
	txdata[3] = TxAPDU.p2;
	txdata[4] = p3;
	if(SC_FAILURE == Character_Send_Blocking(txdata,5))
	{
		PrintLog(0, PLATFORM, WARN_LOG, "uSim T0 tx fail pos1");
		SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
		return SC_FAILURE;
	}
	
#else	

	Character_Send(TxAPDU.cla,waiting_time);	
	Character_Send(TxAPDU.ins,waiting_time);	
	Character_Send(TxAPDU.p1,waiting_time);	
	Character_Send(TxAPDU.p2,waiting_time);	
	Character_Send(p3,waiting_time);	
	
#endif
    //ISO7816_ClockStopDis();
    //ISO7816_IdleETUSet(0);//idle cnt max 4095 etu,cannot stop clk till waitingtime timeout

	//ISO7816_SwitchToRxFromTx();


	while(1)
	{

		if(1 == is_rcv_more)
		{
			result=Character_Receive(&character,waiting_time);
			xor_character = character^0xFF;
			if(SC_FAILURE == result)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
				return SC_FAILURE;
			}
			if(0x60 == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_NULL;
			}
			else if(TxAPDU.ins == character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_INS;
			}
			else if(TxAPDU.ins == xor_character)
			{
				SC7816_Item.T0_state	=	STATE_T0_CMD_COMPLEMENT_INS;
			}
			else if(0x90 == (character&0xF0) || 0x60 == (character&0xF0))
			{
				RxAPDU.sw1	=	character;
				SC7816_Item.T0_state	=	STATE_T0_CMD_SW1;
			}
			else
			{
				//while(1);
				dirty_character[dirty_num]=character;
				dirty_num++;
				if(30==dirty_num)
				{
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;
					//xy_assert(0);
				}
				continue;
			}

		}
		
		switch (SC7816_Item.T0_state)
		{
			case STATE_T0_CMD_NULL:
			case STATE_T0_CMD_NONE:
				break;
			case STATE_T0_CMD_INS:
			{
				if(TxAPDU.Le)
				{
					while(sim_rx_num<TxAPDU.Le)
					{
						result=Character_Receive(&character,waiting_time);
						if(SC_FAILURE == result)
						{
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						RxAPDU.data[sim_rx_num]=character;
						sim_rx_num++;
					}
				}
				else if(TxAPDU.Lc)
				{
					
					ISO7816_SwitchToTxFromRx();

#if (SIM_INT_EN == 1)

					if(tx_num<TxAPDU.Lc)
					{
						if(SC_FAILURE == Character_Send_Blocking(&TxAPDU.data[tx_num],TxAPDU.Lc-tx_num))
						{
							PrintLog(0, PLATFORM, WARN_LOG, "uSim T0 tx fail pos2");
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						tx_num = TxAPDU.Lc;
					}
										
#else
					while(tx_num<TxAPDU.Lc)
					{
						Character_Send(TxAPDU.data[tx_num],waiting_time);	
						
						tx_num++;
					}
#endif					
					//ISO7816_SwitchToRxFromTx();
				}
				else 
				{
					PrintLog(0, PLATFORM, WARN_LOG, "uSim INS wrong Lc and Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				break;
			}
			case STATE_T0_CMD_COMPLEMENT_INS:
			{
				if(TxAPDU.Le)
				{
					if(sim_rx_num<TxAPDU.Le)
					{
						result=Character_Receive(&character,waiting_time);
						if(SC_FAILURE == result)
						{
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						RxAPDU.data[sim_rx_num]=character;
						sim_rx_num++;						

					}
				}
				else if(TxAPDU.Lc)
				{
					ISO7816_SwitchToTxFromRx();

#if (SIM_INT_EN == 1)

					if(tx_num<TxAPDU.Lc)
					{
						if(SC_FAILURE == Character_Send_Blocking(&TxAPDU.data[tx_num],1))
						{
							PrintLog(0, PLATFORM, WARN_LOG, "uSim T0 tx fail pos3");
							SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
							return SC_FAILURE;
						}
						tx_num++;
					}
										
#else
					if(tx_num<TxAPDU.Lc)
					{
						Character_Send(TxAPDU.data[tx_num],waiting_time);	
						tx_num++;
					}
#endif	

					//ISO7816_SwitchToRxFromTx();

				}
				else 
				{
					PrintLog(0, PLATFORM, WARN_LOG, "uSim COMPLEMENT INS wrong Lc and Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				break;
			}
			case STATE_T0_CMD_SW1:
			{
				
				result=Character_Receive(&character,waiting_time);
				if(SC_FAILURE == result)
				{
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;
				}
				RxAPDU.sw2=character;
				SC7816_Item.T0_state 		= 	STATE_T0_CMD_SUCCESS;
				break;
			}
			default:
			{
				PrintLog(0, PLATFORM, WARN_LOG, "uSim wrong STATE_T0_CMD");
				SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
				
				//while(1);
				return SC_FAILURE;
			}
		}

		if(STATE_T0_CMD_SUCCESS == SC7816_Item.T0_state)
		{
			if(TxAPDU.Le&&sim_rx_num)
			{
				if(sim_rx_num<TxAPDU.Le)
				{
					PrintLog(0, PLATFORM, WARN_LOG, "uSim sim_rx_num less than Le");
					SC7816_Item.T0_state	=	STATE_T0_CMD_FAILURE;
					return SC_FAILURE;

				}
				else
				{
					RxAPDU.data[TxAPDU.Le] 		= RxAPDU.sw1;
					RxAPDU.data[TxAPDU.Le+1] 	= RxAPDU.sw2;
					*len 						= TxAPDU.Le+2;
				}
			}
			else
			{
				RxAPDU.data[0] = RxAPDU.sw1;
				RxAPDU.data[1] = RxAPDU.sw2;
				*len = 2;
			}
			//if((RxAPDU.sw1!=0x90) && (RxAPDU.sw1!=0x91) && (RxAPDU.sw1!=0x61)  && (RxAPDU.sw1!=0x6C) && (RxAPDU.sw1!=0x63))
			{
				PrintLog(0,PLATFORM,WARN_LOG,"uSim T0_Cmd_Handler : current_procedure=%d,[CLA,INS,P1,P2,P3]=[%x,%x,%x,%x,%x],sw=[%x %x]", SC7816_Item.current_procedure,TxAPDU.cla,TxAPDU.ins,TxAPDU.p1,TxAPDU.p2,p3,RxAPDU.sw1,RxAPDU.sw2);

			}

			SIM_ClkCtrl(SIM_CLK_STOP,SIM_CLK_STOP_TIMEOUT);

			return SC_SUCCESS;
		}
		else{
				is_rcv_more	=	1;
			}
		
	}


}

static void SIM_InvertUint8(uint8_t *DesBuf, uint8_t *SrcBuf)
{
    int i;
    uint8_t temp = 0;

    for(i = 0; i < 8; i++)
    {
        if(SrcBuf[0] & (1 << i))
        {
            temp |= 1<<(7-i);
        }
    }
    DesBuf[0] = temp;
}

static void SIM_InvertUint16(uint16_t *DesBuf, uint16_t *SrcBuf)
{
    int i;
    uint16_t temp = 0;

    for(i = 0; i < 16; i++)
    {
        if(SrcBuf[0] & (1 << i))
        {
            temp |= 1<<(15 - i);
        }
    }
    DesBuf[0] = temp;
}

//参考代码
static uint16_t SIM_CRC16_CCITT(uint8_t *puchMsg, uint32_t usDataLen)
{
    uint16_t wCRCin = 0x0000;
    uint16_t wCPoly = 0x1021;
    uint8_t wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        SIM_InvertUint8(&wChar, &wChar);
        wCRCin ^= (wChar << 8);

        for(int i = 0; i < 8; i++)
        {
            if(wCRCin & 0x8000)
            {
                wCRCin = (wCRCin << 1) ^ wCPoly;
            }
            else
            {
                wCRCin = wCRCin << 1;
            }
        }
    }
    SIM_InvertUint16(&wCRCin, &wCRCin);
    return (wCRCin) ;
}

/* X^16 + X^12 + X^5 + 1 ,polynomial 1021 */
uint16_t SimT1_CRC(ISO7816_BLOCK *block)
{
	uint16_t crc_value;
	uint8_t *databuf = xy_malloc(block->len + 3);
	
	databuf[0] = block->nad_byte;
	databuf[1] = block->pcb_byte;
	databuf[2] = block->len;
	
	if(block->len)
		memcpy(databuf+3,block->inf,block->len);
	
	crc_value = SIM_CRC16_CCITT(databuf,block->len + 3);
	
	xy_free(databuf);
	return crc_value;
}
//计算校验
uint16_t SimT1_CalculateBlockEdc(ISO7816_BLOCK *block)
{
	uint8_t lrc = 0;
	
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
	{
		lrc ^= block->nad_byte;
		lrc ^= block->pcb_byte;
		lrc ^= block->len;
		for(uint8_t i = 0;i < block->len;i++)
		{
			lrc ^= block->inf[i];
		}

		return (uint16_t)lrc;
	}
	else
	{
		return SimT1_CRC(block);
	}
	
	
}

//判断block类型
int SimT1_GetBlockType(ISO7816_BLOCK *txblock)
{
	uint8_t type = (txblock->pcb_byte & 0xC0)>>6;
	
	if( type == 0x01 || type == 0x00 )
	{
		return BLOCK_I;
	}
	else if(type == 2)
	{
		return BLOCK_R;
	}
	else if(type == 3)
	{
		return BLOCK_S;
	}
	else
	{
		return BLOCK_ERROR;
	}
	
}

//组I块
void SimT1_PackIBlockTDPU(ISO7816_BLOCK *txblock,uint8_t seq_num,uint8_t more_data,uint8_t *txbuff,uint8_t len)
{
	memset((void *)txblock,0,sizeof(ISO7816_BLOCK));
	
	txblock->nad.sad = 0;
	txblock->nad.dad = 0;
	
	txblock->Ipcb.bit8 = 0;
	txblock->Ipcb.Ns = seq_num;
	txblock->Ipcb.M = more_data;
	
	txblock->len = len;

	txblock->inf = txbuff;
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
		txblock->edc.lrc = (uint8_t)SimT1_CalculateBlockEdc(txblock);
	else
		txblock->edc.crc = SimT1_CalculateBlockEdc(txblock);
	
	
}

//组R块
void SimT1_PackRBlockTDPU(ISO7816_BLOCK *txblock,uint8_t expect_seq_num)
{
	memset((void *)txblock,0,sizeof(ISO7816_BLOCK));
	
	txblock->nad.sad = 0;
	txblock->nad.dad = 0;
	
	txblock->Rpcb.bit8_7 = 2;
	txblock->Rpcb.bit6 = 0;
	txblock->Rpcb.Nr = expect_seq_num;
	txblock->Rpcb.error = 0;
	
	txblock->len = 0;

	txblock->inf = NULL;

	if(SC7816_Item.profile.edc == T1_EDC_LRC)
		txblock->edc.lrc = (uint8_t)SimT1_CalculateBlockEdc(txblock);
	else
		txblock->edc.crc = SimT1_CalculateBlockEdc(txblock);
	
}

//组S块
void SimT1_PackSBlockTDPU(ISO7816_BLOCK *txblock,uint8_t req_rsp,uint8_t cmdtype,uint8_t* s_inf_data)
{
	memset((void *)txblock,0,sizeof(ISO7816_BLOCK));
	
	txblock->nad.sad = 0;
	txblock->nad.dad = 0;
	
	txblock->Spcb.bit8_7 = 3;
	txblock->Spcb.req_rsp = req_rsp;
	txblock->Spcb.cmd = cmdtype;

	if(cmdtype == BLOCK_SPCB_CMD_RESYNCH || cmdtype == BLOCK_SPCB_CMD_ABORT)
	{
		txblock->len = 0;
		txblock->inf = NULL;
	}	
	else if(cmdtype == BLOCK_SPCB_CMD_IFS || cmdtype == BLOCK_SPCB_CMD_WTX)
	{
		
		txblock->len = 1;
		txblock->inf = s_inf_data;
	}
	else
	{
		xy_assert(0);
	}
	
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
		txblock->edc.lrc = (uint8_t)SimT1_CalculateBlockEdc(txblock);
	else
		txblock->edc.crc = SimT1_CalculateBlockEdc(txblock);
	
}
//发送block
void SimT1_SendBlockTDPU(ISO7816_BLOCK *txblock)
{
	
	//SIM_SmartCard_WT(2);//at least 22etu for T1 block guard time
	osDelay(SMClk_2_Tick(Etu_2_Clk(22)));
	ISO7816_SwitchToTxFromRx();

#if (SIM_INT_EN == 1)

	uint8_t *txdata = xy_malloc(3 + txblock->len + 2);
	uint32_t txlen,i;
	
	txdata[0] = txblock->nad_byte;
	txdata[1] = txblock->pcb_byte;
	txdata[2] = txblock->len;
	
	for(i = 0;i<txblock->len;i++)
	{
		txdata[3+i] = txblock->inf[i];
	}
	
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
	{
		txdata[3+i] = txblock->edc.lrc;
		txlen = 3+i+1;
	}
	else
	{
		txdata[3+i] = (uint8_t)txblock->edc.crc;
		txdata[3+i+1] = (uint8_t)(txblock->edc.crc>>8);
		txlen = 3+i+2;
	}
	Character_Send_Blocking(txdata,txlen);
	
	xy_free(txdata);
#else	

	Character_Send(txblock->nad_byte, T1_SEND_WT);
	Character_Send(txblock->pcb_byte, T1_SEND_WT);
	Character_Send(txblock->len, T1_SEND_WT);
	for(uint8_t i = 0;i<txblock->len;i++)
	{
		Character_Send(txblock->inf[i], T1_SEND_WT);
	}
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
	{
		Character_Send(txblock->edc.lrc, T1_SEND_WT);
	}
	else
	{
		Character_Send((uint8_t)txblock->edc.crc, T1_SEND_WT);
		Character_Send((uint8_t)(txblock->edc.crc>>8), T1_SEND_WT);
	}
	
#endif


	
}

uint8_t SimT1_RcvBlock(ISO7816_BLOCK *rxblock,uint8_t wtx_multiplier)
{
	uint8_t result;
	uint8_t character;
	//uint8_t len;
	//uint16_t rcv_num = 0;
	uint32_t block_waiting_time = SimCard_GetBWT()*wtx_multiplier;
	uint32_t char_waiting_time = SimCard_GetCWT();

	if(wtx_multiplier == 0)
	{
		xy_assert(0);	
	}

	result = Character_Receive(&character, block_waiting_time);
	if(result == SC_FAILURE)
		return SC_FAILURE;
	//rcv_num++;
	rxblock->nad_byte = character;
	
	result = Character_Receive(&character, char_waiting_time);
	if(result == SC_FAILURE)
		return SC_FAILURE;
	//rcv_num++;
	rxblock->pcb_byte = character;
	
	result = Character_Receive(&character, char_waiting_time);
	if(result == SC_FAILURE)
		return SC_FAILURE;
	//rcv_num++;
	rxblock->len = character;
	
	for(uint8_t i=0;i<rxblock->len;i++)
	{
		result = Character_Receive(&character, char_waiting_time);
		if(result == SC_FAILURE)
			return SC_FAILURE;
		rxblock->inf[i] = character;
	}
	
	if(SC7816_Item.profile.edc == T1_EDC_LRC)
	{
		result = Character_Receive(&character, char_waiting_time);
		if(result == SC_FAILURE)
			return SC_FAILURE;
		//rcv_num++;
		rxblock->edc.lrc = character;
		if(rxblock->edc.lrc != (uint8_t)SimT1_CalculateBlockEdc(rxblock))
			xy_assert(0);
	}
	else
	{
		result = Character_Receive(&character, char_waiting_time);
		if(result == SC_FAILURE)
			return SC_FAILURE;
		rxblock->edc.crc = (uint16_t)character;
		result = Character_Receive(&character, char_waiting_time);
		if(result == SC_FAILURE)
			return SC_FAILURE;

		
		rxblock->edc.crc |= (uint16_t)(character<<8);
		if(rxblock->edc.crc != SimT1_CalculateBlockEdc(rxblock))
			xy_assert(0);
	}
	return SC_SUCCESS;
}

uint8_t sim_rx_buf[T1_MAX_INF_LEN];//数组长度取决于设备能接受的长度，IFSD

uint8_t T1_Adjust_IFSD(uint8_t ifsd)
{
	uint8_t result			=	SC_FAILURE;

	ISO7816_BLOCK TxBlock	=	{0};
	ISO7816_BLOCK RxBlock	=	{0};
	
	//uint32_t rx_rcv_len		=	0;
	uint8_t block_type		=	0;

	uint8_t wtx_multiplier	=	1;
	
	if((PROCEDURE_T1_CMD != SC7816_Item.current_procedure))
	{
		SC7816_Item.T1_state	=	STATE_T1_CMD_FAILURE;

		xy_printf(0,PLATFORM, WARN_LOG, "uSim adjust IFSD Fail:Not in T1 state");		

		return SC_FAILURE;
	}

	RxBlock.inf			=	&sim_rx_buf[0];
	
	//SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

	//SIM_SmartCard_WT(2);//at least 22etu for T1 block_guard_time
	//osDelay(SMClk_2_Tick(Etu_2_Clk(22)));

	//SIM_resetTxFifo();
	//SIM_resetRxFifo();
	

	SimT1_PackSBlockTDPU(&TxBlock,BLOCK_SPCB_REQ,BLOCK_SPCB_CMD_IFS,&ifsd);
	SimT1_SendBlockTDPU(&TxBlock);

	result = SimT1_RcvBlock(&RxBlock,wtx_multiplier);
	if(result == SC_FAILURE)
	{
		SC7816_Item.T1_state = STATE_T1_CMD_FAILURE;
		xy_printf(0,PLATFORM, WARN_LOG, "uSim adjust IFSD:rcv failure");
		return SC_FAILURE;
	}
	else
	{
		block_type = SimT1_GetBlockType(&RxBlock);
		if(block_type == BLOCK_S && RxBlock.Spcb.req_rsp == BLOCK_SPCB_RSP && RxBlock.Spcb.cmd == BLOCK_SPCB_CMD_IFS && RxBlock.inf[0] == ifsd)
		{

			SC7816_Item.profile.IFSD = RxBlock.inf[0];
			SC7816_Item.T1_state = STATE_T1_CMD_SUCCESS;
			return SC_SUCCESS;
		}
		else
		{
			xy_assert(0);
			return SC_FAILURE;
		}
	}
	
}

uint8_t T1_SendSblockGetResponse(uint8_t cmdtype,uint8_t s_inf_data)
{
	uint8_t result			=	SC_FAILURE;

	ISO7816_BLOCK TxBlock	=	{0};
	ISO7816_BLOCK RxBlock	=	{0};
	
	uint8_t block_type		=	0;

	uint8_t wtx_multiplier	=	1;
	
	if((PROCEDURE_T1_CMD != SC7816_Item.current_procedure))
	{
		//SC7816_Item.T1_state	=	STATE_T1_CMD_FAILURE;

		xy_printf(0,PLATFORM, WARN_LOG, "uSim send sblock fail:Not in T1 state");		

		return SC_FAILURE;
	}

	RxBlock.inf			=	&sim_rx_buf[0];
	
	//SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

	//SIM_SmartCard_WT(2);//at least 22etu for T1 block_guard_time
	//osDelay(SMClk_2_Tick(Etu_2_Clk(22)));
	//SIM_resetTxFifo();
	//SIM_resetRxFifo();
	
	SimT1_PackSBlockTDPU(&TxBlock,BLOCK_SPCB_REQ,cmdtype,&s_inf_data);
	SimT1_SendBlockTDPU(&TxBlock);

	result = SimT1_RcvBlock(&RxBlock,wtx_multiplier);
	if(result == SC_FAILURE)
	{
		//SC7816_Item.T1_state = STATE_T1_CMD_FAILURE;
		xy_printf(0,PLATFORM, WARN_LOG, "uSim send sblock fail:rcv failure");
		return SC_FAILURE;
	}
	else
	{
		block_type = SimT1_GetBlockType(&RxBlock);
		if(block_type == BLOCK_S && RxBlock.Spcb.req_rsp == BLOCK_SPCB_RSP && RxBlock.Spcb.cmd == TxBlock.Spcb.cmd)
		{
			if( (TxBlock.Spcb.cmd == BLOCK_SPCB_CMD_IFS || TxBlock.Spcb.cmd == BLOCK_SPCB_CMD_WTX) )
			{
				if(TxBlock.inf[0] == RxBlock.inf[0])
				{
					if(TxBlock.Spcb.cmd == BLOCK_SPCB_CMD_IFS)
					{
						SC7816_Item.profile.IFSD = RxBlock.inf[0];
					}
					else//BLOCK_SPCB_CMD_WTX
					{
						
					}
					return SC_SUCCESS;
				}
				return SC_FAILURE;
			}
			else
			{
				if(TxBlock.Spcb.cmd == BLOCK_SPCB_CMD_RESYNCH)
				{
					SC7816_Item.profile.Ns = 0;
				}
				else//BLOCK_SPCB_CMD_ABORT
				{
					
				}
				return SC_SUCCESS;
			}
				
		}
		else
		{
			xy_assert(0);
			return SC_FAILURE;
		}
	}
	
}

uint8_t T1_Cmd_Handler(uint8_t *Txbuff, uint8_t *Rxbuff, uint32_t* len)
{
	uint8_t result			=	SC_FAILURE;
	//uint8_t character		=	0;
	//uint8_t xor_character	=	0;
	//uint8_t p3 				= 	Txbuff[4];
	//uint8_t tx_num			=	0;
	
	//uint8_t is_rcv_more		=	0;

	//T_Command_APDU TxAPDU	=	{0};

	ISO7816_BLOCK TxBlock	=	{0};
	ISO7816_BLOCK RxBlock	=	{0};
	
	uint32_t tx_info_len	=	*len;
	uint32_t tx_sending_len	=	0;
	uint32_t tx_sended_len	=	0;
	
	//uint8_t send_seq_num	=	0;
	uint8_t more_data		=	0;

	//uint32_t rx_rcv_len		=	0;
	uint8_t block_type		=	0;

	uint8_t s_inf_data		=	0;
	uint8_t wtx_multiplier	=	1;
	
	//uint32_t waiting_time 	= 	SC7816_Item.profile.WI*960*SC7816_Item.profile.Fi+480*SC7816_Item.profile.Fi;
	//uint32_t block_guard_time 	= 	Etu_2_Clk(BGT_ETU);
	
	if((PROCEDURE_T1_CMD != SC7816_Item.current_procedure) && (PROCEDURE_CLK_STOP != SC7816_Item.current_procedure))
	{
		SC7816_Item.T1_state	=	STATE_T1_CMD_FAILURE;

		xy_printf(0,PLATFORM, WARN_LOG, "uSim Not in T1 state");		

		return SC_FAILURE;
	}
	/*
	TxAPDU.cla				=	Txbuff[0];
	TxAPDU.ins				=	Txbuff[1];
	TxAPDU.p1				=	Txbuff[2];
	TxAPDU.p2				=	Txbuff[3];
	
	if(*len==5)
	{
		TxAPDU.Lc		=	0;
		if(0==Txbuff[4])
		{
			TxAPDU.Le	=	256;
		}
		else
		{
			TxAPDU.Le	=	(uint16_t)Txbuff[4];
		}
	}
	else
	{
		TxAPDU.Lc		=	Txbuff[4];
		TxAPDU.Le		=	0;
		TxAPDU.data		=	(uint8_t *)(Txbuff+5);
	}
	*/

	//RxAPDU.data			=	Rxbuff;
	//RxAPDU.sw1			=	0;
	//RxAPDU.sw2			=	0;
	sim_rx_num			=	0;
	dirty_num			=	0;

	RxBlock.inf			=	&sim_rx_buf[0];
	
	SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

	//SIM_SmartCard_WT(2);//at least 22etu for T1 block_guard_time


	//SIM_resetTxFifo();
	//SIM_resetRxFifo();
	

	SC7816_Item.T1_state=STATE_T1_CMD_SND_I;
	while(1)
	{

		switch(SC7816_Item.T1_state)
		{
			case STATE_T1_CMD_SND_I:
				//debug use
				if(tx_info_len <= tx_sended_len)
					xy_assert(0);
				
				if(tx_info_len-tx_sended_len > SC7816_Item.profile.IFSC)
				{
					tx_sending_len = SC7816_Item.profile.IFSC;
					more_data = 1;
				}
				else
				{
					tx_sending_len = tx_info_len-tx_sended_len;
					more_data = 0;
				}

				SimT1_PackIBlockTDPU(&TxBlock, SC7816_Item.profile.Ns, more_data, Txbuff+tx_sended_len, tx_sending_len);
				SimT1_SendBlockTDPU(&TxBlock);
				SC7816_Item.T1_state = STATE_T1_CMD_WAIT_BLOCK;
				break;

			case STATE_T1_CMD_WAIT_BLOCK:
				result = SimT1_RcvBlock(&RxBlock,wtx_multiplier);
				wtx_multiplier = 1;	//set to default value
				if(result == SC_FAILURE)
				{
					SC7816_Item.T1_state=STATE_T1_CMD_FAILURE;
					xy_printf(0,PLATFORM, WARN_LOG, "uSim T1 CMD:rcv failure");
					return SC_FAILURE;
				}
				else
				{
					block_type = SimT1_GetBlockType(&RxBlock);
					if(block_type == BLOCK_I)
					{
						memcpy((void*)(Rxbuff+sim_rx_num),RxBlock.inf,RxBlock.len);
						sim_rx_num += RxBlock.len;
						SC7816_Item.profile.Ns = !SC7816_Item.profile.Ns;
						if(RxBlock.Ipcb.M)
						{
							SimT1_PackRBlockTDPU(&TxBlock, !RxBlock.Ipcb.Ns);
							SimT1_SendBlockTDPU(&TxBlock);
							SC7816_Item.T1_state=STATE_T1_CMD_WAIT_BLOCK;
						}
						else
						{
							*len = sim_rx_num;
							SC7816_Item.T1_state=STATE_T1_CMD_SUCCESS;

							SIM_ClkCtrl(SIM_CLK_STOP,SIM_CLK_STOP_TIMEOUT);
							
							return SC_SUCCESS;
						}
						
						xy_printf(0,PLATFORM, WARN_LOG, "uSim T1 CMD:rcv I");

					}
					else if(block_type == BLOCK_R)
					{
						if(RxBlock.Rpcb.Nr == SC7816_Item.profile.Ns)
						{
							//re-send I block
							SC7816_Item.T1_state = STATE_T1_CMD_SND_I;
						}
						else
						{
							SC7816_Item.profile.Ns = RxBlock.Rpcb.Nr;
							tx_sended_len += tx_sending_len;
							SC7816_Item.T1_state = STATE_T1_CMD_SND_I;
						}
					}
					else if(block_type == BLOCK_S)
					{
						if(RxBlock.Spcb.cmd == BLOCK_SPCB_CMD_WTX)
						{
							wtx_multiplier 	= RxBlock.inf[0];
							s_inf_data		= RxBlock.inf[0];
							SimT1_PackSBlockTDPU(&TxBlock, BLOCK_SPCB_RSP,BLOCK_SPCB_CMD_WTX,&s_inf_data);
							SimT1_SendBlockTDPU(&TxBlock);
						}
						else if(RxBlock.Spcb.cmd == BLOCK_SPCB_CMD_IFS)
						{
							SC7816_Item.profile.IFSC = RxBlock.inf[0];
							s_inf_data		= RxBlock.inf[0];
							SimT1_PackSBlockTDPU(&TxBlock, BLOCK_SPCB_RSP,BLOCK_SPCB_CMD_IFS,&s_inf_data);
							SimT1_SendBlockTDPU(&TxBlock);
							
						}
						xy_assert(0);
					}
					else
					{
						xy_assert(0);
					}
				}
				
				break;
/*
			case STATE_T1_CMD_SND_I:
				break;

			case STATE_T1_CMD_SND_I:
				break;

			case STATE_T1_CMD_SND_I:
				break;
*/
			default:
				break;
		}
		/*
		first,send I(Ns,M),or S();
		if M = 1,wait for R(Nr),Nr=!Ns;
		if M = 0,wait for I(0,M);
		*/
	}

}


void sim_apdu_process(uint8_t *pApduBuf,uint8_t *pRxBuffer,uint32_t *uLen)
{
	//uint32_t csp_int_status;
	unsigned char loop,loopmax;
	char vol_class[2] = {VOLTAGE_CLASS_C_SUPPORT,VOLTAGE_CLASS_B_SUPPORT};//1.8v & 3v
	
	if(NULL == g_smartcard_sem)
	{
		g_smartcard_sem = osSemaphoreNew(1,0,NULL);
	}
	if(NULL == g_smartcard_mutex)
	{
    	g_smartcard_mutex = osMutexNew(NULL);
	}

	osMutexAcquire(g_smartcard_mutex, osWaitForever);

	switch(pApduBuf[0])
    {
		//case Pw_warmreset:
    	case  Pw_on: 
	    {
	        loop = 0;
            loopmax = 2;
			while(loop < loopmax)
			{
				if(pApduBuf[0] == Pw_on)
				{
					SimCard_Init();
				}
                Smartcard_Sleep_Disable();
				SimCard_PowerReset(pApduBuf[0], vol_class[loop]);
                PrintLog(0, PLATFORM, WARN_LOG, "uSim syshclkdiv %d,pclk2div %d",Get_Sys_Div(),Get_Peri2_Div());
				if(SC7816_Item.current_procedure ==	PROCEDURE_PPS)
				{
					PPS_Exchange();
					if(SC7816_Item.current_procedure !=	PROCEDURE_T0_CMD && SC7816_Item.current_procedure != PROCEDURE_T1_CMD)
					{
						PrintLog(0, PLATFORM, WARN_LOG, "uSim PPS failed:PPS_response = %x,%x,%x,%x,%x,SM_DBGSTAT=%x", PPS_response_debug.PPSS, PPS_response_debug.PPS0, PPS_response_debug.PPS1, PPS_response_debug.PPS2, PPS_response_debug.PPS3,HWREGB(SM_DBGSTAT));
					}
				}
				
				if(SC7816_Item.current_procedure ==	PROCEDURE_T0_CMD || SC7816_Item.current_procedure == PROCEDURE_T1_CMD)
				{
					if(SC7816_Item.current_procedure == PROCEDURE_T1_CMD)
					{
						//if(T1_Adjust_IFSD(T1_MAX_INF_LEN) != SC_SUCCESS)
						if(T1_SendSblockGetResponse(BLOCK_SPCB_CMD_IFS,T1_MAX_INF_LEN) == SC_FAILURE)
							xy_assert(0);
					}
					pRxBuffer[0] = 0x90;
		            pRxBuffer[1] = 0x00;
		            *uLen = 2;
					g_uicc_focus=0;
					SIM_ClkCtrl(SIM_CLK_STOP,SIM_CLK_STOP_TIMEOUT);
                    Smartcard_Sleep_Enable();
					break;//break while
				}
				else
				{
					pRxBuffer[0] = 0x63;
	                pRxBuffer[1] = 0x01;
					*uLen=2;
					SimCard_Deactivation();
					Smartcard_Sleep_Enable();
					loop++;
					if(	loop >= loopmax 
						|| (SC7816_Item.profile.class_clock !=0 && (SC7816_Item.profile.class_clock & vol_class[loop]) == 0) )
					{
						PrintLog(0, PLATFORM, WARN_LOG, "uSim initial failed!");
                        osDelay(50);
						break;//out while
					}
					else
					{
                        //delay & try next voltage
                        PrintLog(0, PLATFORM, WARN_LOG, "uSim first volclass initial failed!");
                        osDelay(50);	
					}
				}
			}

			break;
		}
		case  Pw_off:    
	    {
			if(SC7816_Item.current_procedure!=PROCEDURE_DEACTIVATION)
			{
				//SIM_ClkCtrl(SIM_CLK_RESTART,SIM_CLK_RESTART_TIMEOUT);

	        	SimCard_Deactivation();
			}
	    	
	        pRxBuffer[0] = 0x90;
	        pRxBuffer[1] = 0x00;
	        *uLen = 2;

	        break;
	    }
		default:	
	    {
            Smartcard_Sleep_Disable();
			if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T0)
			{
	    		T0_Cmd_Handler(pApduBuf,pRxBuffer,uLen);

				if(SC7816_Item.T0_state==STATE_T0_CMD_FAILURE)
				{

					pRxBuffer[0] = 0x63;
					pRxBuffer[1] = 0x01;
					*uLen=2;

					PrintLog(0,PLATFORM,WARN_LOG,"uSim SC7816_command fail,[CLA,INS,P1,P2,P3]=[%x,%x,%x,%x,%x],SM_DBGSTAT=%x", pApduBuf[0],pApduBuf[1],pApduBuf[2],pApduBuf[3],pApduBuf[4],HWREGB(SM_DBGSTAT));
#if SIM_DEBUG_CODE
                    PrintLog(0, PLATFORM, WARN_LOG, "uSim recv debug: tick_before=%x,tick_after=%x,timeout=%x,ANAXTALRDY=%x,syclkflag=%x",tickcnt_before_iso7816_recv,tickcnt_after_iso7816_recv,g_iso7816_debug_timeout,g_iso7816_debug_plllock,g_iso7816_debug_sysclk);
#endif	

					SimCard_Deactivation();
                    osDelay(10);
				}
			}
			else if(SC7816_Item.profile.T_protocol_used == PROTOCOL_T1)
			{
				T1_Cmd_Handler(pApduBuf,pRxBuffer,uLen);
				if(SC7816_Item.T1_state==STATE_T1_CMD_FAILURE)
				{
					xy_assert(0);
				}
			}
			else
			{
				xy_assert(0);
			}
            Smartcard_Sleep_Enable();
			break;
		}
	}



	osMutexRelease(g_smartcard_mutex);


}

void SC7816_command(uint8_t *pApduBuf, uint8_t *pRxBuffer, uint32_t *uLen)
{
	if( Is_softsim_type() )
		softsim_apdu_process(pApduBuf, *uLen, pRxBuffer, uLen); // SoftSIM APDU Handler
	else
		sim_apdu_process(pApduBuf,pRxBuffer,uLen); // 实体SIM
}


char testcmd1[] = "0070000001";
char testcmd2[] = "01A404040CA00000000345494453494D01";
char testcmd3[] = "817C00003BC0020004C10100C232200A434D19109B000000074B210F383639393736303330313333353734220F343630303430343630393130303836290201F4";
char testcmd4[] = "81C0000061";

//reponse to cmd4
char testcmd5[] = "0E9D553941E57A1B9C628FE268D94B29E2E01555368908ED315EE514365768FD0429988B4F7FA65AF2A8D2FA0E4A7003DDB3485563DCD92057DFFE215FA77446EA361378F16F2C69B9990DAFE73262DD811456B015B553F3DFD1170427B6CACD899000";

uint8_t sim_AsciiToHex(uint8_t cNum) 
{     
	if(cNum>='0'&&cNum<='9')
	{          
		cNum -= '0';     
	}     
	else if(cNum>='A'&&cNum<='F')
	{        
		cNum -= 'A';       
		cNum += 10;     
	}     
	else if(cNum>='a'&&cNum<='f')
	{       
		cNum -= 'a';      
		cNum += 10; 
	}  
	return cNum; 
}
void sim_cmd_convert(char *ascii_cmd,unsigned char *cmd,uint32_t asc_len)
{

	for(uint32_t i = 0;i < asc_len/2;i++)
	{
		cmd[i] = sim_AsciiToHex(ascii_cmd[i*2]) * 16 + sim_AsciiToHex(ascii_cmd[i*2+1]);
	}
}


unsigned char sim_tx[270] = {0};
unsigned char sim_rx[270] = {0};
uint32_t sim_data_len;

extern void send_debug_by_at_uart(char *buf);

volatile unsigned char yus = 1;

void smartcard_test()
{

	//unsigned char i;
	
	char str[100];

	while(yus);
	sim_tx[0] = Pw_on;
	sim_data_len = 1;
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("Pwon:%x%x",sim_rx[0],sim_rx[1]);
	//printf("ATR:%x%x%x%x%x%x%x%x%x%x",ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9]);
	sprintf(str,"Pwon:%02x%02x\n",sim_rx[0],sim_rx[1]);
	send_debug_by_at_uart(str);
	sprintf(str,"ATR:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",ATR_Rsp[0],ATR_Rsp[1],ATR_Rsp[2],ATR_Rsp[3],ATR_Rsp[4],ATR_Rsp[5],ATR_Rsp[6],ATR_Rsp[7],ATR_Rsp[8],ATR_Rsp[9]);
	send_debug_by_at_uart(str);

	/* 以下test代码适用3gpp卡*/
	
	// Select MF: 00A4000C023F00 => 9000
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xA4;
	sim_tx[2] = 0x00;
	sim_tx[3] = 0x0C;
	sim_tx[4] = 0x02;
	sim_tx[5] = 0x3F;
	sim_tx[6] = 0x00;
	
	sim_data_len = 7;
	
	sim_rx[0] = 0;
	sim_rx[1] = 0;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("Select MF:%x%x",sim_rx[0],sim_rx[1]);
	sprintf(str,"Select MF:%02x%02x\r\n",sim_rx[0],sim_rx[1]);
	send_debug_by_at_uart(str);
	// Select ICCID: 00A4080C022FE2 => 9000
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xA4;
	sim_tx[2] = 0x08;
	sim_tx[3] = 0x0C;
	sim_tx[4] = 0x02;
	sim_tx[5] = 0x2F;
	sim_tx[6] = 0xE2;
	
	sim_data_len = 7;
	
	sim_rx[0] = 0;
	sim_rx[1] = 0;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("Select ICCID:%x%x",sim_rx[0],sim_rx[1]);
	sprintf(str,"Select ICCID:%02x%02x\r\n",sim_rx[0],sim_rx[1]);
	send_debug_by_at_uart(str);
	// 00B000000A => 986811818204009947869000
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xB0;
	sim_tx[2] = 0x00;
	sim_tx[3] = 0x00;
	sim_tx[4] = 0x0A;
	
	sim_data_len = 5;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("00B000000A:%x%x%x%x%x%x%x%x%x%x%x%x",sim_rx[0],sim_rx[1],sim_rx[2],sim_rx[3],sim_rx[4],sim_rx[5],sim_rx[6],sim_rx[7],sim_rx[8],sim_rx[9],sim_rx[10],sim_rx[11]);
	sprintf(str,"00B000000A:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",sim_rx[0],sim_rx[1],sim_rx[2],sim_rx[3],sim_rx[4],sim_rx[5],sim_rx[6],sim_rx[7],sim_rx[8],sim_rx[9],sim_rx[10],sim_rx[11]);
	send_debug_by_at_uart(str);

	
	sim_data_len = strlen(testcmd1)/2;
	sim_cmd_convert(testcmd1,sim_tx,sim_data_len*2);
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);

	sim_data_len = strlen(testcmd2)/2;
	sim_cmd_convert(testcmd2,sim_tx,sim_data_len*2);
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);

	sim_data_len = strlen(testcmd3)/2;
	sim_cmd_convert(testcmd3,sim_tx,sim_data_len*2);
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);

	sim_data_len = strlen(testcmd4)/2;
	sim_cmd_convert(testcmd4,sim_tx,sim_data_len*2);
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);

	sim_data_len = strlen(testcmd5);
	while(1);
	


	/*以下代码适用T1测试卡，无3gpp文件*/
	/*
	// Select 0005
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xA4;
	sim_tx[2] = 0x00;
	sim_tx[3] = 0x00;
	sim_tx[4] = 0x02;
	sim_tx[5] = 0x00;
	sim_tx[6] = 0x05;
	
	sim_data_len = 7;
	
	sim_rx[0] = 0;
	sim_rx[1] = 0;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("Select MF:%x%x",sim_rx[0],sim_rx[1]);
	sprintf(str,"Select 0005 File:%02x%02x\r\n",sim_rx[0],sim_rx[1]);
	send_debug_by_at_uart(str);
	// write: 00D600001E000133000000FF0033000003000082540003201410140000000000030000 => 9000
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xD6;
	sim_tx[2] = 0x00;
	sim_tx[3] = 0x00;
	sim_tx[4] = 0x1E;
	sim_tx[5] = 0x00;
	sim_tx[6] = 0x01;
	sim_tx[7] = 0x33;
	sim_tx[8] = 0x00;
	sim_tx[9] = 0x00;
	sim_tx[10] = 0x00;
	sim_tx[11] = 0xFF;
	sim_tx[12] = 0x00;
	sim_tx[13] = 0x33;
	sim_tx[14] = 0x00;
	sim_tx[15] = 0x00;
	sim_tx[16] = 0x03;
	sim_tx[17] = 0x00;
	sim_tx[18] = 0x00;
	sim_tx[19] = 0x82;
	sim_tx[20] = 0x54;
	sim_tx[21] = 0x00;
	sim_tx[22] = 0x03;
	sim_tx[23] = 0x20;
	sim_tx[24] = 0x14;
	sim_tx[25] = 0x10;
	sim_tx[26] = 0x14;
	sim_tx[27] = 0x00;
	sim_tx[28] = 0x00;
	sim_tx[29] = 0x00;
	sim_tx[30] = 0x00;
	sim_tx[31] = 0x00;
	sim_tx[32] = 0x03;
	sim_tx[33] = 0x00;
	sim_tx[34] = 0x00;
	
	sim_data_len = 35;
	
	sim_rx[0] = 0;
	sim_rx[1] = 0;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("Select ICCID:%x%x",sim_rx[0],sim_rx[1]);
	sprintf(str,"Write 0005 File:%02x%02x\r\n",sim_rx[0],sim_rx[1]);
	send_debug_by_at_uart(str);

	
	// 00B000001E
	sim_tx[0] = 0x00;
	sim_tx[1] = 0xB0;
	sim_tx[2] = 0x00;
	sim_tx[3] = 0x00;
	sim_tx[4] = 0x1E;
	
	sim_data_len = 5;
	
	SC7816_command(&sim_tx[0], &sim_rx[0], &sim_data_len);
	//printf("00B000000A:%x%x%x%x%x%x%x%x%x%x%x%x",sim_rx[0],sim_rx[1],sim_rx[2],sim_rx[3],sim_rx[4],sim_rx[5],sim_rx[6],sim_rx[7],sim_rx[8],sim_rx[9],sim_rx[10],sim_rx[11]);
	sprintf(str,"Read 0005:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",sim_rx[0],sim_rx[1],sim_rx[2],sim_rx[3],sim_rx[4],sim_rx[5],sim_rx[6],sim_rx[7],sim_rx[8],sim_rx[9],sim_rx[10],sim_rx[11]);
	send_debug_by_at_uart(str);
	*/
	osThreadExit();
}

#define SMARTCARD_TEST_THREAD_NAME "sim_test"
#define SMARTCARD_TEST_TASK_STACK_SIZE    osStackShared
#define SMARTCARD_TEST_TASK_PRIORITY      osPriorityLow

void smartcard_test_init()
{
	/*
	osThreadAttr_t thread_attr = {0};

	thread_attr.name	   = SMARTCARD_THREAD_NAME;
	thread_attr.priority   = osPriorityLow;
	thread_attr.stack_size = 0x800;

	
	osThreadNew ((osThreadFunc_t)(smartcard_test),NULL,&thread_attr);
	*/

    osThreadAttr_t thread_attr;

	thread_attr.name = SMARTCARD_TEST_THREAD_NAME;
    thread_attr.attr_bits = osThreadDetached;
    thread_attr.cb_mem = NULL;
    thread_attr.cb_size = 0;
    thread_attr.stack_mem = NULL;
    thread_attr.stack_size = SMARTCARD_TEST_TASK_STACK_SIZE;
    thread_attr.priority = SMARTCARD_TEST_TASK_PRIORITY;
    thread_attr.tz_module = 0;

	osThreadNew((osThreadFunc_t)smartcard_test, NULL, &thread_attr);
}
