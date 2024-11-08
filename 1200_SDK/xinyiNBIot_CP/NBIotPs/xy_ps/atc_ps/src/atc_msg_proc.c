#include "atc_ps.h"

static int AtcAp_SendCmdEventToPs()
{
    unsigned char*          pCodeStream;
    unsigned short          usCodeStreamLen;
    int                     ret;

    if(NULL == g_AtcApInfo.pCurrEvent)
    {
        return ATC_AP_FALSE;
    }

    AtcAp_Encode_UN_ATC_CMD_EVENT((UN_ATC_CMD_EVENT*)g_AtcApInfo.pCurrEvent, &pCodeStream, &usCodeStreamLen);

    AtcAp_FreeEventBuffer((unsigned char*)g_AtcApInfo.pCurrEvent);
    g_AtcApInfo.pCurrEvent = NULL;
    
#ifdef SINGLE_CORE
    ret = AtcAp_SndDataReqToPs(pCodeStream, usCodeStreamLen);
#else
    ret = AtcAp_SndDataReqToShm(pCodeStream, usCodeStreamLen);
#endif
    AtcAp_Free(pCodeStream);

    return ret;
}

void AtcAp_CascadeAtProc_NextAt()
{
    unsigned short i;
    unsigned short usLength = 0;
    unsigned char *pucData  = NULL;
    ST_ATC_CASCADE_AT_INFO *pCascadeAtInfo;

    if(ATC_FALSE == g_AtcApInfo.atCascateInfo.ucCascadeAtFlg)
    {
        return;
    }

    AtcAp_PrintLog(0, LRRC_THREAD_ID, WARN_LOG, "[AtcAp_CascadeAtProc_NextAt] ucCascadeAtFlg=%d, ucCascadeAtCnt=%d",
            g_AtcApInfo.atCascateInfo.ucCascadeAtFlg,
            g_AtcApInfo.atCascateInfo.ucCascadeAtCnt);

    pCascadeAtInfo = &g_AtcApInfo.atCascateInfo;
    if(NULL == pCascadeAtInfo->pCasaadeAtBuff)
    {
        AtcAp_PrintLog(0, LRRC_THREAD_ID, WARN_LOG, "[AtcAp_CascadeAtProc_NextAt]pCasaadeAtBuff : null");
        return;
    }

    for(i = 0; i < pCascadeAtInfo->usLen - pCascadeAtInfo->offset; i++)
    {
        if (D_ATC_N_SEMICOLON == pCascadeAtInfo->pCasaadeAtBuff[pCascadeAtInfo->offset + i])
        {
            if(0 == pCascadeAtInfo->offset)//first at
            {
                usLength = i + 1;
                pucData = (unsigned char *)AtcAp_Malloc(usLength);
                if(0 != i)
                {
                    AtcAp_MemCpy(pucData, pCascadeAtInfo->pCasaadeAtBuff + pCascadeAtInfo->offset, i);
                }
            }
            else
            {
                usLength = i + 3;
                pucData = (unsigned char *)AtcAp_Malloc(usLength);
                pucData[0] = 'A';
                pucData[1] = 'T';
                if(0 != i)
                {
                    AtcAp_MemCpy(pucData + 2, pCascadeAtInfo->pCasaadeAtBuff + pCascadeAtInfo->offset, i);
                }
            }
            pucData[usLength - 1] = D_ATC_DEFAULT_CR;
            pCascadeAtInfo->offset += (i + 1);
            
            if(pCascadeAtInfo->offset == pCascadeAtInfo->usLen)
            {
                AtcAp_Free(g_AtcApInfo.atCascateInfo.pCasaadeAtBuff);
                g_AtcApInfo.atCascateInfo.usLen = 0;
            }
            
            ATC_SendApDataReq(ATC_AP_FALSE, 0, usLength, pucData, 0);
            AtcAp_Free(pucData);
            
            return;
        }
    }

    //last at
    usLength = pCascadeAtInfo->usLen - pCascadeAtInfo->offset + 3;
    pucData = (unsigned char *)AtcAp_Malloc(usLength);
    pucData[0] = 'A';
    pucData[1] = 'T';
    AtcAp_MemCpy(pucData + 2, pCascadeAtInfo->pCasaadeAtBuff + pCascadeAtInfo->offset, pCascadeAtInfo->usLen - pCascadeAtInfo->offset);
    pucData[usLength - 1] = D_ATC_DEFAULT_CR;

    AtcAp_Free(g_AtcApInfo.atCascateInfo.pCasaadeAtBuff);
    g_AtcApInfo.atCascateInfo.usLen = 0;
    
    ATC_SendApDataReq(ATC_AP_FALSE, 0, usLength, pucData, 0);
    AtcAp_Free(pucData);

}

static unsigned char AtcAp_IsCascateAtChk(ATC_AP_MSG_DATA_REQ_STRU* pDataReq)
{
    unsigned short i;
    unsigned short usSendLen = 0;
    
    if(ATC_AP_FALSE == pDataReq->ucExternalFlg)
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_IsCascateAtChk] ucExternalFlg=0");
        return ATC_AP_FALSE;
    }

    if(ATC_AP_TRUE == g_AtcApInfo.atCascateInfo.ucCascadeAtFlg) //abnormal:waitting for cascade at to end
    {
        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_IsCascateAtChk] abnormal:waitting for cascade at to end");
        AtcAp_SendErrorRsp();
        return ATC_AP_TRUE;
    }

#ifdef NBIOT_SMS_FEATURE
    if(ATC_AP_TRUE == AtcAp_CurrEventChk_IsWaitSmsPdu())
    {
        return ATC_AP_FALSE;
    }
#endif
    
    g_AtcApInfo.atCascateInfo.ucCascadeAtCnt = 0;
    for(i = 0; i< pDataReq->usMsgLen; i++)
    {
        if (D_ATC_N_SEMICOLON == pDataReq->aucMsgData[i])
        {
            g_AtcApInfo.atCascateInfo.ucCascadeAtCnt++;
            usSendLen = i;
        }
    }

    if(usSendLen < pDataReq->usMsgLen)
    {
        g_AtcApInfo.atCascateInfo.ucCascadeAtCnt++;
    }

    if(g_AtcApInfo.atCascateInfo.ucCascadeAtCnt > 1)
    {
        g_AtcApInfo.atCascateInfo.ucCascadeAtFlg = ATC_AP_TRUE;
        g_AtcApInfo.atCascateInfo.pCasaadeAtBuff = (unsigned char*)AtcAp_Malloc(pDataReq->usMsgLen);
        AtcAp_MemCpy(g_AtcApInfo.atCascateInfo.pCasaadeAtBuff, pDataReq->aucMsgData, pDataReq->usMsgLen);
        g_AtcApInfo.atCascateInfo.usLen = pDataReq->usMsgLen;
        g_AtcApInfo.atCascateInfo.offset = 0;

        AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_IsCascateAtChk] new cascade at");
        AtcAp_CascadeAtProc_NextAt();
        
        return ATC_AP_TRUE;
    }

    AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG, "[ATC_IsCascateAtChk] no cascade at");
    return ATC_AP_FALSE;
}

/*******************************************************************************
  MODULE    : ATC_DataReqResultHandle
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  GCM   2018.10.15   create
*******************************************************************************/
static void AtcAp_DataReqResultHandle(unsigned char ucResult)
{
    switch(ucResult)
    {
    case D_ATC_COMMAND_END:
        AtcAp_SendOkRsp();
        break;
    case D_ATC_COMMAND_DELIMITOR:
    case D_ATC_COMMAND_SYNTAX_ERROR:                     
    case D_ATC_COMMAND_MODE_ERROR:     
        AtcAp_SendErrorRsp();
        break;
    case D_ATC_COMMAND_PARAMETER_ERROR:
    case D_ATC_COMMAND_TOO_MANY_PARAMETERS:   
        AtcAp_SendCmeeErr(D_ATC_AP_CME_INCORRECT_PARAMETERS);
        break;
    case D_ATC_COMMAND_ERROR:
        AtcAp_SendCmeeErr(D_ATC_AP_CME_OPER_NOT_SUPPORED);
        break;
    default:
        AtcAp_SendErrorRsp();
        break;    
    }
    return;
}

static unsigned char AtcAp_Command_Excute(ST_ATC_CMD_COM_EVENT* pCmdEvent)
{
    unsigned char i;

    for(i = 0; i < D_ATC_CMD_PROC_TBL_SIZE; i++)
    {
        if(pCmdEvent->usEvent == AtcAp_CmdProcTable[i].usEvent)
        {
            return AtcAp_CmdProcTable[i].CommandProc((unsigned char*)pCmdEvent);
        }
    }

    return ATC_AP_FALSE;
}

static void AtcAp_DataReqProc_AtCmd(ATC_AP_MSG_DATA_REQ_STRU* pAtcDataReq)
{
    unsigned char           ucResult;

    if(ATC_AP_TRUE == AtcAp_IsCascateAtChk(pAtcDataReq))
    {
        return;
    }

    if(NULL != g_AtcApInfo.pCurrEvent)
    {
        AtcAp_FreeEventBuffer((unsigned char*)g_AtcApInfo.pCurrEvent);
    }
    g_AtcApInfo.pCurrEvent = (ST_ATC_CMD_COM_EVENT*)AtcAp_Malloc(sizeof(UN_ATC_CMD_EVENT));

    ucResult = ATC_Command_Analysis((unsigned char*)pAtcDataReq, (unsigned char*)g_AtcApInfo.pCurrEvent);
    g_AtcApInfo.usCurrEvent = g_AtcApInfo.pCurrEvent->usEvent;
    if (D_ATC_COMMAND_OK != ucResult)
    {
        AtcAp_DataReqResultHandle(ucResult);
        return;
    }

    if(ATC_AP_FALSE == AtcAp_Command_Excute(g_AtcApInfo.pCurrEvent))
    {
        ATC_NSET_AtHeaderSetProc((unsigned char*)g_AtcApInfo.pCurrEvent);
        if(AtcAp_SendCmdEventToPs() != ATC_AP_TRUE)
        {
            AtcAp_SendErrorRsp();
        }
    }
}

static void AtcAp_SaveAtcDataReq2List(ATC_AP_MSG_DATA_REQ_STRU* pAtcDataReq)
{
    ST_ATC_AP_ATC_DATA_REQ_NODE* pAtcDataReqNode;

    pAtcDataReqNode = (ST_ATC_AP_ATC_DATA_REQ_NODE*)AtcAp_Malloc(sizeof(ST_ATC_AP_ATC_DATA_REQ_NODE));
    pAtcDataReqNode->pDataReq = pAtcDataReq;
    
    AtcAp_LinkList_AddNode((ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAtcDataReqList.pHead,
                           (ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAtcDataReqList.pTail,
                           (ST_ATC_AP_LINK_NODE*)pAtcDataReqNode);
}

static void AtcAp_DeleteFirstAtcDataReqInList()
{
    ST_ATC_AP_ATC_DATA_REQ_NODE* pAtcDataReqNode;

    pAtcDataReqNode = g_AtcApInfo.stAtcDataReqList.pHead;
    AtcAp_LinkList_RemoveNode((ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAtcDataReqList.pHead,
                              (ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAtcDataReqList.pTail,
                              (ST_ATC_AP_LINK_NODE*)pAtcDataReqNode);

    AtcAp_Free(pAtcDataReqNode);
}

static ATC_AP_MSG_DATA_REQ_STRU* AtcAp_GetFirstAtcDataReqFromList()
{
    ATC_AP_MSG_DATA_REQ_STRU*    pDataReq;

    if(NULL == g_AtcApInfo.stAtcDataReqList.pHead)
    {
        return NULL;
    }

    pDataReq = g_AtcApInfo.stAtcDataReqList.pHead->pDataReq;

    AtcAp_DeleteFirstAtcDataReqInList();
    
    return pDataReq;
}

static unsigned char AtcAp_MsgProc_AtcApDataReq(ATC_AP_MSG_DATA_REQ_STRU* pAtcDataReq)
{
    AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "AtcAp_MsgProc_AtcApDataReq: cmd=%s, ucExternalFlg=%d, ulSemaId=%d, TaskSource=%d",pAtcDataReq->aucMsgData, pAtcDataReq->ucExternalFlg, pAtcDataReq->ulSemaId, pAtcDataReq->ucTaskSource);
    if(ATC_AP_TRUE == pAtcDataReq->ucExternalFlg)
    {
        if(ATC_AP_FALSE == AtcAp_CurrEventChk_IsWaitSmsPdu())
        {
            if(ATC_AP_TRUE == g_AtcApInfo.ucWaitOKOrErrorFlg)
            {
                if(pAtcDataReq->ulSemaId != 0 && pAtcDataReq->ucTaskSource == 1)
                {
                    xy_assert(0);
                }
                AtcAp_SaveAtcDataReq2List(pAtcDataReq);
                return ATC_AP_FALSE;
            }
            g_AtcApInfo.ucWaitOKOrErrorFlg = ATC_AP_TRUE;
        
            if(0 == pAtcDataReq->ulSemaId)
            {
                g_AtcApInfo.ucUserAtFlg = ATC_AP_FALSE;
            }
            else
            {
                g_AtcApInfo.ucUserAtFlg = ATC_AP_TRUE;
                AtcAp_AddAppInterfaceInfo(pAtcDataReq->ulSemaId);
            }
        }
        else if(0 != pAtcDataReq->ulSemaId)
        {
            if(g_AtcApInfo.ucUserAtFlg == ATC_AP_FALSE)
            {
                AtcAp_SaveAtcDataReq2List(pAtcDataReq);
                return ATC_AP_FALSE;
            }
            else
            {
                AtcAp_AddAppInterfaceInfo(pAtcDataReq->ulSemaId);
            }
        }
        else if(0 == pAtcDataReq->ulSemaId)
        {
            if(g_AtcApInfo.ucUserAtFlg == ATC_AP_TRUE)
            {
                AtcAp_SaveAtcDataReq2List(pAtcDataReq);
                return ATC_AP_FALSE;
            }
        }
    }

    AtcAp_DataReqProc_AtCmd(pAtcDataReq);
    
    return ATC_AP_TRUE;
}

static unsigned char AtcAp_MsgProc_UserAtCnf(ATC_MSG_DATA_IND_STRU* pAtcDataInd)
{
    ST_ATC_AP_CMD_RST*                pCmdRst = (ST_ATC_AP_CMD_RST*)(pAtcDataInd->aucMsgData);
    ST_ATC_AP_APP_INTERFACE_NODE*     pAppInterfaceInfo;
    
    if(0 == pAtcDataInd->ucSeqNum)
    {
        return ATC_AP_FALSE;
    }

    if(D_ATC_AP_AT_CMD_RST == pCmdRst->usEvent)
    {
        g_AtcApInfo.ucTempSeqNum = pAtcDataInd->ucSeqNum;
        AtcAp_MsgProc_AT_CMD_RST(pAtcDataInd->aucMsgData);
    }
    else if(D_ATC_AP_SMS_PDU_IND == pCmdRst->usEvent)
    {
        AtcAp_AppInterfaceInfo_CmdRstProc(pAtcDataInd->ucSeqNum, D_APP_INTERFACE_RESULT_SUCC);
    }
    else
    {
        osMutexAcquire(g_AtcApInfo.stAppInterfaceInfo.mutex, osWaitForever);
        pAppInterfaceInfo = AtcAp_GetAppInterfaceInfo_BySeqNum(pAtcDataInd->ucSeqNum);
        if(pAppInterfaceInfo == NULL)
        {
            osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
            return ATC_AP_FALSE;
        }
        
        pAppInterfaceInfo->usDataLen = pAtcDataInd->usMsgLen;
        if(0 != pAtcDataInd->usMsgLen)
        {
            if(pAppInterfaceInfo->pucData != NULL)
            {
                AtcAp_Free(pAppInterfaceInfo->pucData);
            }
            pAppInterfaceInfo->pucData = (unsigned char*)AtcAp_Malloc(pAtcDataInd->usMsgLen + 1);
            AtcAp_MemCpy(pAppInterfaceInfo->pucData, pAtcDataInd->aucMsgData, pAtcDataInd->usMsgLen);
        }
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
    }

    return ATC_AP_TRUE;
}

static unsigned char AtcAp_DataIndProc_IsRegEventIndChk(unsigned short usEvent, unsigned long* pulEventId)
{
    unsigned char i;
    
    for(i = 0; i < D_ATC_USER_REG_EVENT_TBL_SIZE; i++)
    {
        if(ATC_AP_PsRegEventIdMapTable[i][1] == usEvent)
        {
            if(0 != (ATC_AP_PsRegEventIdMapTable[i][0] & (*g_pRegEventId)))
            {
                *pulEventId = ATC_AP_PsRegEventIdMapTable[i][0];
                return ATC_AP_TRUE;
            }
            break;
        }
    }

    return ATC_AP_FALSE;
}

void  AtcAp_DataIndProc_RegEventInd(unsigned char* pRecvMsg, unsigned short usLen)
{
    ST_ATC_CMD_COM_EVENT*             pCmdEvent;
    unsigned long                     ulEvnetId;
    ST_ATC_AP_PS_EVENT_REGISTER_INFO* pNode;
    
    pCmdEvent = (ST_ATC_CMD_COM_EVENT*)pRecvMsg;
    if(ATC_AP_FALSE == AtcAp_DataIndProc_IsRegEventIndChk(pCmdEvent->usEvent, &ulEvnetId))
    {
        return;
    }

    osMutexAcquire(g_pRegEventId_Mutex, osWaitForever);
    pNode = g_AtcApInfo.pEventRegList;
    while(pNode != NULL)
    {
        if(pNode->eventId == ulEvnetId && NULL != pNode->callback)
        {
            AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_DataIndProc_RegEventInd]: CB event=%d",pCmdEvent->usEvent);
            pNode->callback(ulEvnetId, (void*)pRecvMsg, usLen);
        }
        pNode = (ST_ATC_AP_PS_EVENT_REGISTER_INFO*)(pNode->node.next);
    }
    osMutexRelease(g_pRegEventId_Mutex);
}

static void AtcAp_MsgProc_AtcApDataInd(ATC_MSG_DATA_IND_STRU* pAtcDataInd)
{
    ST_ATC_CMD_COM_EVENT*   pCmdEvent;

    pCmdEvent = (ST_ATC_CMD_COM_EVENT*) pAtcDataInd->aucMsgData;

    if(ATC_AP_TRUE == AtcAp_MsgProc_UserAtCnf(pAtcDataInd))
    {
        AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_MsgProc_AtcApDataInd]: ucSeqNum=%d, event=%d, userAtCnf",pAtcDataInd->ucSeqNum, pCmdEvent->usEvent);
    }
    else
    {
        AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[AtcAp_MsgProc_AtcApDataInd]: ucSeqNum=%d, event=%d",pAtcDataInd->ucSeqNum, pCmdEvent->usEvent);
    }

    AtcAp_SendMsg2AtcAp((void*)pAtcDataInd, &g_AtcApInfo.MsgInfo_EventCb);
}

void AtcAp_AtcDataReqListProc()
{
    ATC_AP_MSG_DATA_REQ_STRU*    pDataReq;
    
    pDataReq = AtcAp_GetFirstAtcDataReqFromList();
    if(pDataReq != NULL)
    {
        if(ATC_AP_TRUE == AtcAp_MsgProc_AtcApDataReq(pDataReq))
        {
            AtcAp_Free(pDataReq);
        }
    }
}

unsigned char AtcAp_NormalMsgDistribute(unsigned char *pucRcvMsg)
{
    ATC_AP_MSG_HEADER_STRU         *pMsgHeader;
    unsigned char                   ucRtnCode   = ATC_AP_TRUE;

    osMutexAcquire(g_AtcApInfo.stAtRspInfo.mutex, osWaitForever);
    if(NULL == g_AtcApInfo.stAtRspInfo.aucAtcRspBuf)
    {
        g_AtcApInfo.stAtRspInfo.aucAtcRspBuf = (unsigned char*)AtcAp_Malloc(D_ATC_RSP_MAX_BUF_SIZE);
    }

    pMsgHeader = (ATC_AP_MSG_HEADER_STRU*)pucRcvMsg;
    if (D_ATC_AP_DATA_REQ == pMsgHeader->ulMsgName)
    {
        ucRtnCode = AtcAp_MsgProc_AtcApDataReq((ATC_AP_MSG_DATA_REQ_STRU*)pucRcvMsg);
    }
    else if(D_ATC_DATA_IND == pMsgHeader->ulMsgName)
    {
        AtcAp_MsgProc_AtcApDataInd((ATC_MSG_DATA_IND_STRU*)pucRcvMsg);
        ucRtnCode = ATC_AP_FALSE;
    }

    if(g_AtcApInfo.msgInfo.ucCnt == 0)
    {
        AtcAp_Free(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    osMutexRelease(g_AtcApInfo.stAtRspInfo.mutex);
    
    return ucRtnCode;
}


