#ifndef _ATC_AP_DEFINE_H_
#define _ATC_AP_DEFINE_H_

#include "xy_atc_interface.h"
#ifndef SINGLE_CORE
#include "xy_utils.h"
#endif

#ifndef NULL
#define NULL                         ((void*)0)
#endif

#ifdef SINGLE_CORE
#define D_THEARD_ATC_AP_PRIO         9
#define D_THEARD_ATC_AP_STACK        osStackShared
#define D_THEARD_ATC_AP_CB_STACK     osStackShared
#else //M3
#define D_THEARD_ATC_AP_PRIO         osPriorityNormal2
#define D_THEARD_ATC_AP_STACK        2048
#define D_THEARD_ATC_AP_CB_STACK     1536
#endif

#define ATC_AP_TRUE                  1
#define ATC_AP_FALSE                 0

#ifndef ATC_OK
#define ATC_OK                          0
#define ATC_NG                          1
#endif

#ifndef ATC_FALSE
#define ATC_FALSE                       0
#define ATC_TRUE                        1
#endif

#define D_ATC_AP_CME_UNKNOWN                    100
#define D_ATC_AP_CME_INCORRECT_PARAMETERS       50
#define D_ATC_AP_CME_OPER_NOT_SUPPORED          4

#ifndef D_ATC_DEFAULT_CR
/* &F and init default value */
#define D_ATC_DEFAULT_CR                13
#define D_ATC_DEFAULT_LF                10
#define D_ATC_DEFAULT_BS                8
#define D_ATC_DEFAULT_ECHO              0
#define D_ATC_DEFAULT_RSP_FMT           1
#define D_ATC_DEFAULT_ERR_MOD           1
#endif

#define D_ATC_EMM_CAUSE_TBL_SIZE             28
#define D_ATC_ESM_CAUSE_TBL_SIZE             40

#define D_ATC_CMD_PROC_TBL_SIZE              82//75->77
#define D_ATC_DATAIND_PROC_TBL_SIZE          137//135->137, D_ATC_AP_SYSINFO_FAIL_IND
#define D_ATC_USER_REG_EVENT_TBL_SIZE        27//26->27 mnbiotevent

#define D_ATC_FAC_NUM                        1//22
#define D_ATC_TFT_TEST_MSG_MAX_LENGTH        1563
#define D_ATC_CGCONRRDP_MSG_MAX_LENGTH       1278
#define D_ATC_READ_TFT_MAX_VALUE             339                  
#define D_ATC_NQPODCP_IND_MSG_MAX_LENGTH     930

/* Command kind */
#define D_ATC_CMD_FUNC_NOEQUAL_SET      0                                               /* No equal set commnad                 */
#define D_ATC_CMD_FUNC_SET              1                                               /* Set commnad                          */
#define D_ATC_CMD_FUNC_READ             2                                               /* Read commnad                         */
#define D_ATC_CMD_FUNC_TEST             3                                               /* Test commnad                         */

/* PARAMETER ANALYSIS RESULT */
#ifndef D_ATC_PARAM_OK
#define D_ATC_PARAM_OK                  0
#define D_ATC_PARAM_EMPTY               1
#define D_ATC_PARAM_ERROR               2
#define D_ATC_PARAM_SYNTAX_ERROR        3
#endif

#ifndef D_ATC_STOP_CHAR_CR
#define D_ATC_STOP_CHAR_CR                            0
#define D_ATC_STOP_CHAR_KANMA                         1
#define D_ATC_STOP_CHAR_SEMICOLON                     2
#define D_ATC_STOP_CHAR_STR                           3
#endif

/* ERR MODE */
#define D_ATC_ERRMOD_NOT_CMEE           0                                               /*lint !e755*/
#define D_ATC_ERRMOD_DIG_CMEE           1
#define D_ATC_ERRMOD_STR_CMEE           2

#ifndef D_ATC_COMMAND_OK
/* Command analysis result */
#define D_ATC_COMMAND_OK                0                                               /* Command normal                       */
#define D_ATC_COMMAND_ERROR             1                                               /* Not define command                   */
#define D_ATC_COMMAND_PARAMETER_ERROR   2                                               /* Parameter error                      */
#define D_ATC_COMMAND_END               3                                               /* Command end                          */
#define D_ATC_COMMAND_SYNTAX_ERROR      4                                               /* Command error                        */
#define D_ATC_COMMAND_DELIMITOR         5
#define D_ATC_COMMAND_SINGLE_OK         6
#define D_ATC_COMMAND_PDU_CANCEL        7
#define D_ATC_COMMAND_TOO_MANY_PARAMETERS     8                                         /* Parameter too many error             */
#define D_ATC_COMMAND_MODE_ERROR        9                                               /* Workmode error */
#endif

#define D_ATC_P_COPS_OPER_SIZE_MAX              6                                       /* +COPS:<per>                          */

#ifndef D_ATC_N_CR
#define D_ATC_N_CR                                    D_ATC_DEFAULT_CR                  /* <cr> define                          */
#define D_ATC_N_LF                                    D_ATC_DEFAULT_LF                  /* <lf> define                          */
#define D_ATC_N_COMMA                                 44                                /* ','                                  */
#define D_ATC_N_SEMICOLON                             59                                /* ';'                                  */
#define D_ATC_N_QUOTATION                             34                                /* '"'                                  */
#define D_ATC_N_CTRL_Z                                26                                /*  ctrl-Z                              */
#define D_ATC_N_ESC                                   27                                /*  ESC                                 */
#define D_ATC_N_SUB                                   45                                /* '-'                                  */
#define D_ATC_N_DOT                                   46                                /* '.'                                  */
#endif

/* LNBATC */
#define D_ATC_PARAM_MAX_P_CGDCONT               16                                      /* +CGDCONT PARAMETER NUMBER            */
#define D_ATC_PARAM_MAX_P_CGDSCONT              4                                       /* +CGDCONT PARAMETER NUMBER            */
#define D_ATC_PARAM_MAX_P_CGEQOS                6                                       /* +CGEQOS  PARAMETER NUMBER            */
#define D_ATC_PARAM_MAX_P_CGTFT                 12                                      /* +CGTFT  PARAMETER NUMBER             */
#define D_ATC_CGTFT_PARAM_SPI_LEN               8                                       /* +CGTFT  ipsec security parameter index */
#define D_ATC_CGTFT_PARAM_FLOW_LABEL_LEN        5                                       /* +CGTFT  flow label (ipv6)            */
#define D_ATC_PARAM_MAX_P_CFUN                  2                                       /* +CFUN PARAMETER NUMBER               */
#define D_ATC_PARAM_MAX_P_CSODCP                6                                       /* +CSODCP PARAMETER NUMBER             */
#define D_ATC_P_CSODCP_CPDATA_SIZE_MAX          65535                                   /* +CSODCP:<cpdata>                     */
#define D_ATC_PARAM_MAX_P_CEDRXS                4                                       /* +CEDRXS PARAMETER NUMBER             */
#define D_ATC_P_CEDRXS_EDRX_VALUE_MAX           4                                       /* +CEDRXS:<eDRX value>                 */
#define D_ATC_P_CPSMS_ReqPeriTAU_VALUE_MAX      8                                       /* +CPSMS:<Requested_Periodic-TAU value>*/
#define D_ATC_P_CPSMS_ReqActTime_VALUE_MAX      8                                       /* +CPSMS:<Requested_Active-Time value> */
#define D_ATC_PARAM_MAX_P_CPSMS                 5//3                                       /* +CPSMS PARAMETER NUMBER              */
#define D_ATC_PARAM_MAX_P_CNMI                  5                                       /* +CNMI PARAMETER NUMBER               */
#define D_ATC_PARAM_MAX_P_CMUX                  9
#define D_ATC_PARAM_MAX_P_CNMA                  3                                       /* +CNMA PARAMETER NUMBER               */
#define D_ATC_PARAM_MAX_P_COPS                  4                                       /* +COPS PARAMETER NUMBER               */
#define D_ATC_PARAM_MAX_P_NEARFCN               3                                       /* +NEARFCN PARAMETER NUMBER            */
#define D_ATC_PARAM_MAX_P_NBAND                 8
#define D_ATC_PARAM_MAX_P_QLWULDATAEX           3
#define D_ATC_PARAM_MAX_P_NATSPEED              5
#define D_ATC_PARAM_MAX_P_NSET                  3                                       /* +NSET PARAMETER NUMBER               */
#define D_ATC_PARAM_MAX_P_SIMUUICC              4  
#define D_ATC_PARAM_MAX_P_CCIOTOPT              3
//shao add for USAT
#define D_ATC_PARAM_MAX_P_CSIM                  2
#define D_ATC_PARAM_MAX_P_CCHC                  2
#define D_ATC_PARAM_MAX_P_CCHO                  1
#define D_ATC_PARAM_MAX_P_CGLA                  3
#define D_ATC_PARAM_MAX_P_CRSM                  7
#define D_ATC_PARAM_MAX_P_CPWD                  3                                       /* +CPWD PARAMETER NUMBER                   */
#define D_ATC_PARAM_MAX_P_CLCK                  3//4                                    /* +CLCK PARAMETER NUMBER                   */
#define D_ATC_PARAM_MAX_P_CPIN                  2                                       /* +CPIN PARAMETER NUMBER                   */

#define D_ATC_PARAM_MAX_P_CMOLR                 15

/* Parameter size */
#define D_ATC_P_CGDCONT_APN_SIZE_MAX                           100                      /* <APN> MAX SIZE                       */
#define D_ATC_P_CGTFT_REMOTEADDRESSANDSUBNETMASK_SIZE_MAX      130                      /* <remote address and subnet mask> MAX
                                                                                            SIZE                                */ 
#define D_ATC_P_CGTFT_LOCAL_PORT_RANG_SIZE_MAX                 15                       /* <local port range> MAX SIZE          */
#define D_ATC_P_CGTFT_REMOTE_PORT_RANG_SIZE_MAX                15                       /* <remote port range> MAX SIZE         */
#define D_ATC_P_CGTFT_TOSANDMASK_TRACFFICCLASSANDMAK_SIZE_MAX  10                       /* <type of service (tos) (ipv4) and mask/
                                                                                        traffic class (ipv6) and mask> MAX SIZE */
#define D_ATC_P_CGTFT_LOCALADDRESSANDSUBNETMASK_SIZE_MAX       130                     /* <remote address and subnet mask> MAX 
                                                                                                                           SIZE */

//for CRSM 
#define D_ATC_CRSM_COMMAND_READ_BINARY               176
#define D_ATC_CRSM_COMMAND_READ_RECORD               178
#define D_ATC_CRSM_COMMAND_GET_RESPONSE              192
#define D_ATC_CRSM_COMMAND_UPDATE_BINARY             214
#define D_ATC_CRSM_COMMAND_UPDATE_RECORD             220
#define D_ATC_CRSM_COMMAND_STATUS                    242
#define D_ATC_CRSM_COMMAND_RETRIEVE_DATA             203
#define D_ATC_CRSM_COMMAND_SET_DATA                  219

#ifdef LCS_MOLR_ENABLE
#define D_ATC_CMOLRG_MAX_SIZE                4000

//+CMOLG XML element
#define   LCS_XML_ELEMENT_HEAD                   "<?xml version=\"1.0\" ?>"
#define   LCS_XML_ELEMEN_LCATION_PARAM           "location_parameters"
#define   LCS_XML_ELEMEN_TIME                    "time"
#define   LCS_XML_ELEMEN_DIRECTION               "direction"
#define   LCS_XML_ELEMEN_SHAPE_DATA              "shape_data"
#define   LCS_XML_ELEMEN_ELLIP_POINT             "ellipsoid_point"
#define   LCS_XML_ELEMEN_ELLIP_UNCERT_CIRCLE     "ellipsoid_point_uncert_circle"
#define   LCS_XML_ELEMEN_ELLIP_UNCERT_ELLIP      "ellipsoid_point_uncert_ellipse"
#define   LCS_XML_ELEMEN_POLYGON                 "polygon"
#define   LCS_XML_ELEMEN_ELLIP_POINT_ALT         "ellipsoid_point_alt"
#define   LCS_XML_ELEMEN_ELLIP_POINT_ALT_UNCET   "ellipsoid_point_alt_uncertellipse"
#define   LCS_XML_ELEMEN_ELLIP_POINT_ARC         "ellips_arc"
#define   LCS_XML_ELEMEN_COORDINATE              "coordinate"
#define   LCS_XML_ELEMEN_LATITUDE                "latitude"
#define   LCS_XML_ELEMEN_NORTH                   "north"
#define   LCS_XML_ELEMEN_DEGRESS                 "degrees"
#define   LCS_XML_ELEMEN_LONGITUDE               "longitude"
#define   LCS_XML_ELEMEN_UNCERT_CIRCLE           "uncert_circle"
#define   LCS_XML_ELEMEN_UNCERT_ELLIP            "uncert_ellipse"
#define   LCS_XML_ELEMEN_UNCERT_SEMI_MAJOR       "uncert_semi_major"
#define   LCS_XML_ELEMEN_UNCERT_SEMI_MIJOR       "uncert_semi_minor"
#define   LCS_XML_ELEMEN_ORIENT_MIJOR            "orient_major"
#define   LCS_XML_ELEMEN_CONFIDENCE              "confidence"
#define   LCS_XML_ELEMEN_ALT                     "altitude"
#define   LCS_XML_ELEMEN_HEIGHT_ABOVE_SURFACE    "height_above_surface"
#define   LCS_XML_ELEMEN_HEIGHT                  "height"
#define   LCS_XML_ELEMEN_UNCERT_ALT              "uncert_alt"
#define   LCS_XML_ELEMEN_INNER_RAD               "inner_rad"
#define   LCS_XML_ELEMEN_UNCERT_RAD              "uncert_rad"
#define   LCS_XML_ELEMEN_OFFSET_ANGLE            "offset_angle"
#define   LCS_XML_ELEMEN_INC_ANGLE               "included_angle"

//velocity_data
#define   LCS_XML_ELEMEN_VEL_DATA                "velocity_data"
#define   LCS_XML_ELEMEN_HOR_VEL                 "hor_velocity"
#define   LCS_XML_ELEMEN_VERT_VEL                "vert_velocity"
#define   LCS_XML_ELEMEN_VERT_VEL_DIRECT         "vert_velocity_direction"
#define   LCS_XML_ELEMEN_HOR_UNCERT              "hor_uncert"
#define   LCS_XML_ELEMEN_VERT_UNCERT             "vert_uncert"
#endif

enum {
    EVENT_CGSN = 0, 
    EVENT_CEREG,    /* 1 */
    EVENT_CGATT,    /* 2 */
    EVENT_CIMI,     /* 3 */
    EVENT_CGDCONT,  /* 4 */
    EVENT_CFUN,     /* 5 */
    EVENT_CMEE,     /* 6 */
    EVENT_CLAC,     /* 7 */
    EVENT_CESQ,     /* 8 */
    EVENT_CGPADDR,  /* 9 */
    EVENT_CGACT,    /* 10 */
    EVENT_CSODCP,   /* 11 */
    EVENT_CRTDCP,   /* 12 */
    //EVENT_E,        /* 13 */
    EVENT_SIMST,    /* 14 */
    EVENT_CEDRXS,   /* 15 */
    EVENT_CPSMS,    /* 16 */
    EVENT_CGAPNRC,  /* 17 */
#ifdef ESM_DEDICATED_EPS_BEARER
    EVENT_CGDSCONT, /* 18 */
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
    EVENT_CGTFT,    /* 19 */
#endif
    EVENT_CGEQOS,   /* 20 */
#ifdef ESM_EPS_BEARER_MODIFY
    EVENT_CGCMOD,   /* 21 */
#endif
    EVENT_CSMS,     /* 22 */
    EVENT_CMGF,     /* 23 */
    EVENT_CSCA,     /* 24 */
    //EVENT_CNMI,     /* 25 */
    EVENT_CMGS,     /* 26 */
    EVENT_CNMA,     /* 27 */
    EVENT_COPS,     /* 28 */
    EVENT_CSCON,    /* 29 */    
    EVENT_CGEREP,   /* 30 */ 
    EVENT_CCIOTOPT, /* 31 */ 
    EVENT_CEDRXRDP, /* 32 */ 
    EVENT_CGEQOSRDP,/* 33 */ 
    EVENT_CTZR,     /* 34 */ 
    EVENT_CGCONTRDP,/* 35 */     
    EVENT_CPIN,     /* 36 */ 
    EVENT_CLCK,     /* 37 */ 
    EVENT_CPWD,     /* 38 */ 
    EVENT_NUESTATS, /* 39 */ 
    EVENT_NEARFCN,  /* 40 */
    EVENT_NBAND,    /* 41 */
    EVENT_NCONFIG,  /* 42 */
    EVENT_NCCID,    /* 43 */
    EVENT_NCSEARFCN,/* 44 */
    EVENT_RAI,      /* 45 */
    EVENT_NFPLMN,   /* 46 */
    EVENT_NL2THP,   /* 47 */
    EVENT_CSQ,      /* 48 */
    EVENT_NSET,     /* 49 */
#ifdef LCS_MOLR_ENABLE
    EVENT_CMOLR,    /* 50 */
#endif
    EVENT_CCHC,
    EVENT_CCHO,
    EVENT_CGLR,
    
    EVENT_CEER,
    EVENT_CIPCA,
    EVENT_CGAUTH,
    EVENT_CNMPSD,
    EVENT_CPINR,
    EVENT_CRSM,
    EVENT_CSIM,
    EVENT_CMGC,
    EVNET_CMMS,
    EVNET_NPOWERCLASS,
    EVNET_NPTWEDRXS,
    EVNET_NPIN,
    EVNET_NTSETID,
    EVNET_NCIDSTATUS,
    EVNET_NGACTR,
    EVNET_NPOPB,
    EVNET_NIPINFO,
    EVNET_NQPODCP,
    EVNET_NSNPD,
    EVNET_NQPNPD,
    EVNET_CNEC,
    EVENT_NRNPDM,
    EVENT_NCPCDPR,
    EVENT_CEID,
    EVENT_MNBIOTEVENT,
    EVENT_CGPIAF,
#if SIMMAX_SUPPORT
    EVENT_CUPREFER,
    EVENT_CUPREFERTH,
#endif
    EVENT_NPLMNS,
    EVENT_NLOCKF,
    EVENT_ZICCID,
    EVENT_ZCELLINFO,
    EVENT_QCGDEFCONT,
    EVENT_QBAND,
    EVENT_QCCID,
    EVENT_QENG,
    EVENT_QCFG,
    EVENT_NSIMWC,
    EVENT_NUICC,
    EVENT_PSTEST,
    EVENT_QNIDD,
    EVENT_XYCELLS,
    EVENT_PRESETFREQ,
    EVENT_NPBPLMNS,
    EVENT_NBACKOFF,
    EVENT_SIMUUICC,
    EVENT_QLOCKF,
    EVENT_QCSEARFCN,
    EVENT_W,
    EVENT_QICSGP,
    EVENT_QSPCHSC,
};

#endif
