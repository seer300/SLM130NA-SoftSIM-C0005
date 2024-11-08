/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "prcm.h"
#include "xy_svd.h"
#include "hal_gpio.h"
#include "xy_at_api.h"
#include "at_cmd_regist.h"

uint8_t g_svd_val = 0;	        //svd阈值设置

__FLASH_FUNC static void SVDInit(uint8_t Svd_val, uint32_t Svd_period)
{
	PRCM_AonWkupSourceEnable(AON_WKUP_SOURCE_SVD);
	SVD_THD_VoltageSet(SVD_THDMIN,(((Svd_val-22)*30)+670));
	SVD_THD_VoltageSet(SVD_THD1,(((Svd_val-22)*30)+670));
	SVD_THD_VoltageSet(SVD_THD2,(((Svd_val-22)*30)+670));
	SVD_THD_VoltageSet(SVD_THD3,(((Svd_val-22)*30)+670));
    SVD_THDMIN_HCPD_Enable();
	SVD_IntStatusClear();
    SVD_IntEnable(SVD_THDMIN);	
	SVD_IntEnable(SVD_THD1);
	SVD_IntEnable(SVD_THD2);
	SVD_IntEnable(SVD_THD3);	

	SVD->CTRL |= 0x800;  //set gpio7
    SVD_Config(SVD_MODE_FILTER_HRC_2p5us,SVD_FILTER_NUM_6,SVD_PRDUNIT_SEL_RC32K5ms,Svd_period,SVD_SAMPLE_NUM_3,SVD_WARM_DLY_80us);
}

__FLASH_FUNC static void SVDDeInit()
{
	PRCM_AonWkupSourceDisable(AON_WKUP_SOURCE_SVD);
    SVD_IntDisable(SVD_THDMIN);		
    SVD_Config(SVD_MODE_DISABLE,SVD_FILTER_NUM_6,SVD_PRDUNIT_SEL_RC32K5ms,SVD_PERIOD_NUM_0,SVD_SAMPLE_NUM_3,SVD_WARM_DLY_80us);
}

__FLASH_FUNC static void GPIOTestInit(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_54;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_PULL_DOWN;
	HAL_GPIO_Init(&gpio_init);

	// Set pad to GPIO7 mode
	gpio_init.Pin  = GPIO_PAD_NUM_7;
	gpio_init.Mode = GPIO_MODE_INOUT;
	gpio_init.Pull = GPIO_FLOAT;
	HAL_GPIO_Init(&gpio_init);
}

__FLASH_FUNC static void GPIOTestDeInit(void)
{
	HAL_GPIO_InitTypeDef gpio_init = {0};

	gpio_init.Pin = GPIO_PAD_NUM_54;
	gpio_init.Mode = GPIO_MODE_INPUT;
	gpio_init.Pull = GPIO_PULL_DOWN;
	HAL_GPIO_Init(&gpio_init);
}

/**
 * @brief 配置SVD特性并打开。
 * 
 *  设置：发送：AT+SVDCFG=x,xx  其中x为1表示打开SVD配置，xx为SVD不同阈值设置，不同阈值为22-35范围，返回：\r\nOK\r\n 表示成功，其余表示失败
 *                             其中x为0表示关闭SVD配置，后面xx参数无效，无须配置，返回：\r\nOK\r\n 表示成功，其余表示失败
 */
/*
__FLASH_FUNC int at_SVDCFG_req(char *param,char **rsp_cmd)
{
	(void)rsp_cmd;
 	if (g_cmd_type == AT_CMD_REQ) //设置类
	{
		int32_t val = -1;
		int32_t svd_val = 0;
		at_parse_param("%d,%d",param,&val,&svd_val);
		if(val == 1)
		{
			if((svd_val < 22) || (svd_val > 35))
			{
				return XY_ERR_PARAM_INVALID;
			}
			else
			{
				g_svd_val = svd_val;
				//调用SVD配置函数
				SVDInit(g_svd_val);
				//调用GPIO配置函数
				GPIOTestInit(); 	
			}
		}
		else if(val == 0)
		{
			SVDDeInit();
			GPIOTestDeInit(); 
		}
		else
		{
			return XY_ERR_PARAM_INVALID;
		}

	}
	else
	{
		return XY_ERR_PARAM_INVALID;
	}
	return XY_OK;
}*/

/**
  * @brief  Set the SVD Init.
  * @param  val_set This parameter can be one of the following values:
  *         @arg @ref SVD_VAL_2P2V
  *         @arg @ref SVD_VAL_2P3V
  *         @arg @ref SVD_VAL_2P4V
  *         @arg @ref SVD_VAL_2P5V
  *         @arg @ref SVD_VAL_2P6V
  *         @arg @ref SVD_VAL_2P7V
  *         @arg @ref SVD_VAL_2P8V
  *         @arg @ref SVD_VAL_2P9V
  *         @arg @ref SVD_VAL_3P0V
  *         @arg @ref SVD_VAL_3P1V
  *         @arg @ref SVD_VAL_3P2V
  *         @arg @ref SVD_VAL_3P3V
  *         @arg @ref SVD_VAL_3P4V
  *         @arg @ref SVD_VAL_3P5V
  * @param  period_sec This parameter can be one of the following values:
  *         @arg @ref SVD_PERIOD_0S
  *         @arg @ref SVD_PERIOD_1S
  *         @arg @ref SVD_PERIOD_5S
  *         @arg @ref SVD_PERIOD_10S
  * 
  * @retval None
  */
__FLASH_FUNC void xy_SVD_Init(uint8_t val_set, uint32_t period_sec)
{
	xy_assert(val_set>=20 && val_set<=40);
	xy_assert(period_sec==0||period_sec==1||period_sec==5||period_sec==10);

	if(period_sec == 0)
		period_sec = ((uint32_t)0<<SVD_PRD_CFG_Pos);
	else if (period_sec == 1)
		period_sec = ((uint32_t)32<<SVD_PRD_CFG_Pos);
	else if (period_sec == 5)
		period_sec = ((uint32_t)128<<SVD_PRD_CFG_Pos);
	else if(period_sec == 10)
		period_sec = ((uint32_t)255<<SVD_PRD_CFG_Pos);
	else
		xy_assert(0);

	GPIOTestInit();
	SVDInit(val_set, period_sec);
}

__FLASH_FUNC void xy_SVD_DeInit(void)
{
	SVDDeInit();
	GPIOTestDeInit();
}

/**
 * @brief SVD的唤醒中断回调函数注册接口
 */
pFunType_void p_Svd_WakeupCallback = NULL;
__FLASH_FUNC void Svd_Wakeup_Cb_Reg(pFunType_void pfun)
{
	p_Svd_WakeupCallback = pfun;
	mark_dyn_addr(&p_Svd_WakeupCallback);
}