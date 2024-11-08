#ifndef __HW_QSPI_FLASH_H__
#define __HW_QSPI_FLASH_H__

/* Transfer mode */
#define QSPI_INST_TYPE_SINGLE			0
#define QSPI_INST_TYPE_DUAL             1
#define QSPI_INST_TYPE_QUAD             2

#define QSPI_STIG_DATA_LEN_MAX			8

#define QSPI_DUMMY_CLKS_PER_BYTE		8
#define QSPI_DUMMY_BYTES_MAX			4

/****************************************************************************
 * Controller's configuration and status register (offset from QSPI_BASE)
 ****************************************************************************/
#define	QSPI_REG_CONFIG                     (QSPI_BASE + 0x00)
#define	QSPI_REG_CONFIG_ENABLE              BIT(0)
#define	QSPI_REG_CONFIG_DMA                 BIT(3)
#define	QSPI_REG_CONFIG_DIRECT              BIT(6)
#define	QSPI_REG_CONFIG_XIP_NEXT_READ       BIT(5)
#define	QSPI_REG_CONFIG_BAUD_LSB            16
#define	QSPI_REG_CONFIG_IDLE_LSB            31
#define	QSPI_REG_CONFIG_BAUD_MASK           0xF

#define	QSPI_REG_RD_INSTR_DEV0				(QSPI_BASE + 0x14)
#define	QSPI_REG_RD_INSTR_DEV1				(QSPI_BASE + 0x18)
#define	QSPI_REG_RD_INSTR_DEV2				(QSPI_BASE + 0x1C)
#define	QSPI_REG_RD_INSTR_DEV3				(QSPI_BASE + 0x20)
#define	QSPI_REG_RD_INSTR_OPCODE_LSB		0
#define	QSPI_REG_RD_INSTR_TYPE_INSTR_LSB	12
#define	QSPI_REG_RD_INSTR_TYPE_ADDR_LSB	    8
#define	QSPI_REG_RD_INSTR_TYPE_DATA_LSB	    10
#define	QSPI_REG_RD_INSTR_MODE_EN_LSB		24
#define	QSPI_REG_RD_INSTR_DUMMY_LSB		    16
#define	QSPI_REG_RD_INSTR_TYPE_INSTR_MASK	0x3
#define	QSPI_REG_RD_INSTR_TYPE_ADDR_MASK	0x3
#define	QSPI_REG_RD_INSTR_TYPE_DATA_MASK	0x3
#define	QSPI_REG_RD_INSTR_DUMMY_MASK		0x1F

#define	QSPI_REG_WR_INSTR_DEV0				(QSPI_BASE + 0x24)
#define	QSPI_REG_WR_INSTR_DEV1				(QSPI_BASE + 0x28)
#define	QSPI_REG_WR_INSTR_DEV2				(QSPI_BASE + 0x2C)
#define	QSPI_REG_WR_INSTR_DEV3				(QSPI_BASE + 0x30)
#define	QSPI_REG_WR_INSTR_OPCODE_LSB        0

#define	QSPI_REG_DELAY                      (QSPI_BASE + 0x04)
#define	QSPI_REG_DELAY_TSLCH_LSB            0
#define	QSPI_REG_DELAY_TCHSH_LSB		    8
#define	QSPI_REG_DELAY_TSD2D_LSB		    16
#define	QSPI_REG_DELAY_TSHSL_LSB		    24
#define	QSPI_REG_DELAY_TSLCH_MASK		    0xFF
#define	QSPI_REG_DELAY_TCHSH_MASK		    0xFF
#define	QSPI_REG_DELAY_TSD2D_MASK		    0xFF
#define	QSPI_REG_DELAY_TSHSL_MASK		    0xFF

#define	QSPI_REG_SIZE				    	(QSPI_BASE + 0x0C)
#define	QSPI_REG_SIZE_ADDRESS_LSB       	0
#define	QSPI_REG_SIZE_PAGE_LSB          	16
#define	QSPI_REG_SIZE_BLOCK_LSB         	8
#define	QSPI_REG_SIZE_ADDRESS_MASK      	0xF
#define	QSPI_REG_SIZE_PAGE_MASK         	0xFFFF
#define	QSPI_REG_SIZE_BLOCK_MASK        	0x1F

#define	QSPI_REG_BST_BASE_ADDR				(QSPI_BASE + 0x50)

#define	QSPI_REG_REMAP                  	(QSPI_BASE + 0x10)

#define	QSPI_REG_MODE_BIT_DEV0				(QSPI_BASE + 0x38)
#define	QSPI_REG_MODE_BIT_DEV1				(QSPI_BASE + 0x39)
#define	QSPI_REG_MODE_BIT_DEV2				(QSPI_BASE + 0x3A)
#define	QSPI_REG_MODE_BIT_DEV3				(QSPI_BASE + 0x3B)

#define QSPI_REG_RBUF_FILL_LEVEL			(QSPI_BASE + 0x46)
#define QSPI_REG_WBUF_FILL_LEVEL			(QSPI_BASE + 0x47)
#define QSPI_REG_DMA_BST_SIZE               (QSPI_BASE + 0x4C)

#define	QSPI_REG_RD_BST_CTRL				(QSPI_BASE + 0x60)
#define	QSPI_REG_RD_BST_CTRL_START			BIT(0)

#define	QSPI_REG_RD_BST_MEM_ADDR			(QSPI_BASE + 0x58)
#define	QSPI_REG_RD_BST_NUM					(QSPI_BASE + 0x5C)

#define	QSPI_REG_BST_RANGE					(QSPI_BASE + 0x54)

#define	QSPI_REG_CMDCTRL			        (QSPI_BASE + 0x8C)
#define	QSPI_REG_CMDCTRL_EXECUTE		    BIT(0)
#define	QSPI_REG_CMDCTRL_INPROGRESS	        BIT(7)
#define	QSPI_REG_CMDCTRL_DUMMY_LSB		    14
#define	QSPI_REG_CMDCTRL_WR_BYTES_LSB	    18
#define	QSPI_REG_CMDCTRL_WR_EN_LSB		    9
#define	QSPI_REG_CMDCTRL_ADD_BYTES_LSB	    16
#define	QSPI_REG_CMDCTRL_ADDR_EN_LSB	    8
#define	QSPI_REG_CMDCTRL_RD_BYTES_LSB	    21
#define	QSPI_REG_CMDCTRL_RD_EN_LSB		    10
#define	QSPI_REG_CMDCTRL_OPCODE_LSB	        24
#define QSPI_REG_CMDCTRL_MODE_EN_LSB        11
#define	QSPI_REG_CMDCTRL_DUMMY_MASK	        0x3
#define	QSPI_REG_CMDCTRL_WR_BYTES_MASK		0x7
#define	QSPI_REG_CMDCTRL_ADD_BYTES_MASK	    0x3
#define	QSPI_REG_CMDCTRL_RD_BYTES_MASK		0x7
#define	QSPI_REG_CMDCTRL_OPCODE_MASK		0xFF

#define	QSPI_REG_WR_BST_CTRL				(QSPI_BASE + 0x6C)
#define	QSPI_REG_WR_BST_CTRL_START		    BIT(0)


#define	QSPI_REG_WR_BST_MEM_ADDR			(QSPI_BASE + 0x64)
#define	QSPI_REG_WR_BST_NUM					(QSPI_BASE + 0x68)

#define	QSPI_REG_CMDADDRESS			        (QSPI_BASE + 0x90)
#define	QSPI_REG_CMDREADDATALOWER		    (QSPI_BASE + 0x94)
#define	QSPI_REG_CMDREADDATAUPPER		    (QSPI_BASE + 0x98)
#define	QSPI_REG_CMDWRITEDATALOWER		    (QSPI_BASE + 0x9C)
#define	QSPI_REG_CMDWRITEDATAUPPER		    (QSPI_BASE + 0xA0)

#define QSPI_REG_IRQMASK                    (QSPI_BASE + 0xA4)
#define QSPI_REG_IRQSTATUS                  (QSPI_BASE + 0xA8)
#define	QSPI_REG_CLK_CTRL			        (QSPI_BASE + 0xB0)

#define QSPI_SUSPEND_TIMEOUT_INT            BIT(16)

#define QSPI_ERASE_WR_CP_REQ_INT            BIT(24)
#define QSPI_ERASE_WR_AP_REQ_ACK_INT        BIT(25)
#define QSPI_ERASE_WR_CP_DONE_INT           BIT(26)

#define QSPI_ERASE_WR_AP_REQ_INT            BIT(28)
#define QSPI_ERASE_WR_CP_REQ_ACK_INT        BIT(29)
#define QSPI_ERASE_WR_AP_DONE_INT           BIT(30)


#define QSPI_POLL_STATUS					(QSPI_BASE + 0x74)
#define	QSPI_POLL_STATUS_VALID_LSB			8

#define QSPI_REG_IS_IDLE()					((readl(QSPI_REG_CONFIG) >> QSPI_REG_CONFIG_IDLE_LSB) & 0x1)
#define QSPI_POLL_VALID()					((readl(QSPI_POLL_STATUS) >> QSPI_POLL_STATUS_VALID_LSB) & 0x1)

#define QSPI_DEV0							0
#define QSPI_DEV1							1
#define QSPI_DEV2							2
#define QSPI_DEV3							3



//*****************************************************************************
//
// The following are defines for the QSPI SUSPEND/RESUME registers.
//
//*****************************************************************************
#define QSPI_INT_STATUS_CP					(QSPI_BASE + 0xAC)
#define QSPI_INT_STATUS_AP					(QSPI_BASE + 0xAE)

#define QSPI_SUS_CMD0                       (QSPI_BASE + 0xC0)
#define QSPI_SUS_CMD1                       (QSPI_BASE + 0xC1)
#define QSPI_SUS_CMD2                       (QSPI_BASE + 0xC2)
#define QSPI_SUS_CMD3                       (QSPI_BASE + 0xC3)

#define QSPI_SUS_CTRL                       (QSPI_BASE + 0xC4)
#define SUS_CTRL_SUSPEND_EN                 BIT(0)
#define SUS_CTRL_RESUME_EN                  BIT(1)
#define SUS_CTRL_CMD_CHECK_EN               BIT(4)
#define SUS_CTRL_TSUS_REPEAT_EN             BIT(5)
#define SUS_CTRL_SUSPEND_CMD_HIT			BIT(8)
#define SUS_CTRL_CS_Pos                     16
#define SUS_CTRL_CS_Msk                     (0xF << SUS_CTRL_CS_Pos)
#define SUS_CTRL_BLOCK_ALLDEVICE_Pos        24
#define SUS_CTRL_BLOCK_ALLDEVICE_Msk        (1UL << SUS_CTRL_BLOCK_ALLDEVICE_Pos)

#define	QSPI_SUS_CTRL1						(QSPI_BASE + 0xC5)

#define QSPI_SUS_CS							(QSPI_BASE + 0xC6)

#define QSPI_T_SUS_RESUME_CNT				(QSPI_BASE + 0xC8)
#define QSPI_TSUS_CNT                       (QSPI_BASE + 0xC8)
#define QSPI_TSUS_CNT_Msk                   (0xFFFFFF)

#define QSPI_TRESUME_CNT                    (QSPI_BASE + 0xCB)

#define QSPI_TSUS_TRESUME_CNT               (QSPI_BASE + 0xCC) //16 bits
#define QSPI_TRESUME_TSUS_CNT               (QSPI_BASE + 0xCE) //16 bits
#define QSPI_TSUS_TIMEOUT_CNT               (QSPI_BASE + 0xD0)
#define QSPI_SUS_OPCODE                     (QSPI_BASE + 0xD4)
#define QSPI_RESUME_OPCODE                  (QSPI_BASE + 0xD5)

#define QSPI_SUS_STATUS                     (QSPI_BASE + 0xD8)
#define QSPI_SUS_STATUS_Msk                 (3UL)

#define QSPI_SUS_AP_CTRL0                   (QSPI_BASE + 0xE0)
#define QSPI_ERASE_WR_AP_REQ                BIT(0)
#define QSPI_ERASE_WR_AP_DMA_REQ            BIT(1)

#define QSPI_SUS_AP_CTRL1                   (QSPI_BASE + 0xE1)
#define QSPI_ERASE_WR_BLOCK_AP              BIT(0)
#define QSPI_ERASE_WR_BLOCK_AP_DMA          BIT(1)

#define QSPI_SUS_AP_CTRL2                   (QSPI_BASE + 0xE2)
#define QSPI_ERASE_WR_AP2CP_ACK             BIT(0)

#define QSPI_SUS_AP_CTRL3                   (QSPI_BASE + 0xE3)
#define QSPI_ERASE_WR_AP2CP_DONE            BIT(0)

#define QSPI_SUS_AP_INT_CTRL0               (QSPI_BASE + 0xE4)
#define QSPI_ERASE_WR_CP_REQ_INT_ENA        BIT(0)
#define QSPI_ERASE_WR_AP_REQ_ACK_INT_ENA    BIT(1)
#define QSPI_ERASE_WR_CP_DONE_INT_ENA       BIT(2)

//#define QSPI_SUS_AP_INT_CTRL1               (QSPI_BASE + 0xE5)
//#define QSPI_ERASE_WR_CP_REQ_INT_CLEAR      BIT(0)
//#define QSPI_ERASE_WR_AP_REQ_ACK_INT_CLEAR  BIT(1)
//#define QSPI_ERASE_WR_CP_DONE_INT_CLEAR     BIT(2)

#define QSPI_SUS_AP_INT_STAT0               (QSPI_BASE + 0xE6)
#define QSPI_ERASE_WR_CP_REQ_RO             BIT(0)
#define QSPI_ERASE_WR_AP_REQ_ACK_RO         BIT(1)
#define QSPI_ERASE_WR_CP_DONE_RO            BIT(2)

#define QSPI_SUS_CP_CTRL0                   (QSPI_BASE + 0xF0)
#define QSPI_ERASE_WR_CP_REQ                BIT(0)
#define QSPI_ERASE_WR_CP_DMA_REQ            BIT(1)

#define QSPI_SUS_CP_CTRL1                   (QSPI_BASE + 0xF1)
#define QSPI_ERASE_WR_BLOCK_CP              BIT(0)
#define QSPI_ERASE_WR_BLOCK_CP_DMA          BIT(1)

#define QSPI_SUS_CP_CTRL2                   (QSPI_BASE + 0xF2)
#define QSPI_ERASE_WR_CP2AP_ACK             BIT(0)

#define QSPI_SUS_CP_CTRL3                   (QSPI_BASE + 0xF3)
#define QSPI_ERASE_WR_CP2AP_DONE            BIT(0)

#define QSPI_SUS_CP_INT_CTRL0               (QSPI_BASE + 0xF4)
#define QSPI_ERASE_WR_AP_REQ_INT_ENA        BIT(0)
#define QSPI_ERASE_WR_CP_REQ_ACK_INT_ENA    BIT(1)
#define QSPI_ERASE_WR_AP_DONE_INT_ENA       BIT(2)

//#define QSPI_SUS_CP_INT_CTRL1               (QSPI_BASE + 0xF5)
//#define QSPI_ERASE_WR_AP_REQ_INT_CLEAR      BIT(0)
//#define QSPI_ERASE_WR_CP_REQ_ACK_INT_CLEAR  BIT(1)
//#define QSPI_ERASE_WR_AP_DONE_INT_CLEAR     BIT(2)

#define QSPI_SUS_CP_INT_STAT0               (QSPI_BASE + 0xF6)
#define QSPI_ERASE_WR_AP_REQ_RO             BIT(0)
#define QSPI_ERASE_WR_CP_REQ_ACK_RO         BIT(1)
#define QSPI_ERASE_WR_AP_DONE_RO            BIT(2)

#define QSPI_SUS_REPEAT_NUM					(QSPI_BASE + 0xFC)
#define QSPI_RESUME_REPEAT_NUM				(QSPI_BASE + 0xFD)
#define QSPI_REPEAT_DLY						(QSPI_BASE + 0xFE)

#endif // __HW_QSOI_FLASH_H__


