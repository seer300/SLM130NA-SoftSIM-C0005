#ifndef __RC32K_CALI_H__
#define __RC32K_CALI_H__

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

#include "xy_memmap.h"

//*****************************************************************************
//
// Values that can be passed to TimerConfigure as the ulConfig parameter.
//
//*****************************************************************************

#define MCNT_RC32K_COUNT 		3200
#define MCNT_RC32K_CALI_COUNT 	3200

typedef struct
{
	uint8_t capsel;
	uint8_t sdm;
}lut_param_t;

typedef struct
{
	uint32_t mcnt_cnt;
	uint32_t pll_divn;
}xtal_freq_t;

#define FLASH_32KCALI_TBL_LEN					(182)	
#define FLASH_32KFREQ_TBL_LEN					(728)
typedef struct
{
	lut_param_t  rc_param_tbl[91];   //RC32K的91组校准参数（capsel,sdm）
	uint16_t padding1;
	uint32_t last_reset_tick;        //RC32K最近一次执行老化流程的时间
	uint32_t low_tempera_offset;     //g_low_tempera_offset
	uint32_t regular_tempera_offset; //g_regular_tempera_offset
	uint32_t high_tempera_offset;    //g_high_tempera_offset
	xtal_freq_t  xtal_freq_tbl[91];  //XTAL32K的91组频率计算参数（每档4字节的pll倍频系数值，4字节的mcnt校准值）
	uint32_t default_freq;
	uint16_t osc_cal;
	uint16_t padding2;
	uint32_t low_offset;      //以下为三个温度段的default校准值和相应的default offset，在初始化阶段获取
	uint32_t low_capsel;
	uint32_t low_sdm;
	uint32_t mid_offset;
	uint32_t mid_capsel;
	uint32_t mid_sdm;
	uint32_t high_offset;
	uint32_t high_capsel;
	uint32_t high_sdm;
	uint8_t  clk_src;         //32K时钟源，1：xtal32k，0：rc32k，CP会读取
	uint8_t padding[3];

	uint32_t hrc_clk;         //hrc26M的实际频率，通常为26000000附近，CP核会读
}Cali_Ftl_t;




#define RC_PRECISION_LIMIT						(550)									// RC32K精度阈值，实时PPM超过此值出发AP MCNT频率测量

extern int g_rc32k_test;
extern uint16_t g_osc_cal;
extern volatile uint32_t g_32k_default_freq;



extern uint32_t PRCM_32KClkSrcGet(void);
extern void PRCM_SelectXtal32k(void);
extern void PRCM_SelectRC32k(void);

extern void RC32K_CALIB_En(void);
extern void RC32K_CALIB_Dis(void);
extern void RC32K_CALIB_TCOUNT_Set(uint8_t tCountVal);

extern void RC_LOOKUP_Tbl_Check(void);
extern uint16_t rc32k_lpts_calibration(int16_t temperaVal, uint8_t lptsResoVal);
extern int8_t rc32k_get_temperature(uint16_t oscCalVal, uint8_t lptsResoVal);
extern int8_t rc32k_get_last_temperature();
extern uint32_t rc32k_calibration(uint8_t capSelInitVal, uint8_t *capSelNV, int8_t *sdmNV,uint32_t *mcnt_cal_count, uint32_t *pll_divn );
extern uint32_t rc32k_get_count_by_mcnt(uint32_t mcnt_rc32k_count, uint32_t *mcnt_cnt, uint32_t *freq_32k, uint32_t *pll_divn );
extern void RC32K_CALIB_Done(void);
extern void RC32K_CALIB_LOOKUP_TABLE_Set(uint8_t tempAddr, uint8_t capSelVal, uint8_t sdmDataVal);
extern void RC32K_CALIB_LOOKUP_TABLE_Get(uint8_t tempAddr, uint8_t *capSelVal, uint8_t *sdmDataVal);
extern uint8_t RC32K_CALIB_CAPSEL_GetCur(void);
extern uint8_t RC32K_CALIB_SDMDATA_GetCur(void);
extern void rc32k_int_handler(void);

extern void rc32k_init(void);
//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
