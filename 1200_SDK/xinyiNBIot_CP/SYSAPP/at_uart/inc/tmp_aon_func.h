#ifndef TMP_AON_FUNC_H
#define TMP_AON_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_device.h"
#include "prcm.h"

/** LPUA1_CTRL可配值 **/
#ifndef AON_HRC_DIV2
    #define AON_HRC_DIV2            (0x01)
#endif
#define _32K_DIV1                   (0x05)

/** LPUA1_CLKFLAG可读值 **/
#define LPUART1_CLKSRC_AON_HRC      (0x01)
#define LPUART1_CLKSRC_32K          (0x00)

void set_lpuart1(FunctionalState NewState);
void set_lpuart1_ClkSrcDiv(uint8_t clksrc);
uint8_t get_lpuart1_ClkSrc(void);
uint32_t get_lpuart1_ClkDiv(void);
uint32_t get_lpuart1_ClkFreq(void);

uint32_t AutoBaudDetection(void);

#ifdef __cplusplus
}
#endif

#endif  /* TMP_AON_FUNC_H */
