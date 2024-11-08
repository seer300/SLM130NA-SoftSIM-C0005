#include "xinyi_nvic.h"

/* defined in startup_cm3.s */
extern void Default_Handler(void);


/*!< Application Interrupt Reset Control Register(AIRCR), Access Key */
#define AIRCR_VECTKEY_MASK    ((uint32_t)0x05FA0000)


void NVIC_PriorityGroupConfig(uint32_t NVIC_PriGroup)
{
	/* Check the parameters */
	assert_param(IS_NVIC_PRIORITY_GROUP(NVIC_PriGroup));

	SCB->AIRCR = AIRCR_VECTKEY_MASK | NVIC_PriGroup;
}


void NVIC_SetVectorTable(uint32_t *NVIC_Vectors)
{
	SCB->VTOR = (uint32_t)NVIC_Vectors;
}


void NVIC_SystemLPConfig(uint8_t LowPowerMode, FunctionalState NewState)
{
	/* Check the parameters */
	assert_param(IS_NVIC_LP(LowPowerMode));
	assert_param(IS_FUNCTIONAL_STATE(NewState));

	if (NewState != DISABLE)
	{
		SCB->SCR |= LowPowerMode;
	}
	else
	{
		SCB->SCR &= (uint32_t)(~(uint32_t)LowPowerMode);
	}
}


void NVIC_IntRegister(IRQn_Type IRQn, IRQnHandle_Type IRQnHandle, uint32_t priority)
{
	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));
	assert_param(IS_NORMAL_IRQnHandle(IRQnHandle));

	/* Interrupt register */
	g_pfnVectors[IRQn + 16] = (uint32_t)IRQnHandle;

	/* Set interrupt priority */
	NVIC_SetPriority(IRQn, priority);

	/* Enable interrupt */
	if(IRQn >= 0)
	{
		NVIC_EnableIRQ(IRQn);
	}
}


void NVIC_IntUnregister(IRQn_Type IRQn)
{
	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	/* Interrupt unregister */
	g_pfnVectors[IRQn + 16] = (uint32_t)Default_Handler;

	/* Set interrupt priority */
	NVIC_SetPriority(IRQn, 0);

	if(IRQn >= 0)
	{
		NVIC_DisableIRQ(IRQn);
	}
}


IRQnHandle_Type NVIC_GetIntHandler(IRQn_Type IRQn)
{
	IRQnHandle_Type IRQnHandle;

	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	/* Interrupt handler */
	IRQnHandle = (IRQnHandle_Type)g_pfnVectors[IRQn + 16];

	return IRQnHandle;
}


void NVIC_SetIntPending(IRQn_Type IRQn)
{
	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	NVIC_SetPendingIRQ(IRQn);
}


void NVIC_ClearIntPending(IRQn_Type IRQn)
{
	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	NVIC_ClearPendingIRQ(IRQn);
}


FlagStatus NVIC_GetIntPending(IRQn_Type IRQn)
{
	uint32_t PendingFlag;

	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	PendingFlag = NVIC_GetPendingIRQ(IRQn);

	return (PendingFlag ? SET : RESET);
}


FlagStatus NVIC_GetIntActive(IRQn_Type IRQn)
{
	uint32_t PendingFlag;

	/* Check the parameters */
	assert_param(IS_NVIC_IRQn(IRQn));

	PendingFlag = NVIC_GetActive(IRQn);

	return (PendingFlag ? SET : RESET);
}


void NVIC_VectorReset(void)
{
	SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)    |
				 (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
				 SCB_AIRCR_VECTRESET_Msk);                     /* Keep priority group unchanged */
	__DSB();                                                   /* Ensure completion of memory access */
	while(1);                                                  /* wait until reset */
}
