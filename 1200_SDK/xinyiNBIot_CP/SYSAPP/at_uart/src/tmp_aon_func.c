#include "tmp_aon_func.h"
#include "prcm.h"


/* ==== lpuart1用到的AON_PRCM寄存器定义 ==== */
#define LPUA1_CTRL                  HWREGB(AONPRCM_BASE+0xa0)
#define LPUA1_CLKFLAG               HWREGB(AONPRCM_BASE+0xa1)

/**
 * @brief 配置lpuart1使能或失能，若使能则gpio3/4固定为lpuart1的txd/rxd，若失能则gpio3/4可以任意使用
 * @note 目前cp只有AT口会用到lpuart1,且使用时AT口初始化会自己使能lpuart
 */
void set_lpuart1(FunctionalState NewState)
{
    if(NewState==ENABLE)
        LPUA1_CTRL = (LPUA1_CTRL & ~0x03) | 0x02; //RXD使用GPIO4
    else
        LPUA1_CTRL &= ~0x03;
}


/**
 * @brief 配置lpuart1时钟源与分频系数
 * @param clksrc: AON_HRC_DIV2,_32K_DIV1
 */
void set_lpuart1_ClkSrcDiv(uint8_t clksrcdiv)
{
    LPUA1_CTRL = (LPUA1_CTRL & ~0x70) | (clksrcdiv << 4);
}


/**
 * @brief 获取lpuart1时钟源
 * @return 0:LPUART1_CLKSRC_32K, 1:LPUART1_CLKSRC_AON_HRC
 */
uint8_t get_lpuart1_ClkSrc(void)
{
    return (LPUA1_CLKFLAG);
}


/**
 * @brief 获取lpuart1分频系数
 * @return 分频值
 */
uint32_t get_lpuart1_ClkDiv(void)
{
    return (LPUA1_CTRL & 0x70) >> 4;
}


/**
 * @brief 获取lpuart1时钟频率
 * @return 频率值，单位Hz
 */
uint32_t get_lpuart1_ClkFreq(void)
{
    uint32_t clock[] = {32778, 26000000};
    uint32_t div = get_lpuart1_ClkDiv();

    if (div <= 4){
        return clock[1] >> (div == 0 ? 0 : (1 << (div - 1)));
    }
    else if (div == 5){
        return clock[0];
    }
    return 0;
}






