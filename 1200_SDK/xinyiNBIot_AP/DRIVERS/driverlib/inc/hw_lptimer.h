#ifndef __HW_LPTIMER_H__
#define __HW_LPTIMER_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the Timer register offsets.
//
//*****************************************************************************
#define LPTIMER_CNT               0x00000000  // LPTimer current count value
#define LPTIMER_RLD               0x00000004  // LPTimer reload value; In compare mode, this is the comparing value
#define LPTIMER_PWM               0x00000008  // LPTimer controls PWM modulation; Store the Capture values for the CAPTURE and CAPTURE/COMPARE modes.
#define LPTIMER_CTL               0x0000000C  // LPTimer control
#define LPTIMER_CONFIG            0x00000014  // LPTimer clock configure
#define LPTIMER_EXT_CLK_FLAG      0x00000018  // LPTimer external clock flag
#define LPTIMER_INT_ENABLE        0x40003080  // LPTimer interrupt enable
#define LPTIMER_INT_STATUS        0x40003084  // LPTimer interrupt status

//*****************************************************************************
//
// The following are defines for the bit fields in the TIMER_CNT register.
//
//*****************************************************************************
#define LPTIMER_CNT_COUNT_Pos       0
#define LPTIMER_CNT_COUNT_Msk       0x0000FFFF

//*****************************************************************************
//
// The following are defines for the bit fields in the TIMER_RLD register.
//
//*****************************************************************************
#define LPTIMER_RLD_RELOAD_Pos      0
#define LPTIMER_RLD_RELOAD_Msk      0x0000FFFF

//*****************************************************************************
//
// The following are defines for the bit fields in the TIMER_PWM register.
//
//*****************************************************************************
#define LPTIMER_PWM_PWM_Pos         0
#define LPTIMER_PWM_PWM_Msk         0x0000FFFF

//*****************************************************************************
//
// The following are defines for the bit fields in the TIMER_CTL register.
//
//*****************************************************************************
#define LPTIMER_CTL_INPCAP_Pos            0
#define LPTIMER_CTL_INPCAP_Msk            (1UL << LPTIMER_CTL_INPCAP_Pos)

#define LPTIMER_CTL_PWMD_Pos              1
#define LPTIMER_CTL_PWMD_Msk              (7UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_NO_DELAY         (0UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_2_CYCLE    (1UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_4_CYCLE    (2UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_8_CYCLE    (3UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_16_CYCLE   (4UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_32_CYCLE   (5UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_64_CYCLE   (6UL << LPTIMER_CTL_PWMD_Pos)
#define LPTIMER_CTL_PWMD_DELAY_128_CYCLE  (7UL << LPTIMER_CTL_PWMD_Pos)

#define LPTIMER_CTL_RSTCNT_Pos            4
#define LPTIMER_CTL_RSTCNT_Msk            (1UL << LPTIMER_CTL_RSTCNT_Pos)
#define LPTIMER_CTL_RSTCNT_EN             (1UL << LPTIMER_CTL_RSTCNT_Pos)
#define LPTIMER_CTL_RSTCNT_DIS            (0UL << LPTIMER_CTL_RSTCNT_Pos)

#define LPTIMER_CTL_TICONFIG_Pos          5
#define LPTIMER_CTL_TICONFIG_Msk          (3UL << LPTIMER_CTL_TICONFIG_Pos)
#define LPTIMER_CTL_TICONFIG_ALL_EVENT    (0UL << LPTIMER_CTL_TICONFIG_Pos) // Reload/Compare/Capture/Deassertion
#define LPTIMER_CTL_TICONFIG_OUTER_EVENT  (2UL << LPTIMER_CTL_TICONFIG_Pos) // Capture/Deassertion
#define LPTIMER_CTL_TICONFIG_INNER_EVENT  (3UL << LPTIMER_CTL_TICONFIG_Pos) // Reload/Compare

#define LPTIMER_CTL_TEN_Pos               7
#define LPTIMER_CTL_TEN_Msk               (1UL << LPTIMER_CTL_TEN_Pos)
#define LPTIMER_CTL_TEN_EN                (1UL << LPTIMER_CTL_TEN_Pos)
#define LPTIMER_CTL_TEN_DIS               (0UL << LPTIMER_CTL_TEN_Pos)

#define LPTIMER_CTL_TMODE_Pos             8
#define LPTIMER_CTL_TMODE_Msk             (15UL << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_ONE_SHOT        (0UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_CONTINUOUS      (1UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_COUNTER         (2UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_PWM_SINGLE      (3UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_CAPTURE         (4UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_COMPARE         (5UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_GATED           (6UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_CAP_AND_CMP     (7UL  << LPTIMER_CTL_TMODE_Pos)
#define LPTIMER_CTL_TMODE_PWM_DUAL        (8UL  << LPTIMER_CTL_TMODE_Pos)

#define LPTIMER_CTL_PRES_Pos              12
#define LPTIMER_CTL_PRES_Msk              (7UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_1         (0UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_2         (1UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_4         (2UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_8         (3UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_16        (4UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_32        (5UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_64        (6UL << LPTIMER_CTL_PRES_Pos)
#define LPTIMER_CTL_PRES_DIVIDE_128       (7UL << LPTIMER_CTL_PRES_Pos)

#define LPTIMER_CTL_TPOL_Pos              15
#define LPTIMER_CTL_TPOL_Msk              (1UL << LPTIMER_CTL_TPOL_Pos)
#define LPTIMER_CTL_TPOL_TRUE             (1UL << LPTIMER_CTL_TPOL_Pos)
#define LPTIMER_CTL_TPOL_FALSE            (0UL << LPTIMER_CTL_TPOL_Pos)

#define LPTIMER_CTL_DUAL_Pos              16                             //only lptimer1 
#define LPTIMER_CTL_DUAL_Msk              (1UL << LPTIMER_CTL_DUAL_Pos)
#define LPTIMER_CTL_DUAL_EN               (1UL << LPTIMER_CTL_DUAL_Pos)
#define LPTIMER_CTL_DUAL_DIS              (0UL << LPTIMER_CTL_DUAL_Pos)

#define LPTIMER_CTL_ALL_Msk               0x1FFFF              


//*****************************************************************************
//
// The following are defines for the bit fields in the TIMER_CONFIG register.
//
//*****************************************************************************
#define LPTIMER_CONFIG_CLK_MUX_Pos                0 
#define LPTIMER_CONFIG_CLK_MUX_Msk                (7UL << LPTIMER_CONFIG_CLK_MUX_Pos) 
#define LPTIMER_CONFIG_CLK_MUX_EXT_ONLY           (1UL << LPTIMER_CONFIG_CLK_MUX_Pos) 
#define LPTIMER_CONFIG_CLK_MUX_APB_ONLY           (2UL << LPTIMER_CONFIG_CLK_MUX_Pos)
#define LPTIMER_CONFIG_CLK_MUX_APB_EXT            (3UL << LPTIMER_CONFIG_CLK_MUX_Pos)
#define LPTIMER_CONFIG_CLK_MUX_INTER_ONLY         (4UL << LPTIMER_CONFIG_CLK_MUX_Pos)
#define LPTIMER_CONFIG_CLK_MUX_INTER_EXT          (5UL << LPTIMER_CONFIG_CLK_MUX_Pos)
#define LPTIMER_CONFIG_CLK_MUX_APB_INTER          (6UL << LPTIMER_CONFIG_CLK_MUX_Pos)


#define LPTIMER_CONFIG_GATE_FILTER_Pos            3
#define LPTIMER_CONFIG_GATE_FILTER_Msk            (7UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_NO_DELAY       (0UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_2_CYCLE  (1UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_4_CYCLE  (2UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_8_CYCLE  (3UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_1_MS     (4UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_2_MS     (5UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_4_MS     (6UL << LPTIMER_CONFIG_GATE_FILTER_Pos)
#define LPTIMER_CONFIG_GATE_FILTER_DELAY_10_MS    (7UL << LPTIMER_CONFIG_GATE_FILTER_Pos)


#define LPTIMER_CONFIG_EXTCLK_FILTER_Pos            6
#define LPTIMER_CONFIG_EXTCLK_FILTER_Msk            (3UL << LPTIMER_CONFIG_EXTCLK_FILTER_Pos)
#define LPTIMER_CONFIG_EXTCLK_FILTER_NO_DELAY       (0UL << LPTIMER_CONFIG_EXTCLK_FILTER_Pos)
#define LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_2_CYCLE  (1UL << LPTIMER_CONFIG_EXTCLK_FILTER_Pos)
#define LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_4_CYCLE  (2UL << LPTIMER_CONFIG_EXTCLK_FILTER_Pos)
#define LPTIMER_CONFIG_EXTCLK_FILTER_DELAY_8_CYCLE  (3UL << LPTIMER_CONFIG_EXTCLK_FILTER_Pos)


#define LPTIMER_CONFIG_CLK_POLARITY_Pos             8
#define LPTIMER_CONFIG_CLK_POLARITY_Msk             (3UL << LPTIMER_CONFIG_CLK_POLARITY_Pos)
#define LPTIMER_CONFIG_CLK_POLARITY_RISE            (0UL << LPTIMER_CONFIG_CLK_POLARITY_Pos)
#define LPTIMER_CONFIG_CLK_POLARITY_FALL            (1UL << LPTIMER_CONFIG_CLK_POLARITY_Pos)
#define LPTIMER_CONFIG_CLK_POLARITY_BOTH            (2UL << LPTIMER_CONFIG_CLK_POLARITY_Pos)

#define LPTIMER_CONFIG_CLK_SWITCH_Pos               10
#define LPTIMER_CONFIG_CLK_SWITCH_Msk               (1UL << LPTIMER_CONFIG_CLK_SWITCH_Pos)
#define LPTIMER_CONFIG_CLK_SWITCH_RECOVER           (0UL << LPTIMER_CONFIG_CLK_SWITCH_Pos)
#define LPTIMER_CONFIG_CLK_SWITCH_START             (1UL << LPTIMER_CONFIG_CLK_SWITCH_Pos)

#define LPTIMER_CONFIG_CLK_SWITCH_EN_Pos            11
#define LPTIMER_CONFIG_CLK_SWITCH_EN_Msk            (1UL << LPTIMER_CONFIG_CLK_SWITCH_EN_Pos)

#define LPTIMER_CONFIG_EXT_PHASE_EN_Pos             12
#define LPTIMER_CONFIG_EXT_PHASE_EN_Msk             (1UL << LPTIMER_CONFIG_EXT_PHASE_EN_Pos)

#define LPTIMER_CONFIG_EXT_PHASE_Mode_Pos           13
#define LPTIMER_CONFIG_EXT_PHASE_Mode_Msk           (1UL << LPTIMER_CONFIG_EXT_PHASE_Mode_Pos)
#define LPTIMER_CONFIG_EXT_PHASE_Mode_0             (0UL << LPTIMER_CONFIG_EXT_PHASE_Mode_Pos)
#define LPTIMER_CONFIG_EXT_PHASE_Mode_1             (1UL << LPTIMER_CONFIG_EXT_PHASE_Mode_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the LPTMR_FLAG_STATUS register.
//
//*****************************************************************************
#define LPTIMER_CLK_EXT_PIN_Pos                    0
#define LPTIMER_CLK_EXT_PIN_Msk                    (1UL << LPTIMER_CLK_EXT_PIN_Pos)

#define LPTIMER_CLK_SWITCH_FLAG_Pos                 1
#define LPTIMER_CLK_SWITCH_FLAG_Msk                 (1UL << LPTIMER_CLK_SWITCH_FLAG_Pos)

#define LPTIMER_CLK_APB_FLAG_Pos                    2
#define LPTIMER_CLK_APB_FLAG_Msk                    (1UL << LPTIMER_CLK_APB_FLAG_Pos)

#define LPTIMER_CLK_INTER_FLAG_Pos                  3
#define LPTIMER_CLK_INTER_FLAG_Msk                  (1UL << LPTIMER_CLK_INTER_FLAG_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the INTERRUPT_ENABLE register.
//
//*****************************************************************************
#define LPTIMER_INT_LPTIMER1_Pos                    0
#define LPTIMER_INT_LPTIMER1_Msk                    (1UL << LPTIMER_INT_LPTIMER1_Pos)

#define LPTIMER_INT_LPTIMER2_Pos                    1
#define LPTIMER_INT_LPTIMER2_Msk                    (1UL << LPTIMER_INT_LPTIMER2_Pos)




#endif //__HW_LPTIMER_H__
