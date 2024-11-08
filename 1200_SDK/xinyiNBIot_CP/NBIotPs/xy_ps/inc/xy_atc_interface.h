/** 
* @file         xy_atc_interface.h
* @brief        该头文件给出的是3GPP相关的AT命令主动上报对应的结构体。\n
***  3GPP相关AT命令处理接口，包括两个部分，一个是主动上报类消息的处理机制xy_atc_registerPSEventCallback；\n
***  另一个是用户触发的AT请求类消息的处理xy_atc_interface_call。
* @attention    3GPP细节较多，用户二次开发时请务必确认清楚后再执行相关AT的改造
*
*/


#ifndef _ATC_AP_INTERFACE_H_
#define _ATC_AP_INTERFACE_H_

#include "nbiot_ps_export_interface.h"
#include "factory_nv.h"

/* For Result */
#define D_RESULT_SUCCESS        0
#define D_RESULT_FAILURE        1

#define  D_MAX_ATCID                            11
#define  D_MAX_CNT_CID                          2

#define  D_ATC_MAX_CID                          D_MAX_ATCID
#define  D_ATC_MIN_CID                          0

#define D_ATC_MAX_PACKET_FILTER                 6

#define D_APN_LEN                               100
#define D_EPS_IMEI_LEN                          15
#define EMM_SIM_UICC_ID_LEN                     10
#define D_ATC_CRTDCP_IND_LENGTH                 26
#define D_ATC_NRNPDM_IND_LENGTH                 26
#define D_ATC_QNIDD_IND_LENGTH                  26

#define D_ATC_P_CSCA_IND_SCA_SIZE_MAX           22

#define EPS_CSODCP_CPDATA_LENGTH_MAX            1600

#define D_ATC_NONIP_DATA_LEN_MAX                1358

#define D_ATC_EARFCN_MAX_NUM                    19

#define D_ATC_P_CPWD_OLDPWD_SIZE_MAX            8                                       /* +CPWD:old pwd max length                 */
#define D_ATC_P_CPWD_NEWPWD_SIZE_MAX            8                                       /* +CPWD:new pwd max length                 */
#define D_ATC_P_CPIN_PIN_SIZE_MAX               8                                       /* +CPIN:pin max size                       */
#define D_ATC_P_CPIN_NEWPIN_SIZE_MAX            8                                       /* +CPIN:newpin max size                    */
#define D_ATC_P_CLCK_PASSWD_SIZE_MAX            8                                       /* +CLCK:<passwd>                           */
#define D_ATC_P_NSET_INS_SIZE_MAX               20
#define D_ATC_P_SIMUUICC_INS_SIZE_MAX           20
#define D_ATC_AP_USIM_MAX_APDU_SIZE             261

#define D_ATC_SIMUUICC_IMSI_SIZE_MAX           15
#define NAS_AUTH_KEY_LTH   16
#define NAS_AUTH_OP_LTH    16

#define D_ATC_SIMUUICC_FILE_ID_MAX                4
#define D_ATC_SIMUUICC_FILE_CONTENT_MAX           32

#define D_ATC_PARAM_FAC_SC                      0                                       /* <fac>: "SC"                              */
#define D_ATC_PARAM_FAC_AO                      1                                       /* <fac>: "AO"                              */
#define D_ATC_PARAM_FAC_OI                      2                                       /* <fac>: "OI"                              */
#define D_ATC_PARAM_FAC_OX                      3                                       /* <fac>: "OX"                              */
#define D_ATC_PARAM_FAC_AI                      4                                       /* <fac>: "AI"                              */
#define D_ATC_PARAM_FAC_IR                      5                                       /* <fac>: "IR"                              */
#define D_ATC_PARAM_FAC_PS                      6                                       /* <fac>: "PS"                              */
#define D_ATC_PARAM_FAC_PN                      7                                       /* <fac>: "PN"                              */
#define D_ATC_PARAM_FAC_PU                      8                                       /* <fac>: "PU"                              */
#define D_ATC_PARAM_FAC_PP                      9                                       /* <fac>: "PP"                              */
#define D_ATC_PARAM_FAC_PC                      10                                      /* <fac>: "PC"                              */
#define D_ATC_PARAM_FAC_FD                      11                                      /* <fac>: "FD"                              */
#define D_ATC_PARAM_FAC_P2                      12                                        /* <fac>: "P2"                              */
#define D_ATC_PARAM_FAC_AB                      13                                        /* <fac>: "AB"                              */
#define D_ATC_PARAM_FAC_AG                      14                                        /* <fac>: "AG"                              */
#define D_ATC_PARAM_FAC_AC                      15                                        /* <fac>: "AC"                              */
#define D_ATC_PARAM_FAC_CS                      16                                      /* <fac>: "CS"                              */
#define D_ATC_PARAM_FAC_PF                      17                                      /* <fac>: "PF"                              */
#define D_ATC_PARAM_FAC_NT                      18                                      /* <fac>: "NT"                              */
#define D_ATC_PARAM_FAC_NM                      19                                      /* <fac>: "NM"                              */
#define D_ATC_PARAM_FAC_NS                      20                                      /* <fac>: "NS"                              */
#define D_ATC_PARAM_FAC_NA                      21                                      /* <fac>: "NA"                              */

#define D_ATC_DL_GBR_MAX                        10000000
#define D_ATC_UL_GBR_MAX                        10000000
#define D_ATC_DL_MBR_MAX                        10000000
#define D_ATC_UL_MBR_MAX                        10000000

/* local flag */                             
#define D_ATC_FLAG_FALSE                     0
#define D_ATC_FLAG_TRUE                      1

#define D_ATC_CGEV_NW_DETACH                0
#define D_ATC_CGEV_ME_DETACH                1
#define D_ATC_CGEV_ME_PDN_ACT               2
#define D_ATC_CGEV_NW_ACT                   3
#define D_ATC_CGEV_ME_ACT                   4
#define D_ATC_CGEV_NW_PDN_DEACT             5
#define D_ATC_CGEV_ME_PDN_DEACT             6
#define D_ATC_CGEV_NW_DEACT                 7
#define D_ATC_CGEV_ME_DEACT                 8
#define D_ATC_CGEV_NW_MODIFY                9
#define D_ATC_CGEV_ME_MODIFY                10
#define D_ATC_CGEV_OOS                      11
#define D_ATC_CGEV_IS                       12
#define D_ATC_CGEV_MAX                      5

#define    D_ATC_NPIN_TYPE_PIN        0
#define    D_ATC_NPIN_TYPE_PUK        1

/* register PS eventId */
#ifndef PS_REG_EVENT
#define PS_REG_EVENT
#define D_XY_PS_REG_EVENT_SIMST              0x00000001 //ATC_MSG_SIMST_IND_STRU
#define D_XY_PS_REG_EVENT_XYIPDNS            0x00000002 //ATC_MSG_XYIPDNS_IND_STRU
#define D_XY_PS_REG_EVENT_CRTDCP             0x00000004 //ATC_MSG_CRTDCP_IND_STRU
#define D_XY_PS_REG_EVENT_CGAPNRC            0x00000008 //ATC_MSG_CGAPNRC_IND_STRU
#define D_XY_PS_REG_EVENT_CGEV               0x00000010 //ATC_MSG_CGEV_IND_STRU
#define D_XY_PS_REG_EVENT_CEREG              0x00000020 //ATC_MSG_CEREG_IND_STRU
#define D_XY_PS_REG_EVENT_CSCON              0x00000040 //ATC_MSG_CSCON_IND_STRU
#define D_XY_PS_REG_EVENT_NPTWEDRXP          0x00000080 //ATC_MSG_NPTWEDRXP_IND_STRU
#define D_XY_PS_REG_EVENT_CEDRXP             0x00000100 //ATC_MSG_CEDRXP_IND_STRU
#define D_XY_PS_REG_EVENT_CCIOTOPTI          0x00000200 //ATC_MSG_CCIOTOPTI_IND_STRU
#define D_XY_PS_REG_EVENT_CESQ_Ind           0x00000400 //ATC_MSG_CESQ_IND_STRU, Report RSRP/SINR every 10 seconds
#define D_XY_PS_REG_EVENT_LOCALTIMEINFO      0x00000800 //ATC_MSG_LOCALTIMEINFO_IND_STRU
#define D_XY_PS_REG_EVENT_PDNIPADDR          0x00001000 //ATC_MSG_PdnIPAddr_IND_STRU
#define D_XY_PS_REG_EVENT_NGACTR             0x00002000 //ATC_MSG_NGACTR_IND_STRU
#define D_XY_PS_REG_EVENT_CMT                0x00004000 //ATC_MSG_CMT_IND_STRU
#define D_XY_PS_REG_EVENT_CMOLRE             0x00008000 //ATC_MSG_CMOLRE_IND_STRU
#define D_XY_PS_REG_EVENT_CMOLRG             0x00010000 //ATC_MSG_CMOLRG_IND_STRU
#define D_XY_PS_REG_EVENT_NRNPDM             0x01000000 //ATC_MSG_NRNPDM_IND_STRU
#define D_XY_PS_REG_EVENT_MNBIOTEVENT        0x02000000 //ATC_MSG_MNBIOTEVENT_IND_STRU
#endif
/* XY internal use only */
#define D_XY_PS_REG_EVENT_PSINFO             0x00020000 //ATC_MSG_PSINFO_IND_STRU
#define D_XY_PS_REG_EVENT_PINSTATUS          0x00040000 //ATC_MSG_PIN_STATUS_IND_STRU
#define D_XY_PS_REG_EVENT_CELLSRCH           0x00080000 //ATC_MSG_CELLSRCH_TEST_STRU
#define D_XY_PS_REG_EVENT_L2_THP             0x00100000 //ATC_MSG_L2_THP_IND_STRU
#define D_XY_PS_REG_EVENT_MALLOC_ADDR        0x00200000 //ATC_MSG_MALLOC_ADDR_IND_STRU
#define D_XY_PS_REG_EVENT_NoCarrier          0x00400000 //ATC_MSG_NO_CARRIER_IND_STRU
#define D_XY_PS_REG_EVENT_IPSN               0x00800000 //ATC_MSG_IPSN_IND_STRU
#define D_XY_PS_REG_EVENT_OK_ERROR           0x04000000 //ST_ATC_AP_CMD_RST

/* ATC command event table  */
/* Basic */
enum {
    //D_ATC_EVENT_CGMI = 0,                                                               /* 0  */
    ///D_ATC_EVENT_CGMI_T,                                                                 /* 1  */
    //D_ATC_EVENT_CGMR,                                                                   /* 2  */
    //D_ATC_EVENT_CGMR_T,                                                                 /* 3  */
    D_ATC_EVENT_CGSN = 1,                                                               /* 0  */
    D_ATC_EVENT_CGSN_T,                                                                 /* 1  */
    D_ATC_EVENT_CEREG,                                                                  /* 2  */
    D_ATC_EVENT_CEREG_R,                                                                /* 3  */
    D_ATC_EVENT_CEREG_T,                                                                /* 4  */
    D_ATC_EVENT_CGATT,                                                                  /* 5  */
    D_ATC_EVENT_CGATT_R,                                                                /* 6  */
    D_ATC_EVENT_CGATT_T,                                                                /* 7  */
    D_ATC_EVENT_CIMI,                                                                   /* 8  */
    D_ATC_EVENT_CIMI_T,                                                                 /* 9  */
    D_ATC_EVENT_CGDCONT,                                                                /* 10 */
    D_ATC_EVENT_CGDCONT_R,                                                              /* 11 */
    D_ATC_EVENT_CGDCONT_T,                                                              /* 12 */
    D_ATC_EVENT_CFUN,                                                                   /* 13 */
    D_ATC_EVENT_CFUN_R,                                                                 /* 14 */
    D_ATC_EVENT_CFUN_T,                                                                 /* 15 */
    D_ATC_EVENT_CMEE,                                                                   /* 16 */
    D_ATC_EVENT_CMEE_R,                                                                 /* 17 */
    D_ATC_EVENT_CMEE_T,                                                                 /* 18 */
    D_ATC_EVENT_CLAC,                                                                   /* 19 */
    D_ATC_EVENT_CLAC_T,                                                                 /* 20 */
    D_ATC_EVENT_CESQ,                                                                   /* 21 */
    D_ATC_EVENT_CESQ_T,                                                                 /* 22 */
    D_ATC_EVENT_CGPADDR,                                                                /* 23 */
    D_ATC_EVENT_CGPADDR_T,                                                              /* 24 */
    //D_ATC_EVENT_CGMM,                                                                   /* 25 */
    //D_ATC_EVENT_CGMM_T,                                                                 /* 26 */
    //D_ATC_EVENT_CGDATA,                                                                 /* 25 */
    //D_ATC_EVENT_CGDATA_T,                                                               /* 26 */
    D_ATC_EVENT_CGACT,                                                                  /* 27 */
    D_ATC_EVENT_CGACT_R,                                                                /* 28 */
    D_ATC_EVENT_CGACT_T,                                                                /* 29 */
    D_ATC_EVENT_CSODCP,                                                                 /* 30 */
    D_ATC_EVENT_CSODCP_T,                                                               /* 31 */
    D_ATC_EVENT_CRTDCP,                                                                 /* 32 */
    D_ATC_EVENT_CRTDCP_R,                                                               /* 33 */
    D_ATC_EVENT_CRTDCP_T,                                                               /* 34 */
    //D_ATC_EVENT_CRC,                                                                    /* 35 */
    //D_ATC_EVENT_CRC_R,                                                                  /* 36 */
    //D_ATC_EVENT_CRC_T,                                                                  /* 37 */
    //D_ATC_EVENT_CMUX,                                                                   /* 38 */
    //D_ATC_EVENT_CMUX_R,                                                                 /* 39 */
    //D_ATC_EVENT_CMUX_T,                                                                 /* 40 */
    //D_ATC_EVENT_S3,                                                                     /* 41 */
    //D_ATC_EVENT_S3_R,                                                                   /* 42 */
    //D_ATC_EVENT_S4,                                                                     /* 43 */
    //D_ATC_EVENT_S4_R,                                                                   /* 44 */
    //D_ATC_EVENT_S5,                                                                     /* 45 */
    //D_ATC_EVENT_S5_R,                                                                   /* 46 */
    //D_ATC_EVENT_E,                                                                      /* 47 */
    //D_ATC_EVENT_V,                                                                      /* 48 */
    //D_ATC_EVENT_F,                                                                      /* 49 */
    D_ATC_EVENT_SIMST_R,                                                                /* 50 */
    D_ATC_EVENT_CEDRXS,                                                                 /* 51 */
    D_ATC_EVENT_CEDRXS_R,                                                               /* 52 */
    D_ATC_EVENT_CEDRXS_T,                                                               /* 53 */
    D_ATC_EVENT_CPSMS,                                                                  /* 54 */
    D_ATC_EVENT_CPSMS_R,                                                                /* 55 */
    D_ATC_EVENT_CPSMS_T,                                                                /* 56 */
    D_ATC_EVENT_CGAPNRC,                                                                /* 57 */
    D_ATC_EVENT_CGAPNRC_T,                                                              /* 58 */
/* Other */
    D_ATC_EVENT_CGDSCONT,                                                               /* 59 */
    D_ATC_EVENT_CGDSCONT_R,                                                             /* 60 */
    D_ATC_EVENT_CGDSCONT_T,                                                             /* 61 */
    D_ATC_EVENT_CGTFT,                                                                  /* 62 */
    D_ATC_EVENT_CGTFT_R,                                                                /* 63 */
    D_ATC_EVENT_CGTFT_T,                                                                /* 64 */
    D_ATC_EVENT_CGEQOS,                                                                 /* 65 */
    D_ATC_EVENT_CGEQOS_R,                                                               /* 66 */
    D_ATC_EVENT_CGEQOS_T,                                                               /* 67 */
    D_ATC_EVENT_CGCMOD,                                                                 /* 68 */
    D_ATC_EVENT_CGCMOD_T,                                                               /* 69 */
/* SMS */
    D_ATC_EVENT_CSMS,                                                                   /* 70 */
    D_ATC_EVENT_CSMS_R,                                                                 /* 71 */
    D_ATC_EVENT_CSMS_T,                                                                 /* 72 */
    D_ATC_EVENT_CMGF,                                                                   /* 73 */
    D_ATC_EVENT_CMGF_R,                                                                 /* 74 */
    D_ATC_EVENT_CMGF_T,                                                                 /* 75 */
    D_ATC_EVENT_CSCA,                                                                   /* 76 */
    D_ATC_EVENT_CSCA_R,                                                                 /* 77 */
    D_ATC_EVENT_CSCA_T,                                                                 /* 78 */
    //D_ATC_EVENT_CNMI,                                                                   /* 79 */
    //D_ATC_EVENT_CNMI_R,                                                                 /* 80 */
    //D_ATC_EVENT_CNMI_T,                                                                 /* 81 */
    D_ATC_EVENT_CMGS,                                                                   /* 82 */
    D_ATC_EVENT_CMGS_T,                                                                 /* 83 */
    D_ATC_EVENT_CNMA,                                                                   /* 84 */
    D_ATC_EVENT_CNMA_T,                                                                 /* 85 */
    D_ATC_EVENT_COPS,                                                                   /* 86 */
    D_ATC_EVENT_COPS_R,                                                                 /* 87 */
    D_ATC_EVENT_COPS_T,                                                                 /* 88 */
//shao add for USAT
    D_ATC_EVENT_CSIM,                                                                   /* 89 */
    D_ATC_EVENT_CSIM_T,                                                                 /* 90 */
    D_ATC_EVENT_CCHC,                                                                   /* 91 */
    D_ATC_EVENT_CCHC_T,                                                                 /* 92 */
    D_ATC_EVENT_CCHO,                                                                   /* 93 */
    D_ATC_EVENT_CCHO_T,                                                                 /* 94 */
    D_ATC_EVENT_CGLA,                                                                   /* 95 */
    D_ATC_EVENT_CGLA_T,                                                                 /* 96 */
    D_ATC_EVENT_CRSM,                                                                   /* 97 */
    D_ATC_EVENT_CRSM_T,                                                                 /* 98 */
    D_ATC_EVENT_CSCON,                                                                  /* 99 */
    D_ATC_EVENT_CSCON_R,                                                                /* 100 */
    D_ATC_EVENT_CSCON_T,                                                                 /* 101 */
    D_ATC_EVENT_CGEREP,                                                                  /* 102 */
    D_ATC_EVENT_CGEREP_R,                                                                /* 103 */
    D_ATC_EVENT_CGEREP_T,                                                                /* 104 */
    D_ATC_EVENT_CCIOTOPT,                                                                /* 105 */
    D_ATC_EVENT_CCIOTOPT_R,                                                              /* 106 */
    D_ATC_EVENT_CCIOTOPT_T,                                                              /* 107 */
    D_ATC_EVENT_CEDRXRDP,                                                                /* 108 */
    D_ATC_EVENT_CEDRXRDP_T,                                                              /* 109 */
    D_ATC_EVENT_CGEQOSRDP,                                                               /* 110 */
    D_ATC_EVENT_CGEQOSRDP_T,                                                             /* 111 */
    D_ATC_EVENT_CTZR,                                                                    /* 112 */
    D_ATC_EVENT_CTZR_R,                                                                  /* 113 */
    D_ATC_EVENT_CTZR_T,                                                                  /* 114 */
    D_ATC_EVENT_CGCONTRDP,                                                               /* 115 */
    D_ATC_EVENT_CGCONTRDP_T,                                                             /* 116 */
    D_ATC_EVENT_CPIN,                                                                    /* 117 */
    D_ATC_EVENT_CPIN_R,                                                                  /* 118 */
    D_ATC_EVENT_CPIN_T,                                                                  /* 119 */
    D_ATC_EVENT_CLCK,                                                                    /* 120 */
    D_ATC_EVENT_CLCK_T,                                                                  /* 121 */
    D_ATC_EVENT_CPWD,                                                                    /* 122 */
    D_ATC_EVENT_CPWD_T,                                                                  /* 123 */
   // D_ATC_EVENT_NRB,
    D_ATC_EVENT_NUESTATS,
    D_ATC_EVENT_NUESTATS_T,
    D_ATC_EVENT_NEARFCN,
    D_ATC_EVENT_NEARFCN_R,
    D_ATC_EVENT_NEARFCN_T,
    D_ATC_EVENT_NBAND,
    D_ATC_EVENT_NBAND_R,
    D_ATC_EVENT_NBAND_T,
    D_ATC_EVENT_NCONFIG,
    D_ATC_EVENT_NCONFIG_R,
    D_ATC_EVENT_NCONFIG_T,
    D_ATC_EVENT_NCCID,
    D_ATC_EVENT_NCCID_T,
    //D_ATC_EVENT_NPSMR,
    //D_ATC_EVENT_NMGS,
    //D_ATC_EVENT_NMGR,
    //D_ATC_EVENT_NMGS,
    //D_ATC_EVENT_NMGR,
    //D_ATC_EVENT_NQMGS,
    //D_ATC_EVENT_NQMGR,
    //D_ATC_EVENT_QLWUDATAEX,
    //D_ATC_EVENT_NCDP,
    D_ATC_EVENT_NCSEARFCN,
    D_ATC_EVENT_RAI,
    D_ATC_EVENT_NFPLMN,
        
    D_ATC_EVENT_NL2THP,             
    D_ATC_EVENT_NL2THP_R,      
    D_ATC_EVENT_NL2THP_T,  
    //D_ATC_EVENT_NATSPEED,
    //D_ATC_EVENT_NSOCR,
    //D_ATC_EVENT_NSOST,
    //D_ATC_EVENT_NPING,
    D_ATC_EVENT_CSQ,
    D_ATC_EVENT_CSQ_T,
    D_ATC_EVENT_NSET,
    D_ATC_EVENT_NSET_R,
    D_ATC_EVENT_CMOLR,
    D_ATC_EVENT_CMOLR_R,                                                             
    D_ATC_EVENT_CMOLR_T,

    D_ATC_EVENT_CEER,
    D_ATC_EVENT_CEER_T,
    D_ATC_EVENT_CIPCA,
    D_ATC_EVENT_CIPCA_R,
    D_ATC_EVENT_CIPCA_T,
    D_ATC_EVENT_CGAUTH,
    D_ATC_EVENT_CGAUTH_R,
    D_ATC_EVENT_CGAUTH_T,
    D_ATC_EVENT_CNMPSD,
    D_ATC_EVENT_CNMPSD_T,
    D_ATC_EVENT_CPINR,
    D_ATC_EVENT_CPINR_T,
    D_ATC_EVENT_CMGC,
    D_ATC_EVENT_CMGC_T,
    D_ATC_EVENT_CMMS,
    D_ATC_EVENT_CMMS_R,
    D_ATC_EVENT_CMMS_T,
    D_ATC_EVENT_NPOWERCLASS,
    D_ATC_EVENT_NPOWERCLASS_R,
    D_ATC_EVENT_NPOWERCLASS_T,
    D_ATC_EVENT_NPTWEDRXS,
    D_ATC_EVENT_NPTWEDRXS_R,
    D_ATC_EVENT_NPTWEDRXS_T,
    D_ATC_EVENT_NPIN,
    D_ATC_EVENT_NPIN_T,
    D_ATC_EVENT_NTSETID,
    D_ATC_EVENT_NTSETID_T,
    D_ATC_EVENT_NCIDSTATUS,
    D_ATC_EVENT_NCIDSTATUS_R,
    D_ATC_EVENT_NCIDSTATUS_T,
    D_ATC_EVENT_NGACTR,
    D_ATC_EVENT_NGACTR_R,
    D_ATC_EVENT_NGACTR_T,
    D_ATC_EVENT_NPOPB,
    D_ATC_EVENT_NPOPB_R,
    D_ATC_EVENT_NIPINFO,
    D_ATC_EVENT_NIPINFO_R,
    D_ATC_EVENT_NIPINFO_T,
    D_ATC_EVENT_NQPODCP,
    D_ATC_EVENT_NQPODCP_T,
    D_ATC_EVENT_NSNPD,
    D_ATC_EVENT_NSNPD_T,
    D_ATC_EVENT_NQPNPD,
    D_ATC_EVENT_NQPNPD_T,
    D_ATC_EVENT_CNEC,
    D_ATC_EVENT_CNEC_R,
    D_ATC_EVENT_CNEC_T,
    D_ATC_EVENT_NRNPDM,
    D_ATC_EVENT_NRNPDM_R,
    D_ATC_EVENT_NRNPDM_T,
    D_ATC_EVENT_NCPCDPR,
    D_ATC_EVENT_NCPCDPR_R,
    D_ATC_EVENT_NCPCDPR_T,
    D_ATC_EVENT_CEID,
    D_ATC_EVENT_CEID_T,
    D_ATC_EVENT_MNBIOTEVENT,
    D_ATC_EVENT_MNBIOTEVENT_R,
    D_ATC_EVENT_MNBIOTEVENT_T,
    D_ATC_EVENT_CGPIAF,
    D_ATC_EVENT_CGPIAF_R,
    D_ATC_EVENT_CGPIAF_T,
    D_ATC_EVENT_CUPREFER,
    D_ATC_EVENT_CUPREFERTH,
    D_ATC_EVENT_CUPREFERTH_R,
    D_ATC_EVENT_CUPREFERTH_T,
    D_ATC_EVENT_NPLMNS,
    D_ATC_EVENT_NPLMNS_R,
    D_ATC_EVENT_NPLMNS_T,
    D_ATC_EVENT_NLOCKF,
    D_ATC_EVENT_NLOCKF_R,
    D_ATC_EVENT_NLOCKF_T,
    D_ATC_EVENT_ZICCID,
    D_ATC_EVENT_ZICCID_T,
    D_ATC_EVENT_ZCELLINFO,
    D_ATC_EVENT_QCGDEFCONT,
    D_ATC_EVENT_QCGDEFCONT_R,
    D_ATC_EVENT_QCGDEFCONT_T,
    D_ATC_EVENT_QBAND,
    D_ATC_EVENT_QBAND_R,
    D_ATC_EVENT_QBAND_T,
    D_ATC_EVENT_QCCID,
    D_ATC_EVENT_QENG,
    D_ATC_EVENT_QENG_T,
    D_ATC_EVENT_QCFG,
    D_ATC_EVENT_QCFG_R,
    D_ATC_EVENT_QCFG_T,
    D_ATC_EVENT_NSIMWC,
    D_ATC_EVENT_NUICC,
    D_ATC_EVENT_NUICC_T,
    D_ATC_EVENT_PSTEST,
    D_ATC_EVENT_QNIDD,
    D_ATC_EVENT_XYCELLS,
    D_ATC_EVENT_PRESETFREQ,
    D_ATC_EVENT_PRESETFREQ_R,
    D_ATC_EVENT_NPBPLMNS,
    D_ATC_EVENT_NPBPLMNS_R,
    D_ATC_EVENT_NPBPLMNS_T,
    D_ATC_EVENT_NBACKOFF,
    D_ATC_EVENT_NBACKOFF_R,
    D_ATC_EVENT_NBACKOFF_T,
    D_ATC_EVENT_SIMUUICC,
    D_ATC_EVENT_SIMUUICC_R,
    D_ATC_EVENT_QLOCKF,
    D_ATC_EVENT_QLOCKF_R,
    D_ATC_EVENT_QLOCKF_T,
    D_ATC_EVENT_QCSEARFCN,
    D_ATC_EVENT_QCSEARFCN_T,
    D_ATC_EVENT_W,
    D_ATC_EVENT_QICSGP,
    D_ATC_EVENT_QICSGP_R,
    D_ATC_EVENT_QICSGP_T,
    D_ATC_EVENT_QSPCHSC,
    D_ATC_EVENT_QSPCHSC_R,
    D_ATC_EVENT_QSPCHSC_T,
    D_ATC_EVENT_NULL
};

typedef enum {
    D_ATC_AP_AT_CMD_RST = D_ATC_EVENT_NULL,
    D_ATC_AP_SMS_PDU_IND,
    D_ATC_AP_SIMST_IND,
    D_ATC_AP_CRTDCP_IND,
    D_ATC_AP_CGAPNRC_IND,
    D_ATC_AP_CGEV_IND,
    D_ATC_AP_CEREG_IND,
    D_ATC_AP_CSCON_IND,
    D_ATC_AP_NPTWEDRXP_IND,
    D_ATC_AP_CEDRXP_IND,
    D_ATC_AP_CCIOTOPTI_IND,
    D_ATC_AP_CMT_IND,
    D_ATC_AP_PINSTATUS_IND,
    D_ATC_AP_L2THP_IND,
    D_ATC_AP_XYIPDNS_IND,
    D_ATC_AP_MALLOCADDR_IND,
    D_ATC_AP_IPSN_IND,
    D_ATC_AP_CMOLRE_IND,
    D_ATC_AP_CMOLRG_IND,
    D_ATC_AP_IPADDR_IND,
    D_ATC_AP_NGACTR_IND,
    D_ATC_AP_LOCALTIMEINFO_IND,
    D_ATC_OPELIST_SRCH_CNF,
    D_ATC_NO_CARRIER_IND,
    D_ATC_AP_SMS_PDU_REQ,
    D_ATC_AP_CELL_SRCH_IND,
    D_ATC_AP_EVENT_CESQ_IND,
    D_ATC_AP_PSINFO_IND,
    D_ATC_AP_CSODCPR_IND,
    D_ATC_AP_NSNPDR_IND,
    D_ATC_AP_NIPINFO_IND,
    D_ATC_AP_NQPODCP_IND,
    D_ATC_AP_CNEC_IND,
    D_ATC_AP_NRNPDM_IND,
    D_ATC_AP_MNBIOTEVENT_IND,
    D_ATC_AP_QNIDD_IND,
    D_ATC_AP_XYCELLS_IND,
    D_ATC_AP_NPBPLMNS_IND,
    D_ATC_AP_NBACKOFF_IND,
    D_ATC_AP_NPLMNS_OOS_IND,
    D_ATC_AP_SIM_DATA_DOWNLOAD_IND,
    D_ATC_AP_FREQ_RSSI_IND,
    D_ATC_AP_SYSINFO_FAIL_IND,
} ATC_AP_EXTEND_EVENT_ID;

enum 
{
    D_ATC_NCONFIG_AUTOCONNECT   = 0,
    D_ATC_NCONFIG_COMBINE_ATTACH,
    D_ATC_NCONFIG_CELL_RESELECTION,
    D_ATC_NCONFIG_ENABLE_BIP,
    D_ATC_NCONFIG_MULTITONE,
    D_ATC_NCONFIG_BARRING_RELEASE_DELAY,
    D_ATC_NCONFIG_RELEASE_VERSION,
    D_ATC_NCONFIG_SYNC_TIME_PERIOD,
    D_ATC_NCONFIG_PCO_IE_TYPE,
    D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE,
    D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT,
    D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME,
    D_ATC_NCONFIG_RAI,
    D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE,
    D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING,
    D_ATC_NCONFIG_CR_0859_SI_AVOID,
    D_ATC_NCONFIG_NPLMNS_OOS_IND,
    D_ATC_NCONFIG_MAX,
};

#ifndef D_PDP_TYPE_INVALID
#define D_PDP_TYPE_INVALID      0
#define D_PDP_TYPE_IPV4         1
#define D_PDP_TYPE_IPV6         2
#define D_PDP_TYPE_IPV4V6       3
#define D_PDP_TYPE_NonIP        5
#endif

typedef struct atc_data_req
{
    MSG_HEADER_STRU             MsgHead;
    //unsigned char               ucReqType;
    unsigned char               ucSeqNum;
    unsigned char               ucAplNum;
    unsigned short              usMsgLen;
    unsigned char               aucMsgData[4];
} ST_ATC_DATA_REQ;

typedef struct
{
    unsigned char               ucSeqNum;
    unsigned char               ucPadding;
    unsigned short              usMsgLen;
    unsigned char               aucMsgData[4];
} ATC_INTER_CORE_MSG_M3ToDSP_STRU;

typedef struct
{
    unsigned long               ulMsgName;
} ATC_AP_MSG_HEADER_STRU;

typedef struct
{
    ATC_AP_MSG_HEADER_STRU      MsgHead;
    unsigned long               ulSemaId;     //0: AtCmd, >0: appInterfaceCmd
    unsigned char               ucExternalFlg;
    unsigned char               ucAplNum;
    unsigned short              usMsgLen;
    unsigned char               ucTaskSource;
    unsigned char               aucMsgData[4];
} ATC_AP_MSG_DATA_REQ_STRU;

/* the message of PS send to ATC_AP */
typedef struct
{
    ATC_AP_MSG_HEADER_STRU      MsgHead;
    unsigned short              usMsgLen;
    unsigned char               ucSeqNum;
    unsigned char               ucPadding;
    unsigned char               aucMsgData[4];
} ATC_MSG_DATA_IND_STRU;

/************************************************************************/
/*                 Structure used for command analysis                  */
/************************************************************************/
typedef struct {
    unsigned short  usEvent;
}ST_ATC_CMD_COM_EVENT;

/* +CGDCONT command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;                                                                       /* PDP Context Identifier               */
    unsigned char  ucPdpTypeFlg;                                                                /* PDP type number value flag           */
    unsigned char  ucPdpTypeValue;                                                              /* PDP type number value                */
    unsigned char  ucD_comp;
    unsigned char  ucH_compFlag;
    unsigned char  ucH_comp;
    unsigned char  ucIpv4AddrAllocFlag;
    unsigned char  ucIpv4AddrAlloc;
    unsigned char  ucRequestTypeFlag;
    unsigned char  ucRequestType;
    unsigned char  ucPCscfDiscoveryFlag;
    unsigned char  ucPCscfDiscovery;
    unsigned char  ucImCnSignallingFlagIndFlag;
    unsigned char  ucImCnSignallingFlagInd;
    unsigned char  ucNSLPIFlag;
    unsigned char  ucNSLPI;
    unsigned char  ucSecurePcoFlag;
    unsigned char  ucSecurePco;
    unsigned char  ucIpv4MtuDiscoveryFlag;
    unsigned char  ucIpv4MtuDiscovery;
    unsigned char  ucLocalAddrIndFlag;
    unsigned char  ucLocalAddrInd;
    unsigned char  ucNonIpMtuDiscoveryFlag;
    unsigned char  ucNonIpMtuDiscovery;
    unsigned char  ucReliableDataServiceFlag;
    unsigned char  ucReliableDataService;
    unsigned char  ucApnLen;                                                                    /* Access point name length             */
    unsigned char  aucApnValue[64];   //ucApnLen <= 64
    unsigned char* pucApnValue;       //ucApnLen > 64
} ST_ATC_CGDCONT_PARAMETER;

/* +QCGEFDCONT command parameter structure */
typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucPdpTypeFlg;
    unsigned char                 ucPdpTypeValue;
    unsigned char                 ucApnFlg;
    unsigned char                 ucApnLen;
    unsigned char                 ucUsernameLen;
    unsigned char                 ucPasswordLen;
    unsigned char                 ucAuthProt;
    unsigned char                 aucUsername[16];
    unsigned char                 aucPassword[16];
    unsigned char                 aucApnValue[38];
} ST_ATC_QCGDEFCONT_PARAMETER;

/* D* command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucGPRS_SC;                                                                   /*  GPRS Service Code                   */
    unsigned char  ucCalledAddr;                                                                /*  Called address                      */
    unsigned char  ucL2P;                                                                       /*  PPP                                 */
    unsigned char  ucCid;                                                                       /*  PDP Context Identifier              */
} ST_ATC_DPS_PARAMETER;

/* temp for NPSMR by xuejin li*/
typedef struct {
    unsigned short usEvent;
    unsigned long  ulPSMRTimer;
} ST_ATC_PSMR_PARAMETER;

/* H E S3 S4 S5 V &F O +CMEE +CREG +CREG ^SPN  +CSTA +CRC  +CLIP  +CLIR  +CHLD  +COLP */
/* +CSMS +CMGF +CGSMS +CMGL +CGATT +RESET  ^CGREG ^CCATT ^CMFL CMMS RAI */
/* Command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucValue;
}ST_ATC_CMD_PARAMETER;

/* LNBACT */
/* +CGSN command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucSnt;
}ST_ATC_CGSN_PARAMETER;

/* +CGDSCONT command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
    unsigned char  ucP_cidFlg;
    unsigned char  ucP_cid;
    unsigned char  ucD_comp;
    unsigned char  ucH_comp;
    unsigned char  ucImCnSignallingFlagInd;
    unsigned char  ucImCnSignallingFlagIndFlag;
}ST_ATC_CGDSCONT_PARAMETER;    
    
/* +CGEQOS command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
    unsigned char  ucQciFlag;
    unsigned char  ucQci;
    unsigned char  ucDl_GbrFlag;
    unsigned long  ulDl_Gbr;
    unsigned char  ucUl_GbrFlag;
    unsigned long  ulUl_Gbr;
    unsigned char  ucDl_MbrFlag;
    unsigned long  ulDl_Mbr;
    unsigned char  ucUl_MbrFlag;
    unsigned long  ulUl_Mbr;
}ST_ATC_CGEQOS_PARAMETER;

/* +CGCMOD command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
}ST_ATC_CGCMOD_PARAMETER;

/* +CGTFT command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
    unsigned char  ucPacketFilterIdentifierFlag;
    unsigned char  ucPacketFilterIdentifier;
    unsigned char  ucEvaluationPrecedenceIndexFlg;
    unsigned char  ucEvaluationPrecedenceIndex;
    unsigned char  ucRemoteAddressAndSubnetMaskLen;
    unsigned char  aucRemoteAddressAndSubnetMaskValue[32];
    unsigned char  ucProtocolNumber_NextHeaderFlag;
    unsigned char  ucProtocolNumber_NextHeader;
    unsigned char  ucLocalPortRangeLen;
    unsigned short ausLocalPortRangeValue[2];
    unsigned char  ucRemotePortRangeLen;
    unsigned short ausRemotePortRangeValue[2];
    unsigned char  ucIpsecSpiFlag;
    unsigned long  ulIpsecSpi;
    unsigned char  ucTosAndMask_TracfficClassAndMaskLen;
    unsigned char  aucTosAndMask_TracfficClassAndMaskValue[2];
    unsigned char  ucFlowLabelFlag;
    unsigned long  ulFlowLabel;
    unsigned char  ucDirectionFlag;
    unsigned char  ucDirection;
    unsigned char  ucLocalAddressAndSubnetMaskLen;
    unsigned char  aucLocalAddressAndSubnetMaskValue[32];
}ST_ATC_CGTFT_PARAMETER;    

/* LNBACT */
/* +CEREG command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucNFlag;
    unsigned char  ucN;
}ST_ATC_CEREG_PARAMETER;

/* +CSODCP command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCid;
    unsigned char  ucRAI;
    unsigned char  ucRAIFlag;
    unsigned char  ucSequence;
    unsigned char  ucTUserData;
    unsigned char  ucTUserDataFlag;
    unsigned short usCpdataLength;
    unsigned char  *pucCpdata;
}ST_ATC_CSODCP_PARAMETER;

/* +CGAPNRC command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
}ST_ATC_CGAPNRC_PARAMETER;

/* +CRTDCP command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucReportingFlag;
    unsigned char  ucReporting;
}ST_ATC_CRTDCP_PARAMETER;

/* +CEDRXS command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucModeFlag;
    unsigned char  ucMode;
    unsigned char  ucActTypeFlg;
    unsigned char  ucActType;
    unsigned char  ucEDRXValueFlag;
    unsigned char  ucEDRXValue;
    unsigned char  ucPtwValueFlag;
    unsigned char  ucPtwValue;
    unsigned char  ucNptwEDrxsCmdFlg;
}ST_ATC_CEDRXS_PARAMETER;

/* +CFUN command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucModeFlag;
    unsigned char  ucMode;
    unsigned char  ucReqPeriRAUFlg;
    unsigned char  ucReqPeriRAU;
    unsigned char  ucReqGPRSReadyTimerFlg;
    unsigned char  ucReqGPRSReadyTimer;
    unsigned char  ucReqPeriTAUFlg;
    unsigned char  ucReqPeriTAU;
    unsigned char  ucReqActTimeFlag;
    unsigned char  ucReqActTime;
}ST_ATC_CPSMS_PARAMETER;

/* +CFUN command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucFun;
    unsigned char  ucRst;
}ST_ATC_CFUN_PARAMETER;

/* +CFUN command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned short usTimerVal;  //extended param: non-3gpp
}ST_ATC_CESQ_PARAMETER;

/* +CGACT command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCidFlag;
    unsigned char  ucCid;
    unsigned char  ucStateFlag;
    unsigned char  ucState;
}ST_ATC_CGACT_PARAMETER;

#ifdef NBIOT_SMS_FEATURE
/* +CNMI command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucMode;
    unsigned char  ucMtFlag;
    unsigned char  ucMt;
//     unsigned char  ucBmFlag;
//     unsigned char  ucBm;
    unsigned char  ucDsFlag;
    unsigned char  ucDs;
    unsigned char  ucBfrFlag;
    unsigned char  ucBfr;
}ST_ATC_CNMI_PARAMETER;

/* +CSCA command parameter structure */
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucScaLen;                                                               /* <sca> length                         */ 
    unsigned char       aucScaData[D_ATC_P_CSCA_IND_SCA_SIZE_MAX];
    unsigned char       ucToscaFlag;                                                            /* <tosca> flag                         */
    unsigned char       ucTosca;                                                                /* <tosca>                              */
}ST_ATC_CSCA_PARAMETER;

typedef struct {
    unsigned short      usEvent;    //D_ATC_AP_SMS_PDU_REQ
    unsigned short      usCmdEvent; //+CMGS/CMGC/CNMA Evnet
    unsigned short      usPduLength;
    unsigned char*      pucPduData;
} ST_ATC_AP_SMS_PDU_PARAMETER;

/* +CMGS +CMGW +CMGC +CNMA ^CMGS command parameter structure */
typedef struct {
    unsigned short      usEvent;    // +CMGS/CMGC/CNMA 
//    unsigned char       ucSmsMode;
    unsigned char       ucStat;
    unsigned char       ucNum;
    unsigned char       ucPduLength;
    unsigned char       aucPduData[94];  //ucPduLength <= 94 
    unsigned char*      pucPduData;      //ucPduLength > 94
}ST_ATC_PDU_TPDU_PARAMETER;
#endif

/* +CGDATA  command parameter structure */
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucCidFlag;
    unsigned char       ucCid;
    unsigned char       ucL2pFlag;
    unsigned char       ucL2p;
}ST_ATC_CGDATA_PARAMETER;

/* +CGPADDR  command parameter structure */
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucAllCidFlg;
    unsigned char       ucCidNum;
    unsigned char       aucCid[11];
}ST_ATC_CGPADDR_PARAMETER;

typedef struct {
    unsigned short      usEvent;
    unsigned char       ucMode;
    unsigned char       ucSubSet;
    unsigned char       ucSubSetFlg;
    unsigned char       ucPortSpeed;
    unsigned char       ucPortSpeedFlg;
    unsigned short      usMaxFraSize;
    unsigned char       ucMaxFraSizeFlg;
    unsigned char       ucTimer1;
    unsigned char       ucTimer1Flg;
    unsigned char       ucMaxNum;
    unsigned char       ucMaxNumFlg;
    unsigned char       ucTimer2;
    unsigned char       ucTimer2Flg;
    unsigned char       ucTimer3;
    unsigned char       ucTimer3Flg; 
    unsigned char       ucWinSize;
    unsigned char       ucWinSizeFlg;
}ST_ATC_CMUX_PARAMETER;

/* +COPS command parameter structure */
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucMode;
    unsigned char       ucFormatFlag;
    unsigned char       ucFormat;
    unsigned char       ucPerFlag;
    unsigned char       aucPer[3];
    unsigned char       ucActFlg;
    unsigned char       ucAct;
}ST_ATC_COPS_PARAMETER;

//shao add for USAT

/* +CSIM command parameter structure */
typedef struct {
    unsigned short      usEvent;
    unsigned short      usLength;
    unsigned char       aucCommand[96]; //usLength <= 96
    unsigned char*      pucCommand;     //usLength > 96
}ST_ATC_CSIM_PARAMETER;

typedef struct {
    unsigned short      usEvent;
    unsigned char       ucLength;
    unsigned char       aucDfName[16];
}ST_ATC_CCHO_PARAMETER;

typedef struct {
    unsigned short      usEvent;
    unsigned char       ucSesionId;

}ST_ATC_CCHC_PARAMETER;

typedef struct {
    unsigned short      usEvent;
    unsigned char       ucSesionId;
    unsigned short      usLength;
    unsigned char       aucCommand[96]; //usLength <= 96
    unsigned char*      pucCommand;     //usLength > 96
}ST_ATC_CGLA_PARAMETER;

typedef struct {
    unsigned short      usEvent;
    unsigned char       ucCommand;
    unsigned long       ulField;
    unsigned char       ucP1;
    unsigned char       ucP2;
    unsigned char       ucP3;
    unsigned char       ucPathLen;
    unsigned char       aucPathId[12];
    unsigned char       ucDataLen;
    unsigned char*      pucData;
}ST_ATC_CRSM_PARAMETER;
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucNFlag;
    unsigned char       ucN;
    unsigned char       ucSuppUeOptFlag;
    unsigned char       ucSupportUeOpt;
    unsigned char       ucPreUeOptFlag;
    unsigned char       ucPreferredUeOpt;
}ST_ATC_CCIOTOPT_PARAMETER;
typedef struct {
    unsigned short      usEvent;
    unsigned char       ucCid;
    unsigned char       ucCidFlag;

}ST_ATC_CGEQOSRDP_PARAMETER;

/* +CPWD command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucFac;
    unsigned char  ucOldPwdLen;
    unsigned char  aucOldPwd[D_ATC_P_CPWD_OLDPWD_SIZE_MAX];
    unsigned char  ucNewPwdLen;
    unsigned char  aucNewPwd[D_ATC_P_CPWD_NEWPWD_SIZE_MAX];
}ST_ATC_CPWD_PARAMETER;

/* +CPIN command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucNewPinFlg;
    unsigned char  aucPin[D_ATC_P_CPIN_PIN_SIZE_MAX];
    unsigned char  aucNewPin[D_ATC_P_CPIN_NEWPIN_SIZE_MAX];
}ST_ATC_CPIN_PARAMETER;

/* +CLCK command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucFac;
    unsigned char  ucMode;
    unsigned char  ucPassWdFlag;
    unsigned char  ucPassWdLen;
    unsigned char  aucPassWd[D_ATC_P_CLCK_PASSWD_SIZE_MAX];
}ST_ATC_CLCK_PARAMETER;

typedef struct {
    unsigned char ucEventId;
    unsigned char ucCid;
    unsigned char ucPcid;
    unsigned char ucEventType;
    unsigned char ucReason;
#define  D_ATC_TFT_CHANG_FLG              0x01
#define  D_ATC_QOS_CHANG_FLG              0x02
#define  D_ATC_WLAN_CHANG_FLG             0x04
    unsigned char ucChangeReason;
    unsigned char ucCidOther;
    unsigned char ucWlanOffload;
}ST_ATC_CGEV_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucCid;
    unsigned char                 ucUsernameLen;
    unsigned char                 ucPasswordLen;
    unsigned char                 ucAuthProt;
    unsigned char                 aucUsername[16];
    unsigned char                 aucPassword[16];
} ST_ATC_CGAUTH_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucN;
    unsigned char                 ucAttWithoutPDN;
} ST_ATC_CIPCA_PARAMETER;

/* +CGEREP  command parameter structure */
typedef struct {
    unsigned short  usEvent;
    unsigned char   ucModeFlag;
    unsigned char   ucMode;
}ST_ATC_CGEREP_PARAMETER;

/* +CTZR command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucReporting;
}ST_ATC_CTZR_PARAMETER;
/* +CGCONTRDP command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucCid;
    unsigned char  ucCidFlag;
}ST_ATC_CGCONTRDP_PARAMETER;


#define D_ATC_PARAM_MAX_NSOCR         4
#define D_ATC_MAX_PARAM_NUM_2         2
#define D_ATC_MAX_PARAM_NUM_3         3
#define D_ATC_MAX_PARAM_NUM_4         4
#define D_ATC_MAX_PARAM_NUM_5         5

typedef struct
{
    unsigned short  usEvent;
#define ATC_NUESTATS_TYPE_RADIO         0
#define ATC_NUESTATS_TYPE_CELL          1
#define ATC_NUESTATS_TYPE_BLER          2
#define ATC_NUESTATS_TYPE_THP           3
#define ATC_NUESTATS_TYPE_APPSMEM       4
#define ATC_NUESTATS_SBAND              5
#define ATC_NUESTATS_TYPE_ALL           6
#define ATC_NUESTATS_TYPE_NOPARAMETER   7
#define ATC_NUESTATS_MAX                8
    unsigned char   mu8Type;
}ST_ATC_NUESTATS_PARAMETER;

typedef struct
{
    unsigned short  usEvent;
    unsigned char   mu8Mode;
    unsigned char   mu8Padding;
    unsigned long   mu32Earfcn;
    unsigned short  mu16Pci;
    unsigned short  mu16Padding;
}ST_ATC_NEARFCN_PARAMETER;

typedef struct
{
    unsigned short  usEvent;
    unsigned char   mu8Num;
    unsigned char   mau8Band[14];
}ST_ATC_NBAND_PARAMETER,
ST_ATC_QBAND_PARAMETER;

typedef struct
{
    unsigned short  usEvent;
    unsigned char   mu8Func;
    unsigned short  mu16Val;
}ST_ATC_NCONFIG_PARAMETER;


/* +NSET command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucInsLen;                                                                     
    unsigned char  aucInsValue[D_ATC_P_NSET_INS_SIZE_MAX];      /* Instruction Name */
    unsigned char  ucParam1Flg;
    unsigned long  ulParam1;
    unsigned char  ucParam2Flg;
    unsigned long  ulParam2;
    unsigned char  ucLen;
    unsigned char  aucData[NVM_MAX_SN_LEN + 1];
} ST_ATC_NSET_PARAMETER;

typedef struct {
    unsigned char  ucImsiLen;
    unsigned char  aucImsi[D_ATC_SIMUUICC_IMSI_SIZE_MAX];
} ST_ATC_SIMUUICC_IMSI_PARAMETER;

typedef struct {
    unsigned char  ucCardNum;
    unsigned char  ucSubCardNum;
} ST_ATC_SIMUUICC_CARDNUM_PARAMETER;

typedef struct {
    unsigned char  ucAppType;
    unsigned char  aucFileId[D_ATC_SIMUUICC_FILE_ID_MAX+1];
    unsigned char  aucFileContent[D_ATC_SIMUUICC_FILE_CONTENT_MAX*2+1];
} ST_ATC_SIMUUICC_FILECONTENT_PARAMETER;

typedef struct {
    unsigned char  aucKey[NAS_AUTH_KEY_LTH*2 + 1];
    unsigned char  aucOp[NAS_AUTH_OP_LTH*2 + 1];
} ST_ATC_SIMUUICC_AUTH_PARAMETER;

/* +SIMUUICC command parameter structure */
typedef struct {
    unsigned short usEvent;
    unsigned char  ucInsLen;                                                                     
    unsigned char  aucInsValue[D_ATC_P_SIMUUICC_INS_SIZE_MAX];      /* Instruction Name */
    union
    {
        ST_ATC_SIMUUICC_CARDNUM_PARAMETER stCardNum;
        ST_ATC_SIMUUICC_FILECONTENT_PARAMETER stFileContent;
        ST_ATC_SIMUUICC_IMSI_PARAMETER    stImsi;
        ST_ATC_SIMUUICC_AUTH_PARAMETER    stAuth;
    } u;
} ST_ATC_SIMUUICC_PARAMETER;

#ifdef LCS_MOLR_ENABLE
#define NMEA_STRING_LEN                                     100
#define THIRD_PARTY_ADDRESS_LEN                             100
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucMethodFlg:1;
    unsigned char               ucVerAccFlg:1;
    unsigned char               ucHorAccFlg:1;
    unsigned char               ucPlane:1;                  /* 0: Control plane
                                                               1: Secure user plane (SUPL)
                                                             */
    unsigned char               ucPadding:1;               
                                                            /* ucMethod   : 0x01 
                                                               ucRepMode  : 0x02
                                                               usTimeOut  : 0x04
                                                               usInterval : 0x08
                                                               ucShapeRep : 0x10
                                                             */
    unsigned char               ucMethod:3;                 /* 0: Unassisted GPS. Autonomous GPS only, no use of assistance data.
                                                               1: A-GPS
                                                               2: A-GNSS
                                                               3: A-GPS and A-GNSS
                                                               4: Basic self location
                                                               5: Transfer to third party
                                                               6: Retrieval from third party
                                                             */
    
    unsigned char               ucEnableReportPos:2;        /* 0: Disables reporting and positioning
                                                               1: +CMOLRN: <NMEA-string>
                                                               2: +CMOLRG: <location_parameters>
                                                               3: +CMOLRG: <location_parameters>and +CMOLRN: <NMEA-string>
                                                             */
    unsigned char               ucHorAccSet:1;              /* 0: Horizontal accuracy not set/specified
                                                               1: Horizontal accuracy set in parameter <hor-acc>
                                                              */
    
    unsigned char               ucVerReq:1;                 /* 0: Vertical coordinate (altitude) is not requested, 2D location fix is acceptable. The parameters <ver-accset>and <ver-acc>do not apply. 
                                                               1: Vertical coordinate (altitude) is requested, 3D location fix is required
                                                             */
    unsigned char               ucVerAccSet:1;              /* 0: Vertical accuracy not set/specified
                                                               1: Vertical accuracy set/specified in parameter <ver-acc>
                                                             */

    unsigned char               ucVelReq:3;                 /* 0: Velocity not requested
                                                               1: Horizontal velocity requested
                                                               2: Horizontal velocity and vertical velocity requested
                                                               3: Horizontal velocity with uncertainty requested
                                                               4: Horizontal velocity with uncertainty and vertical velocity with uncertainty requested
                                                             */

    unsigned char               ucRepMode:1;                /* 0: Single report
                                                               1: Periodic reporting
                                                             */
    unsigned char               ucShapeRep:7;               /* 1: Ellipsoid point
                                                               2: Ellipsoid point with uncertainty circle
                                                               4: Ellipsoid point with uncertainty ellipse
                                                               8: Polygon
                                                               16: Ellipsoid point with altitude
                                                               32: Ellipsoid point with altitude and uncertainty ellipsoid
                                                               64: Ellipsoid arc
                                                             */                                                           
    unsigned char               ucNmeaRepLen;
    unsigned char               ucVerAcc;                   /* 0-127 */
    unsigned char               ucHorAcc;                   /* 0-127 */
//    unsigned char               ucNmeaRep;
//    unsigned char               aucNmeaRep[NMEA_STRING_LEN];
//    unsigned char               ucThirdPartyAddLen;
//    unsigned char               aucThirdPartyAddress[THIRD_PARTY_ADDRESS_LEN];
    
    unsigned short              usTimeOut;                  /* in seconds from 1 to 65535 */
    unsigned short              usInterval;                 /* periodic reporting only : in seconds from 1 to 65535, and must be greater than or equal to <timeout> */  
}ST_ATC_CMOLR_PARAMETER;
#endif

typedef struct {
    unsigned short                usEvent;
#define D_ATC_MAX_SEL_CODE_LEN     16
    unsigned char                 ucLen;
    unsigned char                 ucNoParamValue;
    unsigned char                 aucSelCode[D_ATC_MAX_SEL_CODE_LEN];
} ST_ATC_CPINR_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucVal;
} ST_ATC_CMMS_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucBand;
    unsigned char                 ucPowerClass;
} ST_ATC_NPOWERCLASS_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucCommand;
    unsigned char                 ucParam1Len;
    unsigned char                 ucParam2Len;
#define D_ATC_MAX_PIN_LEN      8    
    unsigned char                 aucParam1[D_ATC_MAX_PIN_LEN];
    unsigned char                 aucParam2[D_ATC_MAX_PIN_LEN];
} ST_ATC_NPIN_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucSnt;
    unsigned char                 ucDataLen;
    unsigned char                 aucData[15];
} ST_ATC_NTSETID_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucCid;
} ST_ATC_NCIDSTATUS_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 n;
} ST_ATC_NGACTR_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucPlmnFlg;
    unsigned char                 ucBandNumFlg;
    unsigned char                 ucOperatorIndex;
    unsigned char                 ucBandNum;
    unsigned short                usOffset;
    unsigned long                 ulPlmn;
    unsigned long                 ulStartFreq;
} ST_ATC_NPOPB_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucN;
} ST_ATC_NIPINFO_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucCid;
} ST_ATC_NQPODCP_PARAMETER,
  ST_ATC_NQPNPD_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucCid;
    unsigned char                 ucRai;
    unsigned char                 ucTypeData;
    unsigned char                 ucSquence;
    unsigned short                usNonIpDataLen;
    unsigned char*                pucNonIpData;
} ST_ATC_NSNPD_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucN;
} ST_ATC_CNEC_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucReporting;
} ST_ATC_NRNPDM_PARAMETER;

typedef struct {
    unsigned short                usEvent;
#define D_ATC_NCPCDPR_IPV4_DNS_REQ  0
#define D_ATC_NCPCDPR_IPV6_DNS_REQ  1
    unsigned char                 ucParam;
    unsigned char                 ucState;
} ST_ATC_NCPCDPR_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucEnable;
} ST_ATC_MNBIOTEVENT_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucIpv6AddressFormatFlag;
    unsigned char                 ucIpv6AddressFormat;
    unsigned char                 ucIpv6SubnetNotationFlag;
    unsigned char                 ucIpv6SubnetNotation;
    unsigned char                 ucIpv6LeadingZerosFlag;
    unsigned char                 ucIpv6LeadingZeros;
    unsigned char                 ucIpv6CompressZerosFlag;
    unsigned char                 ucIpv6CompressZeros;
} ST_ATC_CGPIAF_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucMode;
    unsigned char                 ucSimwcEnableFlag;
    unsigned char                 ucSimwcEnable;
} ST_ATC_NSIMWC_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucThreshold;
} ST_ATC_CUPREFERTH_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucMode;
    unsigned char                 ucEarfcnNum;
    unsigned long                 aulEarfcnList[D_ATC_EARFCN_MAX_NUM];
    unsigned short                usPci;
} ST_ATC_NLOCKF_PARAMETER;

typedef struct
{
    unsigned short  usEvent;
    unsigned char   ucVal;
}ST_ATC_QENG_PARAMETER;

typedef struct
{
    unsigned short  usEvent;
    unsigned char   ucVal;
    unsigned char   ucFunc[10];
}ST_ATC_QCFG_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucType;
    unsigned short                usDataLen;
    unsigned char*                pucData;
} ST_ATC_PSTEST_PARAMETER;

typedef struct {
    unsigned short                usEvent;
#define D_QNIDD_OPTION_CREATE_ACCOUNT       0
#define D_QNIDD_OPTION_ESTABLISH_CONNECT    1
#define D_QNIDD_OPTION_ACTIVE_CONNECT       2
#define D_QNIDD_OPTION_SEND_NONIP_DATA      3
    unsigned char                 ucOption;
    unsigned char                 ucApnLen;
    unsigned char                 aucApnValue[64];
    unsigned char                 ucUsernameLen;
    unsigned char                 aucUsername[16];
    unsigned char                 ucPasswordLen;
    unsigned char                 aucPassword[16];
    unsigned char                 ucAccountID;
    unsigned char                 ucNIDD_ID;
    unsigned short                usDataLen;
    unsigned char                *pucData;
} ST_ATC_QNIDD_PARAMETER;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucOperatorIndex;
    unsigned char                 ucFreqFlg;
    unsigned long                 ulEarfcn;
} ST_ATC_PRESETFREQ_PARAMETER;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucMode;
    unsigned char                 ucEarfcnFlg;
    unsigned long                 ulEarfcn;
    unsigned short                usPci;
    unsigned char                 ucPciFlg;
    unsigned char                 ucEarfcnOffset;
} ST_ATC_QLOCKF_PARAMETER;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucATCid;
    unsigned char                 ucPdpTypeFlg;
    unsigned char                 ucPdpTypeValue;
    unsigned char                 ucApnLen;
    unsigned char                 ucUsernameLen;
    unsigned char                 ucPasswordLen;
    unsigned char                 ucAuthProt;
    unsigned char                 aucUsername[D_PCO_AUTH_MAX_LEN];
    unsigned char                 aucPassword[D_PCO_AUTH_MAX_LEN];
    unsigned char                 aucApnValue[FACTORY_USER_SET_APN_LEN];
} ST_ATC_QICSGP_PARAMETER;

typedef union
{
    ST_ATC_CGDCONT_PARAMETER           stCgdcontParam;
    ST_ATC_DPS_PARAMETER               stDpsParam;
    ST_ATC_PSMR_PARAMETER              stPsmrParam;
    ST_ATC_CMD_PARAMETER               stCmdParam;
    ST_ATC_CGSN_PARAMETER              stCgsnParam;
    ST_ATC_CGDSCONT_PARAMETER          stCgdscontParam;
    ST_ATC_CGEQOS_PARAMETER            stCgeqosParam;
    ST_ATC_CGCMOD_PARAMETER            stCgcmodParam;
    ST_ATC_CGTFT_PARAMETER             stCgtftParam;
    ST_ATC_CEREG_PARAMETER             stCeregParam;
    ST_ATC_CSODCP_PARAMETER            stCsodcpParam;
    ST_ATC_CGAPNRC_PARAMETER           stCgapnrcParam;
    ST_ATC_CRTDCP_PARAMETER            stCrtdcpParam;
    ST_ATC_CEDRXS_PARAMETER            stCedrxsParam;
    ST_ATC_CPSMS_PARAMETER             stCpsmsParam;
    ST_ATC_CFUN_PARAMETER              stCfunParam;
    ST_ATC_CGACT_PARAMETER             stCgactParam;
#ifdef NBIOT_SMS_FEATURE    
    ST_ATC_CNMI_PARAMETER              stCnmiParam;
    ST_ATC_CSCA_PARAMETER              stCscaParam;
    ST_ATC_AP_SMS_PDU_PARAMETER        stAtcApPduParam;
    ST_ATC_PDU_TPDU_PARAMETER          stPduTpduParam;
#endif
    ST_ATC_CGDATA_PARAMETER            stCgdataParam;
    ST_ATC_CGPADDR_PARAMETER           stCgpaddrParam;
    ST_ATC_CMUX_PARAMETER              stCmuxParam;
    ST_ATC_COPS_PARAMETER              stCopsParam;
    ST_ATC_CSIM_PARAMETER              stCsimParam;
    ST_ATC_CCHO_PARAMETER              stCchoParam;
    ST_ATC_CCHC_PARAMETER              stCchcParam;
    ST_ATC_CGLA_PARAMETER              stCglaParam;
    ST_ATC_CRSM_PARAMETER              stCrsmParam;
    ST_ATC_CCIOTOPT_PARAMETER          stCciotoptParam;
    ST_ATC_CGEQOSRDP_PARAMETER         stCgeqosrdpParam; 
    ST_ATC_CPWD_PARAMETER              stCpwdParam;
    ST_ATC_CPIN_PARAMETER              stCpinParam;
    ST_ATC_CLCK_PARAMETER              stClckParam;
    ST_ATC_CGEV_PARAMETER              stCgevParam;
    ST_ATC_CIPCA_PARAMETER             stCipcaParam;
    ST_ATC_CGAUTH_PARAMETER            stCgauthParam;
    ST_ATC_CGEREP_PARAMETER            stCgerepParam;
    ST_ATC_CTZR_PARAMETER              stCtzrParam;
    ST_ATC_CGCONTRDP_PARAMETER         stCgcontrdpParam;
    ST_ATC_NUESTATS_PARAMETER          stNuestatsParam;
    ST_ATC_NEARFCN_PARAMETER           stNearfcnParam;
    ST_ATC_NBAND_PARAMETER             stNbandParam;
    ST_ATC_NCONFIG_PARAMETER           stNconfigParam;
    //ST_ATC_SOCKET_INFO_STRU            stSocketInfoParam;
    //ST_ATC_UDP_DATA_STRU               stUdpDataParam;
    //ST_ATC_PING_DATA_STRU              stPingDataParam;
    ST_ATC_NSET_PARAMETER              stNsetParam;
    ST_ATC_CPINR_PARAMETER             stCpinrParam;
    ST_ATC_CMMS_PARAMETER              stCmmsParam;
    ST_ATC_NPOWERCLASS_PARAMETER       stNpowerclassParam;
    ST_ATC_NPIN_PARAMETER              stNpinParam;
    ST_ATC_NTSETID_PARAMETER           stNtsetIdParam;
    ST_ATC_NCIDSTATUS_PARAMETER        stNcidstatusParam;
    ST_ATC_NGACTR_PARAMETER            stNgactrParam;
    ST_ATC_NPOPB_PARAMETER             stNpopbParam;
    ST_ATC_NIPINFO_PARAMETER           stNipInfoParam;
    ST_ATC_NQPODCP_PARAMETER           stNqpodcpParam;
    ST_ATC_NSNPD_PARAMETER             stNsnpdParam;
    ST_ATC_CNEC_PARAMETER              stCnecParam;
    ST_ATC_NCPCDPR_PARAMETER           stNcpcdprParam;
    ST_ATC_MNBIOTEVENT_PARAMETER       stMnbiotEventParam;
    ST_ATC_CGPIAF_PARAMETER            stCgpiafParam;
    ST_ATC_CUPREFERTH_PARAMETER        stCuPreferThParam;
    ST_ATC_NLOCKF_PARAMETER            stNlockfParam;
    ST_ATC_QCGDEFCONT_PARAMETER        stQcgdefcontParam;
    ST_ATC_QBAND_PARAMETER             stQbandParam;
    ST_ATC_QENG_PARAMETER              stQengParam;
    ST_ATC_QCFG_PARAMETER              stQcfgParam;
    ST_ATC_NSIMWC_PARAMETER            stNsimwcParam;
    ST_ATC_PSTEST_PARAMETER            stPstestParam;
    ST_ATC_QNIDD_PARAMETER             stQniddParam;
    ST_ATC_PRESETFREQ_PARAMETER        stPresetpreqParam;
    ST_ATC_QLOCKF_PARAMETER            stQlockfParam;
    ST_ATC_QICSGP_PARAMETER            stQicsgpParam;
}UN_ATC_CMD_EVENT;

/*******************************************************************************
* The message for PS send to ATC_AP
*******************************************************************************/
/****************************STRU of internal message *************************/
typedef struct  
{
#define  EPS_IPV6_LEN          16
    unsigned char     aucIpv6Addrs[EPS_IPV6_LEN];
}LTE_IPV6_ADDRESS_STRU;

typedef struct  
{
#define  EPS_IPV4_LEN           4
    unsigned char     aucIpv4Addrs[EPS_IPV4_LEN];
}LTE_IPV4_ADDRESS_STRU;

typedef struct  
{
    unsigned char                  ucCid;
    unsigned char                  ucPdpaddr1Flg;
    LTE_IPV4_ADDRESS_STRU          Pdpaddr1;
    unsigned char                  ucPdpaddr2Flg;
    LTE_IPV6_ADDRESS_STRU          PdpAddr2;
}LTE_ESM_PDP_ADRR_INFO_READ_STRU;

typedef struct  
{
    unsigned char                     ucCidNum;
    LTE_ESM_PDP_ADRR_INFO_READ_STRU   aPdpAddr[D_MAX_CNT_CID];
}API_ESM_PDP_ADRR_INFO_READ_STRU;

typedef struct  
{
    unsigned char                     ucCidNum;
    unsigned char                     aucCid[D_MAX_CNT_CID];
}API_ESM_DEFINED_CID_LIST_STRU;

typedef struct
{
    unsigned char        ucCid;                     

#define D_EPS_CONTEXT_STA_DEACT     0
#define D_EPS_CONTEXT_STA_ACTIVE    1
    unsigned char        ucState;                   
} EPS_CID_STATE_INFO;

typedef struct
{
    unsigned char        ucValidNum;                
    EPS_CID_STATE_INFO   aCidSta[D_MAX_CNT_CID];               
} API_EPS_CTXT_STATE_INFO_STRU;

typedef struct  
{
    unsigned char             ucCid;
    unsigned char             ucAdditionExcepReportFlg;                                         /* 0: NOT allowed, 1: allowed  */
    unsigned long             ulUplinkTimeUnit;                                                 /* unit is 1 min  */
    unsigned long             ulMaxUplinkRate;
    unsigned short            usMaxUplinkMsgSize;
}EPS_APN_RATE_CONTRL_STRU;

/* API_EPS_CGDSCONT_Para_Set_Req */
typedef struct
{
    unsigned char        ucCid;                                                         /* 1-10 */
    unsigned char        ucDelFlg;
    unsigned char        ucP_cid;                                                       /* 0-10 */
    unsigned char        ucImCnSignallingFlagIndFlg;
    unsigned char        ucImCnSignallingFlagInd;
} API_EPS_CGDSCONT_PARA_SET_REQ_STRU;

typedef struct
{
    unsigned char     ucPacketFilterId:5;                                               /* Value range is from 1 to 16            */
    unsigned char     ucDirection:2; 
    unsigned char     ucProtocolNum_NextHeaderFlg:1;

    unsigned char     ucIpsecSPIFlg:1;
    unsigned char     ucFlowLabelFlg:1;
    unsigned char     ucLocalPortRangeLen:2;                                              /* <local port range>: string type. The string is given as dot-separated numeric (0-65535) parameters on the form "f.t".*/ 
    unsigned char     ucRemotePortRangeLen:2;                                             /* remote port range>: string type. The string is given as dot-separated numeric (0-65535) parameters on the form "f.t". */
    unsigned char     ucTypeOfServiceAndMaskLen:2;                                        /* <type of service (tos) (ipv4) and mask / traffic class (ipv6) and mask>: */
      
    unsigned char     ucEvaluationPrecedenceIndex;                                      /* The value range is from 0 to 255       */
    unsigned char     ucRemoteAddrAndSubMaskLen;                                        /* Length of local_addr and subnet_mask, 8 for IPv4,32 for IPv6    */
    unsigned char     aucRemoteAddrAndSubMask[32];                                      /* remote address and subnet mask */

    unsigned char     ucProtocolNum_NextHeader;                                         /* protocol number (ipv4) / next header (ipv6)>: integer type. Value range is from 0 to 255. */
                                                          /* <ipsec security parameter index (spi)>: numeric value in hexadecimal format. The value range is from 00000000 to FFFFFFFF.  */

    unsigned char     aucTypeOfServiceAndMask[2];                                       /*string type. The string is given as dot-separated numeric (0-255) parameters on the form "t.m". */
                                                         /* <flow label (ipv6)>: numeric value in hexadecimal format. The value range is from 00000 to FFFFF. Valid */
                                                                                        /*  for IPv6 only. */
                                                                                        /*  0  Pre-Release 7 TFT filter (see 3GPP TS 24.008 [8], table 10.5.162) */
                                                                                        /* 1 Uplink,  2 Downlink ,3 Birectional (Up & Downlink) */

    unsigned char     ucLocalAddrAndSubMaskLen;
    unsigned char     aucLocalAddrAndSubMask[32];                                       /* <local address and subnet mask>: string type. The string is given as dot-separated numeric (0-255) 
                                                                                           parameters on the form: 
                                                                                           "a1.a2.a3.a4.m1.m2.m3.m4" for IPv4 or 
                                                                                           "a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a11.a12.a13.a14.a15.a16.m1.m2.m3.m4.m5.m6.m7.m8.m9.m10.m11.m12.m13.
                                                                                           m14.m15.m16", for IPv6. 
                                                                                           When +CGPIAFis supported, its settings can influence the format of this parameter returned with the read form 
                                                                                           of +CGTFT */

    unsigned short    ausLocalPortRange[2];
    unsigned short    ausRemotePortRange[2];

    unsigned long     ulIpsecSPI;
    unsigned long     ulFlowLabel; 
} EPS_PF_INFO;

/* API_EPS_CGDSCONT_Para_Read_Req */
typedef struct  
{
    unsigned char                           ucValidNum;
    API_EPS_CGDSCONT_PARA_SET_REQ_STRU       aSPdpCtxtPara[D_MAX_CNT_CID - 1];
}API_EPS_CGDSCONT_PARA_READ_REQ_STRU;

/* API_EPS_Qos_Para_Set_Req */
typedef struct
{
    unsigned char        ucQci;           
    unsigned char        ucBrFlg;                                                       /* For all non-GBR QCIs,the maximum and guaranteed bit rates shall be omitted */
    unsigned long        ulDlGbr;
    unsigned long        ulUlGbr;
    unsigned long        ulDlMbr;
    unsigned long        ulUlMbr;
} EPS_QOS_INFO;

typedef struct
{
    unsigned char           ucCid;                                                      /* rang: 0-10                       */
    unsigned char           ucDelFlg;
    EPS_QOS_INFO            stQosInfo;
}API_EPS_QOS_PARA_SET_REQ_STRU;

/* API_EPS_Qos_Para_Read_Req */
typedef struct
{
    unsigned char           ucCid;                                                      /* rang: 0-10                       */
    EPS_QOS_INFO            stQosInfo;
}EPS_QOS_PARA_READ_STRU;

typedef struct
{
    unsigned char           ucValidNum;
    EPS_QOS_PARA_READ_STRU  aQosInfo[10];
}API_EPS_QOS_PARA_READ_REQ_STRU;

/* API_EPS_OpeSelInf_Read_Req */ 
typedef struct
{
#define D_ATC_AP_COPS_MODE_MANUAL                    1
#define D_ATC_AP_COPS_MODE_MANUAL_AUTO               4
    unsigned char     ucMode;
    unsigned char     ucPlmnSelFlg;
#define PLMN_FORMAT_NUMERIC 2
    unsigned char     ucFormat;
    unsigned long     ulPlmnNum;
#define PLMN_ACT_E_UTRAN_NB 9
    unsigned char     ucAct;
    unsigned char     ucPsAttachFlg;
} API_EPS_OPESELINFO_READ_REQ_STRU;

typedef struct
{        
    unsigned char       ucN;
#define EPS_NO_SUPPORT                    0
#define EPS_SUPPORT_CP_ONLY               1
#define EPS_SUPPORT_UP_ONLY               2
#define EPS_DEFAULT_SUPPORT_UPCP          3   /*NAS Default value*/
    unsigned char       ucSupptUeOpt;
#define EPS_NO_PREFERENCE                 0
#define EPS_DEFAULT_PREFERENCE_FOR_CP     1 /*NAS Default value*/
#define EPS_PREFERENCE_FOR_UP             2
    unsigned char       ucPreferOpt;
    unsigned char       ucCciotoptN;
} API_CCIOTOPT_CONFIG_STRU;

/*  API_EPS_CEDRXS_Para_Set_Req   */
typedef struct  
{
    unsigned char  ucMode;
#define EMM_EDRX_DEFAULT_VALUE              2                                           /* The minimum EDRXValue */
#define EMM_EDRX_INVALID_VALUE              0xFF                                        /* The invalid EDRXValue */
    unsigned char  ucIsDefaultEDRXVal;
    unsigned char  ucEDRXValue;                                                         /* Do not support 0,1,4,6,7,8,   24008    10.5.5.32  */
#define EMM_PTW_DEFAULT_VALUE               3                                           /* The Default PTWVALUE */
#define EMM_PTW_INVALID_VALUE               0xFF                                        /* The invalid PTWVALUE */
    unsigned char  ucPTWValue;         
}EPS_CEDRXS_PARA_STRU;

typedef struct  
{
#define LNB_EDRX_NOT_USING                  0
#define LNB_EDRX_E_UTRAN_NBS1               5
    unsigned char                                   ucNptwEDrxMode;
    unsigned char                                   ucActType;
    unsigned char                                   ucReqDRXValue;                              /* Requested eDRX value */
    unsigned char                                   ucReqPagingTimeWin;                          /* Requested provided Paging_time_window */
    unsigned char                                   ucNWeDRXValueFlg;
    unsigned char                                   ucNWeDRXValue;                               /* NW provided eDRX value */
    unsigned char                                   ucPagingTimeWinFlg;
    unsigned char                                   ucPagingTimeWin;                             /* Paging_time_window */
}API_NWCEDRXS_PARA_STRU;

typedef struct
{
    unsigned char        ucApnAmbrFlag;
    unsigned long        ulDlApnAmbr;                                                   /* APN aggregate maximum bit rate for downlink  */
    unsigned long        ulUlApnAmbr;                                                   /* APN aggregate maximum bit rate for uplink    */
} EPS_APN_AMBR_INFO;

/* CGEQOSRDP */
typedef struct
{
    unsigned char           ucCid;                                                      /* rang: 0-10                       */
    EPS_QOS_INFO            stQosInfo;
    EPS_APN_AMBR_INFO       stApnAmbrInfo;
}EPS_CGEQOSRDP_PARA_READ_STRU;
typedef struct
{
    unsigned char                ucValidNum;
    EPS_CGEQOSRDP_PARA_READ_STRU  aQosApnAmbrInfo[D_MAX_CNT_CID];
}API_EPS_CGEQOSRDP_PARA_READ_REQ_STRU;

typedef struct
{
    unsigned char       ucPriDnsAddr_IPv4Flg:1;
    unsigned char       ucSecDnsAddr_IPv4Flg:1;
    unsigned char       ucPriDnsAddr_IPv6Flg:1;
    unsigned char       ucSecDnsAddr_IPv6Flg:1;
    unsigned char       :4;
    unsigned char       ucPriDnsAddr_IPv4[4];                                           /* Primary IPv4 DNS Address */               
    unsigned char       ucSecDnsAddr_IPv4[4];                                           /* Secondary IPv4 DNS Address */ 
    unsigned char       ucPriDnsAddr_IPv6[16];
    unsigned char       ucSecDnsAddr_IPv6[16];
} EPS_DNS_ADDR_STRU;

/* API_EPS_PDN_IPDNS_IND_STRU */
typedef struct
{
    unsigned char       ucCid;                                                          /* rang: 0-10                       */
    unsigned char       ucPdpType;                                                      /* PDP type number value            */
    unsigned char       aucIPv4Addr[4];
    unsigned char       aucIPv6Addr[16];
    EPS_DNS_ADDR_STRU   stDnsAddr;
}API_EPS_PDN_IPDNS_IND_STRU;

typedef struct
{
//#define D_ATC_ADDR_LEN     32
    unsigned char   ucPdpType;
    unsigned char   ucCid;
    unsigned char   ucBearerId;
    unsigned char   ucApnLen;
    unsigned char   aucApn[D_APN_LEN];
    unsigned char   aucPdpAddrValue[12];          /* octet:0-7: IPV6, octet: 8-11: IPV4 */
    //unsigned char   ucGwAddrLen;
    //unsigned char aucGwAddr[2];
    //unsigned char   ucDNSPrimAddrLen;
    //unsigned char   aucDNSPrimAddr[D_ATC_ADDR_LEN];
    //unsigned char   ucDNSSecAddrLen;
    //unsigned char   aucDNSSecAddr[D_ATC_ADDR_LEN];
    //unsigned char   ucPCSCFPrimAddrLen;
    //unsigned char   aucPCSCFPrimAddr[D_ATC_ADDR_LEN];
    //unsigned char   ucPCSCFSecAddrLen;
    //unsigned char   aucPCSCFSecAddr[D_ATC_ADDR_LEN];
    //unsigned char   ucIMCNSignallingFlag;
    //unsigned char   ucIMCNSignalling;
    //unsigned char   ucLIPAIndFlag;
    //unsigned char   ucLIPAInd;
    unsigned char   ucIPv4MTUFlag;
    unsigned short  usIPv4MTU;
    //unsigned char   ucWLANOffloadFlag;
    //unsigned char   ucWLANOffload;
    //unsigned char   ucLocalAddrIndFlag;
    //unsigned char   ucLocalAddrInd;
    unsigned char   ucNonIPMTUFlag;
    unsigned short  usNonIPMTU;
    unsigned char   ucServingPLMNRateCtrValueFlag;
    unsigned short  usServingPLMNRateCtrValue;
    EPS_DNS_ADDR_STRU   stDnsAddr;
}EPS_CGCONTRDP_DYNAMIC_INFO;

typedef struct
{
    unsigned char                 ucValidNum;
    EPS_CGCONTRDP_DYNAMIC_INFO    aucPdpDynamicInfo[D_MAX_CNT_CID];
}API_CGCONTRDP_READ_DYNAMIC_PARA_STRU;

/* Bind_EPS_OpeList_Search_Cnf */
typedef struct
{
#define D_NETWORK_STA_UNKNOWN       0
#define D_NETWORK_STA_AVAILABLE     1
#define D_NETWORK_STA_REGISTER      2
#define D_NETWORK_STA_FORBIDDEN     3
    unsigned char        ucState;                   
    unsigned long                aulPlmnNum;                                                    /* MCC2+MCC1+MNC3+MCC3+MNC2+MNC1        */
} EPS_OPERATOR_INF;

typedef struct
{
    unsigned char               ucOpeInfCnt;
    EPS_OPERATOR_INF            aOpeInf[5];               
    unsigned char               ucErrCode;                                              /* when ucOpeInfCnt is 0,ucErrCode is valid     */
} API_EPS_OPELIST_SRCH_CNF_STRU;

typedef struct
{
    unsigned char       ucLocalTimeZoneFlg;                                             /* ucLocalTimeZone valid flag               */
    unsigned char       ucLocalTimeZone;                                                /* Local Time Zone                          */
    unsigned char       ucUtAndLtzFlg;                                                  /* aucUtAndLtz valid flag                   */
#define D_ATC_UTANDLTZ_LEN    7
    unsigned char       aucUtAndLtz[D_ATC_UTANDLTZ_LEN];                                /* Universal Time And Local Time Zone       */
    unsigned char       ucNwDayltSavTimFlg;                                             /* ucNwDayltSavTim valid flag               */
    unsigned char       ucNwDayltSavTim; 
}LNB_NAS_LOCAL_TIME_STRU;

/* API_EMM_Read_Register_State  */
typedef struct
{
    unsigned int        ucOnlyUserRegEventIndFlg:1;
    unsigned int        OP_TacLac       : 1;
    //unsigned int        OP_Rac            : 1;
    unsigned int        OP_Act            : 1;
    unsigned int        OP_CellId        : 1;
    unsigned int        OP_CauseType    : 1;
    unsigned int        OP_RejectCause    : 1;
    unsigned int        OP_ActiveTime    : 1;
    unsigned int        OP_PeriodicTAU    : 1;
    unsigned int        OP_Spare    : 24;
#define LNB_EPS_REG_STATUS_NOT_REGISTERED       0
#define LNB_EPS_REG_STATUS_REG_HPLMN            1
#define LNB_EPS_REG_STATUS_REGINI_OR_SEARCH     2
#define LNB_EPS_REG_STATUS_REG_DENIED           3
#define LNB_EPS_REG_STATUS_UNKNOWN              4
#define LNB_EPS_REG_STATUS_REG_ROAMING          5
#define LNB_EPS_REG_STATUS_REG_EMERG_ONLY       8
    unsigned char        ucIndPara;   
    unsigned char        ucEPSRegStatus;
    #define LNB_E_UTRAN_NBS1_MODE               9
    unsigned char        ucAct;
    unsigned short       usTacOrLac;
   // unsigned short       usRac;
    unsigned int         ulCellId;
#define LNB_REJECT_EMM_CAUSE_VALUE              0
#define LNB_REJECT_SPEC_CAUSE_VALUE             1
    unsigned char        ucCauseType;
    unsigned char        ucRejectCause;
    unsigned char        ucActiveTime;
    unsigned char        ucPeriodicTAU;
} API_EPS_REG_STATUS_STRU;

/* API_IMSI_Read_Req */
typedef struct
{
    unsigned char        ucImsiLen;
#define D_EPS_IMSI_MAX_LEN                16
    unsigned char        aucImsi[D_EPS_IMSI_MAX_LEN];                                       /* read the IMEI (International Mobile station Equipment Identity) */
} API_EPS_IMSI_READ_STRU;

typedef struct
{        
    unsigned int        OP_State       : 1;
    unsigned int        OP_Access    : 1;
    unsigned int        OP_Spare    : 30;
    unsigned char        ucIndPara;   
#define EPS_SIGNALLING_IDLE           0
#define EPS_SIGNALLING_CONNECTED      1
    unsigned char        ucMode;
#define EPS_E_UTRAN_CONNECTED_STATE   7
    unsigned char        ucState;
#define EPS_E_UTRAN_TDD               3
#define EPS_E_UTRAN_FDD               4
    unsigned char       ucAccess;
} API_SIGNAL_CONN_STATUS_STRU;

#define     LRRC_NAS_MAX_CELL_NUM            6
typedef struct
{
    unsigned long               ulDlEarfcn;
    unsigned short              usPhyCellId;
    signed short               sRsrp;
    signed short               sRsrq;
    signed short               sSinr;
    signed short               sRssi;
}API_CELL_INFO_STRU;

typedef struct
{
    unsigned char ucCellNum;
    unsigned char ucEcl;
    unsigned char ucOperationMode;
    API_CELL_INFO_STRU aNCell[LRRC_NAS_MAX_CELL_NUM];
}API_CELL_LIST_STRU;

typedef struct
{
    unsigned char       ucSupportBandNum;
    unsigned char       aucSuppBand[G_LRRC_SUPPORT_BAND_MAX_NUM];
}API_SUPP_BAND_LIST_STRU;

typedef struct
{
    unsigned char       ucCid;
#define   D_STATUS_AVAILABLE   0
#define   D_STATUS_NOT_EXIST   1
#define   D_STATUS_FLOW_CTRL   2
#define   D_STATUS_BACKOFF     3
    unsigned char       ucStatus;
    unsigned long       ulBackOffVal;
} EPS_CID_STATUS_INFO;

typedef struct
{
    unsigned char             ucNum;
    EPS_CID_STATUS_INFO       aCidStaus[D_MAX_CNT_CID];
} API_EPS_CID_STATUS_STRU;


#ifdef LCS_MOLR_ENABLE
/********************************** LCS MOLR RESULT ********************************/
typedef struct { 
    unsigned char                           ucUncertnSemiMajor;            /* INTEGER(0..127) */
    unsigned char                           ucUncertnSemiMin;              /* INTEGER(0..127) */
    unsigned char                           ucOrientMajorAxis;             /* INTEGER(0..179) */
} LCS_UNCERTN_ELLIP_STRU;

typedef struct { 
    unsigned char                           ucHeightAboveSurface;
    unsigned short                          usAlti;
} LCS_ALTI_STRU;

typedef struct { 
    unsigned char                           ucLatitudeSign;
    unsigned int                            uiDegreesLatitude;            /* INTEGER(0..8388607) */
} LCS_LATITUDE_STRU;

typedef struct { 
    LCS_LATITUDE_STRU               tLatitude;
    INT                             iDegreesLongitude;            /* INTEGER(-8388608..8388607) */
} LCS_COOR_STRU,LCS_SHAPE_ELLIP_POINT_STRU;

typedef struct { 
    LCS_COOR_STRU                   tCoordinate;
    unsigned char                           ucUncertn;                    /* INTEGER(0..127) */
} LCS_SHAPE_ELLIP_POINT_UNCERT_CIRCLE_STRU;

typedef struct { 
    LCS_COOR_STRU                   tCoordinate;
    LCS_UNCERTN_ELLIP_STRU          tUncertEllipse;
    unsigned char                           ucConfidence;                  /* INTEGER(0..100) */
} LCS_SHAPE_ELLIP_POINT_UNCERT_ELLIP_STRU;

typedef struct { 
    unsigned char                           ucCnt;
    LCS_COOR_STRU                   atCoordinate[15];
} LCS_SHAPE_POLYGON_STRU;

typedef struct { 
    LCS_COOR_STRU                   tCoordinate;
    LCS_ALTI_STRU                   tAltitude;
} LCS_SHAPE_ELLIP_POINT_ALT_STRU;

typedef struct { 
    LCS_COOR_STRU                   tCoordinate;
    LCS_ALTI_STRU                   tAltitude;
    LCS_UNCERTN_ELLIP_STRU          tUncertEllipse;
    unsigned char                           ucUncertnAlti;                 /* INTEGER(0..127) */
    unsigned char                           ucConfidence;                  /* INTEGER(0..100) */
} LCS_SHAPE_ELLIP_POINT_ALT_UNCERT_STRU;

typedef struct { 
    LCS_COOR_STRU                   tCoordinate;
    unsigned short                          usInnerRadius;                  /* INTEGER(0..65535) */
    unsigned char                           ucUncertnRadius;                /* INTEGER(0..127) */
    unsigned char                           ucOffsetAngle;                  /* INTEGER(0..179) */
    unsigned char                           ucIncludedAngle;                /* INTEGER(0..179) */
    unsigned char                           ucConfidence;                   /* INTEGER(0..100) */
} LCS_SHAPE_ELLIP_ARC_STRU;

typedef struct { 
#define LCS_SHAPE_ELLIP_POINT                           0
#define LCS_SHAPE_ELLIP_POINT_UNCERT_CIRCLE             1
#define LCS_SHAPE_ELLIP_POINT_UNCERT_ELLIP              3
#define LCS_SHAPE_POLYGON                               5
#define LCS_SHAPE_ELLIP_POINT_ALT                       8
#define LCS_SHAPE_ELLIP_POINT_ALT_UNCERT                9
#define LCS_SHAPE_ELLIP_ARC                             10
    unsigned char                                               ucShapeType;
    union  {
        LCS_SHAPE_ELLIP_POINT_STRU                      tEllipPoint;
        LCS_SHAPE_ELLIP_POINT_UNCERT_CIRCLE_STRU        tEllipPointUncertCircle;
        LCS_SHAPE_ELLIP_POINT_UNCERT_ELLIP_STRU         tEllipPointUncertEllip;
        LCS_SHAPE_POLYGON_STRU                          tPolygon;
        LCS_SHAPE_ELLIP_POINT_ALT_STRU                  tEllipPointWithAlti;
        LCS_SHAPE_ELLIP_POINT_ALT_UNCERT_STRU           tEllipPointWithAltiUncert;
        LCS_SHAPE_ELLIP_ARC_STRU                        tEllipArc;
    } u;
} LCS_SHAPE_DATA_STRU;

typedef struct
{
    unsigned short                                              usBearing;              
    unsigned short                                              usSpeed; 
} LCS_VEL_HORIZONTAL_STRU;

typedef struct
{
    unsigned char                                               ucDirect;              
    unsigned short                                              ucSpeed; 
} LCS_VEL_VERTICAL_STRU;

typedef struct { 
    unsigned char                                               OP_VertSpeed            : 1;
    unsigned char                                               OP_HorUncert            : 1;
    unsigned char                                               OP_VertUncert           : 1;
    unsigned char                                               OP_Spare                : 5;

    LCS_VEL_HORIZONTAL_STRU                             tHorizSpeed;
    LCS_VEL_VERTICAL_STRU                               tVertSpeed;             
    unsigned char                                               ucHorSpeedUncert;         
    unsigned char                                               ucVertSpeedUncert;
} LCS_VELOCITY_DATA_STRU;
#endif

/************************ PS->ATC_AP message ****************************/
//OK/ERR
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucResult;
    unsigned short              usErrCode;
#define   ATC_CMD_TYPE_CMEE  0
#define   ATC_CMD_TYPE_CMS   1
    unsigned char               ucCmdType;
    unsigned char               ucCpinOkFlg;
} ST_ATC_AP_CMD_RST;

//+CGSN[=0/1/2/3]
typedef struct
{
    unsigned short              usEvent;
#define   D_ATC_CGSN_TYPE_SN      0
#define   D_ATC_CGSN_TYPE_IMEI    1
#define   D_ATC_CGSN_TYPE_IMEISV  2
#define   D_ATC_CGSN_TYPE_SVN     3
    unsigned char               snt;
    unsigned char               ucLen;
    unsigned char               aucData[4];
} ATC_MSG_CGSN_CNF_STRU;

//+CEREG?
typedef struct
{
    unsigned short              usEvent;
    API_EPS_REG_STATUS_STRU     tRegisterState;
} ATC_MSG_CEREG_R_CNF_STRU;

//+CGATT?
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucState;
} ATC_MSG_CGATT_R_CNF_STRU;

//+CIMI
typedef struct
{
    unsigned short              usEvent;
    API_EPS_IMSI_READ_STRU      stImsi;
} ATC_MSG_CIMI_CNF_STRU;

typedef struct
{
    unsigned char               ucAtCid;
    unsigned char               ucPdpType;
    unsigned char               aucApnValue[D_APN_LEN];
    unsigned char               ucH_comp;
    unsigned char               ucNSLPI;
    unsigned char               ucSecurePco;
} ATC_PDP_CONTEXT_STRU;

//+CGDCONT?
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucNum;
    ATC_PDP_CONTEXT_STRU        stPdpContext[D_MAX_CNT_CID];
} ATC_MSG_CGDCONT_R_CNF_STRU;

//+CFUN?
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucFunMode;
} ATC_MSG_CFUN_R_CNF_STRU;

//+CMEE=<val>/+CMEE?
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucErrMod;
} ATC_MSG_CMEE_CNF_STRU,
  ATC_MSG_CMEE_R_CNF_STRU;

//+CESQ
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucRxlev;
    unsigned char               ucBer;
    unsigned char               ucRscp;
    unsigned char               ucEcno;
    unsigned char               ucRsrq;
    unsigned char               ucRsrp;
    char                        cSinr;
} ATC_MSG_CESQ_CNF_STRU,
  ATC_MSG_CESQ_IND_STRU;

//+CSQ
typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucRxlev;
    unsigned char               ucBer;
} ATC_MSG_CSQ_CNF_STRU;

//+CGPADDR[=<cid>]
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_PDP_ADRR_INFO_READ_STRU       stPara; 
} ATC_MSG_CGPADDR_CNF_STRU;

//+CGPADDR=?
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_DEFINED_CID_LIST_STRU         stPara; 
} ATC_MSG_CGPADDR_T_CNF_STRU;

//+CGACT?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucRusult;
    API_EPS_CTXT_STATE_INFO_STRU          stState; 
} ATC_MSG_CGACT_R_CNF_STRU;

//+CRTDCP=?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCrtdcpRepValue;
} ATC_MSG_CRTDCP_R_CNF_STRU;

//+CEDRXS?;+NPTWEDRXS?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCedrxMode;
    API_NWCEDRXS_PARA_STRU                stPara;
} ATC_MSG_CEDRXS_R_CNF_STRU,
  ATC_MSG_NPTWEDRXS_R_CNF_STRU;

//+CPSMS?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucMode;
    unsigned char                         ucReqActTime;
    unsigned char                         ucReqPeriTAU;
} ATC_MSG_CPSMS_R_CNF_STRU;

//+CGAPNRC[=<cid>]
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
    EPS_APN_RATE_CONTRL_STRU              stEpsApnRateCtl[D_MAX_CNT_CID];
} ATC_MSG_CGAPNRC_CNF_STRU;

//+CGAPNRC=?
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_DEFINED_CID_LIST_STRU          stPara;
} ATC_MSG_CGAPNRC_T_CNF_STRU;

//+CSCON?
typedef struct
{
    unsigned short                        usEvent;
    API_SIGNAL_CONN_STATUS_STRU           stPara;
} ATC_MSG_CSCON_R_CNF_STRU;

//+NL2THP?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucL2THPFlag;
} ATC_MSG_NL2THP_R_CNF_STRU;

typedef struct
{
    short                                 rsrp;
    short                                 rssi;
    short                                 current_tx_power_level;
    unsigned long                         total_tx_time;
    unsigned long                         total_rx_time;
    unsigned long                         last_cell_ID;
    unsigned char                         last_ECL_value;
    short                                 last_snr_value;
    unsigned long                         last_earfcn_value;
    unsigned short                        last_pci_value;
    short                                 rsrq;
    unsigned long                         current_plmn;
    unsigned short                        current_tac;
    unsigned char                         band;
    unsigned char                         operation_mode;
} ATC_NUESTATS_RSLT_RADIO_STRU;

typedef struct
{
    API_CELL_LIST_STRU                    stCellList;
} ATC_NUESTATS_RSLT_CELL_STRU;

typedef struct
{
    unsigned long                         rlc_ul_bler;
    unsigned long                         rlc_dl_bler;
    unsigned long                         mac_ul_bler;
    unsigned long                         mac_dl_bler;
    unsigned long                         total_bytes_transmit;
    unsigned long                         total_bytes_receive;
    
    unsigned long                         transport_blocks_send;
    unsigned long                         transport_blocks_receive;
    unsigned long                         transport_blocks_retrans;
    unsigned long                         total_ackOrNack_msg_receive;
} ATC_NUESTATS_RSLT_BLER_STRU;

typedef struct
{
    unsigned long                         rlc_ul;
    unsigned long                         rlc_dl;
    unsigned long                         mac_ul;
    unsigned long                         mac_dl;
} ATC_NUESTATS_RSLT_THP_STRU;

typedef struct
{
    unsigned long                         band;
} ATC_NUESTATS_RSLT_SBAND_STRU;

//+NUESTATS=<type>
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         type;
    unsigned char                         ucPadding;
    ATC_NUESTATS_RSLT_RADIO_STRU          stRadio;
    ATC_NUESTATS_RSLT_CELL_STRU           stCell;
    ATC_NUESTATS_RSLT_BLER_STRU           stBler;
    ATC_NUESTATS_RSLT_THP_STRU            stThp;
    ATC_NUESTATS_RSLT_SBAND_STRU          stBand;
} ATC_MSG_NUESTATS_CNF_STRU;

//+NEARFCN?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         search_mode;
    unsigned long                         earfcn;
    unsigned short                        pci;
} ATC_MSG_NEARFCN_R_CNF_STRU;

//+NBAND?;+NBAND=?
typedef struct
{
    unsigned short                        usEvent;
    API_SUPP_BAND_LIST_STRU               stSupportBandList;
} ATC_MSG_NBAND_R_CNF_STRU,
  ATC_MSG_NBAND_T_CNF_STRU,
  ATC_MSG_QBAND_R_CNF_STRU,
  ATC_MSG_QBAND_T_CNF_STRU;

//+NCONFIG?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         autoconnect;
    unsigned char                         combine_attach;
    unsigned char                         cell_reselection;
    unsigned char                         enable_bip;
    unsigned char                         multitone;
    unsigned char                         release_version;
    unsigned short                        barring_release_delay;
    unsigned short                        sync_time_period;
    unsigned char                         pco_ie_type;
    unsigned char                         non_ip_no_sms_enable;
    unsigned char                         t3324_t3412_ext_chg_report;
    unsigned char                         cr_0354_0338_scrambling;
    unsigned char                         cr_0859_si_avoid;
    unsigned char                         nas_sim_power_saving_enable;
    unsigned char                         rai;
    unsigned char                         nplmns_oos_ind_enable;
} ATC_MSG_NCONFIG_R_CNF_STRU;

//+QCFG?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucFunctionFlg;
} ATC_MSG_QCFG_R_CNF_STRU;

//+NCONFIG=?
typedef struct
{
    unsigned short                        usEvent;
#define D_ATC_NCONFIG_T_BIP_TYPE_SINGLE   0
#define D_ATC_NCONFIG_T_BIP_TYPE_NORMAL   1
    unsigned char                         ucBipSupportType;
    unsigned char                         enable_bip;
} ATC_MSG_NCONFIG_T_CNF_STRU;

//+NSET=<function>
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         aucInsValue[20];
    union
    {
        unsigned long                     ulValue;
        unsigned char                     aucValue[40];
    } u;
} ATC_MSG_NSET_R_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         aucInsValue[20];
    union
    {
        unsigned char                     ucValue;
        unsigned char                     aucValue[40];
    } u;
} ATC_MSG_SIMUUICC_R_CNF_STRU;

//+CGDSCONT?
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_CGDSCONT_PARA_READ_REQ_STRU   stPara;
} ATC_MSG_CGDSCONT_R_CNF_STRU;

//+CGDSCONT=?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
    unsigned char                         aucCid[D_ATC_MAX_CID];
    unsigned char                         ucP_CidNum;
    unsigned char                         aucP_Cid[D_ATC_MAX_CID];
} ATC_MSG_CGDSCONT_T_CNF_STRU;

typedef struct
{
    unsigned char                         ucAtCid;
    unsigned char                         ucPacketFilterNum;
    EPS_PF_INFO                           atPacketFilter[D_ATC_MAX_PACKET_FILTER];
} ATC_EPS_BEARER_TFT_INFO_STRU;

//+CGTFT?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
    ATC_EPS_BEARER_TFT_INFO_STRU          atBearerTft[D_MAX_CNT_CID];
} ATC_MSG_CGTFT_R_CNF_STRU;

//+CGEQOS?
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_QOS_PARA_READ_REQ_STRU        stPara;
} ATC_MSG_CGEQOS_R_CNF_STRU;

//+CGCMOD=?
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_DEFINED_CID_LIST_STRU         stPara;
} ATC_MSG_CGCMOD_T_CNF_STRU;

//+COPS?;+COPS=?
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_OPESELINFO_READ_REQ_STRU      stPara;
} ATC_MSG_COPS_R_CNF_STRU,
  ATC_MSG_COPS_T_CNF_STRU;

//+CGEREP?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCgerepMode;
} ATC_MSG_CGEREP_R_CNF_STRU;

//+CCIOTOPT?
typedef struct
{
    unsigned short                        usEvent;
    API_CCIOTOPT_CONFIG_STRU              stPara;
} ATC_MSG_CCIOTOPT_R_CNF_STRU;

//+CEDRXRDP
typedef struct
{
    unsigned short                        usEvent;
    API_NWCEDRXS_PARA_STRU                stPara;
} ATC_MSG_CEDRXRDP_CNF_STRU;

//+CGEQOSRDP[=<cid>]
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_CGEQOSRDP_PARA_READ_REQ_STRU  stPara;
} ATC_MSG_CGEQOSRDP_CNF_STRU;

//+CGEQOSRDP=?
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_DEFINED_CID_LIST_STRU         stPara;
} ATC_MSG_CGEQOSRDP_T_CNF_STRU;

//+CTZR?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCtzrReport;
} ATC_MSG_CTZR_R_CNF_STRU;

//+CGCONTRDP[=<cid>]
typedef struct
{
    unsigned short                        usEvent;
    API_CGCONTRDP_READ_DYNAMIC_PARA_STRU  stPara;
} ATC_MSG_CGCONTRDP_CNF_STRU;

//+CGCONTRDP=?
typedef struct
{
    unsigned short                        usEvent;
    API_ESM_DEFINED_CID_LIST_STRU         stPara;
} ATC_MSG_CGCONTRDP_T_CNF_STRU;

//+CPIN?
typedef struct
{
    unsigned short                        usEvent;
    unsigned short                        ucPinStatus;
} ATC_MSG_CPIN_R_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
#define D_ATC_PIN_STATUS_PIN_REQUIRED     0
#define D_ATC_PIN_STATUS_PUK_REQUIRED     1
#define D_ATC_PIN_STATUS_PUK_BLOCK        2
    unsigned char                         ucPinStatus;
} ATC_MSG_PIN_STATUS_IND_STRU;

//+CLCK=<fac>,<mode>[,<passwd>]
typedef struct
{
    unsigned short                        usEvent;
#define D_ATC_CLCK_STATUS_ACTIVE       0
#define D_ATC_CLCK_STATUS_DEACTIVE     1
    unsigned short                        ucStatus;
} ATC_MSG_CLCK_CNF_STRU;

//+NPIN=<command>,<paremeter1>
typedef struct
{
    unsigned short                        usEvent;
#define    D_ATC_ERROR_PIN_DISABLED        0
#define    D_ATC_ERROR_PIN_ENABLED         1
#define    D_ATC_ERROR_PIN_BLOCKED         2
    unsigned short                        usResult;
    unsigned char                         ucNpinFlg;
} ATC_MSG_NPIN_CNF_STRU;

typedef struct
{
#define D_ATC_CPINR_TYPE_PIN1      0
#define D_ATC_CPINR_TYPE_PUK1      1
#define D_ATC_CPINR_TYPE_PIN2      2
#define D_ATC_CPINR_TYPE_PUK2      3
    unsigned char                         ucPinType;
    unsigned char                         ucRetriesNum;
} ATC_PIN_RETRIES_RES_STRU;

//+CPINR[=<sel_code>]
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
    ATC_PIN_RETRIES_RES_STRU              aPinRetires[4];
} ATC_MSG_CPINR_CNF_STRU;

//+CEER:<err>
typedef struct
{
    unsigned short                        usEvent;
#ifndef D_FAIL_CAUSE_TYPE_NULL
#define  D_FAIL_CAUSE_TYPE_NULL      0
#define  D_FAIL_CAUSE_TYPE_EMM       1
#define  D_FAIL_CAUSE_TYPE_ESM       2
#define  D_FAIL_CAUSE_TYPE_USER_OPT  3
#endif
    unsigned char                         ucType;
    unsigned char                         ucCause;
} ATC_MSG_CEER_IND_STRU;

//+CIPCA?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         n;
    unsigned char                         ucAttachWithOutPdn;
} ATC_MSG_CIPCA_R_CNF_STRU;

typedef struct
{
    unsigned char                         ucAtCid;
    unsigned char                         ucAuthProt;
    unsigned char                         aucUserName[D_PCO_AUTH_MAX_LEN];
    unsigned char                         aucPassword[D_PCO_AUTH_MAX_LEN];
} ATC_CGAUTH_INFO_STRU;

//+CGAUTH?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
    ATC_CGAUTH_INFO_STRU                  stCgauth[D_MAX_CNT_CID];
} ATC_MSG_CGAUTH_R_CNF_STRU;

typedef struct
{
    unsigned char                         ucBand;
    unsigned char                         ucPowerClass;
} ATC_SUPPORT_BAND_INFO_STRU;

//+NPOWERCLASS?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         unNum;
    ATC_SUPPORT_BAND_INFO_STRU            stSupportBandInfo[G_LRRC_SUPPORT_BAND_MAX_NUM];
} ATC_MSG_NPOWERCLASS_R_CNF_STRU;

//+NPOWERCLASS=?
typedef struct
{
  unsigned short                        usEvent;
  unsigned char                         unNum;
  unsigned char                         aucBand[G_LRRC_SUPPORT_BAND_MAX_NUM];
} ATC_MSG_NPOWERCLASS_T_CNF_STRU;

//+NCIDSTATUS[=<cid>]
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_CID_STATUS_STRU               stCidStatus;
} ATC_MSG_NCIDSTATUS_CNF_STRU,
  ATC_MSG_NCIDSTATUS_R_CNF_STRU;

typedef struct
{
    unsigned char                 ucRegMode;
    unsigned char                 ucRegState;
    unsigned short                usTac;
    unsigned long                 ulCellId;
    unsigned long                 ulActTime;
    unsigned long                 ulTauTime;
} ST_ATC_REG_CONTEXT_STRU;

//+NGACTR?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNgactrN;
    ST_ATC_REG_CONTEXT_STRU               stRegContext;
    unsigned short                        usOperType;   /* 0:china telecom; 1:china mobile; 2:china unicom; x0FFFF:unknown */
} ATC_MSG_NGACTR_R_CNF_STRU;

typedef struct
{
    unsigned char     ucSuppBand;
    unsigned short    usOffset;
    unsigned long     ulStartFreq;
} ATC_NPOB_PRE_BAND_STRU;

typedef struct
{
    unsigned long                         aulPlmn[NVM_MAX_OPERATOR_PLMN_NUM];
    ATC_NPOB_PRE_BAND_STRU                aPreBand[NVM_MAX_PRE_BAND_NUM];
} ATC_NPOB_INFO_STRU;

//+NPOPB?
typedef struct
{
    unsigned short                        usEvent;
    ATC_NPOB_INFO_STRU                    stNpobList[NVM_MAX_OPERATOR_NUM];
} ATC_MSG_NPOPB_R_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucN;
} ATC_MSG_NIPINFO_R_CNF_STRU,
  ATC_MSG_CNEC_R_CNF_STRU;

typedef struct
{   
    unsigned char                         ucParam;
    unsigned char                         ucState;
} API_PDP_CONTEXT_DNS_SET_INFO;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNum;
#define D_ATC_NCPCDPR_R_MAX_NUM           2
    API_PDP_CONTEXT_DNS_SET_INFO          stPdpCtxDnsSet[D_ATC_NCPCDPR_R_MAX_NUM];
} ATC_MSG_NCPCDPR_R_CNF_STRU;

//+CSMS=<service>;+CSMS?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucMsgService;
} ATC_MSG_CSMS_CNF_STRU,
  ATC_MSG_CSMS_R_CNF_STRU;

//+CMGF?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucValue;
} ATC_MSG_CMGF_R_CNF_STRU;

//+CSCA?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucScaLen;
    unsigned char                         aucSca[12];
    unsigned char                         ucToSca;
} ATC_MSG_CSCA_R_CNF_STRU;

//+CMMS?
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCmmsN;
} ATC_MSG_CMMS_R_CNF_STRU;

//+CMT:"HEX",<ucPduLen>\r\n<aucData>
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucPduLen;
    unsigned char                         ucDataLen;
    unsigned char                         aucData[190];
} ATC_MSG_CMT_IND_STRU;

//+CMGC=<length><CR>PDU<ctrl-Z>;
//+CMGS=<length><CR>PDU<ctrl-Z>
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCmdType;
    unsigned char                         ucTpmr;
    unsigned char                         ucTpduLen;
    unsigned char                         aucTpdu[164];
} ATC_MSG_CMGC_CNF_STRU,
  ATC_MSG_CMGS_CNF_STRU;

//+SIMST:<ucSimStatus>
typedef struct
{
    unsigned short                        usEvent;
#define    D_ATC_SIMST_NOT_PRESENT          0
#define    D_ATC_SIMST_SUCC_INIT            1
#define    D_ATC_SIMST_SUCC_NO_INIT         2
    unsigned char                         ucSimStatus; 
    API_EPS_IMSI_READ_STRU                stImsi;
} ATC_MSG_SIMST_IND_STRU;

//+CRTDCP:<ucAtCid>,<usLen>,<pucReportData>
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNonIpDataFlg;
    unsigned char                         ucNrnpdmRepValue; 
    unsigned char                         ucCrtdcpRepValue;
    unsigned char                         ucAtCid;
    unsigned short                        usLen;         //appending data after usLen
    unsigned char                        *pucReportData;
} ATC_MSG_CRTDCP_IND_STRU;

//+CGAPNRC:<cid>,...
typedef struct
{
    unsigned short                        usEvent;
    EPS_APN_RATE_CONTRL_STRU              stEpsApnRateCtl;
} ATC_MSG_CGAPNRC_IND_STRU;

//+CGEV:...
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCgerepMode;
    unsigned char                         ucCgevEventId;
    ST_ATC_CGEV_PARAMETER                 stCgevPara;
} ATC_MSG_CGEV_IND_STRU;

//+CGREG:<stat>,...
typedef struct
{
    unsigned short                        usEvent;
    API_EPS_REG_STATUS_STRU               stPara;
} ATC_MSG_CEREG_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCsconN;
    API_SIGNAL_CONN_STATUS_STRU           stPara;
} ATC_MSG_CSCON_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    API_NWCEDRXS_PARA_STRU                stPara;
} ATC_MSG_NPTWEDRXP_IND_STRU,
  ATC_MSG_CEDRXP_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCciotoptN;
    unsigned char                         ucSupptNwOpt;
} ATC_MSG_CCIOTOPTI_IND_STRU;


typedef struct
{
    unsigned short                        usEvent;
    unsigned long                         ulRlcUl;
    unsigned long                         ulRlcDl;
    unsigned long                         ulMacUl;
    unsigned long                         ulMacDl;
} ATC_MSG_L2_THP_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned long                         ulRlcUl;
    unsigned char                         aucICCIDstring[EMM_SIM_UICC_ID_LEN*2 + 1];
} ATC_MSG_UICCID_CNF_STRU,
ATC_MSG_ZICCID_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    API_EPS_PDN_IPDNS_IND_STRU            stPara;
} ATC_MSG_XYIPDNS_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned long                         ulAddr;
    unsigned short                        usNvRamLen;
} ATC_MSG_MALLOC_ADDR_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned short                        usIpSn;
#define D_ATC_IPSN_STATUS_FAIL   0
#define D_ATC_IPSN_STATUS_SUCC   1
    unsigned char                         ucStatus;
    unsigned char                         ucFlowCtrlStatusFlg;
    unsigned char                         ucFlowCtrlStatus;
    unsigned char                         ucFlowCtrlUrcFlg;
} ATC_MSG_IPSN_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucAtCid;
    unsigned char                         ucIPv4Flg;
    unsigned char                         ucIPv6Flg;
    unsigned char                         aucIPv4Addr[4];
    unsigned char                         aucIPv6Addr[16];
} ATC_MSG_PdnIPAddr_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucNgactrN;
    unsigned char                         ucAtCid;
#define D_ATC_NGACTR_STATE_DEACT                 0
#define D_ATC_NGACTR_STATE_ACT                   1
    unsigned char                         state;
#define D_ATC_NGACTR_RST_SUCC                     0
#define D_ATC_NGACTR_RST_CID_UNDEF                1
#define D_ATC_NGACTR_RST_LOCAL_REJ                5
#define D_ATC_NGACTR_RST_APN_ERR                  6
#define D_ATC_NGACTR_RST_IPV4_ONLY                8
#define D_ATC_NGACTR_RST_IPV6_ONLY                9
#define D_ATC_NGACTR_RST_IP_ONLY                  10
#define D_ATC_NGACTR_RST_NONIP_ONLY               11
#define D_ATC_NGACTR_RST_SINGLE_IP_ONLY           12
#define D_ATC_NGACTR_RST_SERVICE_ERR              13
#define D_ATC_NGACTR_RST_CTX_MAX_NUM              14
#define D_ATC_NGACTR_RST_LAST_PDN_DIS_NOT_ALLOW   16
#define D_ATC_NGACTR_RST_UNHNOW_ERR               17
    unsigned char                         result;
} ATC_MSG_NGACTR_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
#define D_ATC_TYPE_LOCK_FREQ            0
#define D_ATC_TYPE_LOCK_CELL            1
#define D_ATC_TYPE_CELL                 2
#define D_ATC_TYPE_MIB                  3
#define D_ATC_TYPE_EARFCN               4
#define D_ATC_TYPE_REDIRECT             5
#define D_ATC_TYPE_SCAN_CAND_FREQ       6
#define D_ATC_TYPE_SCAN_STORE_FREQ      7
#define D_ATC_TYPE_SCAN_PRE_FREQ        8
#define D_ATC_TYPE_SCAN_LOCK_FREQLIST   9
#define D_ATC_TYPE_SCAN_PRE_BAND        10
#define D_ATC_TYPE_SCAN_ALL_BAND        11
    unsigned char                         ucFstSrchType;
    unsigned char                         ucCampOnType;
} ATC_MSG_CELLSRCH_TEST_STRU;

typedef struct
{
    unsigned short                        usEvent;
    API_EPS_OPELIST_SRCH_CNF_STRU         stPara;
} ATC_MSG_OPELIST_SRCH_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned short                        usCmdType;
    unsigned short                        usRspLen;
    unsigned char                         aucRsp[257];
} ATC_MSG_CSIM_CNF_STRU,
  ATC_MSG_CGLA_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucChannelId;
} ATC_MSG_CCHO_CNF_STRU,
  ATC_MSG_CCHC_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCommand;
    unsigned char                         sw1;
    unsigned char                         sw2;
    unsigned short                        usRspLen;
    unsigned char                         aucResponse[4];
} ATC_MSG_CRSM_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucCtzrReport;
    LNB_NAS_LOCAL_TIME_STRU               stPara;
} ATC_MSG_LOCALTIMEINFO_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
} ATC_MSG_SMS_PDU_IND_STRU,
  ATC_MSG_PS_AT_IDLE_IND_STRU,
  ATC_MSG_NO_CARRIER_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
#define D_MSG_PSINFO_TYPE_SIMVCC          1
#define D_MSG_PSINFO_TYPE_NVWRITE         2
#define D_MSG_PSINFO_TYPE_SOFT_RESET      3
    unsigned char                         ucType;
    unsigned char                         ucValue;
} ATC_MSG_PSINFO_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucAtCid;
    unsigned char                         ucSequence;
    unsigned char                         ucStatus;   /* 0-err, 1-succ */
} ATC_MSG_CSODCPR_IND_STRU,
  ATC_MSG_NSNPDR_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucAtCid;
    unsigned char                         ucResult;
    unsigned char                         ucPdpType;
    unsigned char                         ucIPv4Flg;
    unsigned char                         ucIPv6Flg;
    unsigned char                         aucIPv4Addr[4];
    unsigned char                         aucIPv6Addr[16];
#define D_ATC_NIPINFO_CAUSE_IPV4_ONLY            1
#define D_ATC_NIPINFO_CAUSE_IPV6_ONLY            2
#define D_ATC_NIPINFO_CAUSE_SINGLE_ADDR_ONLY     3
#define D_ATC_NIPINFO_CAUSE_IPV6_RA_TIME_OUT     4
#define D_ATC_NIPINFO_CAUSE_UNKNOWN              5
    unsigned char                         ucFailCause;
} ATC_MSG_NIPINFO_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
#define D_ATC_CNEC_TYPE_EMM               0
#define D_ATC_CNEC_TYPE_ESM               1
    unsigned char                         ucType;
    unsigned char                         ucErrCode;
    unsigned char                         ucAtCidFlg;
    unsigned char                         ucAtCid;
} ATC_MSG_CNEC_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucPsmEnable;
#define D_ATC_MNBIOTEVENT_ENTER_PSM       0
#define D_ATC_MNBIOTEVENT_EXIT_PSM        1
    unsigned char                         ucPsmState;
} ATC_MSG_MNBIOTEVENT_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucMbioteventValue;
} ATC_MSG_MNBIOTEVENT_R_CNF_STRU;

typedef struct
{
    #define  Data_Sequence_MAX_NUM  254
    unsigned char       ucSequenceNum;
    unsigned char       Sequence[Data_Sequence_MAX_NUM];
}API_DATA_LIST_STRU;

typedef struct
{
    unsigned short                        usEvent;
    API_DATA_LIST_STRU                    stSequence;
} ATC_MSG_NQPODCP_CNF_STRU,
  ATC_MSG_NQPNPD_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucReporting;
} ATC_MSG_NRNPDM_R_CNF_STRU;

//+CEID
typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucLen;
#define D_ATC_SIM_EID_MAX_LEN   16    
    unsigned char                         aucEid[D_ATC_SIM_EID_MAX_LEN];
} ATC_MSG_CEID_CNF_STRU;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucIpv6AddressFormat;
    unsigned char                 ucIpv6SubnetNotation;
    unsigned char                 ucIpv6LeadingZeros;
    unsigned char                 ucIpv6CompressZeros;
} ATC_MSG_CGPIAF_R_CNF_STRU;

typedef struct {
    unsigned short                usEvent;
    unsigned char                 ucThresold;
} ATC_MSG_CUPREFERTH_R_CNF_STRU;

typedef struct {
    unsigned short                usEvent;
#define D_ATC_NPLMNS_NO_ACTIVE         0
#define D_ATC_NPLMNS_PLMN_SEARCHING    1
#define D_ATC_NPLMNS_PLMN_SELECTED     2
#define D_ATC_NPLMNS_OOS               3
    unsigned char                 ucState;
    unsigned long                 ulOosTimerLeftLen;
} ATC_MSG_NPLMNS_R_CNF_STRU,
  ATC_MSG_NPLMNS_OOS_IND_STRU;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucEarLockFlg;
    unsigned char                 ucPciLockFlg;
    unsigned short                usLockPci;
    unsigned long                 ulLockEar;

    unsigned long                 aulEarfcnList[D_ATC_EARFCN_MAX_NUM];
    unsigned char                 ucEarfcnNum;
} ATC_MSG_NLOCKF_R_CNF_STRU;

typedef struct {
    unsigned short                        usEvent;
    short                                 rsrp;
    short                                 rssi;
    short                                 current_tx_power_level;
    unsigned long                         last_cell_ID;
    unsigned char                         last_earfcn_offset;
    unsigned char                         last_ECL_value;
    short                                 last_snr_value;
    unsigned long                         last_earfcn_value;
    unsigned short                        last_pci_value;
    short                                 rsrq;
    unsigned short                        current_tac;
    unsigned char                         band;
} ATC_MSG_ZCELLINFO_CNF_STRU;

typedef struct 
{
    unsigned short              usEvent;
    unsigned char               ucPdpType;
    unsigned char               ucApnLen;
    unsigned char               ucAuthProt;
    unsigned char               ucUsernameLen;
    unsigned char               ucPasswordLen;
    unsigned char               aucApnValue[D_APN_LEN];

    unsigned char               aucUserName[D_PCO_AUTH_MAX_LEN];
    unsigned char               aucPassword[D_PCO_AUTH_MAX_LEN];
} ATC_MSG_QCGDEFCONT_R_CNF_STRU;

typedef struct 
{
#define D_EMM_STATE_NULL                0
#define D_EMM_STATE_DEREG               1
#define D_EMM_STATE_REG_INIT            2
#define D_EMM_STATE_REG                 3
#define D_EMM_STATE_DEREG_INIT          4
#define D_EMM_STATE_TAU_INIT            5
#define D_EMM_STATE_SR_INIT             6
#define D_EMM_STATE_UNKNOWN             7
    unsigned char                       ucEmmState;

#define D_EMM_MODE_IDLE                 0
#define D_EMM_MODE_PSM                  1
#define D_EMM_MODE_CONN                 2
#define D_EMM_MODE_UNKNOWN              3
    unsigned char                       ucEmmMode;

#define D_PLMN_STATE_NO_PLMN            0
#define D_PLMN_STATE_SERCHING           1
#define D_PLMN_STATE_SELECTED           2
#define D_PLMN_STATE_UNKNOWN            3
    unsigned char                       ucPlmnState;

#define D_PLMN_TYPE_HPLMN               0
#define D_PLMN_TYPE_EHPLMN              1
#define D_PLMN_TYPE_VPLMN               2
#define D_PLMN_TYPE_UPLMN               3
#define D_PLMN_TYPE_OPLMN               4
#define D_PLMN_TYPE_UNKNOWN             5
    unsigned char                       ucPlmnType;
    unsigned long                       ulSelectPlmn;
} ATC_QENG_PLMN_STATUS_STRU;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucQengValue;
    ATC_NUESTATS_RSLT_RADIO_STRU  stRadio;
    ATC_NUESTATS_RSLT_CELL_STRU   stCell;
    ATC_NUESTATS_RSLT_BLER_STRU   stBler;
    ATC_NUESTATS_RSLT_THP_STRU    stThp;
    ATC_QENG_PLMN_STATUS_STRU     stPlmnStatus;
} ATC_MSG_QENG_CNF_STRU;

typedef struct 
{
    unsigned short                usSimFileName;
    unsigned long                 ulSimFileWriteCount;
} LNB_SIMWC_Info_STRU;

typedef struct 
{
    unsigned short                usEvent;
#define D_ATC_SIMWC_MAX_NUM   5
    LNB_SIMWC_Info_STRU           astSimwcInfo[D_ATC_SIMWC_MAX_NUM];
} ATC_MSG_NSIMWC_CNF_STRU;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucOption;
    unsigned char                 ucValue;
} ATC_MSG_QNIDD_CNF_STRU;

typedef struct 
{
    unsigned short                usEvent;
    unsigned char                 ucNIDD_ID;
    unsigned short                usDataLen;
    unsigned char                *pucData;
} ATC_MSG_QNIDD_IND_STRU;

typedef struct 
{
   unsigned long                  ulPlmn;
   unsigned char                  ucCnt;
   unsigned long                  aulEarfcn[16];
   unsigned short                 ausPhyCellId[16];
   short                          asRsrp[16];
   short                          asSinr[16];
} PLMN_LIST_INFO_STRU;

typedef struct 
{
   unsigned char                  ucCnt;
   PLMN_LIST_INFO_STRU            PlmnList[10];
} API_CELLLIST_INFO_STRU;

typedef struct 
{
    unsigned short                usEvent;
    API_CELLLIST_INFO_STRU        CellList;
} ATC_MSG_XYCELLS_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucIndex;
    unsigned char                         ucNum;
    unsigned long                         aulfreq[D_ATC_EARFCN_MAX_NUM];
} ATC_MSG_PRESETFREQ_R_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucN;
#define D_PREBNAD_PLMN_SEARCH_NOT_START         0
#define D_PREBNAD_PLMN_SEARCH_RUNNING           1
#define D_PREBNAD_PLMN_SEARCH_FAIL              2
#define D_PREBNAD_PLMN_SEARCH_SUCC              3
    unsigned char                         ucStatus;
} ATC_MSG_NPBPLMNS_R_CNF_STRU,
  ATC_MSG_NPBPLMNS_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucN;
    unsigned long                         ulBackoffLen;
#define D_BACKOFF_NO_TIMER                0
#define D_BACKOFF_T3402                   1
#define D_BACKOFF_T3346                   2
#define D_BACKOFF_T3245                   3
#define D_BACKOFF_T3247                   4
#define D_BACKOFF_T3448                   5
#define D_BACKOFF_T3396                   6
        unsigned char                     ucTimerId;
} ATC_MSG_NBACKOFF_R_CNF_STRU,
  ATC_MSG_NBACKOFF_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucMode;
    unsigned char                         ucEarfcnFlg;
    unsigned long                         ulEarfcn;
    unsigned short                        usPci;
    unsigned char                         ucPciFlg;
    unsigned char                         ucEarfcnOffset;
} ATC_MSG_QLOCKF_R_CNF_STRU;

typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucATCid;
    unsigned char               ucPdpType;
    unsigned char               ucApnLen;
    unsigned char               ucAuthProt;
    unsigned char               ucUsernameLen;
    unsigned char               ucPasswordLen;
    unsigned char               aucApnValue[FACTORY_USER_SET_APN_LEN];

    unsigned char               aucUserName[D_PCO_AUTH_MAX_LEN];
    unsigned char               aucPassword[D_PCO_AUTH_MAX_LEN];

} ATC_MSG_QICSGP_R_CNF_STRU;

typedef struct
{
    unsigned short              usEvent;
    unsigned char               ucMode;
} ATC_MSG_QSPCHSC_R_CNF_STRU;

typedef struct 
{
    unsigned long                         ulEarfcn;
    unsigned short                        usPhyCellId;
    unsigned char                         ucMode;/*0 mib, 1 Sib1, 2 SI*/
} API_SYSINO_FAIL_INFO_STRU;

typedef struct 
{
#define	D_ATC_SCAN_MAX_FREQS        16
    unsigned long                         aulEarfcn[D_ATC_SCAN_MAX_FREQS];
    short                        asRssi[D_ATC_SCAN_MAX_FREQS];  //range in -1330~-250
    unsigned char                         ucFreqNum;
} API_FREQRSSI_INFO_STRU;

typedef struct 
{
    unsigned short                        usEvent;
    API_SYSINO_FAIL_INFO_STRU             tSysInfo;
} ATC_MSG_SYSINO_FAIL_INFO_STRU;

typedef struct 
{
    unsigned short                        usEvent;
    API_FREQRSSI_INFO_STRU                tFreqRssi;
} ATC_MSG_FREQRSSI_INFO_STRU;


#ifdef LCS_MOLR_ENABLE
typedef struct
{
    unsigned short                        usEvent;

    unsigned char                         method_flag:1;
    unsigned char                         hor_acc_flag:1;
    unsigned char                         ver_acc_flag:1;
    unsigned char                         rep_mode:1;
    unsigned char                         vel_req:3;
    unsigned char                         ucPadding:1;

    unsigned char                         enable:2;
    unsigned char                         method:3;
    unsigned char                         hor_acc_set:1;
    unsigned char                         ver_req:1;
    unsigned char                         ver_acc_set:1;

    unsigned char                         shape_rep:7;
    unsigned char                         plane:1;

    unsigned char                         hor_acc;
    unsigned char                         ver_acc;

    unsigned short                        timeout;
    unsigned short                        interval;
} ATC_MSG_CMOLR_R_CNF_STRU;

typedef struct
{
    unsigned short                        usEvent;
    unsigned char                         ucErrCode;
} ATC_MSG_CMOLRE_IND_STRU;

typedef struct
{
    unsigned short                        usEvent;
    LCS_SHAPE_DATA_STRU                   stShapeData;
    LCS_VELOCITY_DATA_STRU                stVelData;
} ATC_MSG_CMOLRG_IND_STRU;
#endif

#define D_DATA_REQ_TYPE_AT_CMD              1
#define D_DATA_REQ_TYPE_USER_INTERFACE_REQ  2
typedef struct
{
    unsigned char        ucAplNum;
    unsigned char        ucReqType;
    unsigned short       usLen;
    unsigned char        *pucData;
} T_ATC_DATA_REQ;

extern void AtcAp_TaskEntry(void* pArgs);
extern void atc_ap_task_init();

#endif



