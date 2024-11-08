 #pragma once


#define PS_FAC_LEN 1536
#define RF_FAC_LEN 128

#define G_LRRC_SUPPORT_BAND_MAX_NUM 19
#define D_NVM_MAX_SIZE_IMEI 9
#define D_NVM_MAX_SIZE_IMEI_SV 10

#define NVM_MAX_ADD_SPECTRUM_EMISSION_MAX_NUM 5

#define NVM_MAX_DCN_ID_NUM 32
#define NVM_MAX_CANDIDATE_FREQ_NUM 32
#define NVM_MAX_OPERATOR_NUM 3
#define NVM_MAX_OPERATOR_PLMN_NUM 10
#define NVM_MAX_EPLMN_NUM 16

#define NVM_MAX_PRE_EARFCN_NUM G_LRRC_SUPPORT_BAND_MAX_NUM
#define NVM_MAX_PRE_BAND_NUM 5
#define NVM_MAX_LOCK_FREQ_NUM 20
#define NVM_MAX_SN_LEN 64

#define NAS_IMSI_LTH 9
#define NAS_AUTH_KEY_LTH   16
#define NAS_AUTH_OP_LTH    16
#define G_LRRC_PRE_EARFCN_INVALID 1111111

typedef struct
{
	unsigned char ucProfile0x0002SupportFlg;
	unsigned char ucProfile0x0003SupportFlg;
	unsigned char ucProfile0x0004SupportFlg;
	unsigned char ucProfile0x0006SupportFlg;
	unsigned char ucProfile0x0102SupportFlg;
	unsigned char ucProfile0x0103SupportFlg;
	unsigned char ucProfile0x0104SupportFlg;
} NVM_ROHC_PROFILE_STRU;

typedef enum
{
	LRRC_MAX_ROHC_SESSION_CS2 = 0,
	LRRC_MAX_ROHC_SESSION_CS4,
	LRRC_MAX_ROHC_SESSION_CS8,
	LRRC_MAX_ROHC_SESSION_CS12
} LRRC_MAX_ROHC_SESSION_VALUE;

typedef struct
{
	unsigned char ucSuppBand;
	unsigned char ucPowerClassNB20dBmR13Flg;
	unsigned char ucAddSpectrumEmissionNum;
	unsigned char aucAddSpectrumEmission[NVM_MAX_ADD_SPECTRUM_EMISSION_MAX_NUM];
} LRRC_SUPPORT_BAND_STRU;

typedef struct
{
	unsigned char ucSuppBand;
	unsigned char ucEnable;	 /* 0:not Valid 1:Valid */
	unsigned short usOffset; /*EndFreq = ulStartFreq + usOffset*/
	unsigned long ulStartFreq;
} LRRC_PRE_BAND_STRU;

typedef struct
{
	unsigned char ucApn[22];
	unsigned char auPlmnList[NVM_MAX_OPERATOR_PLMN_NUM][3]; /* Byte Order: MCC2 MCC1 | MNC3 MCC3 | MNC2 MNC1 */
	unsigned long aulPreEarfcn[NVM_MAX_PRE_EARFCN_NUM];
	LRRC_PRE_BAND_STRU aPreBand[NVM_MAX_PRE_BAND_NUM];
} LNB_NAS_OPERATOR_CFG_DATA_STUR;

typedef struct {
    unsigned char    aucImei[D_NVM_MAX_SIZE_IMEI]; /* IMEI */               //##8,74,85,2,1,51,69,147,9##&&
    unsigned char    ucReserved1;                                           //##0##
    unsigned char    aucImeisv[D_NVM_MAX_SIZE_IMEI_SV];/* IMEI(SV) */       //##9,67,85,2,1,51,69,147,121,240##&&
}T_ImeiInfo;

typedef struct
{
#define     D_AUTH_PROT_NONE            0
#define     D_AUTH_PROT_PAP             1
#define     D_AUTH_PROT_CHAP            2
    unsigned short                      ucUseFlg:1;                        //##0##
    unsigned short                      ucUsernameLen:5;                   //##0##
    unsigned short                      ucPasswordLen:5;                   //##0##
    unsigned short                      ucAuthProt:3;                      //##0##
    unsigned short                      ucPadding:2;                       //##0##
#define     D_PCO_AUTH_MAX_LEN          16 
    unsigned char                       aucUsername[D_PCO_AUTH_MAX_LEN];   //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
    unsigned char                       aucPassword[D_PCO_AUTH_MAX_LEN];   //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
}T_PCO_AUTH_INFO_STRU;

typedef struct
{
    unsigned char    ucUsed:1;                                              //##0##
    unsigned char    ucPdpType:3;                                           //##0##
    unsigned char    ucAtCid:4;                                             //##0##

    unsigned char    ucH_comp:1;                                            //##0##
    unsigned char    ucSecurePco:1;                                         //##0##
    unsigned char    ucQicsgpFlg:1;                                         //##0##,use to record weather cid is seted by AT+QCICSGP
    unsigned char    ucPadding6:5;
    unsigned char    ucPadding;

    unsigned char    ucNSLPI:1;                                             //##0##
    unsigned char    ucP_cid:1;                                             //##0##, cid index
    unsigned char    ucApnLen:6;                                            //##0##
    
#define FACTORY_USER_SET_APN_LEN    38
    unsigned char    ucApn[FACTORY_USER_SET_APN_LEN];                       //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
    T_PCO_AUTH_INFO_STRU tAuthInfo; //34 byte
}LNB_NAS_USER_SET_CID_INFO;

typedef struct
{
    unsigned char                      ucUserSetFlg:1;                      //##0##
    unsigned char                      ucPdpTypeFlag:1;                   //##0##
    unsigned char                      ucApnLen:6;                        //##0##
    unsigned char                      ucPdpType:3;                       //##0##
    //############################ QUECTEL ######################################
    unsigned char                      ucUsedFlg:1;                       //##0##
    unsigned char                      ucH_comp:1;                        //##0##
    unsigned char                      ucSecurePco:1;                     //##0##
    unsigned char                      ucNSLPI:1;                         //##0##
    //###########################################################################
    unsigned char                      ucPadding5:1;

    unsigned char    aucApn[FACTORY_USER_SET_APN_LEN];                       //##0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##
    T_PCO_AUTH_INFO_STRU                   tAuthInfo;
}LNB_NAS_USER_SET_DEFAULT_CID_INFO;

typedef struct
{
    unsigned char     ucBakFlg:1;
    unsigned char     ucERegMode:3;
    unsigned char     ucCsconN:1;
    unsigned char     ucCtzrReport:2;
    unsigned char     ucNipInfoN:1;

    unsigned char     ucCciotoptN:2;
    unsigned char     ucMsgService:1;
    unsigned char     ucNrnpdmRepValue:1;
    unsigned char     ucNgactrN:1;
    unsigned char     ucNptwEDrxMode:2;
    unsigned char     :1;
} LNB_NAS_FACTORY_SET_BAK_INFO;

typedef struct
{
	unsigned char ucAcsStratumRelType : 4;		//##0##
	unsigned char ucMultiToneSupportFlg : 1;	//##1##
	unsigned char ucMultiDRBsupportFlg : 1;		//##1##
	unsigned char ucMaxROHCContextSessions : 2; //##0##

	unsigned char ucDataInactMonR14SupFlg : 1;	  //##0##
	unsigned char ucRaiSupptR14SupFlg : 1;		  //##0##
	unsigned char ucMulCarrNprachR14SupFlg : 1;	  //##0##
	unsigned char ucTwoHarqProR14SupFlg : 1;	  //##0##
	unsigned char ucPwrClassNb14dBmR14SupFlg : 1; //##0##
	unsigned char ucMulCarrPagSupportFlg : 1;	  //##0##
	unsigned char ucMultiCarrierSupportFlg : 1;	  //##0##
	unsigned char ucMultiNSPmaxSupportFlg : 1;	  //##0##

	unsigned char ucLRrcPdcpParamFlg : 1;		 //##1##
	unsigned char ucProfile0x0002SupportFlg : 1; //##0##
	unsigned char ucProfile0x0003SupportFlg : 1; //##0##
	unsigned char ucProfile0x0004SupportFlg : 1; //##0##
	unsigned char ucProfile0x0006SupportFlg : 1; //##0##
	unsigned char ucProfile0x0102SupportFlg : 1; //##0##
	unsigned char ucProfile0x0103SupportFlg : 1; //##0##
	unsigned char ucProfile0x0104SupportFlg : 1; //##0##

	unsigned char ucUeCategory : 2;					  //##0##
	unsigned char ucInterferenceRandomisationR14 : 1; //##0##
	unsigned char ucEarlyContentionResolutionR14 : 1; //##0##

	unsigned char ucInterFreqRSTDmeasFlg : 1;	  //##1##
	unsigned char ucAddNeighbCellInfoListFlg : 1; //##1##
	unsigned char ucPrsIdFlg : 1;				  //##1##
	unsigned char ucTpSeparationViaMutingFlg : 1; //##0##

	unsigned char ucSupportBandNum; //##3##

	unsigned char ucAddPrsCfgFlg : 1;			//##0##
	unsigned char ucPrsBasedTbsFlg : 1;			//##1##
	unsigned char ucAddPathReportFlg : 1;		//##1##
	unsigned char ucDensePrsCfgFlg : 1;			//##0##
	unsigned char ucPrsOccGroupFlg : 1;			//##0##
	unsigned char ucPrsFreqHoppingFlg : 1;		//##0##
	unsigned char ucPeriodicalReportingFlg : 1; //##1##
	unsigned char ucMultiPrbNprsFlg : 1;		//##1##

	unsigned char ucIdleStateForMeasFlg : 1;	 //##1##
	unsigned char ucMaxSupptPrsBandwidthFlg : 1; //##0##
	unsigned char ucMaxSupptPrsBandwidth : 3;	 //##0##
	unsigned char ucMaxSupptPrsCfgFlg : 1;		 //##0##
	unsigned char ucMaxSupptPrsCfg : 2;			 //##0##1/2

    unsigned char    aucPadding[1];                                        //##0##

	LRRC_SUPPORT_BAND_STRU    aSupportBandInfo[G_LRRC_SUPPORT_BAND_MAX_NUM];

    unsigned char    ucMixedOperationModeR15SupFlg:1;
    unsigned char    ucWakeUpSignalR15SupFlg:1;
    unsigned char    ucRlcUmR15SupFlg:1; 
    unsigned char    ucSrSpsBsrR15SupFlg:1;
    unsigned char    ucSrWithHarqACKR15SupFlg:1;
    unsigned char    ucSrWithoutHarqACKR15SupFlg:1;
    unsigned char    ucNprachFormat2R15SupFlg:1;
    unsigned char    ucAdditionalTransmissionSIB1R15SupFlg:1;

    unsigned char    ucNpusch3dot75kHzScsTddR15SupFlg:1;
    unsigned char    ucCpEdt_SupFlg:1;
    unsigned char    ucUpEdt_SupFlg:1;
    unsigned char    ucCrtdcpRptIndFlg:1;
    unsigned char    ucWakeUpSignalMinGapEdrxR15SupEnum:2;
    unsigned char    ucBipEnableFlg:1;
    unsigned char    ucPadding1:1;

    unsigned char    aucPadding1[18];
}T_UeCapa;

typedef struct
{
	unsigned char ucATLockEarfcnFlg;
	unsigned char ucATLockCellIDFlg;
	unsigned short usATLockCellID;

	unsigned long ulATLockEarfcn;
} T_AtLockEarfcnCellInfo;
typedef struct
{
	unsigned char ucMsgService : 1;
	unsigned char ucCmmsN : 2;
	unsigned char ucScaLen : 4;
	unsigned char ucPadding : 1;
	unsigned char ucToSca;
#define D_SMS_SCA_SIZE_MAX 12
	unsigned char aucSca[D_SMS_SCA_SIZE_MAX];
} T_SMS_CFG_INFO;

typedef struct
{
	unsigned char ucNfCapabilitySupportFlg : 1;		 //##0##
	unsigned char ucS1UDataSupportFlg : 1;			 //##1##
	unsigned char ucERwOpdnSupportFlg : 1;			 //##0##
	unsigned char ucExtPeriodicTimersSupportFlg : 1; //##1##
	unsigned char ucEdrxSupportFlg : 1;				 //##1##
	unsigned char ucPsmSupportFlg : 1;				 //##1##
	unsigned char ucAUTV : 1;						 //##1##
	unsigned char ucCpBackOffSupportFlg : 1;		 //##0##

	unsigned char ucRestrictECSupportFlg : 1;	  //##0##
	unsigned char uc3gppsDataOffSupportFlg : 1;	  //##0##
	unsigned char ucRdsSupportFlg : 1;			  //##0##
	unsigned char ucApnRateControlSupportFlg : 1; //##1##
	unsigned char ucPsmEnableFlg : 1;			  //##1##&&
	unsigned char ucEdrxEnableFlg : 1;			  //##0##&&
	unsigned char ucAutoConnectFlg : 1;			  //##1##&&
	unsigned char ucL2THPFlag : 1;				  //##0##

	unsigned char ucActTimeBakFlg : 1;	//##0##
	unsigned char ucEdrxsValBakFlg : 1; //##0##
	unsigned char ucTraceLevel : 1;		//##0##
	unsigned char ucPowerTest : 1;		//##0##
	unsigned char aucBandScan : 2;		//##0##
	unsigned char ucFreqScanType : 2;	//##0##

	unsigned char ucPreferCiotOpt : 2;	//##1##
	unsigned char ucDataInactTimer : 5; //##0##
	unsigned char ucPSRptInfoFlg : 1;	//##0##

	unsigned char    ucReqPeriodiTauValue;                                 //##72##&&
	unsigned char    ucReqActTimeValue;                                    //##33##&&
	unsigned char    ucReqEdrxValue;                                       //##2##&&
	unsigned char    ucSinrOffset;                                         //##0##

	unsigned char    ucPowOffSleep:1;                                      //##0##
	unsigned char    ucUpCIoTSupportFlg:1;                                 //##0##
	unsigned char    ucSuppBandNum:6;                                      //##2##&&
	unsigned char    aucSupportBandList[G_LRRC_SUPPORT_BAND_MAX_NUM];      //##5,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0##&&

	LNB_NAS_OPERATOR_CFG_DATA_STUR      aOperatorCfgData[NVM_MAX_OPERATOR_NUM]; //0:china mobile,1:china unicom,2:china telecom
    LNB_NAS_USER_SET_CID_INFO tCidInfo0; //cid index = 0
    unsigned short      usOosTimerLen;                                     //##60##&&
    unsigned char    ucSecAlg;
    unsigned char    ucPsTestTauLen;                                       //##0##

    unsigned long    aulLockFreqList[NVM_MAX_LOCK_FREQ_NUM];          //##1200,1201,1575,1949,2400,2401,2525,2649,3450,3451,3625,3799,0,0,0,0,0,0,0,0##

	unsigned char    ucLockFreqNum;                                   //##12##
    unsigned char    ucUpReqFlag:1;//1                                                    //##0##&&
    unsigned char    ucCellReselForbitFlg:1;  /* 0: can cell reselection; 1: can not  */  //##0##
    unsigned char    ucUpRptFlag:1; //####&& /*0: Up Capa not support for Pulbic Net;  1: Up Report depend on Nv*/
    unsigned char    ucPsSpeedFlag:1; /*0:    1: update vrr=vrh when appear SnGap*/ 
    unsigned char    ucPsmReqFlag:1;
    unsigned char    ucWakeupInitUsimFlg:1;
    unsigned char    ucCciotoptN:2;                                        //##0##
    unsigned char    ucLockFreqListStat:1;
    unsigned char    ucNoSimSlpTimeLen:7;                                 //##127##&&
    unsigned char    ucPTWValue;                                          //##3##&&

    unsigned char    ucUlRateCtrlBufSize;
    unsigned char    ucERegMode:3;                                         //##0##&&
    unsigned char    ucNipInfoN:1;                                         //##0##
    unsigned char    ucCgerepMode:1;                                       //##1##
    unsigned char    ucCrtdcpRepValue:1;                                   //##0##
    unsigned char    ucEDrxMode:2;                                         //##0##

    unsigned char    ucCtzrReport:2;                                       //##3##
    unsigned char    ucStorageOperType:4;                                  //##0##
    unsigned char    ucPadding6:2;                                         //##1##

    unsigned char    ucNgactrN:1;                                          //##0##
    unsigned char    ucNptwEDrxMode:2;                                     //##0##
    unsigned char    ucSrvGapCtrlSupportFlg:1;                             //##1##
    unsigned char    ucNotSinglBand:1;                                     //##0##
    unsigned char    ucCidInfoForMtNetFlg:1;                               //##0##
    unsigned char    ucCellSrchInd:1;                                      //##0##
    unsigned char    ucSimwcFlg:1;                                         //##0##

    LNB_NAS_USER_SET_CID_INFO tCidInfo1; //cid index = 1

    T_AtLockEarfcnCellInfo tAtLockEarfcnCell;

    unsigned char    ucReqPeriodiTauValue_Bak;                             //##0##
    unsigned char    ucReqActTimeValue_Bak;                                //##0##
    unsigned char    ucReqEdrxValue_Bak;                                   //##0##
    unsigned char    ucPTWValue_Bak;                                       //##0##

    unsigned short   usSyncTimePeriod;                                     //##0##
    unsigned short   usBarringReleaseDelay:11;                             //##0##
    unsigned short   ucEpcoSupportFlg:1;                                   //##1##
    unsigned short   ucNonIpNoSmsFlg:1;                                    //##0##
    unsigned short   ucT3412ExtChgReportFlg:1;                             //##1##
    unsigned short   ucMtNetTestCloseFlg:1;                                //##0##
    unsigned short   ucAtHeaderSpaceFlg:1;                                 //##0##

    unsigned char    ucDiscLen;                                            //##0#
    unsigned char    ucCciotBakFlg:1;                                      //##0##
    unsigned char    ucUpCIoTSupportFlg_Bak:1;                             //##0##
    unsigned char    ucPreferCiotOpt_Bak:2;                                //##0##
    unsigned char    ucUiccDebugFlg:1;                                     //##0##
    unsigned char    ucNrnpdmRepValue:1;                                   //##0##
    unsigned char    ucIpv4DnsReqForbFlg:1;                                //##0##
    unsigned char    ucIpv6DnsReqForbFlg:1;                                //##0##
    unsigned char    ucCnecN;                                              //##0##

    unsigned char    ucCsconN:2;                                           //##0##
    unsigned char    ucNotStartPsFlg:1;                                    //##0##
    unsigned char    ucIpv6AddressFormat:1;                                //##0##
    unsigned char    ucIpv6SubnetNotation:1;                               //##0##
    unsigned char    ucIpv6LeadingZeros:1;                                 //##0##
    unsigned char    ucIpv6CompressZeros:1;                                //##0##
    unsigned char    ucNoNeedMeSIMCardInitFlg:1;                           //##0##


    
    T_SMS_CFG_INFO   tSmsCfgInfo;
    unsigned char  ucMaxUlPowLimit;                                        //unit: 1db
    unsigned char    ucUnicomPreferThreshold;                               //##0##

    LNB_NAS_USER_SET_DEFAULT_CID_INFO tInitDefCidInfo;                      //inital default cid info
    
    unsigned char    ucPsmStateRptFlg:1;                                    //##0##
    unsigned char    ucSimMaxSupportFlg:1;                                 //##0##
    unsigned char    ucCsodpcpDelayClose:1;                                 //##0##
    unsigned char    ucAutoApnFlg:1;                                        //##0##
    unsigned char    ucCombineAttachFlg:1;                                  //##0##
    unsigned char    ucNasSimPower:1;                                      //##0##
    unsigned char    ucCR0354Scrambling:1;                                 //##0##
    unsigned char    ucCR0859SIAvoid:1;                                    //##0##
    //############################# CUSTOM VER #######################################
    unsigned char    ucCustomVer15:1;                                       //##0##, WuHan Tianyu
    unsigned char    ucCustomVer09:1;                                        //##0##,quectel
    unsigned char    ucBC95Flg:1;                                           //##0##
    unsigned char    ucCustomVer16:1;                                         //##0##,simcom
    unsigned char    ucSimMaxFlg:1;                                         //##0##
    unsigned char    ucBC25Flg:1;                                           //##0##,BC25
    unsigned char    ucPadding5:2;                                          //##0##
    unsigned char    aucPadding6[2];                                        //##0##
    //################################################################################

    LNB_NAS_FACTORY_SET_BAK_INFO      tFactorySetBak;

    unsigned char    ucATCacheFlg:1;                                        //##0##
    unsigned char    ucIgrRelWaitTime:1;                                    //##0##
    unsigned char    ucUeSpecDrxCycle:3;                                    //##0##
    unsigned char    ucOldNasCodec:1;                                       //##0##
    unsigned char    ucPSFlowCtrlUrcFlg:1;                                  //##0##
    unsigned char    ucRaiRelExcepIgnoreFlg:1;                              //##0##,1 for quectel

    unsigned char    ucCopsMode:3;                                          //##0##
    unsigned char    ucNpbplmnsVal:1;                                       //##0##
    unsigned char    ucNbackoffVal:1;                                       //##0##
    unsigned char    ucNPlmnsOosIndFlg:1;                                   //##0##
    unsigned char    ucRefreshRestartPsFlg:1;                               //##0##
    unsigned char    ucPowOnATHandle:1;                                     //##0##,1:AT+NBAND,can use in poweroff
    
    unsigned short   usTDelayLen_S;                                         //##180##&&
    unsigned char    bXtalTrackDisable:1;                                   //whether to enable crystal aging tracking,.default 0(enable)
    unsigned char    bAgcGainJumpDisable:1;                                 //whether to enable agc gain jump when encounter strong interference(i.e. DC problem),default 0(enable)
    unsigned char    bMaxTxPowerEnhEnable:1;                                //whether to increase tx power additioally when confgiuring 23dBm 0:disable(default);1:enbale;
    unsigned char    ucMaxTxPowerRsrpOffset:5;                              //only enable txPowerEnh when  rsrp is larger than -126dBm+ucMaxTxPowerRsrpOffset; unit:1dB max=2^5-1=31
    unsigned char    ucSrchBandMaxNum;                                      //##0##
    unsigned char    ucSimulatorUiccFlg:1;
    unsigned char    ucSimulatorCardNum:4;
    unsigned char    ucSimulatorSubCardNum:2;
    unsigned char    ucRpmSupportFlag:1;                                    //##0##
    unsigned char    aucSimuuiccImsi[NAS_IMSI_LTH];
    unsigned char    aucSimuuiccAuthKey[NAS_AUTH_KEY_LTH];                  //##000102030405060708090A0B0C0D0E0F##
    unsigned char    aucSimuuiccAuthOp[NAS_AUTH_OP_LTH];                    //##34343434343434343434343434343434##
	unsigned char    uc8821OtaTest;
    unsigned char    ucQlockeranoffset;                                      //##0##
    unsigned char    ucCfun0RstFlg:1;                                        //##0##
    unsigned char    ucQspchscMode:1;                                       //##0##
    unsigned char    ucDwellRptFlg:1;                                        //##0## whether to report freq and sys result.for quectel
    unsigned char    ucHpplmnSrhClose:1;                                     //##0##
    unsigned char    ucFollow3gppProtFlg:1;                                  //##0##
    unsigned char    ucPadding7:3;                                           //##0##
	unsigned char    aucPadding3[409];/* in all :1536 */
}T_NasNvInfo;


typedef struct
{
    int                 magic_num;        //erase flash magic num,such as 5A5A5A5A
    T_UeCapa            tUeCapa;
    T_NasNvInfo         tNasNv;
}T_PsNvInfo;

enum Peri_Remap_Type{
	REMAP_SWD = 0,
	REMAP_CSP2,
	REMAP_UART,
	REMAP_CSP3,
	REMAP_I2C1,
	REMAP_SPI,
	REMAP_TMR1,
	REMAP_JTAG,
	REMAP_LED,
	
	REMAP_PERI_MAX
};

typedef struct
{
	int16_t dc_i;
	int16_t dc_q;
	int16_t iq_amp;
}rf_tx_dc_iq_t;

typedef struct
{
	rf_tx_dc_iq_t	rflb_tx_dc_iq;
	rf_tx_dc_iq_t	rfhb_tx_dc_iq;
	uint32_t reserved[2];
}rf_dc_iq_t;

typedef struct 
{
	rf_dc_iq_t dcIqNv_LLT;
	rf_dc_iq_t dcIqNv_HLT;
	rf_dc_iq_t dcIqNv_LMT;	
	rf_dc_iq_t dcIqNv_HMT;
	rf_dc_iq_t dcIqNv_LHT;
	rf_dc_iq_t dcIqNv_HHT;
	
	unsigned char	dcIqNvFlag_LLT;
	unsigned char	dcIqNvFlag_HLT;
	unsigned char	dcIqNvFlag_LMT;
	unsigned char	dcIqNvFlag_HMT;
	unsigned char	dcIqNvFlag_LHT;
	unsigned char	dcIqNvFlag_HHT;

	signed char txIqPhaseLB;
	signed char txIqPhaseHB;	
}rf_dciq_t;

typedef struct 
{
	uint8_t rxTIAPush_I;
	uint8_t rxTIAPull_I;
	uint8_t rxTIAPush_Q;
	uint8_t rxTIAPull_Q;
	uint8_t dcCurrentBase_I;
	uint8_t dcCurrentBase_Q;
}rx_dc_tia_state_t;

typedef struct
{
	rx_dc_tia_state_t rxDCOC_byLna[5];
}rf_dcoc_by_freq_t;

typedef struct
{
	rf_dcoc_by_freq_t rxDCOC_byFreq[6];
}rf_dcoc_by_tempera_t;

typedef struct
{
	rf_dcoc_by_tempera_t rxDCOC_byTempera[6];
	unsigned char	rxDCOCFlag_byTempera[6];
	uint8_t    rfPadding[2];
}rf_rx_dcoc_t;


typedef struct
{
	uint8_t hib_enable;	   // ##1## 未使用，ble_hibernate休眠,0:只能进lpm，该状态下广播，可唤醒NB；1：休眠，不广播，不能唤醒NB
	uint8_t pairing_mode;  // ##0## 配对模式 0:不加密;2：加密，需要手机输入验证码，支持MITM;(1:加密，用户不需要操作;3:SC属性加密，用户不需要操作;4：SC属性加密，需要手机输入验证码，支持MITM;)
	uint8_t key_mode;	   // ##0## 0:随机密钥；1:固定密钥，只有在ble_pairing_mode为2(或4)时生效
	uint8_t rf_power;	   // ##3## 蓝牙发射功率0:-20dB  1:-5dB  2:-3dB  3:0dB  4:3dB  5:5db  6:6dB  7:7dB  8:10dB  9:-30dB

	uint32_t passkey; 	   // ##123456##&& 配对验证码，只有在ble_pairing_mode为2(或4)时生效。密码仅可配置为六位数字。

	uint16_t mtusize;	   // ##512##蓝牙之间单包数据最大长度
	uint8_t freq_offset;   // ##23## 蓝牙频偏，由RFnv对应参数决定。
	uint8_t log_enable;	   // ##0## 是否通过蓝牙输出log。该功能只有在RELEASE模式可配。

	uint16_t interval_min; // ##20## interval min,蓝牙最小连接间隔,[10,4000]ms
	uint16_t interval_max; // ##20## interval max，最大连接间隔，[10,4000]ms
	uint16_t latency;	   // ##0## slave latency,从机可以在connection interval忽略的包个数,最大值30
	uint16_t conn_timeout; // ##200## 连接超时时间ms

	uint8_t ble_name[30];  // ##XY_3100E## 蓝牙设备名称，最大长度为29字节。初始蓝牙名称为"XY_3100E-"加上BLEMAC前3字节的hex形式。
	uint8_t ble_name_adv;  // ##0## 不可设置，当全局使用，ble设置广播后不能设置名称
	uint8_t padding1;

	uint16_t advt_min;     // ##160## 蓝牙广播周期最小值。32 - 16448
	uint16_t advt_max;     // ##160## 蓝牙广播周期最大值。32 - 16448

	uint8_t accessmode;    // ##0## 0:连接成功不进入透传模式，1:连接成功自动进入透传模式
    uint8_t connecturc;    // ##0## 0:连接成功没有URC上报，1:连接成功有URC上报
    uint8_t broadcast;     // ##0## 0:关闭广播，1：打开广播
	uint8_t padding2;

	uint8_t ble_mac[6];    // ##0## 蓝牙设备地址,由RFnv对应参数决定。
	uint8_t reserved[15];
} BLE_cfg_t;


typedef struct
{
    uint8_t  deepsleep_enable;      //##1##&& 芯片进入深睡开关
    uint8_t  lpm_standby_enable;    //##1## standby睡眠开关
    uint8_t  wfi_enable;            //##1## wfi睡眠开关
    uint8_t  off_debug;             //##1##&& 产线上需配为1。CP核的软件debug开关，值为0时，assert及看门狗异常不会自动重启

    uint8_t  deepsleep_delay;       //##1##&& 非RTC唤醒或有外部AT命令输入，系统延迟若干秒后方容许尝试进深睡。若设为0，则必须由用户输入"AT+WORKLOCK=0"方可以进入深睡
    uint8_t  deepsleep_threshold;   //##120## CP核进入深睡的空闲时长阈值，小于该值进入standby，大于等于该值进入深睡
    uint8_t  frame_cal;             //##1##&& 设为1表示依靠物理层帧信息进行RC32K的精校准，以保证实时世界时间的精度
    uint8_t  padding1;              //

    uint8_t  dump_mem_into_flash;   //##0##&& CP核软件死机时，保存现场到flash的FOTA备份区；OPENCPU产品该值不生效，需要AP核用户主动调用dump_into_to_flash
    uint8_t  open_log;              //##1##&& 按大模块进行log过滤。0:无log;1:输出所有log;2:phy不输出；3：phy和ps底层不输出；4：phy和ps全部不输出；5：仅USER_LOG+AP_CORE_LOG输出；6：仅AP核log输出；7：仅AP核明文输出
    uint8_t  log_size;              //##45## log使用AP ram的大小，单位为1024字节；其中12K给CP核log缓存使用
    uint8_t  fast_recovery_threshold;    //##120## 用户禁止修改！CP核快速恢复的时间阈值，保持与deepsleep_threshold相等。

    uint8_t  g_NPSMR_enable;        //##0##&& NPSMR上报开关，若打开，则深睡时上报+NPSMR:
    uint8_t  g_NITZ;                //##1##&& 世界时间的设置模式;1：由3gpp上报的+CTZEU:更新;0：依靠用户输入AT+CCLK=<yy/MM/dd,hh:mm:ss>[<+-zz>]来更新
    uint8_t  cmee_mode;             //##1## AT命令ERR错误上报模式； 0:\r\nERROR\r\n  1:\r\nCME ERROR:错误码\r\n  2:\r\nCME ERROR:ERROR_STRING\r\n
    uint8_t  echo_mode;             //##0## AT命令回显，0,关闭;1,打开
    
    uint8_t  fota_close;            //##0## FOTA开关，部分客户可能不容许执行FOTA升级
    uint8_t  close_drx_dsleep;      //##0## OPENCPU产品建议设为1，以关闭DRX/eDRX期间进深睡，防止频繁深睡唤醒造成的用户开发影响
    uint8_t  watchdog_enable;       //##1##&& CP核看门狗启动开关，AP核看门狗需要用户自行使用ap_watchdog.h接口实现
    uint8_t  log_rate;              //##0##&& CP核log口速率，默认为0。0:921600 1:9600 2:19200 3:38400 4:57600 5:115200 6:380400 7:460800

    uint8_t  padding2;              //
    uint8_t  sim_type;              //##1## 1,使用物理SIM卡；0，使用软SIM，暂不可用
    uint8_t  need_start_dm;         //##0## 公有云DM开关；若打开，会造成功耗和流量的大大增加
    uint8_t  cdp_register_mode;     //##0## CDP注册模式;0:由用户通过AT命令或API进行手工注册;1:系统上电后自动注册CDP

    uint16_t at_uart_rate;          //##4##&& AT串口波特率设置,单位粒度为2400bit(针对1200波特率，此粒度下不能表示，故使用3特指1200波特率);其中，低9位为设置的波特率；高7位为波特率自适应的结果；二者只能一种有效
    uint16_t at_recv_timeout;       //##500## 禁止修改！等待AT命令结束符的最大时长，每次收到串口数据后都会重新计时；不得超过1000ms,也不准设置为0

    uint8_t  at_txd_pin;            //##3##&&  禁止修改
    uint8_t  at_rxd_pin;            //##4##&&  禁止修改
    uint8_t  log_txd_pin;           //##18##&& 禁止修改
    uint8_t  log_rxd_pin;           //##19##&& 禁止修改

    uint8_t  led_pin;               //##5##&&  used for debug CP,0XFF is close LED debug
    uint8_t  resetctl;              //##0## 用于设置 wkup_en 引脚的唤醒、复位脉宽。默认值0，表示复位脉宽为5.12s，唤醒脉宽为160ms。详情见 wkup_en_init
    uint8_t  swd_swclk_pin;         //##22##&&  CP swd for JLINK
    uint8_t  swd_swdio_pin;         //##23##&&  CP swd for JLINK

    uint8_t  product_ver[28];       //##""##&& tail char must is '\0'
    uint8_t  modul_ver[20];         //##"XYS-XYM110"##&& module version,tail char must is '\0'
    uint8_t  hardver[20];           //##"XYM110_HW_V1.0"##&& hardware version,tail char must is '\0'
    uint8_t  versionExt[28];        //##"XYM110_SW_V0.3.7"##&& external version,tail char must is '\0'

    uint8_t  io_output_vol;         //##1## 二级boot中设置GPIO引脚高电平电压值。(0:1.8V；1:3V；2:使用硬件默认配置)
    uint8_t  standby_delay;         //##10##&& 单位:秒,串口波特率大于9600时, 串口发送AT命令会关闭standby并维持的时间，超过该时间后，重新打开standby
    uint16_t bakmem_threshold;      //##0## 单位:分钟。0:深睡8K BAKMEM强制下电(对标B0);0xFFFF:深睡8K BAKMEM保持供电(OpenCpu);其他:非深睡8K BAKMEM动态下电，仅模组且该值不应小于半小时，深睡时长超过此值时，bakmem区域回写flash后下电。

    uint16_t ra_timeout;            //##0## 等待ipv6 ra报文超时时间,取值[0-65535]
    uint16_t LimitedRemainingHeap;  //仅内存调试使用，记录上次工作时堆内存的开销

    uint8_t cldo_set_vol;           //##0## cldo电压设置，0：默认设置，为1.2V； 1：降低1个code，为1.1V； 2：降低2个code，为1.0V   
    uint8_t padding3;              
    uint16_t mtu;                   //##1500## 默认网卡MTU大小
	
    uint32_t rc32k_aging_period;    //##0## 单位为分钟：0表示芯片不老化，芯片不会偷偷唤醒；设置为非0，则芯片会后台主动唤醒以应对老化。唤醒的频次与本NV的时长相关，时间越短，后台唤醒余越频繁。（推荐为120分钟）
    uint8_t mem_debug;              //##0## 内存跟踪及死机调试信息开关，开启后会增加2K左右内存开销 
    uint8_t telecom_ctl;            //##0## 1表示CDP终端上电自动连接AEP平台
	uint8_t test;                   //##0## 内部测试开关，1：提供CP核主频；2：HTOL测试版本
	uint8_t rfSidoSetting_mg;		//##0## 美格海外版本sido配置NV，1：美格海外版本；0：正常版本

	/*以下4个字节专供BC25对标使用*/
	uint8_t qsclk_mode;             //##1## BC25睡眠模式，0：禁止进入深睡和Standby；1：可以进入深睡和Standby；2：只能进Standby
	uint8_t wakup_URC;              //##1## 进出深睡时URC上报的开关
	uint8_t sock_data_format;       //##0## 设置socket数据格式。bit0：发送数据的格式（0：字符串文本；1：十六进制）；bit1：接收数据的格式（0：字符串文本；1：十六进制）；bit2：已接收数据的输出格式（0：header\r\ndata；1：header,data)；bit3：在缓存模式下是否侠士可选长度参数，0表示不显示。
	uint8_t sock_async;             //##0## 是否为异步socket

	/*260Y对标专用*/
	uint8_t  off_dsevent;			//##0## 芯片进入深睡时是否关闭URC上报
	uint8_t  off_wakeupRXD;			//##0## 是否关闭AT串口深睡唤醒能力，一旦关闭只能通过外部唤醒引脚来触发唤醒	
	uint8_t padding10[2];

    int16_t FR_low_temp_threshold;   //##0##当前未使用
    int16_t FR_high_temp_threshold;  //##0##快速恢复高温阈值，超过此温度禁止快速恢复功能！ default为0，代表高温阈值为50摄氏度

    uint8_t  _32K_CLK_MODE;     //##2##,配置32k模式。0:暂未使用; 1:强制使用xtal32k; 2:强制使用RC32k
    uint8_t  adc_volrange;      //##0##,adc的量程。0:（0-1.5V),所有ADC通道都支持，精度好 1:(0-2.2V),特殊定制，通用客户无需关注；2:（0-VBAT）,仅AUX_ADC1/2支持，精度较差
    uint8_t  at_parity;        //##0##,AT口奇偶校验位，0，无校验;1 EVEN校验；2：ODD校验。此nv参数为特殊客户功能定制使用。
    uint8_t  off_check;        //##0##&& OTP和FT有效参数值检查，产线上必须设为0

    uint8_t padding11[4];

} softap_fac_nv_t;

//use 1824 bytes
typedef struct {
    T_PsNvInfo    tNvData; /*1536 Byte,must cache aligned*/
    rf_dciq_t     rf_dciq_nv;/*128 Byte,must cache aligned*/
	rf_rx_dcoc_t  rf_dcoc_nv;/* 1088 Byte*/
    softap_fac_nv_t  softap_fac_nv;   //begin from 0Xac0,platform factory nv,have used 160 bytes
}factory_nv_t;

typedef struct
{
	T_ImeiInfo tImei;
	unsigned char ucSnLen;
	unsigned char padding[3];
	unsigned char aucSN[NVM_MAX_SN_LEN];
	unsigned int         check_result;
    unsigned int         lockImeiSNFlag;
} T_PsImeiSnInfo;



#define POWER_CNT 68

typedef struct
{
	unsigned int txCaliFreq;
	char txCaliTempera;
	char txCaliVolt;
	char reserved[2];
	short bandPower[POWER_CNT];
} rf_tx_calibration_t;

typedef struct
{
	unsigned int txFlatFreq;
	char txCaliTempera;
	char txCaliVolt;
	char reserved[2];
	short txFlatLPower;
	short txFlatMPower;
	short txFlatHPower;
	short padding;
} rf_tx_flatten_t;

typedef struct
{
	unsigned int version;
	unsigned int nvFlag0;
	int freqErr;
	rf_tx_calibration_t txCaliLB;
	rf_tx_calibration_t txCaliHB;
	rf_tx_flatten_t txFlatLB[12];
	rf_tx_flatten_t txFlatHB[12];
	unsigned short txFactorLB;
	unsigned short txFactorHB;
	unsigned short txFactorLT;
	unsigned short txFactorHT;
	unsigned short txFactorVolt[8];
	unsigned int reserved[4];
	unsigned int nvFlag1;
	unsigned int tSensorValue;
} rf_mt_nv_t;

typedef enum
{
	RF_CALI_NV = 0,
	RF_PSIMEI_SN = 0x400,
	RF_GOLDMACHINE_NV = 0x800,
	RF_STATE_RFIT = 0xA00,
	RF_UPDATE = 0xFFF,
} rf_info_type;

typedef struct
{
	unsigned int msg_len;
	unsigned int msg_addr;
} rf_info_t;


