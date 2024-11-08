#include "rf_drv.h"
#include "common.h"

void PRCM_Clock_Mode_Force_XTAL(void)
{
	HWREGB(COREPRCM_BASE + 0x0C) |= 0x01;
    
    while((HWREG(COREPRCM_BASE + 0x0C) & 0x100) == 0)
    {
    }
}

void PRCM_Clock_Mode_Auto(void)
{
	HWREGB(COREPRCM_BASE + 0x0C) &= 0xFE;
}

// void RF_BBPLL_Cal_On(void)
// {
// 	HWREGB(COREPRCM_BASE + 0x73) |= (1 << 1);
// }

// void RF_BBPLL_Cal_Off(void)
// {
// 	HWREGB(COREPRCM_BASE + 0x73) &= ~(1 << 1);
// }

// void RF_BBPLL_DivN_Set(uint32_t ulDivN)
// {
// 	/*bbpll divN*/
// 	HWREGH(COREPRCM_ADIF_BASE + 0x30) = ulDivN; // 28
// }

// unsigned int RF_BBPLL_DivN_Get(void)
// {
// 	return (HWREGH(COREPRCM_ADIF_BASE + 0x30) & 0x3FF);
// }

// void RF_BBPLL_FracN_Set(uint32_t ulFracN)
// {
// 	HWREG(COREPRCM_ADIF_BASE + 0x34) = ulFracN;
// }

// unsigned int RF_BBPLL_FracN_Get(void)
// {
// 	return HWREG(COREPRCM_ADIF_BASE + 0x34);
// }

// uint8_t RF_BBPLL_Lock_Status_Get(void)
// {
// 	// return ((HWREGB(COREPRCM_BASE + 0x0D) >> 2) & 0x1);
// 	return ((HWREGB(COREPRCM_BASE + 0x76) >> 4) & 0x1);
// }

// void RF_BBPLL_Freq_Set(uint32_t ulFreqHz)
// {
// 	uint32_t ulDivN = 0;
// 	uint32_t ulFracN = 0;
// 	uint32_t tmpFracN = 0;
// 	uint32_t xtalCLK = 26000000;
// 	uint32_t bbpllCLK = ulFreqHz;

// 	RF_BBPLL_Cal_Off();

// 	ulDivN = (bbpllCLK * 2) / xtalCLK;
// 	tmpFracN = (bbpllCLK * 2) % xtalCLK;
// 	ulFracN = (uint32_t)((tmpFracN / (xtalCLK * 1.0)) * 0xFFFFFF);

// 	RF_BBPLL_DivN_Set(ulDivN);
// 	RF_BBPLL_FracN_Set(ulFracN);

// 	HWREGB(COREPRCM_ADIF_BASE + 0x29) |= (1 << 1); // bbpll_cntl <105> = 0b???e?e1
// 	HWREGB(COREPRCM_ADIF_BASE + 0x2B) |= (1 << 6); // bbpll_cntl <126> = 0b???e?e1

// 	/*bbpll nfvld */
// 	HWREGB(COREPRCM_ADIF_BASE + 0x32) = 0;
// 	HWREGB(COREPRCM_ADIF_BASE + 0x32) = 1;

// 	RF_BBPLL_Cal_On();
// }


//void RF_BBPLL_Cal_On(void)
//{
//	HWREGB(COREPRCM_BASE + 0x5B) |= 0x04;
//}
//
//void RF_BBPLL_Cal_Off(void)
//{
//	HWREGB(COREPRCM_BASE + 0x5B) &= 0xFB;
//}
//
//void RF_BBPLL_DivN_Set(uint32_t ulDivN)
//{
//	ulDivN &= 0x3FF;
//
//	HWREGB(COREPRCM_BASE + 0x5A) = (uint8_t)ulDivN;
//
//	HWREGB(COREPRCM_BASE + 0x5B) = (HWREGB(COREPRCM_BASE + 0x5B) & 0xFC) | (ulDivN >> 8);
//}
//
//unsigned int RF_BBPLL_DivN_Get(void)
//{
//    return ((HWREG(COREPRCM_BASE + 0x58) >> 16) & 0x3FF);
//}
//
//void RF_BBPLL_FracN_Set(uint32_t ulFracN)
//{
//	HWREG(COREPRCM_BASE + 0x54) = ulFracN;
//}
//
//unsigned int RF_BBPLL_FracN_Get(void)
//{
//    return HWREG(COREPRCM_BASE + 0x54);
//}
//
//void RF_BBPLL_Freq_Set(uint32_t ulFreqHz)
//{
//    uint64_t FreqHz = ulFreqHz * 10;
//    uint64_t tmp = 0;
//
//    uint32_t ulDivN  = 0;
//    uint32_t ulFracN = 0;
//
//    RF_BBPLL_Cal_Off();
//
//#if 1
////    RF_BBPLL_DivN_Set(18);
////    RF_BBPLL_FracN_Set(0);
//
////    RF_BBPLL_DivN_Set(20);
////    RF_BBPLL_FracN_Set(0x666666);
//
//    HWREGB(0xA011006F) &= 0xFD;
//
//#else
//    // Calculate DivN and FracN from ulFreqHz
//    tmp = (FreqHz << 24) / XTAL_CLK;
//
//    ulDivN  = (tmp >> 24);
//    ulFracN = tmp & 0xFFFFFF;
//
//    RF_BBPLL_DivN_Set(ulDivN);
//    RF_BBPLL_FracN_Set(0);      // Analog Bug
//
//    HWREGB(0xa0110068) &= 0x7F; // set bbpll_cntl<71>=0
//    HWREGB(0xA011006F) |= 0x02; // set bbpll_cntl<121>=1
//#endif
//
//    RF_BBPLL_Cal_On();
//}
//
//uint8_t RF_BBPLL_Lock_Status_Get(void)
//{
//	return ((HWREGB(COREPRCM_BASE + 0x0D) >> 2) & 0x1);
//}

void DFE_HWTX_CTL_DISABLE()
{
	HWREGB(DFE_TX_CTRL + 0x02) &= ~(uint8_t)0x2;
}

void DFE_TX_EN_DISABLE(void)
{
	HWREGB(DFE_TX_CTRL) = 0x0;
}

void RF_TXLB_BIAS_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA2,	 0,  0, 0x0);
}

void RF_TXLB_MX_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA2,	 1,  1, 0x0);
}

void RF_TXLB_PAD_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA2,  4,  4, 0x0);
}


void RF_TXLB_PA_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA2,  6,  6, 0x0);
}

void RF_TXHB_BIAS_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA1,  0,  0, 0x0);
}

void RF_TXHB_MX_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA1,  1,  1, 0x0);
}

void RF_TXHB_PAD_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA1,  4,  4, 0x0);
}

void RF_TXHB_PA_Power_Off(void)
{
	REG_Bus_Field_Set(DFE_ADIF_BASE + 0xA1,  6,  6, 0x0);
}

void RF_SIDO1P8_POWEROFF(void)
{
	/**/
	HWREGB(COREPRCM_ADIF_BASE + 0x18) &=  0xEF;
	//HWREGB(DFE_ADIF_BASE + 0xA0) &=  0xFE;
}

void DFE_TX_MODE_Disable()
{
	HWREGB(DFE_TX_CTRL + 0x02) &= ~((uint8_t)0x1);
}

void DFE_RX_EN_DISABLE(void)
{
	HWREGB(DFE_RX_CTRL) = 0x0;
}

void DFERF_CNTL_Dis(void)
{
	//REG_Bus_Field_Set(DFE_ADIF_BASE + 0xB3,	 3,  0, 0x08);	  //2019-05-05 huangxb
	HWREGB(COREPRCM_ADIF_BASE + 0x64) = 0x0;
}

void RfTXClose(void)
{
	RF_SIDO1P8_POWEROFF();
}

void rf_trx_close(void)
{
	RfTXClose();
	// core_adif ldo_ctl pd
	HWREGB(0x40004860) &= 0x08; //modify by wuxh, resolved for adc read fifo blocking

	// core_adif dfe_ctl pd
	DFERF_CNTL_Dis();
}

