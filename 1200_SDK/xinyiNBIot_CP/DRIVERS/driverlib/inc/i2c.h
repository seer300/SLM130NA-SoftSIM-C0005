#ifndef __I2C_H__
#define __I2C_H__

#include "hw_i2c.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "debug.h"
#include "interrupt.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    I2C_slave = 0,
    I2C_master_normal_noStartByte = 8,
    I2C_master_normal_withStartByte = 9,
    I2C_master_hs_noStartByte = 10,
    I2C_master_hs_withStartByte = 11,
    I2C_master_generalCallAddress = 12
}I2C_workmode;

typedef enum {
    I2C_addr_7b = 0,
    I2C_addr_10b = 1
}I2C_addrbits;

typedef enum {
    I2C_standard_mode = 0,    //100k
    I2C_fast_mode,            //400k
    I2C_fast_mode_pluse,      //1M
    I2C_highspeed_mode        //3.4M
}I2C_clockmode;

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
extern void I2CInit(unsigned long ulBase,
                I2C_workmode workmode,
                I2C_clockmode clockmode,
                I2C_addrbits addrbits,
                unsigned long pclk);

extern void I2CReset(unsigned long ulBase);
extern void I2CDeInit(unsigned long ulBase);
extern void I2CSetFiFoThreshold(unsigned long ulBase, unsigned char TxFiFoThreshold, unsigned char RxFiFoThreshold);
extern void I2CSetAddr(unsigned long ulBase, unsigned short addr);
extern void I2CSetMasterClockStretch(unsigned long ulBase, unsigned char enable);
extern void I2CSetDelayBetweenByte(unsigned long ulBase, unsigned char enable, unsigned char delayBetweenByte);
extern void I2CIntEnable(unsigned long ulBase, unsigned long ulIntFlags);
extern void I2CIntDisable(unsigned long ulBase, unsigned long ulIntFlags);
extern void I2CIntClear(unsigned long ulBase, unsigned long ulIntFlags);
extern unsigned long I2CIntStatus(unsigned long ulBase);
extern void I2CIntRegister(unsigned long ulBase, unsigned long *g_pRAMVectors, void (*pfnHandler)(void));
extern void I2CIntUnregister(unsigned long ulBase, unsigned long *g_pRAMVectors);
extern unsigned short I2CStatus(unsigned long ulBase);
extern unsigned char I2CTxFiFoLevel(unsigned long ulBase);
extern unsigned char I2CRxFiFoLevel(unsigned long ulBase);
extern void I2CPutData(unsigned long ulBase, unsigned short data);
extern unsigned char I2CGetData(unsigned long ulBase);
extern unsigned long I2CMasterWriteData(unsigned long ulBase, unsigned char *pdata, unsigned long len);
extern unsigned long I2CSlaveWriteData(unsigned long ulBase, unsigned char *pdata, unsigned long len);
extern unsigned long I2CMasterReadData(unsigned long ulBase, unsigned char *pdata, unsigned long len);
extern unsigned long I2CSlaveReadData(unsigned long ulBase, unsigned char *pdata, unsigned long len);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __I2C_H__
