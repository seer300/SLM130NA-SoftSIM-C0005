#ifndef __HW_SPI_H__
#define __HW_SPI_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the SPI register offsets.
//
//*****************************************************************************
#define SPI_CONFIG              (SPI_BASE + 0x00)  // RW 0x000 Configuration
#define SPI_INT_STATUS          (SPI_BASE + 0x04)  // RO 0x014 Interrupt Status
#define SPI_IEN                 (SPI_BASE + 0x08)  // WO - Interrupt Enable.
#define SPI_IDIS                (SPI_BASE + 0x0C)  // WO - Interrupt Disable
#define SPI_IMASK               (SPI_BASE + 0x10)  // RO 0x0 Interrupt Mask
#define SPI_ENABLE              (SPI_BASE + 0x14)  // RW 0x0 SPI Enable
#define SPI_DELAY               (SPI_BASE + 0x18)  // RW 0x0 Delay Register
#define SPI_TXD                 (SPI_BASE + 0x1C)  // WO - Transmit Data(TX FIFO)
#define SPI_RXD                 (SPI_BASE + 0x20)  // RO 0x0 Receive Data(RX FIFO)
#define SPI_SIC                 (SPI_BASE + 0x24)  // RW 0x0FF Slave Idle Count Register
#define SPI_TX_THRESH           (SPI_BASE + 0x28)  // RW 0x01 TX FIFO Threshold level
#define SPI_RX_THRESH           (SPI_BASE + 0x2C)  // RW 0x01 RX FIFO Threshold level
#define SPI_TX_FIFO_OP          (SPI_BASE + 0x30)  // RW 0x02 TX FIFO Operation
#define SPI_RX_FIFO_OP          (SPI_BASE + 0x34)  // RW 0x02 RX FIFO Operation
#define SPI_TX_FIFO_STATUS      (SPI_BASE + 0x38)  // RW 0x20 TX FIFO Status
#define SPI_RX_FIFO_STATUS      (SPI_BASE + 0x3C)  // RW 0x20 RX FIFO Status
#define SPI_MOD_ID              (SPI_BASE + 0xFC)  // RO 0x01090100 Module ID register

//*****************************************************************************
//
// The following are defines for the bit fields in the SPI_CONFIG register.
//
//*****************************************************************************
#define SPI_CONFIG_MODE_Pos             0UL
#define SPI_CONFIG_MODE_Msk             (1UL << SPI_CONFIG_MODE_Pos)
#define SPI_CONFIG_MODE_MASTER          (1UL << SPI_CONFIG_MODE_Pos)
#define SPI_CONFIG_MODE_SLAVE           (0UL << SPI_CONFIG_MODE_Pos)
#define SPI_CONFIG_CPOL_Pos             1UL
#define SPI_CONFIG_CPOL_Msk             (1UL << SPI_CONFIG_CPOL_Pos)
#define SPI_CONFIG_CPOL_HIGH            (1UL << SPI_CONFIG_CPOL_Pos)
#define SPI_CONFIG_CPOL_LOW             (0UL << SPI_CONFIG_CPOL_Pos)
#define SPI_CONFIG_CPHA_Pos             2UL
#define SPI_CONFIG_CPHA_Msk             (1UL << SPI_CONFIG_CPHA_Pos)
#define SPI_CONFIG_CPHA_ACTIVE          (0UL << SPI_CONFIG_CPHA_Pos)
#define SPI_CONFIG_CPHA_INACTIVE        (1UL << SPI_CONFIG_CPHA_Pos)
#define SPI_CONFIG_CLK_DIV_Pos          3UL
#define SPI_CONFIG_CLK_DIV_Msk          (7UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_2            (0UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_4            (1UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_8            (2UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_16           (3UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_32           (4UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_64           (5UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_128          (6UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_CLK_DIV_256          (7UL << SPI_CONFIG_CLK_DIV_Pos)
#define SPI_CONFIG_WORD_SIZE_Pos        6UL
#define SPI_CONFIG_WORD_SIZE_Msk        (3UL << SPI_CONFIG_WORD_SIZE_Pos)
#define SPI_CONFIG_WORD_SIZE_BITS_8     (0UL << SPI_CONFIG_WORD_SIZE_Pos)
#define SPI_CONFIG_WORD_SIZE_BITS_16    (1UL << SPI_CONFIG_WORD_SIZE_Pos)
#define SPI_CONFIG_WORD_SIZE_BITS_32    (3UL << SPI_CONFIG_WORD_SIZE_Pos)
#define SPI_CONFIG_CLK_SEL_Pos          8UL
#define SPI_CONFIG_CLK_SEL_Msk          (1UL << SPI_CONFIG_CLK_SEL_Pos)
#define SPI_CONFIG_CLK_SEL_SPI_REF      (0UL << SPI_CONFIG_CLK_SEL_Pos)
#define SPI_CONFIG_CLK_SEL_EXT          (1UL << SPI_CONFIG_CLK_SEL_Pos)
#define SPI_CONFIG_SS_MODE_Pos           9UL
#define SPI_CONFIG_SS_MODE_Msk          (1UL << SPI_CONFIG_SS_MODE_Pos)
#define SPI_CONFIG_SS_MODE_NORMAL       (0UL << SPI_CONFIG_SS_MODE_Pos)
#define SPI_CONFIG_SS_MODE_DECODE       (1UL << SPI_CONFIG_SS_MODE_Pos)
#define SPI_CONFIG_SS_LINES_Pos         10UL
#define SPI_CONFIG_SS_LINES_Msk         (15UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_SS0         (2UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_SS1         (1UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_NONE        (3UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_NO_PERI_SEL (15UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_OUT_00      (0UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_OUT_01      (1UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_OUT_10      (2UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_SS_LINES_OUT_11      (3UL << SPI_CONFIG_SS_LINES_Pos)
#define SPI_CONFIG_MANUAL_CS_Pos        14UL
#define SPI_CONFIG_MANUAL_CS_Msk        (1UL << SPI_CONFIG_MANUAL_CS_Pos)
#define SPI_CONFIG_SLAVE_BURST_Pos      14UL
#define SPI_CONFIG_SLAVE_BURST_Msk      (1UL << SPI_CONFIG_SLAVE_BURST_Pos)
#define SPI_CONFIG_MANUAL_START_EN_Pos  15UL
#define SPI_CONFIG_MANUAL_START_EN_Msk  (1UL << SPI_CONFIG_MANUAL_START_EN_Pos)
#define SPI_CONFIG_MANUAL_START_CMD_Pos 16UL
#define SPI_CONFIG_MANUAL_START_CMD_Msk (1UL << SPI_CONFIG_MANUAL_START_CMD_Pos)
#define SPI_CONFIG_MODE_FAIL_GEN_Pos    17UL
#define SPI_CONFIG_MODE_FAIL_GEN_Msk    (1UL << SPI_CONFIG_MODE_FAIL_GEN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the SPI_INT_STATUS register.
// Also for SPI_IEN/SPI_IDIS/SPI_IMASK registers.
//*****************************************************************************
#define SPI_INT_RX_OVERFLOW_Pos         0UL
#define SPI_INT_RX_OVERFLOW             (1UL << SPI_INT_RX_OVERFLOW_Pos)
#define SPI_INT_MODE_FAIL_Pos           1UL
#define SPI_INT_MODE_FAIL               (1UL << SPI_INT_MODE_FAIL_Pos)
#define SPI_INT_TX_FIFO_NFULL_Pos       2UL
#define SPI_INT_TX_FIFO_NFULL           (1UL << SPI_INT_TX_FIFO_NFULL_Pos)
#define SPI_INT_TX_FIFO_FULL_Pos        3UL
#define SPI_INT_TX_FIFO_FULL            (1UL << SPI_INT_TX_FIFO_FULL_Pos)
#define SPI_INT_RX_FIFO_NEMPTY_Pos      4UL
#define SPI_INT_RX_FIFO_NEMPTY          (1UL << SPI_INT_RX_FIFO_NEMPTY_Pos)
#define SPI_INT_RX_FIFO_FULL_Pos        5UL
#define SPI_INT_RX_FIFO_FULL            (1UL << SPI_INT_RX_FIFO_FULL_Pos)
#define SPI_INT_TX_FIFO_UNDERFLOW_Pos   6UL
#define SPI_INT_TX_FIFO_UNDERFLOW       (1UL << SPI_INT_TX_FIFO_UNDERFLOW_Pos)
#define SPI_INT_ALL                     0x7F

//*****************************************************************************
//
// The following are defines for the bit fields in the SPI_DELAY register.
//
//*****************************************************************************
#define SPI_DELAY_INIT_Pos              0UL
#define SPI_DELAY_INIT_Msk              (0xFFUL << SPI_DELAY_INIT_Pos)
#define SPI_DELAY_AFTER_Pos             8UL
#define SPI_DELAY_AFTER_Msk             (0xFFUL << SPI_DELAY_AFTER_Pos)
#define SPI_DELAY_BTWN_Pos              16UL
#define SPI_DELAY_BTWN_Msk              (0xFFUL << SPI_DELAY_BTWN_Pos)
#define SPI_DELAY_NSS_Pos               24UL
#define SPI_DELAY_NSS_Msk               (0xFFUL << SPI_DELAY_NSS_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the FIFO_OP register.
//
//*****************************************************************************
#define SPI_FIFO_OP_RESET_Pos           0UL
#define SPI_FIFO_OP_RESET_Msk           (1UL << SPI_FIFO_OP_RESET_Pos) 
#define SPI_FIFO_OP_Normal              (0UL << SPI_FIFO_OP_RESET_Pos)
#define SPI_FIFO_OP_Flush               (1UL << SPI_FIFO_OP_RESET_Pos)
#define SPI_FIFO_OP_START_Pos           1UL
#define SPI_FIFO_OP_START_Msk           (1UL << SPI_FIFO_OP_START_Pos) 
#define SPI_FIFO_OP_AUTO_SEND_Pos       2UL
#define SPI_FIFO_OP_AUTO_SEND_Msk       (1UL << SPI_FIFO_OP_AUTO_SEND_Pos) 

//*****************************************************************************
//
// The following are defines for the bit fields in the FIFO_STATUS register.
//
//*****************************************************************************
#define SPI_FIFO_STATUS_DATALEN_Pos     0UL
#define SPI_FIFO_STATUS_DATALEN_Msk     (0xFFUL << SPI_FIFO_STATUS_DATALEN_Pos)
#define SPI_FIFO_STATUS_FULL_Pos        8UL
#define SPI_FIFO_STATUS_FULL_Msk        (1UL << SPI_FIFO_STATUS_FULL_Pos)
#define SPI_FIFO_STATUS_EMPTY_Pos       9UL
#define SPI_FIFO_STATUS_EMPTY_Msk       (1UL << SPI_FIFO_STATUS_EMPTY_Pos)

#endif // __HW_SPI_H__
