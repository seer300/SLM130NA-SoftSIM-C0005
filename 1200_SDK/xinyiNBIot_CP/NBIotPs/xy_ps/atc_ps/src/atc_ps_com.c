#include "atc_ps.h"
#include <stdarg.h>

extern int ps_get_ip6addr(unsigned int *ip6addr);
/* add for TimeZone Conversion */
const unsigned char ATC_MonthDayTbl[2][12] = 
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},                                   /* common year */
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}                                    /* leap year */
};

const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE PCmeErrorTextTbl[] = 
{
    /* usErrCode  */       /* pCmeErrorText  */
    {   1,                (unsigned char *)"no connection to phone"      },
    {   2,                (unsigned char *)"phone-adaptor link reserved" },
    {   3,                (unsigned char *)"operation not allowed"       },
    {   4,                (unsigned char *)"operation not supported"     },
    {   5,                (unsigned char *)"PH-SIM PIN required"         },
    {   6,                (unsigned char *)"PH-FSIM PIN required"        },
    {   7,                (unsigned char *)"PH-FSIM PUK required"        },
    {   10,               (unsigned char *)"SIM not inserted"            },
    {   11,               (unsigned char *)"SIM PIN required"            },
    {   12,               (unsigned char *)"SIM PUK required"            },
    {   13,               (unsigned char *)"SIM failure"                 },
    {   14,               (unsigned char *)"SIM busy"                    },
    {   15,               (unsigned char *)"SIM wrong"                   },
    {   16,               (unsigned char *)"incorrect password"          },
    {   17,               (unsigned char *)"SIM PIN2 required"           },
    {   18,               (unsigned char *)"SIM PUK2 required"           },
    {   20,               (unsigned char *)"memory full"                 },
    {   21,               (unsigned char *)"invalid index"               },
    {   22,               (unsigned char *)"not found"                   },
    {   23,               (unsigned char *)"memory failure"              },
    {   24,               (unsigned char *)"text string too long"        },
    {   25,               (unsigned char *)"invalid characters in text string"                 },
    {   26,               (unsigned char *)"dial string too long"                              },
    {   27,               (unsigned char *)"invalid characters in dial string"                 },
    {   30,               (unsigned char *)"no network service"                                },
    {   31,               (unsigned char *)"network timeout"                                   },
    {   32,               (unsigned char *)"network not allowed - emergency calls only"        },
    {   40,               (unsigned char *)"network personalisation PIN required"              },
    {   41,               (unsigned char *)"network personalisation PUK required"              },
    {   42,               (unsigned char *)"network subset personalisation PIN required"       },
    {   43,               (unsigned char *)"network subset personalisation PUK required"       },
    {   44,               (unsigned char *)"service provider personalisation PIN required"     },
    {   45,               (unsigned char *)"service provider personalisation PUK required"     },
    {   46,               (unsigned char *)"corporate personalisation PIN required"            },
    {   47,               (unsigned char *)"corporate personalisation PUK required"            },
    {   48,               (unsigned char *)"hidden key required"                               },
    {   49,               (unsigned char *)"EAP method not supported"                          },
    {   50,               (unsigned char *)"Incorrect parameters"                              },
    {   51,               (unsigned char *)"command implemented but currently disabled"        },
    {   52,               (unsigned char *)"command aborted by user"                           },
    {   53,               (unsigned char *)"not attached to network due to MT functionality restrictions"             },
    {   54,               (unsigned char *)"modem not allowed - MT restricted to emergency calls only"                },
    {   55,               (unsigned char *)"operation not allowed because of MT functionality restrictions"           },
    {   56,               (unsigned char *)"fixed dial number only allowed - called number is not a fixed dial number"},
    {   57,               (unsigned char *)"temporarily out of service due to other MT usage"                         },
    {   58,               (unsigned char *)"language/alphabet not supported"                   },
    {   59,               (unsigned char *)"unexpected data value"                             },
    {   60,               (unsigned char *)"system failure"                                    },
    {   61,               (unsigned char *)"data missing"                                      },
    {   62,               (unsigned char *)"call barred"                                       },
    {   63,               (unsigned char *)"message waiting indication subscription failure"   },
    {   100,              (unsigned char *)"unknown"                                           },
    /* GPRS and EPS-related errors start */
    {   103,              (unsigned char *)"Illegal MS (#3)"                                      },
    {   106,              (unsigned char *)"Illegal ME (#6)"                                      },
    {   107,              (unsigned char *)"GPRS services not allowed (#7)"                       },
    {   108,              (unsigned char *)"GPRS services and non-GPRS services not allowed (#8)" },
    {   111,              (unsigned char *)"PLMN not allowed (#11)"                               },
    {   112,              (unsigned char *)"Location area not allowed (#12)"                      },
    {   113,              (unsigned char *)"Roaming not allowed in this location area (#13)"      },
    {   114,              (unsigned char *)"GPRS services not allowed in this PLMN (#14)"         },
    {   115,              (unsigned char *)"No Suitable Cells In Location Area (#15)"             },
    {   122,              (unsigned char *)"Congestion (#22))"                                    },
    {   125,              (unsigned char *)"Not authorized for this CSG (#25)",                   },
    {   171,              (unsigned char *)"Last PDN disconnection not allowed (#49) "            },
    {   172,              (unsigned char *)"Semantically incorrect message (#95) "                },
    {   173,              (unsigned char *)"Mandatory information element error (#96)"            },
    {   174,              (unsigned char *)"Information element non-existent or not implemented (#97)" },
    {   175,              (unsigned char *)"Conditional IE error (#99)"                           },
    {   176,              (unsigned char *)"Protocol error, unspecified (#111)"                   },
    /* GPRS and EPS-related errors end */
    /* Errors related to a failure to activate a context start */
    {   177,              (unsigned char *)"Operator Determined Barring (#8)"                        },
    {   126,              (unsigned char *)"insufficient resources (#26)"                            },
    {   127,              (unsigned char *)"missing or unknown APN (#27)"                            },
    {   128,              (unsigned char *)"unknown PDP address or PDP type (#28)"                   },
    {   129,              (unsigned char *)"user authentication failed (#29)"                        },
    {   130,              (unsigned char *)"activation rejected by GGSN, Serving GW or PDN GW (#30)" },
    {   131,              (unsigned char *)"activation rejected, unspecified (#31)"                  },
    {   132,              (unsigned char *)"service option not supported (#32)"                      },
    {   133,              (unsigned char *)"requested service option not subscribed (#33)"           },
    {   134,              (unsigned char *)"service option temporarily out of order (#34)"           },
    {   140,              (unsigned char *)"feature not supported (#40)"                             },
    {   141,              (unsigned char *)"semantic error in the TFT operation (#41)"               },
    {   142,              (unsigned char *)"syntactical error in the TFT operation (#42)"            },
    {   143,              (unsigned char *)"unknown PDP context (#43)"                               },
    {   144,              (unsigned char *)"semantic errors in packet filter(s) (#44)"               },
    {   145,              (unsigned char *)"syntactical errors in packet filter(s) (#45)"            },
    {   146,              (unsigned char *)"PDP context without TFT already activated (#46)"         },
    {   149,              (unsigned char *)"PDP authentication failure"                              },
    {   178,              (unsigned char *)"maximum number of PDP contexts reached (#65)"            },
    {   179,              (unsigned char *)"requested APN not supported in current RAT and PLMN combination (#66)" },
    {   180,              (unsigned char *)"request rejected, Bearer Control Mode violation (#48)"   },
    {   181,              (unsigned char *)"unsupported QCI value (#83)"                             },
    /* Errors related to a failure to activate a context end */
    {   171,              (unsigned char *)"Last PDN disconnection not allowed (#49)"                },
};

const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE PEmmCauseTextTbl[D_ATC_EMM_CAUSE_TBL_SIZE] = 
{
    {   2,                (unsigned char *)"IMSI unknown in HSS"                                     },
    {   3,                (unsigned char *)"Illegal UE"                                              },
    {   5,                (unsigned char *)"IMEI not accepted"                                       },
    {   6,                (unsigned char *)"Illegal ME"                                              },
    {   7,                (unsigned char *)"EPS services not allowed"                                },
    {   8,                (unsigned char *)"EPS services and non,EPS services not allowed"           },
    {   9,                (unsigned char *)"UE identity cannot be derived by the network"            },
    {   10,               (unsigned char *)"Implicitly detached"                                     },
    {   11,               (unsigned char *)"PLMN not allowed"                                        },
    {   12,               (unsigned char *)"Tracking area not allowed"                               },
    {   13,               (unsigned char *)"Roaming not allowed in this tracking area"               },
    {   14,               (unsigned char *)"EPS services not allowed in this PLMN"                   },
    {   15,               (unsigned char *)"No suitable cells in tracking area"                      },
    {   16,               (unsigned char *)"MSC temporarily not reachable"                           },
    {   17,               (unsigned char *)"Network failure"                                         },
    {   18,               (unsigned char *)"CS domain not available"                                 },
    {   19,               (unsigned char *)"ESM failure"                                             },
    {   20,               (unsigned char *)"MAC failure"                                             },
    {   21,               (unsigned char *)"Synch failure"                                           },
    {   22,               (unsigned char *)"Congestion"                                              },
    {   23,               (unsigned char *)"UE security capabilities mismatch"                       },
    {   24,               (unsigned char *)"Security mode rejected, unspecified"                     },
    {   25,               (unsigned char *)"Not authorized for this CSG"                             },
    {   26,               (unsigned char *)"Non,EPS authentication unacceptable"                     },
    {   35,               (unsigned char *)"Requested service option not authorized in this PLMN"    },
    {   39,               (unsigned char *)"CS service temporarily not available"                    },
    {   40,               (unsigned char *)"No EPS bearer context activated"                         },
    {   42,               (unsigned char *)"Severe network failure"                                  },
};

const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE PEsmCauseTextTbl[D_ATC_ESM_CAUSE_TBL_SIZE] = 
{
    {   8,                 (unsigned char *)"Operator Determined Barring"                            },
    {   26,                (unsigned char *)"Insufficient resources"                                 },
    {   27,                (unsigned char *)"Missing or unknown APN"                                 },
    {   28,                (unsigned char *)"Unknown PDN type"                                       },
    {   29,                (unsigned char *)"User authentication failed"                             },
    {   30,                (unsigned char *)"Request rejected by Serving GW or PDN GW"               },
    {   31,                (unsigned char *)"Request rejected, unspecified"                          },
    {   32,                (unsigned char *)"Service option not supported"                           },
    {   33,                (unsigned char *)"Requested service option not subscribed"                },
    {   34,                (unsigned char *)"Service option temporarily out of order"                },
    {   35,                (unsigned char *)"PTI already in use"                                     },
    {   36,                (unsigned char *)"Regular deactivation"                                   },
    {   37,                (unsigned char *)"EPS QoS not acceptedETSI"                               },
    {   38,                (unsigned char *)"Network failure"                                        },
    {   39,                (unsigned char *)"Reactivation requested"                                 },
    {   41,                (unsigned char *)"Semantic error in the TFT operation."                   },
    {   42,                (unsigned char *)"Syntactical error in the TFT operation."                },
    {   43,                (unsigned char *)"Invalid EPS bearer identity"                            },
    {   44,                (unsigned char *)"Semantic errors in packet filter(s)"                    },
    {   45,                (unsigned char *)"Syntactical error in packet filter(s)"                  },
    {   47,                (unsigned char *)"PTI mismatch"                                           },
    {   49,                (unsigned char *)"Last PDN disconnection not allowed"                     },
    {   50,                (unsigned char *)"PDN type IPv4 only allowed"                             },
    {   51,                (unsigned char *)"PDN type IPv6 only allowed"                             },
    {   52,                (unsigned char *)"single address bearers only allowed"                    },
    {   53,                (unsigned char *)"ESM information not receivedETSI"                       },
    {   54,                (unsigned char *)"PDN connection does not exist"                          },
    {   55,                (unsigned char *)"Multiple PDN connections for a given APN not allowed"   },
    {   56,                (unsigned char *)"Collision with network initiated request"               },
    {   57,                (unsigned char *)"PDN type IPv4v6 only allowed"                           },
    {   58,                (unsigned char *)"PDN type non IP only allowed"                           },
    {   59,                (unsigned char *)"Unsupported QCI value"                                  },
    {   60,                (unsigned char *)"Bearer handling not supported"                          },
    {   65,                (unsigned char *)"Maximum number of EPS bearers reached"                  },
    {   66,                (unsigned char *)"Requested APN not supported in current RAT and PLMN combination"},
    {   81,                (unsigned char *)"Invalid PTI value"                                      },
    {   112,               (unsigned char *)"APN restriction value incompatible with active EPS bearer context."},
    {   113,               (unsigned char *)"Multiple accesses to a PDN connection not allowed"      }, 
    {   200,               (unsigned char *)"User detached by command +COPS=2"                       }, //D_ATC_USER_DETACH_BY_CMD_COPS
    {   201,               (unsigned char *)"User detached by command +CGATT=0"                      }, //D_ATC_USER_DETACH_BY_CMD_CGATT
};

const unsigned char ATC_PinCodeTbl[16][14] =
{
    "READY",                                                                            /* 0 */
    "SIM PIN",                                                                          /* 1 */
    "SIM PUK",                                                                          /* 2 */
    "PH-SIM PIN",                                                                       /* 3 */
    "PH-FSIM PIN",                                                                      /* 4 */
    "PH-FSIM PUK",                                                                      /* 5 */
    "SIM PIN2",                                                                         /* 6 */
    "SIM PUK2",                                                                         /* 7 */
    "PH-NET PIN",                                                                       /* 8 */
    "PH-NET PUK",                                                                       /* 9 */
    "PH-NETSUB PIN",                                                                    /* 10 */
    "PH-NETSUB PUK",                                                                    /* 11 */
    "PH-SP PIN",                                                                        /* 12 */
    "PH-SP PUK",                                                                        /* 13 */
    "PH-CORP PIN",                                                                      /* 14 */
    "PH-CORP PUK"                                                                       /* 15 */
};

static void ATC_DelCascadeInfo()
{
    if(NULL != g_AtcApInfo.atCascateInfo.pCasaadeAtBuff)
    {
        AtcAp_Free(g_AtcApInfo.atCascateInfo.pCasaadeAtBuff);
    }

    AtcAp_MemSet(&g_AtcApInfo.atCascateInfo, 0, sizeof(ST_ATC_CASCADE_AT_INFO));
}

void ATC_SendApDataReq(unsigned char ucExternalFlg, unsigned long ulAppSemaId, unsigned short usDataLen, unsigned char* pucData, unsigned char ucTaskSource)
{
    ATC_AP_MSG_DATA_REQ_STRU   *pAtcApDataReq = NULL;
    unsigned long              ulLength;

    if (usDataLen < 4)
    {
        ulLength = sizeof(ATC_AP_MSG_DATA_REQ_STRU);
    }
    else
    {
        ulLength = sizeof(ATC_AP_MSG_DATA_REQ_STRU) + usDataLen - 4 + 1;
    }
    
    pAtcApDataReq = (ATC_AP_MSG_DATA_REQ_STRU *)AtcAp_Malloc(ulLength);

    pAtcApDataReq->ucExternalFlg = ucExternalFlg;
    pAtcApDataReq->ulSemaId = ulAppSemaId;
    pAtcApDataReq->usMsgLen = usDataLen;
    AtcAp_MemCpy(pAtcApDataReq->aucMsgData, pucData, usDataLen);
    pAtcApDataReq->ucAplNum = 0;
    pAtcApDataReq->ucTaskSource = ucTaskSource;

    pAtcApDataReq->MsgHead.ulMsgName = D_ATC_AP_DATA_REQ;
    AtcAp_SendMsg2AtcAp((void*)pAtcApDataReq, &g_AtcApInfo.msgInfo);
}

void AtcAp_SendOkRsp()
{
    if(NULL != g_AtcApInfo.pCurrEvent)
    {
        AtcAp_FreeEventBuffer((unsigned char*)g_AtcApInfo.pCurrEvent);
        g_AtcApInfo.pCurrEvent = NULL;
    }
    g_AtcApInfo.usCurrEvent = 0xFFFF;

    if(g_AtcApInfo.atCascateInfo.ucCascadeAtCnt > 1)
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendOkRsp]CascadeAtCnt = %d",g_AtcApInfo.atCascateInfo.ucCascadeAtCnt);
        g_AtcApInfo.atCascateInfo.ucCascadeAtCnt--;
        AtcAp_CascadeAtProc_NextAt();
        return;
    }
    else if( g_AtcApInfo.atCascateInfo.ucCascadeAtCnt == 0)
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendOkRsp]RedundancyRpt OK");
        return;
    }

    if(g_AtcApInfo.ucTempSeqNum != 0)
    {
        AtcAp_AppInterfaceInfo_CmdRstProc(g_AtcApInfo.ucTempSeqNum, D_APP_INTERFACE_RESULT_SUCC);
    }
    g_AtcApInfo.ucTempSeqNum = 0;

    ATC_DelCascadeInfo();
    if(g_AtcApInfo.ucUserAtFlg == ATC_AP_FALSE)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\nOK\r\n");
        g_AtcApInfo.usCurrRspEvent = D_ATC_AP_AT_CMD_RST;
        AtcAp_SendDataInd();
    }

    g_AtcApInfo.ucWaitOKOrErrorFlg = ATC_AP_FALSE;
    AtcAp_AtcDataReqListProc();
    return;
}

void AtcAp_SendCmeeErr(unsigned short usErrCode)
{
    unsigned short   i;
    
    if(NULL != g_AtcApInfo.pCurrEvent)
    {
        AtcAp_FreeEventBuffer((unsigned char*)g_AtcApInfo.pCurrEvent);
        g_AtcApInfo.pCurrEvent = NULL;
    }
    g_AtcApInfo.usCurrEvent = 0xFFFF;
    
    if( g_AtcApInfo.atCascateInfo.ucCascadeAtCnt > 1)
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendCmeeErr]CascadeAtCnt = %d",g_AtcApInfo.atCascateInfo.ucCascadeAtCnt);
    }
    else if( g_AtcApInfo.atCascateInfo.ucCascadeAtCnt == 0)
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendCmeeErr]RedundancyRpt CME ERROR");
        return;
    }

    if(g_AtcApInfo.ucTempSeqNum != 0)
    {
        AtcAp_AppInterfaceInfo_CmdRstProc(g_AtcApInfo.ucTempSeqNum, D_APP_INTERFACE_RESULT_FAIL);
    }
    g_AtcApInfo.ucTempSeqNum = 0;

    ATC_DelCascadeInfo();
    if(g_AtcApInfo.ucUserAtFlg == ATC_AP_FALSE)
    {
        if (D_ATC_ERRMOD_DIG_CMEE == get_cmee_mode())
        {
            g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
                (const unsigned char *)"\r\n+CME ERROR:%d\r\n", usErrCode);
        }
        else if (D_ATC_ERRMOD_STR_CMEE == get_cmee_mode())
        {
            for (i = 0; i < D_ATC_CME_ERROR_TBL_SIZE; i++)
            {
                if (usErrCode == PCmeErrorTextTbl[i].usErrCode)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
                        (const unsigned char *)"\r\n+CME ERROR:%c%s%c\r\n",
                        D_ATC_N_QUOTATION, PCmeErrorTextTbl[i].pCmeErrorText, D_ATC_N_QUOTATION);
                    break;
                }
            }
            
            if (i ==  D_ATC_CME_ERROR_TBL_SIZE)
            {
                 g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
                    (const unsigned char *)"\r\n+CME ERROR:%c%s%c\r\n",
                    D_ATC_N_QUOTATION, "unknown", D_ATC_N_QUOTATION);
            }
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
                    (const unsigned char *)"\r\nERROR\r\n");
        }
        g_AtcApInfo.usCurrRspEvent = D_ATC_AP_AT_CMD_RST;
        AtcAp_SendDataInd();
    }

    g_AtcApInfo.ucWaitOKOrErrorFlg = ATC_AP_FALSE;
    AtcAp_AtcDataReqListProc();
}

void AtcAp_SendErrorRsp()
{
    AtcAp_SendCmeeErr(D_ATC_AP_CME_UNKNOWN);
}

/*******************************************************************************
  MODULE    : ATC_SendCmsErr
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
void AtcAp_SendCmsErr(unsigned short usErrCode)
{
    if(NULL != g_AtcApInfo.pCurrEvent)
    {
        AtcAp_FreeEventBuffer((unsigned char*)g_AtcApInfo.pCurrEvent);
        g_AtcApInfo.pCurrEvent = NULL;
    }
    g_AtcApInfo.usCurrEvent = 0xFFFF;

    if( g_AtcApInfo.atCascateInfo.ucCascadeAtCnt > 1)
    {
        //AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendCmsErr]CascadeAtCnt = %d",g_AtcApInfo.atCascateInfo.ucCascadeAtCnt);
    }
    else if( g_AtcApInfo.atCascateInfo.ucCascadeAtCnt == 0)
    {
        //AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_SendCmsErr]RedundancyRpt ERROR");
        return;
    }

    if(g_AtcApInfo.ucTempSeqNum != 0)
    {
        AtcAp_AppInterfaceInfo_CmdRstProc(g_AtcApInfo.ucTempSeqNum, D_APP_INTERFACE_RESULT_FAIL);
    }
    g_AtcApInfo.ucTempSeqNum = 0;

    ATC_DelCascadeInfo();
    if(g_AtcApInfo.ucUserAtFlg == ATC_AP_FALSE)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+CMS ERROR:%d\r\n", usErrCode);
        g_AtcApInfo.usCurrRspEvent = D_ATC_AP_AT_CMD_RST;
        AtcAp_SendDataInd();
    }

    g_AtcApInfo.ucWaitOKOrErrorFlg = ATC_AP_FALSE;
    AtcAp_AtcDataReqListProc();
    return;
}

extern void  SendAtInd2User(char *pAt, unsigned int ulAtLen);

void AtcAp_AtCacheLogHandle(unsigned char* pbuffer ,unsigned short usAtLogLen)
{
    unsigned char*  pAllBuffer;
    unsigned char*  pPutPoint;
    ST_ATCNF_INFO*  pTemBuffer;
    unsigned char   i = 0;
    
    AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_AtCacheLogHandle] %d   %s",g_AtcApInfo.stAtLogCacheList.ucAtLogCount, (const char *)pbuffer);

    if(g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList == NULL)
    {
        g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList = (ST_ATCNF_INFO*)AtcAp_Malloc(sizeof(ST_ATCNF_INFO) * AT_LOG_CACHE_MAX_NUM);
        g_AtcApInfo.stAtLogCacheList.ucBufferNum = AT_LOG_CACHE_MAX_NUM;
    }
    g_AtcApInfo.stAtLogCacheList.ucAtLogCount++;
    if(g_AtcApInfo.stAtLogCacheList.ucAtLogCount > g_AtcApInfo.stAtLogCacheList.ucBufferNum)
    {
        g_AtcApInfo.stAtLogCacheList.ucBufferNum += AT_LOG_CACHE_MAX_NUM;
        pTemBuffer = (ST_ATCNF_INFO*)AtcAp_Malloc(sizeof(ST_ATCNF_INFO) * g_AtcApInfo.stAtLogCacheList.ucBufferNum);
        AtcAp_MemCpy(pTemBuffer, g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList ,(sizeof(ST_ATCNF_INFO) * g_AtcApInfo.stAtLogCacheList.ucAtLogCount));
        AtcAp_Free(g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList);
        g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList = pTemBuffer;
    }
    
    g_AtcApInfo.stAtLogCacheList.ucAtTotalLogsize += usAtLogLen;
    g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[g_AtcApInfo.stAtLogCacheList.ucAtLogCount - 1].usCnfLen = usAtLogLen;
    g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[g_AtcApInfo.stAtLogCacheList.ucAtLogCount - 1].pAtCnfBuf = AtcAp_Malloc(usAtLogLen + 1);
    AtcAp_MemCpy(g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[g_AtcApInfo.stAtLogCacheList.ucAtLogCount - 1].pAtCnfBuf, pbuffer, usAtLogLen);

    //AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_AtCacheLogHandle] %s", (const char *)g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[g_AtcApInfo.stAtLogCacheList.ucAtLogCount - 1].pAtCnfBuf);

    if(g_AtcApInfo.usCurrRspEvent != D_ATC_AP_AT_CMD_RST)
    {
        return;
    }

    pAllBuffer = AtcAp_Malloc(g_AtcApInfo.stAtLogCacheList.ucAtTotalLogsize + 1);
    pPutPoint = pAllBuffer;
    for(i = 0; i < g_AtcApInfo.stAtLogCacheList.ucAtLogCount; i++)
    {
        AtcAp_MemCpy(pPutPoint ,
            g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[i].pAtCnfBuf,
            g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[i].usCnfLen);
        pPutPoint += g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[i].usCnfLen;

        AtcAp_Free(g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList[i].pAtCnfBuf);
    }
    AtcAp_Free(g_AtcApInfo.stAtLogCacheList.pucAtLogCacheList);
    AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_AtCacheLogHandle] %s",(const char *)pAllBuffer);

#if OLD_ATC
    AT_RCV_FROM_PS(pAllBuffer, g_AtcApInfo.stAtLogCacheList.ucAtTotalLogsize);
#else
    SendAtInd2User((char*)pAllBuffer, g_AtcApInfo.stAtLogCacheList.ucAtTotalLogsize);
#endif

    AtcAp_Free(pAllBuffer);
    AtcAp_MemSet(&g_AtcApInfo.stAtLogCacheList, 0, sizeof(ST_ATCNF_CACHE_INFO));

}

void AtcAp_SendDataInd(void)
{
    if(g_AtcApInfo.ucAtHeaderSpaceFlg == 255)
    {
        g_AtcApInfo.ucAtHeaderSpaceFlg = g_factory_nv->tNvData.tNasNv.ucAtHeaderSpaceFlg;
    }

    if(g_AtcApInfo.ucAtHeaderSpaceFlg == 1)
    {
        ATC_CmdHeaderWithSpaceProc((char**)&g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, &g_AtcApInfo.stAtRspInfo.usRspLen, D_ATC_RSP_MAX_BUF_SIZE - 1);
    }

    if(g_AtcApInfo.stAtRspInfo.usRspLen != 0)
    {
        if((g_factory_nv->tNvData.tNasNv.ucATCacheFlg == ATC_AP_TRUE)
            && (g_AtcApInfo.usCurrRspEvent <= D_ATC_AP_AT_CMD_RST)
            && (g_AtcApInfo.usCurrRspEvent != D_ATC_EVENT_NPIN))
        {
            AtcAp_AtCacheLogHandle(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,g_AtcApInfo.stAtRspInfo.usRspLen);

            g_AtcApInfo.stAtRspInfo.usRspLen = 0;
            AtcAp_MemSet(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 0 ,D_ATC_RSP_MAX_BUF_SIZE);
            g_AtcApInfo.usCurrRspEvent = 0;
            return;
        }

#if OLD_ATC
        AT_RCV_FROM_PS(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,g_AtcApInfo.stAtRspInfo.usRspLen);
#else
        SendAtInd2User(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,g_AtcApInfo.stAtRspInfo.usRspLen);
#endif
    }
    g_AtcApInfo.stAtRspInfo.usRspLen = 0;
    AtcAp_MemSet(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 0 ,D_ATC_RSP_MAX_BUF_SIZE);
    g_AtcApInfo.usCurrRspEvent = 0;
    return;
}

/*******************************************************************************
  MODULE    : ATC_SendLongDataInd
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  GCM   2018.11.02   create
*******************************************************************************/
void AtcAp_SendLongDataInd(unsigned char **pBuffer, unsigned short usMaxLen)
{
    if(g_AtcApInfo.ucAtHeaderSpaceFlg == 255)
    {
        g_AtcApInfo.ucAtHeaderSpaceFlg = g_factory_nv->tNvData.tNasNv.ucAtHeaderSpaceFlg;
    }

    if(g_AtcApInfo.ucAtHeaderSpaceFlg == 1)
    {
        ATC_CmdHeaderWithSpaceProc((char**)pBuffer, &g_AtcApInfo.stAtRspInfo.usRspLen, usMaxLen);
    }

    if(g_AtcApInfo.stAtRspInfo.usRspLen != 0)
    {
        if((g_factory_nv->tNvData.tNasNv.ucATCacheFlg == ATC_AP_TRUE)
            && (g_AtcApInfo.usCurrRspEvent <= D_ATC_AP_AT_CMD_RST)
            && (g_AtcApInfo.usCurrRspEvent != D_ATC_EVENT_NPIN))
        {
            AtcAp_AtCacheLogHandle(*pBuffer,g_AtcApInfo.stAtRspInfo.usRspLen);

            g_AtcApInfo.stAtRspInfo.usRspLen = 0;
            g_AtcApInfo.usCurrRspEvent = 0;
            return;
        }
#ifdef OLD_ATC
        AT_RCV_FROM_PS(*pBuffer, g_AtcApInfo.stAtRspInfo.usRspLen);
#else
        SendAtInd2User(*pBuffer, g_AtcApInfo.stAtRspInfo.usRspLen);
#endif
        g_AtcApInfo.stAtRspInfo.usRspLen = 0;
    }
    g_AtcApInfo.usCurrRspEvent = 0;
    return;
}

/*******************************************************************************
  MODULE    : ATC_FreeEventBuffer
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  GCM   2018.10.15   create
*******************************************************************************/
void AtcAp_FreeEventBuffer(unsigned char* pCmdEvent)
{
    UN_ATC_CMD_EVENT* pCmdComEvent;

    if(NULL == pCmdEvent)
    {
        return;
    }
    
    pCmdComEvent = (UN_ATC_CMD_EVENT*)pCmdEvent;
    switch(pCmdComEvent->stCgdcontParam.usEvent)
    {
        case D_ATC_EVENT_CGDCONT:
            if(NULL != pCmdComEvent->stCgdcontParam.pucApnValue)
            {
                AtcAp_Free(pCmdComEvent->stCgdcontParam.pucApnValue);
            }
            break;
        case D_ATC_EVENT_CSODCP:
            if(NULL != pCmdComEvent->stCsodcpParam.pucCpdata)
            {
                AtcAp_Free(pCmdComEvent->stCsodcpParam.pucCpdata);
            }
            break;
        case D_ATC_EVENT_CSIM:
            if(NULL != pCmdComEvent->stCsimParam.pucCommand)
            {
                AtcAp_Free(pCmdComEvent->stCsimParam.pucCommand);
            }
            break;
        case D_ATC_EVENT_CGLA:
            if(NULL != pCmdComEvent->stCglaParam.pucCommand)
            {
                AtcAp_Free(pCmdComEvent->stCglaParam.pucCommand);
            }
            break;
        case D_ATC_EVENT_CRSM:
            if(NULL != pCmdComEvent->stCrsmParam.pucData)
            {
                AtcAp_Free(pCmdComEvent->stCrsmParam.pucData);
            }
            break;
        case D_ATC_EVENT_NSNPD:
            if(NULL != pCmdComEvent->stNsnpdParam.pucNonIpData)
            {
                AtcAp_Free(pCmdComEvent->stNsnpdParam.pucNonIpData);
            }
            break;
        case D_ATC_EVENT_PSTEST:
            if(NULL != pCmdComEvent->stPstestParam.pucData)
            {
                AtcAp_Free(pCmdComEvent->stPstestParam.pucData);
            }
            break;
        case D_ATC_EVENT_QNIDD:
            if(NULL != pCmdComEvent->stQniddParam.pucData)
            {
                AtcAp_Free(pCmdComEvent->stQniddParam.pucData);
            }
            break;
        default:
            break;
    }

    AtcAp_Free(pCmdComEvent);
}

#ifdef SINGLE_CORE
static void AtcAp_SendAtcDateReqToNas(unsigned char *pReqMsg, unsigned short usMsgLen)
{
    PS_MSG_FORMAT_TYPE      PSMsg;
    MSG_HEADER_STRU         *pMsgHdr;

    pMsgHdr = (MSG_HEADER_STRU *)pReqMsg;

    pMsgHdr->ulSrcTskId  = NAS_THREAD_ID;
    pMsgHdr->ulDestTskId = ATC_THREAD_ID;
    pMsgHdr->ulMemSize   = usMsgLen;
    pMsgHdr->ulMsgClass  = APP_PS_AP_CLASS;
    pMsgHdr->ulMsgName   = D_ATC_DATA_REQ;

    PSMsg.MsgType    = 0;
    PSMsg.MsgPointer = (unsigned long)pReqMsg;

    PsSendPsComExtMsg(&PSMsg);
}

int AtcAp_SndDataReqToPs(unsigned char *pCodeStream, unsigned short usCodeStreamLen)
{
    ST_ATC_DATA_REQ*        pAtcDataReq;
    unsigned short          usLen;
    UN_ATC_CMD_EVENT*       pCmdEvent;

#if RF_MT_MODE == 1
    if (g_rf_mt.rf_mt_mode == 1)
    {
        return ATC_AP_FALSE;
    }
#endif

    if(usCodeStreamLen <= 4)
    {
        usLen = sizeof(ST_ATC_DATA_REQ);
    }
    else
    {
        usLen = sizeof(ST_ATC_DATA_REQ) + usCodeStreamLen - 4;
    }
    
    pAtcDataReq = (ST_ATC_DATA_REQ *)AtcAp_Malloc(usLen + 1);
    if(g_AtcApInfo.ucUserAtFlg == ATC_AP_TRUE)
    {
        pAtcDataReq->ucSeqNum = g_AtcApInfo.stAppInterfaceInfo.ucSeqNum;
    }
    pAtcDataReq->usMsgLen = usCodeStreamLen;
    AtcAp_MemCpy(pAtcDataReq->aucMsgData, pCodeStream, usCodeStreamLen);

    pCmdEvent  = (UN_ATC_CMD_EVENT*)pCodeStream; 

    AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_SndDataReqToPs] ucUserAtFlg=%d, ucSeqNum=%d, event=%d", g_AtcApInfo.ucUserAtFlg, pAtcDataReq->ucSeqNum, pCmdEvent->stCgdcontParam.usEvent);
    AtcAp_SendAtcDateReqToNas((unsigned char*)pAtcDataReq, usLen);

    return ATC_AP_TRUE;
}
#else //M3
int AtcAp_SndDataReqToShm(unsigned char *pCodeStream, unsigned short usCodeStreamLen)
{
    unsigned short          usLen;
    UN_ATC_CMD_EVENT*       pCmdEvent;
    int                     ret;
    
    ATC_INTER_CORE_MSG_M3ToDSP_STRU* pInterCoreMsg;
    
    if(usCodeStreamLen <= 4)
    {
        usLen = sizeof(ATC_INTER_CORE_MSG_M3ToDSP_STRU);
    }
    else
    {
        usLen = sizeof(ATC_INTER_CORE_MSG_M3ToDSP_STRU) + usCodeStreamLen - 4;
    }

    pInterCoreMsg = (ATC_INTER_CORE_MSG_M3ToDSP_STRU *)AtcAp_Malloc(usLen);
    if(g_AtcApInfo.ucUserAtFlg == ATC_AP_TRUE)
    {
        pInterCoreMsg->ucSeqNum = g_AtcApInfo.stAppInterfaceInfo.ucSeqNum;
    }
    else
    {
        pInterCoreMsg->ucSeqNum = 0;
    }
    pInterCoreMsg->usMsgLen = usCodeStreamLen;
    if(0 != usCodeStreamLen)
    {
        AtcAp_MemCpy(pInterCoreMsg->aucMsgData, pCodeStream, usCodeStreamLen);
    }
    pCmdEvent  = (UN_ATC_CMD_EVENT*)pCodeStream; 
    AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_SndDataReqToShm] ucUserAtFlg=%d, ucSeqNum=%d, event=%d", g_AtcApInfo.ucUserAtFlg, pInterCoreMsg->ucSeqNum, pCmdEvent->stCgdcontParam.usEvent);
    
    ret = send_ps_shm_msg(pInterCoreMsg, usLen);
    AtcAp_Free(pInterCoreMsg);

    return ret;
}
#endif

unsigned short AtcAp_StrPrintf_AtcRspBuf(const char* FormatBuffer,...)
{
    unsigned short          usLen;
    va_list                 ap;
    
    va_start(ap, FormatBuffer);
    usLen = vsnprintf((char*)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), D_ATC_RSP_MAX_BUF_SIZE - g_AtcApInfo.stAtRspInfo.usRspLen - 1, FormatBuffer, ap);
    g_AtcApInfo.stAtRspInfo.usRspLen += usLen;
    va_end(ap);

    return usLen;
}

static unsigned char AtcAp_IsCmdComEventChk(unsigned short usEvent)
{
    switch(usEvent)
    {
        case D_ATC_EVENT_CIMI:
        case D_ATC_EVENT_CLAC:
        case D_ATC_EVENT_CEDRXRDP:
        case D_ATC_EVENT_NCCID:
        case D_ATC_EVENT_NCSEARFCN:
        case D_ATC_EVENT_NFPLMN:
        case D_ATC_EVENT_CSQ:
        case D_ATC_EVENT_CEER:
        case D_ATC_EVENT_CNMPSD:
#if SIMMAX_SUPPORT
        case D_ATC_EVENT_CUPREFER:
        case D_ATC_EVENT_CUPREFERTH_R:
#endif
        case D_ATC_EVENT_ZICCID:
        case D_ATC_EVENT_ZCELLINFO:
        case D_ATC_EVENT_QCCID:
        case D_ATC_EVENT_QBAND_R:
        case D_ATC_EVENT_QBAND_T:
        case D_ATC_EVENT_QCGDEFCONT_R:
        case D_ATC_EVENT_QCSEARFCN:
        case D_ATC_EVENT_QLOCKF_R:
        case D_ATC_EVENT_W:
        case D_ATC_EVENT_QSPCHSC_R:
            return ATC_AP_TRUE;
        default:
            return ATC_AP_FALSE;
    }
}

static unsigned char AtcAp_IsCmdParameterChk(unsigned short usEvent)
{
    switch(usEvent)
    {
        case D_ATC_EVENT_CGATT:
        case D_ATC_EVENT_CGATT_R:
        case D_ATC_EVENT_CMEE:
#ifdef NBIOT_SMS_FEATURE
        case D_ATC_EVENT_CSMS:
        case D_ATC_EVENT_CMGF:
#endif
        case D_ATC_EVENT_CSCON:
        case D_ATC_EVENT_RAI:
        case D_ATC_EVENT_NL2THP:
        case D_ATC_EVENT_CNEC:
        case D_ATC_EVENT_MNBIOTEVENT:
        case D_ATC_EVENT_MNBIOTEVENT_R:
        case D_ATC_EVENT_NPBPLMNS:
        case D_ATC_EVENT_NBACKOFF:
        case D_ATC_EVENT_QICSGP_R:
        case D_ATC_EVENT_QSPCHSC:
            return ATC_AP_TRUE;
        default:
            return ATC_AP_FALSE;
    }
}

static unsigned short AtcAp_GetCmdEventBuffSize(UN_ATC_CMD_EVENT *pCmdEvent)
{
    if(ATC_AP_TRUE == AtcAp_IsCmdComEventChk(pCmdEvent->stCgdcontParam.usEvent))
    {
        return sizeof(ST_ATC_CMD_COM_EVENT);
    }

    if(ATC_AP_TRUE == AtcAp_IsCmdParameterChk(pCmdEvent->stCgdcontParam.usEvent))
    {
        return sizeof(ST_ATC_CMD_PARAMETER);
    }

    switch(pCmdEvent->stCgdcontParam.usEvent)
    {
        case D_ATC_EVENT_CGSN:
            return sizeof(ST_ATC_CGSN_PARAMETER);
        case D_ATC_EVENT_CEREG:
            return sizeof(ST_ATC_CEREG_PARAMETER);
        case D_ATC_EVENT_CGDCONT:
            return offsetof(ST_ATC_CGDCONT_PARAMETER, aucApnValue) + pCmdEvent->stCgdcontParam.ucApnLen;
        case D_ATC_EVENT_CFUN:
            return sizeof(ST_ATC_CFUN_PARAMETER);
        case D_ATC_EVENT_CESQ:
            return sizeof(ST_ATC_CESQ_PARAMETER);
        case D_ATC_EVENT_CGPADDR:
            return sizeof(ST_ATC_CGPADDR_PARAMETER);
        case D_ATC_EVENT_CGACT:
            return sizeof(ST_ATC_CGACT_PARAMETER);
        case D_ATC_EVENT_CSODCP:
            return offsetof(ST_ATC_CSODCP_PARAMETER, pucCpdata) + pCmdEvent->stCsodcpParam.usCpdataLength;
        case D_ATC_EVENT_CRTDCP:
            return sizeof(ST_ATC_CRTDCP_PARAMETER);
        case D_ATC_EVENT_CEDRXS:
            return sizeof(ST_ATC_CEDRXS_PARAMETER);
        case D_ATC_EVENT_CPSMS:
            return sizeof(ST_ATC_CPSMS_PARAMETER);
        case D_ATC_EVENT_CGAPNRC:
            return sizeof(ST_ATC_CGAPNRC_PARAMETER);
#ifdef ESM_DEDICATED_EPS_BEARER
        case D_ATC_EVENT_CGDSCONT:
            return sizeof(ST_ATC_CGDSCONT_PARAMETER);
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
        case D_ATC_EVENT_CGTFT:
            return sizeof(ST_ATC_CGTFT_PARAMETER);
#endif
        case D_ATC_EVENT_CGEQOS:
            return sizeof(ST_ATC_CGEQOS_PARAMETER);
#ifdef ESM_EPS_BEARER_MODIFY
        case D_ATC_EVENT_CGCMOD:
            return sizeof(ST_ATC_CGCMOD_PARAMETER);
#endif
#ifdef NBIOT_SMS_FEATURE
        case D_ATC_EVENT_CSCA:
            return sizeof(ST_ATC_CSCA_PARAMETER);
        case D_ATC_EVENT_CMGS:
        case D_ATC_EVENT_CMGC:
        case D_ATC_EVENT_CNMA:
            return offsetof(ST_ATC_PDU_TPDU_PARAMETER, aucPduData);
        case D_ATC_EVENT_CMMS:
            return sizeof(ST_ATC_CMMS_PARAMETER);
        case D_ATC_AP_SMS_PDU_REQ:
            return offsetof(ST_ATC_AP_SMS_PDU_PARAMETER, pucPduData) + pCmdEvent->stAtcApPduParam.usPduLength;
#endif
        case D_ATC_EVENT_COPS:
            return sizeof(ST_ATC_COPS_PARAMETER);
        case D_ATC_EVENT_CSIM:
            return offsetof(ST_ATC_CSIM_PARAMETER, aucCommand) + pCmdEvent->stCsimParam.usLength;
        case D_ATC_EVENT_CCHC:
            return sizeof(ST_ATC_CCHC_PARAMETER);
        case D_ATC_EVENT_CCHO:
            return sizeof(ST_ATC_CCHO_PARAMETER);
        case D_ATC_EVENT_CGLA:
            return offsetof(ST_ATC_CGLA_PARAMETER, aucCommand) + pCmdEvent->stCglaParam.usLength;
        case D_ATC_EVENT_CRSM:
            return offsetof(ST_ATC_CRSM_PARAMETER, pucData) + pCmdEvent->stCrsmParam.ucDataLen;
        case D_ATC_EVENT_CGEREP:
            return sizeof(ST_ATC_CGEREP_PARAMETER);
        case D_ATC_EVENT_CCIOTOPT:
            return sizeof(ST_ATC_CCIOTOPT_PARAMETER);
        case D_ATC_EVENT_CGEQOSRDP:
            return sizeof(ST_ATC_CGEQOSRDP_PARAMETER);
        case D_ATC_EVENT_CTZR:
            return sizeof(ST_ATC_CTZR_PARAMETER);
        case D_ATC_EVENT_CGCONTRDP:
            return sizeof(ST_ATC_CGCONTRDP_PARAMETER);
        case D_ATC_EVENT_CPIN:
            return sizeof(ST_ATC_CPIN_PARAMETER);
        case D_ATC_EVENT_CLCK:
            return sizeof(ST_ATC_CLCK_PARAMETER);
        case D_ATC_EVENT_CPWD:
            return sizeof(ST_ATC_CPWD_PARAMETER);

        case D_ATC_EVENT_NUESTATS:
            return sizeof(ST_ATC_NUESTATS_PARAMETER);
        case D_ATC_EVENT_NEARFCN:
            return sizeof(ST_ATC_NEARFCN_PARAMETER);
        case D_ATC_EVENT_NBAND:
            return sizeof(ST_ATC_NBAND_PARAMETER);
        case D_ATC_EVENT_NCONFIG:
            return sizeof(ST_ATC_NCONFIG_PARAMETER);
        case D_ATC_EVENT_NSET:
        case D_ATC_EVENT_NSET_R:
            return sizeof(ST_ATC_NSET_PARAMETER);
#ifdef LCS_MOLR_ENABLE
        case D_ATC_EVENT_CMOLR:
            return sizeof(ST_ATC_CMOLR_PARAMETER); 
#endif
        case D_ATC_EVENT_CIPCA:
            return sizeof(ST_ATC_CIPCA_PARAMETER);
        case D_ATC_EVENT_CGAUTH:
            return sizeof(ST_ATC_CGAUTH_PARAMETER);
        case D_ATC_EVENT_CPINR:
            return sizeof(ST_ATC_CPINR_PARAMETER);
        case D_ATC_EVENT_NPOWERCLASS:
            return sizeof(ST_ATC_NPOWERCLASS_PARAMETER);
        case D_ATC_EVENT_NPTWEDRXS:
            return sizeof(ST_ATC_CEDRXS_PARAMETER);
        case D_ATC_EVENT_NPIN:
            return sizeof(ST_ATC_NPIN_PARAMETER);
        case D_ATC_EVENT_NTSETID:
            return sizeof(ST_ATC_NTSETID_PARAMETER);
        case D_ATC_EVENT_NCIDSTATUS:
            return sizeof(ST_ATC_NCIDSTATUS_PARAMETER);
        case D_ATC_EVENT_NGACTR:
            return sizeof(ST_ATC_NGACTR_PARAMETER);
        case D_ATC_EVENT_NPOPB:
            return sizeof(ST_ATC_NPOPB_PARAMETER);
        case D_ATC_EVENT_NIPINFO:
            return sizeof(ST_ATC_NIPINFO_PARAMETER);
        case D_ATC_EVENT_NQPODCP:
            return sizeof(ST_ATC_NQPODCP_PARAMETER);
        case D_ATC_EVENT_NSNPD:
            return offsetof(ST_ATC_NSNPD_PARAMETER, pucNonIpData) + pCmdEvent->stNsnpdParam.usNonIpDataLen;
        case D_ATC_EVENT_NQPNPD:
            return sizeof(ST_ATC_NQPNPD_PARAMETER);
        case D_ATC_EVENT_NRNPDM:
            return sizeof(ST_ATC_NRNPDM_PARAMETER);
        case D_ATC_EVENT_NCPCDPR:
            return sizeof(ST_ATC_NCPCDPR_PARAMETER);
        case D_ATC_EVENT_CGPIAF:
            return sizeof(ST_ATC_CGPIAF_PARAMETER);
#if SIMMAX_SUPPORT
        case D_ATC_EVENT_CUPREFERTH:
            return sizeof(ST_ATC_CUPREFERTH_PARAMETER);
#endif
        case D_ATC_EVENT_NLOCKF:
            return sizeof(ST_ATC_NLOCKF_PARAMETER);
        case D_ATC_EVENT_QCGDEFCONT:
            return sizeof(ST_ATC_QCGDEFCONT_PARAMETER);
        case D_ATC_EVENT_QBAND:
            return sizeof(ST_ATC_QBAND_PARAMETER);
        case D_ATC_EVENT_QENG:
            return sizeof(ST_ATC_QENG_PARAMETER);
        case D_ATC_EVENT_QCFG:
            return sizeof(ST_ATC_QCFG_PARAMETER);
        case D_ATC_EVENT_NSIMWC:
            return sizeof(ST_ATC_NSIMWC_PARAMETER);
        case D_ATC_EVENT_PSTEST:
            return offsetof(ST_ATC_PSTEST_PARAMETER, pucData) + pCmdEvent->stPstestParam.usDataLen;
        case D_ATC_EVENT_QNIDD:
            return offsetof(ST_ATC_QNIDD_PARAMETER, pucData) + pCmdEvent->stQniddParam.usDataLen;
        case D_ATC_EVENT_PRESETFREQ:
        case D_ATC_EVENT_PRESETFREQ_R:
            return sizeof(ST_ATC_PRESETFREQ_PARAMETER);
        case D_ATC_EVENT_SIMUUICC:
        case D_ATC_EVENT_SIMUUICC_R:
            return sizeof(ST_ATC_SIMUUICC_PARAMETER);
        case D_ATC_EVENT_QLOCKF:
            return sizeof(ST_ATC_QLOCKF_PARAMETER);
        case D_ATC_EVENT_QICSGP:
            return sizeof(ST_ATC_QICSGP_PARAMETER);
        default:
            return sizeof(ST_ATC_CMD_COM_EVENT);
    }
}

static void  AtcAp_Encode_ArrayOrPointer(unsigned char* pbDesc, unsigned short usDateLen, unsigned char ucSrc1Size, unsigned char* pSrc1, unsigned char* pSrc2)
{
    if(0 == usDateLen)
    {
        return;
    }

    if(usDateLen <= ucSrc1Size)
    {
        AtcAp_MemCpy(pbDesc, pSrc1, usDateLen);
    }
    else
    {
        AtcAp_MemCpy(pbDesc, pSrc2, usDateLen);
    }
}

void AtcAp_Encode_UN_ATC_CMD_EVENT(UN_ATC_CMD_EVENT *pCmdEvent, unsigned char** ppCodeStream, unsigned short* pusLen)
{
    unsigned short         usLen        = 0;
    unsigned char         *pCodeStream;
    unsigned short         usOffset;

    usLen       = AtcAp_GetCmdEventBuffSize(pCmdEvent);
    
    pCodeStream = (unsigned char *)AtcAp_Malloc(usLen);
    switch(pCmdEvent->stCgdcontParam.usEvent)
    {
        case D_ATC_EVENT_CGDCONT:
            usOffset = offsetof(ST_ATC_CGDCONT_PARAMETER, aucApnValue);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);

            AtcAp_Encode_ArrayOrPointer(pCodeStream + usOffset, 
                                        pCmdEvent->stCgdcontParam.ucApnLen,
                                        sizeof(pCmdEvent->stCgdcontParam.aucApnValue),
                                        pCmdEvent->stCgdcontParam.aucApnValue,
                                        pCmdEvent->stCgdcontParam.pucApnValue);
            break;
        case D_ATC_EVENT_CSODCP:
            usOffset = offsetof(ST_ATC_CSODCP_PARAMETER, pucCpdata);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stCsodcpParam.usCpdataLength)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stCsodcpParam.pucCpdata, pCmdEvent->stCsodcpParam.usCpdataLength);
            }
            break;
        case D_ATC_EVENT_CSIM:
            usOffset = offsetof(ST_ATC_CSIM_PARAMETER, aucCommand);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);

            AtcAp_Encode_ArrayOrPointer(pCodeStream + usOffset, 
                                        pCmdEvent->stCsimParam.usLength,
                                        sizeof(pCmdEvent->stCsimParam.aucCommand),
                                        pCmdEvent->stCsimParam.aucCommand,
                                        pCmdEvent->stCsimParam.pucCommand);
            break;
        case D_ATC_EVENT_CGLA:
            usOffset = offsetof(ST_ATC_CGLA_PARAMETER, aucCommand);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);

            AtcAp_Encode_ArrayOrPointer(pCodeStream + usOffset, 
                                        pCmdEvent->stCglaParam.usLength,
                                        sizeof(pCmdEvent->stCglaParam.aucCommand),
                                        pCmdEvent->stCglaParam.aucCommand,
                                        pCmdEvent->stCglaParam.pucCommand);
            break;
        case D_ATC_EVENT_CRSM:
            usOffset = offsetof(ST_ATC_CRSM_PARAMETER, pucData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stCrsmParam.ucDataLen)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stCrsmParam.pucData, pCmdEvent->stCrsmParam.ucDataLen);
            }
            break;
        case D_ATC_EVENT_NSNPD:
            usOffset = offsetof(ST_ATC_NSNPD_PARAMETER, pucNonIpData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stNsnpdParam.usNonIpDataLen)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stNsnpdParam.pucNonIpData, pCmdEvent->stNsnpdParam.usNonIpDataLen);
            }
            break;
#ifdef NBIOT_SMS_FEATURE
        case D_ATC_AP_SMS_PDU_REQ:
            usOffset = offsetof(ST_ATC_AP_SMS_PDU_PARAMETER, pucPduData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stAtcApPduParam.pucPduData)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stAtcApPduParam.pucPduData, pCmdEvent->stAtcApPduParam.usPduLength);
            }
            break;
        case D_ATC_EVENT_CMGS:
        case D_ATC_EVENT_CMGC:
        case D_ATC_EVENT_CNMA:
            usOffset = offsetof(ST_ATC_PDU_TPDU_PARAMETER, aucPduData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            break;
#endif
        case D_ATC_EVENT_PSTEST:
            usOffset = offsetof(ST_ATC_PSTEST_PARAMETER, pucData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stPstestParam.usDataLen)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stPstestParam.pucData, pCmdEvent->stPstestParam.usDataLen);
            }
            break;
        case D_ATC_EVENT_QNIDD:
            usOffset = offsetof(ST_ATC_QNIDD_PARAMETER, pucData);
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usOffset);
            if(0 != pCmdEvent->stQniddParam.usDataLen)
            {
                AtcAp_MemCpy(pCodeStream + usOffset, pCmdEvent->stQniddParam.pucData, pCmdEvent->stQniddParam.usDataLen);
            }
            break;
        default:
            AtcAp_MemCpy(pCodeStream, pCmdEvent, usLen);
            break;
    }

    *pusLen = usLen;
    *ppCodeStream = pCodeStream;
}

/*******************************************************************************
  MODULE    : AtcAp_Strncmp
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
int AtcAp_Strncmp(unsigned char *pStr1, unsigned char *pStr2)
{
    return strcmp((char*)pStr1, (char*)pStr2);
}

/*******************************************************************************
  MODULE    : ATC_DecConv
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
static void  AtcAp_DecConv(ST_ATC_AP_FORMAT *FCTable, unsigned int IntData)
{
    unsigned char  i;
    unsigned char  DispData[20] = { 0 };
    unsigned char  Length = 0;

    if (0 == IntData)
    {
        DispData[Length++] = 0x30;
    }
    else
    {
        while(IntData)
        {
            DispData[Length++] = (unsigned char)(IntData % 10 + 0x30);
            IntData = IntData / 10;
        };
    }

    for(i=0;i!=Length;i++){
        *FCTable->EditBufRef++ = DispData[(Length-i)-1];
        FCTable->EditCount++;
    }

    *FCTable->EditBufRef = 0;
}

/*******************************************************************************
  MODULE    : ATC_HexConv
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
static void  AtcAp_HexConv(ST_ATC_AP_FORMAT *FCTable, unsigned int HexData)
{
    unsigned char  DispData[20];
    unsigned short  Length = 0;
    unsigned short  i;
    const unsigned char LargeTable[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    for (;;)
    {
        DispData[Length++] = LargeTable[HexData & 0x0F];
        HexData = HexData >> 4;
        if (0 == HexData)
        {
            break;
        }
 
    }
    if (1 == (Length % 2))
    {
        DispData[Length++] = 0x30;
    }

    for(i = 0;i != Length;i++)
    {
        *FCTable->EditBufRef++ = DispData[(Length - i) - 1];
        FCTable->EditCount++;
    }
}

/*******************************************************************************
  MODULE    : ATC_FixedLenHexConv
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  GCM   2018.12.12   create
*******************************************************************************/
static void  AtcAp_FixedLenHexConv(ST_ATC_AP_FORMAT *FCTable, unsigned int HexData, unsigned char ucHexStrLen)
{
    unsigned char  DispData[8];
    unsigned short  Length = 0;
    unsigned short  i;
    const unsigned char LargeTable[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    if(ucHexStrLen == 0)
    {
        if(HexData != 0)
        {
            i = 28;
            ucHexStrLen = 8;
            while(i != 0 && HexData>>i == 0)
            {
                i-=4;
                ucHexStrLen--;
            }            
        }
        else
        {
            ucHexStrLen = 1;
        }
    }

    for(Length = 0; Length < ucHexStrLen; Length++)
    {
        DispData[Length] = LargeTable[HexData & 0x0F];
        HexData = HexData >> 4;
    }

    for(i = 0;i != Length;i++)
    {
        *FCTable->EditBufRef++ = DispData[(Length - i) - 1];
        FCTable->EditCount++;
    }    
}

unsigned short AtcAp_StrPrintf(unsigned char *EditBuffer, const unsigned char *FormatBuffer,...)
{
    va_list         ap;
    unsigned char           *StringDataPoint;
     signed int             IntData;
    unsigned int            uIntData;
    unsigned char i;
    ST_ATC_AP_FORMAT      FCTable;

    va_start(ap,FormatBuffer);          
    FCTable.FormatRef = (unsigned char *)FormatBuffer;
    FCTable.EditBufRef = (unsigned char *)EditBuffer;
    FCTable.EditCount = 0;
    *FCTable.EditBufRef = 0;

    while(*FCTable.FormatRef){
        if (*FCTable.FormatRef == '%')
        {
           FCTable.FormatRef++;
            switch(*FCTable.FormatRef++)
            {
                case 'c':               
                    *FCTable.EditBufRef++ = (unsigned char)va_arg(ap,  signed int);      
                    *FCTable.EditBufRef = 0;
                    FCTable.EditCount++;
                    break;
                case 's':               
                    StringDataPoint = (unsigned char *)va_arg(ap, unsigned char *);    
                    for (i = 0; ;i++)
                    {
                        if (0x00 == *StringDataPoint)
                        {
                            break;
                        }
                        else
                        {
                            *FCTable.EditBufRef++ = *StringDataPoint++;
                            FCTable.EditCount++;
                        }
                    }
                    *FCTable.EditBufRef = 0;
                    break;
                case 'd':               
                    IntData = va_arg(ap,  signed int);
                    AtcAp_DecConv(&FCTable, (unsigned int)IntData);
                    break;
                case 'x':               
                    StringDataPoint = (unsigned char *)va_arg(ap,unsigned char *);
                    for (i = 0; i < g_AtcApInfo.stAtRspInfo.ucHexStrLen; i++)
                    {
                        AtcAp_HexConv(&FCTable, *StringDataPoint);

                        StringDataPoint++;
                    }
                    *FCTable.EditBufRef = 0;
                    break;
                case 'h':               
                    uIntData = va_arg(ap,  signed int);
                    AtcAp_FixedLenHexConv(&FCTable, uIntData, g_AtcApInfo.stAtRspInfo.ucHexStrLen);
                    g_AtcApInfo.stAtRspInfo.ucHexStrLen = 0;
                    *FCTable.EditBufRef = 0;
                    break;
                default:
                    break;
            }
        }
        else{
            *FCTable.EditBufRef++ = *FCTable.FormatRef++;
            *FCTable.EditBufRef = 0;
            FCTable.EditCount++;
        }
    }
    va_end(ap);

    return(FCTable.EditCount);
}


void AtcAp_WriteHexPara_M(unsigned int uiFlg, unsigned int uiPara, unsigned char* pRespBuff, unsigned char ucHexLen)
{
    g_AtcApInfo.stAtRspInfo.ucHexStrLen = ucHexLen;
    if (uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%h",
            uiPara);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",");
    }
    g_AtcApInfo.stAtRspInfo.ucHexStrLen = 0;
    return;
}

/*******************************************************************************
  MODULE    : AtcAp_WriteStrPara_M
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   JiangNa   2018.07.17   create
*******************************************************************************/
void AtcAp_WriteStrPara_M(unsigned int uiFlg, unsigned char *pucPara )
{
    if (NULL == pucPara)
    {
        return;
    }
    if (0 != uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%s%c",
            D_ATC_N_QUOTATION,pucPara,D_ATC_N_QUOTATION);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",");
    }
    return;
}

void AtcAp_WriteStrPara_M_NoQuotation(unsigned int uiFlg, unsigned char *pucPara )
{
    if (NULL == pucPara)
    {
        return;
    }
    if (0 != uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%s",pucPara);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",");
    }
    return;
}

/*******************************************************************************
  MODULE    : AtcAp_WriteIntPara_M
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   JiangNa   2018.07.17   create
*******************************************************************************/
void AtcAp_WriteIntPara_M(unsigned int uiFlg, unsigned int uiPara, unsigned char *pucAtcRspBuf)
{
    if (uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d",
            uiPara);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",");
    }
    return;
}

/*******************************************************************************
  MODULE    : AtcAp_WriteIntPara
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   JiangNa   2018.07.17   create
*******************************************************************************/
void AtcAp_WriteIntPara(unsigned int uiFlg, unsigned int uiPara )
{
    if (uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d",
            uiPara);
    }
    return;
}

/*******************************************************************************
  MODULE    : ATC_Write4BitData
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   tianchengbin   2018.11.26   create
*******************************************************************************/
void AtcAp_Write4BitData(unsigned char ucData)
{
    unsigned char i;
    unsigned char aucStr[5] = {0};
    for (i = 0; i < 4; i++)
    {
        aucStr[i] = ((ucData >> (3-i))&0x01) + 0x30;
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)",%c%s%c", D_ATC_N_QUOTATION, aucStr, D_ATC_N_QUOTATION);
}


/*******************************************************************************
  MODULE    : AtcAp_OutputAddr
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JiangNa    2018/12/18   create
*******************************************************************************/
void AtcAp_OutputAddr(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf)
{   
    if(4 == ucDataLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%d.%d.%d.%d%c",
            D_ATC_N_QUOTATION,  pData[0], pData[1], 
            pData[2], pData[3], D_ATC_N_QUOTATION);
    }
    else if (8 == ucDataLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%d.%d.%d.%d.%d.%d.%d.%d%c",
            D_ATC_N_QUOTATION,  pData[0], pData[1], 
            pData[2], pData[3], pData[4],
            pData[5], pData[6], pData[7], D_ATC_N_QUOTATION);
    }
    else if(16 == ucDataLen && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d%c",
            D_ATC_N_QUOTATION,  pData[0], pData[1], pData[2], pData[3], pData[4], pData[5], pData[6], pData[7], 
            pData[8], pData[9], pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], D_ATC_N_QUOTATION);
    }
    else if (32 == ucDataLen && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d%c",
            D_ATC_N_QUOTATION, pData[0], pData[1], pData[2], pData[3], pData[4], pData[5], pData[6], pData[7], 
            pData[8], pData[9], pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], pData[16], 
            pData[17], pData[18], pData[19], pData[20], pData[21], pData[22], pData[23], pData[24], pData[25],
            pData[26], pData[27], pData[28], pData[29], pData[30], pData[31], D_ATC_N_QUOTATION);
    }
    else if(ucDataLen >=16 && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 1)
    {
        AtcAp_OutputAddr_IPv6(ucDataLen, pData, pucAtcRspBuf);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%c", D_ATC_N_QUOTATION, D_ATC_N_QUOTATION);
    }
    return;
}

void AtcAp_OutputAddr_IPv6(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf)
{
    int     i = 0,j = 0;
    unsigned char    MaskCidr = 0;
    unsigned char    result;
    unsigned char    start = 8, end = 0, start1 =16, end1 = 8;
    unsigned char    ucZeroFlag = 0,ucZeroFlag1 = 0;
    unsigned short    pusIpv6Addr[16] = {0};
    
    for(i = 0; i < (ucDataLen / 2); i++)
    {
        pusIpv6Addr[j++] = pData[2*i] << 8 | pData[2*i+1];
    }

    if(g_factory_nv->tNvData.tNasNv.ucIpv6CompressZeros == 1)
    {
        for(i = 0; i < 8; i++)
        {
            if(pusIpv6Addr[i] == 0)
            {
                ucZeroFlag++;
                if(start == 8)
                {
                    start = i;
                }
            }
            else
            {
                if(ucZeroFlag == 1)
                {
                    ucZeroFlag--;
                    start = 8;
                }
                else if(ucZeroFlag > 1)
                {
                    end = i - 1;
                    break;
                }
            }
        }
        if(ucZeroFlag > 1 && end == 0)
        {
            end = i - 1;
        }
        
        if(ucDataLen == 32 )
        {
            for(i = 8; i < 16; i++)
            {
                if(pusIpv6Addr[i] == 0)
                {
                    ucZeroFlag1++;
                    if(start1 == 16)
                    {
                        start1 = i;
                    }
                }
                else
                {
                    if(ucZeroFlag1 == 1)
                    {
                        ucZeroFlag1--;
                        start1 = 16;
                    }
                    else if(ucZeroFlag1 > 1)
                    {
                        end1 = i - 1;
                        break;
                    }
                }
            }
            if(ucZeroFlag1 > 1 && end1 == 8)
            {
                end1 = i - 1;
            }
        }
    }
    
    if(ucDataLen == 32 && g_factory_nv->tNvData.tNasNv.ucIpv6SubnetNotation ==1)
    {
        result = 1;
        MaskCidr = 0;
        for(i = 16; i < 32; i++)
        {
            for(j = 0; j < 8; j++)
            {
                if(pData[i] & (0x80 >>j))
                {
                    MaskCidr++;
                }
                else
                {
                    result = 0;
                    break;
                }
            }
            if(result == 0)
            {
                break;
            }
        }
    }
#if (VER_BC95 || Custom_09)
    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)",");
#else
    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)",%c", D_ATC_N_QUOTATION);
#endif
    if(g_factory_nv->tNvData.tNasNv.ucIpv6LeadingZeros == 0)
    {
        for(i = 0; i < 8; i++)
        {
            if((i < start) || (i > end)) 
            {
                if(i == 0)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)"%X", pusIpv6Addr[i]);
                }
                else
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":%X",pusIpv6Addr[i]);
                }
            }
            else if(i == start )
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                i = end;
                if(i == 7)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                }
            }
        }

        if(ucDataLen == 32 && g_factory_nv->tNvData.tNasNv.ucIpv6SubnetNotation ==1)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)"/%d",MaskCidr);
        }
        else if(ucDataLen == 32 && g_factory_nv->tNvData.tNasNv.ucIpv6SubnetNotation == 0)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)" ");
            for(i = 8; i < 16; i++)
            {
                if((i < start1) || (i > end1))
                {
                    if(i == 8)
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)"%X", pusIpv6Addr[i]);
                    }
                    else
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":%X", pusIpv6Addr[i]);
                    }
                }
                else if(i == start1 )
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                    i = end1;
                    if(i == 15)
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                    }
                }
            }
        }
    }
    else
    {
        for(i = 0; i < 8; i++)
        {
            if((i < start) || (i > end))
            {
                if(i == 0)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)"%04X", pusIpv6Addr[i]);
                }
                else
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":%04X", pusIpv6Addr[i]);
                }
            }
            else if(i == start )
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                i = end;
                if(i == 7)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                }
            }
        }

        if(ucDataLen == 32 && g_factory_nv->tNvData.tNasNv.ucIpv6SubnetNotation ==1)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)"/%d",MaskCidr);
        }
        else if(ucDataLen == 32 && g_factory_nv->tNvData.tNasNv.ucIpv6SubnetNotation == 0)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)" ");
            for(i = 8; i < 16; i++)
            {
                if((i < start1) || (i > end1))
                {
                    if(i == 8)
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)"%04X", pusIpv6Addr[i]);
                    }
                    else
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":%04X", pusIpv6Addr[i]);
                    }
                }
                else if(i == start1 )
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                    i = end1;
                    if(i == 15)
                    {
                        g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
                    }
                }
            }
        }
    }
#if (VER_BC95 || Custom_09)
    return;
#endif
    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const char *)"%c", D_ATC_N_QUOTATION);

    return;
}

void AtcAp_OutputAddr_NoQuotation(unsigned char ucDataLen, unsigned char *pData, unsigned char *pucAtcRspBuf)
{   
    if(4 == ucDataLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d.%d.%d.%d",
            pData[0], pData[1], 
            pData[2], pData[3]);
    }
    else if (8 == ucDataLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d.%d.%d.%d.%d.%d.%d.%d",
            pData[0], pData[1], 
            pData[2], pData[3], pData[4],
            pData[5], pData[6], pData[7]);
    }
    else if(16 == ucDataLen  && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d",
            pData[0], pData[1], pData[2], pData[3], pData[4], pData[5], pData[6], pData[7], 
            pData[8], pData[9], pData[10], pData[11], pData[12], pData[13], pData[14], pData[15]);
    }
    else if (32 == ucDataLen  && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d",
            pData[0], pData[1], pData[2], pData[3], pData[4], pData[5], pData[6], pData[7], 
            pData[8], pData[9], pData[10], pData[11], pData[12], pData[13], pData[14], pData[15], pData[16], 
            pData[17], pData[18], pData[19], pData[20], pData[21], pData[22], pData[23], pData[24], pData[25],
            pData[26], pData[27], pData[28], pData[29], pData[30], pData[31]);
    }
    else if(ucDataLen >=16 && g_factory_nv->tNvData.tNasNv.ucIpv6AddressFormat == 1)
    {
        AtcAp_OutputAddr_IPv6(ucDataLen, pData, pucAtcRspBuf);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",");
    }
    return;
}

void AtcAp_OutputAddr_IPv6ColonFormat(unsigned char *pData, unsigned char *pucAtcRspBuf)
{
#if (VER_BC95 && !Custom_09)
    unsigned char    ucZeroFalg = 0;
    int      i,start = 14,end = 0;

    for(i = 0; (i <= 14  && end == 0); (i=i+2))
    {
        if((pData[i] == 0) && (pData[i+1] == 0))
        {
            ucZeroFalg = ucZeroFalg + 1;
            if(start == 14)
            {
                start = i;
            }
        }
        else
        {
            if(ucZeroFalg == 1)
            {
                ucZeroFalg = ucZeroFalg - 1;
                start = 14;
            }
            if(ucZeroFalg > 1)
            {
                end = i - 2;
            }
        }
    }
    if(ucZeroFalg > 1 && end == 0)
    {
        end = i - 2;
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)",");
    for(i = 0 ;i <= 14 ;(i = i+2))
    {
        if((i < start) || (i > end))
        {
            if(i == 0)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)"%X", (unsigned short)(pData[i] << 8 | pData[i+1]));
            }
            else
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":%X", (unsigned short)(pData[i] << 8 | pData[i+1]));
            }
        }
        else if(i == start )
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
            i = end;
            if(i == 14)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)":");
            }
        }
    }
#else
    g_AtcApInfo.stAtRspInfo.usRspLen += sprintf((char*)pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const char *)",\"%X:%X:%X:%X:%X:%X:%X:%X\"", 
                                                (unsigned short)(pData[0] << 8 | pData[1]), (unsigned short)(pData[2] << 8 | pData[3]),
                                                (unsigned short)(pData[4] << 8 | pData[5]), (unsigned short)(pData[6] << 8 | pData[7]),
                                                (unsigned short)(pData[8] << 8 | pData[9]), (unsigned short)(pData[10] << 8 | pData[11]),
                                                (unsigned short)(pData[12] << 8 | pData[13]), (unsigned short)(pData[14] << 8 | pData[15]));
#endif
}

/*******************************************************************************
  MODULE    : ATC_IntegerToPlmn
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JiangNa   2018.09.28   create
*******************************************************************************/
void AtcAp_IntegerToPlmn(unsigned long ulInputData, unsigned char *pOutputData)
{
    if (NULL != pOutputData)
    {
        pOutputData[0] = (unsigned char)(((ulInputData) >> 16) & 0x0F) + 0x30;
        pOutputData[1] = (unsigned char)(((ulInputData) >> 20) & 0x0F) + 0x30;
        pOutputData[2] = (unsigned char)(((ulInputData) >> 8) & 0x0F) + 0x30;
        pOutputData[3] = (unsigned char)((ulInputData) & 0x0F) + 0x30;
        pOutputData[4] = (unsigned char)(((ulInputData) >> 4) & 0x0F) + 0x30;
        if (0x0F != (((ulInputData) >> 12) & 0x0F))
        {
            pOutputData[5] = (((ulInputData) >> 12) & 0x0F) + 0x30;
        }
    }

    return;
}

void AtcAp_OutputPortRange(unsigned char ucDataLen, unsigned short *pData, unsigned char* pRespBuff)
{
    if (0 != ucDataLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen, (const unsigned char *)",\"%d.%d\"", pData[0], pData[1]);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen, (const unsigned char *)",\"\"");
    }
}

static void AtcAp_GetIpv4SubnetMask(unsigned char ucFirstAddr, unsigned char* pbSubnetMask)
{
    AtcAp_MemSet(pbSubnetMask, 0, 4);
    //Class A
    if(1 <= ucFirstAddr && ucFirstAddr <= 126)
    {
        pbSubnetMask[0] = 255;
    }
    //Class B
    else if(128 <= ucFirstAddr && ucFirstAddr <= 191)
    {
        pbSubnetMask[0] = 255;
        pbSubnetMask[1] = 255; 
    }
    //Class C
    else if(192 <= ucFirstAddr && ucFirstAddr <= 223)
    {
        pbSubnetMask[0] = 255;
        pbSubnetMask[1] = 255;
        pbSubnetMask[2] = 255;
    }
    else
    {
        pbSubnetMask[0] = 255;
        pbSubnetMask[1] = 255;
        pbSubnetMask[2] = 255;
        pbSubnetMask[3] = 255;
    }   
}

static void AtcAp_OutputIpAddrAndSubMask(unsigned char ucPdpType, unsigned char* pucIpAddr, unsigned char *pucAtcRspBuf,unsigned char uccurrent_cid)
{
    unsigned char ucIpAddrAndSubMaskLen  = 0;
    unsigned char ucIpAddrAndSubMask[32] = { 0 };
    unsigned char ucIpv6addr_all[16];
    
    if(ucPdpType == D_PDP_TYPE_IPV4)
    {
        AtcAp_MemCpy(ucIpAddrAndSubMask, pucIpAddr, 4);
        AtcAp_GetIpv4SubnetMask(ucIpAddrAndSubMask[0], ucIpAddrAndSubMask + 4);
        ucIpAddrAndSubMaskLen = 8;
    }
    else //IPv6
    {
        if(ATC_GET_IPV6ADDR_ALL(ucIpv6addr_all) == ATC_AP_TRUE && uccurrent_cid == xy_get_working_cid())
        {
            AtcAp_MemCpy(ucIpAddrAndSubMask , ucIpv6addr_all, 16);
            ucIpAddrAndSubMaskLen = 32;
        }
        else
        {
            ucIpAddrAndSubMask[0] = 0xFE;
            ucIpAddrAndSubMask[1] = 0x80;
            AtcAp_MemCpy(ucIpAddrAndSubMask + 8, pucIpAddr, 8);
            ucIpAddrAndSubMaskLen = 32;
        }
    }
#if (VER_BC95 || Custom_09)
    AtcAp_OutputAddr_NoQuotation(ucIpAddrAndSubMaskLen, ucIpAddrAndSubMask, pucAtcRspBuf);
#else
    AtcAp_OutputAddr(ucIpAddrAndSubMaskLen, ucIpAddrAndSubMask, pucAtcRspBuf);
#endif
}

void AtcAp_CGCONTRDP_Print(EPS_CGCONTRDP_DYNAMIC_INFO *ptPdpDynamicInfo, unsigned char bPdpType, unsigned char *pucAtcRspBuf)
{
#if Custom_09
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
        (const unsigned char *)"\r\n+CGCONTRDP:%d,", ptPdpDynamicInfo->ucCid, ptPdpDynamicInfo->ucBearerId);
#else
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
        (const unsigned char *)"\r\n+CGCONTRDP:%d,%d", ptPdpDynamicInfo->ucCid, ptPdpDynamicInfo->ucBearerId);
#endif
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
        (const unsigned char *)",%c%s%c", D_ATC_N_QUOTATION, ptPdpDynamicInfo->aucApn, D_ATC_N_QUOTATION);

#if (VER_BC95 || Custom_09)
    //IpAddrAndSubMask
#if !Custom_09
    if(D_PDP_TYPE_IPV4 == bPdpType)
    {
        AtcAp_OutputIpAddrAndSubMask(bPdpType, ptPdpDynamicInfo->aucPdpAddrValue + 8, pucAtcRspBuf, ptPdpDynamicInfo->ucCid);
    }
    else if(D_PDP_TYPE_IPV6 == bPdpType)
    {
        AtcAp_OutputIpAddrAndSubMask(bPdpType, ptPdpDynamicInfo->aucPdpAddrValue, pucAtcRspBuf, ptPdpDynamicInfo->ucCid);
    }
    else
    {
        AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf);
    }

    AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //GwAddr
#else
    AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //IpAddrAndSubMask
    AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //GwAddr
#endif

    if(D_PDP_TYPE_IPV4 == bPdpType)
    {
        if(ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv4Flg)
        {
            AtcAp_OutputAddr_NoQuotation(4, ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv4, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf);
        }

        if(ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv4Flg)
        {
            AtcAp_OutputAddr_NoQuotation(4, ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv4, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf);
        }
    }
    else if(D_PDP_TYPE_IPV6 == bPdpType)
    {
        if(ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr_NoQuotation(16, ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv6, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf);
        }

        if(ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr_NoQuotation(16, ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv6, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf);
        }
    }
    else //Non-IP
    {
        AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //PriDnsAddr
        AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //SecDnsAddr
    }

#if Custom_09
    return;
#endif
    AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //PCSCFPrimAddr
    AtcAp_OutputAddr_NoQuotation(0, NULL, pucAtcRspBuf); //PCSCFSecAddr
#else
    //IpAddrAndSubMask
    if(D_PDP_TYPE_IPV4 == bPdpType)
    {
        AtcAp_OutputIpAddrAndSubMask(bPdpType, ptPdpDynamicInfo->aucPdpAddrValue + 8, pucAtcRspBuf, ptPdpDynamicInfo->ucCid);
    }
    else if(D_PDP_TYPE_IPV6 == bPdpType)
    {
        AtcAp_OutputIpAddrAndSubMask(bPdpType, ptPdpDynamicInfo->aucPdpAddrValue, pucAtcRspBuf, ptPdpDynamicInfo->ucCid);
    }
    else
    {
        AtcAp_OutputAddr(0, NULL, pucAtcRspBuf);
    }

    AtcAp_OutputAddr(0, NULL, pucAtcRspBuf); //GwAddr

    if(D_PDP_TYPE_IPV4 == bPdpType)
    {
        if(ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv4Flg)
        {
            AtcAp_OutputAddr(4, ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv4, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr(0, NULL, pucAtcRspBuf);
        }

        if(ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv4Flg)
        {
            AtcAp_OutputAddr(4, ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv4, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr(0, NULL, pucAtcRspBuf);
        }
    }
    else if(D_PDP_TYPE_IPV6 == bPdpType)
    {
        if(ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr(16, ptPdpDynamicInfo->stDnsAddr.ucPriDnsAddr_IPv6, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr(0, NULL, pucAtcRspBuf);
        }

        if(ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr(16, ptPdpDynamicInfo->stDnsAddr.ucSecDnsAddr_IPv6, pucAtcRspBuf);
        }
        else
        {
            AtcAp_OutputAddr(0, NULL, pucAtcRspBuf);
        }
    }
    else //Non-IP
    {
        AtcAp_OutputAddr(0, NULL, pucAtcRspBuf); //PriDnsAddr
        AtcAp_OutputAddr(0, NULL, pucAtcRspBuf); //SecDnsAddr
    }

    AtcAp_OutputAddr(0, NULL, pucAtcRspBuf); //PCSCFPrimAddr
    AtcAp_OutputAddr(0, NULL, pucAtcRspBuf); //PCSCFSecAddr
#endif
    AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf); //IMCNSignalling
    AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf); //LIPAInd

    if(D_PDP_TYPE_IPV4 == bPdpType)
    {
        AtcAp_WriteIntPara_M(ptPdpDynamicInfo->ucIPv4MTUFlag, ptPdpDynamicInfo->usIPv4MTU, pucAtcRspBuf);
    }
    else
    {
        AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf);
    }  
    
    AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf); //WLANOffload
    AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf); //LocalAddrInd

    if(D_PDP_TYPE_NonIP == bPdpType)
    {
        AtcAp_WriteIntPara_M(ptPdpDynamicInfo->ucNonIPMTUFlag, ptPdpDynamicInfo->usNonIPMTU, pucAtcRspBuf);
    }
    else
    {
        AtcAp_WriteIntPara_M(ATC_AP_FALSE, 0, pucAtcRspBuf);
    }

    AtcAp_WriteIntPara_M(ptPdpDynamicInfo->ucServingPLMNRateCtrValueFlag, ptPdpDynamicInfo->usServingPLMNRateCtrValue, pucAtcRspBuf);
}

void AtcAp_CSCA_ConvertScaByte2Str(unsigned char* pucScaData, unsigned char ucScaLen, unsigned char* pScaStr)
{
    unsigned char  i = 0;
    unsigned char  aucScaData[D_ATC_P_CSCA_IND_SCA_SIZE_MAX + 2] = { 0 };

    for (i = 0; i < ucScaLen; i++)
    {
        aucScaData[2 * i]     = (unsigned char)(pucScaData[i] & 0x0F);
        aucScaData[2 * i + 1] = (unsigned char)((pucScaData[i] & 0xF0) >> 4);
    }

    for(i = 0; i < ucScaLen * 2; i++)
    {
        if (10 == aucScaData[i])
        {
            pScaStr[i] = '*';
        }
        else if (11 == aucScaData[i])
        {
            pScaStr[i] = '#';
        }
        else if (aucScaData[i] >= 12 && aucScaData[i] <= 14)
        {
            pScaStr[i] = (unsigned char)(aucScaData[i] + 'a' - 12);
        }
        else if(aucScaData[i] <= 9)
        {
            pScaStr[i] = (unsigned char)(aucScaData[i] + '0');
        }
        else //'F'
        {
            break;
        }
    }
}

/*******************************************************************************
  MODULE    : ATC_HexToAsc
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JiangNa   2018.08.28   create
*******************************************************************************/
void AtcAp_HexToAsc(unsigned     short usLength, unsigned char *pOutData, unsigned char *pInputData)
{
    unsigned     short i = 0;
    unsigned     short j = 0;
    for (i = 0;i < usLength;i++)
    {
        if (((pInputData[i] >> 4) >= 0x0a) && ((pInputData[i] >> 4) <= 0x0f))
        {
            pOutData[j++] = (pInputData[i] >> 4) - 0x0a + 'A';
        }
        else if((pInputData[i] >> 4) <= 0x09)
        {
            pOutData[j++] = (pInputData[i] >> 4) + 0x30;
        }
        else
        {

        }
        if (((pInputData[i] & 0x0F) >= 0x0a) && ((pInputData[i] & 0x0F) <= 0x0f))
        {
            pOutData[j++] = (pInputData[i] & 0x0F) - 0x0a + 'A';
        }
        else if(((pInputData[i] & 0x0F) >= 0x00) && ((pInputData[i] & 0x0F) <= 0x09))
        {
            pOutData[j++] = (pInputData[i] & 0x0F) + 0x30;
        }
        else
        {

        }
    }
        return;
}

void AtcAp_ConvertByte2BitStr(unsigned char ucVal, unsigned char len, unsigned char* pBitStr)
{
    unsigned char  i;
    unsigned char  ucTemp  = 0;

    for (i  = 0; i < len; i++)
    {
        ucTemp = (0x08 >> i) & ucVal;
        pBitStr[i] = (ucTemp >> (3-i)) + 0x30;
    }
}

/*******************************************************************************
  MODULE    : AtcAp_WriteStrPara
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   JiangNa   2018.07.17   create
*******************************************************************************/
void AtcAp_WriteStrPara(unsigned          int uiFlg, unsigned char *pucPara )
{
    if (NULL == pucPara)
    {
        return;
    }
    if (uiFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",%c%s%c",
            D_ATC_N_QUOTATION,pucPara,D_ATC_N_QUOTATION);
    }
    return;
}

/*******************************************************************************
  MODULE    : AtcAp_RevertHexToDecimal
  FUNCTION  : convert reverted Hex to Decimal number
  NOTE      :
  HISTORY   :
      1.  WangCong    2018/12/11   create
*******************************************************************************/
unsigned char AtcAp_RevertHexToDecimal(unsigned char ucHex)
{
    unsigned char ucDecimal = 0;

    ucDecimal = (ucHex & 0x0F) * 10;
    ucDecimal += (ucHex & 0xF0) >> 4;

    return ucDecimal;
}

/*******************************************************************************
  MODULE    : AtcAp_ConvertTimeZone
  FUNCTION  : Convert GMT to local time
  NOTE      : temporarily save the resulting time in the input parameter
  HISTORY   :
      1.  WangCong    2018/12/11   create
*******************************************************************************/
void AtcAp_ConvertTimeZone(unsigned char *pTimeZoneTime,unsigned char ucDayLightTime)
{
    unsigned char i                 = 0;
    unsigned char ucYear            = 0;
    unsigned char ucMonth           = 0;
    unsigned char ucHour            = 0;
    unsigned char ucMinute          = 0;
    unsigned char ucSecond          = 0;
    unsigned char ucTimeZoneOffset  = 0;                                                        /* MSB0: positive    MGB1: negative */
    unsigned char ucLeapYear        = ATC_FALSE;                                                /* 0common year    1: leap year */
    unsigned short usDay            = 0;
    unsigned long ulTimeInMinute    = 0;                                                        /* save current time in MINUTE */
    unsigned long ulTimeInMinuteTmp = 0;

    /* retrieve GMT time */
    ucYear = AtcAp_RevertHexToDecimal(pTimeZoneTime[0]);
    ucMonth = AtcAp_RevertHexToDecimal(pTimeZoneTime[1]);
    usDay = (unsigned short)AtcAp_RevertHexToDecimal(pTimeZoneTime[2]);
    ucHour = AtcAp_RevertHexToDecimal(pTimeZoneTime[3]);
    ucMinute = AtcAp_RevertHexToDecimal(pTimeZoneTime[4]);
    ucSecond = AtcAp_RevertHexToDecimal(pTimeZoneTime[5]);
    ucTimeZoneOffset = (pTimeZoneTime[6] & 0x07) * 10;
    ucTimeZoneOffset += (pTimeZoneTime[6] & 0xF0) >> 4;
    ucTimeZoneOffset += ucDayLightTime * 4;

    ulTimeInMinute += (unsigned long)((ucYear * 365 + ucYear / 4) * 24 * 60);                   /* consider AD 0001.01.01 as 365 days passed */
    if(0 == ucYear % 4)
    {
        ucLeapYear = ATC_TRUE;
    }
    else
    {
        ucLeapYear = ATC_FALSE;
    }

    for(i = 0; i < (ucMonth - 1) && i < 11; i++)
    {
        ulTimeInMinute += (unsigned long)(ATC_MonthDayTbl[ucLeapYear][i] * 24 * 60);
    }

    ulTimeInMinute += (unsigned long)((usDay - 1) * 60 * 24);
    ulTimeInMinute += (unsigned long)(ucHour * 60);
    ulTimeInMinute += (unsigned long)ucMinute;

    if(0 == (pTimeZoneTime[6] & 0x08))
    {
        ulTimeInMinute += (unsigned long)(ucTimeZoneOffset * 15);
        if(ulTimeInMinute > (unsigned long)(((ucYear + 1) * 365 + ucYear / 4) * 24 * 60))
        {
            ucYear += 1;
            if(0 == ucYear % 4)
            {
                ucLeapYear = ATC_TRUE;
                ulTimeInMinute += 60 * 24;                                              /* an extra Feb.29 will be calculated afterwards */
            }
            else
            {
                if(ATC_TRUE == ucLeapYear)
                {
                    ulTimeInMinute -= 60 * 24;                                          /* an extra Feb.29 will be calculated afterwards */
                    ucLeapYear = ATC_FALSE;
                }
            }
        }
    }
    else
    {
        ulTimeInMinute -= (unsigned long)(ucTimeZoneOffset * 15);
        if(ulTimeInMinute < (unsigned long)((ucYear * 365 + ucYear / 4) * 24 * 60))
        {
            ucYear -= 1;
            if(0 == ucYear % 4)
            {
                ucLeapYear = ATC_TRUE;
                ulTimeInMinute += 60 * 24;                                              /* an extra Feb.29 will be calculated afterwards */
            }
            else
            {
                if(ATC_TRUE == ucLeapYear)
                {
                    ulTimeInMinute -= 60 * 24;                                          /* an extra Feb.29 will be calculated afterwards */
                    ucLeapYear = ATC_FALSE;
                }
            }
        }
    }

    ulTimeInMinute -= (unsigned long)((ucYear * 365 + ucYear / 4) * 24 * 60);                   /* exclude YEAR */
    usDay = (unsigned short)(ulTimeInMinute / (60 * 24) + 1);                                   /* calculate which day is it of the year */
    ulTimeInMinuteTmp = (unsigned long)((usDay - 1) * 60 * 24);
    for(i = 0; i < 12; i++)
    {
        if(usDay > ATC_MonthDayTbl[ucLeapYear][i])
        {
            usDay -= (unsigned short)ATC_MonthDayTbl[ucLeapYear][i];
        }
        else
        {
            break;
        }
    }
    ucMonth = i + 1;

    ulTimeInMinute -= ulTimeInMinuteTmp;                                                /* exclude DAY */
    ucHour = (unsigned char)(ulTimeInMinute / 60);
    ucMinute = (unsigned char)(ulTimeInMinute % 60);

    pTimeZoneTime[0] = ucYear;
    pTimeZoneTime[1] = ucMonth;
    pTimeZoneTime[2] = (unsigned char)usDay;
    pTimeZoneTime[3] = ucHour;
    pTimeZoneTime[4] = ucMinute;
    pTimeZoneTime[5] = ucSecond;
    pTimeZoneTime[6] = 0x00;

    return;
}


/*******************************************************************************
  MODULE    : ATC_OutputLocalTime
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
void AtcAp_OutputLocalTime(LNB_NAS_LOCAL_TIME_STRU* pLocalTime)
{
    unsigned char aucUtAndLtz[D_ATC_UTANDLTZ_LEN] = {0};
    unsigned char ucDayLightTime = 0;
    if (D_ATC_FLAG_TRUE == pLocalTime->ucUtAndLtzFlg)
    {
        AtcAp_MemCpy(aucUtAndLtz, pLocalTime->aucUtAndLtz, D_ATC_UTANDLTZ_LEN);

        if (D_ATC_FLAG_TRUE == pLocalTime->ucNwDayltSavTimFlg)
        {
            ucDayLightTime = pLocalTime->ucNwDayltSavTim;
        }

        AtcAp_ConvertTimeZone(aucUtAndLtz, ucDayLightTime);

        AtcAp_StrPrintf_AtcRspBuf((const char *)"20%d%d/%d%d/%d%d,%d%d:%d%d:%d%d",
            aucUtAndLtz[0] / 10,aucUtAndLtz[0] % 10,
            aucUtAndLtz[1] / 10,aucUtAndLtz[1] % 10,
            aucUtAndLtz[2] / 10,aucUtAndLtz[2] % 10,
            aucUtAndLtz[3] / 10,aucUtAndLtz[3] % 10,
            aucUtAndLtz[4] / 10,aucUtAndLtz[4] % 10,
            aucUtAndLtz[5] / 10,aucUtAndLtz[5] % 10
            );
    }
}
/*******************************************************************************
  MODULE    : AtcAp_OutputUniversalTime
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
void AtcAp_OutputUniversalTime(LNB_NAS_LOCAL_TIME_STRU* pLocalTime)
{
    if (D_ATC_FLAG_TRUE == pLocalTime->ucUtAndLtzFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf 
            + g_AtcApInfo.stAtRspInfo.usRspLen),(const unsigned char *)"20%d%d/%d%d/%d%d,%d%d:%d%d:%d%d",
                pLocalTime->aucUtAndLtz[0] & 0x0F,
                (pLocalTime->aucUtAndLtz[0] & 0xF0) >> 4,
                pLocalTime->aucUtAndLtz[1] & 0x0F,
                (pLocalTime->aucUtAndLtz[1] & 0xF0) >> 4,
                pLocalTime->aucUtAndLtz[2] & 0x0F,
                (pLocalTime->aucUtAndLtz[2] & 0xF0) >> 4,
                pLocalTime->aucUtAndLtz[3] & 0x0F,
                (pLocalTime->aucUtAndLtz[3] & 0xF0) >> 4,
                pLocalTime->aucUtAndLtz[4] & 0x0F,
                (pLocalTime->aucUtAndLtz[4] & 0xF0) >> 4,
                pLocalTime->aucUtAndLtz[5] & 0x0F,
                (pLocalTime->aucUtAndLtz[5] & 0xF0) >> 4
            );
    }
}
/*******************************************************************************
  MODULE    : ATC_OutputTimeZone
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JiangNa    2018/12/18   create
*******************************************************************************/
static void AtcAp_OutputTimeZone_Sub(unsigned char* pucData, unsigned char ucCtzrReport, LNB_NAS_LOCAL_TIME_STRU* pLocalTime)
{
    unsigned char aucDataTmp[] = {"+XYCTZEU"};
    unsigned char ucLocalTimeZone = 0;
    unsigned char ucDayLightTime = 0;

    ucLocalTimeZone = (pLocalTime->ucLocalTimeZone & 0x07) * 10 + ((pLocalTime->ucLocalTimeZone & 0xf0) >> 4);
    if (D_ATC_FLAG_TRUE == pLocalTime->ucNwDayltSavTimFlg)
    {
        ucDayLightTime = (pLocalTime->ucNwDayltSavTim & 3) * 4;
    }

    if (0 == (pLocalTime->ucLocalTimeZone & 0x08))
    {/* bit8: 0-positive 1-negative */
        //ucLocalTimeZone += ucDayLightTime;
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s:+%d%d", pucData,
            (ucLocalTimeZone / 10), (ucLocalTimeZone % 10));
    }
    else
    {
        //ucLocalTimeZone -= ucDayLightTime;
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s:-%d%d", pucData,
            (ucLocalTimeZone / 10), (ucLocalTimeZone % 10));
    }
    if ((2 == ucCtzrReport) || (3 == ucCtzrReport)
        || 0 == AtcAp_Strncmp(pucData, aucDataTmp))
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
            (const unsigned char *)",%d,", ucDayLightTime / 4);
    }

    return;
}

void AtcAp_OutputTimeZone(unsigned char ucCtzrReport, LNB_NAS_LOCAL_TIME_STRU* pLocalTime)
{
    unsigned char aucData[][7] = {"","+CTZV","+CTZE","+CTZEU"};
    AtcAp_OutputTimeZone_Sub(aucData[ucCtzrReport], ucCtzrReport, pLocalTime);
    return;
}

void AtcAp_OutputTimeZone_XY(unsigned char ucCtzrReport, LNB_NAS_LOCAL_TIME_STRU* pLocalTime)
{
    unsigned char aucData[] = {"+XYCTZEU"};
    AtcAp_OutputTimeZone_Sub(aucData, ucCtzrReport, pLocalTime);
    return;
}

void AtcAp_ConvertInDotFormat(char* pStrBuff, unsigned char* pAddr, unsigned char ucLen)
{
    unsigned char index;
    unsigned char ucBuffIdx = 0;

    if(NULL == pAddr)
    {
        return;
    }

    if(0 != strlen(pStrBuff))
    {
        pStrBuff[ucBuffIdx++] = '.';
    }

    for(index = 0; index < ucLen; index++)
    {
        ucBuffIdx += sprintf(pStrBuff + ucBuffIdx, "%d", pAddr[index]);
        if(index != ucLen - 1)
        {
            pStrBuff[ucBuffIdx++] = '.';
        }
    }
}

void AtcAp_OutputAddr_IpDns(unsigned char ucV4V6Fg, unsigned char* pAddr_v4, unsigned char* pAddr_v6)
{
    char   aucAddr_v4_str[20] = { 0 };
    char   aucAddr_v6_str[64] = { 0 };
    
    AtcAp_ConvertInDotFormat(aucAddr_v4_str, pAddr_v4, 4);
    AtcAp_ConvertInDotFormat(aucAddr_v6_str, pAddr_v6, 16);
    
    if(ATC_AP_TRUE == ucV4V6Fg)
    {
        if(0 != strlen(aucAddr_v4_str) || 0 != strlen(aucAddr_v6_str))
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s.%s\"", aucAddr_v4_str, aucAddr_v6_str);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)",\"\"");
        }
    }
    else
    {
        if(NULL != pAddr_v4)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", aucAddr_v4_str);
        }
        else if(NULL != pAddr_v6)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", aucAddr_v6_str);
        }
        else
        {
             AtcAp_StrPrintf_AtcRspBuf((const char *)",\"\"");
        }
    }
}

#ifdef LCS_MOLR_ENABLE
static void Lcs_OutputXML_ElementOlny_Start(unsigned char* pXmlData, char* pElementName)
{
    char  dataStr[100]         = { 0 };
    char  spaceFormat[10]      = { 0 };

    g_AtcApInfo.ucXmlElementLevel++;
    if(0 != g_AtcApInfo.ucXmlElementLevel)
    {
        sprintf(spaceFormat, "%%%ds", g_AtcApInfo.ucXmlElementLevel * 2);
        sprintf(dataStr, spaceFormat, " ");
    }   
    sprintf(dataStr + strlen(dataStr), "<%s>\r\n", pElementName);

    strcat((char*)pXmlData, dataStr);
}

static void Lcs_OutputXML_ElementWithValue_Str(unsigned char* pXmlData, char* pElementName, char* pValue)
{
    char  dataStr[100]         = { 0 };
    char  spaceFormat[10]      = { 0 };

    sprintf(spaceFormat, "%%%ds", (g_AtcApInfo.ucXmlElementLevel + 1) * 2);
    sprintf(dataStr, spaceFormat, " ");   
    sprintf(dataStr + strlen(dataStr), "<%s>%s</%s>\r\n", pElementName, pValue, pElementName);
    
    strcat((char*)pXmlData, dataStr);
}

static void Lcs_OutputXML_ElementWithValue_Float(unsigned char* pXmlData, char* pElementName, float Val)
{
    char  dataStr[100]         = { 0 };
    char  spaceFormat[10]      = { 0 };

    sprintf(spaceFormat, "%%%ds", (g_AtcApInfo.ucXmlElementLevel + 1) * 2);
    sprintf(dataStr, spaceFormat, " ");   
    sprintf(dataStr + strlen(dataStr), "<%s>%f</%s>\r\n", pElementName, Val, pElementName);
    
    strcat((char*)pXmlData, dataStr);
}


static void Lcs_OutputXML_ElementWithValue_Interger(unsigned char* pXmlData, char* pElementName,  signed int Val)
{
    char  dataStr[100]         = { 0 };
    char  spaceFormat[10]      = { 0 };
    
    sprintf(spaceFormat, "%%%ds", (g_AtcApInfo.ucXmlElementLevel + 1) * 2);
    sprintf(dataStr, spaceFormat, " ");   
    sprintf(dataStr + strlen(dataStr), "<%s>%d</%s>\r\n", pElementName, Val, pElementName);
    
    strcat((char*)pXmlData, dataStr);
}

static void Lcs_OutputXML_ElementOlny_End(unsigned char* pXmlData, char* pElementName)
{
    char  dataStr[100]         = { 0 };
    char  spaceFormat[10]      = { 0 };    

    if(0 != g_AtcApInfo.ucXmlElementLevel)
    {
        sprintf(spaceFormat, "%%%ds", g_AtcApInfo.ucXmlElementLevel * 2);
        sprintf(dataStr, spaceFormat, " ");
    }   
    sprintf(dataStr + strlen(dataStr), "</%s>\r\n", pElementName);
    g_AtcApInfo.ucXmlElementLevel--;    

    strcat((char*)pXmlData, dataStr);
}

static void Lcs_OutputXML_CoordinateInfo(unsigned char* pXmlData, LCS_COOR_STRU* pCoordinate)
{
    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_COORDINATE);

    //<latitude>
    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_LATITUDE);
    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_NORTH, pCoordinate->tLatitude.ucLatitudeSign);
    Lcs_OutputXML_ElementWithValue_Float(pXmlData, LCS_XML_ELEMEN_DEGRESS, pCoordinate->tLatitude.uiDegreesLatitude * 90.0 / (1 << 23));
    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_LATITUDE);
    //<longitude>
    Lcs_OutputXML_ElementWithValue_Float(pXmlData, LCS_XML_ELEMEN_LONGITUDE, pCoordinate->iDegreesLongitude * 360.0 / (1 << 24));
     
    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_COORDINATE);
}

static void Lcs_OutputXML_UncertInfo(unsigned char* pXmlData, unsigned char* pElementName, unsigned char ucUncertn)
{
    float   fUncert; 
    char    OutputStr[20] = { 0 };

    fUncert = 10* (pow(1 + 0.1, ucUncertn) - 1);
    sprintf(OutputStr, "%0.1f", fUncert);

    Lcs_OutputXML_ElementWithValue_Str(pXmlData, pElementName, OutputStr);
}

static void Lcs_OutputXML_Confidence(unsigned char* pXmlData, unsigned char ucConfidence)
{
    if(0 == ucConfidence || ucConfidence > 100)
    {
        return;
    }

    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_CONFIDENCE, ucConfidence);
}


static void Lcs_OutputXML_UncertEcliipseInfo(unsigned char* pXmlData, LCS_UNCERTN_ELLIP_STRU* pUncertEllipse, unsigned char ucConfidence)
{
    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_UNCERT_ELLIP);

    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_UNCERT_SEMI_MAJOR, pUncertEllipse->ucUncertnSemiMajor);
    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_UNCERT_SEMI_MIJOR, pUncertEllipse->ucUncertnSemiMin);
    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_ORIENT_MIJOR, pUncertEllipse->ucOrientMajorAxis);
    Lcs_OutputXML_Confidence(pXmlData, ucConfidence);

    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_UNCERT_ELLIP);
}

static void Lcs_OutputXML_Altitude(unsigned char* pXmlData, LCS_ALTI_STRU* pAltitude)
{
    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ALT);

    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_HEIGHT_ABOVE_SURFACE, pAltitude->ucHeightAboveSurface);
    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_HEIGHT, pAltitude->usAlti);

    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ALT);
}

static void Lcs_OutputXML_UncertAlti(unsigned char* pXmlData, unsigned char ucUncertnAlti)
{
    float   val; 
    char    OutputStr[20] = { 0 };

    val = 45 * (pow(1 + 0.025, ucUncertnAlti) - 1);
    sprintf(OutputStr, "%0.2f", val);

    Lcs_OutputXML_ElementWithValue_Str(pXmlData, LCS_XML_ELEMEN_UNCERT_ALT, OutputStr);
}

static void Lcs_OutputXml_ShapeDataInfo(unsigned char* pXmlData, LCS_SHAPE_DATA_STRU* pShapeData)
{
    unsigned char    i;
    if(NULL == pShapeData)
    {
        return;
    }

    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_SHAPE_DATA);   
    switch(pShapeData->ucShapeType)
    {
        case LCS_SHAPE_ELLIP_POINT:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT);
            Lcs_OutputXML_CoordinateInfo(pXmlData, (LCS_COOR_STRU*)&pShapeData->u.tEllipPoint);
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT);
            break;
        case LCS_SHAPE_ELLIP_POINT_UNCERT_CIRCLE:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_UNCERT_CIRCLE);
            Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tEllipPointUncertCircle.tCoordinate);
            Lcs_OutputXML_UncertInfo(pXmlData, LCS_XML_ELEMEN_UNCERT_CIRCLE, pShapeData->u.tEllipPointUncertCircle.ucUncertn);
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_UNCERT_CIRCLE);
            break;
        case LCS_SHAPE_ELLIP_POINT_UNCERT_ELLIP:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_UNCERT_ELLIP);
            Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tEllipPointUncertEllip.tCoordinate);
            Lcs_OutputXML_UncertEcliipseInfo(pXmlData, &pShapeData->u.tEllipPointUncertEllip.tUncertEllipse, pShapeData->u.tEllipPointUncertEllip.ucConfidence);
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_UNCERT_ELLIP);
            break;
        case LCS_SHAPE_POLYGON:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_POLYGON);
            for(i = 0; i < pShapeData->u.tPolygon.ucCnt && i < 8; i++) //i < 8: prevent exceeding the size of pXmlData by 4096 byte
            {
                Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tPolygon.atCoordinate[i]);
            }
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_POLYGON);
            break;
        case LCS_SHAPE_ELLIP_POINT_ALT:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ALT);
            Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tEllipPointWithAlti.tCoordinate);
            Lcs_OutputXML_Altitude(pXmlData, &pShapeData->u.tEllipPointWithAlti.tAltitude);
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ALT);
            break;
        case LCS_SHAPE_ELLIP_POINT_ALT_UNCERT:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ALT_UNCET);
            Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tEllipPointWithAltiUncert.tCoordinate);
            Lcs_OutputXML_Altitude(pXmlData, &pShapeData->u.tEllipPointWithAltiUncert.tAltitude);
            Lcs_OutputXML_UncertEcliipseInfo(pXmlData, &pShapeData->u.tEllipPointWithAltiUncert.tUncertEllipse, pShapeData->u.tEllipPointWithAltiUncert.ucConfidence);
            Lcs_OutputXML_UncertAlti(pXmlData, pShapeData->u.tEllipPointWithAltiUncert.ucUncertnAlti);
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ALT_UNCET);
            break;
        case LCS_SHAPE_ELLIP_ARC:
            Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ARC);
            Lcs_OutputXML_CoordinateInfo(pXmlData, &pShapeData->u.tEllipArc.tCoordinate);
            Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_INNER_RAD, 5 * pShapeData->u.tEllipArc.usInnerRadius);
            Lcs_OutputXML_UncertInfo(pXmlData, LCS_XML_ELEMEN_UNCERT_RAD, pShapeData->u.tEllipPointUncertCircle.ucUncertn);
            Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_OFFSET_ANGLE, 2 * pShapeData->u.tEllipArc.ucOffsetAngle);
            Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_INC_ANGLE, 2 * (pShapeData->u.tEllipArc.ucIncludedAngle + 1));
            Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_ELLIP_POINT_ARC);
            break;
        default:
            break;
    }
    
    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_SHAPE_DATA);
}

static void Lcs_OutputXml_VelocityInfo(unsigned char* pXmlData, LCS_VELOCITY_DATA_STRU* pVelData)
{
    if(NULL == pVelData)
    {
        return;
    }

    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_VEL_DATA);
    
    Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_HOR_VEL, pVelData->tHorizSpeed.usSpeed);
    if(pVelData->OP_VertSpeed)
    {
        Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_VERT_VEL, pVelData->tVertSpeed.ucSpeed);
        Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_VERT_VEL_DIRECT, pVelData->tVertSpeed.ucDirect);
    }
    
    if(pVelData->OP_HorUncert)
    {
        Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_HOR_UNCERT, pVelData->ucHorSpeedUncert);
    }

    if(pVelData->OP_VertUncert)
    {
        Lcs_OutputXML_ElementWithValue_Interger(pXmlData, LCS_XML_ELEMEN_VERT_UNCERT, pVelData->ucVertSpeedUncert);
    }

    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_VEL_DATA);
}


unsigned short Lcs_MolrResult_OutputXML(unsigned char* pXmlData, unsigned short* pSize, LCS_SHAPE_DATA_STRU* pShapeData, LCS_VELOCITY_DATA_STRU* pVelData)
{
    g_AtcApInfo.ucXmlElementLevel = 255;
    
    sprintf((char*)pXmlData, "%s\r\n", LCS_XML_ELEMENT_HEAD);
    Lcs_OutputXML_ElementOlny_Start(pXmlData, LCS_XML_ELEMEN_LCATION_PARAM);
    
    Lcs_OutputXml_ShapeDataInfo(pXmlData, pShapeData);
    Lcs_OutputXml_VelocityInfo(pXmlData, pVelData);

    Lcs_OutputXML_ElementOlny_End(pXmlData, LCS_XML_ELEMEN_LCATION_PARAM);

    return strlen((char*)pXmlData);
}
#endif

void AtcAp_Build_NCell(API_CELL_LIST_STRU* pCellList)
{
    unsigned char  ucIdx = 0;
    unsigned short u16Rsrp;
    unsigned short u16Rsrq;
    unsigned short u16Rssi;
    unsigned short u16Snr;

    for (ucIdx = 1; ucIdx < pCellList->ucCellNum; ucIdx++)
    {
#if Custom_09
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, 
            (const unsigned char *)"%s%d%c%d%s", "\r\nNUESTATS:CELL,", pCellList->aNCell[ucIdx].ulDlEarfcn, ',', pCellList->aNCell[ucIdx].usPhyCellId, ",0,");
#else
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, 
            (const unsigned char *)"%s%d%c%d%s", "NUESTATS:CELL,", pCellList->aNCell[ucIdx].ulDlEarfcn, ',', pCellList->aNCell[ucIdx].usPhyCellId, ",0,");
#endif
        if(0 == pCellList->aNCell[ucIdx].sRsrp)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"%s", "0,");
        }
        else
        {
            u16Rsrp = (~(pCellList->aNCell[ucIdx].sRsrp - 1)) & 0x7FFF;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"%c%d%c", '-', u16Rsrp, ',');
        }

        if(pCellList->aNCell[ucIdx].sRsrq < 0)
        {
            u16Rsrq = (~(pCellList->aNCell[ucIdx].sRsrq - 1)) & 0x7FFF;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%c%d%c", '-', u16Rsrq, ',');
        }
        else
        {
            u16Rsrq = pCellList->aNCell[ucIdx].sRsrq;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%d%c", u16Rsrq, ',');
        }

        if(pCellList->aNCell[ucIdx].sRssi < 0)
        {
            u16Rssi = (~(pCellList->aNCell[ucIdx].sRssi - 1)) & 0x7FFF;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%c%d%c", '-', u16Rssi, ',');
        }
        else
        {
            u16Rssi = pCellList->aNCell[ucIdx].sRssi;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%d%c", u16Rssi, ',');
        }

        if(pCellList->aNCell[ucIdx].sSinr < 0)
        {
            u16Snr = (~(pCellList->aNCell[ucIdx].sSinr - 1)) & 0x7FFF;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%c%d\r\n", '-', u16Snr);
        }
        else
        {
            u16Snr = pCellList->aNCell[ucIdx].sSinr;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)"%d\r\n", u16Snr);
        }
    }
}

unsigned char AtcAp_CurrEventChk_IsWaitSmsPdu()
{
    switch(g_AtcApInfo.usCurrEvent)
    {
        case D_ATC_EVENT_CMGS:
        case D_ATC_EVENT_CMGC:
        case D_ATC_EVENT_CNMA:
            return ATC_AP_TRUE;
        default:
            return ATC_AP_FALSE;
    }
}

/*******************************************************************************
  MODULE    : ATC_CmdFuncInf
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CmdFuncInf(unsigned char *pCommandBuffer, unsigned char ucEventIdx,  unsigned char *pEventBuffer, unsigned char *pucCmdFunc)
{
    *pucCmdFunc = 0xFF;
    if ((*pCommandBuffer == '=') && (*(pCommandBuffer + 1) == '?'))                     /* TEST COMMAND                         */
    {
        if(*(pCommandBuffer + 2) == D_ATC_DEFAULT_CR)
        {
            *pucCmdFunc = D_ATC_CMD_FUNC_TEST;
            ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = ATC_Event_Table[ucEventIdx].ucFucTest;
        }
        else
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else if (*pCommandBuffer == '?')                                                    /* READ COMMAND                         */
    {
        if(*(pCommandBuffer + 1) == D_ATC_DEFAULT_CR)
        {
            *pucCmdFunc = D_ATC_CMD_FUNC_READ;
            ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = ATC_Event_Table[ucEventIdx].ucFucRead;
        }
        else
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else if (*pCommandBuffer == '=')                                                    /* SET COMMAND                          */
    {
        if(*(pCommandBuffer + 1) == D_ATC_DEFAULT_CR)
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
        *pucCmdFunc = D_ATC_CMD_FUNC_SET;
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = ATC_Event_Table[ucEventIdx].ucFucSet;
    }
    else if(*pCommandBuffer == D_ATC_DEFAULT_CR)                                        /* SET COMMAND NO PARAMETER             */
    {
        *pucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;
         ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = ATC_Event_Table[ucEventIdx].ucFucNoQualSet;
    }
    else
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    if(((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent == 0xFF)
    {
        return D_ATC_COMMAND_SYNTAX_ERROR;
    }
    else
    {
        return D_ATC_COMMAND_OK;
    }
}

/*******************************************************************************
  MODULE    : ATC_UlongToBinary
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char AtcAp_UlongToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned long *pBinaryData, unsigned char ucAllowZeroStartFlg)
{
    unsigned long ulNum;                                                                        /* binary data                          */                                                                  /* 0:Positive  1:Negative               */
    unsigned char ucCount;

    ulNum = 0;
    for (ucCount = 0; ucCount < ucLength; ucCount++)
    {   
        if ((*(pInputCharStr + ucCount) >= '0') && (*(pInputCharStr + ucCount) <= '9'))
        {
            if(0 == ucAllowZeroStartFlg && 0 == ulNum && 0 != ucCount)
            {
                return ATC_NG;
            }
            ulNum = (ulNum * 10);
            ulNum += (*(pInputCharStr + ucCount) - '0');
        }
        else
        {
            return ATC_NG;
        }
    }

    *pBinaryData = ulNum;                                                               /* binary data set                      */
    return ATC_OK;
}


/*******************************************************************************
  MODULE    : ATC_CheckLongNumParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CheckLongNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
                                unsigned long *pBinaryData, unsigned char *pStopStrInf, unsigned char ucAllowZeroStartFlg)
{
    unsigned char ucResultData;
    unsigned char ucCount;
    unsigned char ucJudgucResult;

    if (D_ATC_N_CR == *pCommandBuffer)                                                  /* check <CR>                           */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)                                      /* check ';'                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)                                          /* check ','                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if ((((*pCommandBuffer) < '0') || ((*pCommandBuffer) > '9')) && ((*pCommandBuffer) != '-'))  /* check other str    */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_STR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else
    {
        *pLength = 0;                                                                   /* Initial period                       */
        for (ucCount = 1; ; ucCount++)
        {
            if (D_ATC_N_CR == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if (D_ATC_N_SEMICOLON == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
                break;
            }
            else if (D_ATC_N_COMMA == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
                break;
            }
            else if (((*(pCommandBuffer + ucCount) < '0') || (*(pCommandBuffer + ucCount) > '9'))
                      && (*(pCommandBuffer + ucCount) != '-'))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else if (ucCount >= uiFigure)
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else
            {
            }
        }
        *pBinaryData = 0;
        ucJudgucResult = AtcAp_UlongToBinary(pCommandBuffer, ucCount, pBinaryData, ucAllowZeroStartFlg);
        if (ATC_OK == ucJudgucResult)
        {
            *pLength = (unsigned char)(*pLength + ucCount);
            ucResultData = D_ATC_PARAM_OK;
        }
        else
        {
            ucResultData = D_ATC_PARAM_ERROR;
        }
    }

    return ucResultData;
}


unsigned char ATC_GetDecimalParameterByte(unsigned char *pCommandBuffer, 
                                          unsigned short *pOffSet, 
                                          unsigned int uiFigure, 
                                          unsigned char *pData, 
                                          unsigned char *pDataFlag,
                                          unsigned char ucMin,
                                          unsigned char ucMax, 
                                          unsigned char ucOptFlag,
                                          unsigned char ucLastParameter)
{
    unsigned char ucParResult;                                                                  /* parameter result                     */                                           
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned long ulBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucResultData = D_ATC_COMMAND_OK;

    ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + (*pOffSet)), uiFigure,
                               &ucParLen, &ulBinaryData, &ucStopStrInf, 0);

    (*pOffSet) += ucParLen;   /* parameter length set                 */
    
    if(D_ATC_PARAM_OK == ucParResult)
    {
        if ((ulBinaryData >= ucMin) && (ulBinaryData <= ucMax) && (ulBinaryData <= 255)) 
        {
            *pData = (unsigned char)ulBinaryData;   
            if(pDataFlag != NULL)
            {
                *pDataFlag = D_ATC_FLAG_TRUE;
            }            
        }
        else                         
        {
            ucResultData =  D_ATC_COMMAND_PARAMETER_ERROR;     
        }                
    }
    else if(ucOptFlag == 0 && D_ATC_PARAM_EMPTY == ucParResult)/* Mandetory Option */
    {
        ucResultData =  D_ATC_COMMAND_SYNTAX_ERROR;  
    }
    else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
    {
        ucResultData =  D_ATC_COMMAND_SYNTAX_ERROR;  
    } 
    else if(D_ATC_PARAM_ERROR == ucParResult)
    {
        ucResultData =  D_ATC_COMMAND_PARAMETER_ERROR;  
    }
    else
    {
        /* Option  do nothing */
    }
    if(D_ATC_STOP_CHAR_CR != ucStopStrInf && ucResultData == D_ATC_COMMAND_OK)
    {
        if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
        {
            if(ucLastParameter == 1)
            {
                ucResultData = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else
            {
                (*pOffSet) += 1;   /*  ',' set  */
            }
        }
        else
        {
            ucResultData = D_ATC_COMMAND_SYNTAX_ERROR;
        }
    }    
    return ucResultData;
}

/*******************************************************************************
  MODULE    : ATC_GetStrLen
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned int ATC_GetStrLen(unsigned char *pStr)
{
    unsigned int counter = 0;
    
    while (pStr != NULL)
    {
        if (*(pStr++) == 0x00)
        {
            break;
        }
        else
        {
            counter++;
        }
    }
    
    return counter;
}


/*******************************************************************************
  MODULE    : ATC_Strncpy
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
void ATC_Strncpy(unsigned char *pStr1, unsigned char *pStr2, unsigned short usCount)
{
    unsigned short i;

    for (i = 0; i < usCount; i++)
    {
        *(pStr1 + i) = *(pStr2 + i);
    }
    return ; 
}

/*******************************************************************************
  MODULE    : ATC_CheckNumParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CheckNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
                            unsigned char *pBinaryData, unsigned char *pStopStrInf)
{
    unsigned char ucResultData;
    unsigned char ucCount;
    unsigned char ucJudgucResult;

    if (D_ATC_N_CR == *pCommandBuffer)                                                  /* check <CR>                           */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)                                      /* check ';'                            */
    {                                                                                                                           
        *pLength = 0;                                                                                                           
        *pBinaryData = 0;                                                                                                       
        *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;                                                                               
        ucResultData = D_ATC_PARAM_EMPTY;                                                                                       
    }                                                                                                                           
    else if (D_ATC_N_COMMA == *pCommandBuffer)                                          /* check ','                            */
    {                                                                                                                           
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if ((((*pCommandBuffer) < '0') || ((*pCommandBuffer) > '9')) && ((*pCommandBuffer) != '-'))  /* check other str         */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_STR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else
    {
        *pLength = 0;                                                                   /* Initial period                       */
        for (ucCount = 1; ; ucCount++)
        {
            if (D_ATC_N_CR == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if (D_ATC_N_SEMICOLON == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
                break;
            }
            else if (D_ATC_N_COMMA == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
                break;
            }
            else if (((*(pCommandBuffer + ucCount) < '0') || (*(pCommandBuffer + ucCount) > '9'))
                      && (*(pCommandBuffer + ucCount) != '-'))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else if (ucCount >= uiFigure)
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else
            {
            }
        }
        *pBinaryData = 0;
        ucJudgucResult = ATC_CharToBinary(pCommandBuffer, ucCount, pBinaryData, 0);
        if (ATC_OK == ucJudgucResult)
        {
            *pLength = (unsigned char)(*pLength + ucCount);
            ucResultData = D_ATC_PARAM_OK;
        }
        else
        {
            ucResultData = D_ATC_PARAM_ERROR;
        }
    }
    return ucResultData;
}

/*******************************************************************************
  MODULE    : ATC_CheckHexParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  GCM   2018.12.11   create
*******************************************************************************/
unsigned char ATC_CheckHexParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
                            unsigned long *pBinaryData, unsigned char *pStopStrInf)
{
    unsigned char ucResultData;
    unsigned char i;
    unsigned char *pTmpCommandBuffer = NULL;
    unsigned char ucOffset           = 0;

    if (D_ATC_N_CR == *pCommandBuffer)                                                  /* check <CR>                           */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)                                      /* check ';'                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)                                          /* check ','                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else
    {
        if (((*pCommandBuffer) == '0') && (((*(pCommandBuffer+1)) == 'x') || ((*(pCommandBuffer+1)) == 'X')))
        {
            ucOffset = 2;
        }
        pTmpCommandBuffer = pCommandBuffer + ucOffset;
        
        for(i = 0; ; i++)
        {
            if(D_ATC_N_COMMA == *(pTmpCommandBuffer))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
                break;
            }
            else if(D_ATC_N_CR == *(pTmpCommandBuffer))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if (((*(pTmpCommandBuffer) >= '0') && (*(pTmpCommandBuffer) <= '9'))
                || ((*(pTmpCommandBuffer) >= 'A') && (*(pTmpCommandBuffer) <= 'F'))
                || ((*(pTmpCommandBuffer) >= 'a') && (*(pTmpCommandBuffer) <= 'f')))
            {
                //no process;
                if ((*(pTmpCommandBuffer) >= '0') && (*(pTmpCommandBuffer) <= '9'))
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - '0';
                }
                else if ((*(pTmpCommandBuffer) >= 'A') && (*(pTmpCommandBuffer) <= 'F'))
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - 'A' + 10;
                }
                else
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - 'a' + 10;
                }
                //end
            }
            else
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;                                     /* end of character :"CR" error         */
                ucResultData = D_ATC_PARAM_SYNTAX_ERROR;                                /* result error set                     */
                return ucResultData;  
            }

            pTmpCommandBuffer++;
        }
        if(i > uiFigure || (ucOffset != 0 && i == 0))
        {
            ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
            return ucResultData;                                                        /* error stop                           */
        }
        *pLength = i + ucOffset;
        *pBinaryData = 0;
        for(i = 0; i < *pLength - ucOffset; i++)
        {
            *pBinaryData = (*pBinaryData) * 16;
            *pBinaryData = (*pBinaryData) + pCommandBuffer[i + ucOffset];
        }
        ucResultData = D_ATC_PARAM_OK;
    }

    return ucResultData;
}
/*******************************************************************************
  MODULE    : ATC_CheckStrParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CheckStrParameter(unsigned char *pCommandBuffer,  signed int iFigure, unsigned char *pLength ,
                            unsigned char *pStrLength, unsigned char *pParaStr, unsigned char *pStopStrInf)
{
    unsigned char ucStopChar;
    unsigned char ucResultData;
     signed int iPalamCounter;
     signed int iCpyCounter;

    ucStopChar = D_ATC_STOP_CHAR_STR;
    ucResultData = D_ATC_PARAM_OK;
    *pLength = 0;
    *pStrLength = 0;

    if (D_ATC_N_CR == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else
    {
        if (*pCommandBuffer == '"')
        {
            for(iPalamCounter = 1; iPalamCounter <= (iFigure + 1); iPalamCounter++)
            {
                if(*(pCommandBuffer + iPalamCounter) == D_ATC_N_COMMA || *(pCommandBuffer + iPalamCounter) == D_ATC_N_CR)
                {
                    break;
                }
            }
            if(iPalamCounter < 2 || iPalamCounter > (iFigure + 2))
            {
                ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
                return ucResultData;                                                        /* error stop                           */
            }
            if(*(pCommandBuffer + iPalamCounter - 1) != '"' 
                || (*(pCommandBuffer + iPalamCounter) != D_ATC_N_COMMA && *(pCommandBuffer + iPalamCounter) != D_ATC_N_CR))
            {
                ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
                return ucResultData;
            }
            
            (*pStrLength)  = (unsigned char)(iPalamCounter - 2);                                    /* check parameter length               */
            for (iCpyCounter = 0; iCpyCounter < (iPalamCounter - 2); iCpyCounter++) 
            {
                pParaStr[iCpyCounter] = pCommandBuffer[iCpyCounter + 1];                    /* store parameter address              */
            }
            //pParaStr[iCpyCounter] = '\0';                                                   /* NULL  set                            */
            *pLength  = (unsigned char)(iPalamCounter);                                          /* parameter length set                 */
            if(*(pCommandBuffer + *pLength) == D_ATC_N_CR)
            {
                ucStopChar = D_ATC_STOP_CHAR_CR;                                            /* "CR" set                             */
            }
            else if(*(pCommandBuffer + *pLength) == D_ATC_N_SEMICOLON)
            {
                ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;                                     /* ";"  set                             */
            }
            else if(*(pCommandBuffer + *pLength) == D_ATC_N_COMMA)
            {
                ucStopChar = D_ATC_STOP_CHAR_KANMA;                                         /* ","  set                             */
            }
            else
            {
                ucStopChar = D_ATC_STOP_CHAR_STR;                                           /* other character set                  */
            }
        }
        else
        {
            for(iPalamCounter = 1; iPalamCounter <= iFigure; iPalamCounter++)
            {
                if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_CR )
                {
                    ucStopChar = D_ATC_STOP_CHAR_CR;
                    break;
                }
                else if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_COMMA )
                {
                    ucStopChar = D_ATC_STOP_CHAR_KANMA;
                    break;
                }
            }
            if (iPalamCounter > iFigure)
            {
                ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
                return ucResultData;
            }
            else
            {
                (*pStrLength)  = (unsigned char)iPalamCounter;                                         /* check parameter length                   */
                for ( iCpyCounter = 0; iCpyCounter < iPalamCounter ; iCpyCounter++ )
                {
                    pParaStr[iCpyCounter] = pCommandBuffer[iCpyCounter];                        /* store parameter character                */
                }
                //pParaStr[iCpyCounter] = '\0';                                                   /* NULL set                                 */
                *pLength  = (unsigned char)iPalamCounter;                                               /* parameter length set                 */
            }
        }
    }
    (*pStopStrInf) = ucStopChar;

    return ucResultData;
}

  unsigned char ATC_GetDecimalParameterLong(unsigned char *pCommandBuffer, 
                                            unsigned short *pOffSet, 
                                            unsigned int uiFigure, 
                                            unsigned long *pData, 
                                            unsigned char *pDataFlag,
                                            unsigned long Min,
                                            unsigned long Max, 
                                            unsigned char ucOptFlag,
                                            unsigned char ucLastParameter,
                                            unsigned char ucAllowZeroStartFlg)
  {
      unsigned char ucParResult;                                                                  /* parameter result                     */                                           
      unsigned char ucParLen = 0;                                                                 /* parameter length                     */
      unsigned long ulBinaryData;
      unsigned char ucStopStrInf;
      unsigned char ucResultData = D_ATC_COMMAND_OK;
  
      ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + (*pOffSet)), uiFigure,
                                 &ucParLen, &ulBinaryData, &ucStopStrInf, ucAllowZeroStartFlg);
  
      (*pOffSet) += ucParLen;   /* parameter length set                 */
      
      if(D_ATC_PARAM_OK == ucParResult)
      {
          if ((ulBinaryData >= Min) && (ulBinaryData <= Max)) 
          {
              *pData = ulBinaryData;   
              if(pDataFlag != NULL)
              {
                  *pDataFlag = D_ATC_FLAG_TRUE;
              }            
          }
          else                         
          {
              ucResultData =  D_ATC_COMMAND_PARAMETER_ERROR;     
          }                
      }
      else if(ucOptFlag == 0 && D_ATC_PARAM_EMPTY == ucParResult)/* Mandetory Option */
      {
          ucResultData =  D_ATC_COMMAND_SYNTAX_ERROR;  
      }
      else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
      {
          ucResultData =  D_ATC_COMMAND_SYNTAX_ERROR;  
      } 
      else if(D_ATC_PARAM_ERROR == ucParResult)
      {
          ucResultData =  D_ATC_COMMAND_PARAMETER_ERROR;  
      }
      else
      {
          /* Option  do nothing */
      }
      if(D_ATC_STOP_CHAR_CR != ucStopStrInf && ucResultData == D_ATC_COMMAND_OK)
      {
          if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
          {
              if(ucLastParameter == 1)
              {
                  ucResultData = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
              }
              else
              {
                  (*pOffSet) += 1;   /*  ',' set  */
              }
          }
          else
          {
              ucResultData = D_ATC_COMMAND_SYNTAX_ERROR;
          }
      }    
      return ucResultData;            
  }

 unsigned char ATC_GetStrParameter(unsigned char *pCommandBuffer, unsigned short *offset, unsigned short usMaxLen, unsigned char *pParamLen, unsigned char *pParaStr, unsigned char ucOptFlg, unsigned char ucLastParameter)
 {
    unsigned char   ucResult;
    unsigned char   ucParLen   =  0;
    unsigned char   ucStopStrInf;
    
    ucResult = ATC_CheckStrParameter(pCommandBuffer + *offset, usMaxLen, &ucParLen, pParamLen, pParaStr, &ucStopStrInf);
    if(D_ATC_PARAM_ERROR == ucResult || D_ATC_PARAM_SYNTAX_ERROR == ucResult)
    {
        return ucResult;
    }

    if(0 == ucOptFlg)
    {
        if(D_ATC_PARAM_EMPTY == ucResult || 0 == *pParamLen)
        {
            return D_ATC_PARAM_ERROR;
        }
    }
    
    *offset += ucParLen;   
    if(ucStopStrInf != D_ATC_STOP_CHAR_CR) //"\r"
    {
        if(1 == ucLastParameter)
        {
            return D_ATC_PARAM_ERROR;
        }

        if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA) //","
        {
            *offset += 1;
        }
    }
    
    return D_ATC_PARAM_OK;
}

unsigned char ATC_GetHexStrParameter(unsigned char *pCommandBuffer, unsigned short *pOffset, unsigned short usMaxLen, unsigned short *pDataLen, unsigned char *pData, unsigned char ucOptFlg, unsigned char ucLastParameter)
{
    unsigned short   usParLen;
    unsigned char    ucStopStrInf;
    
    *pDataLen = 0;
    if(D_ATC_PARAM_ERROR == ATC_CheckHexStrParameter(pCommandBuffer + *pOffset, usMaxLen, &usParLen, pDataLen, pData, &ucStopStrInf))
    {
        return D_ATC_PARAM_ERROR;
    }

    if(0 == ucOptFlg && 0 == *pDataLen)
    {
        return D_ATC_PARAM_ERROR;
    }

    if(1 == ucLastParameter && D_ATC_STOP_CHAR_CR != ucStopStrInf)
    {
        return D_ATC_PARAM_ERROR;
    }

    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
    {
        usParLen++;
    }

    *pOffset = (*pOffset) +  usParLen;
    
    return D_ATC_PARAM_OK;
}

unsigned char ATC_ChkStrPara(unsigned char *pCommandBuffer , signed int iFigure ,unsigned short *pStrLength, unsigned char *pParaStr ,unsigned char *pStopStrInf)
{
    unsigned char ucStopChar;
    unsigned char ucResult;
     signed int iPalamCounter;
     signed int iCpyCounter;

    ucStopChar = D_ATC_STOP_CHAR_STR;
    ucResult = D_ATC_PARAM_OK;

    if (D_ATC_N_CR == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_CR;
        ucResult = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;
        ucResult = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_KANMA;
        ucResult = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else
    {
        /* AUGUSTA-2G-BUG-209 Dep2_066 2010/04/29 M S */
        for(iPalamCounter = 1; iPalamCounter < iFigure; iPalamCounter++)
        /* AUGUSTA-2G-BUG-209 Dep2_066 2010/04/29 M E */
        {
            if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_CR )
            {
                ucStopChar = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_COMMA )
            {
                ucStopChar = D_ATC_STOP_CHAR_KANMA;
                break;
            }
        }

        /* AUGUSTA-2G-BUG-209 Dep2_066 2010/04/29 M S */
        if (iPalamCounter == iFigure)
        {
            ucStopChar = D_ATC_STOP_CHAR_STR;
            ucResult = D_ATC_PARAM_EMPTY;
            pParaStr[0] = '\0';
        }
        else
        {
            (*pStrLength)  = (unsigned short)iPalamCounter;                                         /* check parameter length                   */
            for ( iCpyCounter = 0; iCpyCounter < iPalamCounter ; iCpyCounter++ )
            {
                pParaStr[iCpyCounter] = pCommandBuffer[iCpyCounter];                        /* store parameter character                */
            }
            pParaStr[iCpyCounter] = '\0';                                                   /* NULL set                                 */
        }
        /* AUGUSTA-2G-BUG-209 Dep2_066 2010/04/29 M E */
    }
    (*pStopStrInf) = ucStopChar;

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_ShortToBinary
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_ShortToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned short *pBinaryData)
{
    unsigned short usNum;                                                                       /* binary data                          */
    unsigned char ucSymbolFlg;                                                                  /* 0:Positive  1:Negative               */
    unsigned char ucCount;

    ucSymbolFlg = 0;                                                                    /* Initial period                       */

    if ((*pInputCharStr) == '-')
    {
        pInputCharStr = pInputCharStr + 1;
        ucSymbolFlg = 1;                                                                /* negative number set                  */
        if (ucLength < 2)
        {
            return  ATC_NG;
        }
        else
        {
            ucLength--;
        }
    }

    usNum = 0;
    for (ucCount = 0; ucCount < ucLength; ucCount++)
    {   
        if ((*(pInputCharStr + ucCount) >= '0') && (*(pInputCharStr + ucCount) <= '9'))
        {
            usNum = (usNum * 10);
            usNum += (*(pInputCharStr + ucCount) - '0');
        }
        else
        {
            return ATC_NG;
        }
    }
    if (1 == ucSymbolFlg)
    {
        usNum = usNum * (-1);                                                           /*lint !e732*/
    }

    *pBinaryData = usNum;                                                               /* binary data set                          */
    return ATC_OK;
}

//shao add for USAT
/*******************************************************************************
  MODULE    : ATC_HexToBinary
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
static unsigned char ATC_HexToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned short *pBinaryData)
{
    unsigned short usNum = 0;                                                                       /* binary data                          */
    unsigned char  ucCount;

    usNum = 0;
    for (ucCount = 0; ucCount < ucLength; ucCount++)
    {   
        if ((*(pInputCharStr + ucCount) >= '0') && (*(pInputCharStr + ucCount) <= '9'))
        {
            usNum = (unsigned short)(usNum * 16);
            usNum = (unsigned short)(usNum + (*(pInputCharStr + ucCount) - '0'));
        }
        else if((*(pInputCharStr + ucCount) >= 'a') && (*(pInputCharStr + ucCount) <= 'f'))
        {
            usNum = (unsigned short)(usNum * 16);
            usNum = (unsigned short)(usNum + (*(pInputCharStr + ucCount) - 'a'+10));
        }
        else if((*(pInputCharStr + ucCount) >= 'A') && (*(pInputCharStr + ucCount) <= 'F'))
        {
            usNum = (unsigned short)(usNum * 16);
            usNum = (unsigned short)(usNum + (*(pInputCharStr + ucCount) - 'A'+10));
        }
        else
        {
            return ATC_NG;
        }
    }

    *pBinaryData = usNum;                                                               /* binary data set                      */
    return ATC_OK;
}
/*******************************************************************************
  MODULE    : ATC_CheckHexNumParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CheckHexNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
                                       unsigned short *pBinaryData, unsigned char *pStopStrInf)
{
    unsigned char ucResultData = D_ATC_PARAM_ERROR;
    unsigned char ucCount = 0;
    unsigned char ucJudgucResult = ATC_NG;
    unsigned char *pTmpCommandBuffer = NULL;
    unsigned char ucCheckFlg = 0;

    if (D_ATC_N_CR == *pCommandBuffer)                                                  /* check <CR>                           */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)                                      /* check ';'                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)                                          /* check ','                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_STR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (((*pCommandBuffer) == '0') && (((*(pCommandBuffer+1)) == 'x') || ((*(pCommandBuffer+1)) == 'X')))\
    {
        pTmpCommandBuffer = pCommandBuffer +2;
        ucCheckFlg=1;
    }
    else if (((*(pCommandBuffer) >= '0') && (*(pCommandBuffer) <= '9'))
        || ((*(pCommandBuffer) >= 'A') && (*(pCommandBuffer) <= 'F'))
        || ((*(pCommandBuffer) >= 'a') && (*(pCommandBuffer) <= 'f')))
    {

        pTmpCommandBuffer = pCommandBuffer;
        ucCheckFlg=1;
        
    }
    else if ((((*(pCommandBuffer+2) >= '0') && (*(pCommandBuffer+2) <= '9'))
            || ((*(pCommandBuffer+2) >= 'A') && (*(pCommandBuffer+2) <= 'F'))
            || ((*(pCommandBuffer+2) >= 'a') && (*(pCommandBuffer+2) <= 'f')))
            && (((*(pCommandBuffer+3) >= '0') && (*(pCommandBuffer+3) <= '9'))
            || ((*(pCommandBuffer+3) >= 'A') && (*(pCommandBuffer+3) <= 'F'))
            || ((*(pCommandBuffer+3) >= 'a') && (*(pCommandBuffer+3) <= 'f'))))

    {  /* check other str        */
        *pBinaryData = 0;
        ucJudgucResult = ATC_HexToBinary(pCommandBuffer+2, 2, pBinaryData);
        if (ATC_OK == ucJudgucResult)
        {
            *pLength = 4;
            ucResultData = D_ATC_PARAM_OK;
        }
        else
        {
            ucResultData = D_ATC_PARAM_SYNTAX_ERROR;
        }
    }
    else
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_STR;
        ucResultData = D_ATC_PARAM_SYNTAX_ERROR;
    }
    if (ucCheckFlg)
    {
        *pLength = 0;                                                                   /* Initial period                       */
        for (ucCount = 1; ; ucCount++)
        {
            if (D_ATC_N_CR == *(pTmpCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if (D_ATC_N_SEMICOLON == *(pTmpCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
                break;
            }
            else if (D_ATC_N_COMMA == *(pTmpCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
                break;
            }
            else if (((*(pTmpCommandBuffer) >= '0') && (*(pTmpCommandBuffer) <= '9'))
                || ((*(pTmpCommandBuffer) >= 'A') && (*(pTmpCommandBuffer) <= 'F'))
                || ((*(pTmpCommandBuffer) >= 'a') && (*(pTmpCommandBuffer) <= 'f')))
            {

            }
            else if (ucCount >= uiFigure)
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else
            {
                break;
            }
        }
        *pBinaryData = 0;
        ucJudgucResult = ATC_HexToBinary(pTmpCommandBuffer, ucCount, pBinaryData);
        if (ATC_OK == ucJudgucResult)
        {
            *pLength = (unsigned char)(*pLength + ucCount);
            ucResultData = D_ATC_PARAM_OK;
        }
        else
        {
            ucResultData = D_ATC_PARAM_SYNTAX_ERROR;
        }
    }


    return ucResultData;
}

unsigned char ATC_CheckHexStrParameter(unsigned char *pCommandBuffer,  signed int iFigure, unsigned short *pStrLength ,
    unsigned short *pLength, unsigned char *pParaStr, unsigned char *pStopStrInf)
{
    unsigned char ucStopChar = D_ATC_STOP_CHAR_STR;
    unsigned char ucResultData = D_ATC_PARAM_OK;
     signed int iPalamCounter = 0;
     signed int iCpyCounter = 0;
    unsigned char *pTmpCommandBuffer = NULL;
    unsigned char  ucQuotationFlg    = ATC_FALSE;

    *pStrLength = 0;
    if (D_ATC_N_CR == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
        //pParaStr[0] = '\0';
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
        //pParaStr[0] = '\0';
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
        //pParaStr[0] = '\0';
    }
    else
    {
        if(D_ATC_N_QUOTATION == *pCommandBuffer) 
        {
            ucQuotationFlg = ATC_TRUE;
            for(iPalamCounter = 1; ; iPalamCounter++)
            {
                if(D_ATC_N_QUOTATION == *(pCommandBuffer + iPalamCounter))
                {
                    *(pCommandBuffer + iPalamCounter - 1) = *(pCommandBuffer + iPalamCounter + 1);
                    break;
                }

                if(D_ATC_N_COMMA == *(pCommandBuffer + iPalamCounter) || D_ATC_N_CR == *(pCommandBuffer + iPalamCounter))
                {
                    return  D_ATC_PARAM_ERROR;
                }
                
                *(pCommandBuffer + iPalamCounter - 1) =  *(pCommandBuffer + iPalamCounter);
            }
        }

        pTmpCommandBuffer = pCommandBuffer;
        for(iPalamCounter = 0; ; iPalamCounter++)
        {
            if(D_ATC_N_COMMA == *(pTmpCommandBuffer))
            {
                break;
            }
            if(D_ATC_N_CR == *(pTmpCommandBuffer))
            {
                break;
            }
            if (((*(pTmpCommandBuffer) >= '0') && (*(pTmpCommandBuffer) <= '9'))
                || ((*(pTmpCommandBuffer) >= 'A') && (*(pTmpCommandBuffer) <= 'F'))
                || ((*(pTmpCommandBuffer) >= 'a') && (*(pTmpCommandBuffer) <= 'f')))
            {
                //no process;
                if ((*(pTmpCommandBuffer) >= '0') && (*(pTmpCommandBuffer) <= '9'))
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - '0';
                }
                else if ((*(pTmpCommandBuffer) >= 'A') && (*(pTmpCommandBuffer) <= 'F'))
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - 'A' + 10;
                }
                else
                {
                    *(pTmpCommandBuffer) = *(pTmpCommandBuffer) - 'a' + 10;
                }
                //end
            }
            else
            {
                ucStopChar = D_ATC_STOP_CHAR_STR;                                        /* end of character :"CR" error         */
                (*pStopStrInf) = ucStopChar;
                ucResultData = D_ATC_PARAM_ERROR;                                        /* result error set                     */
                return ucResultData;  
            }

            pTmpCommandBuffer++;
        }
        if(iPalamCounter > iFigure || 0 != iPalamCounter % 2)
        {
            ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
            return ucResultData;                                                        /* error stop                           */
        }

        if(ATC_TRUE == ucQuotationFlg)
        {
            *pStrLength = (unsigned short)(iPalamCounter + 2);
        }
        else
        {
            *pStrLength = iPalamCounter;
        }

        for (iCpyCounter = 0 ; iCpyCounter < (iPalamCounter/2); iCpyCounter++) 
        {
            pParaStr[iCpyCounter] = pCommandBuffer[2*iCpyCounter]*16 + pCommandBuffer[2*iCpyCounter+1];
        }
        *pLength  = iPalamCounter / 2;                                          /* parameter length set                 */
        
        if(*(pCommandBuffer + *pStrLength) == D_ATC_N_CR)
        {
            ucStopChar = D_ATC_STOP_CHAR_CR;                                            /* "CR" set                             */
        }
        else if(*(pCommandBuffer + *pStrLength) == D_ATC_N_SEMICOLON)
        {
            ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;                                     /* ";"  set                             */
        }
        else if(*(pCommandBuffer + *pStrLength) == D_ATC_N_COMMA)
        {
            ucStopChar = D_ATC_STOP_CHAR_KANMA;                                         /* ","  set                             */
        }
        else
        {
            ucStopChar = D_ATC_STOP_CHAR_STR;                                           /* other character set                  */
        }
    }
    (*pStopStrInf) = ucStopChar;

    return ucResultData;
}

/*******************************************************************************
  MODULE    : ATC_CharToBinary
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CharToBinary(unsigned char *pInputCharStr ,unsigned char ucLength ,unsigned char *pBinaryData, unsigned char ucSignedFlg)
{
    unsigned char ucNum;                                                                        /* binary data                          */
    unsigned char ucSymbolFlg;                                                                  /* 0:Positive  1:Negative               */
    unsigned char ucCount;
    unsigned long ulTemp = 0;

    ucSymbolFlg = 0;                                                                    /* Initial period                       */

    if ((*pInputCharStr) == '-')
    {
        pInputCharStr = pInputCharStr + 1;
        ucSymbolFlg = 1;                                                                /* negative number set                  */
        if (ucLength < 2)
        {
            return  ATC_NG;
        }
        else
        {
            ucLength = (unsigned char)(ucLength - 1);
        }
    }

    ucNum = 0;
    for (ucCount = 0; ucCount < ucLength; ucCount++)
    {   
        if ((*(pInputCharStr + ucCount) >= '0') && (*(pInputCharStr + ucCount) <= '9'))
        {
            if(0 == ulTemp && 0 != ucCount)
            {
                return ATC_NG;
            }
            ulTemp = ulTemp * 10;
            ulTemp = (ulTemp + *(pInputCharStr + ucCount) - '0');
        }
        else
        {
            return ATC_NG;
        }
    }

    if(0 == ucSignedFlg)
    {
        if(1 == ucSymbolFlg || ulTemp > 255)
        {
            return ATC_NG;
        }
    }
    else // -128 ~ 127
    {
        if((1 == ucSymbolFlg && ulTemp > 128) || (0 == ucSymbolFlg && ulTemp > 127))
        {
            return ATC_NG;
        }
    }
    
    ucNum = ulTemp;
    if (1 == ucSymbolFlg)
    {
        ucNum = (unsigned char)(ucNum * (-1));
    }

    *pBinaryData = ucNum;                                                               /* binary data set                      */
    return ATC_OK;
}

/*******************************************************************************
  MODULE    : ATC_UshortToBinary
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_UshortToBinary(unsigned char *pInputCharStr, unsigned char ucLength, unsigned short *pBinaryData)
{
    unsigned short usNum;                                                                       /* binary data                          */
    unsigned char  ucSymbolFlg;                                                                 /* 0:Positive  1:Negative               */
    unsigned char  ucCount;

    ucSymbolFlg = 0;                                                                    /* Initial period                       */

    if ((*pInputCharStr) == '-')
    {
        pInputCharStr = pInputCharStr + 1;
        ucSymbolFlg = 1;                                                                /* negative number set                  */
        if (ucLength < 2)
        {
            return  ATC_NG;
        }
        else
        {
            ucLength = (unsigned char)(ucLength - 1);
        }
    }

    usNum = 0;
    for (ucCount = 0; ucCount < ucLength; ucCount++)
    {   
        if ((*(pInputCharStr + ucCount) >= '0') && (*(pInputCharStr + ucCount) <= '9'))
        {
            usNum = (unsigned short)(usNum * 10);
            usNum = (unsigned short)(usNum + (*(pInputCharStr + ucCount) - '0'));
        }
        else
        {
            return ATC_NG;
        }
    }
    if (1 == ucSymbolFlg)
    {
        usNum = (unsigned short)(usNum * (-1));
    }

    *pBinaryData = usNum;                                                               /* binary data set                      */
    return ATC_OK;
}

/*******************************************************************************
  MODULE    : ATC_CheckUSStrParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JiangNa   2018.08.27   create
*******************************************************************************/
unsigned char ATC_CheckUSStrParameter(unsigned char *pCommandBuffer,  signed int iFigure, unsigned short *pLength ,
                            unsigned short *pStrLength, unsigned char *pParaStr, unsigned char *pStopStrInf)
{
    unsigned char ucStopChar;
    unsigned char ucResultData;
     signed int iPalamCounter;
     signed int iCpyCounter;

    ucStopChar = D_ATC_STOP_CHAR_STR;
    ucResultData = D_ATC_PARAM_OK;
    *pLength = 0;

    if (D_ATC_N_CR == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)
    {
        ucStopChar = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
        pParaStr[0] = '\0';
    }
    else
    {
        if (*pCommandBuffer == '"')
        {
            for(iPalamCounter = 1; iPalamCounter <= (iFigure + 1); iPalamCounter++)
            {
                if(*(pCommandBuffer + iPalamCounter) == '"')
                {
                    break;
                }
                else if(*(pCommandBuffer + iPalamCounter) == D_ATC_N_CR)
                {
                    ucStopChar = D_ATC_STOP_CHAR_CR;                                        /* end of character :"CR" error         */
                    (*pStopStrInf) = ucStopChar;
                    ucResultData = D_ATC_PARAM_SYNTAX_ERROR;                                /* result error set                     */
                    return ucResultData;                                                    /* error stop                           */
                }
                else
                {
                    /* no process */
                }
            }
            if(iPalamCounter > (iFigure + 1))
            {
                ucResultData = D_ATC_PARAM_ERROR;                                           /* result error set                     */
                return ucResultData;                                                        /* error stop                           */
            }
            (*pStrLength)  = (unsigned short)(iPalamCounter - 1);                                    /* check parameter length               */
            for (iCpyCounter = 0; iCpyCounter < (iPalamCounter - 1); iCpyCounter++) 
            {
                pParaStr[iCpyCounter] = pCommandBuffer[iCpyCounter + 1];                    /* store parameter address              */
            }
            pParaStr[iCpyCounter] = '\0';                                                   /* NULL  set                            */
            *pLength  = (unsigned short)(iPalamCounter +1);                                          /* parameter length set                 */
            if(*(pCommandBuffer + *pLength) == D_ATC_N_CR)
            {
                ucStopChar = D_ATC_STOP_CHAR_CR;                                            /* "CR" set                             */
            }
            else if(*(pCommandBuffer + *pLength) == D_ATC_N_SEMICOLON)
            {
                ucStopChar = D_ATC_STOP_CHAR_SEMICOLON;                                     /* ";"  set                             */
            }
            else if(*(pCommandBuffer + *pLength) == D_ATC_N_COMMA)
            {
                ucStopChar = D_ATC_STOP_CHAR_KANMA;                                         /* ","  set                             */
            }
            else
            {
                ucStopChar = D_ATC_STOP_CHAR_STR;                                           /* other character set                  */
            }
        }
        else
        {
            for(iPalamCounter = 1; iPalamCounter <= iFigure; iPalamCounter++)
            {
                if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_CR )
                {
                    ucStopChar = D_ATC_STOP_CHAR_CR;
                    break;
                }
                else if( *(pCommandBuffer + iPalamCounter) == D_ATC_N_COMMA )
                {
                    ucStopChar = D_ATC_STOP_CHAR_KANMA;
                    break;
                }
            }
            if (iPalamCounter > iFigure)
            {
                ucStopChar = D_ATC_STOP_CHAR_STR;
                ucResultData = D_ATC_PARAM_EMPTY;
                pParaStr[0] = '\0';
            }
            else
            {
                (*pStrLength)  = (unsigned short)iPalamCounter;                                         /* check parameter length                   */
                for ( iCpyCounter = 0; iCpyCounter < iPalamCounter ; iCpyCounter++ )
                {
                    pParaStr[iCpyCounter] = pCommandBuffer[iCpyCounter];                        /* store parameter character                */
                }
                pParaStr[iCpyCounter] = '\0';                                                   /* NULL set                                 */
                *pLength  = (unsigned short)iPalamCounter;                                               /* parameter length set                 */
            }
        }
    }
    (*pStopStrInf) = ucStopChar;

    return ucResultData;
}

/*******************************************************************************
  MODULE    : ATC_CheckUSNumParameter
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CheckUSNumParameter(unsigned char *pCommandBuffer, unsigned int uiFigure, unsigned char *pLength, 
                                       unsigned short *pBinaryData, unsigned char *pStopStrInf)
{
    unsigned char ucResultData;
    unsigned char ucCount;
    unsigned char ucJudgucResult;

    if (D_ATC_N_CR == *pCommandBuffer)                                                  /* check <CR>                           */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_CR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)                                      /* check ';'                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)                                          /* check ','                            */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else if ((((*pCommandBuffer) < '0') || ((*pCommandBuffer) > '9')) && ((*pCommandBuffer) != '-'))  /* check other str        */
    {
        *pLength = 0;
        *pBinaryData = 0;
        *pStopStrInf = D_ATC_STOP_CHAR_STR;
        ucResultData = D_ATC_PARAM_EMPTY;
    }
    else
    {
        *pLength = 0;                                                                   /* Initial period                       */
        for (ucCount = 1; ; ucCount++)
        {
            if (D_ATC_N_CR == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_CR;
                break;
            }
            else if (D_ATC_N_SEMICOLON == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_SEMICOLON;
                break;
            }
            else if (D_ATC_N_COMMA == *(pCommandBuffer + ucCount))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_KANMA;
                break;
            }
            else if (((*(pCommandBuffer + ucCount) < '0') || (*(pCommandBuffer + ucCount) > '9'))
                      && (*(pCommandBuffer + ucCount) != '-'))
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else if (ucCount >= uiFigure)
            {
                *pStopStrInf = D_ATC_STOP_CHAR_STR;
                break;
            }
            else
            {
                /* no process */
            }
        }
        *pBinaryData = 0;
        ucJudgucResult = ATC_UshortToBinary(pCommandBuffer, ucCount, pBinaryData);
        if (ATC_OK == ucJudgucResult)
        {
            *pLength = (unsigned char)(*pLength + ucCount);
            ucResultData = D_ATC_PARAM_OK;
        }
        else
        {
            ucResultData = D_ATC_PARAM_SYNTAX_ERROR;
        }
    }
    return ucResultData;
}

/*******************************************************************************
  MODULE    : ATC_PassWordStrCheck
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  tianchengbin    2018/12/18   create
*******************************************************************************/
unsigned char ATC_PassWordStrCheck(unsigned char *pInputStr, unsigned char *pEventBuffer)
{
    unsigned short ucCount;
    unsigned char  ucResult = ATC_OK;

    for (ucCount = 0; *(pInputStr + ucCount) != '\0'; ucCount++)
    {
        switch(*(pInputStr + ucCount))
        {
        case '0' :
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
            continue;
        default:
            ucResult = ATC_NG;
            break;
        }
        break;
    }

    if (ATC_OK == ucResult)
    {
        ATC_Strncpy(pEventBuffer, pInputStr, ucCount);
    }

    return ucResult;
}

const char* ATC_ConvertErrCode2Str(const ST_ATC_GLOBLE_ERROR_CAUSE_TABLE* pErrTab, unsigned char size, unsigned char ucErrCode)
{ 
    unsigned char          index;
    const char*    unknown = "unknown";

    for(index = 0; index < size; index++)
    {
        if(ucErrCode == pErrTab[index].usErrCode)
        {
            return (const char*)pErrTab[index].pCmeErrorText;
        }
    }
    return unknown;
}

void ATC_NSET_AtHeaderSetProc(unsigned char* pCurrEvent)
{
    ST_ATC_NSET_PARAMETER * pNsetParam = NULL;

    pNsetParam = (ST_ATC_NSET_PARAMETER*)pCurrEvent;
    if(pNsetParam->usEvent != D_ATC_EVENT_NSET || 0 != strcmp((const char *)pNsetParam->aucInsValue, "AT_HEADER_SPACE"))
    {
        return;
    }

    if(pNsetParam->ulParam1 == 0 || pNsetParam->ulParam1 == 1)
    {
        g_AtcApInfo.ucAtHeaderSpaceFlg = pNsetParam->ulParam1;
    }
}

unsigned short ATC_GetExtendSpaceNum(char* pBuff)
{
    char* pSubStr;
    char* at_line_header = "\r\n";
    unsigned short usConut = 0;

    pSubStr = strstr(pBuff, at_line_header);
    while(NULL != pSubStr)
    {
        pSubStr = strstr(pSubStr, ":");   
        if(pSubStr == NULL)
        {
            break;
        }

        pSubStr++;
        if(pSubStr[0] != ' ')
        {
            usConut++;
        }

        pSubStr = strstr(pSubStr, at_line_header);
    }

    return usConut;
}

void ATC_CmdHeaderWithSpaceProc(char** pBuff, unsigned short* pusLen, unsigned short usMaxLen)
{
    char* pInStr;
    char* pSubStr;
    char* key_str = "\r\n";
    unsigned short usSpaceCnt;
    char* pTempBuff;
    
    if(NULL != strstr(*pBuff, "+PSINFO:") || NULL != strstr(*pBuff, "+DBGINFO:")|| NULL != strstr(*pBuff, "^SIMST:") )
    {
        return;
    }

    usSpaceCnt = ATC_GetExtendSpaceNum(*pBuff);
    if(usMaxLen < *pusLen + usSpaceCnt)
    {
        pTempBuff = (char*)AtcAp_Malloc(*pusLen + usSpaceCnt + 1);
        strncpy(pTempBuff, *pBuff, *pusLen);
        AtcAp_Free(*pBuff);
        *pBuff = pTempBuff;
  
        usMaxLen = *pusLen + usSpaceCnt;
    }

    pInStr = (char*)*pBuff;
    pSubStr = strstr(pInStr, key_str);
    while(NULL != pSubStr)
    {
        pInStr = pSubStr + strlen(key_str);
        pSubStr = strstr(pInStr, ":");
        if(pSubStr == NULL)
        {
            break;
        }
        pInStr = (pSubStr + 1);

        if(pInStr[0] != ' ')
        {
            if(strlen((const char *)(*pBuff)) + 1 > usMaxLen)
            {
                xy_assert(0);
            }

            if(strlen(pInStr) != 0)
            {
                memmove(pInStr + 1, pInStr, strlen(pInStr) + 1);
            }
            pInStr[0] = ' ';
        }
        pInStr++;

        pSubStr = strstr(pInStr, key_str);
    }

    *pusLen += usSpaceCnt;
}

unsigned char ATC_NCONFIG_SET_IsStrChk(unsigned char ucType)
{
    switch(ucType)
    {
        case D_ATC_NCONFIG_AUTOCONNECT:
        case D_ATC_NCONFIG_COMBINE_ATTACH:
        case D_ATC_NCONFIG_CELL_RESELECTION:
        case D_ATC_NCONFIG_ENABLE_BIP:
        case D_ATC_NCONFIG_MULTITONE:
        case D_ATC_NCONFIG_PCO_IE_TYPE:
        case D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE:
        case D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT:
        case D_ATC_NCONFIG_RAI:
        case D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING:
        case D_ATC_NCONFIG_CR_0859_SI_AVOID:
        case D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE:
        case D_ATC_NCONFIG_NPLMNS_OOS_IND:
            return ATC_AP_TRUE;
        default:
            return ATC_AP_FALSE;
    }
}

unsigned char ATC_GET_IPV6ADDR_ALL(unsigned char* pucIpv6Addr)
{
    unsigned int  uiIpv6Addr[4] = { 0 };
    int i = 0;

    if(ps_get_ip6addr(uiIpv6Addr) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    for(i = 0; i < 4; i++)
    {
        *(pucIpv6Addr+4*i+3) = (unsigned char)((uiIpv6Addr[i] & 0xFF000000) >> 24);
        *(pucIpv6Addr+(4*i+2)) = (unsigned char)((uiIpv6Addr[i] & 0xFF0000) >> 16);
        *(pucIpv6Addr+(4*i+1)) = (unsigned char)((uiIpv6Addr[i] & 0xFF00) >> 8);
        *(pucIpv6Addr+(4*i+0)) = (unsigned char)(uiIpv6Addr[i] & 0xFF);
    }

    return ATC_AP_TRUE;
}

void AtcApFreeWithMemClear(void **pMem)
{
    osMemoryFree(*pMem);
    *pMem = NULL;
}

void* AtcApMallocWithTrack(unsigned long ulSize,char *file,unsigned long line)
{
    void *tmp_ptr=NULL;

    #if ( configUSE_HEAP_MALLOC_DEBUG == 1 )
        tmp_ptr = osMemoryAllocDebug(ulSize,file,line);
    #else
        tmp_ptr = osMemoryAlloc(ulSize);
    #endif
        xy_assert(tmp_ptr != NULL);
        memset(tmp_ptr,0,ulSize);

    return tmp_ptr;
}
