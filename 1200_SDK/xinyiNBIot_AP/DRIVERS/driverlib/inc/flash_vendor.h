#ifndef __FLASH_VENDOR_H__
#define __FLASH_VENDOR_H__

#include "hw_types.h"
#include "hw_dma.h"
#include "debug.h"
#include "interrupt.h"
#include "dma.h"
#include "qspi_flash.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define QSPI_DMA_INDIRECT	0x00
#define QSPI_DMA_DIRECT		0x80

#define STATUS_REG_S15_S0	0
#define STATUS_REG_S7_S0	1
#define STATUS_REG_S15_S8	2
#define STATUS_REG_S23_S16	3

enum FLASH_VENDOR {
    FLASH_UNKNOWN = 0,
    FLASH_GD25Q16,
    FLASH_GD25Q32,
    FLASH_XT25W16,
    FLASH_XT25W32,
    FLASH_PU25F16,
    FLASH_PU25F32,
	FLASH_BY25Q16,
	FLASH_BY25Q32,
	FLASH_XM25Q16,
	FLASH_XM25U32,  //一级boot该条目为25Q32
	FLASH_MAX,
};
typedef enum {
    PROTECT_CMP_0 = 0,  /* cmp 0 */
    PROTECT_CMP_1 = 1   /* cmp 1 */
} FLASH_PROTECT_CMP;

typedef struct {
    uint8_t mode;
    uint8_t cmp;
} flash_protect;

typedef enum {
    FLASH_2M_PROTECT_MODE_0 = 0,  /* cmp 0, none;             cmp 1, 000000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_1,      /* cmp 0, 1F0000H-1FFFFFH;  cmp 1, 000000H-1EFFFFH */
	FLASH_2M_PROTECT_MODE_2,      /* cmp 0, 1E0000H-1FFFFFH;  cmp 1, 000000H-1DFFFFH */
	FLASH_2M_PROTECT_MODE_3,      /* cmp 0, 1C0000H-1FFFFFH;  cmp 1, 000000H-1BFFFFH */
	FLASH_2M_PROTECT_MODE_4,      /* cmp 0, 180000H-1FFFFFH;  cmp 1, 000000H-17FFFFH */
	FLASH_2M_PROTECT_MODE_5,      /* cmp 0, 100000H-1FFFFFH;  cmp 1, 000000H-0FFFFFH */
	FLASH_2M_PROTECT_MODE_9 = 9,  /* cmp 0, 000000H-00FFFFH;  cmp 1, 010000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_10,     /* cmp 0, 000000H-01FFFFH;  cmp 1, 020000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_11,     /* cmp 0, 000000H-03FFFFH;  cmp 1, 040000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_12,     /* cmp 0, 000000H-07FFFFH;  cmp 1, 080000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_13,     /* cmp 0, 000000H-0FFFFFH;  cmp 1, 100000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_14,     /* cmp 0, 000000H-1FFFFFH;  cmp 1, none            */
	FLASH_2M_PROTECT_MODE_17 = 17,/* cmp 0, 1FF000H-1FFFFFH;  cmp 1, 000000H-1FEFFFH */
	FLASH_2M_PROTECT_MODE_18,     /* cmp 0, 1FE000H-1FFFFFH;  cmp 1, 000000H-1FDFFFH */
	FLASH_2M_PROTECT_MODE_19,     /* cmp 0, 1FC000H-1FFFFFH;  cmp 1, 000000H-1FBFFFH */
	FLASH_2M_PROTECT_MODE_20,     /* cmp 0, 1F8000H-1FFFFFH;  cmp 1, 000000H-1F7FFFH */
	FLASH_2M_PROTECT_MODE_25 = 25,/* cmp 0, 000000H-000FFFH;  cmp 1, 001000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_26,     /* cmp 0, 000000H-001FFFH;  cmp 1, 002000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_27,     /* cmp 0, 000000H-003FFFH;  cmp 1, 004000H-1FFFFFH */
	FLASH_2M_PROTECT_MODE_28,     /* cmp 0, 000000H-007FFFH;  cmp 1, 008000H-1FFFFFH */
	
	FLASH_4M_PROTECT_MODE_0 = 0,  /* cmp 0, none;             cmp 1, 000000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_1,      /* cmp 0, 3F0000H-3FFFFFH;  cmp 1, 000000H-3EFFFFH */
	FLASH_4M_PROTECT_MODE_2,      /* cmp 0, 3E0000H-3FFFFFH;  cmp 1, 000000H-3DFFFFH */
	FLASH_4M_PROTECT_MODE_3,      /* cmp 0, 3C0000H-3FFFFFH;  cmp 1, 000000H-3BFFFFH */
	FLASH_4M_PROTECT_MODE_4,      /* cmp 0, 380000H-3FFFFFH;  cmp 1, 000000H-37FFFFH */
	FLASH_4M_PROTECT_MODE_5,      /* cmp 0, 300000H-3FFFFFH;  cmp 1, 000000H-2FFFFFH */
	FLASH_4M_PROTECT_MODE_6,      /* cmp 0, 200000H-3FFFFFH;  cmp 1, 000000H-1FFFFFH */
	FLASH_4M_PROTECT_MODE_9 = 9,  /* cmp 0, 000000H-00FFFFH;  cmp 1, 010000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_10,     /* cmp 0, 000000H-01FFFFH;  cmp 1, 020000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_11,     /* cmp 0, 000000H-03FFFFH;  cmp 1, 040000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_12,     /* cmp 0, 000000H-07FFFFH;  cmp 1, 080000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_13,     /* cmp 0, 000000H-0FFFFFH;  cmp 1, 100000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_14,     /* cmp 0, 000000H-1FFFFFH;  cmp 1, 200000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_15,     /* cmp 0, 000000H-3FFFFFH;  cmp 1, none            */
	FLASH_4M_PROTECT_MODE_17 = 17,/* cmp 0, 3FF000H-3FFFFFH;  cmp 1, 000000H-3FEFFFH */
	FLASH_4M_PROTECT_MODE_18,     /* cmp 0, 3FE000H-3FFFFFH;  cmp 1, 000000H-3FDFFFH */
	FLASH_4M_PROTECT_MODE_19,     /* cmp 0, 3FC000H-3FFFFFH;  cmp 1, 000000H-3FBFFFH */
	FLASH_4M_PROTECT_MODE_20,     /* cmp 0, 3F8000H-3FFFFFH;  cmp 1, 000000H-3F7FFFH */
	FLASH_4M_PROTECT_MODE_22 = 22,/* cmp 0, 3F8000H-3FFFFFH;  cmp 1, 000000H-3F7FFFH */
	FLASH_4M_PROTECT_MODE_25 = 25,/* cmp 0, 000000H-000FFFH;  cmp 1, 001000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_26,     /* cmp 0, 000000H-001FFFH;  cmp 1, 002000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_27,     /* cmp 0, 000000H-003FFFH;  cmp 1, 004000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_28,     /* cmp 0, 000000H-007FFFH;  cmp 1, 008000H-3FFFFFH */
	FLASH_4M_PROTECT_MODE_30 = 30 /* cmp 0, 000000H-007FFFH;  cmp 1, 008000H-3FFFFFH */
} FLASH_PROTECT_MODE;

#define RDID_GD25Q16                    0xC84015
#define RDID_GD25Q32                    0xC84016
#define RDID_XT25W16                    0x0B6515
#define RDID_XT25W32                    0x0B6516
#define RDID_PU25F16                    0x856015
#define RDID_PU25F32                    0x856016
#define RDID_BY25Q16                    0x684015
#define RDID_BY25Q32                    0x684016
#define	RDID_XM25Q16					0x204215
#define	RDID_XM25Q32					0x204216
#define	RDID_XM25U32					0x205016


#define OTP_BASE_ADDR_GD25Q16           0x30000000
#define OTP_BASE_ADDR_GD25Q32           0x30001000
#define OTP_BASE_ADDR_XT25W16           0x30001000
#define OTP_BASE_ADDR_XT25W32           0x30001000
#define OTP_BASE_ADDR_PU25F16           0x30001000
#define OTP_BASE_ADDR_PU25F32           0x30001000
#define OTP_BASE_ADDR_XM25Q16			0x30001000
#define OTP_BASE_ADDR_XM25Q32			0x30001000
#define OTP_BASE_ADDR_XM25U32			0x30001000


#define FLASH_CMD_WRITE_ENABLE          0x06
#define FLASH_CMD_WRITE_DISABLE         0x04
#define FLASH_CMD_CHIP_ERASE            0x60
#define FLASH_CMD_SECTOR_ERASE          0x20
#define FLASH_CMD_BLOCK_ERASE_32K       0x52
#define FLASH_CMD_BLOCK_ERASE_64K       0xD8
#define FLASH_CMD_READ_DEVICEID         0x9F
#define FLASH_CMD_READ_UNIQUEID         0x4B
#define FLASH_CMD_SECURITY_ERASE        0x44
#define FLASH_CMD_SECURITY_WRITE        0x42
#define FLASH_CMD_SECURITY_READ         0x48
#define FLASH_CMD_PE_SUSPEND            0x75
#define FLASH_CMD_PE_RESUME             0x7A
#define FLASH_CMD_WRITE_QUAD			0x32
#define	FLASH_CMD_LOW_POWER_ENTER       0xB9
#define	FLASH_CMD_LOW_POWER_EXIT        0xAB

#define PSRAM_CMD_RSTEN                 0x66
#define PSRAM_CMD_RST                   0x99

#define PSRAM_QUAD_READ                 0x00060AEB  //0x060220EB
#define PSRAM_QUAD_WRITE                0x03000A38  //0x00022138

#define FLASH_CMD_WRITE_STATUS_REG		0x01		// 01H S7-S0 S15-S8		GD25Q16/XT25W16/XT25W32/PU25Q16/PU25Q32
#define FLASH_CMD_WRITE_STATUS_REG1		0x01        // 01H S7-S0			GD25Q32/        XM25Q16/XM25Q32
#define FLASH_CMD_WRITE_STATUS_REG2		0x31        // 31H S15-S8			GD25Q32/XT25W16/XM25Q16/XM25Q32
#define FLASH_CMD_WRITE_STATUS_REG3		0x11        // 11H S23-S16			GD25Q32/XT25W16/XM25Q16/XM25Q32

#define FLASH_CMD_READ_STATUS_REG1		0x05        // 05H S7-S0			GD25Q16/GD25Q32/XT25W16/XT25W32/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define FLASH_CMD_READ_STATUS_REG2		0x35        // 35H S15-S8			GD25Q16/GD25Q32/XT25W16/XT25W32/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define FLASH_CMD_READ_STATUS_REG3		0x15        // 15H S23-S16			GD25Q32

#define	STATUS_REG2_QE					0x02		// bit S9				GD25Q16/GD25Q32/XT25W16/XT25W32/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define	STATUS_REG2_LB					0x04		// bit S10				GD25Q16/XT25W32
#define	STATUS_REG2_LB1					0x08		// bit S11				GD25Q32/XT25W16/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define	STATUS_REG2_LB2					0x10		// bit S12				GD25Q32/XT25W16/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define	STATUS_REG2_LB3					0x20		// bit S13				GD25Q32/XT25W16/PU25Q16/PU25Q32/XM25Q16/XM25Q32
#define STATUS_REG2_SUS					0x80		// bit S15

#define STATUS_REG1_WIP					0x01		// bit S0
#define STATUS_REG1_WEL					0x02		// bit S1



#define QSPI_READ_DEFAULT               0x00000003
#define QSPI_READ_QUAD                  0x0008086B	//0x0802006B
#define QSPI_READ_XIP                   0x01040AEB	//0x041220EB
#define QSPI_READ_SECURITY              0x00080048	//0x08000048
#define QSPI_READ_UID					0x0008004B

#define QSPI_WRITE_DEFAULT              0x00000002
#define QSPI_WRITE_QUAD                 0x00000832	//0x00020032
#define QSPI_WRITE_SECURITY             0x00000042

#define QSPI_TODO_SUSPEND_COMMANDS      0x52203202  //32K Block Erase 52H, Sector Erase 20H, Quad Write 32H, DEFAULT Write 02H

#define QSPI_DELAY_DEFAULT              0x03010101

#define FLASH_DMA_CHANNEL DMA_CHANNEL_0
    
extern void FLASH_Init(QSPI_FLASH_Def *flash_vendor, unsigned int ref_clk_hz, unsigned int sclk_hz);
extern unsigned char FLASH_ReadStatusRegIdx(unsigned char ucStatusRegIdx);
extern void FLASH_WriteEnable(void);
extern void FLASH_WriteStatusReg(unsigned char ucStatusRegVal0, unsigned char ucStatusRegVal1);
extern void FLASH_WriteStatusRegIdx(unsigned char ucStatusRegIdx, unsigned char ucStatusRegVal);
extern void FLASH_StigSendCmd(unsigned char ucCmd);
extern void FLASH_ChipErase(void);
extern void FLASH_SectorErase(unsigned int address);
extern void FLASH_BlockErase32K(unsigned int address);
extern void FLASH_BlockErase64K(unsigned int address);
extern void FLASH_WriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_ReadData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_OTPReadData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_OTPWriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum,unsigned char ucMemType);
extern void FLASH_OTPUpdateData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_UpdateOtpData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_UpdateData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_FAST_WriteData(QSPI_FLASH_Def *flash_vendor, unsigned long ulSrcAddr, unsigned long ulDstAddr, unsigned long ulLenByte, unsigned long ulChannelNum, unsigned char ucMemType);
extern void FLASH_WaitIdle(void);
extern void FLASH_WaitIdleNoSus(void);
extern void FLASH_SetProtectMode(FLASH_PROTECT_MODE protect_mode, FLASH_PROTECT_CMP cmp);
extern flash_protect FLASH_ProtectDisable(void);
extern unsigned char FLASH_GetProtectMode(void);
extern unsigned char FLASH_GetProtectCmp(void);
extern void FLASH_EnableQEFlag(QSPI_FLASH_Def *flash_vendor);
extern void FLASH_EnterXIPMode(QSPI_FLASH_Def *flash_vendor, char xip_dummy);
extern void FLASH_ExitXIPMode(QSPI_FLASH_Def *flash_vendor);
extern void FLASH_SetReadMode(unsigned char devId, unsigned int mode);
extern void FLASH_SetWriteMode(unsigned char devId, unsigned int mode);
extern void FLASH_SecurityErase(unsigned long ulEraseAddr);
extern void FLASH_GetUniqueID128(QSPI_FLASH_Def *flash_vendor, unsigned char *pucUniqueId, unsigned char ucMemType);
extern unsigned long FLASH_GetDeviceID(void);
extern void FLASH_LowPowerEnter(void);
extern void FLASH_LowPowerEnter_Ext(void);
extern void FLASH_LowPowerExit(void);
extern void FLASH_LowPowerExit_Ext(void);
extern unsigned char FLASH_isXIPMode(void);
extern unsigned char FLASH_need_Resume(QSPI_FLASH_Def *flash_vendor);
extern unsigned char FLASH_GetStatusReg1(void);
extern unsigned char FLASH_GetStatusReg2(void);
extern void PSRAM_ResetEnable(void);
extern void PSRAM_Reset(void);
extern void Multiple_EnterXIPMode(void);
extern void Multiple_ExitXIPMode(void);
extern void PSRAM_Init(void);

extern void Flash_Poweron_Delay(void);
extern void Flash_Poweroff(void);

#ifdef __cplusplus
}
#endif

#endif //  __FLASH_VENDOR_H__

