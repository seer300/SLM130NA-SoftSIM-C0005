#ifndef __HW_ISO7816_H__
#define __HW_ISO7816_H__

//*****************************************************************************
//
// The following are defines for the ISO7816 register offsets.
//
//*****************************************************************************
#define SM_ACTIVATE    				(ISO7816_BASE + 0x00)
#define SM_CLKMODE					(ISO7816_BASE + 0x01)
#define SM_TRXRETRY					(ISO7816_BASE + 0x02)
#define SM_CLKDIV					(ISO7816_BASE + 0x04)
#define SM_ETUCYC0					(ISO7816_BASE + 0x06)
#define SM_ETUCYC1					(ISO7816_BASE + 0x07)
#define SM_IDLETU					(ISO7816_BASE + 0x08)
#define SM_FIFO						(ISO7816_BASE + 0x0C)
#define SM_FIFOSTAT					(ISO7816_BASE + 0x10)
#define SM_FIFONUM					(ISO7816_BASE + 0x11)
#define SM_FIFOCTLSTAT				(ISO7816_BASE + 0x12)
#define SM_FIFOFLUSH				(ISO7816_BASE + 0x14)
#define SM_FIFO_LEVEL				(ISO7816_BASE + 0x16)
#define SM_INTSTAT					(ISO7816_BASE + 0x18)
#define SM_INTENA					(ISO7816_BASE + 0x1C)
#define SM_DBGSTAT					(ISO7816_BASE + 0x20)
#define SM_DBGCTL					(ISO7816_BASE + 0x21)
#define SM_UACFG					(ISO7816_BASE + 0x24)
#define SM_TXETUWAIT				(ISO7816_BASE + 0x25)
#define SM_TXRETRYETUWAIT			(ISO7816_BASE + 0x26)
#define SM_ATRSTAT					(ISO7816_BASE + 0x28)
#define SM_RXRETRYCNT				(ISO7816_BASE + 0x29)
#define SM_TXRETRYCNT				(ISO7816_BASE + 0x2A)


//*****************************************************************************
//
// The following are defines for the bit fields in the SM_ACTIVATE register.
//
//*****************************************************************************
#define SM_ACTIVATE_ACT_Pos			0
#define SM_ACTIVATE_ACT_Msk			(1UL << SM_ACTIVATE_ACT_Pos)

#define SM_ACTIVATE_WARMRST_Pos		1
#define SM_ACTIVATE_WARMRST_Msk		(1UL << SM_ACTIVATE_WARMRST_Pos)

#define SM_ACTIVATE_ISO7816RST_Pos	2
#define SM_ACTIVATE_ISO7816RST_Msk	(1UL << SM_ACTIVATE_ISO7816RST_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the SM_CLKMODE register.
//
//*****************************************************************************
#define SM_CLKMODE_EN_Pos			0
#define SM_CLKMODE_EN_Msk			(1UL << SM_CLKMODE_EN_Pos)
#define SM_CLKMODE_VAL_Pos			1
#define SM_CLKMODE_VAL_Msk			(1UL << SM_CLKMODE_VAL_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the SM_FIFOSTAT register.
//
//*****************************************************************************
#define SM_FIFOSTAT_FULL_Pos		6
#define SM_FIFOSTAT_FULL_Msk		(1UL << SM_FIFOSTAT_FULL_Pos)

#define SM_FIFOSTAT_EMPTY_Pos		7
#define SM_FIFOSTAT_EMPTY_Msk		(1UL << SM_FIFOSTAT_EMPTY_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the SM_INTENA register.
//
//*****************************************************************************
#define SM_INTENA_FIFO_EMPTY_Pos	0
#define SM_INTENA_FIFO_EMPTY_Msk	(1UL << SM_INTENA_FIFO_EMPTY_Pos)
#define SM_INTENA_FIFO_FULL_Pos		1
#define SM_INTENA_FIFO_FULL_Msk		(1UL << SM_INTENA_FIFO_FULL_Pos)
#define SM_INTENA_FIFO_LEVEL_Pos	2
#define SM_INTENA_FIFO_LEVEL_Msk	(1UL << SM_INTENA_FIFO_LEVEL_Pos)
#define SM_INTENA_FRAME_ERR_Pos		3
#define SM_INTENA_FRAME_ERR_Msk		(1UL << SM_INTENA_FRAME_ERR_Pos)
#define SM_INTENA_OVERRUN_ERR_Pos	4
#define SM_INTENA_OVERRUN_ERR_Msk	(1UL << SM_INTENA_OVERRUN_ERR_Pos)
#define SM_INTENA_TS_RCV_Pos		5
#define SM_INTENA_TS_RCV_Msk		(1UL << SM_INTENA_TS_RCV_Pos)
#define SM_INTENA_IDLE_Pos			6
#define SM_INTENA_IDLE_Msk			(1UL << SM_INTENA_IDLE_Pos)
#define SM_INTENA_RETRY_Pos			7
#define SM_INTENA_RETRY_Msk			(1UL << SM_INTENA_RETRY_Pos)

#define SM_INTENA_ALL_Msk			(0xFFUL)

//*****************************************************************************
//
// The following are defines for the bit fields in the SM_ATRSTAT register.
//
//*****************************************************************************
#define SM_ATRSTAT_ACTIVATED_Pos	0
#define SM_ATRSTAT_ACTIVATED_Msk	(1UL << SM_ATRSTAT_ACTIVATED_Pos)

#define SM_ATRSTAT_TS_RECEIVED_Pos	1
#define SM_ATRSTAT_TS_RECEIVED_Msk	(1UL << SM_ATRSTAT_TS_RECEIVED_Pos)

#define SM_ATRSTAT_TS_ERROR_Pos		2
#define SM_ATRSTAT_TS_ERROR_Msk		(1UL << SM_ATRSTAT_TS_ERROR_Pos)

#define SM_ATRSTAT_TS_EARLY_Pos		3
#define SM_ATRSTAT_TS_EARLY_Msk		(1UL << SM_ATRSTAT_TS_EARLY_Pos)

#define SM_ATRSTAT_TS_LATE_Pos		4
#define SM_ATRSTAT_TS_LATE_Msk		(1UL << SM_ATRSTAT_TS_LATE_Pos)

#define SM_ATRSTAT_TS_LSBF_Pos		5
#define SM_ATRSTAT_TS_LSBF_Msk		(1UL << SM_ATRSTAT_TS_LSBF_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the SM_DBGSTAT register.
//
//*****************************************************************************
#define SM_DBGSTAT_TXRUNNING_Pos	0
#define SM_DBGSTAT_TXRUNNING_Msk	(1UL << SM_DBGSTAT_TXRUNNING_Pos)

#define SM_DBGSTAT_TXPENDING_Pos	1
#define SM_DBGSTAT_TXPENDING_Msk	(1UL << SM_DBGSTAT_TXPENDING_Pos)

#define SM_DBGSTAT_RXRUNNING_Pos	2
#define SM_DBGSTAT_RXRUNNING_Msk	(1UL << SM_DBGSTAT_RXRUNNING_Pos)



#endif // __HW_ISO7816_H__
