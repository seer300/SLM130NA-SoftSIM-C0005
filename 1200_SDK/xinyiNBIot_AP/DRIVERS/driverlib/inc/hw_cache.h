#ifndef __HW_CACHE_H__
#define __HW_CACHE_H__

#include "xinyi2100.h"


//*****************************************************************************
//
// The following are defines for the cache register offsets.
//
//*****************************************************************************
#define		CACHE_CCR0				(0x00)
#define		CACHE_CCR1				(0x01)
#define		CACHE_SR0				(0x04)
#define		CACHE_SR1				(0x05)
#define		CACHE_SR2				(0x06)
#define 	CACHE_IRQM				(0x08)
#define		CACHE_IRQS				(0x0C)
#define		CACHE_RD_HITCNT			(0x10)
#define		CACHE_RD_MISSCNT		(0x14)
#define		CACHE_IDX				(0x18)
#define		CACHE_ELR_ADDR			(0x1C)
#define		CACHE_CRE				(0x20)
#define		CACHE_WR_HITCNT			(0x24)
#define		CACHE_WR_MISS			(0x28)
#define		CACHE_PARAMS0			(0x30)
#define		CACHE_PARAMS1			(0x31)
#define		CACHE_CID				(0x34)
#define		CACHE_BADDR				(0x38)
#define		CACHE_TADDR				(0x3C)
#define		CACHE_BADDR1			(0x40)
#define		CACHE_TADDR1			(0x44)
#define		CACHE_BADDR2			(0x48)
#define		CACHE_TADDR2			(0x4C)
#define		CACHE_BADDR3			(0x50)
#define		CACHE_TADDR3			(0x54)
#define		CACHE_BADDR4			(0x58)
#define		CACHE_TADDR4			(0x5C)
#define		CACHE_BADDR5			(0x60)
#define		CACHE_TADDR5			(0x64)
#define		CACHE_BADDR6			(0x68)
#define		CACHE_TADDR6			(0x6C)
#define		CACHE_BADDR7			(0x70)
#define		CACHE_TADDR7			(0x74)
#define		CACHE_ATTR				(0x78)



//*****************************************************************************
//
// The following are defines for the bit fields in the CACHE_CCR0 register.
//
//*****************************************************************************
#define CACHE_CCR0_WB_POLICY_Pos     3
#define CACHE_CCR0_WB_POLICY_Msk     (1UL << CACHE_CCR0_WB_POLICY_Pos)
#define CACHE_CCR0_WRITE_THROUGH     (0UL << CACHE_CCR0_WB_POLICY_Pos)
#define CACHE_CCR0_WRITE_BACK        (1UL << CACHE_CCR0_WB_POLICY_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the CACHE_IRQM register.
//
//*****************************************************************************
#define CACHE_IRQM_ERR_RESP_Pos      0
#define CACHE_IRQM_ERR_RESP_Msk      (1UL << CACHE_IRQM_ERR_RESP_Pos)

#define CACHE_IRQM_ERR_WRTHR_Pos      0
#define CACHE_IRQM_ERR_WRTHR_Msk     (1UL << CACHE_IRQM_ERR_WRTHR_Pos)

#define CACHE_IRQM_ERR_FLUSH_Pos      0
#define CACHE_IRQM_ERR_FLUSH_Msk     (1UL << CACHE_IRQM_ERR_FLUSH_Pos)

//*****************************************************************************
//
// The following are defines for the bit fields in the CACHE_IRQS register.
//
//*****************************************************************************
#define CACHE_IRQS_ERR_RESP_Pos      0
#define CACHE_IRQS_ERR_RESP_Msk      (1UL << CACHE_IRQS_ERR_RESP_Pos)

#define CACHE_IRQS_ERR_WRTHR_Pos      0
#define CACHE_IRQS_ERR_WRTHR_Msk     (1UL << CACHE_IRQS_ERR_WRTHR_Pos)

#define CACHE_IRQS_ERR_FLUSH_Pos      0
#define CACHE_IRQS_ERR_FLUSH_Msk     (1UL << CACHE_IRQS_ERR_FLUSH_Pos)

#endif // __HW_CACHE_H__



