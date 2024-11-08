#include "mpu.h"

#if (__MPU_PRESENT == 1U)


uint32_t MPU_GetNumberOfRegion(uint32_t RegionType)
{
	/* Check the parameters */
	assert_param(IS_MPU_REGION_TYPE(RegionType));

	if (RegionType == MPU_REGION_TYPE_IREGION)
	{
	return (MPU->TYPE & MPU_TYPE_IREGION_Msk) >> MPU_TYPE_IREGION_Pos;
	}
	else if (RegionType == MPU_REGION_TYPE_DREGION)
	{
	return (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	}
	else if (RegionType == MPU_REGION_TYPE_SEPARATE)
	{
	return (MPU->TYPE & MPU_TYPE_SEPARATE_Msk) >> MPU_TYPE_SEPARATE_Pos;
	}

	return 0;
}


void MPU_Control(uint32_t Contrl, FunctionalState NewState)
{
	/* Check the parameters */
	assert_param(IS_FUNCTIONAL_STATE(NewState));

	if (NewState != DISABLE)
	{
	MPU->CTRL |= Contrl;
	}
	else
	{
	MPU->CTRL &= ~Contrl;
	}
}


void MPU_Cmd(FunctionalState NewState)
{
	/* Check the parameters */
	assert_param(IS_FUNCTIONAL_STATE(NewState));

	if (NewState != DISABLE)
	{
	MPU->CTRL |= MPU_CTRL_ENABLE_Msk;
	}
	else
	{
	MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;
	}
}


void MPU_RegionInit(uint32_t RegionNum, uint32_t RegionAddr, uint32_t RegionLen, uint32_t Config)
{
	/* Check the parameters */
	assert_param(IS_MPU_REGION_NUMBER(RegionNum));
	assert_param(IS_MPU_REGION_LENGTH(RegionLen));

	/* Set region number and address, and then manipulate it */
	MPU->RBAR = (RegionAddr & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | RegionNum;

	/* Set current region attributes */
	MPU->RASR = Config | (RegionLen << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;
}


void MPU_SubregionCmd(uint32_t RegionNum, uint32_t SubRegionNum, FunctionalState NewState)
{
	/* Check the parameters */
	assert_param(IS_FUNCTIONAL_STATE(NewState));

	/* Set region number */
	MPU->RNR = RegionNum;

	/* Disable or enable subregion */
	if (NewState != DISABLE)
	{
	MPU->RASR &= ~SubRegionNum;
	}
	else
	{
	MPU->RASR |= SubRegionNum;
	}
}


#endif  /* __MPU_PRESENT == 1U */
