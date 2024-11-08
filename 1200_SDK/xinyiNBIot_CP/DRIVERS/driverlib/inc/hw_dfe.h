#ifndef __HW_DFE_H__
#define __HW_DFE_H__

//*****************************************************************************
//
// The following are defines for DFE register address.
//
//*****************************************************************************
#define      DFE_RX_ACI_BW300KHZ				0
#define      DFE_RX_ACI_BW190KHZ				1

#define      DFE_CTRL							(0x00 + DFE_REG_BASE)
#define      DFE_RX_CTRL						(0x04 + DFE_REG_BASE)
#define      DFE_RX_SPUR_DET_CTRL				(0x08 + DFE_REG_BASE)
#define      DFE_RX_SPUR_DET_RAM_ADDR			(0x0C + DFE_REG_BASE)
#define      DFE_RX_SPUR_DET_RAM_CTRL			(0x0D + DFE_REG_BASE)
#define      DFE_RX_SPUR_DET_RAM_DATA			(0x10 + DFE_REG_BASE)
#define      DFE_RX_LO_FCW						(0x14 + DFE_REG_BASE)
#define      DFE_RX_LO_PHS_ADJ					(0x18 + DFE_REG_BASE)
#define      DFE_RX_TNF1_Z0_I					(0x1C + DFE_REG_BASE)
#define      DFE_RX_TNF1_Z0_Q					(0x1E + DFE_REG_BASE)
#define      DFE_RX_TNF1_ATT					(0x20 + DFE_REG_BASE)
#define      DFE_RX_TNF2_Z0_I					(0x24 + DFE_REG_BASE)
#define      DFE_RX_TNF2_Z0_Q					(0x26 + DFE_REG_BASE)
#define      DFE_RX_TNF2_ATT					(0x28 + DFE_REG_BASE)
#define      DFE_RX_TSSI_DATA					(0x2C + DFE_REG_BASE)
#define      DFE_RX_TSSI_VALID					(0x2E + DFE_REG_BASE)
#define      DFE_RX_SRC_MUX						(0x30 + DFE_REG_BASE)
#define      DFE_RX_DATA_DLY					(0x34 + DFE_REG_BASE)
#define      DFE_RX_TSSI_CIC0_GAIN				(0x40 + DFE_REG_BASE)
#define      DFE_RX_TSSI_CIC1_GAIN				(0x44 + DFE_REG_BASE)

#define      DFE_RX_CIC_GAIN					(0x48 + DFE_REG_BASE)
#define      DFE_RX_ACI_GAIN					(0x4C + DFE_REG_BASE)
#define      DFE_RX_ACI_BW						(0x4E + DFE_REG_BASE)
#define      DFE_RX_AIC_GAIN_DLY				(0x50 + DFE_REG_BASE)
#define      DFE_RX_CIC_GAIN_DLY				(0x54 + DFE_REG_BASE)
#define      DFE_RXBB_GAIN_DLY					(0x58 + DFE_REG_BASE)

#define      DFE_RXFIFO_DLY						(0x68 + DFE_REG_BASE)
#define      DFE_RX_RF_DLY						(0x6A + DFE_REG_BASE)
#define      DFE_RX_TMUX_SEL					(0x70 + DFE_REG_BASE)
#define      DFE_INT_STATUS						(0x74 + DFE_REG_BASE)

#define      RFPLL_CTRL							(0x78 + DFE_REG_BASE)

#define      DFE_TX_CTRL						(0x80 + DFE_REG_BASE)
#define      DFE_DDFS0_PHS						(0x84 + DFE_REG_BASE)
#define      DFE_DDFS0_FCW						(0x88 + DFE_REG_BASE)
#define      DFE_DDFS1_PHS						(0x8C + DFE_REG_BASE)
#define      DFE_DDFS1_FCW						(0x90 + DFE_REG_BASE)
#define      DFE_TX_SRC_MUX						(0x94 + DFE_REG_BASE)
#define      DFE_TX_SRC_MUX_H					(0x96 + DFE_REG_BASE)

#define      DFE_TX_192CNT_INIT					(0x97 + DFE_REG_BASE)
#define      DFE_TX_IQ_AMP_EST					(0x98 + DFE_REG_BASE)
#define      DFE_TX_IQ_PHS_EST_I				(0x9C + DFE_REG_BASE)
#define      DFE_TX_IQ_PHS_EST_Q				(0x9E + DFE_REG_BASE)
#define      DFE_TX_DVGA_GAIN					(0xA0 + DFE_REG_BASE)
#define      DFE_TX_DC_EST_I					(0xA4 + DFE_REG_BASE)
#define      DFE_TX_DC_EST_Q					(0xA6 + DFE_REG_BASE)

#define      DFE_TX_RF_DLY						(0xAC + DFE_REG_BASE)
#define      DFE_TXRF_PU_DLY					(0xB0 + DFE_REG_BASE)
#define      DFE_TXRF_OFF_DLY					(0xB4 + DFE_REG_BASE)

#define      DFE_TXBB_GAIN_DLY					(0xB8 + DFE_REG_BASE)
#define      DFE_TX_ACI_GAIN_DLY				(0xBC + DFE_REG_BASE)
#define      DFE_TX_CIC_GAIN_DLY				(0xBE + DFE_REG_BASE)

#define      DFE_TX_DPD_RAM_ADDR				(0xC0 + DFE_REG_BASE)
#define      DFE_TX_DPD_RAM_CTRL				(0xC1 + DFE_REG_BASE)
#define      DFE_TX_DPD_RAM_DATA				(0xC4 + DFE_REG_BASE)

#define      DFE_TX_FIFO_DLY					(0xC8 + DFE_REG_BASE)
#define      DFE_TX_CLK1P92M_DLY				(0xC9 + DFE_REG_BASE)
#define      DFE_TX_TMUX_SEL					(0xD0 + DFE_REG_BASE)

#define		 DFE_TX_BBRAM_BYPASS				(0xD4 + DFE_REG_BASE)
#define		 DFE_TX_BBRAM_CTRL					(0xD8 + DFE_REG_BASE)

#define		 DFE_TX_WND_COEFF					(0xDC + DFE_REG_BASE)

#endif // __HW_DFE_H__
