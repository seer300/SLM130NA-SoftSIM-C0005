#ifndef __HW_CRC_H__
#define __HW_CRC_H__

//*****************************************************************************
//
// The following are defines for the CRC register offsets.
//
//*****************************************************************************
#define CRC_CTRL0					(CRC_BASE + 0x00)
#define CRC_CTRL1               	(CRC_BASE + 0x01)
#define CRC_POLY_COEF				(CRC_BASE + 0x04)
#define CRC_INIT					(CRC_BASE + 0x08)
#define CRC_STATUS					(CRC_BASE + 0x0C)
#define CRC_XOR						(CRC_BASE + 0x10)
#define CRC_DATA					(CRC_BASE + 0x20)

//*****************************************************************************
//
// The following are defines for the bit fields in the CRC_CTRL0 register.
//
//*****************************************************************************
#define CRC_CTRL0_SOFT_RESET_Pos		0
#define CRC_CTRL0_SOFT_RESET_Msk		(1UL << CRC_CTRL0_SOFT_RESET_Pos)


//*****************************************************************************
//
// The following are defines for the bit fields in the CRC_CTRL1 register.
//
//*****************************************************************************
#define CRC_CTRL1_POLY_SIZE_Pos		0
#define CRC_CTRL1_POLY_SIZE_Msk		(3UL << CRC_CTRL1_POLY_SIZE_Pos)

#define CRC_CTRL1_REV_IN_Pos		2
#define CRC_CTRL1_REV_IN_Msk		(3UL << CRC_CTRL1_REV_IN_Pos)

#define CRC_CTRL1_REV_O_Pos			4
#define CRC_CTRL1_REV_O_Msk			(1UL << CRC_CTRL1_REV_O_Pos)

#define CRC_CTRL1_XOR_EN_Pos		5
#define CRC_CTRL1_XOR_EN_Msk		(1UL << CRC_CTRL1_XOR_EN_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the CRC_STATUS register.
//
//*****************************************************************************
#define CRC_STATUS_CRC_DONE_Pos		0
#define CRC_STATUS_CRC_DONE_Msk		(1UL << CRC_STATUS_CRC_DONE_Pos)

#define CRC_STATUS_CRC_ERR_Pos		1
#define CRC_STATUS_CRC_ERR_Msk		(1UL << CRC_STATUS_CRC_ERR_Pos)



#endif // __HW_I2C_H__
