#ifndef __SMARTCARD_H
#define __SMARTCARD_H

#include "iso7816.h"
#include <string.h>

#define SIM_INT_EN	1
#define SIM_DEBUG_CODE 1

#define SIM_APB_CLK    (368640000/Get_Sys_Div()/Get_Peri2_Div())//(368640000/5/1)///sysclk/hclkdiv/pclk2div


#define SIM_CLK		4000000 //expected simclk
#define SIM_FIFO_MAXNUM		128

#define T1_MAX_INF_LEN 254
#define	CSP_ASYNC_TIMEOUT			8


#define PROTOCOL_T0					0
#define PROTOCOL_T1					1
#define PROTOCOL_T15				15



#define CLK_DELAY_CYCLE				9
#define WHILE_DELAY_CLK				30


#define MAX_ATR_CHARACTER_LEN 		33

#define RST_HIGH					1
#define RST_LOW						0

#define DEFAULT_Fd					372
#define DEFAULT_Dd					1
#define DEFAULT_WI					10

#define DEFAULT_WT_ETU				9600
#define BGT_ETU						22	//BLOCK WAITING TIME
#define T1_SEND_WT					10000000


#define SC_SUCCESS					1
#define SC_FAILURE					0


#define CLK_STOP_NOT_SUPPORT		0x00	//00 00 0000
#define CLK_STOP_STATE_LOW_ONLY	0x40	//01 00 0000
#define CLK_STOP_STATE_HIGH_ONLY		0x80	//10 00 0000
#define CLK_STOP_STATE_HIGH_LOW		0xc0


#define SIM_CLK_STOP				0x00
#define SIM_CLK_RESTART				0x01

#define SIM_CLK_STOP_TIMEOUT		1860
#define SIM_CLK_RESTART_TIMEOUT		744



#define  VOLTAGE_CLASS_A_SUPPORT	0x01
#define  VOLTAGE_CLASS_B_SUPPORT	0x02   //3V
#define  VOLTAGE_CLASS_C_SUPPORT	0x04   //1.8V




//#define  NV_SIMVCC_5V_BIT	0x01
#define  NV_SIMVCC_3V_BIT	0x02     //3V
#define  NV_SIMVCC_1V8_BIT	0x04     //1.8V

#define  NV_SIMVCC_ALL_BIT	(NV_SIMVCC_1V8_BIT | NV_SIMVCC_3V_BIT)


#define Pw_on    0x62
#define Pw_off   0x63
#define	Pw_warmreset	0x64

#define SM_CLK_MIN    4000000
#define SM_CLK_MAX    5000000


typedef enum
{
	 VOLTAGE_CLASS_A		,	//5v
	 VOLTAGE_CALSS_B		,	//3v
	 VOLTAGE_CLASS_C		,	//1.8v
	 VOLTAGE_CLASS_MAX
}Voltage_Class;

typedef enum{
	T1_EDC_LRC,
	T1_EDC_CRC,
}T1_EDC_TYPE;	


typedef enum{
	PROCEDURE_NONE,
	PROCEDURE_COLD_RST_ACTIVATION,
	PROCEDURE_WARM_RST_ACTIVATION,
	PROCEDURE_PPS,
	PROCEDURE_T0_CMD,
	PROCEDURE_T1_CMD,
	PROCEDURE_CLK_STOP,
	PROCEDURE_DEACTIVATION
}SimCard_Procedure;


typedef enum{
	STATE_ATR_NONE,
	//activation
	STATE_ATR_RST_LOW,
	STATE_ATR_VCC_POWERON,
	STATE_ATR_IO_MODE_IN,
	STATE_ATR_CLK_INIT,

	//cold rst
	STATE_ATR_IO_PULLUP,
	STATE_ATR_RST_HIGH,

	//atr parse
	STATE_ATR_PARSE_TS,
	STATE_ATR_PARSE_T0,
	STATE_ATR_PARSE_TA1,
	STATE_ATR_PARSE_TB1,
	STATE_ATR_PARSE_TC1,
	STATE_ATR_PARSE_TD1,
	STATE_ATR_PARSE_TA2,
	STATE_ATR_PARSE_TB2,
	STATE_ATR_PARSE_TC2,
	STATE_ATR_PARSE_TD2,
	STATE_ATR_PARSE_TAi,
	STATE_ATR_PARSE_TBi,
	STATE_ATR_PARSE_TCi,
	STATE_ATR_PARSE_TDi,
	STATE_ATR_PARSE_HISTORY_BYTES,
	STATE_ATR_PARSE_TCK,

	//result
	STATE_ATR_SUCCESS,
	STATE_ATR_FAILURE
}ATR_CTRL_STATE;


typedef enum{
	STATE_PPS_NONE,

	//pps request
	STATE_PPSS_REQUEST,
	STATE_PPS0_REQUEST,
	STATE_PPS1_REQUEST,
	STATE_PPS2_REQUEST,
	STATE_PPS3_REQUEST,
	STATE_PCK_REQUEST,

	//pps response
	STATE_PPSS_RESPONSE,
	STATE_PPS0_RESPONSE,
	STATE_PPS1_RESPONSE,
	STATE_PPS2_RESPONSE,
	STATE_PPS3_RESPONSE,
	STATE_PCK_RESPONSE,

	//pps result
	STATE_PPS_SUCCESS,
	STATE_PPS_FAILURE
		
}PPS_EXCHANGE_STATE;

typedef enum{
	STATE_T0_CMD_NONE,
	STATE_T0_CMD_NULL,
	STATE_T0_CMD_ACK,
	STATE_T0_CMD_INS,
	STATE_T0_CMD_COMPLEMENT_INS,
	STATE_T0_CMD_SW1,
	STATE_T0_CMD_SUCCESS,
	STATE_T0_CMD_FAILURE
}T0_CMD_STATE;

typedef enum{
	STATE_T1_CMD_NONE,
	STATE_T1_CMD_WAIT_BLOCK,

	STATE_T1_CMD_SND_I,
	STATE_T1_CMD_RCV_I,

	STATE_T1_CMD_RCV_R_ACK,
	STATE_T1_CMD_RCV_R_NAK,
	STATE_T1_CMD_RCV_DATA,
	STATE_T1_CMD_SND_S_IFS_REQ,
	STATE_T1_CMD_SND_S_WTX_RSP,
	STATE_T1_CMD_SND_R_ACK,
	STATE_T1_CMD_SND_R_NAK,
	STATE_T1_CMD_RCV_S_ABORT_RSP,
	STATE_T1_CMD_RCV_S_SYNCH_RSP,
	STATE_T1_CMD_RCV_S_IFS_REQ,
	STATE_T1_CMD_RCV_S_IFS_RSP,

	STATE_T1_CMD_SUCCESS,
	STATE_T1_CMD_FAILURE
}T1_CMD_STATE;


typedef enum{
	BLOCK_I,
	BLOCK_R,
	BLOCK_S,
	BLOCK_ERROR
}T1_BLOCK_TYPE;	

typedef enum{
	BLOCK_RPCB_ERROR_FREE,
	BLOCK_RPCB_ERROR_CODE_PARITY,
	BLOCK_RPCB_ERROR_OTHER,
}T1_RPCB_ERROR;

typedef enum{
	BLOCK_SPCB_REQ,
	BLOCK_SPCB_RSP,
}T1_SPCB_REQ_RSP;

typedef enum{
	BLOCK_SPCB_CMD_RESYNCH,
	BLOCK_SPCB_CMD_IFS,
	BLOCK_SPCB_CMD_ABORT,
	BLOCK_SPCB_CMD_WTX,
}T1_SPCB_CMD_TYPE;	

typedef struct {

	union
	{
		struct{
			uint8_t		sad:3;		//source node address
			uint8_t		res1:1;		//0
			uint8_t		dad:3;		//destination node address
			uint8_t		res2:1;		//0
		}nad;
		
		uint8_t nad_byte;
	};
	

	union{
		struct{
			uint8_t		res:5;		//
			uint8_t		M:1;		//more-data bit
			uint8_t		Ns:1;		//send-sequence number
			uint8_t		bit8:1;		//0
		}Ipcb;

		
		struct{
			uint8_t		error:4;		//
			uint8_t		Nr:1;			//
			uint8_t		bit6:1;			//0
			uint8_t		bit8_7:2;		//10	
		}Rpcb;

		
		struct{
			uint8_t		cmd:5;		//destination node address
			uint8_t		req_rsp:1;		//request or response
			uint8_t		bit8_7:2;		//11
		}Spcb;

		uint8_t pcb_byte;
	};
	

	uint8_t 		len;
	
	uint8_t 		*inf;

	union {
		uint8_t		lrc;
		uint16_t	crc;
	}edc;
	
	
}ISO7816_BLOCK;


typedef struct {
	uint8_t 		TA2_SpecificMode;
	uint8_t			class_clock;
	uint8_t 		T_protocol_used;	//T0,or T1
	uint8_t 		WI;

	uint8_t 		TA1;	//
	uint8_t 		N;		//TC1,extra guard time
	uint8_t 		edc;	//lrc,crc
	uint8_t 		Ns;		//send sequence num

	//global,T0,T1,T15,to identify the first character after T1,T15
	uint8_t			global_atrpara_present;
	uint8_t			T0_atrpara_present;
	uint8_t			T1_atrpara_present;
	uint8_t			T15_atrpara_present;
	
	uint8_t 		IFSC;
	uint8_t 		IFSD;
	uint8_t 		CWI;	//character waiting interger
	uint8_t 		BWI;	//block waiting interger
	
	uint16_t 		Fi;		//indicated by TA1
	uint16_t 		Di;
	uint16_t 		Fp;		//parameters tamsmitted in pps,or to be used in cmd
	uint16_t 		Dp;
	uint16_t 		F;		//parameters is using now
	uint16_t 		D;
	uint16_t 		R;		//see 8.3
	uint16_t		clk_div;
	uint16_t 		guard_time;
	uint16_t 		T_indicated;	//protocols that card can support,bitmap,T0,T1,T15
	Voltage_Class 	current_class;		//size is 4 byte
}SimCard_Profile;

typedef enum{
	
	ATR_GLOBAL_PARA_TA1_POS,
	ATR_GLOBAL_PARA_TB1_POS,
	ATR_GLOBAL_PARA_TC1_POS,

	ATR_GLOBAL_PARA_TA2_POS,
	ATR_GLOBAL_PARA_TB2_POS,

}ATR_GLOBAL_PARA_POSITION;

typedef enum{
	
	ATR_SPECIFIC_PARA_TA_POS,
	ATR_SPECIFIC_PARA_TB_POS,
	ATR_SPECIFIC_PARA_TC_POS,

}ATR_SPECIFIC_PARA_POSITION;

typedef struct{
	SimCard_Profile 	profile;
	SimCard_Procedure	current_procedure;
	ATR_CTRL_STATE 		atr_state;
	PPS_EXCHANGE_STATE 	pps_state;
	T0_CMD_STATE		T0_state;
	T1_CMD_STATE		T1_state;

}SimCard_Item;

typedef struct{
	uint8_t 	PPSS;
	uint8_t 	PPS0;
	uint8_t 	PPS1;
	uint8_t 	PPS2;
	uint8_t 	PPS3;
	uint8_t 	PCK;
}PPS_element;


typedef struct{
	uint8_t 	cla;
	uint8_t 	ins;
	uint8_t 	p1;
	uint8_t 	p2;
	uint8_t 	Lc;
	uint8_t 	*data;
	uint16_t 	Le;			//max is 256 ,larger than uint8_t range

}T_Command_APDU;

typedef struct{

	uint8_t 	*data;
	uint8_t 	sw1;
	uint8_t 	sw2;

}T_Response_APDU;

//chip adapt interface
#define ISO7816_Reset() 			(HWREGB(SM_ACTIVATE) |= SM_ACTIVATE_ISO7816RST_Msk)
#define ISO7816_GetIntEna_ISR() 	(HWREGB(SM_INTENA))
#define ISO7816_IntDisAll_ISR() 	(HWREGB(SM_INTENA) = 0)
#define ISO7816_IntClr_ISR(para) 	(HWREGB(SM_INTSTAT) = (para))
#define ISO7816_StopbitsSet(para) 	(HWREGB(SM_UACFG) = (HWREGB(SM_UACFG) & 0xFC) | ( ((para)-1) & 0x03))
#define ISO7816_RxStopbitsSet(para) (HWREGB(SM_UACFG) = (HWREGB(SM_UACFG) & 0x7F) | ( (para)-1))
#define ISO7816_TxETUWaitSet(para) 	(HWREGB(SM_TXETUWAIT) = (para) & 0xFF)
#define ISO7816_TxRetryETUWaitEn() 	(HWREGB(SM_TXRETRYETUWAIT) = 0x01)
#define ISO7816_TxRetryETUWaitDis() (HWREGB(SM_TXRETRYETUWAIT) = 0)

#define ISO7816_ClkDivGet()			(HWREGB(SM_CLKDIV))
#define ISO7816_FifoLevelSet(para)	(HWREGB(SM_FIFO_LEVEL) = (para) & 0xFF)
#define ISO7816_FifoFlush()	        do{HWREGB(SM_FIFOFLUSH) = 1;HWREGB(SM_FIFOFLUSH) = 0;}while(0)

uint8_t Smartcard_Sleep_Allow_Get();

void SC7816_command(uint8_t *pApduBuf,uint8_t *pRxBuffer,uint32_t *uLen);

void SimCard_Deactivation(void);


#endif /* __SMARTCARD_H */
