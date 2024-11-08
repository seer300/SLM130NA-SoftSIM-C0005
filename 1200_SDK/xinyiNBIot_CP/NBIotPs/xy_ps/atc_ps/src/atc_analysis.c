#include "atc_ps.h"

/*******************************************************************************
  MODULE    : ATC_Command_CmdReferStr
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2008.05.09   create
*******************************************************************************/
unsigned char ATC_Command_CmdReferStr(unsigned char *pCommandBuffer,
                                             ST_ATC_COMMAND_ANAL_TABLE *pTblPtr,
                                             unsigned char *pTblPosition)
{
    unsigned char ucCtr;
    unsigned char ucStrCtr;
    unsigned char ucResult;
    unsigned short usAtHeadLen = 0;

    ucCtr = 0;
    ucResult = ATC_NG;
    *pTblPosition = ATC_NG;

    for(;;)
    {
        if(pCommandBuffer[usAtHeadLen] == '?' || pCommandBuffer[usAtHeadLen] == '=' || pCommandBuffer[usAtHeadLen] == '\r')
        {
            break;
        }
        usAtHeadLen++;
    }

    for (; ;)
    {
        if (*(pTblPtr[ucCtr].LetterData.CommandLetter) == '\0')
        {
            break;
        }

        if(strlen((char*)pTblPtr[ucCtr].LetterData.CommandLetter) != usAtHeadLen)
        {
            ucCtr++;
            continue;
        }

        ucStrCtr = 0;

        for (; ;)
        {
            if (*((pTblPtr[ucCtr].LetterData.CommandLetter) + ucStrCtr) == '\0')
            {
                *pTblPosition = ucCtr;
                ucResult = ATC_OK;
                break;
            }

            if (*((pTblPtr[ucCtr].LetterData.CommandLetter) + ucStrCtr) != *(pCommandBuffer + ucStrCtr))
            {
                break;
            }

            ucStrCtr++;
        }

        if (ucResult != ATC_NG)
        {
            break;
        }

        ucCtr++;
    }

    return ucResult;
}

#ifdef NBIOT_SMS_FEATURE
static void AtcAp_SmsPdu_Analysis(ATC_AP_MSG_DATA_REQ_STRU* pAtcDataReq, unsigned char *pEventBuffer, unsigned char* pucResult)
{
    ST_ATC_AP_SMS_PDU_PARAMETER* pSmsPduParam;

    if(ATC_AP_FALSE == AtcAp_CurrEventChk_IsWaitSmsPdu())
    {
        return;
    }
    
    pSmsPduParam = (ST_ATC_AP_SMS_PDU_PARAMETER*)pEventBuffer;
    pSmsPduParam->usEvent = D_ATC_AP_SMS_PDU_REQ;
    pSmsPduParam->usCmdEvent = g_AtcApInfo.usCurrEvent;
    pSmsPduParam->usPduLength = pAtcDataReq->usMsgLen;
    pSmsPduParam->pucPduData = pAtcDataReq->aucMsgData;

    *pucResult = D_ATC_COMMAND_OK;
}

static unsigned char AtcAp_InquirySet(unsigned char *pEventBuffer)
{
    switch (((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent)
    {
        /*basic*/
    case D_ATC_EVENT_CEREG:
    case D_ATC_EVENT_CEREG_R:
    case D_ATC_EVENT_CEREG_T:
    //case D_ATC_EVENT_CGMI:
    //case D_ATC_EVENT_CGMI_T:
    //case D_ATC_EVENT_CGMR:  
    //case D_ATC_EVENT_CGMR_T:
    case D_ATC_EVENT_CGSN:  
    case D_ATC_EVENT_CGSN_T:
    case D_ATC_EVENT_CGATT_R:
    case D_ATC_EVENT_CGATT_T:
    case D_ATC_EVENT_CIMI:
    case D_ATC_EVENT_CIMI_T:
    case D_ATC_EVENT_CGDCONT_R:
    case D_ATC_EVENT_CGDCONT_T:
    case D_ATC_EVENT_CFUN_R:
    case D_ATC_EVENT_CFUN_T:
    case D_ATC_EVENT_CMEE:
    case D_ATC_EVENT_CMEE_R:
    case D_ATC_EVENT_CMEE_T:
    case D_ATC_EVENT_CLAC:
    case D_ATC_EVENT_CLAC_T:
    case D_ATC_EVENT_CESQ:
    case D_ATC_EVENT_CESQ_T:
    case D_ATC_EVENT_CGPADDR:
    case D_ATC_EVENT_CGPADDR_T:
    //case D_ATC_EVENT_CGMM:
    //case D_ATC_EVENT_CGMM_T:
    //case D_ATC_EVENT_CGDATA_T:
    case D_ATC_EVENT_CGACT_R:
    case D_ATC_EVENT_CGACT_T:
    case D_ATC_EVENT_CSODCP_T:
    case D_ATC_EVENT_CRTDCP:
    case D_ATC_EVENT_CRTDCP_R:
    case D_ATC_EVENT_CRTDCP_T:
    //case D_ATC_EVENT_CRC:  
    //case D_ATC_EVENT_CRC_R:
    //case D_ATC_EVENT_CRC_T:
    //case D_ATC_EVENT_CMUX://bind function  
    //case D_ATC_EVENT_CMUX_R:
    //case D_ATC_EVENT_CMUX_T:
    //case D_ATC_EVENT_S3:  
    //case D_ATC_EVENT_S3_R:
    //case D_ATC_EVENT_S4:
    //case D_ATC_EVENT_S4_R:
    //case D_ATC_EVENT_S5:
    //case D_ATC_EVENT_S5_R:
    //case D_ATC_EVENT_E:
    //case D_ATC_EVENT_V:
    //case D_ATC_EVENT_F:
    case D_ATC_EVENT_SIMST_R:
    case D_ATC_EVENT_CEDRXS_R:
    case D_ATC_EVENT_CEDRXS_T:
    case D_ATC_EVENT_CPSMS_R:
    case D_ATC_EVENT_CPSMS_T:
    case D_ATC_EVENT_CGAPNRC:
    case D_ATC_EVENT_CGAPNRC_T:
#ifdef NBIOT_SMS_FEATURE
        /* SMS */
//    case D_ATC_EVENT_CSMS:
    case D_ATC_EVENT_CSMS_R:
    case D_ATC_EVENT_CSMS_T:
//    case D_ATC_EVENT_CMGF:
    case D_ATC_EVENT_CMGF_R:
    case D_ATC_EVENT_CMGF_T:
//    case D_ATC_EVENT_CSCA:
    case D_ATC_EVENT_CSCA_R:
    case D_ATC_EVENT_CSCA_T:
//    case D_ATC_EVENT_CNMI:
//    case D_ATC_EVENT_CNMI_R:
//    case D_ATC_EVENT_CNMI_T:
//    case D_ATC_EVENT_CMGS:
    case D_ATC_EVENT_CMGS_T:
//    case D_ATC_EVENT_CNMA:
    case D_ATC_EVENT_CNMA_T:
#endif
#ifdef ESM_DEDICATED_EPS_BEARER
    case D_ATC_EVENT_CGDSCONT_R:
    case D_ATC_EVENT_CGDSCONT_T:
#endif
#ifdef EPS_BEARER_TFT_SUPPORT
    case D_ATC_EVENT_CGTFT_R:
    case D_ATC_EVENT_CGTFT_T:
#endif
    case D_ATC_EVENT_CGEQOS_R:
    case D_ATC_EVENT_CGEQOS_T:
#ifdef ESM_EPS_BEARER_MODIFY
    case D_ATC_EVENT_CGCMOD_T:
#endif
    case D_ATC_EVENT_CSIM_T:
    case D_ATC_EVENT_CCHC_T:
    case D_ATC_EVENT_CCHO_T:
    case D_ATC_EVENT_CGLA_T:
    case D_ATC_EVENT_CRSM_T:
    case D_ATC_EVENT_CSCON:
    case D_ATC_EVENT_CSCON_R:
    case D_ATC_EVENT_CSCON_T:
    case D_ATC_EVENT_CGEREP:
    case D_ATC_EVENT_CGEREP_R:
    case D_ATC_EVENT_CGEREP_T:
    case D_ATC_EVENT_CCIOTOPT_R:
    case D_ATC_EVENT_CCIOTOPT_T:
    case D_ATC_EVENT_CEDRXRDP:
    case D_ATC_EVENT_CEDRXRDP_T:
    case D_ATC_EVENT_CGEQOSRDP:
    case D_ATC_EVENT_CGEQOSRDP_T:
    case D_ATC_EVENT_CTZR:
    case D_ATC_EVENT_CTZR_R:
    case D_ATC_EVENT_CTZR_T:
    case D_ATC_EVENT_CGCONTRDP:
    case D_ATC_EVENT_CGCONTRDP_T:
    case D_ATC_EVENT_CPIN_T:
    case D_ATC_EVENT_CLCK_T:
    case D_ATC_EVENT_CPWD_T:
    case D_ATC_EVENT_CSQ:
    case D_ATC_EVENT_CSQ_T:
    case D_ATC_EVENT_NBAND_R:
    case D_ATC_EVENT_NBAND_T:
    case D_ATC_EVENT_NEARFCN_R:
    case D_ATC_EVENT_NEARFCN_T:   
    case D_ATC_EVENT_NCONFIG_R:
    case D_ATC_EVENT_NCONFIG_T: 
    case D_ATC_EVENT_NSET_R:
    case D_ATC_EVENT_CEER_T:
    case D_ATC_EVENT_CIPCA_R:
    case D_ATC_EVENT_CIPCA_T:
    case D_ATC_EVENT_CGAUTH_R:
    case D_ATC_EVENT_CGAUTH_T:
    case D_ATC_EVENT_CNMPSD_T:
    case D_ATC_EVENT_CPINR_T:
    case D_ATC_EVENT_CMGC_T:
    case D_ATC_EVENT_CMMS_R:
    case D_ATC_EVENT_CMMS_T:
    case D_ATC_EVENT_NPOWERCLASS_R:
    case D_ATC_EVENT_NPOWERCLASS_T:
    case D_ATC_EVENT_NPTWEDRXS_R:
    case D_ATC_EVENT_NPTWEDRXS_T:
    case D_ATC_EVENT_NPIN_T:
    case D_ATC_EVENT_NTSETID_T:
    case D_ATC_EVENT_NCIDSTATUS_T:
    case D_ATC_EVENT_NGACTR_R:
    case D_ATC_EVENT_NGACTR_T:
    case D_ATC_EVENT_NPOPB_R:
    case D_ATC_EVENT_NRNPDM:
    case D_ATC_EVENT_NRNPDM_R:
    case D_ATC_EVENT_NRNPDM_T:
        return D_ATC_FLAG_TRUE;
    default:
        return D_ATC_FLAG_FALSE;
        break;
    }
}
#endif

/*******************************************************************************
  MODULE    : ATC_Command_Analysis
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2008.05.09   create
*******************************************************************************/
unsigned char ATC_Command_Analysis(unsigned char* pAtcDataReq, unsigned char *pEventBuffer)
{
    unsigned char  ucResult;                                                                     /* Command result                       */
    unsigned char  ucJmpTblCtr;
    unsigned char  ucCmdAnlRes;                                                                  /* Command analysis response            */
    unsigned char  ucJmpTblRes;                                                                  /* Jump table response                  */
    unsigned char  ucFlag = D_ATC_FLAG_FALSE;
    unsigned short usCnt;
    int            iStrCtr;                                                                      /* Count                                */
    unsigned char *pCommandBuffer = NULL;
    ST_ATC_COMMAND_ANAL_TABLE *pWorkJumpTblPtr = NULL;                                  /* Jump table                           */
    ATC_AP_MSG_DATA_REQ_STRU       *pRcvMsg = NULL;
#ifdef NBIOT_SMS_FEATURE
    unsigned char  ucCfunFlg;
    unsigned char  ucInquiryFlg;
#endif

    pRcvMsg = (ATC_AP_MSG_DATA_REQ_STRU *)pAtcDataReq;

    //OutputLongTraceMessage(NAS_LOG_LEVEL_2,(const char *)pRcvMsg->aucMsgData);

    pCommandBuffer = pRcvMsg->aucMsgData;
    for (; *pCommandBuffer != '\0';)
    {
        if (*pCommandBuffer != D_ATC_N_LF)
        {
            break;
        }
        else
        {
            pCommandBuffer++;
        }
    }


    for (usCnt = 0; *(pCommandBuffer + usCnt) != '\0'; usCnt++)
    {
        if (((*(pCommandBuffer + usCnt) == 'A') && (*(pCommandBuffer + usCnt + 1) == 'T'))
            || ((*(pCommandBuffer + usCnt) == 'a') && (*(pCommandBuffer + usCnt + 1) == 't'))
            || ((*(pCommandBuffer + usCnt) == 'A') && (*(pCommandBuffer + usCnt + 1) == 't'))
            || ((*(pCommandBuffer + usCnt) == 'a') && (*(pCommandBuffer + usCnt + 1) == 'T')))
        {
            pCommandBuffer += (usCnt + 2);                                              /* Begin to judge after 'AT'                */
            ucFlag = D_ATC_FLAG_TRUE;
            break;
        }
    }

    if (D_ATC_FLAG_FALSE == ucFlag)
    {
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
#ifdef NBIOT_SMS_FEATURE
        AtcAp_SmsPdu_Analysis((ATC_AP_MSG_DATA_REQ_STRU*)pAtcDataReq, pEventBuffer, &ucResult);
#endif
        return ucResult;
    }

    if (D_ATC_N_CR == *pCommandBuffer)
    {   
        ucResult = D_ATC_COMMAND_END;
    }
    else if (D_ATC_N_SEMICOLON == *pCommandBuffer)
    {   
        ucResult = D_ATC_COMMAND_DELIMITOR;
    }
    else if (D_ATC_N_COMMA == *pCommandBuffer)
    {   
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
    }
    else
    {
        iStrCtr = 0;                                                                    /* Count Initial period                 */

        for (; ;)
        {
            if (*(pCommandBuffer + iStrCtr) == D_ATC_N_CR 
                || *(pCommandBuffer + iStrCtr) == '='
                || *(pCommandBuffer + iStrCtr) == '?')
            {
                break;
            }

            /* charactor transformation  */
            if (*(pCommandBuffer + iStrCtr) >='a' && *(pCommandBuffer + iStrCtr) <='z')
            {
                if (('i' == *(pCommandBuffer + iStrCtr)) &&
                    ((*(pCommandBuffer + iStrCtr - 1) >= 0x30 && *(pCommandBuffer + iStrCtr - 1) <= 0x39)))
                {
                    /* *(pCommandBuffer + iStrCtr - 1) is number 0-9 */
                    break;
                }
                *(pCommandBuffer + iStrCtr) -= 0x20;                                    /* Small letter convert to capital letter */
            }

            iStrCtr++;                                                                  /* Count increase                       */
        }

        /* Convert ATDTxxx,ATDPxxx to ATDxxx,ATDxxx */
        iStrCtr = 1;

        if (((*pCommandBuffer == 'D') && (*(pCommandBuffer + 1) == 'T')) ||
            ((*pCommandBuffer == 'D') && (*(pCommandBuffer + 1) == 'P')))
        {
            for (; ;)
            {
                if (*(pCommandBuffer + iStrCtr) == D_ATC_N_CR)
                {
                    break;
                }

                *(pCommandBuffer + iStrCtr) = *(pCommandBuffer + iStrCtr + 1);
                iStrCtr++;
            }
        }

        /* '+' command table                      */
        if ((*pCommandBuffer) == '+')
        {
            pWorkJumpTblPtr = (ST_ATC_COMMAND_ANAL_TABLE *)ATC_Plus_CommandTable;
            ucResult = D_ATC_COMMAND_OK;
        }
        /* Symbol command table                   */
        else if ((*pCommandBuffer) == '&' || (*pCommandBuffer) == '^' || (*pCommandBuffer) == '$')
        {
            pWorkJumpTblPtr = (ST_ATC_COMMAND_ANAL_TABLE *)ATC_Symbol_CommandTable;
            ucResult = D_ATC_COMMAND_OK;
        }
        /* Single command table                   */
        else if (((*pCommandBuffer) >= 'A') && ((*pCommandBuffer) <= 'Z'))
        {
            pWorkJumpTblPtr = (ST_ATC_COMMAND_ANAL_TABLE *)ATC_Single_CommandTable;
            ucResult = D_ATC_COMMAND_OK;
        }
        else
        {
            ucResult = D_ATC_COMMAND_ERROR;                                             /* No define command                    */
        }
        
        if (ucResult != D_ATC_COMMAND_ERROR)
        {
            ucJmpTblRes = ATC_Command_CmdReferStr(pCommandBuffer,
                                                pWorkJumpTblPtr, &ucJmpTblCtr);

            if (ucJmpTblRes != ATC_NG)
            {
                ucCmdAnlRes = (*pWorkJumpTblPtr[ucJmpTblCtr].CommandProc)
                                                (pCommandBuffer, pEventBuffer);         /*lint !e613*/

                switch(ucCmdAnlRes)
                {
                case D_ATC_COMMAND_OK :
                case D_ATC_COMMAND_SYNTAX_ERROR:                                        /* Syntax error                         */
                case D_ATC_COMMAND_PARAMETER_ERROR:                                     /* Parameter error                      */
                case D_ATC_COMMAND_PDU_CANCEL:                                          /* PDU cancel                           */
                case D_ATC_COMMAND_TOO_MANY_PARAMETERS:                                 /* Too many parameter                   */
                    ucResult = ucCmdAnlRes;
                    break;
                default:
                    break;
                }
            }
            else 
            {
                ucResult = D_ATC_COMMAND_ERROR;
            }
        }
    }

    if (D_ATC_COMMAND_OK == ucResult)
    {
#ifdef NBIOT_SMS_FEATURE
        ucInquiryFlg = AtcAp_InquirySet(pEventBuffer);
        if (D_ATC_EVENT_CFUN == ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent)
        {
            if ((4 == ((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun) 
                || (5 == ((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun)
                || (0 == ((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun))
            {
                ucCfunFlg = D_ATC_FLAG_TRUE;
            }
        }
        if ((D_ATC_FLAG_FALSE == ucCfunFlg)
            && (D_ATC_FLAG_FALSE == ucInquiryFlg))
        {
            AtcAp_SmsPdu_Analysis((ATC_AP_MSG_DATA_REQ_STRU*)pAtcDataReq, pEventBuffer, &ucResult);
        }
#endif
    }
    else
    {
#ifdef NBIOT_SMS_FEATURE
        AtcAp_SmsPdu_Analysis((ATC_AP_MSG_DATA_REQ_STRU*)pAtcDataReq, pEventBuffer, &ucResult);
#endif
    }

    return ucResult;
}

