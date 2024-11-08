#include "atc_ps.h"

ST_ATC_AP_INFO                g_AtcApInfo;


const ST_ATC_AP_CMD_PROC_TABLE AtcAp_CmdProcTable[D_ATC_CMD_PROC_TBL_SIZE] = 
{
    { D_ATC_EVENT_CGSN_T,                       AtcAp_CGSN_T_LNB_Process        },
    { D_ATC_EVENT_CEREG_T,                      AtcAp_CEREG_T_LNB_Process       },
    { D_ATC_EVENT_CGATT_T,                      AtcAp_CGATT_T_LNB_Process       },
    { D_ATC_EVENT_CIMI_T,                       AtcAp_CIMI_T_LNB_Process        },
    { D_ATC_EVENT_CGDCONT_T,                    AtcAp_CGDCONT_T_LNB_Process     },
    { D_ATC_EVENT_CFUN_T,                       AtcAp_CFUN_T_LNB_Process        },
    { D_ATC_EVENT_CMEE,                         AtcAp_CMEE_LNB_Process          },
    { D_ATC_EVENT_CMEE_R,                       AtcAp_CMEE_R_LNB_Process        },
    { D_ATC_EVENT_CMEE_T,                       AtcAp_CMEE_T_LNB_Process        },
    { D_ATC_EVENT_CLAC  ,                       AtcAp_CLAC_LNB_Process          },
    { D_ATC_EVENT_CLAC_T,                       AtcAp_CLAC_T_LNB_Process        },
    { D_ATC_EVENT_CESQ_T,                       AtcAp_CESQ_T_LNB_Process        },
    { D_ATC_EVENT_CGACT_T,                      AtcAp_CGACT_T_LNB_Process       },
    { D_ATC_EVENT_CSODCP_T,                     AtcAp_CSODCP_T_LNB_Process      },
    { D_ATC_EVENT_CRTDCP_T,                     AtcAp_CRTDCP_T_LNB_Process      },
    { D_ATC_EVENT_CEDRXS_T,                     AtcAp_CEDRXS_T_LNB_Process      },
    { D_ATC_EVENT_CPSMS_T,                      AtcAp_CPSMS_T_LNB_Process       },
#ifdef EPS_BEARER_TFT_SUPPORT
    { D_ATC_EVENT_CGTFT_T,                      AtcAp_CGTFT_T_LNB_Process       },
#endif
    { D_ATC_EVENT_CGEQOS_T,                     AtcAp_CGEQOS_T_LNB_Process      },
#ifdef NBIOT_SMS_FEATURE
    { D_ATC_EVENT_CSMS_T,                       AtcAp_CSMS_T_Process            },
    { D_ATC_EVENT_CMGF_T,                       AtcAp_CMGF_T_Process            },
    { D_ATC_EVENT_CSCA_T,                       AtcAp_CSCA_T_Process            },
    { D_ATC_EVENT_CMGS_T,                       AtcAp_CMGS_T_Process            },
    { D_ATC_EVENT_CNMA_T,                       AtcAp_CNMA_T_Process            },
    { D_ATC_EVENT_CMGC_T,                       AtcAp_CMGC_T_Process            },
    { D_ATC_EVENT_CMMS_T,                       AtcAp_CMMS_T_Process            },
#endif
    { D_ATC_EVENT_CSIM_T,                       AtcAp_CSIM_T_LNB_Process        },
    { D_ATC_EVENT_CCHC_T,                       AtcAp_CCHC_T_LNB_Process        },
    { D_ATC_EVENT_CCHO_T,                       AtcAp_CCHO_T_LNB_Process        },
    { D_ATC_EVENT_CGLA_T,                       AtcAp_CGLA_T_LNB_Process        },
    { D_ATC_EVENT_CRSM_T,                       AtcAp_CRSM_T_LNB_Process        },
    { D_ATC_EVENT_CSCON_T,                      AtcAp_CSCON_T_LNB_Process       },
    { D_ATC_EVENT_CGEREP_T,                     AtcAp_CGEREP_T_Process          },
    { D_ATC_EVENT_CCIOTOPT_T,                   AtcAp_CCIOTOPT_T_Process        },
    { D_ATC_EVENT_CEDRXRDP_T,                   AtcAp_CEDRXRDP_T_LNB_Process    },
    { D_ATC_EVENT_CTZR_T,                       AtcAp_CTZR_T_LNB_Process        },
    { D_ATC_EVENT_CPIN_T,                       AtcAp_CPIN_T_Process            },
    { D_ATC_EVENT_CLCK_T,                       AtcAp_CLCK_T_Process            },
    { D_ATC_EVENT_CPWD_T,                       AtcAp_CPWD_T_Process            },
    { D_ATC_EVENT_NUESTATS_T,                   AtcAp_NUESTATS_T_LNB_Process    },
    { D_ATC_EVENT_NEARFCN_T,                    AtcAp_NEARFCN_T_LNB_Process     },
    { D_ATC_EVENT_NCCID_T,                      AtcAp_NCCID_T_LNB_Process       },
    { D_ATC_EVENT_NL2THP_T,                     AtcAp_NL2THP_T_LNB_Process      },
    { D_ATC_EVENT_CSQ_T,                        AtcAp_CSQ_T_LNB_Process         },
#ifdef LCS_MOLR_ENABLE
    { D_ATC_EVENT_CMOLR_T,                      AtcAp_CMOLR_T_LNB_Process       },
#endif
    { D_ATC_EVENT_CEER_T,                       AtcAp_CEER_T_LNB_Process        },
    { D_ATC_EVENT_CIPCA_T,                      AtcAp_CIPCA_T_LNB_Process       },
    { D_ATC_EVENT_CGAUTH_T,                     AtcAp_CGAUTH_T_LNB_Process      },
    { D_ATC_EVENT_CNMPSD_T,                     AtcAp_CNMPSD_T_LNB_Process      },
    { D_ATC_EVENT_CPINR_T,                      AtcAp_CPINR_T_LNB_Process       },
    { D_ATC_EVENT_NPIN_T,                       AtcAp_NPIN_T_LNB_Process        },
    { D_ATC_EVENT_NPTWEDRXS_T,                  AtcAp_NPTWEDRXS_T_LNB_Process   },
    { D_ATC_EVENT_NTSETID_T,                    AtcAp_NTSETID_T_LNB_Process     },
    { D_ATC_EVENT_NCIDSTATUS_T,                 AtcAp_NCIDSTATUS_T_LNB_Process  },
    { D_ATC_EVENT_NGACTR_T,                     AtcAp_NGACTR_T_LNB_Process      },
    { D_ATC_EVENT_NIPINFO_T,                    AtcAp_NIPINFO_T_LNB_Process     },
    { D_ATC_EVENT_NQPODCP_T,                    AtcAp_NQPODCP_T_LNB_Process     },
    { D_ATC_EVENT_NSNPD_T,                      AtcAp_NSNPD_T_LNB_Process       },
    { D_ATC_EVENT_NQPNPD_T,                     AtcAp_NQPNPD_T_LNB_Process      },
    { D_ATC_EVENT_CNEC_T,                       AtcAp_CNEC_T_LNB_Process        },
    { D_ATC_EVENT_NRNPDM_T,                     AtcAp_NRNPDM_T_LNB_Process      },
    { D_ATC_EVENT_NCPCDPR_T,                    AtcAp_NCPCDPR_T_LNB_Process     },
    { D_ATC_EVENT_CEID_T,                       AtcAp_CEID_T_LNB_Process        },
    { D_ATC_EVENT_NCONFIG,                      AtcAp_NCONFIG_LNB_Process       },
    { D_ATC_EVENT_MNBIOTEVENT_T,                AtcAp_MNBIOTEVENT_T_LNB_Process },
    { D_ATC_EVENT_CGPIAF_T,                     AtcAp_CGPIAF_T_LNB_Process      },
#if SIMMAX_SUPPORT
    { D_ATC_EVENT_CUPREFERTH_T,                 AtcAp_CUPREFERTH_T_LNB_Process  },
#endif
    { D_ATC_EVENT_NPLMNS_T,                     AtcAp_NPLMNS_T_LNB_Process      },
    { D_ATC_EVENT_NLOCKF_T,                     AtcAp_NLOCKF_T_LNB_Process      },
    { D_ATC_EVENT_ZICCID_T,                     AtcAp_ZICCID_T_LNB_Process      },
    { D_ATC_EVENT_QCGDEFCONT_T,                 AtcAp_QCGDEFCONT_T_LNB_Process  },
    { D_ATC_EVENT_QENG_T,                       AtcAp_QENG_T_LNB_Process        },
    { D_ATC_EVENT_QCFG_T,                       AtcAp_QCFG_T_LNB_Process        },
    { D_ATC_EVENT_NUICC,                        AtcAp_NUICC_LNB_Process         },
    { D_ATC_EVENT_NUICC_T,                      AtcAp_NUICC_T_LNB_Process       },
    { D_ATC_EVENT_NPBPLMNS_T,                   AtcAp_NPBPLMNS_T_LNB_Process    },
    { D_ATC_EVENT_NBACKOFF_T,                   AtcAp_NBACKOFF_T_LNB_Process    },
#if VER_BC25
    { D_ATC_EVENT_CPIN_R,                       AtcAp_CPIN_R_Process            },
#endif
    { D_ATC_EVENT_QLOCKF_T,                     AtcAp_QLOCKF_T_LNB_Process      },
    { D_ATC_EVENT_QCSEARFCN_T,                  AtcAp_QCSEARFCN_T_LNB_Process   },
    { D_ATC_EVENT_QICSGP_T,                     AtcAp_QICSGP_T_LNB_Process      },
    { D_ATC_EVENT_QSPCHSC_T,                    AtcAp_QSPCHSC_T_LNB_Process     },
};

const ST_ATC_AP_MSG_PROC_TABLE AtcAp_DataIndProcTable[D_ATC_DATAIND_PROC_TBL_SIZE] =
{
    //OK/Err
    { D_ATC_AP_AT_CMD_RST,                      AtcAp_MsgProc_AT_CMD_RST        },

    //Ind
    { D_ATC_AP_SIMST_IND,                       AtcAp_MsgProc_SIMST_Ind         },
    { D_ATC_AP_XYIPDNS_IND,                     AtcAp_MsgProc_XYIPDNS_Ind       },
    { D_ATC_AP_CRTDCP_IND,                      AtcAp_MsgProc_CRTDCP_Ind        },
    { D_ATC_AP_CGAPNRC_IND,                     AtcAp_MsgProc_CGAPNRC_Ind       },
    { D_ATC_AP_CGEV_IND,                        AtcAp_MsgProc_CGEV_Ind          },
    { D_ATC_AP_CEREG_IND,                       AtcAp_MsgProc_CEREG_Ind         },
    { D_ATC_AP_CSCON_IND,                       AtcAp_MsgProc_CSCON_Ind         },
    { D_ATC_AP_NPTWEDRXP_IND,                   AtcAp_MsgProc_NPTWEDRXP_Ind     },
    { D_ATC_AP_CEDRXP_IND,                      AtcAp_MsgProc_CEDRXP_Ind        },
    { D_ATC_AP_CCIOTOPTI_IND,                   AtcAp_MsgProc_CCIOTOPTI_Ind     },
    { D_ATC_AP_L2THP_IND,                       AtcAp_MsgProc_L2_THP_Ind        },
    { D_ATC_AP_MALLOCADDR_IND,                  AtcAp_MsgProc_MALLOC_ADDR_Ind   },
    { D_ATC_AP_IPSN_IND,                        AtcAp_MsgProc_IPSN_Ind          },
    { D_ATC_AP_LOCALTIMEINFO_IND,               AtcAp_MsgProc_LOCALTIMEINFO_Ind },
    { D_ATC_OPELIST_SRCH_CNF,                   AtcAp_MsgProc_OPELIST_SRCH_CNF  },
    { D_ATC_AP_IPADDR_IND,                      AtcAp_MsgProc_PDNIPADDR_Ind     },
    { D_ATC_AP_NGACTR_IND,                      AtcAp_MsgProc_NGACTR_Ind        },
#ifdef NBIOT_SMS_FEATURE
    { D_ATC_AP_CMT_IND,                         AtcAp_MsgProc_CMT_Ind           },
    { D_ATC_AP_SMS_PDU_IND,                     AtcAp_MsgProc_SMS_PDU_Ind       },
#endif
    { D_ATC_AP_PINSTATUS_IND,                   AtcAp_MsgProc_PINSTATUS_Ind     },
    { D_ATC_AP_CELL_SRCH_IND,                   AtcAp_MsgProc_CELLSRCH_Ind      },
    { D_ATC_AP_NPBPLMNS_IND,                    AtcAp_MsgProc_NPBPLMNS_Ind      },
    { D_ATC_AP_NBACKOFF_IND,                    AtcAp_MsgProc_NBACKOFF_Ind      },
    { D_ATC_AP_NPLMNS_OOS_IND,                  AtcAp_MsgProc_NPLMNS_OOS_Ind    },
    { D_ATC_AP_FREQ_RSSI_IND,                   AtcAp_MsgProc_FREQ_RSSI_Ind     },
    { D_ATC_AP_SYSINFO_FAIL_IND,                AtcAp_MsgProc_SYSINFO_FAIL_Ind  },

    //AT CMD result
    { D_ATC_EVENT_CGSN,                         AtcAp_MsgProc_CGSN_Cnf          },
    { D_ATC_EVENT_CEREG_R,                      AtcAp_MsgProc_CEREG_R_Cnf       },
    { D_ATC_EVENT_CGATT_R,                      AtcAp_MsgProc_CGATT_R_Cnf       },
    { D_ATC_EVENT_CIMI,                         AtcAp_MsgProc_CIMI_Cnf          },
    { D_ATC_EVENT_CGDCONT_R,                    AtcAp_MsgProc_CGDCONT_R_Cnf     },
    { D_ATC_EVENT_CFUN_R,                       AtcAp_MsgProc_CFUN_R_Cnf        },
    { D_ATC_EVENT_CESQ,                         AtcAp_MsgProc_CESQ_Cnf          },
    { D_ATC_EVENT_CSQ,                          AtcAp_MsgProc_CSQ_Cnf           },
    { D_ATC_EVENT_CGPADDR,                      AtcAp_MsgProc_CGPADDR_Cnf       },
    { D_ATC_EVENT_CGPADDR_T,                    AtcAp_MsgProc_CGPADDR_T_Cnf     },
    { D_ATC_EVENT_CGACT_R,                      AtcAp_MsgProc_CGACT_R_Cnf       },
    { D_ATC_EVENT_CRTDCP_R,                     AtcAp_MsgProc_CRTDCP_R_Cnf      },
    { D_ATC_EVENT_SIMST_R,                      AtcAp_MsgProc_SIMST_R_Cnf       },
    { D_ATC_EVENT_CEDRXS_R,                     AtcAp_MsgProc_CEDRXS_R_Cnf      },
    { D_ATC_EVENT_CPSMS_R,                      AtcAp_MsgProc_CPSMS_R_Cnf       },
    { D_ATC_EVENT_CGAPNRC,                      AtcAp_MsgProc_CGAPNRC_Cnf       },
    { D_ATC_EVENT_CGAPNRC_T,                    AtcAp_MsgProc_CGAPNRC_T_Cnf     },
    { D_ATC_EVENT_CSCON_R,                      AtcAp_MsgProc_CSCON_R_Cnf       },
    { D_ATC_EVENT_NL2THP_R,                     AtcAp_MsgProc_NL2THP_R_Cnf      },
    { D_ATC_EVENT_NUESTATS,                     AtcAp_MsgProc_NUESTATS_Cnf      },
    { D_ATC_EVENT_NEARFCN_R,                    AtcAp_MsgProc_NEARFCN_R_Cnf     },
    { D_ATC_EVENT_NBAND_R,                      AtcAp_MsgProc_NBAND_R_Cnf       },
    { D_ATC_EVENT_NBAND_T,                      AtcAp_MsgProc_NBAND_T_Cnf       },
    { D_ATC_EVENT_NCONFIG_R,                    AtcAp_MsgProc_NCONFIG_R_Cnf     },
    { D_ATC_EVENT_NCONFIG_T,                    AtcAp_MsgProc_NCONFIG_T_Cnf     },
    { D_ATC_EVENT_NSET_R,                       AtcAp_MsgProc_NSET_R_Cnf        },
#ifdef ESM_DEDICATED_EPS_BEARER
    { D_ATC_EVENT_CGDSCONT_R,                   AtcAp_MsgProc_CGDSCONT_R_Cnf    },
    { D_ATC_EVENT_CGDSCONT_T,                   AtcAp_MsgProc_CGDSCONT_T_Cnf    },
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
    { D_ATC_EVENT_CGTFT_R,                      AtcAp_MsgProc_CGTFT_R_Cnf       },
#endif
    { D_ATC_EVENT_CGEQOS_R,                     AtcAp_MsgProc_CGEQOS_R_Cnf      },
#ifdef ESM_EPS_BEARER_MODIFY
    { D_ATC_EVENT_CGCMOD_T,                     AtcAp_MsgProc_CGCMOD_T_Cnf      },
#endif
    { D_ATC_EVENT_COPS_R,                       AtcAp_MsgProc_COPS_R_Cnf        },
    { D_ATC_EVENT_COPS_T,                       AtcAp_MsgProc_COPS_T_Cnf        },
    { D_ATC_EVENT_CGEREP_R,                     AtcAp_MsgProc_CGEREP_R_Cnf      },
    { D_ATC_EVENT_CCIOTOPT_R,                   AtcAp_MsgProc_CCIOTOPT_R_Cnf    },
    { D_ATC_EVENT_CEDRXRDP,                     AtcAp_MsgProc_CEDRXRDP_Cnf      },
    { D_ATC_EVENT_CGEQOSRDP,                    AtcAp_MsgProc_CGEQOSRDP_Cnf     },
    { D_ATC_EVENT_CGEQOSRDP_T,                  AtcAp_MsgProc_CGEQOSRDP_T_Cnf   },
    { D_ATC_EVENT_CTZR_R,                       AtcAp_MsgProc_CTZR_R_Cnf        },
    { D_ATC_EVENT_CGCONTRDP,                    AtcAp_MsgProc_CGCONTRDP_Cnf     },
    { D_ATC_EVENT_CGCONTRDP_T,                  AtcAp_MsgProc_CGCONTRDP_T_Cnf   },
    { D_ATC_EVENT_CEER,                         AtcAp_MsgProc_CEER_Cnf          },
    { D_ATC_EVENT_CIPCA_R,                      AtcAp_MsgProc_CIPCA_R_Cnf       },
    { D_ATC_EVENT_CGAUTH_R,                     AtcAp_MsgProc_CGAUTH_R_Cnf      },
    { D_ATC_EVENT_NPOWERCLASS_R,                AtcAp_MsgProc_NPOWERCLASS_R_Cnf },
    { D_ATC_EVENT_NPOWERCLASS_T,                AtcAp_MsgProc_NPOWERCLASS_T_Cnf },
    { D_ATC_EVENT_NPTWEDRXS_R,                  AtcAp_MsgProc_NPTWEDRXS_R_Cnf   },
    { D_ATC_EVENT_NCIDSTATUS,                   AtcAp_MsgProc_NCIDSTATUS_Cnf    },
    { D_ATC_EVENT_NCIDSTATUS_R,                 AtcAp_MsgProc_NCIDSTATUS_R_Cnf  },
    { D_ATC_EVENT_NGACTR_R,                     AtcAp_MsgProc_NGACTR_R_Cnf      },
    { D_ATC_EVENT_NPOPB_R,                      AtcAp_MsgProc_NPOPB_R_Cnf       },
    { D_ATC_EVENT_NIPINFO_R,                    AtcAp_MsgProc_NIPINFO_R_Cnf     },
    { D_ATC_EVENT_NQPODCP,                      AtcAp_MsgProc_NQPODCP_Cnf       },
    { D_ATC_EVENT_NQPNPD,                       AtcAp_MsgProc_NQPNPD_Cnf        },
    { D_ATC_EVENT_CNEC_R,                       AtcAp_MsgProc_CNEC_R_Cnf        },
    { D_ATC_EVENT_NRNPDM_R,                     AtcAp_MsgProc_NRNPDM_R_Cnf      },
    { D_ATC_EVENT_NCPCDPR_R,                    AtcAp_MsgProc_NCPCDPR_R_Cnf     },
    { D_ATC_EVENT_CEID,                         AtcAp_MsgProc_CEID_Cnf          },
#ifdef NBIOT_SMS_FEATURE
    { D_ATC_EVENT_CSMS,                         AtcAp_MsgProc_CSMS_Cnf          },
    { D_ATC_EVENT_CSMS_R,                       AtcAp_MsgProc_CSMS_R_Cnf        },
    { D_ATC_EVENT_CMGC,                         AtcAp_MsgProc_CMGC_Cnf          },
    { D_ATC_EVENT_CMGS,                         AtcAp_MsgProc_CMGS_Cnf          },
    { D_ATC_EVENT_CMGF_R,                       AtcAp_MsgProc_CMGF_R_Cnf        },
    { D_ATC_EVENT_CSCA_R,                       AtcAp_MsgProc_CSCA_R_Cnf        },
    { D_ATC_EVENT_CMMS_R,                       AtcAp_MsgProc_CMMS_R_Cnf        },
#endif
    { D_ATC_EVENT_NPIN,                         AtcAp_MsgProc_NPIN_Cnf          },
    { D_ATC_EVENT_CPIN_R,                       AtcAp_MsgProc_CPIN_R_Cnf        },
    { D_ATC_EVENT_CLCK,                         AtcAp_MsgProc_CLCK_Cnf          },
    { D_ATC_EVENT_CPINR,                        AtcAp_MsgProc_CPINR_Cnf         },
    { D_ATC_EVENT_NCCID,                        AtcAp_MsgProc_NCCID_Cnf         },
    { D_ATC_EVENT_CCHO,                         AtcAp_MsgProc_CCHO_Cnf          },
    { D_ATC_EVENT_CCHC,                         AtcAp_MsgProc_CCHC_Cnf          },

    { D_ATC_EVENT_CRSM,                         AtcAp_MsgProc_CRSM_Cnf          },
    { D_ATC_EVENT_CGLA,                         AtcAp_MsgProc_CGLA_Cnf          },
    { D_ATC_EVENT_CSIM,                         AtcAp_MsgProc_CSIM_Cnf          },
#ifdef LCS_MOLR_ENABLE
    { D_ATC_EVENT_CMOLR_R,                      AtcAp_MsgProc_CMOLR_R_Cnf       },
    { D_ATC_AP_CMOLRE_IND,                      AtcAp_MsgProc_CMOLRE_Ind        },
    { D_ATC_AP_CMOLRG_IND,                      AtcAp_MsgProc_CMOLRG_Ind        },
#endif
    { D_ATC_NO_CARRIER_IND,                     AtcAp_MsgProc_NoCarrier_Ind     },
    { D_ATC_AP_PSINFO_IND,                      AtcAp_MsgProc_PSINFO_Ind        },
    { D_ATC_AP_CSODCPR_IND,                     AtcAp_MsgProc_CSODCPR_Ind       },
    { D_ATC_AP_NSNPDR_IND,                      AtcAp_MsgProc_NSNPDR_Ind        },
    { D_ATC_AP_NIPINFO_IND,                     AtcAp_MsgProc_NIPINFO_Ind       },
    { D_ATC_AP_CNEC_IND,                        AtcAp_MsgProc_CNEC_Ind          },
    { D_ATC_AP_MNBIOTEVENT_IND,                 AtcAp_MsgProc_MNBIOTEVENT_Ind   },
    { D_ATC_EVENT_CGPIAF_R,                     AtcAp_MsgProc_CGPIAF_R_Cnf      },
#if SIMMAX_SUPPORT
    { D_ATC_EVENT_CUPREFERTH_R,                 AtcAp_MsgProc_CUPREFERTH_R_Cnf  },
#endif
    { D_ATC_EVENT_NPLMNS_R,                     AtcAp_MsgProc_NPLMNS_R_Cnf      },
    { D_ATC_EVENT_NLOCKF_R,                     AtcAp_MsgProc_NLOCKF_R_Cnf      },
    { D_ATC_EVENT_ZICCID,                       AtcAp_MsgProc_ZICCID_Cnf        },
    { D_ATC_EVENT_ZCELLINFO,                    AtcAp_MsgProc_ZCELLINFO_Cnf     },
    { D_ATC_EVENT_QCGDEFCONT_R,                 AtcAp_MsgProc_QCGDEFCONT_R_Cnf  },
    { D_ATC_EVENT_QBAND_R,                      AtcAp_MsgProc_QBAND_R_Cnf       },
    { D_ATC_EVENT_QBAND_T,                      AtcAp_MsgProc_QBAND_T_Cnf       },
    { D_ATC_EVENT_QCCID,                        AtcAp_MsgProc_QCCID_Cnf         },
    { D_ATC_EVENT_QENG,                         AtcAp_MsgProc_QENG_Cnf          },
    { D_ATC_EVENT_QCFG_R,                       AtcAp_MsgProc_QCFG_R_Cnf        },
    { D_ATC_EVENT_MNBIOTEVENT_R,                AtcAp_MsgProc_MNBIOTEVENT_R_Cnf },
    { D_ATC_EVENT_NSIMWC,                       AtcAp_MsgProc_NSIMWC_Cnf        },
    { D_ATC_EVENT_QNIDD,                        AtcAp_MsgProc_QNIDD_Cnf         },
    { D_ATC_AP_QNIDD_IND,                       AtcAp_MsgProc_QNIDD_Ind         },
    { D_ATC_AP_XYCELLS_IND,                     AtcAp_MsgProc_XYCELLS_Ind       },
    { D_ATC_EVENT_PRESETFREQ_R,                 AtcAp_MsgProc_PRESETFREQ_R_Cnf  },
    { D_ATC_EVENT_NPBPLMNS_R,                   AtcAp_MsgProc_NPBPLMNS_R_Cnf    },
    { D_ATC_EVENT_NBACKOFF_R,                   AtcAp_MsgProc_NBACKOFF_R_Cnf    },
    { D_ATC_EVENT_SIMUUICC_R,                   AtcAp_MsgProc_SIMUUICC_R_Cnf        },
    { D_ATC_EVENT_QLOCKF_R,                     AtcAp_MsgProc_QLOCKF_R_Cnf      },
    { D_ATC_EVENT_QICSGP_R,                     AtcAp_MsgProc_QICSGP_R_Cnf      },
    { D_ATC_EVENT_QSPCHSC_R,                    AtcAp_MsgProc_QSPCHSC_R_Cnf     },
    { D_ATC_AP_SIM_DATA_DOWNLOAD_IND,           AtcAp_MsgProc_SimDataDownload_Ind },
};

const ST_ATC_COMMAND_ANAL_TABLE ATC_Plus_CommandTable[] =
{
//basic
    //{ (unsigned char *)"+CGMI"      ,    ATC_CGMI_LNB_Command           },
    //{ (unsigned char *)"+CGMR"      ,    ATC_CGMR_LNB_Command           },
    { {(unsigned char *)"+CGSN"      },    ATC_CGSN_LNB_Command           },
    { {(unsigned char *)"+CEREG"     },    ATC_CEREG_LNB_Command          },
    { {(unsigned char *)"+CGATT"     },    ATC_CGATT_LNB_Command          },
    { {(unsigned char *)"+CIMI"      },    ATC_CIMI_LNB_Command           },
    { {(unsigned char *)"+CGDCONT"   },    ATC_CGDCONT_LNB_Command        },
    { {(unsigned char *)"+CFUN"      },    ATC_CFUN_LNB_Command           },
    { {(unsigned char *)"+CMEE"      },    ATC_CMEE_LNB_Command           },
    { {(unsigned char *)"+CLAC"      },    ATC_CLAC_LNB_Command           },
    { {(unsigned char *)"+CESQ"      },    ATC_CESQ_LNB_Command           },
    { {(unsigned char *)"+CSQ"       },    ATC_CSQ_LNB_Command            },
    { {(unsigned char *)"+CGPADDR"   },    ATC_CGPADDR_LNB_Command        },                      //To be continue
    //{ (unsigned char *)"+CGMM"      ,    ATC_CGMM_LNB_Command           },
    //{ (unsigned char *)"+CGDATA"    ,    ATC_CGDATA_LNB_Command         },
    { {(unsigned char *)"+CSODCP"    },    ATC_CSODCP_LNB_Command         },
    { {(unsigned char *)"+CRTDCP"    },    ATC_CRTDCP_LNB_Command         },
    //{ (unsigned char *)"+CRC"       ,    ATC_CRC_LNB_Command            },
    //{ (unsigned char *)"+CMUX"      ,    ATC_CMUX_LNB_Command           },
    { {(unsigned char *)"+CGACT"     },    ATC_CGACT_LNB_Command          },
    { {(unsigned char *)"+CEDRXS"    },    ATC_CEDRXS_LNB_Command         },
    { {(unsigned char *)"+CPSMS"     },    ATC_CPSMS_LNB_Command          },
    { {(unsigned char *)"+CGAPNRC"   },    ATC_CGAPNRC_LNB_Command        },
    { {(unsigned char *)"+CGEQOSRDP" },    ATC_CGEQOSRDP_LNB_Command      },
//other
#ifdef ESM_DEDICATED_EPS_BEARER
    { {(unsigned char *)"+CGDSCONT"  },    ATC_CGDSCONT_LNB_Command       },
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
    { {(unsigned char *)"+CGTFT"     },    ATC_CGTFT_LNB_Command          },
#endif
    { {(unsigned char *)"+CGEQOS"    },    ATC_CGEQOS_LNB_Command         },
#ifdef ESM_EPS_BEARER_MODIFY
    { {(unsigned char *)"+CGCMOD"    },    ATC_CGCMOD_LNB_Command         },
#endif
    { {(unsigned char *)"+CGEREP"    },    ATC_CGEREP_Command         },
//SMS
#ifdef NBIOT_SMS_FEATURE
    { {(unsigned char *)"+CSMS"      },    ATC_CSMS_Command               },
    { {(unsigned char *)"+CMGF"      },    ATC_CMGF_Command               },
    { {(unsigned char *)"+CSCA"      },    ATC_CSCA_Command               },
    //{ (unsigned char *)"+CNMI"      ,    ATC_CNMI_Command               },
    { {(unsigned char *)"+CMGS"      },    ATC_CMGS_Command               },
    { {(unsigned char *)"+CNMA"      },    ATC_CNMA_Command               },
    { {(unsigned char *)"+CMGC"      },    ATC_CMGC_Command               },
    { {(unsigned char *)"+CMMS"      },    ATC_CMMS_Command               },
#endif
    { {(unsigned char *)"+COPS"      },    ATC_COPS_LNB_Command           },
//shao add for USAT
    { {(unsigned char *)"+CSIM"      },    ATC_CSIM_LNB_Command           },
    { {(unsigned char *)"+CCHO"      },    ATC_CCHO_LNB_Command           },
    { {(unsigned char *)"+CCHC"      },    ATC_CCHC_LNB_Command           },
    { {(unsigned char *)"+CGLA"      },    ATC_CGLA_LNB_Command           },
    { {(unsigned char *)"+CRSM"      },    ATC_CRSM_LNB_Command           },
    { {(unsigned char *)"+CSCON"     },    ATC_CSCON_LNB_Command          },
    { {(unsigned char *)"+CCIOTOPT"  },    ATC_CCIOTOPT_LNB_Command       },
    { {(unsigned char *)"+CEDRXRDP"  },    ATC_CEDRXRDP_LNB_Command       },
    { {(unsigned char *)"+CTZR"      },    ATC_CTZR_LNB_Command           },
    { {(unsigned char *)"+CGCONTRDP" },    ATC_CGCONTRDP_LNB_Command      },
#ifdef LCS_MOLR_ENABLE
    { {(unsigned char *)"+CMOLR"     },    ATC_CMOLR_LNB_Command          },
#endif
//
    { {(unsigned char *)"+CPWD"      },    ATC_CPWD_LNB_Command           },
    { {(unsigned char *)"+CPIN"      },    ATC_CPIN_LNB_Command           },
    { {(unsigned char *)"+CLCK"      },    ATC_CLCK_LNB_Command           },
    //{ (unsigned char *)"+NRB"        ,   ATC_NRB_LNB_Command            },
    { {(unsigned char *)"+NUESTATS"   },   ATC_NUESTATS_LNB_Command       },
    { {(unsigned char *)"+NEARFCN"    },   ATC_NEARFCN_LNB_Command        },
    { {(unsigned char *)"+NBAND"      },   ATC_NBAND_LNB_Command          },
    { {(unsigned char *)"+NCONFIG"    },   ATC_NCONFIG_LNB_Command        },
    { {(unsigned char *)"+NCCID"      },   ATC_NCCID_LNB_Command          },
    //{ (unsigned char *)"+NPSMR"      ,   ATC_NPSMR_LNB_Command          },
    //{ (unsigned char *)"+NMGS"       ,   ATC_NMGS_LNB_Command           },
   // { (unsigned char *)"+NMGR"       ,   ATC_NMGR_LNB_Command           },
   // { (unsigned char *)"+NQMGS"      ,   ATC_NQMGS_LNB_Command          },
   // { (unsigned char *)"+NQMGR"      ,   ATC_NQMGR_LNB_Command          },
   // { (unsigned char *)"+QLWUDATAEX" ,   ATC_QLWUDATAEX_LNB_Command     },
   // { (unsigned char *)"+NCDP"       ,   ATC_NCDP_LNB_Command           },
    { {(unsigned char *)"+NCSEARFCN"  },   ATC_NCSEARFCN_LNB_Command        },
    { {(unsigned char *)"+RAI"        },   ATC_RAI_LNB_Command              },
    { {(unsigned char *)"+NFPLMN"     },   ATC_NFPLMN_LNB_Command           },
    { {(unsigned char *)"+NL2THP"     },   ATC_NL2THP_LNB_Command           },
    { {(unsigned char *)"+NSET"       },   ATC_NSET_LNB_Command             },
   // { (unsigned char *)"+NATSPEED"   ,   ATC_NATSPEED_LNB_Command       },
   // { (unsigned char *)"+NSOCR"      ,   ATC_NSOCR_LNB_Command          },
   //{ (unsigned char *)"+NSOST"      ,   ATC_NSOST_LNB_Command          },
   // { (unsigned char *)"+NPING"      ,   ATC_NPING_LNB_Command          },
    { {(unsigned char *)"+CEER"       },     ATC_CEER_LNB_Command           },
    { {(unsigned char *)"+CIPCA"      },     ATC_CIPCA_LNB_Command          },
    { {(unsigned char *)"+CGAUTH"     },     ATC_CGAUTH_LNB_Command         },
    { {(unsigned char *)"+CNMPSD"     },     ATC_CNMPSD_LNB_Command         },
    { {(unsigned char *)"+CPINR"      },     ATC_CPINR_LNB_Command          },
    { {(unsigned char *)"+NPOWERCLASS"},     ATC_NPOWERCLASS_LNB_Command    },
    { {(unsigned char *)"+NPTWEDRXS"  },     ATC_NPTWEDRXS_LNB_Command      },
    { {(unsigned char *)"+NPIN"       },     ATC_NPIN_LNB_Command           },
    { {(unsigned char *)"+NTSETID"    },     ATC_NTSETID_LNB_Command        },
    { {(unsigned char *)"+NCIDSTATUS" },     ATC_NCIDSTATUS_LNB_Command     },
    { {(unsigned char *)"+NGACTR"     },     ATC_NGACTR_LNB_Command         },
    { {(unsigned char *)"+NPOPB"      },     ATC_NPOPB_LNB_Command          },    
    { {(unsigned char *)"+NIPINFO"    },     ATC_NIPINFO_LNB_Command        },
    { {(unsigned char *)"+NQPODCP"    },     ATC_NQPODCP_LNB_Command        },
    { {(unsigned char *)"+NSNPD"      },     ATC_NSNPD_LNB_Command          },
    { {(unsigned char *)"+NQPNPD"     },     ATC_NQPNPD_LNB_Command         },
    { {(unsigned char *)"+CNEC"       },     ATC_CNEC_LNB_Command           },
    { {(unsigned char *)"+NRNPDM"     },     ATC_NRNPDM_LNB_Command         },
    { {(unsigned char *)"+NCPCDPR"    },     ATC_NCPCDPR_LNB_Command        },
    { {(unsigned char *)"+CEID"       },     ATC_CEID_LNB_Command           },
    { {(unsigned char *)"+QCSEARFCN"  },     ATC_QCSEARFCN_LNB_Command      },
    { {(unsigned char *)"+QLOCKF"     },     ATC_QLOCKF_LNB_Command         },
#if VER_BC25
    { {(unsigned char *)"+QNBIOTEVENT"},     ATC_MNBIOTEVENT_LNB_Command    },
#else
    { {(unsigned char *)"+MNBIOTEVENT"},     ATC_MNBIOTEVENT_LNB_Command    },
#endif
    { {(unsigned char *)"+CGPIAF"     },     ATC_CGPIAF_LNB_Command         },
#if SIMMAX_SUPPORT
    { {(unsigned char *)"+CUPREFER"   },     ATC_CUPREFER_LNB_Command       },
    { {(unsigned char *)"+CUPREFERTH" },     ATC_CUPREFERTH_LNB_Command     },
#endif
    { {(unsigned char *)"+NPLMNS" },         ATC_NPLMNS_LNB_Command         },
    { {(unsigned char *)"+NLOCKF"     },     ATC_NLOCKF_LNB_Command         },
    { {(unsigned char *)"+ZICCID"     },     ATC_ZICCID_LNB_Command         },
    { {(unsigned char *)"+ZCELLINFO"  },     ATC_ZCELLINFO_LNB_Command      },
    { {(unsigned char *)"+QCGDEFCONT" },     ATC_QCGDEFCONT_LNB_Command     },
    { {(unsigned char *)"+QBAND"      },     ATC_QBAND_LNB_Command          },
    { {(unsigned char *)"+QCCID"      },     ATC_QCCID_LNB_Command          },
    { {(unsigned char *)"+QENG"       },     ATC_QENG_LNB_Command           },
    { {(unsigned char *)"+QCFG"       },     ATC_QCFG_LNB_Command           },
#if Custom_09
    { {(unsigned char *)"+QUSIMWC"    },     ATC_NSIMWC_LNB_Command         },
#else
    { {(unsigned char *)"+NSIMWC"     },     ATC_NSIMWC_LNB_Command         },
#endif
    { {(unsigned char *)"+NUICC"      },     ATC_NUICC_LNB_Command          },
    { {(unsigned char *)"+PSTEST"     },     ATC_PSTEST_LNB_Command         },
    { {(unsigned char *)"+QNIDD"      },     ATC_QNIDD_LNB_Command          },
    { {(unsigned char *)"+XYCELLS"    },     ATC_XYCELLS_LNB_Command        },
    { {(unsigned char *)"+PRESETFREQ" },     ATC_PRESETFREQ_LNB_Command     },
    { {(unsigned char *)"+NPBPLMNS"   },     ATC_NPBPLMNS_LNB_Command       },
    { {(unsigned char *)"+NBACKOFF"   },     ATC_NBACKOFF_LNB_Command       },
    { {(unsigned char *)"+SIMUUICC"   },     ATC_SIMUUICC_LNB_Command       },
    { {(unsigned char *)"+QICSGP"     },     ATC_QICSGP_LNB_Command         },
    { {(unsigned char *)"+QSPCHSC"    },     ATC_QSPCHSC_LNB_Command        },
    { {(unsigned char *)""            },(unsigned char (*)(unsigned char *,unsigned char *))NULL   }
};

const ST_ATC_COMMAND_ANAL_TABLE ATC_Symbol_CommandTable[] =
{
//    { (unsigned char *)"&F"         ,    ATC_F_LNB_Command              },
    { {(unsigned char *)"&W"          },     ATC_W_LNB_Command              },
    { {(unsigned char *)"&W0"         },     ATC_W0_LNB_Command             },
    { {(unsigned char *)"^SIMST"     },    ATC_SIMST_LNB_Command          },
    { {(unsigned char *)""           },(unsigned char (*)(unsigned char *,unsigned char *))NULL   }
};

const ST_ATC_COMMAND_ANAL_TABLE ATC_Single_CommandTable[] =
{
//    { (unsigned char *)"S3"         ,    ATC_S3_LNB_Command             },
//    { (unsigned char *)"S4"         ,    ATC_S4_LNB_Command             },
//    { (unsigned char *)"S5"         ,    ATC_S5_LNB_Command             },
//    { {(unsigned char *)"E"          },  ATC_E_LNB_Command              },
//    { (unsigned char *)"V"          ,    ATC_V_LNB_Command              },
    { {(unsigned char *)""           },(unsigned char (*)(unsigned char *,unsigned char *))NULL   }
};

const ST_ATC_EVENT_TABLE ATC_Event_Table[] =
{
    {   D_ATC_EVENT_CGSN,              0xFF,                    D_ATC_EVENT_CGSN_T,       D_ATC_EVENT_CGSN},   /*EVENT_CGSN     0 */
    {   D_ATC_EVENT_CEREG,             D_ATC_EVENT_CEREG_R,     D_ATC_EVENT_CEREG_T,      0xFF},               /*EVENT_CEREG    1 */
    {   D_ATC_EVENT_CGATT,             D_ATC_EVENT_CGATT_R,     D_ATC_EVENT_CGATT_T,      0xFF},               /*EVENT_CGATT    2 */
    
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CIMI_T,       D_ATC_EVENT_CIMI},   /*EVENT_CIMI     3 */
    {   D_ATC_EVENT_CGDCONT,           D_ATC_EVENT_CGDCONT_R,   D_ATC_EVENT_CGDCONT_T,    0xFF},               /*EVENT_CGDCONT  4 */
    {   D_ATC_EVENT_CFUN,              D_ATC_EVENT_CFUN_R,      D_ATC_EVENT_CFUN_T,       0xFF},               /*EVENT_CFUN     5 */

    {   D_ATC_EVENT_CMEE,              D_ATC_EVENT_CMEE_R,      D_ATC_EVENT_CMEE_T,       0xFF},               /*EVENT_CMEE     6 */
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CLAC_T,       D_ATC_EVENT_CLAC},   /*EVENT_CLAC     7 */
    {   D_ATC_EVENT_CESQ,              0xFF,                    D_ATC_EVENT_CESQ_T,       D_ATC_EVENT_CESQ},   /*EVENT_CESQ     8 */

    {   D_ATC_EVENT_CGPADDR,           0xFF,                    D_ATC_EVENT_CGPADDR_T,    D_ATC_EVENT_CGPADDR},/*EVENT_CGPADDR  9 */
    {   D_ATC_EVENT_CGACT,             D_ATC_EVENT_CGACT_R,     D_ATC_EVENT_CGACT_T,      0xFF},               /*EVENT_CGACT    10 */
    {   D_ATC_EVENT_CSODCP,            0xFF,                    D_ATC_EVENT_CSODCP_T,     0xFF},               /*EVENT_CSODCP   11 */

    {   D_ATC_EVENT_CRTDCP,            D_ATC_EVENT_CRTDCP_R,    D_ATC_EVENT_CRTDCP_T,     0xFF},               /*EVENT_CRTDCP   12 */
    //{   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_E},      /*EVENT_E        13 */
    {   0xFF,                          D_ATC_EVENT_SIMST_R,     0xFF,                     0xFF},               /*EVENT_SIMST    14 */

    {   D_ATC_EVENT_CEDRXS,            D_ATC_EVENT_CEDRXS_R,    D_ATC_EVENT_CEDRXS_T,     0xFF},               /*EVENT_CEDRXS   15 */
    {   D_ATC_EVENT_CPSMS,             D_ATC_EVENT_CPSMS_R,     D_ATC_EVENT_CPSMS_T,      0xFF},               /*EVENT_CPSMS    16 */
    {   D_ATC_EVENT_CGAPNRC,           0xFF,                    D_ATC_EVENT_CGAPNRC_T,    D_ATC_EVENT_CGAPNRC},/*EVENT_CGAPNRC  17 */
#ifdef ESM_DEDICATED_EPS_BEARER
    {   D_ATC_EVENT_CGDSCONT,          D_ATC_EVENT_CGDSCONT_R,  D_ATC_EVENT_CGDSCONT_T,   0xFF},               /*EVENT_CGDSCONT 18 */
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
    {   D_ATC_EVENT_CGTFT,             D_ATC_EVENT_CGTFT_R,     D_ATC_EVENT_CGTFT_T,      0xFF},               /*EVENT_CGTFT    19 */
#endif
    {   D_ATC_EVENT_CGEQOS,            D_ATC_EVENT_CGEQOS_R,    D_ATC_EVENT_CGEQOS_T,     0xFF},               /*EVENT_CGEQOS   20 */
#ifdef ESM_EPS_BEARER_MODIFY
    {   D_ATC_EVENT_CGCMOD,            0xFF,                    D_ATC_EVENT_CGCMOD_T,     D_ATC_EVENT_CGCMOD}, /*EVENT_CGCMOD   21 */
#endif
    {   D_ATC_EVENT_CSMS,              D_ATC_EVENT_CSMS_R,      D_ATC_EVENT_CSMS_T,       0xFF},               /*EVENT_CSMS     22 */
    {   D_ATC_EVENT_CMGF,              D_ATC_EVENT_CMGF_R,      D_ATC_EVENT_CMGF_T,       0xFF},               /*EVENT_CMGF     23 */

    {   D_ATC_EVENT_CSCA,              D_ATC_EVENT_CSCA_R,      D_ATC_EVENT_CSCA_T,       0xFF},               /*EVENT_CSCA     24 */
    //{   D_ATC_EVENT_CNMI,              D_ATC_EVENT_CNMI_R,      D_ATC_EVENT_CNMI_T,       0xFF},               /*EVENT_CNMI     25 */
    {   D_ATC_EVENT_CMGS,              0xFF,                    D_ATC_EVENT_CMGS_T,       0xFF},               /*EVENT_CMGS     26 */

    {   D_ATC_EVENT_CNMA,              0xFF,                    D_ATC_EVENT_CNMA_T,       D_ATC_EVENT_CNMA},   /*EVENT_CNMA     27 */
    {   D_ATC_EVENT_COPS,              D_ATC_EVENT_COPS_R,      D_ATC_EVENT_COPS_T,       0xFF},               /*EVENT_COPS     28 */
    {   D_ATC_EVENT_CSCON,             D_ATC_EVENT_CSCON_R,     D_ATC_EVENT_CSCON_T,      0xFF},               /*EVENT_CSCON    29 */

    {   D_ATC_EVENT_CGEREP,            D_ATC_EVENT_CGEREP_R,    D_ATC_EVENT_CGEREP_T,     0xFF},               /*EVENT_CGEREP   30 */
    {   D_ATC_EVENT_CCIOTOPT,          D_ATC_EVENT_CCIOTOPT_R,  D_ATC_EVENT_CCIOTOPT_T,   0xFF},                /*EVENT_CCIOTOPT 31 */
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CEDRXRDP_T,   D_ATC_EVENT_CEDRXRDP},/*EVENT_CEDRXRDP 32 */

    {   D_ATC_EVENT_CGEQOSRDP,         0xFF,                    D_ATC_EVENT_CGEQOSRDP_T,  D_ATC_EVENT_CGEQOSRDP},/*EVENT_CGEQOSRDP 33 */
    {   D_ATC_EVENT_CTZR,              D_ATC_EVENT_CTZR_R,      D_ATC_EVENT_CTZR_T,       0xFF},                 /*EVENT_CTZR     34 */
    {   D_ATC_EVENT_CGCONTRDP,         0xFF,                    D_ATC_EVENT_CGCONTRDP_T,  D_ATC_EVENT_CGCONTRDP},/*EVENT_CGCONTRDP 35*/

    {   D_ATC_EVENT_CPIN,              D_ATC_EVENT_CPIN_R,      D_ATC_EVENT_CPIN_T,       0xFF},                 /*EVENT_CPIN     36 */
    {   D_ATC_EVENT_CLCK,              0xFF,                    D_ATC_EVENT_CLCK_T,       0xFF},                 /*EVENT_CLCK     37 */
    {   D_ATC_EVENT_CPWD,              0xFF,                    D_ATC_EVENT_CPWD_T,       0xFF},                 /*EVENT_CPWD     38 */


    {   D_ATC_EVENT_NUESTATS,          0xFF,                    D_ATC_EVENT_NUESTATS_T,   D_ATC_EVENT_NUESTATS}, /*EVENT_NUESTATS 39 */
    {   D_ATC_EVENT_NEARFCN,           D_ATC_EVENT_NEARFCN_R,   D_ATC_EVENT_NEARFCN_T,    0xFF},                 /*EVENT_NEARFCN  40 */
    {   D_ATC_EVENT_NBAND,             D_ATC_EVENT_NBAND_R,     D_ATC_EVENT_NBAND_T,      0xFF},                 /*EVENT_NBAND    41 */

    {   D_ATC_EVENT_NCONFIG,           D_ATC_EVENT_NCONFIG_R,   D_ATC_EVENT_NCONFIG_T,    0xFF},                 /*EVENT_NCONFIG  42 */
    {   0xFF,                          D_ATC_EVENT_NCCID,       D_ATC_EVENT_NCCID_T,      D_ATC_EVENT_NCCID},    /*EVENT_NCCID    43 */
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_NCSEARFCN},/*EVENT_NCSEARFCN 44 */

    {   D_ATC_EVENT_RAI,               0xFF,                    0xFF,                     0xFF},                 /*EVENT_RAI      45 */
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_NFPLMN},   /*EVENT_NFPLMN   46 */
    {   D_ATC_EVENT_NL2THP,            D_ATC_EVENT_NL2THP_R,    D_ATC_EVENT_NL2THP_T,     0xFF},                 /*EVENT_NL2THP   47 */

    {   0xFF,                          0xFF,                    D_ATC_EVENT_CSQ_T,        D_ATC_EVENT_CSQ},      /*EVENT_CSQ      48 */
    {   D_ATC_EVENT_NSET,              0xFF ,                   0xFF,                     0xFF},                 /*EVENT_NSET     49 */
#ifdef LCS_MOLR_ENABLE
    {   D_ATC_EVENT_CMOLR,             D_ATC_EVENT_CMOLR_R ,    D_ATC_EVENT_CMOLR_T,      D_ATC_EVENT_CMOLR},    /*EVENT_CMOLR    50 */
#endif
    {   D_ATC_EVENT_CCHC,              0xFF,                    D_ATC_EVENT_CCHC_T,       0xFF},                 /* EVENT_CCHC */
    {   D_ATC_EVENT_CCHO,              0xFF,                    D_ATC_EVENT_CCHO_T,       0xFF},                 /* EVENT_CCHO */
    {   D_ATC_EVENT_CGLA,              0xFF,                    D_ATC_EVENT_CGLA_T,       0xFF},                 /* EVENT_CGLR */
    
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CEER_T,       D_ATC_EVENT_CEER},     /* EVENT_CEER */
    {   D_ATC_EVENT_CIPCA,             D_ATC_EVENT_CIPCA_R,     D_ATC_EVENT_CIPCA_T,      0xFF},                 /* EVENT_CIPCA */
    {   D_ATC_EVENT_CGAUTH,            D_ATC_EVENT_CGAUTH_R,    D_ATC_EVENT_CGAUTH_T,     0xFF},                 /* EVENT_CGAUTH */
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CNMPSD_T,     D_ATC_EVENT_CNMPSD},   /* EVENT_CNMPSD */
    {   D_ATC_EVENT_CPINR,             0xFF,                    D_ATC_EVENT_CPINR_T,      D_ATC_EVENT_CPINR},                 /* EVENT_CPINR */
    {   D_ATC_EVENT_CRSM,              0xFF,                    D_ATC_EVENT_CRSM_T,       0xFF},                 /* EVENT_CRSM */
    {   D_ATC_EVENT_CSIM,              0xFF,                    D_ATC_EVENT_CSIM_T,       0xFF},                 /* EVENT_CSIM */
    {   D_ATC_EVENT_CMGC,              0xFF,                    D_ATC_EVENT_CMGC_T,       0xFF},                 /* EVENT_CMGC */
    {   D_ATC_EVENT_CMMS,              D_ATC_EVENT_CMMS_R,      D_ATC_EVENT_CMMS_T,       0xFF},                 /* EVNET_CMMS */
    {   D_ATC_EVENT_NPOWERCLASS,    D_ATC_EVENT_NPOWERCLASS_R, D_ATC_EVENT_NPOWERCLASS_T, 0xFF},                 /* EVNET_NPOWERCLASS */
    {   D_ATC_EVENT_NPTWEDRXS,         D_ATC_EVENT_NPTWEDRXS_R, D_ATC_EVENT_NPTWEDRXS_T,  0xFF},                 /* EVNET_NPTWEDRXS */
    {   D_ATC_EVENT_NPIN,              0xFF,                    D_ATC_EVENT_NPIN_T,       0xFF},                 /* EVNET_NPIN */
    {   D_ATC_EVENT_NTSETID,           0xFF,                    D_ATC_EVENT_NTSETID_T,    0xFF},                 /* EVNET_NTSETID */
    {   D_ATC_EVENT_NCIDSTATUS,        0xFF,                    D_ATC_EVENT_NCIDSTATUS_T, D_ATC_EVENT_NCIDSTATUS_R}, /* EVNET_NCIDSTATUS */  
    {   D_ATC_EVENT_NGACTR,            D_ATC_EVENT_NGACTR_R,    D_ATC_EVENT_NGACTR_T,     0xFF},                 /* EVNET_NGACTR */  
    {   D_ATC_EVENT_NPOPB,             D_ATC_EVENT_NPOPB_R,     0xFF,                     0xFF},                 /* EVNET_NPOPB */  
    {   D_ATC_EVENT_NIPINFO,           D_ATC_EVENT_NIPINFO_R,   D_ATC_EVENT_NIPINFO_T,    0xFF},                 /* EVNET_NIPINFO */  
    {   D_ATC_EVENT_NQPODCP,           0xFF,                    D_ATC_EVENT_NQPODCP_T,    0xFF},                 /* EVNET_NQPODCP */
    {   D_ATC_EVENT_NSNPD,             0xFF,                    D_ATC_EVENT_NSNPD_T,      0xFF},                 /* EVNET_NSNPD */
    {   D_ATC_EVENT_NQPNPD,            0xFF,                    D_ATC_EVENT_NQPNPD_T,     0xFF},                 /* EVNET_NQPNPD */
    {   D_ATC_EVENT_CNEC,              D_ATC_EVENT_CNEC_R,      D_ATC_EVENT_CNEC_T,       0xFF},                 /* EVNET_CNEC */
    {   D_ATC_EVENT_NRNPDM,            D_ATC_EVENT_NRNPDM_R,    D_ATC_EVENT_NRNPDM_T,     0xFF},                 /* EVENT_NRNPDM */
    {   D_ATC_EVENT_NCPCDPR,           D_ATC_EVENT_NCPCDPR_R,   D_ATC_EVENT_NCPCDPR_T,    0xFF},                 /* EVENT_NCPCDPR */
    {   0xFF,                          0xFF,                    D_ATC_EVENT_CEID_T,       D_ATC_EVENT_CEID},     /* EVENT_CEID */
    {   D_ATC_EVENT_MNBIOTEVENT,    D_ATC_EVENT_MNBIOTEVENT_R,  D_ATC_EVENT_MNBIOTEVENT_T,0xFF},                 /* EVENT_MNBIOTEVENT */
    {   D_ATC_EVENT_CGPIAF,            D_ATC_EVENT_CGPIAF_R,    D_ATC_EVENT_CGPIAF_T,     0xFF},                 /* EVENT_CGPIAF */
#if SIMMAX_SUPPORT
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_CUPREFER}, /* EVENT_CUPREFER */
    {   D_ATC_EVENT_CUPREFERTH,     D_ATC_EVENT_CUPREFERTH_R,   D_ATC_EVENT_CUPREFERTH_T, 0xFF},                 /* EVENT_CUPREFERTH */
#endif
    {   0xFF,                       D_ATC_EVENT_NPLMNS_R,       D_ATC_EVENT_NPLMNS_T,     D_ATC_EVENT_NPLMNS},   /* EVENT_NPLMNS */
    {   D_ATC_EVENT_NLOCKF,            D_ATC_EVENT_NLOCKF_R,    D_ATC_EVENT_NLOCKF_T,     0xFF},                 /* EVENT_NLOCKF */
    {   0xFF,                          D_ATC_EVENT_ZICCID,      D_ATC_EVENT_ZICCID_T,     D_ATC_EVENT_ZICCID},   /* EVENT_ZICCID */
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_ZCELLINFO},/* EVENT_ZCELLINFO */
    {   D_ATC_EVENT_QCGDEFCONT,     D_ATC_EVENT_QCGDEFCONT_R,   D_ATC_EVENT_QCGDEFCONT_T, 0xFF},                 /* EVENT_QCGDEFCONT */
    {   D_ATC_EVENT_QBAND,             D_ATC_EVENT_QBAND_R,     D_ATC_EVENT_QBAND_T,      0xFF},                 /*EVENT_QBAND    41 */
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_QCCID},   /* EVENT_QCCID */
    {   D_ATC_EVENT_QENG,              0xFF,                    D_ATC_EVENT_QENG_T,       0xFF},                 /* EVENT_QENG */
    {   D_ATC_EVENT_QCFG,              0xFF,                    D_ATC_EVENT_QCFG_T,       0xFF},                 /* EVENT_QCFG */
    {   D_ATC_EVENT_NSIMWC,            0xFF,                    0xFF,                     0xFF},                 /* EVENT_NSIMWC */
    {   D_ATC_EVENT_NUICC,             0xFF,                    D_ATC_EVENT_NUICC_T,      0xFF},                 /* EVENT_NUICC */
    {   D_ATC_EVENT_PSTEST,            0xFF,                    0xFF,                     0xFF},                 /* EVENT_PSTEST */
    {   D_ATC_EVENT_QNIDD,             0xFF,                    0xFF,                     0xFF},                 /* EVENT_QNIDD */
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_XYCELLS},  /* EVENT_XYCELLS */
    {   D_ATC_EVENT_PRESETFREQ,        0xFF,                    0xFF,                     0xFF},                 /* EVENT_PRESETFREQ */  
    {   D_ATC_EVENT_NPBPLMNS,          D_ATC_EVENT_NPBPLMNS_R,  D_ATC_EVENT_NPBPLMNS_T,   0xFF},                  /* EVENT_NPBPLMNS */
    {   D_ATC_EVENT_NBACKOFF,          D_ATC_EVENT_NBACKOFF_R,  D_ATC_EVENT_NBACKOFF_T,   0xFF},                  /* EVENT_NBACKOFF */
    {   D_ATC_EVENT_SIMUUICC,          0xFF ,                   0xFF,                     0xFF},                  /* EVENT_SIMUUICC */
    {   D_ATC_EVENT_QLOCKF,            D_ATC_EVENT_QLOCKF_R,    D_ATC_EVENT_QLOCKF_T,     0xFF},                 /*EVENT_QLOCKF*/
    {   D_ATC_EVENT_QCSEARFCN,         0xFF,                    D_ATC_EVENT_QCSEARFCN_T,  D_ATC_EVENT_QCSEARFCN},/*EVENT_QCSEARFCN*/
    {   0xFF,                          0xFF,                    0xFF,                     D_ATC_EVENT_W},        /*EVENT_W*/
    {   D_ATC_EVENT_QICSGP,            0xFF,                    D_ATC_EVENT_QICSGP_T,     0xFF},                 /*EVENT_QICSGP*/
    {   D_ATC_EVENT_QSPCHSC,           D_ATC_EVENT_QSPCHSC_R,   D_ATC_EVENT_QSPCHSC_T,    0xFF},                 /*EVENT_QSPCHSC*/
};

/* FAC */
const ST_ATC_FAC_TABLE ATC_Fac_Table[D_ATC_FAC_NUM] =
{
    /* Facility lock +CLCK  27.007 7.4 */
    //{   D_ATC_PARAM_FAC_CS,              "CS"                  },
    //{   D_ATC_PARAM_FAC_PS,              "PS"                  },
    //{   D_ATC_PARAM_FAC_PF,              "PF"                  },
      {   D_ATC_PARAM_FAC_SC,              "SC"                  },
    //{   D_ATC_PARAM_FAC_AO,              "AO"                  },
    //{   D_ATC_PARAM_FAC_OI,              "OI"                  },
    //{   D_ATC_PARAM_FAC_OX,              "OX"                  },
    //{   D_ATC_PARAM_FAC_AI,              "AI"                  },
    //{   D_ATC_PARAM_FAC_IR,              "IR"                  },
    //{   D_ATC_PARAM_FAC_NT,              "NT"                  },
    //{   D_ATC_PARAM_FAC_NM,              "NM"                  },
    //{   D_ATC_PARAM_FAC_NS,              "NS"                  },
    //{   D_ATC_PARAM_FAC_NA,              "NA"                  },
    //{   D_ATC_PARAM_FAC_AB,              "AB"                  },
    //{   D_ATC_PARAM_FAC_AG,              "AG"                  },
    //{   D_ATC_PARAM_FAC_AC,              "AC"                  },
    //{   D_ATC_PARAM_FAC_FD,              "FD"                  },
    //{   D_ATC_PARAM_FAC_PN,              "PN"                  },
    //{   D_ATC_PARAM_FAC_PU,              "PU"                  },
    //{   D_ATC_PARAM_FAC_PP,              "PP"                  },
    //{   D_ATC_PARAM_FAC_PC,              "PC"                  },
    /* Change password +CPWD  27.007 7.5 */
    //{   D_ATC_PARAM_FAC_P2,              "P2"                  }
};

const ST_ATC_STR_TABLE ATC_NUESTATS_Table[ATC_NUESTATS_MAX] = 
{
    {   ATC_NUESTATS_TYPE_RADIO,          "RADIO"                },
    {   ATC_NUESTATS_TYPE_CELL,           "CELL"                 },
    {   ATC_NUESTATS_TYPE_BLER,           "BLER"                 },
    {   ATC_NUESTATS_TYPE_THP,            "THP"                  },
    {   ATC_NUESTATS_TYPE_APPSMEM,        "APPSMEM"              },
    {   ATC_NUESTATS_SBAND,               "CURRENTBAND"         },
    {   ATC_NUESTATS_TYPE_ALL,            "ALL"                  }
};
const ST_ATC_STR_TABLE ATC_PdpType_Table[9] = 
{
    {   D_PDP_TYPE_IPV4,          "IP"             },
    {   D_PDP_TYPE_IPV4,          "ip"             },
    {   D_PDP_TYPE_IPV6,          "IPV6"           },
    {   D_PDP_TYPE_IPV6,          "ipv6"           },
    {   D_PDP_TYPE_IPV4V6,        "IPV4V6"         },
    {   D_PDP_TYPE_IPV4V6,        "ipv4v6"         },
    {   D_PDP_TYPE_NonIP,         "Non-IP"         },
    {   D_PDP_TYPE_NonIP,         "non-ip"         },
    {   D_PDP_TYPE_NonIP,         "NONIP"          }
};

const unsigned long ATC_AP_PsRegEventIdMapTable[D_ATC_USER_REG_EVENT_TBL_SIZE][2] =
{
    { D_XY_PS_REG_EVENT_SIMST,         D_ATC_AP_SIMST_IND         },
    { D_XY_PS_REG_EVENT_XYIPDNS,       D_ATC_AP_XYIPDNS_IND       },
    { D_XY_PS_REG_EVENT_CRTDCP,        D_ATC_AP_CRTDCP_IND        },
    { D_XY_PS_REG_EVENT_CGAPNRC,       D_ATC_AP_CGAPNRC_IND       },
    { D_XY_PS_REG_EVENT_CGEV,          D_ATC_AP_CGEV_IND          },
    { D_XY_PS_REG_EVENT_CEREG,         D_ATC_AP_CEREG_IND         },
    { D_XY_PS_REG_EVENT_CSCON,         D_ATC_AP_CSCON_IND         },
    { D_XY_PS_REG_EVENT_NPTWEDRXP,     D_ATC_AP_NPTWEDRXP_IND     },
    
    { D_XY_PS_REG_EVENT_CEDRXP,        D_ATC_AP_CEDRXP_IND        },
    { D_XY_PS_REG_EVENT_CCIOTOPTI,     D_ATC_AP_CCIOTOPTI_IND     },
    { D_XY_PS_REG_EVENT_PINSTATUS,     D_ATC_AP_PINSTATUS_IND     },
    { D_XY_PS_REG_EVENT_IPSN,          D_ATC_AP_IPSN_IND          },
    { D_XY_PS_REG_EVENT_LOCALTIMEINFO, D_ATC_AP_LOCALTIMEINFO_IND },
    { D_XY_PS_REG_EVENT_PDNIPADDR,     D_ATC_AP_IPADDR_IND        },
    { D_XY_PS_REG_EVENT_NGACTR,        D_ATC_AP_NGACTR_IND        },
    { D_XY_PS_REG_EVENT_CMT,           D_ATC_AP_CMT_IND           },
    { D_XY_PS_REG_EVENT_CELLSRCH,      D_ATC_AP_CELL_SRCH_IND     },
    
    { D_XY_PS_REG_EVENT_CMOLRE,        D_ATC_AP_CMOLRE_IND        },
    { D_XY_PS_REG_EVENT_CMOLRG,        D_ATC_AP_CMOLRG_IND        },
    { D_XY_PS_REG_EVENT_L2_THP,        D_ATC_AP_L2THP_IND         },
    { D_XY_PS_REG_EVENT_MALLOC_ADDR,   D_ATC_AP_MALLOCADDR_IND    },
    { D_XY_PS_REG_EVENT_NoCarrier,     D_ATC_NO_CARRIER_IND       },
    { D_XY_PS_REG_EVENT_CESQ_Ind ,     D_ATC_AP_EVENT_CESQ_IND    },
    { D_XY_PS_REG_EVENT_PSINFO ,       D_ATC_AP_PSINFO_IND        },
    { D_XY_PS_REG_EVENT_NRNPDM,        D_ATC_AP_NRNPDM_IND        },
    { D_XY_PS_REG_EVENT_OK_ERROR,      D_ATC_AP_AT_CMD_RST        },
    { D_XY_PS_REG_EVENT_MNBIOTEVENT,   D_ATC_AP_MNBIOTEVENT_IND   },
};

