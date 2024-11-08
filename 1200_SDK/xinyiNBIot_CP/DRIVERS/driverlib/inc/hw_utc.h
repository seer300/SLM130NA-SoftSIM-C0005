#ifndef __HW_UTC_H__
#define __HW_UTC_H__

//*****************************************************************************
//
// The following are defines for the UTC register offsets.
//
//*****************************************************************************
#define UTC_CTRL                    (UTC_BASE + 0x00000000)  //
#define UTC_HOUR_MODE               (UTC_BASE + 0x00000004)  //
#define UTC_TIMER                   (UTC_BASE + 0x00000008)  //
#define UTC_CAL                     (UTC_BASE + 0x0000000C)  //
#define UTC_ALARM_TIMER             (UTC_BASE + 0x00000010)  //
#define UTC_ALARM_CAL               (UTC_BASE + 0x00000014)  //
#define UTC_ALARM_EN                (UTC_BASE + 0x00000018)  //
#define UTC_INT_STAT                (UTC_BASE + 0x0000001C)  //
#define UTC_INT_EN                  (UTC_BASE + 0x00000020)  //
#define UTC_INT_DIS                 (UTC_BASE + 0x00000024)  //
#define UTC_INT_MSK                 (UTC_BASE + 0x00000028)  //
#define UTC_STATUS                  (UTC_BASE + 0x0000002C)  //
#define UTC_KEEP_UTC                (UTC_BASE + 0x00000030)  //
#define UTC_CLK_CNT                 (UTC_BASE + 0x00000034)  //
#define UTC_ALARM_CLK_CNT           (UTC_BASE + 0x00000038)  //
#define UTC_32768_SEL               (UTC_BASE + 0x00000040)  //
#define UTC_WDT_CTRL                (UTC_BASE + 0x00000044)  //
#define UTC_WDT_TICK_CONFIG         (UTC_BASE + 0x00000048)  //
#define UTC_WDT_TIMER_DATA          (UTC_BASE + 0x0000004C)  //
#define UTC_WDT_CALENDAR_DATA       (UTC_BASE + 0x00000050)  //
#define UTC_WDT_INT_STAT            (UTC_BASE + 0x00000054)  //


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_CTRL register.
//
//*****************************************************************************
#define UTC_CTRL_TIMER_STOP_Pos     0
#define UTC_CTRL_TIMER_STOP         (1UL << UTC_CTRL_TIMER_STOP_Pos)

#define UTC_CTRL_CAL_STOP_Pos       1
#define UTC_CTRL_CAL_STOP           (1UL << UTC_CTRL_CAL_STOP_Pos)

#define UTC_CTRL_DIVCNT_Pos         2
#define UTC_CTRL_DIVCNT             (1UL << UTC_CTRL_DIVCNT_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_HOUR_MODE register.
//
//*****************************************************************************
#define UTC_HOUR_MODE_Pos           0
#define UTC_HOUR_MODE_Msk           (1UL << UTC_HOUR_MODE_Pos)
#define UTC_HOUR_MODE_12H           (1UL << UTC_HOUR_MODE_Pos)
#define UTC_HOUR_MODE_24H           (0UL << UTC_HOUR_MODE_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_TIMER register.
//
//*****************************************************************************
#define UTC_TIMER_HU_Pos            0
#define UTC_TIMER_HU_Msk            (0xF << UTC_TIMER_HU_Pos)
#define UTC_TIMER_HT_Pos            4
#define UTC_TIMER_HT_Msk            (0xF << UTC_TIMER_HT_Pos)
#define UTC_TIMER_SU_Pos            8
#define UTC_TIMER_SU_Msk            (0xF << UTC_TIMER_SU_Pos)
#define UTC_TIMER_ST_Pos            12
#define UTC_TIMER_ST_Msk            (0x7 << UTC_TIMER_ST_Pos)
#define UTC_TIMER_MU_Pos            16
#define UTC_TIMER_MU_Msk            (0xF << UTC_TIMER_MU_Pos)
#define UTC_TIMER_MT_Pos            20
#define UTC_TIMER_MT_Msk            (0x7 << UTC_TIMER_MT_Pos)
#define UTC_TIMER_MRU_Pos           24
#define UTC_TIMER_MRU_Msk           (0xF << UTC_TIMER_MRU_Pos)
#define UTC_TIMER_MRT_Pos           28
#define UTC_TIMER_MRT_Msk           (0x3 << UTC_TIMER_MRT_Pos)
#define UTC_TIMER_PM_Pos            30
#define UTC_TIMER_PM_Msk            (1UL << UTC_TIMER_PM_Pos)
#define UTC_TIMER_PM                (1UL << UTC_TIMER_PM_Pos)
#define UTC_TIMER_CH_Pos            31
#define UTC_TIMER_CH_Msk            (1UL << UTC_TIMER_CH_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_CAL register.
//
//*****************************************************************************
#define UTC_CAL_DAY_Pos             0
#define UTC_CAL_DAY_Msk             (0x7 << UTC_CAL_DAY_Pos)
#define UTC_CAL_MU_Pos              3
#define UTC_CAL_MU_Msk              (0xF << UTC_CAL_MU_Pos)
#define UTC_CAL_MT_Pos              7
#define UTC_CAL_MT_Msk              (1UL << UTC_CAL_MT_Pos)
#define UTC_CAL_DU_Pos              8
#define UTC_CAL_DU_Msk              (0xF << UTC_CAL_DU_Pos)
#define UTC_CAL_DT_Pos              12
#define UTC_CAL_DT_Msk              (3UL << UTC_CAL_DT_Pos)
#define UTC_CAL_YU_Pos              16
#define UTC_CAL_YU_Msk              (0xF << UTC_CAL_YU_Pos)
#define UTC_CAL_YT_Pos              20
#define UTC_CAL_YT_Msk              (0xF << UTC_CAL_YT_Pos)
#define UTC_CAL_CU_Pos              24
#define UTC_CAL_CU_Msk              (0xF << UTC_CAL_CU_Pos)
#define UTC_CAL_CT_Pos              28
#define UTC_CAL_CT_Msk              (3UL << UTC_CAL_CT_Pos)
#define UTC_CAL_CH_Pos              31
#define UTC_CAL_CH_Msk              (1UL << UTC_CAL_CH_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_ALARM_TIMER register.
//
//*****************************************************************************
#define UTC_ALARM_TIMER_HU_Pos      0
#define UTC_ALARM_TIMER_HU_Msk      (0xF << UTC_ALARM_TIMER_HU_Pos)
#define UTC_ALARM_TIMER_HT_Pos      4
#define UTC_ALARM_TIMER_HT_Msk      (0xF << UTC_ALARM_TIMER_HT_Pos)
#define UTC_ALARM_TIMER_SU_Pos      8
#define UTC_ALARM_TIMER_SU_Msk      (0xF << UTC_ALARM_TIMER_SU_Pos)
#define UTC_ALARM_TIMER_ST_Pos      12
#define UTC_ALARM_TIMER_ST_Msk      (0x7 << UTC_ALARM_TIMER_ST_Pos)
#define UTC_ALARM_TIMER_MU_Pos      16
#define UTC_ALARM_TIMER_MU_Msk      (0xF << UTC_ALARM_TIMER_MU_Pos)
#define UTC_ALARM_TIMER_MT_Pos      20
#define UTC_ALARM_TIMER_MT_Msk      (0x7 << UTC_ALARM_TIMER_MT_Pos)
#define UTC_ALARM_TIMER_MRU_Pos     24
#define UTC_ALARM_TIMER_MRU_Msk     (0xF << UTC_ALARM_TIMER_MRU_Pos)
#define UTC_ALARM_TIMER_MRT_Pos     28
#define UTC_ALARM_TIMER_MRT_Msk     (0x3 << UTC_ALARM_TIMER_MRT_Pos)
#define UTC_ALARM_TIMER_PM_Pos      30
#define UTC_ALARM_TIMER_PM_Msk      (1UL << UTC_ALARM_TIMER_PM_Pos)
#define UTC_ALARM_TIMER_PM          (1UL << UTC_ALARM_TIMER_PM_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_ALARM_CAL register.
//
//*****************************************************************************
#define UTC_ALARM_CAL_MU_Pos        3
#define UTC_ALARM_CAL_MU_Msk        (0xF << UTC_ALARM_CAL_MU_Pos)
#define UTC_ALARM_CAL_MT_Pos        7
#define UTC_ALARM_CAL_MT_Msk        (1UL << UTC_ALARM_CAL_MT_Pos)
#define UTC_ALARM_CAL_DU_Pos        8
#define UTC_ALARM_CAL_DU_Msk        (0xF << UTC_ALARM_CAL_DU_Pos)
#define UTC_ALARM_CAL_DT_Pos        12
#define UTC_ALARM_CAL_DT_Msk        (3UL << UTC_ALARM_CAL_DT_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_ALARM_EN register.
//
//*****************************************************************************
#define UTC_ALARM_MIN_SEC           0x00000001
#define UTC_ALARM_SEC               0x00000002
#define UTC_ALARM_MIN               0x00000004
#define UTC_ALARM_HOUR              0x00000008
#define UTC_ALARM_DATE              0x00000010
#define UTC_ALARM_MONTH             0x00000020
#define UTC_ALARM_ALL               0x0000003F


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_INT_XXX register.
//
//*****************************************************************************
#define UTC_INT_MIN_SEC             0x00000001
#define UTC_INT_SEC                 0x00000002
#define UTC_INT_MIN                 0x00000004
#define UTC_INT_HOUR                0x00000008
#define UTC_INT_DATE                0x00000010
#define UTC_INT_MONTH               0x00000020
#define UTC_INT_ALARM               0x00000040
#define UTC_INT_ALL                 0x0000007F


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_STATUS register.
//
//*****************************************************************************
#define UTC_STATUS_VALID_TIMER_Pos          0
#define UTC_STATUS_VALID_TIMER_Msk          (1UL << UTC_STATUS_VALID_TIMER_Pos)
#define UTC_STATUS_VALID_CAL_Pos            1
#define UTC_STATUS_VALID_CAL_Msk            (1UL << UTC_STATUS_VALID_CAL_Pos)
#define UTC_STATUS_VALID_ALARM_TIMER_Pos    2
#define UTC_STATUS_VALID_ALARM_TIMER_Msk    (1UL << UTC_STATUS_VALID_ALARM_TIMER_Pos)
#define UTC_STATUS_VALID_ALARM_CAL_Pos      3
#define UTC_STATUS_VALID_ALARM_CAL_Msk      (1UL << UTC_STATUS_VALID_ALARM_CAL_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_KEEP_UTC register.
//
//*****************************************************************************
#define UTC_KEEP_UTC_Pos                    0
#define UTC_KEEP_UTC_Msk                    (1UL << UTC_KEEP_UTC_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_CLK_CNT register.
//
//*****************************************************************************
#define UTC_CLK_CNT_Pos                     0
#define UTC_CLK_CNT_Msk                     (0x7FFF << UTC_CLK_CNT_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the UTCALARMCLKCNT_CFG register.
//
//*****************************************************************************
#define UTC_ALARM_CNT_CFG_Pos       0
#define UTC_ALARM_CNT_32768_CFG_Msk       (0x7FFF << UTC_ALARM_CNT_CFG_Pos)
#define UTC_ALARM_CNT_32000_CFG_Msk       (0x1FF << UTC_ALARM_CNT_CFG_Pos)

#define UTC_ALARM_CNT_CHECK_EN_Pos  15
#define UTC_ALARM_CNT_CHECK_EN_Msk  (1UL << UTC_ALARM_CNT_CHECK_EN_Pos)
//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_WDT_CTRL register.
//
//*****************************************************************************
#define UTC_WDT_CTRL_EN_Pos                 0
#define UTC_WDT_CTRL_EN_Msk                 (1UL << UTC_WDT_CTRL_EN_Pos)

#define UTC_WDT_CTRL_MODE_Pos               1
#define UTC_WDT_CTRL_MODE_Msk               (1UL << UTC_WDT_CTRL_MODE_Pos)
#define UTC_WDT_CTRL_MODE_LONG              (1UL << UTC_WDT_CTRL_MODE_Pos)
#define UTC_WDT_CTRL_MODE_SHORT             (0UL << UTC_WDT_CTRL_MODE_Pos)

#define UTC_WDT_CTRL_RESET_Pos              2
#define UTC_WDT_CTRL_RESET_Msk              (1UL << UTC_WDT_CTRL_RESET_Pos)
#define UTC_WDT_CTRL_REPEAT_EN_Pos          3
#define UTC_WDT_CTRL_REPEAT_EN_Msk          (1UL << UTC_WDT_CTRL_REPEAT_EN_Pos)
#define UTC_WDT_CTRL_REPEAT_VALUE_Pos       4
#define UTC_WDT_CTRL_REPEAT_VALUE_Msk       (1UL << UTC_WDT_CTRL_REPEAT_VALUE_Pos)
#define UTC_WDT_CTRL_REPEAT_MAX_VALUE       (0UL << UTC_WDT_CTRL_REPEAT_VALUE_Pos)
#define UTC_WDT_CTRL_REPEAT_REG_VALUE       (1UL << UTC_WDT_CTRL_REPEAT_VALUE_Pos)

#define UTC_WDT_CTRL_INT_EN_Pos             5
#define UTC_WDT_CTRL_INT_EN_Msk             (1UL << UTC_WDT_CTRL_INT_EN_Pos)
#define UTC_WDT_CTRL_CLOCK_TICK_Pos         6
#define UTC_WDT_CTRL_CLOCK_TICK_Msk         (1UL << UTC_WDT_CTRL_CLOCK_TICK_Pos)
#define UTC_WDT_CTRL_DATA_VALID_Pos         7
#define UTC_WDT_CTRL_DATA_VALID_Msk         (1UL << UTC_WDT_CTRL_DATA_VALID_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_WDT_TICK_CONFIG register.
//
//*****************************************************************************
#define UTC_WDT_TICK_CONFIG_ACCURACY_Pos    0
#define UTC_WDT_TICK_CONFIG_ACCURACY_Msk    (0x0F << UTC_WDT_TICK_CONFIG_ACCURACY_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_WDT_TIMER_DATA register.
//
//*****************************************************************************
#define UTC_WDT_TIMER_DATA_START_VALUE_Pos  0
#define UTC_WDT_TIMER_DATA_START_VALUE_Msk  (0x0F << UTC_WDT_TIMER_DATA_START_VALUE_Pos)
#define UTC_WDT_START_AFTER_WAKEUP_Pos      8
#define UTC_WDT_START_AFTER_WAKEUP_Msk      (1UL << UTC_WDT_START_AFTER_WAKEUP_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the UTC_WDT_INT_STAT register.
//
//*****************************************************************************
#define UTC_WDT_INT_STAT_Pos                0
#define UTC_WDT_INT_STAT_Msk                (0x01 << UTC_WDT_INT_STAT_Pos)

#endif // __HW_UTC_H__

