#pragma once
#include "xy_timer.h"

#define MCNT_RC32K_CALI_COUNT 	3200


#define TIMER_LP_RCCALI 						TIMER_LP_USER1

#define CRITICAL_LOW_TEMPERATURE	20			// 低温-常温界限
#define CRITICAL_HIGH_TEMPERATURE	50			// 常温-高温界限

#define RC32K_PARM_AGING_TIME	    3600000	    // RC校准参数的老化周期，单位ms: 默认3600000（1小时）
#define RC32K_CALI_PROCESS_TIMEOUT	    5000	// RC单次校准的总耗时，超出该时长认为RC校准出现异常，结束校准流程，单位ms: 默认5s

#define RC32K_TEMPER_INDEX(temperaVal)			((((temperaVal) + 50)/2))

#define RC32K_MEASURE_SUCCESS			            0     //check_mcnt_result返回值
#define RC32K_MEASURE_FAILED				        1

//rc32k_calibration校准成功的返回值必然是一个确切的RC32K频率值，校准失败返回值为以下失败原因
#define CALI_FAIL_CHECK_FAIL           0    //MCNT check失败：包括时钟源切换、pll动态调频
#define CALI_FAIL_CONVERGENCE_FAIL     1    //多次计算频率仍无法收敛，数字问题
#define CALI_SUSPEND                   2


#define BLOCK_FLAG        0
#define NONBLOCK_FLAG     1


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
#define XTAL_FREQ_TBL_OFFSET                    ((uint32_t)&(((Cali_Ftl_t *)0)->xtal_freq_tbl))
typedef struct
{
	lut_param_t rc_param_tbl[91];    //RC32K的91组校准参数（capsel,sdm）
	uint16_t padding1;
	uint32_t last_reset_tick;        //RC32K最近一次执行老化流程的时间
	uint32_t low_tempera_offset;     //g_low_tempera_offset
	uint32_t regular_tempera_offset; //g_regular_tempera_offset
	uint32_t high_tempera_offset;    //g_high_tempera_offset
	xtal_freq_t xtal_freq_tbl[91];   //XTAL32K的91组频率计算参数（每档4字节的pll倍频系数值，4字节的mcnt校准值）
	uint32_t default_freq;
	uint16_t osc_cal;
	uint16_t padding2;
	uint32_t low_offset;        //以下为三个温度段的default校准值和相应的default offset，在初始化阶段获取
	uint32_t low_capsel;
	uint32_t low_sdm;
	uint32_t mid_offset;
	uint32_t mid_capsel;
	uint32_t mid_sdm;
	uint32_t high_offset;
	uint32_t high_capsel;
	uint32_t high_sdm;
	uint8_t  clk_src;         //32K时钟源，1：xtal32k，0：rc32k，CP会读取
	uint8_t  padding[3];

	uint32_t hrc_clk;         //hrc26M的实际频率，通常为26000000附近，CP核会读
}Cali_Ftl_t;


typedef struct {
	uint32_t measure_length;
	uint8_t clock_source;
	uint8_t  K_1P92;
	uint32_t pll_divn;
	uint32_t mcnt_result;	
	uint32_t freq;	
} mcnt_t;

typedef struct {
	int8_t  tempera;
	uint8_t capsel;
	uint8_t sdm;
} rc_param_t;

typedef struct
{
	struct List_t *next;
	uint8_t index;
	uint8_t sdmNV;
	uint8_t capSelNV;
	uint8_t padding;
}CaliTbl_Node_t;


extern void RC32k_User_Hook(void);

extern uint32_t rc32k_calibration(uint8_t capSelInitVal, uint8_t *capSelNV, uint8_t *sdmNV, bool block_or_not );

extern void rc32k_int_handler(void);

extern void select_32k_clk_src(void);

extern void RC32K_Cali_Failed_hook(uint8_t tempera, uint8_t done_flag);

extern void rc32k_get_default_value(uint8_t *capSelNV, uint8_t *sdmNV);

extern int32_t get_golden_ref_offset(uint8_t capSelNV, uint8_t sdmNV, uint8_t temper);

extern void Update_Aon_LUT_By_Offset(uint8_t lower_temperature, uint8_t upper_lower_temperature, int32_t offset );

extern void RC32k_Cali_Init( uint16_t osc_cal);

extern uint32_t update_32k_freq(void);

extern void Save_32kCaliTbl_To_Flash(void);

extern void Fill_Aon_LUT_With_Golden_Ref();

extern void Fill_Aon_LUT_With_Linear_Interpolation();

/*该函数运行在main主函数，执行校准算法动作，并保存数据.该函数耗时达到秒级*/
/*返回值：1表示当前RC模块正在校准，不允许进入睡眠；0表示当前RC模块空闲*/
extern int RC32k_Cali_Process(void);

void RC32K_cleanup_event_set(uint32_t LUT_clean_period);