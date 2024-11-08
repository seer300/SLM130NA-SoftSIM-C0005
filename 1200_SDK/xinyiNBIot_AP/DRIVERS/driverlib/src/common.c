#include "common.h"
#include "prcm.h"
#include "sys_clk.h"

uint8_t REG_Bus_Field_Set(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long ulValue)
{
    uint8_t ucBitBoundary;
    uint8_t ucRegValueByte;
    uint8_t ucMaskByte;
	uint8_t ucRegBaseOffset;
    unsigned long ulRegValueWord;
    unsigned long ulRegOffset;
    unsigned long ulMaskWord;
    unsigned long ulRegValue;
    
    RegAccessMode reg_access_mode = AccessModeUnknow;
	
	ucRegBaseOffset = ulRegBase & 0x03;
	
	if(ucRegBaseOffset)
	{
		ulRegBase -= ucRegBaseOffset;
		
		ucBit_high += (ucRegBaseOffset << 3);
		
		ucBit_low  += (ucRegBaseOffset << 3);
	}
    
    if(ucBit_high < ucBit_low || (ucBit_high - ucBit_low) > 31)
    {
        return 1;
    }
    
    if(ucBit_high - ucBit_low > 7)
    {
        reg_access_mode = AccessModeWord;
    }
    else if((ucBit_high & 0xF8) != (ucBit_low & 0xF8))
    {
        reg_access_mode = AccessModeWord;
    }
    else
    {
        reg_access_mode = AccessModeByte;
    }
    
    if(reg_access_mode == AccessModeWord)
    {
        if((ucBit_high - ucBit_low == 31) && ((ucBit_low & 0x1F) == 0x00))
        {
            ulRegOffset = (ucBit_low >> 5) << 2;
            
            HWREG(ulRegBase + ulRegOffset) = ulValue;
        }
        else if((ucBit_high & 0xE0) == (ucBit_low & 0xE0))
        {
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((unsigned long)((unsigned long)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulValue << ucBitBoundary);
        }
        else
        {
            // First Word
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((unsigned long)((unsigned long)0x01 << (32 - (ucBit_low & 0x1F)))) - 0x01;
            
            ulRegValue = ulValue & ulMaskWord;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulRegValue << ucBitBoundary);
            
            // Second Word
            ulRegOffset += 4;
            
            ucBitBoundary = 0;
            
            ulMaskWord = ((unsigned long)((unsigned long)0x01 << ((ucBit_high & 0x1F) + 1))) - 0x01;
            
            ulRegValue = ulValue >> (32 - (ucBit_low & 0x1F));
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
            
            HWREG(ulRegBase + ulRegOffset) = (ulRegValueWord & (~(ulMaskWord << ucBitBoundary))) | (ulRegValue << ucBitBoundary);
        }
    }
    else
    {
        ulRegOffset = ucBit_low >> 3;
        
        ucBitBoundary = ucBit_low & 0x07;
        
        ucMaskByte = ((uint8_t)((uint8_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
        
        ucRegValueByte = HWREGB(ulRegBase + ulRegOffset);
        
        HWREGB(ulRegBase + ulRegOffset) = (ucRegValueByte & (~(ucMaskByte << ucBitBoundary))) | (ulValue << ucBitBoundary);
    }
    
    return 0;
}


uint8_t REG_Bus_Field_Get(unsigned long ulRegBase, uint8_t ucBit_high, uint8_t ucBit_low, unsigned long *ulValue)
{
    uint8_t ucBitBoundary;
    uint8_t ucRegValueByte;
    uint8_t ucMaskByte;
	uint8_t ucRegBaseOffset;
    unsigned long ulRegValueWord;
    unsigned long ulRegOffset;
    unsigned long ulMaskWord;
	unsigned long ulRegValue;
    
    RegAccessMode reg_access_mode = AccessModeUnknow;
	
	ucRegBaseOffset = ulRegBase & 0x03;
	
	if(ucRegBaseOffset)
	{
		ulRegBase -= ucRegBaseOffset;
		
		ucBit_high += (ucRegBaseOffset << 3);
		
		ucBit_low  += (ucRegBaseOffset << 3);
	}
    
    if(ucBit_high < ucBit_low || (ucBit_high - ucBit_low) > 31)
    {
        return 1;
    }
    
    if(ucBit_high - ucBit_low > 7)
    {
        reg_access_mode = AccessModeWord;
    }
    else if((ucBit_high & 0xF8) != (ucBit_low & 0xF8))
    {
        reg_access_mode = AccessModeWord;
    }
    else
    {
        reg_access_mode = AccessModeByte;
    }
    
    if(reg_access_mode == AccessModeWord)
    {
		
		if((ucBit_high - ucBit_low == 31) && ((ucBit_low & 0x1F) == 0x00))
        {
			ulRegOffset = (ucBit_low >> 5) << 2;
			
			*ulValue = HWREG(ulRegBase + ulRegOffset);
		}
		else if((ucBit_high & 0xE0) == (ucBit_low & 0xE0))
		{
			ulRegOffset = (ucBit_low >> 5) << 2;
        
			ucBitBoundary = ucBit_low & 0x1F;
			
			ulMaskWord = ((unsigned long)((unsigned long)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
			
			ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			*ulValue = (ulRegValueWord >> ucBitBoundary) & ulMaskWord;
		}
		else
        {
            // First Word
            ulRegOffset = (ucBit_low >> 5) << 2;
        
            ucBitBoundary = ucBit_low & 0x1F;
            
            ulMaskWord = ((unsigned long)((unsigned long)0x01 << (32 - (ucBit_low & 0x1F)))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			ulRegValue = (ulRegValueWord >> ucBitBoundary) & ulMaskWord;
            
            // Second Word
            ulRegOffset += 4;
            
            ucBitBoundary = 0;
            
            ulMaskWord = ((unsigned long)((unsigned long)0x01 << ((ucBit_high & 0x1F) + 1))) - 0x01;
            
            ulRegValueWord = HWREG(ulRegBase + ulRegOffset);
			
			*ulValue = (((ulRegValueWord & ulMaskWord) << (32 - (ucBit_low & 0x1F))) | ulRegValue);
        }
        
    }
    else
    {
        ulRegOffset = ucBit_low >> 3;
        
        ucBitBoundary = ucBit_low & 0x07;
        
        ucMaskByte = ((uint8_t)((uint8_t)0x01 << (ucBit_high - ucBit_low + 1))) - 0x01;
        
        ucRegValueByte = HWREGB(ulRegBase + ulRegOffset);
        
        *ulValue = (ucRegValueByte >> ucBitBoundary) & ucMaskByte;
    }
    
    return 0;
}

volatile float g_unit_timer = 0;

void delay_func_init(void)
{
    //这里计算执行一次汇编函数delay_count的cpu cycle的耗时，3表示执行一次delay_count需耗3个cpu cycle 
    //乘以1000000意味着转换g_unit_tiemr的单位为微秒
    g_unit_timer = (float)3000000 / GetAPClockFreq();

}

/**
 * @brief 用户调用的高精度延时接口，传入值单位为us，使用时必须保证不被中断打断
 * @brief 工作原理：AP通过执行cpu cycle固定的汇编循环语句达到延时效果.
 * @warning 传入参数不得超过12s
 * @warning 接口必须大于最小延时才能正常工作，低于最小延时按照最小延时处理。最小延迟时间计算公式为：3000000/AP的时钟频率*60；
 * 几个经典场景的延时如下：
 * AP时钟 HRC 4分频时，最小延迟不得低于40us，留有余量；
 * AP时钟 HRC 2分频时，最小延迟不得低于20us，留有余量；
 * AP时钟 HRC 1分频时，最小延迟不得低于10us，留有余量；
 * AP时钟 PLL 10分频时，最小延迟不得低于5us
 * @note  utc_cnt_delay接口为低精度延迟接口，粒度为30us。
 */

/*  高精度接口的延时误差为：
            xtal 1分频	                                    pll 4分频
    理论（us）	实测（us）	误差（us）           理论（us）	 实测（us）	 误差（us）
	100         100.083333	0.083333            100	        100	        0
	200	        200.166667	0.166667            200	        200	        0
	300	        300.25	    0.25                300	        300.08	    0.08
	400	        400.166667	0.166667            400	        400	        0
	500	        500.166667	0.166667            500	        500	        0
	600	        600.25	    0.25                600	        600.08	    0.08
	700	        700.166667	0.166667            700	        700.08	    0.08
	800	        800.25	    0.25                800	        800.08	    0.08
	900	        900.25	    0.25                900	        900.08	    0.08
	1000	    1000.166667	0.166667            1000	    1000.08	    0.08
 */
void delay_func_us(float tick_us)
{
    uint32_t count = (uint32_t)(tick_us / g_unit_timer);
    count = count < 61 ? 61 : count;
    delay_count(count - 60);//这里的60为计算delay_count函数入参所需的cpu cycle
}

