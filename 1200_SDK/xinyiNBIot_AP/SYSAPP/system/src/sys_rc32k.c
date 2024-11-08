/**
 * @file sys_rc32k.c
 * @brief
 * @version 1.0
 * @date 2021-07-10
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 *
 */
#include "stdlib.h"
#include "uart.h"
#include "gpio.h"
#include "prcm.h"
#include "common.h"
#include "rc32k.h"
#include "mcnt.h"
#include "sys_clk.h"
#include "sys_rc32k.h"
#include "nvic.h"
#include "xy_list.h"
#include "xy_system.h"
#include "adc.h"
#include "xy_cp.h"
#include "fast_recovery.h"
#include "xy_event.h"
#include "xy_flash.h"
#include "adc_adapt.h"
#include "xy_utils.h"
#include "xy_timer.h"
#include "xy_ftl.h"

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
/**
 * @brief	Users don't need to care
 */

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
#define VAILD_TEMP_LIMIT	4	// 线性插值拟合的温度差上限
#define EXTEND_TEMP_LIMIT	4	// 线性插值拟合的合理延伸范围
#define K_MAGIC				4096	// 斜率的初始无效值
#define READ_RC_PARAM(type,addr)     (*(type *)(addr))

/*******************************************************************************
 *                       Global variable declarations                          *
 ******************************************************************************/
extern void MCNT_Handler(void);

uint16_t g_osc_cal = 0;
short g_osc_ref = 2000;
volatile uint8_t g_tempera_t0 = 0;
volatile uint32_t g_rc32k_cali_flag = 0;
volatile uint32_t g_rc32k_aging_flag = 0;
int32_t g_low_tempera_offset;
int32_t g_regular_tempera_offset;
int32_t g_high_tempera_offset;
volatile uint32_t g_peri1_div_bak;
volatile uint32_t g_peri2_div_bak;
volatile uint32_t g_LUT_clean_period_min = 0;
volatile uint8_t g_LUT_cleanup_flag = 0;
volatile uint8_t g_XTAL26M_power_state = 0;

volatile uint32_t g_cali_start_tick = 0;

volatile uint32_t g_debug_error1;
volatile uint32_t g_debug_error2;
volatile uint32_t g_debug_error3;
volatile uint32_t g_debug_error4;

volatile uint32_t g_last_reset_tick = 0;  //记录上一次重置Aon LUT的时刻点，以应对模组的RC老化问题

volatile uint8_t g_clk_src_last = 0XFF;  //记录上一次上电时使用的32K时钟源，若当前上电发生变化需要清空flash tbl

// golden ref参照表 {capsel,sdm}
uint8_t g_golden_ref[91][2] = 
{	{1,38},{1,41},{1,45},{1,48},{1,51},{1,55},{1,58},{1,62},{2,1},{2,4},
	{2,8},{2,11},{2,15},{2,18},{2,21},{2,25},{2,28},{2,32},{2,35},{2,38},
	{2,42},{2,44},{2,47},{2,50},{2,52},{2,55},{2,57},{2,59},{2,60},{2,62},
	{2,63},{3,0},{3,1},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,1},
	{3,1},{2,64},{2,62},{2,61},{2,59},{2,58},{2,56},{2,53},{2,51},{2,48},
	{2,45},{2,36},{2,33},{2,29},{2,26},{2,23},{2,19},{2,16},{2,12},{2,9},
	{2,6},{2,2},{1,63},{1,59},{1,56},{1,53},{1,49},{1,46},{1,42},{1,39},
	{1,36},{1,32},{1,29},{1,25},{1,22},{1,19},{1,15},{1,12},{1,8},{1,5},
	{1,2},{0,62},{0,59},{0,55},{0,52},{0,49},{0,45},{0,42},{0,38},{0,35},{0,32}
};
#if (MODULE_VER == 0x0)
uint8_t g_cali_tbl[91][3] = {0};
#endif
/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
#if(MODULE_VER)
/* AP睡眠前写RC32K相关参数的链表*/
ListHeader_t g_caliTbl_list = {0};

//新增Cali节点，插入链表
__FLASH_FUNC void Insert_CaliTbl_Node(uint8_t index, uint8_t sdmNV, uint8_t capSelNV)
{
	CaliTbl_Node_t *pxlist;
	pxlist = xy_malloc(sizeof(CaliTbl_Node_t));

	pxlist->next = NULL;
	pxlist->index = index;
	pxlist->sdmNV = sdmNV;
	pxlist->capSelNV = capSelNV;

	ListInsert((List_t *)pxlist, &g_caliTbl_list);
}

// 获取下一个Cali节点
List_t * Get_Next_CaliTbl_Node()
{
	return g_caliTbl_list.head;
}

//删除相应地址对应的结点
void Delete_CaliTbl_Node(uint8_t index)
{
	List_t *current_node = g_caliTbl_list.head;
	List_t *pre_node = g_caliTbl_list.head;

	DisablePrimask();
	// while(dezerocpy_debug);
	while(current_node != NULL)
	{
		if(((CaliTbl_Node_t *)current_node)->index == index)
		{
			if(current_node == g_caliTbl_list.head)
			{
				g_caliTbl_list.head = (List_t *)current_node->next;
				if(current_node->next == NULL)
					g_caliTbl_list.tail = NULL;
			}
			else
			{
				pre_node->next = current_node->next;
				if(current_node->next == NULL)
					g_caliTbl_list.tail = pre_node;
			}
			g_caliTbl_list.node_count -= 1;
			xy_free(current_node);
			EnablePrimask();
			return;
		}
		else
		{
			pre_node = current_node;
			current_node = (List_t *)current_node->next;
		}
	}
	EnablePrimask();
	xy_assert(0);
}

void Save_32kCaliTbl_To_Flash(void)
{  
	CaliTbl_Node_t *CaliNode;	
	Cali_Ftl_t *g_32K_table;
	
	if(Get_Next_CaliTbl_Node() != NULL)  //链表不为空
	{
		g_32K_table = xy_malloc(sizeof(Cali_Ftl_t));
		xy_assert(xy_ftl_read(CALIB_FREQ_BASE, (void *)g_32K_table, sizeof(Cali_Ftl_t)) == 1);
		
		while( (CaliNode = (CaliTbl_Node_t *)Get_Next_CaliTbl_Node()) != NULL )
		{
			g_32K_table->rc_param_tbl[CaliNode->index].capsel = CaliNode->capSelNV;
			g_32K_table->rc_param_tbl[CaliNode->index].sdm = CaliNode->sdmNV;

			Delete_CaliTbl_Node(CaliNode->index);
		}

		g_32K_table->last_reset_tick = g_last_reset_tick;
		g_32K_table->low_tempera_offset = g_low_tempera_offset;
		g_32K_table->regular_tempera_offset = g_regular_tempera_offset;
		g_32K_table->high_tempera_offset = g_high_tempera_offset;

		xy_assert(xy_ftl_write(CALIB_FREQ_BASE, (void *)g_32K_table, sizeof(Cali_Ftl_t)) == 1);
		xy_free(g_32K_table);
	}
}

__FLASH_FUNC void reset_32KCali_flash_tbl()
{
	CaliTbl_Node_t *CaliNode;	
	char *table_base = xy_malloc(FLASH_32KCALI_TBL_LEN);

	memset(table_base, 0xFF, FLASH_32KCALI_TBL_LEN);
	
	xy_ftl_write(CALIB_FREQ_BASE, table_base, FLASH_32KCALI_TBL_LEN);

	xy_free(table_base);

	if(Get_Next_CaliTbl_Node() != NULL)  //链表不为空
	{
		while( (CaliNode = (CaliTbl_Node_t *)Get_Next_CaliTbl_Node()) != NULL )
		{
			Delete_CaliTbl_Node(CaliNode->index);
		}
	}
}

__FLASH_FUNC void Restore_Lookup_Tbl_from_flash(void)
{
	uint8_t sdmNV;
	uint8_t capSelNV;
	uint8_t tempAddr;
	int32_t offset;
	uint32_t tmp;

	// 获取历史温度值以及对应的golden ref RC参数
	tempAddr = rc32k_get_last_temperature();
	if(tempAddr <= CRITICAL_LOW_TEMPERATURE ){
		offset = g_low_tempera_offset;
	}
	else if(tempAddr <= CRITICAL_HIGH_TEMPERATURE ){
		offset = g_regular_tempera_offset;
	}
	else{
		offset = g_high_tempera_offset;
	}

	tmp = g_golden_ref[tempAddr][0] * 64 + (g_golden_ref[tempAddr][1] & 0x7F) + offset;
	capSelNV = tmp / 64;
	sdmNV = tmp % 64;

	RC32K_CALIB_Dis();

	char *table_base = xy_malloc(FLASH_32KCALI_TBL_LEN);
	xy_assert(xy_ftl_read(CALIB_FREQ_BASE, table_base, FLASH_32KCALI_TBL_LEN) == 1);

	for (int i = 0; i <= 90; i++)
	{
		if(( *(uint8_t *)(table_base + 2 * i) == 0xFF ) && ( *(uint8_t *)(table_base + 2 * i + 1) == 0xFF ) )
		{
			RC32K_CALIB_LOOKUP_TABLE_Set(i, 0, 0);
		}
		else
		{
			// sdm高位为有效位(置bit7为1)
			RC32K_CALIB_LOOKUP_TABLE_Set(i, *(uint8_t *)(table_base + 2 * i), (*(uint8_t *)(table_base + 2 * i + 1)) | 0x80 );
		}
	}

	xy_free(table_base);

	// 强制使用缺省RC参数值，确保初始化阶段的RC稳定性
	RC32K_CALIB_NV_Save(capSelNV , sdmNV);

	// 使能后台周期性查表机制
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,  1,  0, 0x0);

	RC32K_CALIB_En();
}
#endif

// 查温补表，返回实时频率
__FLASH_FUNC uint32_t  get_freq32k_by_tempera(int8_t index)
{
	uint32_t uclock_32k = 0;
	uint64_t freq_pll;
	xtal_freq_t freq_param = {0xFFFFFFFF, 0xFFFFFFFF};  //赋初始值为无效值，若ftl_read失败，就使用default值

	// 查温补表，找到对应的频率
	xy_ftl_read(CALIB_FREQ_BASE + XTAL_FREQ_TBL_OFFSET + 8*index, &freq_param, 8);

	if(freq_param.mcnt_cnt == 0xFFFFFFFF)
	{
		// 温补表缺项，使用默认的频率值
		READ_CALI_PARAM(default_freq, &uclock_32k);
		xy_assert(uclock_32k != 0);
	}
	else
	{
		// 根据倍频系数计算出pll频率
		freq_pll = ( ((uint64_t)freq_param.pll_divn >> 24) * 13000000 + ((uint64_t)freq_param.pll_divn & 0xFFFFFF) * 13000000 / 0xffffff );
		// 根据pll频率和mcnt测量值，计算出32k频率
		uclock_32k = freq_pll * MCNT_RC32K_CALI_COUNT / (uint64_t)freq_param.mcnt_cnt;

		if(uclock_32k < 31768 || uclock_32k > 33768)
		{
			while(1);  //频率高\低于32768超过1000Hz，则系统主动卡死！
		}
	}

	return uclock_32k;
}

extern uint8_t g_ADCVref;
extern volatile uint8_t g_CalibrationFlag;
/*****************************************************************************************************
* @brief  仅限芯翼平台内部使用！供底层快速读取当前芯片温度，不走标准HAL接口      
*****************************************************************************************************/
__FLASH_FUNC int16_t get_adc_temp_quickly()
{
	int16_t temp = 0;

	PRCM_ClockEnable(CORE_CKG_CTL_ADC_EN);

	// 当时钟源使用HRC时，使用ADC时开启adc相关电源
	if(PRCM_SysclkSrcGet() == SYSCLK_SRC_HRC)
	{
		COREPRCM->ANATRXBG_CTL = 0x01;
	}
	ADC_bbldoEnable(); //开ADC时钟
	delay_func_us(70);

	ADC_Select_Vref(g_ADCVref);
	
	ADC_TsensorPowerEN(); 
	temp = get_adc_value_quickly(ADC_TSENSOR, GetlsioFreq() / 1000, ADC_CLK_480K, g_CalibrationFlag, 0,g_ADCVref);
	ADC_TsensorPowerDIS();

	if(PRCM_SysclkSrcGet() == SYSCLK_SRC_HRC)
	{
		COREPRCM->ANATRXBG_CTL = 0x00;
	}	

	return temp;
}


// 读取温度并更新真实的32k频率
__FLASH_FUNC uint32_t update_32k_freq(void)
{
	int8_t index;
	int16_t tempera;

	if( HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
	{
		g_32k_clock = 32000;
	}
	else
	{
		tempera = get_adc_temp_quickly();
		index = RC32K_TEMPER_INDEX(tempera);	

		// 查温补表，返回实时频率值
		g_32k_clock = get_freq32k_by_tempera(index);
	}
	return g_32k_clock;
}

// 32k自识别功能
__FLASH_FUNC void select_32k_clk_src(void)
{
	uint8_t mode = READ_FAC_NV(uint8_t, _32K_CLK_MODE);
	READ_CALI_PARAM(clk_src, &g_clk_src_last);

	if(mode == 0)   //32k时钟源自适应，优先选择外部32k
	{
		/*外部可能有假信号造成误识别，不能使用*/
		xy_assert(0);
	}
	else if(mode == 1)   //强制使用xtal32k
	{
		Prcm_PowerUpXtal32k();
		while(!Prcm_GetXtal32kRdy());    //pu xtal32k并阻塞式等待ready信号,若xtal32k没有焊接，系统卡死！
		PRCM_SlectXtal32k();
		HWREGB(BAK_MEM_32K_CLK_SRC) = XTAL_32K;
	}
	else if(mode == 2)   //强制使用RC32k
	{
		PRCM_SlectRc32k(); 
		HWREGB(BAK_MEM_32K_CLK_SRC) = RC_32K;	
	}
	else
	{
		while(1);  //不支持其他模式，AT、log未初始化，只能令系统卡死
	}

	uint8_t clk_src_32k = HWREGB(BAK_MEM_32K_CLK_SRC);
	WRITE_CALI_PARAM(clk_src, &clk_src_32k);
}

// lpts模块校准
extern void RC32K_CALIB_DisNow(void);
extern void RC32K_CALIB_EnNow(void);
__FLASH_FUNC uint16_t rc32k_lpts_calibration(int16_t temperaVal, uint8_t lptsResoVal)
{
	uint8_t cycle_times = 0;
	int16_t oscCalVal = 0;
	uint16_t oscPStatVal = 2000;
	
	g_osc_ref = 2000;

retry:
	cycle_times++;
	RC32K_CALIB_Done();
	RC32K_CALIB_Dis();
	RC32K_CALIB_OSC_REF_Set(g_osc_ref);
	RC32K_CALIB_OSC_CAL_Set(oscCalVal);	
	RC32K_CALIB_LPTS_RESO_Set(lptsResoVal);
	RC32K_CALIB_TEMP_VALID_Clr();

	RC32K_CALIB_EnNow();
	RC32K_CALIB_En();

	utc_cnt_delay(2);
	RC32K_CALIB_DisNow();

	while(!RC32K_CALIB_TEMP_is_VALID());

	oscPStatVal = RC32K_CALIB_OSC_PTAT_Get();
	RC32K_CALIB_TEMP_VALID_Clr();
	if(oscPStatVal == 0)
	{
		if(cycle_times < 3)
		{
			goto retry;
		}
		else
		{
#if XY_DUMP
			xy_assert(0);
#else
			return 2200;
#endif
		}
	}

	oscCalVal = (int16_t)((temperaVal - 24)*lptsResoVal + g_osc_ref - oscPStatVal);
	if (oscCalVal < 0)
	{
		oscCalVal = 2048 + abs(oscCalVal);
	}

	return oscCalVal;
}

// 使能rc32k硬件后台机制
__FLASH_FUNC void RC32k_Cali_Init( uint16_t osc_cal)
{
	RC32K_CALIB_WakeUp_En();
	
	RC32K_CALIB_SDM_En();

	RC32K_CALIB_OSC_CAL_Set(osc_cal);	
	RC32K_CALIB_OSC_REF_Set(2000);
	RC32K_CALIB_LPTS_RESO_Set(5);

	RC32K_CALIB_TCOUNT_Set(1); // 每200ms使用lpts读取一次温度
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,  1,  0, 0x0);
	RC32K_CALIB_En();
}

// 获取golden_ref中rc参数
__FLASH_FUNC void get_golden_capsel(rc_param_t * golden_param)
{
	int32_t tmp;

#if (MODULE_VER != 0x0)
{
	int32_t offset;
	// 获取当前温度档对应的offset
	if(golden_param->tempera <= CRITICAL_LOW_TEMPERATURE)
	{
		offset = g_low_tempera_offset;
	}
	else if(golden_param->tempera <= CRITICAL_HIGH_TEMPERATURE)
	{
		offset = g_regular_tempera_offset;
	}
	else
	{
		offset = g_high_tempera_offset;
	}

	tmp = g_golden_ref[golden_param->tempera][0] * 64 + (g_golden_ref[golden_param->tempera][1] & 0x7F) + offset;
}
#else
{
	tmp = g_golden_ref[golden_param->tempera][0] * 64 + (g_golden_ref[golden_param->tempera][1] & 0x7F);
}
#endif

	golden_param->sdm = (tmp % 64) | 0x80;
	golden_param->capsel = tmp/64;

	return ;
}

// 根据校准后的cap sdm参数获取golden ref的offset偏移
__FLASH_FUNC int32_t get_golden_ref_offset(uint8_t capSelNV, uint8_t sdmNV, uint8_t temper)
{
	uint32_t cur;
	uint32_t golden_ref;

	//	先乘以64避免计算小数引发的精度问题
	cur = capSelNV * 64 + (sdmNV & 0x7f);
	golden_ref = g_golden_ref[temper][0] * 64 + (g_golden_ref[temper][1] & 0x7F);

	return ( cur - golden_ref );
}

#if (MODULE_VER == 0x0)

// 根据校准后的cap sdm参数获取Cali_Tbl的offset偏移
__FLASH_FUNC int32_t get_Cali_Tbl_Offset(uint8_t capSelNV, uint8_t sdmNV, uint8_t temper)
{
	uint32_t cur;
	uint32_t cali_ref;

	if( g_cali_tbl[temper][2] == 0  )
	{
		return 0;
	}
	else
	{
		//	先乘以64避免计算小数引发的精度问题
		cur = capSelNV * 64 + (sdmNV & 0x7f);
		cali_ref = g_cali_tbl[temper][0] * 64 + (g_cali_tbl[temper][1] & 0x7F);
	}
	return ( cur - cali_ref );
}

// 根据分段offset偏移更新Aon LUT和golden ref
__FLASH_FUNC void Update_Aon_LUT_By_Offset(uint8_t lower_temperature, uint8_t upper_lower_temperature, int32_t offset )
{
	uint32_t tmp;

	for (int i = lower_temperature; i <= upper_lower_temperature; i++)
	{
		tmp = g_golden_ref[i][0] * 64 + (g_golden_ref[i][1] & 0x7F) + offset;

		g_golden_ref[i][0] = tmp / 64;
		g_golden_ref[i][1] = tmp % 64;

		RC32K_CALIB_LOOKUP_TABLE_Set(i, g_golden_ref[i][0], g_golden_ref[i][1] | 0x80 );
	}
	return ;
}

// 依据校准值和golden ref结合分段offset偏移更新Aon LUT
__FLASH_FUNC void Update_Aon_LUT_By_Offset_Open(uint8_t lower_temperature, uint8_t upper_lower_temperature, int32_t aging_offset )
{
	uint32_t tmp;

	for (int i = lower_temperature; i <= upper_lower_temperature; i++)
	{
		if(g_cali_tbl[i][2] == 0)
		{
			tmp = g_golden_ref[i][0] * 64 + (g_golden_ref[i][1] & 0x7F) + aging_offset;

			RC32K_CALIB_LOOKUP_TABLE_Set(i, tmp / 64, (tmp % 64) | 0x80 );

			#if XY_DEBUG
				if(i == 37 || i ==38 || i ==39)
				{
					xy_printf("RC_Cali golden_set_temp:%d old_cap:%d old_sdm:%d new_cap:%d new_sdm:%d offset:%d\r\n",i, g_golden_ref[i][0], g_golden_ref[i][1] & 0x7f, tmp / 64 , (tmp % 64) & 0x7f , aging_offset );
				}
			#endif
		}
		else
		{
			tmp = g_cali_tbl[i][0] * 64 + (g_cali_tbl[i][1] & 0x7F) + aging_offset;

			RC32K_CALIB_LOOKUP_TABLE_Set(i, tmp / 64, (tmp % 64) | 0x80 );

			#if XY_DEBUG
				if(i == 37 || i ==38 || i ==39)
				{
					xy_printf("RC_Cali cali_set_temp:%d old_cap:%d old_sdm:%d new_cap:%d new_sdm:%d offset:%d\r\n",i, g_cali_tbl[i][0], g_cali_tbl[i][1] & 0x7f,  tmp / 64 , (tmp % 64) & 0x7f ,  aging_offset );
				}
			#endif

		}
	}
	return ;
}

#else

//模组非快速恢复前利用线性插针拟合填充AON LUT
void Fill_Aon_LUT_With_Linear_Interpolation()
{
	char *rc32k_TBL_base;

	int8_t left = 0;
	float k_left = K_MAGIC;	// 初始值为无效值
	uint8_t capsel_lut_left;
	uint8_t sdm_lut_left;

	int8_t right = 0;
	float k_right = K_MAGIC;	// 初始值为无效值
	uint8_t capsel_lut_right;
	uint8_t sdm_lut_right;

	float k_extend = K_MAGIC;	// 初始值为无效值
	uint8_t capsel_lut_extend;
	uint8_t sdm_lut_extend;
	int8_t delta_temp;

	int8_t temp_start;
	int8_t border_temp;
	uint32_t sum_tmp;

	if(HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
	{
		rc32k_TBL_base = xy_malloc(FLASH_32KCALI_TBL_LEN);
		memset(rc32k_TBL_base, 0, FLASH_32KCALI_TBL_LEN);

		// 填写AON LUT时暂停后台状态机切换
		RC32K_CALIB_Dis();

		for (left = 0; left <= 89; left++)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(left, &capsel_lut_left, &sdm_lut_left);
			if(capsel_lut_left == 0){
				continue;
			}

			for (right = left + 1; right <= 90; right++)
			{
				RC32K_CALIB_LOOKUP_TABLE_Get(right, &capsel_lut_right, &sdm_lut_right);
				if(capsel_lut_right == 0){
					continue;
				}

				if( right <= left + VAILD_TEMP_LIMIT)	// 使用线性插值，填充两温度点间的LUT
				{
					k_right = (capsel_lut_right * 64 + sdm_lut_right  - capsel_lut_left * 64 - sdm_lut_left ) / (right - left);
					for( temp_start = left; temp_start <= right - 1; temp_start++)
					{
						sum_tmp = capsel_lut_left * 64 + (sdm_lut_left & 0x7F) + (temp_start - left) * k_right;
						*(uint8_t *)(rc32k_TBL_base + temp_start*2) = sum_tmp / 64;
						*(uint8_t *)(rc32k_TBL_base + temp_start*2 + 1) = (sum_tmp % 64) | 0x80;
					}
				}
				else									// 两温度点间温差过大，无法直接线性插值：则使用低温段线性延伸，用以拟合高于当前温度6℃的LUT
				{
					k_right = K_MAGIC;
					if(k_left != K_MAGIC)
					{
						border_temp = (left - EXTEND_TEMP_LIMIT) >= 0 ? (left - EXTEND_TEMP_LIMIT) : 0;
						for( temp_start = border_temp; temp_start <= left; temp_start++)
						{
							RC32K_CALIB_LOOKUP_TABLE_Get(temp_start, &capsel_lut_extend, &sdm_lut_extend);
							if(capsel_lut_extend != 0){
								break;
							}
						}
						if(temp_start == left  )
						{
							delta_temp = 1;
							k_extend = 1;
						}
						else
						{
							delta_temp = left - temp_start - 1;
							k_extend = (capsel_lut_extend * 64 + sdm_lut_extend  - capsel_lut_left * 64 - sdm_lut_left ) / (temp_start - left);
						}
	
						for( temp_start = left + 1; temp_start <= left + delta_temp && temp_start <= 90; temp_start++)
						{
							sum_tmp = capsel_lut_left * 64 + (sdm_lut_left & 0x7F) + (temp_start - left) * k_extend;
							*(rc32k_TBL_base + temp_start*2) = sum_tmp / 64;
							*(rc32k_TBL_base + temp_start*2 + 1) = (sum_tmp % 64) | 0x80;
						}	
					}
				}
				break;
			}

			if(right == 91)								 // left温度是最高已校准温度：则使用低温段线性延伸，用以拟合高于left 6℃的LUT
			{
				k_right = K_MAGIC;
				if(k_left != K_MAGIC)
				{
					border_temp = (left - EXTEND_TEMP_LIMIT) >= 0 ? (left - EXTEND_TEMP_LIMIT) : 0;
					for( temp_start = border_temp; temp_start <= left; temp_start++)
					{
						RC32K_CALIB_LOOKUP_TABLE_Get(temp_start, &capsel_lut_extend, &sdm_lut_extend);
						if(capsel_lut_extend != 0){
							break;
						}
					}
					if(temp_start == left)
					{
						delta_temp = 1;
						k_extend = 1;
					}
					else
					{
						delta_temp = left - temp_start - 1;
						k_extend = (capsel_lut_extend * 64 + sdm_lut_extend  - capsel_lut_left * 64 - sdm_lut_left ) / (temp_start - left);
					}

					for( temp_start = left + 1; temp_start <= left + delta_temp && temp_start <= 90; temp_start++)
					{
						sum_tmp = capsel_lut_left * 64 + (sdm_lut_left & 0x7F) + (temp_start - left) * k_extend;
						*(rc32k_TBL_base + temp_start*2) = sum_tmp / 64;
						*(rc32k_TBL_base + temp_start*2 + 1) = (sum_tmp % 64) | 0x80;
					}	
				}
			}

			if( k_left == K_MAGIC)								// 低于left的点未拟合: 则使用高温段线性延伸，用以拟合低于left 2-6℃的LUT
			{
				if((k_right != K_MAGIC) && ((right - left) <= EXTEND_TEMP_LIMIT))
				{
					border_temp = (left + EXTEND_TEMP_LIMIT) >= 90 ? 90 : (left + EXTEND_TEMP_LIMIT);
					for( temp_start = border_temp; temp_start <= 90 && temp_start >= left; temp_start--)
					{
						RC32K_CALIB_LOOKUP_TABLE_Get(temp_start, &capsel_lut_extend, &sdm_lut_extend);
						if(capsel_lut_extend != 0){
							break;;
						}
					}	

					if(temp_start == left)
					{
						delta_temp = 1;
						k_extend = 1;
					}
					else
					{
						delta_temp = temp_start - left - 1;
						k_extend = (capsel_lut_extend * 64 + sdm_lut_extend  - capsel_lut_left * 64 - sdm_lut_left ) / (temp_start - left);
					}
					
					for( temp_start = left - 1; ((left - temp_start) <= delta_temp) && (temp_start >= 0); temp_start--)
					{
						sum_tmp = capsel_lut_left * 64 + (sdm_lut_left & 0x7F) + (temp_start - left) * k_extend;
						*(uint8_t *)(rc32k_TBL_base + temp_start*2) = sum_tmp / 64;
						*(uint8_t *)(rc32k_TBL_base + temp_start*2 + 1) = (sum_tmp % 64) | 0x80;
					}
				}
			}

			if((right - left) <= EXTEND_TEMP_LIMIT)
				k_left = k_right;
			else
				k_left = K_MAGIC;
			left = right - 1;
		}

		for(temp_start = 0; temp_start <= 90; temp_start++)
		{
			capsel_lut_left = *(uint8_t *)(rc32k_TBL_base + temp_start*2);
			sdm_lut_left = *(uint8_t *)(rc32k_TBL_base + temp_start*2 + 1);
			if(capsel_lut_left != 0 || sdm_lut_left != 0)
			{
				RC32K_CALIB_LOOKUP_TABLE_Set(temp_start, capsel_lut_left, sdm_lut_left);
			}
		}

		RC32K_CALIB_En();

		xy_free(rc32k_TBL_base);
	}
}

void Fill_Aon_LUT_With_Golden_Ref()
{
	if(HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
	{
		uint32_t tmp;
		uint8_t sdmNV;
		uint8_t capSelNV;
		uint8_t tempera_now;

		// 填写AON LUT时暂停后台状态机切换
		RC32K_CALIB_Dis();

		// 清除睡眠锁中断后可能产生的RC32K_IRQn中断，并在进入WFI前清除可能生成的rc_pending
		HWREGB(AONPRCM_BASE + 0x90) |= 0x1;

		// opencpu方案每半小时周期性校准时已经确保Aon LUT的正确性，睡眠前无需兜底
		for (int i = 0; i <= CRITICAL_LOW_TEMPERATURE; i++)
		{
			tmp = g_golden_ref[i][0] * 64 + (g_golden_ref[i][1] & 0x7F) + g_low_tempera_offset;

			RC32K_CALIB_LOOKUP_TABLE_Set(i, tmp / 64, (tmp % 64) | 0x80 );
		}
		for (int i = CRITICAL_LOW_TEMPERATURE + 1; i <= CRITICAL_HIGH_TEMPERATURE; i++)
		{
			tmp = g_golden_ref[i][0] * 64 + (g_golden_ref[i][1] & 0x7F) + g_regular_tempera_offset;

			RC32K_CALIB_LOOKUP_TABLE_Set(i, tmp / 64, (tmp % 64) | 0x80 );
		}
		for (int i = CRITICAL_HIGH_TEMPERATURE + 1; i <= 90; i++)
		{
			tmp = g_golden_ref[i][0] * 64 + (g_golden_ref[i][1] & 0x7F) + g_high_tempera_offset;

			RC32K_CALIB_LOOKUP_TABLE_Set(i, tmp / 64, (tmp % 64) | 0x80 );
		}

		tempera_now = rc32k_get_temperature(g_osc_cal, 5, 0);
		RC32K_CALIB_LOOKUP_TABLE_Get(tempera_now, &capSelNV , &sdmNV);

		HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;  //必须添加，否则会影响频率稳定性，但原因不明！
		RC32K_CALIB_NV_Save(capSelNV , sdmNV);
		REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);	// 取消强制参数传递

		RC32K_CALIB_Done();

		RC32K_CALIB_En();
	}
}

#endif


// 根据分段offset偏移更新Aon LUT和golden ref
__FLASH_FUNC void Reset_Aon_LUT()
{
	for (int i = 0; i <= 90; i++)
	{
		RC32K_CALIB_LOOKUP_TABLE_Set(i, 0, 0);
	}
	return ;
}

// 校准失败hook：参数回填
__FLASH_FUNC void RC32K_Cali_Failed_hook(uint8_t tempera, uint8_t done_flag)
{
	uint8_t i;
	uint8_t sdm = 0;
	uint8_t capSel = 0;
	rc_param_t golden_param;
	uint8_t sdm_lut, capSel_lut = 0xff;

	RC32K_CALIB_Dis();

	HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;  //必须添加，否则会影响频率稳定性，但原因不明！

	// 查询LUT中相邻20摄氏度的参数，校准期间供模拟过渡使用
	for( i = 0 ; i < 6 ;  i++ )
	{	
		if( tempera + i <= 90)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(tempera + i, &capSel , &sdm);
			if(capSel != 0 && sdm != 0 )
			{
				break;
			}
		}

		if( tempera - i >= 0)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(tempera - i , &capSel , &sdm);
			if(capSel != 0 && sdm != 0 )
			{
				break;
			}
		}
	}

	// 使用golden ref + offset供模拟过渡
	if( capSel == 0 &&  sdm == 0)
	{
		golden_param.tempera = tempera;
		get_golden_capsel( &golden_param);

		capSel =  golden_param.capsel;
		sdm = golden_param.sdm;
	}

	RC32K_CALIB_LOOKUP_TABLE_Set(tempera, capSel, sdm);	    // 本次校准失败，在下次校准前需提供一组RC的临时参数以维持32K频率稳定
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x3);	// 强制将RC校准参数传给模拟
	delay_func_us(10000);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);	// 取消强制参数传递

	//若校准完成后pll为force off状态，说明后续为单核状态，且sysclk非xtal26M，出于功耗考虑，force off xtal26M
	if((HWREGB(AON_SYSCLK_CTRL2) & 0x02) == 2 && g_XTAL26M_power_state == 1 && PRCM_SysclkSrcGet() != SYSCLK_SRC_XTAL)
	{
		HWREGB(AON_SYSCLK_CTRL2) |= 0x01;   //xtal26M force off
	}

	if(done_flag == 0)
	{
		RC32K_CALIB_Done();	  //调用后才能再次在该温度产生中断，触发校准
	}

#if (MODULE_VER != 0x0)
	RC32K_CALIB_LOOKUP_TABLE_Set(tempera, 0, 0 );
#endif
	RC32K_CALIB_LOOKUP_TABLE_Get(tempera, &capSel_lut , &sdm_lut);

	RC32K_CALIB_En();	

	xy_printf("RC_Cali hook:%d, %d, %d, %d,  %d, %d, %d, %d, lut_now:(%d, %d)\r\n",tempera,g_low_tempera_offset,g_regular_tempera_offset,g_high_tempera_offset, capSel, sdm, golden_param.capsel,  golden_param.sdm  ,capSel_lut, sdm_lut);
	return;
}

#if XY_DEBUG
volatile uint32_t record_mcnt_cnt1 = 0;
volatile uint32_t record_mcnt_cnt2 = 0;
volatile uint32_t record_clk_enable;
volatile uint32_t record_mcnt_start_tick = 0;
#endif
// mcnt测量： 自校准流程的最小计算单元
__FLASH_FUNC void measure_32k_clk(mcnt_t *mcnt_context, uint8_t block_or_not )
{
	PRCM_ClockEnable(CORE_CKG_CTL_MCNT_EN);
	while(!(COREPRCM->CKG_CTRL_L & CORE_CKG_CTL_MCNT_EN));
	
	// 触发32k校准，以更新default 32k value
	MCNT_SetClkSrc(0);
	MCNT_Stop();
#if XY_DEBUG
	record_clk_enable = HWREG(0x40004008);
#endif
	MCNT_SetCNT32k(mcnt_context->measure_length); 
    utc_cnt_delay(50);					//理论delay至少4个utc cnt以上即可,为了安全起见延长至50个cnt
#if XY_DEBUG
	record_mcnt_cnt1 = mcnt_context->measure_length;
	record_mcnt_cnt2 = MCNT_GetCNT32k();
	record_mcnt_start_tick = Get_Tick();
#endif
	NVIC_ClearPendingIRQ(MCNT_IRQn);
	MCNT_Start();
	
	if(block_or_not == BLOCK_FLAG)
	{
		while (0 == NVIC_GetPendingIRQ(MCNT_IRQn));
		NVIC_ClearPendingIRQ(MCNT_IRQn);
	}
}

__FLASH_FUNC void save_mcnt_context(mcnt_t *mcnt_context)
{
	mcnt_context->measure_length = MCNT_RC32K_CALI_COUNT;
	mcnt_context->clock_source = SYSCLK_SRC_XTAL;
}

__FLASH_FUNC uint32_t check_mcnt_result(mcnt_t *mcnt_context)
{
	//时钟源切换，则判定校准失败
	if(MCNT_GetMeasureClk() != 1)
	{
		return RC32K_MEASURE_FAILED;
	}

	//校准过程中xtal26M被下电，则判定校准失败
	if((HWREGB(AON_SYSCLK_CTRL2) & 0x01) == 1)
	{
		return RC32K_MEASURE_FAILED;
	}

	//记录mcnt测量结果
	mcnt_context->mcnt_result = MCNT_GetMCNT();

	//计算校准所得频率
	mcnt_context->freq = ((uint32_t)((uint64_t)(g_xtal_clock) * (uint64_t)(mcnt_context->measure_length) * 10 / (uint64_t)(mcnt_context->mcnt_result - 1)) + 5) / 10;

	MCNT_Stop();
	utc_cnt_delay(50);

	return RC32K_MEASURE_SUCCESS;
}


uint32_t rc_eq1_debug = 0;
uint32_t rc_eq2_debug = 0;
uint32_t rc_eq3_debug = 0;
uint8_t record_capsel_debug1 = 0;
uint8_t record_capsel_debug2 = 0;
uint8_t record_capsel_debug3 = 0;
// 自校准主流程
mcnt_t g_mcnt_context;
__FLASH_FUNC uint32_t rc32k_calibration(uint8_t capSelInitVal, uint8_t *capSelNV, uint8_t *sdmNV, bool block_or_not )
{
	static int32_t deltaRC32K = 0;
	static int32_t deltaRC32K2 = 0;
	static uint32_t rc32k_freq = 0;
	static uint32_t rc32k_freq2 = 0;
	static uint32_t rc32k_freq_h = 32000;
	static uint32_t rc32k_freq_l = 32000;
	static uint32_t rc32k_freq_m = 32000;

	static uint8_t sdm = 0;
	static uint32_t ret_val;
	static uint32_t cali_count = 0;  //收敛计算完成的循环次数
	static uint8_t capSelInitVal_tmp;

	static uint8_t g_rc_step = 0;

	// 状态机切换
	switch (g_rc_step)
	{
		case 1:
			goto g_rc_step_1;
			break;

		case 2:

			goto g_rc_step_2;
			break;
		case 3:

			goto g_rc_step_3;
			break;

		case 4:
			goto g_rc_step_4;
			break;
		default:
			capSelInitVal_tmp = capSelInitVal;
			break;
	}

	RC32K_CALIB_CAPSEL_INIT_Set(capSelInitVal_tmp);
	RC32K_CALIB_FORCE_SDM_OUT_DATA(0x0);
	RC32K_CALIB_SDM_En();
	prcm_delay(0x100);	

	while(cali_count < 100)
	{
		save_mcnt_context(&g_mcnt_context);
		measure_32k_clk(&g_mcnt_context,block_or_not);

		if(block_or_not  == NONBLOCK_FLAG)
		{
			g_rc_step = 1;
			return CALI_SUSPEND;
		}
g_rc_step_1:
		ret_val = check_mcnt_result(&g_mcnt_context);
		if(ret_val != RC32K_MEASURE_SUCCESS){
			goto error_process;
		}

		rc32k_freq = g_mcnt_context.freq;
		rc_eq1_debug = rc32k_freq;
		record_capsel_debug1 = capSelInitVal_tmp;

#if XY_DEBUG
		xy_printf("RC_Cali mcnt_result:%d,%d,%d,capsel1:%d,count:%d\r\n", g_mcnt_context.clock_source, g_mcnt_context.mcnt_result, g_mcnt_context.freq, record_capsel_debug1, cali_count);
#endif

		deltaRC32K = rc32k_freq - 32000;
		if (abs(deltaRC32K) < 200)
		{
			if (deltaRC32K >= 0)
				capSelInitVal_tmp++;
			else
				capSelInitVal_tmp--;

			RC32K_CALIB_CAPSEL_INIT_Set(capSelInitVal_tmp);
			prcm_delay(0x100);	

			save_mcnt_context(&g_mcnt_context);
			measure_32k_clk(&g_mcnt_context,block_or_not);

			if(block_or_not  == NONBLOCK_FLAG)
			{
				g_rc_step = 2;
				return CALI_SUSPEND;
			}
g_rc_step_2:
			ret_val = check_mcnt_result(&g_mcnt_context);
			if(ret_val != RC32K_MEASURE_SUCCESS){
				goto error_process;
			}

			rc32k_freq2 = g_mcnt_context.freq;
			rc_eq2_debug = rc32k_freq2;
			record_capsel_debug2 = capSelInitVal_tmp;

#if XY_DEBUG
			xy_printf("RC_Cali mcnt_result:%d,%d,%d,capsel2:%d,count:%d\r\n", g_mcnt_context.clock_source, g_mcnt_context.mcnt_result, g_mcnt_context.freq, record_capsel_debug2, cali_count);
#endif			

			deltaRC32K2 = rc32k_freq2 - 32000;
			if (deltaRC32K2 * deltaRC32K <= 0)
			{
				if (deltaRC32K >= 0)
				{
					rc32k_freq_h = rc32k_freq;
					capSelInitVal_tmp--;
				}
				else
				{
					rc32k_freq_h = rc32k_freq2;
				}
				break;	
			}
		}
		else
		{
			capSelInitVal_tmp += (deltaRC32K/100);
		}
		RC32K_CALIB_CAPSEL_INIT_Set(capSelInitVal_tmp);

		prcm_delay(0x100);
		cali_count++;
	}

  	//收敛计算次数过多，属于数字问题，软件无法进行容错处理
	if(cali_count >= 100)
	{
		xy_printf("\r\nRC_Cali Converge Fail\r\n");

		cali_count = 0;
		g_rc_step = 0;
		return CALI_FAIL_CONVERGENCE_FAIL;
	}

	RC32K_CALIB_CAPSEL_INIT_Set(capSelInitVal_tmp);
	RC32K_CALIB_FORCE_SDM_OUT_DATA(0x1);

	save_mcnt_context(&g_mcnt_context);
	measure_32k_clk(&g_mcnt_context,block_or_not);

	if(block_or_not  == NONBLOCK_FLAG)
	{
		g_rc_step = 3;
		return CALI_SUSPEND;
	}
g_rc_step_3:
	ret_val = check_mcnt_result(&g_mcnt_context);
	if(ret_val != RC32K_MEASURE_SUCCESS){
		goto error_process;
	}

	rc32k_freq_l =  g_mcnt_context.freq;
	rc_eq3_debug = rc32k_freq_l;
	record_capsel_debug3 = capSelInitVal_tmp;
	rc32k_freq_m = (uint32_t)((rc32k_freq_h - rc32k_freq_l)*10.0/22 + rc32k_freq_l);
	if (rc32k_freq_m > 32000)
		sdm = (rc32k_freq_m - 32000 ) * 64 / (rc32k_freq_m - rc32k_freq_l) + 64;
	else if (rc32k_freq_m < 32000)	 
		sdm = (rc32k_freq_h - 32000 ) * 64  / (rc32k_freq_h - rc32k_freq_m);
	else
		sdm = 64;

#if XY_DEBUG
	xy_printf("RC_Cali mcnt_result:%d,%d,%d,capsel3:%d,sdm:%d\r\n", g_mcnt_context.clock_source, g_mcnt_context.mcnt_result, g_mcnt_context.freq, record_capsel_debug3, sdm);
#endif

	HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;

	RC32K_CALIB_NV_Save(capSelInitVal_tmp, sdm);
	
	save_mcnt_context(&g_mcnt_context);
	measure_32k_clk(&g_mcnt_context,block_or_not);

	if(block_or_not  == NONBLOCK_FLAG)
	{
		g_rc_step = 4;
		return CALI_SUSPEND;
	}
g_rc_step_4:
	ret_val = check_mcnt_result(&g_mcnt_context);
	if(ret_val != RC32K_MEASURE_SUCCESS){
		goto error_process;
	}

	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);
	RC32K_CALIB_Done();

	*sdmNV = sdm;
	*capSelNV = capSelInitVal_tmp;
	g_rc_step = 0;
	cali_count = 0;
	HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;
	return g_mcnt_context.freq;

error_process:
	g_rc_step = 0;
	cali_count = 0;
	HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;
	return CALI_FAIL_CHECK_FAIL;
}

#if XY_DEBUG
volatile uint32_t record_val_addr;
volatile uint32_t record_original_freq = 0;
#endif
// 获取上电参数，用以计算初始offset！ 待FT校准流程完备删除！
__FLASH_FUNC void rc32k_get_default_value(uint8_t *capSelNV, uint8_t *sdmNV)
{
	uint8_t cycle_times = 0;
	uint32_t freq_32k_clk;
	uint8_t capSelNV_tmp;
	int8_t  sdmNV_tmp;
	uint32_t delta;
	uint32_t delta_last = 0;
#if XY_DEBUG
	//为定位开机阶段校准3次无法收敛到32000正负20HZ内的问题添加的debug信息
	char *record_val = xy_malloc(120);
	record_val_addr = (uint32_t)record_val;
#endif

	// 初始化RC频率至30K左右
	// RC32K_CALIB_SDM_En();//rc32k_sdm_en
	// REG_Bus_Field_Set(AONPRCM_ADIF_BASE, 99, 98, 0x2);
	// REG_Bus_Field_Set(AONPRCM_ADIF_BASE, 103, 100, 0x4);

	RC32K_CALIB_Dis();
#if XY_DEBUG
	save_mcnt_context(&g_mcnt_context);
	measure_32k_clk(&g_mcnt_context,BLOCK_FLAG);
	if(check_mcnt_result(&g_mcnt_context) == RC32K_MEASURE_SUCCESS)
	{
		record_original_freq = g_mcnt_context.freq;
	}
#endif

retry:
	cycle_times++;
	// 获取default 32k频率和对应的RC参数
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);	//force_capsel_reg_en	and force_sdm_out_en
	freq_32k_clk = rc32k_calibration(0x80, capSelNV, sdmNV, BLOCK_FLAG);
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x1);	//force_capsel_reg_en	and force_sdm_out_en

#if XY_DEBUG
	*(uint32_t*)(record_val + (cycle_times - 1) * 36) = freq_32k_clk;
	*(uint32_t*)(record_val + (cycle_times - 1) * 36 + 4) = rc_eq1_debug;
	*(uint32_t*)(record_val + (cycle_times - 1) * 36 + 8) = rc_eq2_debug;
	*(uint32_t*)(record_val + (cycle_times - 1) * 36 + 12) = rc_eq3_debug;
	*(uint8_t*)(record_val + (cycle_times - 1) * 36 + 16) = *capSelNV;
	*(uint8_t*)(record_val + (cycle_times - 1) * 36 + 20) = *sdmNV;
	*(uint8_t*)(record_val + (cycle_times - 1) * 36 + 24) = record_capsel_debug1;
	*(uint8_t*)(record_val + (cycle_times - 1) * 36 + 28) = record_capsel_debug2;
	*(uint8_t*)(record_val + (cycle_times - 1) * 36 + 32) = record_capsel_debug3;
#endif

	if(freq_32k_clk < 31980 || freq_32k_clk > 32020)
	{
		if(freq_32k_clk > 32000)
		{
			delta = freq_32k_clk - 32000;
		}
		else
		{
			delta = 32000 - freq_32k_clk;
		}

		if( (delta_last == 0) || (delta < delta_last))
		{
			capSelNV_tmp = *capSelNV;
			sdmNV_tmp = *sdmNV;
			delta_last = delta;
		}

		if(cycle_times < 3)
		{
			goto retry;
		}
		else
		{
#if XY_DUMP
			(void)capSelNV_tmp;
			(void)sdmNV_tmp;
			xy_assert(0);
#else
			*capSelNV = capSelNV_tmp;
			*sdmNV = sdmNV_tmp;
#endif
		}
	}

#if XY_DEBUG
	xy_free(record_val);
#endif
}

/*执行校准动作，由中断触发调用*/
__FLASH_FUNC void Do_RC32k_Cali(void)
{
	uint8_t sdmNV;
	uint8_t capSelNV;
	rc_param_t golden_param;
	int8_t tempera_t1;
	static int8_t tempera_t0_bak;
	uint32_t ret = 0;
	int32_t offset;
#if XY_DEBUG
	__attribute__((unused)) uint8_t LUT_sdmNV;
	__attribute__((unused)) uint8_t LUT_capSelNV;
#endif

	// RC自校准时期暂停后台状态机切换，防止对校准流程造成干扰
 	RC32K_CALIB_Dis();

	if( g_rc32k_cali_flag != 3)
	{
		// 非suspend导致的重入,需要保存初始温度
		tempera_t0_bak = g_tempera_t0;
		g_cali_start_tick = Get_Tick();
	}

	g_rc32k_cali_flag = 0;

	g_XTAL26M_power_state = (HWREGB(AON_SYSCLK_CTRL2) & 0x01);
	if(g_XTAL26M_power_state == 1)
	{
        HWREGB(0x40000059) = 0x7;   // itune = 7
        HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2   
		HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //pu xtal26M
		while(!(COREPRCM->ANAXTALRDY & 0x01));   //xtal26M ready
	}

	MCNT_SelectMeasureClk(1);  //MCNT_measureclk选用XTAL26M

	// 定位无法收敛的bug
	g_debug_error1 = HWREG(0x40000070);
	g_debug_error2 = HWREG(0x40000080);
	g_debug_error3 = HWREG(0x4000008c);
	g_debug_error4 = HWREG(0x40000090);

	// 自校准主流程
	golden_param.tempera = tempera_t0_bak;
	get_golden_capsel( &golden_param);

 #if XY_DEBUG
	xy_printf("RC_Cali before ret:%d,osc_cal:%d,%d, cap_init:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%d,%d\r\n",ret, g_osc_cal, HWREG(BAK_MEM_OTP_RC32KCAL_BASE), golden_param.capsel,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090),Tick_CounterGet(),HWREGB(BAK_MEM_RC32K_CALI_FLAG));
 #endif

    ret = rc32k_calibration( golden_param.capsel , &capSelNV, &sdmNV, NONBLOCK_FLAG);
	if(ret == CALI_SUSPEND) 
	{   
		// 校准未完成,后台查表暂停，
		g_rc32k_cali_flag = 2;
		return;
	}
	else if((ret == CALI_FAIL_CONVERGENCE_FAIL) || (ret == CALI_FAIL_CHECK_FAIL))
	{   
// #if XY_DEBUG
// 		xy_printf("RC_Cali reg_err1_b:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_osc_cal,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
// #endif	
		//无论校准是否成功，校准流程结束后都将置BAK_MEM_RC32K_CALI_FLAG为0，此后允许CP核进入睡眠
		HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0;
		//由于调频、时钟源切换或频率计算无法收敛导致校准失败，放弃本次校准
		RC32K_Cali_Failed_hook(tempera_t0_bak, 0); 
#if XY_DEBUG
		if(ret == CALI_FAIL_CHECK_FAIL)
		{
			xy_printf("RC_Cali measure_fail:%d,%d,%d,%d",g_mcnt_context.clock_source,MCNT_GetMeasureClk(),HWREGB(AON_SYSCLK_CTRL2),PRCM_SysclkSrcGet());
		}
		else if (ret == CALI_FAIL_CONVERGENCE_FAIL)
		{
			xy_printf("RC_Cali conver");
		}
#endif

// #if XY_DEBUG
// 		xy_printf("RC_Cali reg_err1_a:%d, %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",ret, g_osc_cal,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
// #endif
	}
	else if(ret < 31995 || ret > 32005)
	{
#if XY_DEBUG
		// 校准结果异常，还原为默认参数
		xy_printf("RC_Cali reg_err2_b:%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_osc_cal,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
#endif
		//无论校准是否成功，校准流程结束后都将置BAK_MEM_RC32K_CALI_FLAG为0，此后允许CP核进入睡眠
		HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0;
		//校准完成但频率偏离32000超过±2Hz，认定本次校准失败
		RC32K_Cali_Failed_hook(tempera_t0_bak, 1);  

#if XY_DEBUG
		 xy_printf("RC_Cali reg_err2_a:%d,%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",ret,g_osc_cal,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
#endif
	}
	else
	{
		//无论校准是否成功，校准流程结束后都将置BAK_MEM_RC32K_CALI_FLAG为0，此后允许CP核进入睡眠
		HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0;   
		// 确保校准前后的温度一致
		tempera_t1 = rc32k_get_temperature(g_osc_cal, 5, 0);	
		if(tempera_t1 == tempera_t0_bak)
		{
			/* opencpu 形态：根据当前校准结果更新Aging Offset 并更新LUT*/
			#if (MODULE_VER == 0x0)
			    // AON LUT更新期间保持lpts状态机停止
				RC32K_CALIB_Dis();

				// 获取当前温度的offset，更新Aon Lut 以应对aging问题
				offset = get_Cali_Tbl_Offset(capSelNV, sdmNV, tempera_t1);	// 已经校准过的点使用Aging Offset微调参数，未校准的点使用Ft Offset + Aging Offset当作缺省值
				if(tempera_t1 <= CRITICAL_LOW_TEMPERATURE ){
					Update_Aon_LUT_By_Offset_Open(0, CRITICAL_LOW_TEMPERATURE, offset);
				}
				else if(tempera_t1 <= CRITICAL_HIGH_TEMPERATURE ){
					Update_Aon_LUT_By_Offset_Open(CRITICAL_LOW_TEMPERATURE + 1 , CRITICAL_HIGH_TEMPERATURE, offset);
				}
				else{
					Update_Aon_LUT_By_Offset_Open(CRITICAL_HIGH_TEMPERATURE + 1 , 90, offset);
				}

				// 当前温度againg处理
				RC32K_CALIB_LOOKUP_TABLE_Set(tempera_t1, capSelNV, sdmNV | 0x80 );

				// 维护软件的cali_tbl以提高精度
				if( g_cali_tbl[tempera_t1][2] == 0  )
				{
					g_cali_tbl[tempera_t1][0] = capSelNV;
					g_cali_tbl[tempera_t1][1] = sdmNV | 0x80;;
					g_cali_tbl[tempera_t1][2] = 1;
				}

				// 校准完成 重启状态机
				RC32K_CALIB_En();
				#if XY_DEBUG
					xy_printf("RC_Cali success:%x,%d,%d,%d,%d,%d,%d,%d\r\n",g_osc_cal,tempera_t0_bak,g_last_reset_tick,capSelNV,sdmNV,offset,g_low_tempera_offset,g_regular_tempera_offset,g_high_tempera_offset);
					xy_printf("RC_Cali success:%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
				#endif
			#else
				/* 模组形态：获取当前温度挡位的offset  提高睡眠期间的RC精度*/
				offset = get_golden_ref_offset(capSelNV, sdmNV, tempera_t1);
				if(tempera_t1 <= CRITICAL_LOW_TEMPERATURE ){
					g_low_tempera_offset = offset;
				}
				else if(tempera_t1 <= CRITICAL_HIGH_TEMPERATURE ){
					g_regular_tempera_offset = offset;
				}
				else{
					g_high_tempera_offset = offset;
				}

				//成功校准的温度写入链表，睡眠前写入flash_tbl
				Insert_CaliTbl_Node(tempera_t0_bak, (sdmNV | 0x80), capSelNV);	

				//若校准完成后pll为force off状态，说明后续为单核状态，且sysclk非xtal26M，出于功耗考虑，force off xtal26M
				if((HWREGB(AON_SYSCLK_CTRL2) & 0x02) == 2 && g_XTAL26M_power_state == 1 && PRCM_SysclkSrcGet() != SYSCLK_SRC_XTAL)
				{
					HWREGB(AON_SYSCLK_CTRL2) |= 0x01;   //xtal26M force off
				}

				// 校准完成 重启状态机
				RC32K_CALIB_En();

				#if XY_DEBUG
					RC32K_CALIB_LOOKUP_TABLE_Get(tempera_t0_bak, &LUT_capSelNV ,&LUT_sdmNV);
					xy_printf("RC_Cali success:%x,%d,%d,%d,%d,%d,%d,%d,%d\r\n",g_osc_cal,tempera_t0_bak,g_last_reset_tick,capSelNV,sdmNV,offset,g_low_tempera_offset,g_regular_tempera_offset,g_high_tempera_offset);
					xy_printf("RC_Cali success:%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
					xy_printf("RC_Cali success:%d,%d,%d,AON_LUT:%d,%d\r\n", rc_eq1_debug, rc_eq2_debug, rc_eq3_debug, LUT_capSelNV, LUT_sdmNV);
				#endif
			#endif
		}
		else
		{   	
			#if XY_DEBUG
				xy_printf("RC_Cali reg_err_temp_b:%x,%d,%d,%d,%d,%d,%x,%x,%x,%x,%x,%x,%x,%x\r\n",g_osc_cal,capSelNV,sdmNV,tempera_t1,tempera_t0_bak,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
			#endif

			//校准前后温度变化，舍弃本次校准
			RC32K_Cali_Failed_hook(tempera_t0_bak, 1); 
			#if XY_DEBUG
				xy_printf("RC_Cali reg_err_temp_a:%d,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",ret,g_osc_cal,HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090));
			#endif
		}	
	}
}

/*OPENCPU客户专用，触发校准事件*/
void Set_RC32K_Cali_Event(void)
{
	set_event(EVENT_RC32K_CALI);
}

#if (MODULE_VER)
//  模组形态 RC32K老化问题解决方案：
//  		非快速恢复的深睡唤醒：睡眠前填满了LUT，唤醒从flash_tbl中回填已校准值。在初始化时Set_Restore_LUT_Flag，压缩唤醒到开中断时长。
//  		针对永不睡眠的特殊场景，在主循环中比较重置Aon LUT时间差，如果超过RC32K_PARM_AGING_TIME（6小时）则重置Aon LUT,所有温度重新校准
//  此标记位供主循环使用，只在必要时执行老化流程，以减少无效代码执行次数！
uint32_t g_restore_LUT_flag = 0;

void Set_Restore_LUT_Flag(uint8_t flag)
{
	g_restore_LUT_flag = flag;
	return;
}

uint8_t Get_Restore_LUT_Flag(void)
{
	return g_restore_LUT_flag;
}

/*模组形态应对老化问题，主动由平台清空Aon LUT，所有温度重新校准*/
__FLASH_FUNC void Platform_Trigger_RC32K_Cali(void)
{
	uint8_t i;
	uint8_t sdmNV;
	uint8_t capSelNV;
	uint8_t tempAddr;
	int32_t offset;
	uint32_t tmp;

#if XY_DEBUG
 	xy_printf("RC_Cali before cap_init:%x,%x,%x,%x,%x,%x,%x,%x,%x,%d\r\n",HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090),Tick_CounterGet());
	volatile uint8_t g_tempe = 0;
	volatile uint32_t cur_tick;
	cur_tick = Get_Tick();

	g_tempe = RC32K_CALIB_TEMP_VAL_Get();
	RC32K_CALIB_LOOKUP_TABLE_Get(g_tempe, &capSelNV , &sdmNV);
#endif

	RC32K_CALIB_Dis();
	utc_cnt_delay(80);    //若dis en时，lpts已处于周期点内，则必会完成该次周期（3cnt上电+64cnt计数器+3cnt查表、中断）

	// 获取历史温度值以及对应的golden ref RC参数
	tempAddr = rc32k_get_last_temperature();

	// 查询LUT中相邻20摄氏度的参数，校准期间供模拟过渡使用
	for( i = 0 ; i < 6 ;  i++ )
	{	
		if( tempAddr + i <= 90)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(tempAddr + i, &capSelNV , &sdmNV);
			if(capSelNV != 0 && sdmNV != 0 )
			{
				break;
			}
		}

		if( tempAddr - i >= 0)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(tempAddr - i , &capSelNV , &sdmNV);
			if(capSelNV != 0 && sdmNV != 0 )
			{
				break;
			}
		}
	}

	// LUT中相邻20摄氏度的都未曾校准，使用golden ref+ offset供模拟过渡使用
	if(capSelNV == 0 && sdmNV == 0 )
	{
		if(tempAddr <= CRITICAL_LOW_TEMPERATURE ){
			offset = g_low_tempera_offset;
		}
		else if(tempAddr <= CRITICAL_HIGH_TEMPERATURE ){
			offset = g_regular_tempera_offset;
		}
		else{
			offset = g_high_tempera_offset;
		}

		tmp = g_golden_ref[tempAddr][0] * 64 + (g_golden_ref[tempAddr][1] & 0x7F) + offset;
		capSelNV = tmp / 64;
		sdmNV = tmp % 64;
	}	

	// 清空Aon LUT，唤醒后所有温度重新校准
	Reset_Aon_LUT();

	// 强制使用缺省RC参数值，确保初始化阶段的RC稳定性
	RC32K_CALIB_NV_Save(capSelNV , sdmNV);

	// 取消强制参数传递
	REG_Bus_Field_Set(AONPRCM_BASE + 0x82,  1,  0, 0x0);

	// 使能后台周期性查表机制，使用now模式可快速产生新的校准中断
	RC32K_CALIB_EnNow();
	RC32K_CALIB_En();

	utc_cnt_delay(2);   //延时2~3个utc_clk
	RC32K_CALIB_DisNow();
	
	g_last_reset_tick = Get_Tick();   //每次重置Aon LUT，更新记录时刻点，应对老化问题
	reset_32KCali_flash_tbl();  //清空flash_tbl中的所有校准参数信息

#if XY_DEBUG
	xy_printf("RC_Cali platform trigger wkp %d cap %d sdm %d last_tick:%d cur:%dr\n", g_tempe, capSelNV, sdmNV, g_last_reset_tick, cur_tick);
 	xy_printf("RC_Cali before cap_init:%x,%x,%x,%x,%x,%x,%x,%x,%x,%d\r\n",HWREG(0x40000070),HWREG(0x40000074),HWREG(0x40000078),HWREG(0x4000007c),HWREG(0x40000080),HWREG(0x40000084),HWREG(0x40000088),HWREG(0x4000008c), HWREG(0x40000090),Tick_CounterGet());
#endif
}

__FLASH_FUNC void mcnt_check_for_aging(void)
{
	uint32_t ret_val;
#if XY_DEBUG
	int8_t tempera_cur;
	uint8_t capsel_lut;
	uint8_t sdm_lut;
#endif

	switch (g_rc32k_aging_flag)
	{
		case 0:
			g_XTAL26M_power_state = (HWREGB(AON_SYSCLK_CTRL2) & 0x01);
			if(g_XTAL26M_power_state == 1)
			{
                HWREGB(0x40000059) = 0x7;   // itune = 7
                HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2   
				HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //pu xtal26M
				while(!(COREPRCM->ANAXTALRDY & 0x01));   //xtal26M ready
			}
			MCNT_SelectMeasureClk(1);  //MCNT_measureclk选用XTAL26M
			prcm_delay(1000);
			save_mcnt_context(&g_mcnt_context);
			g_mcnt_context.measure_length = MCNT_RC32K_CALI_COUNT * 2;	// 老化测量的时间为正常测量的4倍，避免测量引入误差
			measure_32k_clk(&g_mcnt_context,NONBLOCK_FLAG);
			g_rc32k_aging_flag = 1;								// 老化测量期间，禁止自校准（且禁止休眠）
			
			#if XY_DEBUG
				xy_printf("RC_Cali aging 0 :%d\r\n", g_mcnt_context.measure_length);
			#endif
			break;
		
		case 1:
			xy_assert(0);
			break;
		
		case 2:
			ret_val = check_mcnt_result(&g_mcnt_context);
			if(ret_val != RC32K_MEASURE_SUCCESS)
			{
				g_LUT_cleanup_flag = 0;							// 测量完成，允许进入休眠
				g_rc32k_aging_flag = 0;							// 测量失败，待下一次重新测量
				xy_printf("RC_Cali aging 1 :mcnt_fail\r\n");
			}
			else
			{
				if(  g_mcnt_context.freq > 32010 || g_mcnt_context.freq < 31990)
				{
					#if XY_DEBUG
						for (tempera_cur = 0; tempera_cur <= 89; tempera_cur++)
						{
							RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur, &capsel_lut, &sdm_lut);

							if(capsel_lut != 0)
							{
								xy_printf("RC_Cali 11 tp:%d,cap:%d,sdm:%d\r\n",tempera_cur, capsel_lut,sdm_lut);
							} 
						}
					#endif
					Platform_Trigger_RC32K_Cali();				// 已老化，则清空LUT	
					Set_Restore_LUT_Flag(0);						
				}
				else
				{
					/* null*/									// 未老化，等待进入休眠即可
				}

				g_LUT_cleanup_flag = 0;							// 测量完成，允许进入休眠
				g_rc32k_aging_flag = 0;							// 测量成功，结束老化流程

				#if XY_DEBUG
					xy_printf("RC_Cali aging:%d\r\n",g_mcnt_context.freq);
				#endif
			}
			//若校准完成后pll为force off状态，说明后续为单核状态，且sysclk非xtal26M，出于功耗考虑，force off xtal26M
			if((HWREGB(AON_SYSCLK_CTRL2) & 0x02) == 2 && g_XTAL26M_power_state == 1 && PRCM_SysclkSrcGet() != SYSCLK_SRC_XTAL)
			{
				HWREGB(AON_SYSCLK_CTRL2) |= 0x01;   //xtal26M force off
			}						
			break;
	
		default:
			xy_assert(0);
			break;
	}
}

void RC32k_Cleanup_Timeout(void)
{
	g_LUT_cleanup_flag = 1;
}

__FLASH_FUNC void RC32K_cleanup_event_set(uint32_t LUT_clean_period)
{
	if(	(Get_Boot_Reason() != WAKEUP_DSLEEP)  && (LUT_clean_period != 0) && (HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K))
	{
		Timer_AddEvent(TIMER_LP_RCCALI, LUT_clean_period * 60000 , RC32k_Cleanup_Timeout, 1);	
	}
}

#else
//  opencpu形态 RC32K老化问题解决方案：
//             初始化阶段已经依据golden_ref+ft_offset填满Aon LUT,后续由用户主动周期性触发校准更新并使用Aging_offset

extern void Peri1_PclkDivSet(uint32_t ClockDiv);
extern void Peri2_PclkDivSet(uint32_t ClockDiv);
/*用户触发的RC32K校准流程，内部会使能校准中断*/
__FLASH_FUNC void User_Trigger_RC32K_Cali(void)
{
#if XY_DEBUG
	volatile uint8_t g_tempe = 0;
	uint8_t sdmNV;
	uint8_t capSelNV;

	// 获取触发中断时的温度，确保校准前后温度一致
	g_tempe = RC32K_CALIB_TEMP_VAL_Get();
	RC32K_CALIB_LOOKUP_TABLE_Get(g_tempe, &capSelNV , &sdmNV);
	xy_printf("RC_Cali user trigger wkp %d cap %d sdm %d r\n",g_tempe ,capSelNV,sdmNV);
#endif

	// 触发rc自校准中断，目的是启动自校准流程以更新offset
	*((volatile uint32_t *)0xe000e204) = (0x1 << 13);

	// 避免重复触发
	clear_event(EVENT_RC32K_CALI);
}
#endif

/*
 * @brief 该函数在main主函数的while循环中调用，执行校准算法动作，并保存校准数据。
 * @return 1：当前正在执行校准；0：当前未执行校准流程。
 * @note  该函数执行校准动作时耗时至少需要420ms。校准期间不阻塞，可正常响应中断，不影响用户的业务流程。
 * @note  模组形态：校准由硬件触发，用户无需关心。
 * @note  opencpu形态：校准由用户自主触发，需周期性调用Set_RC32K_Cali_Event接口触发校准事件
*/
int RC32k_Cali_Process(void)
{

    //adc关闭trxbg有时间间隔要求，此处做兜底处理
	ADC_Trxbg_SafelyClose();

	if(HWREGB(BAK_MEM_32K_CLK_SRC) == XTAL_32K)
	{
		return 0;
	}

#if XY_DEBUG
	int8_t tempera_cur;
	uint8_t capsel_lut;
	uint8_t sdm_lut;
	if(CP_Is_Alive() == true)	
	{
		if(  HWREGB(BAK_MEM_RC32K_DIAGNOSE) == 1)
		{	
			HWREGB(BAK_MEM_RC32K_DIAGNOSE) = 0;
			HWREGB(BAK_MEM_MCNT_PROCESS_FLAG) = 1;

			tempera_cur = rc32k_get_last_temperature();
	
			RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur, &capsel_lut, &sdm_lut);
			xy_printf("RC_Cali DIAGNOSE temp:%d,osc_cal:%d,sdm_lut:%d,capsel_lut:%d\r\n",tempera_cur, g_osc_cal,sdm_lut, capsel_lut);

			RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur-1, &capsel_lut, &sdm_lut);
			xy_printf("RC_Cali DIAGNOSE temp:%d,osc_cal:%d,sdm_lut:%d,capsel_lut:%d\r\n",tempera_cur-1, g_osc_cal,sdm_lut, capsel_lut);

			RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur+1, &capsel_lut, &sdm_lut);
			xy_printf("RC_Cali DIAGNOSE temp:%d,osc_cal:%d,sdm_lut:%d,capsel_lut:%d\r\n",tempera_cur+1, g_osc_cal,sdm_lut, capsel_lut);

			HWREGB(BAK_MEM_MCNT_PROCESS_FLAG) = 0;
		}
	}
#endif

#if (MODULE_VER)

	if( g_LUT_clean_period_min != 0)
	{
		// 老化定时器超时 && 当前未执行自校准流程 && 老化处理进行中
		//  g_rc32k_aging_flag 0： 老化流程未开始   1：后台测量中  2：测量完成
		if((g_LUT_cleanup_flag == 1) && (g_rc32k_cali_flag == 0 || g_rc32k_cali_flag == 1) && (g_rc32k_aging_flag == 0 || g_rc32k_aging_flag == 2))
		{
			mcnt_check_for_aging();
		}
	}
	else
	{
		// 本次唤醒距离上一次已校准的唤醒时刻超过1小时，强行令RC32k完全重新校准！
		if(Check_Ms_Timeout(g_last_reset_tick, RC32K_PARM_AGING_TIME) == 1 && (g_rc32k_cali_flag == 0 || g_rc32k_cali_flag == 1))
		{
			Platform_Trigger_RC32K_Cali();
			Set_Restore_LUT_Flag(0);  
		}
	}
		
	if(Get_Restore_LUT_Flag() == 1)
	{
#if XY_DEBUG
		if( g_LUT_clean_period_min != 0)
		{
			for (tempera_cur = 0; tempera_cur <= 90; tempera_cur++)
			{
				RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur, &capsel_lut, &sdm_lut);

				if(capsel_lut != 0)
				{
					xy_printf("RC_Cali 22 tp:%d,cap:%d,sdm:%d\r\n",tempera_cur, capsel_lut,sdm_lut);
				} 
			}
		}	
#endif
		//非快速恢复的深睡唤醒，从flash_tbl向Aon LUT恢复已校准的温度信息
		Restore_Lookup_Tbl_from_flash();
		Set_Restore_LUT_Flag(0);
#if XY_DEBUG
		for (tempera_cur = 0; tempera_cur <= 90; tempera_cur++)
		{
			RC32K_CALIB_LOOKUP_TABLE_Get(tempera_cur, &capsel_lut, &sdm_lut);

			if(capsel_lut != 0)
			{
				xy_printf("RC_Cali 33 tp:%d,cap:%d,sdm:%d\r\n",tempera_cur, capsel_lut,sdm_lut);
			} 
		}
#endif
	}

#if(XY_DUMP == 0)
	if(Check_Ms_Timeout(g_cali_start_tick, RC32K_CALI_PROCESS_TIMEOUT) == 1 && (g_rc32k_cali_flag == 2 || g_rc32k_cali_flag == 3))
	{
		g_rc32k_cali_flag = 0;
		HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0; 
		RC32K_Cali_Failed_hook(g_tempera_t0, 0); 
		PRCM_ClockDisable(CORE_CKG_CTL_MCNT_EN);
		NVIC_ClearPendingIRQ(MCNT_IRQn);
		xy_printf("\r\nRC_Cali MCNT_ABNORMAL\r\n");
	}
#endif

#else
	// open形态由用户协助触发offset补偿，以应对老化问题！
	if(is_event_set(EVENT_RC32K_CALI))
	{
		User_Trigger_RC32K_Cali();
	}
#endif

	// 校准中断事件触发且CP不处于休眠模式时可以进行RC自校准
	if( (g_rc32k_cali_flag == 1 || g_rc32k_cali_flag == 3) && ((CP_Is_Alive() == false) || HWREGB(BAK_MEM_RC32K_CALI_PERMIT) == 1) && (g_rc32k_aging_flag == 0))
	{
		Do_RC32k_Cali();
	}

	if((g_rc32k_cali_flag != 0) || (g_LUT_cleanup_flag != 0))
		return 1;
	else
		return 0;
}


void rc32k_int_handler(void)
{
	// 清中断
	HWREGB(AONPRCM_BASE + 0x90) |= 0x1;

	// 获取触发中断时的温度，确保校准前后温度一致
	g_tempera_t0 = RC32K_CALIB_TEMP_VAL_Get();

	// 主循环RC校准标记位
	g_rc32k_cali_flag = 1;

	// AP传递给CP，校准期间不允许CP核进入睡眠，避免PPM异常
	HWREGB(BAK_MEM_RC32K_CALI_FLAG) = 0xcc;  

 	//唤醒CP,主要针对CP deepsleep
	PRCM_ApCpIntWkupTrigger();  

	//手动除能rc32k_calib_wkup_ena，防止唤醒标记位异常 
	RC32K_CALIB_WakeUp_Dis();
	while((HWREGB(AONPRCM_BASE + 0x9) & 0x40) == 0x40);   //等待rc32k_calib_wkup_stat被清零（只要置1，必须手动清除，硬件不支持自动清除）
	RC32K_CALIB_WakeUp_En();
}

__FLASH_FUNC void get_section_offset(uint8_t tempera, int32_t *ft_offset_low, int32_t *ft_offset_regular, int32_t *ft_offset_high, Cali_Ftl_t *ram_32kfreq_TBL_base_addr)
{
	uint8_t sdmNV;
	uint8_t capSelNV;
	int32_t ft_offset;

	rc32k_get_default_value(&capSelNV , &sdmNV);

	ft_offset= get_golden_ref_offset(capSelNV, sdmNV, tempera);

	if(tempera <= CRITICAL_LOW_TEMPERATURE)
	{
		*ft_offset_low = ft_offset;
		ram_32kfreq_TBL_base_addr->low_offset = ft_offset;  //flash_tbl结构若变动，此处需同步整改
		ram_32kfreq_TBL_base_addr->low_capsel = capSelNV;
		ram_32kfreq_TBL_base_addr->low_sdm = sdmNV;
	}
	else if(tempera <= CRITICAL_HIGH_TEMPERATURE)
	{
		*ft_offset_regular = ft_offset;
		ram_32kfreq_TBL_base_addr->mid_offset = ft_offset;  //flash_tbl结构若变动，此处需同步整改
		ram_32kfreq_TBL_base_addr->mid_capsel = capSelNV;
		ram_32kfreq_TBL_base_addr->mid_sdm = sdmNV;
	}
	else
	{
		*ft_offset_high = ft_offset;
		ram_32kfreq_TBL_base_addr->high_offset = ft_offset;  //flash_tbl结构若变动，此处需同步整改
		ram_32kfreq_TBL_base_addr->high_capsel = capSelNV;
		ram_32kfreq_TBL_base_addr->high_sdm = sdmNV;
	}
}

/*非深睡唤醒必须进行一次校准设置，无太多耗时风险*/

__FLASH_FUNC void init_32k_clk(void)
{
 	uint8_t sdmNV;
	uint8_t capSelNV;
	uint8_t temp_ft;
	uint8_t read_fail;
	int32_t ft_offset_low = 0xFFFFFFFF;	// 置offset为无效值
	int32_t ft_offset_regular = 0xFFFFFFFF;
	int32_t ft_offset_high = 0xFFFFFFFF; // 置offset为无效值
	int32_t ft_offset_valid;

	mcnt_t mcnt_context;
	Cali_Ftl_t *g_32K_table;

	g_LUT_clean_period_min = READ_FAC_NV(uint32_t,rc32k_aging_period);

	if(Get_Boot_Reason() !=  WAKEUP_DSLEEP) 
	{
		if((HWREGB(AON_SYSCLK_CTRL2) & 0x01) == 1)
		{
			if (HWREGB(AON_SYSCLK_CTRL2) & 0x1)
            {
                HWREGB(0x40000059) = 0x7;   // itune = 7
                HWREGB(COREPRCM_ADIF_BASE + 0x5A) |= 0x10;  //xtalm power select ldo2
            }

            HWREGB(AON_SYSCLK_CTRL2) &= 0xFE;   //xtal on
            while (!(COREPRCM->ANAXTALRDY & 0x01)); //xtal ready
		}

		MCNT_SelectMeasureClk(1);  //MCNT_measureclk选用XTAL26M

		select_32k_clk_src();

		g_32K_table = xy_malloc(sizeof(Cali_Ftl_t));

		if(xy_ftl_read(CALIB_FREQ_BASE, (void *)g_32K_table, sizeof(Cali_Ftl_t)) == 0 || g_32K_table->default_freq == 0xFFFFFFFF || g_clk_src_last != HWREGB(BAK_MEM_32K_CLK_SRC))
		{
			/* 校验失败，重置flash_tbl*/ 
			memset(g_32K_table, 0xFF, sizeof(Cali_Ftl_t) - 8);
			read_fail = 0;
		}
		else
		{	
			read_fail = 1;
		}

		// xtal32k初始化
		if( HWREGB(BAK_MEM_32K_CLK_SRC) == XTAL_32K)
		{
			if(read_fail == 0)
			{
				save_mcnt_context(&mcnt_context);
				measure_32k_clk(&mcnt_context, BLOCK_FLAG);
				check_mcnt_result(&mcnt_context);

				if(mcnt_context.freq < 31768 || mcnt_context.freq > 33768)
				{
					while(1);  //频率高\低于32768超过1000Hz，则系统主动卡死！
				}
				else
				{
					g_32K_table->default_freq = mcnt_context.freq;
				}	
			}
			MCNT_SelectMeasureClk(0);  //XTAL32K完成后切换MCNT_measureclk选用SYSCLK，后续在CP侧基于PLL进行测量
		}
		// rc32k初始化
		else if( HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
		{
			if((HWREGB(AONPRCM_BASE + 0x70) & 0x01) == 0)
			{
				// 初始化RC的电阻值，将频率调到30K左右
				REG_Bus_Field_Set(AONPRCM_ADIF_BASE,  106,  106,  0x1);		// 使能sdm模拟开关
				REG_Bus_Field_Set(AONPRCM_ADIF_BASE,  99, 98, 0x2);			// 配置Rn
				REG_Bus_Field_Set(AONPRCM_ADIF_BASE,  103, 100, 0x4);		// 配置Rp
			}
			
			RC32K_CALIB_Dis();
			utc_cnt_delay(80);    //若dis en时，lpts已处于周期点内，则必会完成该次周期（3cnt上电+64cnt计数器+3cnt查表、中断）
			RC32K_CALIB_Done();

			// 重置Aon LUT：模组形态所有温度挡位全部重新校准
			Reset_Aon_LUT();

			if(read_fail == 0)
			{
				if(HWREG(BAK_MEM_OTP_RC32KCAL_BASE) == 0xFFFFFFFF)  //FT阶段没有做lpts校准，otp中无lpts校准参数信息
				{
					// 认定初次上电的温度为25摄氏度(index 37)，主动校准以获得当前的offset并保存至flash中
					get_section_offset(37, &ft_offset_low, &ft_offset_regular, &ft_offset_high, g_32K_table);

					// 校准g_osc_cal参数（25摄氏度 5表示粒度）,待FT校准流程完备删除！
					g_osc_cal = rc32k_lpts_calibration(25, 5);
					
					RC32K_CALIB_OSC_CAL_Set(g_osc_cal);	
					prcm_delay(1000); 										// 实测发现必须delay, 否则温度获取异常！
					temp_ft = 37;
				}
				else
				{
					g_osc_cal = HWREGH(BAK_MEM_OTP_RC32KCAL_BASE);
					RC32K_CALIB_OSC_CAL_Set(g_osc_cal);	
					prcm_delay(1000);
					temp_ft = rc32k_get_temperature(g_osc_cal, 5, 1);
					get_section_offset(temp_ft, &ft_offset_low, &ft_offset_regular, &ft_offset_high, g_32K_table);
				}
				
				g_32K_table->osc_cal = g_osc_cal;
				g_32K_table->default_freq = 32000;
			}
			else 
			{
				// 从flash中恢复lpts校准参数以及分段的ft_offset
				g_osc_cal = g_32K_table->osc_cal;
				ft_offset_low = g_32K_table->low_offset;
				ft_offset_regular = g_32K_table->mid_offset;
				ft_offset_high = g_32K_table->high_offset;

				RC32K_CALIB_OSC_CAL_Set(g_osc_cal);	
				prcm_delay(1000);

				// 若当前温度段的offset缺失，则主动触发校准以获取，最终目的是提高睡眠阶段golden ref的精度
				temp_ft = rc32k_get_temperature(g_osc_cal, 5, 1);
				if(((temp_ft <= CRITICAL_LOW_TEMPERATURE) && (ft_offset_low == (int32_t)0xFFFFFFFF)) ||  \
					((temp_ft > CRITICAL_LOW_TEMPERATURE) && (temp_ft <= CRITICAL_HIGH_TEMPERATURE) && (ft_offset_regular == (int32_t)0xFFFFFFFF)) ||  \
					((temp_ft > CRITICAL_HIGH_TEMPERATURE) && (ft_offset_high == (int32_t)0xFFFFFFFF)) )
				{	
					get_section_offset(temp_ft, &ft_offset_low, &ft_offset_regular, &ft_offset_high,g_32K_table);
				}
			}

			utc_cnt_delay(3);
			RC32K_CALIB_Dis();
			utc_cnt_delay(2);

			//清除中断标志位，防止在初始化阶段产生pending的RC中断
			HWREGB(AONPRCM_BASE + 0x90) |= 0x1;
			RC32K_CALIB_WakeUp_Dis();
			while((HWREGB(AONPRCM_BASE + 0x9) & 0x40) == 0x40);
			*((volatile uint32_t *)0xe000e284) |= (0x1 << (RC32K_IRQn - 32));
			
			RC32K_CALIB_Done();	

			// 更新各温度段的offset全局
			if((uint32_t)ft_offset_regular != 0xFFFFFFFF)
			{
				ft_offset_valid = ft_offset_regular;
			}
			else
			{
				ft_offset_valid = (ft_offset_low == (int32_t)0xFFFFFFFF) ? ft_offset_high : ft_offset_low;
			}
			g_low_tempera_offset = (ft_offset_low == (int32_t)0xFFFFFFFF) ? ft_offset_valid : ft_offset_low;
			g_regular_tempera_offset = ft_offset_valid;
			g_high_tempera_offset = (ft_offset_high == (int32_t)0xFFFFFFFF) ? ft_offset_valid : ft_offset_high;

		
#if (MODULE_VER)
			/*模组形态 非深睡唤醒：需要给RC一个合适的初始状态！*/ 

			// 从flash中读取最适合当前温度的RC参数
			if(temp_ft <= CRITICAL_LOW_TEMPERATURE)
			{
				capSelNV = g_32K_table->low_capsel;
				sdmNV = g_32K_table->low_sdm;
			}
			else if(temp_ft <= CRITICAL_HIGH_TEMPERATURE)
			{
				capSelNV = g_32K_table->mid_capsel;
				sdmNV = g_32K_table->mid_sdm;
			}
			else
			{
				capSelNV = g_32K_table->high_capsel;
				sdmNV = g_32K_table->high_sdm;
			}

			// 重置Aon LUT：模组形态所有温度挡位全部重新校准
			Reset_Aon_LUT();

			// 设置RC参数，确保RC初始状态的精度和稳定性
			HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;  //必须添加，否则会影响频率稳定性，但原因不明！
			RC32K_CALIB_NV_Save(capSelNV , sdmNV);
			REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);	// 取消强制参数传递

			g_last_reset_tick = Get_Tick();  //每次重置Aon LUT，更新记录时刻点，应对老化问题
			memset(g_32K_table, 0xFF, FLASH_32KCALI_TBL_LEN);

			// 设置主循环标记位：异常断电等场景无需考虑老化问题
			Set_Restore_LUT_Flag(0);
#else
			/*  open形态 非深睡唤醒： RC初始精度完全依赖golden ref和分段的ft_offset */

			// 使用golden ref + 分段ft_offset 填充硬件LUT
			Update_Aon_LUT_By_Offset(0,CRITICAL_LOW_TEMPERATURE,g_low_tempera_offset);
			Update_Aon_LUT_By_Offset(CRITICAL_LOW_TEMPERATURE + 1,CRITICAL_HIGH_TEMPERATURE,g_regular_tempera_offset);
			Update_Aon_LUT_By_Offset(CRITICAL_HIGH_TEMPERATURE + 1 ,90,g_high_tempera_offset);

			RC32K_CALIB_LOOKUP_TABLE_Get(temp_ft, &capSelNV , &sdmNV);

			HWREGB(AONPRCM_BASE + 0x83) &= 0xFE;  //必须添加，否则会影响频率稳定性，但原因不明！
			RC32K_CALIB_NV_Save(capSelNV , sdmNV);
			REG_Bus_Field_Set(AONPRCM_BASE + 0x82,	1,	0, 0x0);	// 取消强制参数传递
#endif


			NVIC_IntRegister(RC32K_IRQn, rc32k_int_handler, 2);
			NVIC_IntRegister(MCNT_IRQn, MCNT_Handler, 2);
			IntEnable(INT_MEASURECNT);

			// 使能RC自校准
			RC32k_Cali_Init(g_osc_cal);
		}
		else
		{
			xy_assert(0);
		}

		xy_assert(xy_ftl_write(CALIB_FREQ_BASE, (void *)g_32K_table, sizeof(Cali_Ftl_t)) == 1);
		xy_free(g_32K_table);

		HWREGB(AON_SYSCLK_CTRL2) |= 0x01;   //xtal26M force off
	}
	else 
	{		
		// 深睡唤醒
		if( HWREGB(BAK_MEM_32K_CLK_SRC) == RC_32K)
		{
#if (MODULE_VER)
			if(g_fast_startup_flag != AP_WAKEUP_FASTRECOVERY_AFTER)
			{
				g_32K_table = xy_malloc(sizeof(Cali_Ftl_t));
				xy_assert(xy_ftl_read(CALIB_FREQ_BASE,(void *)g_32K_table,sizeof(Cali_Ftl_t)) == 1);
				
				// 读取ft_offset作为各温度段offset的缺省值
				g_low_tempera_offset = g_32K_table->low_tempera_offset;
				g_regular_tempera_offset = g_32K_table->regular_tempera_offset;
				g_high_tempera_offset = g_32K_table->high_tempera_offset;
				
				g_osc_cal = g_32K_table->osc_cal;

				// RAM掉电，从flash中恢复上次reset Aon LUT的时间
				g_last_reset_tick = g_32K_table->last_reset_tick;
				
				//非快速恢复的深睡唤醒处于功耗考虑，无法重新校准全部温度，选择回填已校准的温度
				Set_Restore_LUT_Flag(1);

				xy_free(g_32K_table);
			}

			NVIC_IntRegister(RC32K_IRQn, rc32k_int_handler, 2);
			NVIC_IntRegister(MCNT_IRQn, MCNT_Handler, 2);
			IntEnable(INT_MEASURECNT);
#endif
		}
	}

	// 将32k晶振的真实频率并传递给CP
	g_32k_clock = update_32k_freq();			
	HWREG(BAK_MEM_RC32K_FREQ) = g_32k_clock;
}


