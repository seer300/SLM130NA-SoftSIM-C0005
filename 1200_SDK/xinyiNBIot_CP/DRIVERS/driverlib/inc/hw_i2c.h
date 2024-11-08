#ifndef __HW_I2C_H__
#define __HW_I2C_H__

//*****************************************************************************
//
// The following are defines for the I2C register offsets.
//
//*****************************************************************************
#define I2C_CTL                 0x00000000  //32 bits
#define I2C_STAT                0x00000004  //16 bits
#define I2C_TX_FIFO_LEVEL       0x00000006  //8 bits
#define I2C_RX_FIFO_LEVEL       0x00000007  //8 bits
#define I2C_ADDR                0x00000008  //16 bits
#define I2C_CMD_DATA            0x0000000A  //16 bits
#define I2C_HS_SPKLEN           0x0000000C  //8 bits
#define I2C_FS_SPKLEN           0x0000000D  //8 bits
#define I2C_TX_FIFO_THD         0x0000000E  //8 bits
#define I2C_RX_FIFO_THD         0x0000000F  //8 bits
#define I2C_FS_SCL_HCNT         0x00000010  //16 bits
#define I2C_FS_SCL_LCNT         0x00000012  //16 bits
#define I2C_HS_SCL_HCNT         0x00000014  //16 bits
#define I2C_HS_SCL_LCNT         0x00000016  //16 bits
#define I2C_FS_SDA_HOLD         0x00000018  //8 bits
#define I2C_HS_SDA_HOLD         0x00000019  //8 bits
#define I2C_HS_MC               0x0000001C  //8 bits
#define I2C_BUS_IDLE_CNT        0x0000001D  //8 bits
#define I2C_TIMEOUT             0x0000001E  //16 bits
#define I2C_INT_STAT            0x00000020  //32 bits  RWC
#define I2C_INT_MASK            0x00000024  //32 bits  RW
#define I2C_FSM_STAT            0x00000028  //32 bits
#define I2C_D_SP_SR             0x0000002C  //32 bits
#define I2C_D_BUS_FREE          0x00000030  //8 bits
#define I2C_D_BYTE              0x00000031  //8 bits

//*****************************************************************************
//
// The following are defines for the bit fields in the I2C_CTL register.
//
//*****************************************************************************
#define I2C_CTL_WORK_MODE_Pos        0
#define I2C_CTL_WORK_MODE_Msk        (0xFUL << I2C_CTL_WORK_MODE_Pos)

#define I2C_CTL_MST_ADD_10B_Pos      4
#define I2C_CTL_MST_ADD_10B_Msk      (1UL << I2C_CTL_MST_ADD_10B_Pos)

#define I2C_CTL_ABORT_Pos            5
#define I2C_CTL_ABORT_Msk            (1UL << I2C_CTL_ABORT_Pos)

#define I2C_CTL_BUSY_Pos             6
#define I2C_CTL_BUSY_Msk             (1UL << I2C_CTL_BUSY_Pos)

#define I2C_CTL_SR_STOP_Pos          7
#define I2C_CTL_SR_STOP_Msk          (1UL << I2C_CTL_SR_STOP_Pos)
#define I2C_CTL_ACK_SR               (1UL << I2C_CTL_SR_STOP_Pos)
#define I2C_CTL_ACK_STOP             (0UL << I2C_CTL_SR_STOP_Pos)

#define I2C_CTL_SLV_RXF_CS_Pos       8
#define I2C_CTL_SLV_RXF_CS_EN        (1UL << I2C_CTL_SLV_RXF_CS_Pos)

#define I2C_CTL_SLV_TXE_CS_Pos       9
#define I2C_CTL_SLV_TXE_CS_EN        (1UL << I2C_CTL_SLV_TXE_CS_Pos)

#define I2C_CTL_MST_CS_Pos           10
#define I2C_CTL_MST_CS_EN            (1UL << I2C_CTL_MST_CS_Pos)

#define I2C_CTL_MST_HOLD_RX_Pos      11
#define I2C_CTL_MST_HOLD_RX_EN       (1UL << I2C_CTL_MST_HOLD_RX_Pos)

#define I2C_CTL_FORCE_REFCLK_Pos     12
#define I2C_CTL_FORCE_REFCLK_EN      (1UL << I2C_CTL_FORCE_REFCLK_Pos)

#define I2C_CTL_FORCE_PCLK_Pos       13
#define I2C_CTL_FORCE_PCLK_EN        (1UL << I2C_CTL_FORCE_PCLK_Pos)

#define I2C_CTL_RSTN_Pos             16
#define I2C_CTL_RSTN                 (1UL << I2C_CTL_RSTN_Pos)

#define I2C_CTL_SPK_Pos              17
#define I2C_CTL_SPK_EN               (1UL << I2C_CTL_SPK_Pos)

#define I2C_CTL_D_BYTE_Pos           18
#define I2C_CTL_D_BYTE_EN            (1UL << I2C_CTL_D_BYTE_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the I2C_STAT register.
//
//*****************************************************************************
#define I2C_STAT_TFNF_Pos            0
#define I2C_STAT_TFNF_Msk            (1UL << I2C_STAT_TFNF_Pos)

#define I2C_STAT_TFE_Pos             1
#define I2C_STAT_TFE_Msk             (1UL << I2C_STAT_TFE_Pos)

#define I2C_STAT_TFAE_Pos            2
#define I2C_STAT_TFAE_Msk            (1UL << I2C_STAT_TFAE_Pos)

#define I2C_STAT_RFNE_Pos            3
#define I2C_STAT_RFNE_Msk            (1UL << I2C_STAT_RFNE_Pos)

#define I2C_STAT_RFF_Pos             4
#define I2C_STAT_RFF_Msk             (1UL << I2C_STAT_RFF_Pos)

#define I2C_STAT_RFAF_Pos            5
#define I2C_STAT_RFAF_Msk            (1UL << I2C_STAT_RFAF_Pos)

#define I2C_STAT_SLV_ACT_Pos         6
#define I2C_STAT_SLV_ACT_Msk         (1UL << I2C_STAT_SLV_ACT_Pos)

#define I2C_STAT_MST_ACT_Pos         7
#define I2C_STAT_MST_ACT_Msk         (1UL << I2C_STAT_MST_ACT_Pos)

#define I2C_STAT_SLV_HOLD_TXE_Pos    8
#define I2C_STAT_SLV_HOLD_TXE_Msk    (1UL << I2C_STAT_SLV_HOLD_TXE_Pos)

#define I2C_STAT_SLV_HOLD_RXF_Pos    9
#define I2C_STAT_SLV_HOLD_RXF_Msk    (1UL << I2C_STAT_SLV_HOLD_RXF_Pos)

#define I2C_STAT_MST_HOLD_TXE_Pos    10
#define I2C_STAT_MST_HOLD_TXE_Msk    (1UL << I2C_STAT_MST_HOLD_TXE_Pos)

#define I2C_STAT_MST_HOLD_RXF_Pos    11
#define I2C_STAT_MST_HOLD_RXF_Msk    (1UL << I2C_STAT_MST_HOLD_RXF_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the I2C_INT register.
//
//*****************************************************************************
#define I2C_INT_COMP                 0x00000001  //(1<<0)
#define I2C_INT_RX_UNF               0x00000002  //(1<<1)
#define I2C_INT_RX_OVF               0x00000004  //(1<<2)
#define I2C_INT_TX_OVF               0x00000008  //(1<<3)
#define I2C_INT_ACT                  0x00000010  //(1<<4)
#define I2C_INT_GEN_CALL             0x00000020  //(1<<5)
#define I2C_INT_ARB_LOST             0x00000040  //(1<<6)
#define I2C_INT_TIMEOUT              0x00000080  //(1<<7)
#define I2C_INT_RX_AF                0x00000100  //(1<<8)
#define I2C_INT_TX_AE                0x00000200  //(1<<9)
#define I2C_INT_REQ_BUSY             0x00000400  //(1<<10)
#define I2C_INT_MST_ON_HOLD          0x00000800  //(1<<11)
#define I2C_INT_SLV_ON_HOLD          0x00001000  //(1<<12)
#define I2C_INT_MST_NACK             0x00002000  //(1<<13)
#define I2C_INT_ALL                  0x00003FFF

#endif // __HW_I2C_H__
