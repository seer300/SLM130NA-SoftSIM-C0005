#ifndef _RF_MT_H_
#define _RF_MT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "PhyLayerTask1.h"
#include "PhyLayerTask2.h"
#include "primitive.h"

#define RF_MANUAL_SET_REG_EN  	1
#define RF_MANUAL_TEMPERA_EN  	1
#define RF_MANUAL_VBAT_EN  		0
#define RF_MT_NV_VERSION		0x08
#define MAX_RF_CALI_NUMBER   10
#define MAX_RFRX_CALI_FREQCNT  8
#define MAX_RFTX_CALI_FREQCNT  12

#define LNB_MSG_HEADER_STRU         MSG_HEADER_STRU

#define RF_CALI_FDT_EN		1


typedef enum
{
	LL1_ACTIVATE_REQ = 0,
	LL1_RADIO_TEST_MODE_ENABLE,
	LL1_RADIO_TEST_NPDSCH_RX,
	LL1_RADIO_TEST_NPUSCH_TX,
	LL1_RADIO_TEST_RF_PRODUCTION_TEST_RX_TX,
	LL1_RADIO_TEST_SWITCH_FREQ,
	LL1_STOP_REQ,
	LL1_RADIO_TEST_SWITCH_TXPOWER,
}RADIOTEST_TEST_enum;
	
typedef struct LL1_RADIO_TEST_NPUSCH_TX
{
	uint32_t DL_Freq; 				//2140000000 # #uint 32
	uint32_t UL_Freq;				//UL Freq: 1920100000 # #uint 32
	uint32_t Dl_earfcn_t;			//: 300[Match with DL Freq] # #uint 32
	uint32_t dl_freq_adjustment_hz;	//:0 # #uint 32
	uint32_t ul_freq_adjustment_hz;	//:0 # #uint 32
	uint32_t options_bitfield;		//:2 # #uint 32
	uint32_t max_samples[3];		//:153600 # #uint 32
	uint32_t resync_interval;		//:0 # #uint 32
	uint32_t iterations;			//: 2 # #uint 32
	uint32_t period;				//:40 # #uint 32
	uint8_t num_sss_resyncs;		//:0 # #uint 8
	uint8_t padding1[3];				//N/A:000000 # #uint 24
	uint32_t lpm_duration;			//:200 # #uint 32
	uint32_t tx_duration;			//:0 # #uint 32
	uint32_t wait_after_sim_access;	//:0 # #uint 32
	uint8_t dl_band;				//:0 # #uint 8
	uint8_t ul_band;				//:0 # #uint 8
	uint8_t ucNpdcchRmax;
	uint8_t ucSchedSIB1;				//N/A:0000 # #uint 16
	uint32_t start_subframe;		//:0 # #uint 32
	uint16_t phy_cell_id;			//:0 # #uint 16
	uint16_t repetition;			//: 1(0:Continous锛?1:Single) # #uint 16
	uint16_t crnti;					//:0 # #uint 16
	uint16_t timing_adjustment_us;	//:0 # #uint 16
	uint16_t tbs_size_bytes;		//:11 # #uint 16
	uint8_t modulation_order;		//:2 # #uint 8
	uint8_t mcs;					//:0 # #uint 8
	uint8_t subcarrier;				//:0[0-11] : Generally no need to modify, default 0 # #uint 8
	uint8_t resource_assignment;	//:1 # #uint 8
	uint8_t redundancy_version;		//:0 # #uint 8
	uint8_t ucSchedULFlag;		    //N/A:00 # #uint 8
	uint16_t tx_power_dbm;			//:23 # #uint 16
	uint8_t subcarrier_spacing;		//:1 # #uint 8
	uint8_t num_prach_elem;			//:0 # #uint 8
	uint16_t temperature;			//:25 # #uint 16
	uint16_t voltage;				//:3600 # #uint 16
}LL1_RADIO_TEST_NPUSCH_TX_STRU;

typedef struct LL1_RADIO_TEST_TXPOWER
{
	uint8_t  subcarrier_spacing;
	uint8_t  subcarrier;
	uint16_t tx_power_dbm;			//:23 # #uint 16
}LL1_RADIO_TEST_TXPOWER_STRU;


typedef struct LL1_RADIO_TEST_NPDSCH_RX
{
	uint32_t DL_Freq; 				//2140000000 # #uint 32
	uint32_t UL_Freq;				//UL Freq: 1920100000 # #uint 32
	uint32_t Dl_earfcn_t;			//: 300[Match with DL Freq] # #uint 32
	uint32_t dl_freq_adjustment_hz;	//:0 # #uint 32
	uint32_t ul_freq_adjustment_hz;	//:0 # #uint 32
	uint32_t options_bitfield;		//:2 # #uint 32
	uint32_t max_samples[3];		//:153600 # #uint 32
	uint32_t resync_interval;		//:0 # #uint 32
	uint32_t iterations;			//: 2 # #uint 32
	uint32_t period;				//:40 # #uint 32
	uint8_t num_sss_resyncs;		//:0 # #uint 8
	uint8_t padding1[3];				//N/A:000000 # #uint 24
	uint32_t lpm_duration;			//:200 # #uint 32
	uint32_t tx_duration;			//:0 # #uint 32
	uint32_t wait_after_sim_access;	//:0 # #uint 32
	uint8_t dl_band;				//:0 # #uint 8
	uint8_t ul_band;				//:0 # #uint 8
	uint8_t padding2[2];			//N/A:0000 # #uint 16
	uint32_t start_subframe;		//:0 # #uint 32
	uint16_t phy_cell_id;			//:0 # #uint 16	
	uint16_t crnti;					//:0 # #uint 16
	uint16_t repetition_number;		//: 1(0:Continous锛?1:Single) # #uint 16
	uint16_t nrs_crs_power_offset;	//: 1(0:Continous锛?1:Single) # #uint 16
	uint16_t tbs_size_bytes;		//:11 # #uint 16
	uint8_t modulation_order;		//:2 # #uint 8
	uint8_t resource_assignment;	//:2 # #uint 8
	uint8_t mcs;					//:0 # #uint 8
	uint8_t deployment_mode;		//:0[0-11] : Generally no need to modify, default 0 # #uint 8
	uint8_t eutra_control_region_size;	//:1 # #uint 8
	uint8_t port_num;				//:1 # #uint 8
	uint8_t harq_combining;			//N/A:00 # #uint 8
	uint8_t SchedInfoSib1;			//:2 # #uint 8
	uint8_t interference_randomisation;	//:1 # #uint 8
}LL1_RADIO_TEST_NPDSCH_RX_STRU;

typedef struct
{
    LNB_MSG_HEADER_STRU                     MsgHeader;                                  /* message header                       */
    LL1_RADIO_TEST_NPUSCH_TX_STRU           stAtTxPara;
}LL1_RADIO_TEST_NPUSCH_TX_MSG_STRU;

typedef struct
{
    LNB_MSG_HEADER_STRU                     MsgHeader;                                  /* message header                       */
    LL1_RADIO_TEST_NPDSCH_RX_STRU           stAtRxPara;
}LL1_RADIO_TEST_NPDSCH_RX_MSG_STRU;

typedef struct
{
    LNB_MSG_HEADER_STRU                     MsgHeader;                                  /* message header                       */
}LL1_RADIO_TEST_STOP_MSG_STRU;

typedef struct
{
    LNB_MSG_HEADER_STRU                     MsgHeader;                                  /* message header                       */
    LL1_RADIO_TEST_TXPOWER_STRU             stTxpowerPara;
}LL1_RADIO_TEST_TXPOWER_MSG_STRU;

typedef struct {
	unsigned int rf_mt_mode;
#if RF_MANUAL_SET_REG_EN == 1
	List_t rfTxRegSettingList;
	List_t rfRxRegSettingList;
#endif
#if RF_MANUAL_TEMPERA_EN == 1
	List_t rfTemperaFactorList;
#endif
#if RF_MANUAL_VBAT_EN == 1
	List_t rfvBatFactorList;
#endif
} rf_system_nv_t;
#define POWER_CNT 68
#define RF_MT_TX_NV_OLD			1
typedef struct
{
	unsigned int txCaliFreq;
	char	txCaliTempera;
	char	txCaliVolt;	
	unsigned short 	txCaliBaseFactor;
	short	bandPower[POWER_CNT];	
}rf_tx_calibration_t;
typedef struct
{
	unsigned int txFlatFreq;
	char	txCaliTempera;
	char	txCaliVolt;	
	char 	reserved[2];
	short	txFlatLPower;     	
	short	txFlatMPower;		
	short	txFlatHPower;		
	short   txFlatSIDO2p5;
}rf_tx_flatten_t;
typedef struct
{
	unsigned int rxCaliFreq;
	char	rxCaliTempera;
	char	rxCaliVolt;	
	char 	reserved[2];
	int8_t	rxGainOffset[MAX_RF_CALI_NUMBER][2];     	//Low Power
}rf_rx_calibration_t;
typedef struct
{
	unsigned int 	version;
	unsigned int 	nvFlag0;
	int 	freqErr;
	rf_tx_calibration_t txCaliLB[2];
	rf_tx_calibration_t txCaliHB[2];
	rf_tx_flatten_t		txFlatLB[MAX_RFTX_CALI_FREQCNT];  
	rf_tx_flatten_t		txFlatHB[MAX_RFTX_CALI_FREQCNT];	
	rf_rx_calibration_t rxCaliLB[MAX_RFRX_CALI_FREQCNT];	
	rf_rx_calibration_t rxCaliHB[MAX_RFRX_CALI_FREQCNT];
	unsigned int 	nvFlag1;
	unsigned short	tSensorCaliNV15;
	unsigned short	tSensorCaliNV22;
	unsigned short	rc32kLptsCaliNV;
	unsigned short	rc32kLptsCRC;	
}rf_cali_nv_t;
typedef struct
{
	volatile unsigned int 	rfModeFlag;   //0 test mode or normal mode; 1 calibration mode  
	volatile unsigned int  	rxTestTotal;
	volatile unsigned int  	rxTestPass;
	volatile unsigned char 	rxSyncFlag;
	volatile unsigned int  	zeroTsensorADC;
	volatile int 			ulpow;
	volatile unsigned int  	ulfreq;
	volatile unsigned int  	dvgaVal;
	volatile unsigned int  	reduceCurrentModeFlag;
	volatile unsigned int  	sido1p8Flag;	
}rf_mt_mode_t;
#if RF_MANUAL_SET_REG_EN == 1
typedef struct
{
	ListItem_t rfRegItem;
	unsigned int regAddr;
	unsigned int regEndBit;
	unsigned int regStartBit;
	unsigned int regData;
}rf_reg_setting_t;
#endif
#if RF_MANUAL_TEMPERA_EN == 1 || RF_MANUAL_VBAT_EN == 1
typedef struct
{
	ListItem_t rfFactorItem;;
	unsigned int startFreq;
	unsigned int endFreq;
	uint16_t arrayFactor[15];
}rf_factor_t;
#endif

typedef struct
{
	uint16_t     msgID;
	uint16_t     arg;

} xRfMtMsg_T;

typedef void (*at_rf_func_t )( char* result,char **rsp_cmd);
typedef struct {
	int32_t code;
	at_rf_func_t proc;
} AT_RF_Fun;


typedef struct
{
        uint32_t FreqKhz;
        int32_t ExpPwr;
        uint32_t Dvga;
}Apc_Cali_Trig_Def;

typedef struct
{
	int32_t ExpPwr;
	uint32_t Dvga;
	int32_t nPointNum;
	int32_t GainStep;
	int32_t bTempEn;

}Apc_Cali_Sec_Def;

typedef struct
{
	uint32_t FreqKhz;
	uint32_t GainIndexNum;
	uint32_t GainIndex[0];
}Agc_Cali_Item_Def;

typedef struct
{
	uint32_t FreqKhz;
	int32_t CaliCount;
	Apc_Cali_Sec_Def Sec[0];
}Apc_Cali_Arfcn_Def;


#define FDT_PAYLOAD_MAX_LENGTH  (1024)
#define FDT_TSENSOR_QUE_MAX_LENGTH  (32)

typedef struct
{
		int state;
		int8_t  tsensor_que[FDT_TSENSOR_QUE_MAX_LENGTH];
		int32_t  tsensor_que_size;

        int32_t freq_num;
        int32_t tx_power_num;
        Apc_Cali_Trig_Def* apc_triger;
        Apc_Cali_Sec_Def* apc_sec;
        uint32_t     payload_idx;
        uint32_t     payload_size;

       	uint32_t payload[0];
}FDT_APC_Control_t;

typedef struct
{
		int state;
        int32_t freq_num;
		volatile int32_t rxFDTAgcFlag;
		
		int16_t  rxCaliRslt[200];
		int32_t  rxCaliRsltIdx;
        Apc_Cali_Trig_Def* agc_triger;
        Agc_Cali_Item_Def* agc_item;
        uint32_t     payload_idx;
        uint32_t     payload_size;

       	uint32_t payload[0];
}FDT_AGC_Control_t;

typedef enum{
	RF_MT_TASK_TX_START,
	RF_MT_TASK_TX_STOP,
	RF_MT_MEAS_TEMPERATURE,		
	RF_MT_WAKEUP,
	RF_MT_TASK_RX_START,
	RF_MT_TASK_RX_STOP,
}RF_MT_TASK_EVT_T;


#define FAPC_TX_START_BASE (23040UL)   //1ms later after at cmd recv
#define FAPC_TX_PERIORD   (23040UL*4)  //4ms @div4 from pclk
#define FAPC_TX_DURATION  (23040*3UL)  //2ms+ @div4 from pclk
#define FAPC_TX_TEMP_MEAS_OFT   (23040*2)  //4ms @div4 from pclk

#define FAGC_RX_START_BASE (23040UL)   //1ms later after at cmd recv
#define FAGC_RX_PERIORD   (23040UL*12)  //4ms @div4 from pclk
#define FAGC_RX_DURATION  (23040*11UL)  //2ms+ @div4 from pclk


#define EVENTS_LIST_SIZE_SHIT (2)
#define EVENTS_LIST_SIZE (1<<(EVENTS_LIST_SIZE_SHIT))
#define EVENTS_LIST_FUL_MASK  ((EVENTS_LIST_SIZE)-1)

typedef struct {
	struct{
		uint32_t event;
		uint32_t param;
		uint32_t clock;
	} events[EVENTS_LIST_SIZE];
	int16_t wp;
	int16_t rp;
}T4RfMtEvt_T;

typedef enum FDT_MODE{
	FDT_STOP=0,
	FDT_APC=1,
	FDT_AGC=2,

}FDT_MODE_u;

typedef enum FDT_STATE{
	FDT_APC_TRIGGER,
	FDT_APC_UPDATE_DVGA,
	FDT_APC_UPDATE_FREQ,
	FDT_APC_UPDATE_POWER,
	FDT_AGC_TRIGGER,
	FDT_AGC_UPDATE_FREQ,
	FDT_AGC_UPDATE_INDEX,
	FDT_STANDBY,
}FDT_STATE_u;

#if RF_MANUAL_TEMPERA_EN == 1
typedef struct
{
	uint32_t startFreq;
	uint32_t endFreq;
	uint16_t tempFactor[14];
}tempera_factor_t;

typedef struct
{
	uint32_t factNum;
	tempera_factor_t tempFactor[10];
	char reserved[16];
	uint32_t factorCRC;
}rf_comp_fact_t;
#endif

#if RF_MANUAL_SET_REG_EN == 1
typedef struct
{
	unsigned int regAddr;
	unsigned int regEndBit;
	unsigned int regStartBit;
	unsigned int regData;
}rf_reg_set_t;

typedef struct
{
	uint32_t regSettingNum;
	rf_reg_set_t regSetting[15];
	char reserved[8];
	uint32_t regSettingCRC;
}rf_regset_t;
#endif



#define RF_TX_VT_COMP_FACTOR_LEN  0x180

typedef enum
{
    RF_CALI_NV = 0,
    RF_PSIMEISN = 0x600,
    RF_TEMPERA = 0x680,
    //RF_VBAT = 0x800,    
    RF_TX_REG = 0x800,
    RF_RX_REG = 0x900,
    RF_GOLDMACHINE_NV = 0xA00,
    RF_NV_VALID = 0xE88,
    RF_BLELO = 0xE90,
    RF_BLEMAC = 0xEA0,
	RF_SDK_VER = 0xEC0,
    RF_STATE_RFIT = 0xF00,
    RF_UPDATE = 0xFA0,
}rf_info_type;

typedef struct
{
	unsigned int 	msg_len;
	unsigned int 	msg_addr;
}rf_info_t;


#define xy_zalloc(ulSize)   	    XY_ZALLOC(ulSize)

extern rf_cali_nv_t 	*ptrRfCaliNV;
extern rf_system_nv_t 	*ptrRfSysNV;

extern int32_t gTempera;
extern rf_mt_mode_t g_rf_mt_mode;

void psNvImeiSnSave();
extern uint8_t RF_isManufactryMode(void)  __RAM_FUNC;


#ifdef __cplusplus
}
#endif


#endif

