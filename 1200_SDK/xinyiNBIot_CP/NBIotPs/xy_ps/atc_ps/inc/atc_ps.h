#ifndef _ATC_PS_H_
#define _ATC_PS_H_

#ifdef SINGLE_CORE  //2100

#else
#include "xy_utils.h"
#include "xy_nbiot_msg_define.h"
#include "xy_nbiot_module_define.h"
#endif
#include "atc_ps_def.h"
#include "xy_atc_interface.h"
#include "atc_ps_stru.h"

extern const ST_ATC_AP_STR_TABLE              ATC_NConfig_Table[D_ATC_NCONFIG_MAX];
extern const ST_ATC_COMMAND_ANAL_TABLE        ATC_Plus_CommandTable[];
extern const ST_ATC_COMMAND_ANAL_TABLE        ATC_Symbol_CommandTable[];
extern const ST_ATC_COMMAND_ANAL_TABLE        ATC_Single_CommandTable[];
extern const ST_ATC_FAC_TABLE                 ATC_Fac_Table[D_ATC_FAC_NUM];
extern const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE  PCmeErrorTextTbl[];
extern const ST_ATC_EVENT_TABLE               ATC_Event_Table[];
extern const ST_ATC_STR_TABLE                 ATC_NUESTATS_Table[ATC_NUESTATS_MAX];
extern const ST_ATC_STR_TABLE                 ATC_PdpType_Table[9];
extern const unsigned long                    ATC_AP_PsRegEventIdMapTable[D_ATC_USER_REG_EVENT_TBL_SIZE][2];

extern const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE  PEmmCauseTextTbl[D_ATC_EMM_CAUSE_TBL_SIZE];
extern const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE  PEsmCauseTextTbl[D_ATC_ESM_CAUSE_TBL_SIZE];
extern const unsigned char                    ATC_PinCodeTbl[16][14];

extern const ST_ATC_AP_CMD_PROC_TABLE         AtcAp_CmdProcTable[D_ATC_CMD_PROC_TBL_SIZE];
extern const ST_ATC_AP_MSG_PROC_TABLE         AtcAp_DataIndProcTable[D_ATC_DATAIND_PROC_TBL_SIZE];

extern ST_ATC_AP_INFO                         g_AtcApInfo;
extern volatile unsigned long*                g_pRegEventId;
extern osMutexId_t                            g_pRegEventId_Mutex;

extern factory_nv_t*                          g_factory_nv;

#define D_ATC_CME_ERROR_TBL_SIZE               92

extern void AtcApFreeWithMemClear(void **pMem);
extern void* AtcApMallocWithTrack(unsigned long ulSize,char *file,unsigned long line);

#ifdef SINGLE_CORE  //2100
#define  AtcAp_Malloc(ulSize)                                  AtcApMallocWithTrack(ulSize,__FILE__,__LINE__)
#define  AtcAp_Free(pMem)                                      AtcApFreeWithMemClear((void**)&pMem)
#define  AtcAp_MemCpy(pDest,pSrcr,ulrLen)                      memcpy(pDest, pSrcr, ulrLen)
#define  AtcAp_MemSet(pBuffer,ucData,ulBufferLen)              memset(pBuffer,ucData,ulBufferLen)
#else
#define  ps_zalloc(ulSize)   	                               XY_ZALLOC(ulSize)
#define  AtcAp_Malloc(ulSize)                                  ps_zalloc(ulSize)
#define  AtcAp_Free(pMem)                                      AtcApFreeWithMemClear(&pMem)
#define  AtcAp_MemCpy(pDest,pSrcr,ulrLen)                      memcpy(pDest,pSrcr,ulrLen)
#define  AtcAp_MemSet(pBuffer,ucData,ulBufferLen)              memset(pBuffer,ucData,ulBufferLen)
#endif

#define Unused_para_ATCAP(param) (void)param
#define PS_TRACE_ATCAP
#ifdef SINGLE_CORE
#define AtcAp_PrintLog(dyn_id, src_id, lev, fmt, ...)          PrintLog(dyn_id, src_id,lev, fmt,##__VA_ARGS__)
#else
#define AtcAp_PrintLog(dyn_id, src_id, lev, fmt, ...)          xy_printf(ATC_AP,lev,fmt, ##__VA_ARGS__)
#endif

/***************** atc_ap_main.c **********************************************/
extern void AtcAp_LinkList_AddNode(ST_ATC_AP_LINK_NODE** ppHead, ST_ATC_AP_LINK_NODE** ppLast, ST_ATC_AP_LINK_NODE* pNode);
extern void AtcAp_LinkList_RemoveNode(ST_ATC_AP_LINK_NODE** ppHead, ST_ATC_AP_LINK_NODE** ppLast, ST_ATC_AP_LINK_NODE* pNode);
extern void AtcAp_DelAppInterfaceResult(ST_ATC_AP_APP_INTERFACE_NODE* pNode);
extern void AtcAp_registerPSEventCallback(unsigned long eventId, xy_psEventCallback_t callback);
extern ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_AddAppInterfaceInfo(unsigned long ulSemaphoreId);
extern ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_GetAppInterfaceInfo_BySeqNum(unsigned char ucSeqNum);
extern ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_GetAppInterfaceInfo_BySema(unsigned long ulSemaphore);
extern ST_ATC_AP_PS_EVENT_REGISTER_INFO* AtcAp_GetPsRegEventInfo(unsigned long eventId);
extern void AtcAp_AppInterfaceInfo_CmdRstProc(unsigned char ucSeqNum, unsigned ucResult);
extern void AtcAp_SendMsg2AtcAp(void* pMsg, ST_ATC_AP_MSG_INFO* pMsgInfo);
/*************** atc_ap_msg_proc.c  start *************************************/
extern unsigned char AtcAp_NormalMsgDistribute(unsigned char *pucRcvMsg);
extern void AtcAp_CascadeAtProc_NextAt();
extern void AtcAp_AtcDataReqListProc();
extern void  AtcAp_DataIndProc_RegEventInd(unsigned char* pRecvMsg, unsigned short usLen);
/*************** atc_ap_msg_proc.c  end *************************************/


/*************** atc_ap_cmd_proc.c start *************************************/
extern unsigned char AtcAp_CGSN_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CEREG_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CIMI_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGATT_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGDCONT_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CFUN_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMEE_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMEE_R_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMEE_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CLAC_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CLAC_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CESQ_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGACT_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CSODCP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CRTDCP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CEDRXS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CPSMS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGTFT_T_LNB_Process (unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGEQOS_T_LNB_Process (unsigned char *pEventBuffer);
#ifdef NBIOT_SMS_FEATURE
extern unsigned char AtcAp_CSMS_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMGF_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CSCA_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMGS_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CNMA_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMGC_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CMMS_T_Process(unsigned char *pEventBuffer);
#endif
extern unsigned char AtcAp_CSIM_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CCHC_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CCHO_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGLA_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CRSM_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CSCON_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGEREP_T_Process(unsigned char*pEventBuffer);
extern unsigned char AtcAp_CCIOTOPT_T_Process(unsigned char*pEventBuffer);
extern unsigned char AtcAp_CEDRXRDP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CTZR_T_LNB_Process(unsigned char *pEventBuffer);
#if VER_BC25
extern unsigned char AtcAp_CPIN_R_Process(unsigned char *pEventBuffer);
#endif
extern unsigned char AtcAp_CPIN_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CLCK_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CPWD_T_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NUESTATS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NEARFCN_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NCCID_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NL2THP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CSQ_T_LNB_Process(unsigned char *pEventBuffer);
#ifdef LCS_MOLR_ENABLE
extern unsigned char AtcAp_CMOLR_T_LNB_Process(unsigned char *pEventBuffer);
#endif
extern unsigned char AtcAp_CEER_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CIPCA_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGAUTH_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CNMPSD_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CPINR_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NPIN_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NPTWEDRXS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NTSETID_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NCIDSTATUS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NGACTR_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NIPINFO_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NQPODCP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NSNPD_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NQPNPD_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CNEC_T_LNB_Process(unsigned char* pEventBuffer);
extern unsigned char AtcAp_NRNPDM_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NCPCDPR_T_LNB_Process(unsigned char* pEventBuffer);
extern unsigned char AtcAp_CEID_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NCONFIG_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_MNBIOTEVENT_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_CGPIAF_T_LNB_Process(unsigned char *pEventBuffer);
#if SIMMAX_SUPPORT
extern unsigned char AtcAp_CUPREFERTH_T_LNB_Process(unsigned char *pEventBuffer);
#endif
extern unsigned char AtcAp_NPLMNS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NLOCKF_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_ZICCID_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QCGDEFCONT_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QENG_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QCFG_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NUICC_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NUICC_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NPBPLMNS_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_NBACKOFF_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QLOCKF_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QCSEARFCN_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QICSGP_T_LNB_Process(unsigned char *pEventBuffer);
extern unsigned char AtcAp_QSPCHSC_T_LNB_Process(unsigned char *pEventBuffer);

extern void AtcAp_MsgProc_AT_CMD_RST(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGSN_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEREG_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGATT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CIMI_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGDCONT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CFUN_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CESQ_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSQ_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGPADDR_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGPADDR_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGACT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CRTDCP_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_SIMST_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEDRXS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CPSMS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGAPNRC_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGAPNRC_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSCON_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NL2THP_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NUESTATS_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NEARFCN_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NBAND_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NBAND_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCONFIG_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCONFIG_T_Cnf(unsigned char *pRecvMsg);
extern void AtcAp_MsgProc_NSET_R_Cnf(unsigned char* pRecvMsg);
#ifdef LCS_MOLR_ENABLE
extern void AtcAp_MsgProc_CMOLR_R_Cnf(unsigned char* pRecvMsg);
#endif
#ifdef ESM_DEDICATED_EPS_BEARER
extern void AtcAp_MsgProc_CGDSCONT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGDSCONT_T_Cnf(unsigned char* pRecvMsg);
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
extern void AtcAp_MsgProc_CGTFT_R_Cnf(unsigned char* pRecvMsg);
#endif
extern void AtcAp_MsgProc_CGEQOS_R_Cnf(unsigned char* pRecvMsg);
#ifdef ESM_EPS_BEARER_MODIFY
extern void AtcAp_MsgProc_CGCMOD_T_Cnf(unsigned char* pRecvMsg);
#endif
extern void AtcAp_MsgProc_COPS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_COPS_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGEREP_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CCIOTOPT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEDRXRDP_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGEQOSRDP_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGEQOSRDP_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CTZR_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGCONTRDP_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGCONTRDP_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEER_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CIPCA_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGAUTH_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPOWERCLASS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPOWERCLASS_T_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPTWEDRXS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCIDSTATUS_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCIDSTATUS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NGACTR_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPOPB_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NIPINFO_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NQPODCP_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NQPNPD_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CNEC_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NRNPDM_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCPCDPR_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEID_Cnf(unsigned char* pRecvMsg);
#ifdef NBIOT_SMS_FEATURE
extern void AtcAp_MsgProc_CSMS_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSMS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMGC_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMT_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMGF_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSCA_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMMS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMGS_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_SMS_PDU_Ind(unsigned char* pRecvMsg);
#endif
extern void AtcAp_MsgProc_SIMST_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPIN_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CPIN_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CLCK_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_PINSTATUS_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CPINR_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CRTDCP_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGAPNRC_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGEV_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEREG_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSCON_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPTWEDRXP_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CEDRXP_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CCIOTOPTI_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_L2_THP_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NCCID_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_XYIPDNS_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_MALLOC_ADDR_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_IPSN_Ind(unsigned char* pRecvMsg);
#ifdef LCS_MOLR_ENABLE
extern void AtcAp_MsgProc_CMOLRE_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CMOLRG_Ind(unsigned char* pRecvMsg);
#endif
extern void AtcAp_MsgProc_PDNIPADDR_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NGACTR_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_OPELIST_SRCH_CNF(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSIM_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGLA_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CCHO_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CCHC_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CRSM_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_LOCALTIMEINFO_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NoCarrier_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CELLSRCH_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_PSINFO_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CSODCPR_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NSNPDR_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NIPINFO_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CNEC_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NRNPDM_Ind(unsigned char ucNrnpdmRepValue, unsigned char ucAtCid,unsigned short usLen,unsigned char* pucReportData);
extern void AtcAp_MsgProc_MNBIOTEVENT_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_CGPIAF_R_Cnf(unsigned char* pRecvMsg);
#if SIMMAX_SUPPORT
extern void AtcAp_MsgProc_CUPREFERTH_R_Cnf(unsigned char* pRecvMsg);
#endif
extern void AtcAp_MsgProc_NPLMNS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NLOCKF_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_ZICCID_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_ZCELLINFO_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QCGDEFCONT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QBAND_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QBAND_T_Cnf(unsigned char *pRecvMsg);
extern void AtcAp_MsgProc_QCCID_Cnf(unsigned char *pRecvMsg);
extern void AtcAp_MsgProc_QENG_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QCFG_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_MNBIOTEVENT_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NSIMWC_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QNIDD_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QNIDD_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_XYCELLS_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_PRESETFREQ_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPBPLMNS_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPBPLMNS_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NBACKOFF_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_SIMUUICC_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NBACKOFF_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_NPLMNS_OOS_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QLOCKF_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QICSGP_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_QSPCHSC_R_Cnf(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_SimDataDownload_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_FREQ_RSSI_Ind(unsigned char* pRecvMsg);
extern void AtcAp_MsgProc_SYSINFO_FAIL_Ind(unsigned char* pRecvMsg);

/*************** atc_ap_cmd_proc.c  end *************************************/


/*****************************************************************/
/************************** Atc_analysis *************************/
/*****************************************************************/
extern  unsigned char ATC_Command_Analysis(unsigned char* pAtcDataReq, unsigned char *pEventBuffer);

/*****************************************************************/
/************************** Atc_cmd_basic ************************/
/*****************************************************************/
//extern  unsigned char ATC_CGMI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CGMR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGSN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CEREG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGATT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CIMI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGDCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CFUN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CMEE_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CLAC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CESQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CSQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGPADDR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CGMM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CGDATA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CSODCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CRTDCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGAPNRC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CRC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CMUX_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_S3_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_S4_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_S5_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_E_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_V_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_F_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_SIMST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CEDRXS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CPSMS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CSCON_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CCIOTOPT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CEDRXRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGEQOSRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CTZR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGCONTRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_NL2THP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_NSET_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#ifdef LPP_MODE_ENABLE
extern  unsigned char ATC_CMOLR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#endif

/*****************************************************************/
/************************** Atc_cmd_other ************************/
/*****************************************************************/
extern  unsigned char ATC_CGDSCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGACT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGTFT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGEQOS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#ifdef ESM_EPS_BEARER_MODIFY
extern  unsigned char ATC_CGCMOD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#endif
extern  unsigned char ATC_CGEREP_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CPWD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CPIN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CLCK_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);

#ifdef NBIOT_SMS_FEATURE
/*****************************************************************/
/************************** Atc_cmd_sms **************************/
/*****************************************************************/
extern  unsigned char ATC_CSMS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CMGF_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CSCA_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern  unsigned char ATC_CNMI_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CMGS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CMGC_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CNMA_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CMMS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#endif
extern unsigned char ATC_COPS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);

//extern unsigned char ATC_NRB_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NUESTATS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NCONFIG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NPSMR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NMGS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NMGR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
///extern unsigned char ATC_NQMGS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NQMGR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_QLWUDATAEX_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NCDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_RAI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_NFPLMN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);

//extern unsigned char ATC_NATSPEED_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NSOCR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NSOST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//extern unsigned char ATC_NPING_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//shao add for USAT
/*****************************************************************/
/************************** Atc_cmd_usat**************************/
/*****************************************************************/
extern  unsigned char ATC_CSIM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CCHO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CCHC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CGLA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_CRSM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
//
extern unsigned char ATC_CEER_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CIPCA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CGAUTH_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CNMPSD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CPINR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NPOWERCLASS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NPTWEDRXS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NPIN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NTSETID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NCIDSTATUS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NGACTR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NPOPB_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NIPINFO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NQPODCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NSNPD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NQPNPD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CNEC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NRNPDM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NCPCDPR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CEID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_MNBIOTEVENT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CGPIAF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#if SIMMAX_SUPPORT
extern unsigned char ATC_CUPREFER_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_CUPREFERTH_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
#endif
extern unsigned char ATC_NPLMNS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NLOCKF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_ZICCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_ZCELLINFO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QCGDEFCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QENG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QCFG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NSIMWC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NUICC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_PSTEST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QNIDD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_XYCELLS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_PRESETFREQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NPBPLMNS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_NBACKOFF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern  unsigned char ATC_SIMUUICC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QLOCKF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_W_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_W0_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QICSGP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
extern unsigned char ATC_QSPCHSC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer);
/*************************** atc_ap_com.c start ************************************/
extern void ATC_SendApDataReq(unsigned char ucExternalFlg, unsigned long ulAppSemaId, unsigned short usDataLen, unsigned char* pucData, unsigned char ucTaskSource);
extern void AtcAp_SendOkRsp();
extern void AtcAp_SendCmeeErr(unsigned short usErrCode);
extern void AtcAp_SendErrorRsp();
extern void AtcAp_SendCmsErr(unsigned short usErrCode);
extern void AtcAp_SendDataInd(void);
extern void AtcAp_WriteStrPara_M(unsigned int uiFlg, unsigned char *pucPara );
extern void AtcAp_WriteStrPara_M_NoQuotation(unsigned int uiFlg, unsigned char *pucPara );
extern void AtcAp_WriteIntPara_M(unsigned int uiFlg, unsigned int uiPara, unsigned char *pucAtcRspBuf);
extern void AtcAp_WriteIntPara(unsigned int uiFlg, unsigned int uiPara );
extern void AtcAp_WriteHexPara_M(unsigned int uiFlg, unsigned int uiPara, unsigned char* pRespBuff, unsigned char ucHexLen);
extern void AtcAp_Write4BitData(unsigned char ucData);
extern void AtcAp_WriteStrPara(unsigned          int uiFlg, unsigned char *pucPara );
extern void AtcAp_OutputAddr(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_OutputAddr_NoQuotation(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_FreeEventBuffer(unsigned char* pCmdEvent);
extern int AtcAp_SndDataReqToPs(unsigned char *pCodeStream, unsigned short usCodeStreamLen);
extern int AtcAp_SndDataReqToShm(unsigned char *pCodeStream, unsigned short usCodeStreamLen);
extern void AtcAp_Encode_UN_ATC_CMD_EVENT(UN_ATC_CMD_EVENT *pCmdEvent, unsigned char** ppCodeStream, unsigned short* pusLen);
extern unsigned short AtcAp_StrPrintf_AtcRspBuf(const char* FormatBuffer,...);
extern unsigned char AtcAp_CurrEventChk_IsWaitSmsPdu();
extern int AtcAp_Strncmp(unsigned char *pStr1, unsigned char *pStr2);
extern unsigned short AtcAp_StrPrintf(unsigned char *EditBuffer, const unsigned char *FormatBuffer,...);
extern void AtcAp_CGCONTRDP_Print(EPS_CGCONTRDP_DYNAMIC_INFO *ptPdpDynamicInfo, unsigned char bPdpType, unsigned char *pucAtcRspBuf);
extern void AtcAp_CSCA_ConvertScaByte2Str(unsigned char* pucScaData, unsigned char ucScaLen, unsigned char* pScaStr);
extern void AtcAp_ConvertByte2BitStr(unsigned char ucVal, unsigned char len, unsigned char* pBitStr);
extern void AtcAp_OutputAddr_IpDns(unsigned char ucV4V6Fg, unsigned char* pAddr_v4, unsigned char* pAddr_v6);


extern  unsigned char ATC_CmdFuncInf(unsigned char *pCommandBuffer, unsigned char ucEventIdx,  unsigned char *pEventBuffer, unsigned char *pucCmdFunc);
extern  unsigned char ATC_CheckNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, unsigned char *pBinaryData, unsigned char *pStopStrInf);
extern unsigned char ATC_GetDecimalParameterByte(unsigned char *pCommandBuffer, 
                                          unsigned short *pOffSet, 
                                          unsigned int uiFigure, 
                                          unsigned char *pData, 
                                          unsigned char *pDataFlag,
                                          unsigned char ucMin,
                                          unsigned char ucMax, 
                                          unsigned char ucOptFlag,
                                          unsigned char ucLastParameter);
extern unsigned char ATC_GetDecimalParameterLong(unsigned char *pCommandBuffer, 
                                          unsigned short *pOffSet, 
                                          unsigned int uiFigure, 
                                          unsigned long *pData, 
                                          unsigned char *pDataFlag,
                                          unsigned long Min,
                                          unsigned long Max, 
                                          unsigned char ucOptFlag,
                                          unsigned char ucLastParameter,
                                          unsigned char ucAllowZeroStartFlg);
extern  unsigned char ATC_GetStrParameter(unsigned char *pCommandBuffer, unsigned short *offset, unsigned short usMaxLen, unsigned char *pParamLen, unsigned char *pParaStr, unsigned char ucOptFlg, unsigned char ucLastParameter);
extern  unsigned char ATC_GetHexStrParameter(unsigned char *pCommandBuffer, unsigned short *pOffset, unsigned short usMaxLen, unsigned short *pDataLen, unsigned char *pData, unsigned char ucOptFlg, unsigned char ucLastParameter);
extern  unsigned char ATC_CheckLongNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, unsigned long *pBinaryData, unsigned char *pStopStrInf, unsigned char ucAllowZeroStartFlg);
extern  unsigned char ATC_CheckHexParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, unsigned long *pBinaryData, unsigned char *pStopStrInf);
extern  unsigned char ATC_CheckStrParameter(unsigned char *pCommandBuffer, signed int iFigure, unsigned char *pLength, unsigned char *pStrLength, unsigned char *pParaStr, unsigned char *pStopStrInf);
extern  unsigned char ATC_CharToBinary(unsigned char *pInputCharStr , unsigned char ucLength , unsigned char *pBinaryData, unsigned char ucSignedFlg);
extern  void ATC_Strncpy(unsigned char *pStr1, unsigned char *pStr2, unsigned short usCount);
extern  unsigned int ATC_GetStrLen(unsigned char *pStr);
extern  unsigned char ATC_ShortToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned short *pBinaryData);
extern  unsigned char AtcAp_UlongToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned long *pBinaryData, unsigned char ucAllowZeroStartFlg);
extern  unsigned char ATC_CheckUSNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, unsigned short *pBinaryData, unsigned char *pStopStrInf);
extern  unsigned char ATC_UshortToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned short *pBinaryData);
extern  void AtcAp_SendLongDataInd(unsigned char **pBuffer, unsigned short usMaxLen);
extern unsigned short AtcAp_StrPrintf_AtcRspBuf(const char* FormatBuffer, ...);
extern const char* ATC_ConvertErrCode2Str(const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE* pErrTab, unsigned char size, unsigned char ucErrCode);
extern  unsigned char ATC_PassWordStrCheck(unsigned char *pInputStr, unsigned char *pEventBuffer);
extern void ATC_NSET_AtHeaderSetProc(unsigned char* pCurrEvent);
extern void ATC_CmdHeaderWithSpaceProc(char** pBuff, unsigned short* pusLen, unsigned short usMaxLen);
extern unsigned char ATC_NCONFIG_SET_IsStrChk(unsigned char ucType);
extern unsigned char ATC_GET_IPV6ADDR_ALL(unsigned char* pucIpv6Addr);
//shao add for USAT
extern unsigned char ATC_CheckHexStrParameter(unsigned char *pCommandBuffer, signed int iFigure, unsigned short *pLength ,
    unsigned short *pStrLength, unsigned char *pParaStr, unsigned char *pStopStrInf);
//
extern unsigned char ATC_CheckUSStrParameter(unsigned char *pCommandBuffer, signed int iFigure, unsigned short *pLength ,
    unsigned short *pStrLength, unsigned char *pParaStr, unsigned char *pStopStrInf);
extern void AtcAp_HexToAsc(unsigned short usLength,unsigned char *pOutData,unsigned char *pInputData);
extern void AtcAp_IntegerToPlmn(unsigned long ulInputData, unsigned char *pOutputData);
extern unsigned char ATC_CheckHexNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
    unsigned short *pBinaryData, unsigned char *pStopStrInf);
extern void AtcAp_ConvertTimeZone(unsigned char *pTimeZoneTime,unsigned char ucDayLightTime);
extern unsigned char AtcAp_RevertHexToDecimal(unsigned char ucHex);
extern void AtcAp_OutputTimeZone(unsigned char ucCtzrReport, LNB_NAS_LOCAL_TIME_STRU* pLocalTime);
extern void AtcAp_OutputTimeZone_XY(unsigned char ucCtzrReport, LNB_NAS_LOCAL_TIME_STRU* pLocalTime);
extern void AtcAp_ConvertInDotFormat(char* pStrBuff, unsigned char* pAddr, unsigned char ucLen);
extern void AtcAp_OutputLocalTime(LNB_NAS_LOCAL_TIME_STRU* pLocalTime);
extern void AtcAp_OutputUniversalTime(LNB_NAS_LOCAL_TIME_STRU* pLocalTime);
extern void AtcAp_OutputAddr(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_OutputAddr_NoQuotation(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_OutputAddr_IPv6(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_OutputAddr_IPv6ColonFormat(unsigned char *pData, unsigned char *pucAtcRspBuf);
extern void AtcAp_OutputPortRange(unsigned char ucDataLen, unsigned short *pData, unsigned char* pRespBuff);
extern unsigned char ATC_ChkStrPara(unsigned char *pCommandBuffer ,signed int iFigure ,unsigned short *pStrLength, unsigned char *pParaStr ,unsigned char *pStopStrInf);
extern void AtcAp_Build_NCell(API_CELL_LIST_STRU* pCellList);


#ifdef LCS_MOLR_ENABLE
extern unsigned short Lcs_MolrResult_OutputXML(unsigned char* pXmlData, unsigned short* pSize, LCS_SHAPE_DATA_STRU* pShapeData, LCS_VELOCITY_DATA_STRU* pVelData);
#endif

#endif





