#include "iso7816.h"

void ISO7816_ColdReset(void)
{
	HWREGB(SM_ACTIVATE) |= SM_ACTIVATE_ACT_Msk;
}

void ISO7816_WarmReset(void)
{
	HWREGB(SM_ACTIVATE) |= SM_ACTIVATE_WARMRST_Msk;
}

void ISO7816_Deactivation(void)
{
	HWREGB(SM_ACTIVATE) &= (~SM_ACTIVATE_ACT_Msk);
}

void ISO7816_IDLEETUSet(unsigned short nEtu)
{
	HWREGH(SM_IDLETU) = nEtu & 0xFFF;
}

void ISO7816_ClockStopEn(unsigned char ucClkLevel)
{
	if(CLOCK_LOW == ucClkLevel)
	{
		HWREGB(SM_CLKMODE) &= (~SM_CLKMODE_VAL_Msk);
	}
	else
	{
		HWREGB(SM_CLKMODE) |= SM_CLKMODE_VAL_Msk;
	}
	
	HWREGB(SM_CLKMODE) |= SM_CLKMODE_EN_Msk;
}

void ISO7816_ClockStopDis(void)
{
	HWREGB(SM_CLKMODE) &= (~SM_CLKMODE_EN_Msk);
}

void ISO7816_TRxRetrySet(unsigned char ucRetryNum)
{
	if(ucRetryNum)
		HWREGB(SM_TRXRETRY) = (1UL<<3) | (ucRetryNum & 0x07);
	else
		HWREGB(SM_TRXRETRY) = 0;
}

void ISO7816_ClockDiVSet(unsigned long ulRefClock, unsigned long ulExpectClock)
{
	unsigned char ucDiv;
	
	if(ulRefClock == ulExpectClock)
	{
		ucDiv = 0;
	}
	else
	{
		ucDiv = (ulRefClock >> 1) / ulExpectClock;
		
		if((ucDiv << 1) * ulExpectClock != ulRefClock)
		{
			ucDiv++;
		}
	}
	
	HWREGB(SM_CLKDIV) = ucDiv;
}

void ISO7816_ETUCycleSet(unsigned short int usiCyclesPerETU)
{
	HWREGH(SM_ETUCYC0) = (usiCyclesPerETU - 1) & 0x1FFF;
}

void ISO7816_IdleETUSet(unsigned short int usiIdleCnt)
{
	HWREGH(SM_IDLETU) = usiIdleCnt & 0xFFF;
}

unsigned char ISO7816_GetFifoByteNum(void)
{
	return HWREGB(SM_FIFONUM);
}

unsigned char ISO7816_GetFifoFullFlag(void)
{
	return ((HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_FULL_Msk) >> SM_FIFOSTAT_FULL_Pos);
}

unsigned char ISO7816_GetFifoEmptyFlag(void)
{
	return ((HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_EMPTY_Msk) >> SM_FIFOSTAT_EMPTY_Pos);
}

unsigned char ISO7816_ByteGet(void)
{
	return HWREGB(SM_FIFO);
}

void ISO7816_BytePut(unsigned char ucByte)
{
	HWREGB(SM_FIFO) = ucByte;
}

void ISO7816_SwitchToTxFromRx(void)
{
	while(!(HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_EMPTY_Msk));
	while((HWREGB(SM_DBGSTAT) & SM_DBGSTAT_RXRUNNING_Msk));
}

void ISO7816_SwitchToRxFromTx(void)
{
	while(!(HWREGB(SM_FIFOSTAT) & SM_FIFOSTAT_EMPTY_Msk));
	while(HWREGB(SM_FIFONUM));
	while(HWREGB(SM_DBGSTAT) & (SM_DBGSTAT_TXRUNNING_Msk | SM_DBGSTAT_TXPENDING_Msk));
}

unsigned char ISO7816_IntStatGet(unsigned long ulIntFlags)
{
	return (HWREGB(SM_INTSTAT) & ulIntFlags);
}

void ISO7816_IntStatClr(unsigned long ulIntFlags)
{
	HWREGB(SM_INTSTAT) = ulIntFlags;
}


void ISO7816_IntEnable(unsigned long ulIntFlags)
{
	HWREGB(SM_INTENA) |= ulIntFlags;
}

void ISO7816_IntDisable(unsigned long ulIntFlags)
{
	HWREGB(SM_INTENA) &= (~ulIntFlags);
}


