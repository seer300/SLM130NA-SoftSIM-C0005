#ifndef _ATC_PS_STRU_H_
#define _ATC_PS_STRU_H_

#ifndef SINGLE_CORE
#include "cmsis_os2.h"
#include "osal_type.h"
#else

#endif
#include "xy_ps_api.h"

/* Size character data table define */
typedef struct {
    unsigned char  *CommandLetter;                                                              /* Valid letter AT Command              */
}ST_ATC_CMD_LTTR_TBL;

/* Command analysis jump table define */
typedef struct {
    ST_ATC_CMD_LTTR_TBL  LetterData;                                                            /* Valid letter Data                    */
    unsigned char  (*CommandProc)(unsigned char *, unsigned char *);                            /* Command Procedure                    */
}ST_ATC_COMMAND_ANAL_TABLE;

typedef struct
{
    unsigned char                               ucFucSet;
    unsigned char                               ucFucRead;
    unsigned char                               ucFucTest;
    unsigned char                               ucFucNoQualSet;
} ST_ATC_EVENT_TABLE;

typedef struct
{
    unsigned char                               ucFac;
    unsigned char                               aucFac[3];
} ST_ATC_FAC_TABLE;

typedef struct
{
    unsigned char                               ucStrVal;
    unsigned char                               aucStr[12];
} ST_ATC_STR_TABLE;

/* ATC_StrPrintf struct */
typedef struct  
{
    unsigned char  *FormatRef;
    unsigned char  *EditBufRef;
    unsigned short  EditCount;
    unsigned char   ConvType;
} ST_ATC_AP_FORMAT;

typedef struct 
{
    unsigned short  usEvent;
    unsigned char            (*CommandProc)(unsigned char *);
} ST_ATC_AP_CMD_PROC_TABLE;

typedef struct
{
    unsigned short  usEvent;
    void            (*CommandProc)(unsigned char *);
} ST_ATC_AP_MSG_PROC_TABLE;

typedef struct
{
    unsigned char                       ucStrVal;
    unsigned char                       aucStr[35];
} ST_ATC_AP_STR_TABLE;

typedef struct
{
    unsigned long         MsgPointer;
}ST_ATC_AP_MSG_TYPE;

typedef struct
{
    unsigned char    ucCascadeAtFlg; 
    unsigned char    ucCascadeAtCnt;
    unsigned char    *pCasaadeAtBuff;
    unsigned short   usLen;
    unsigned short   offset;
} ST_ATC_CASCADE_AT_INFO;

typedef struct
{
    osMutexId_t             mutex;
    unsigned char           ucHexStrLen;
#define D_ATC_RSP_MAX_BUF_SIZE          668
    unsigned char*          aucAtcRspBuf;
    unsigned short          usRspLen;
} ST_ATC_AP_RSP_INFO;

typedef struct st_atc_ap_msg_node
{
    struct st_atc_ap_msg_node*     next;
    unsigned long           ulMsg;
} ST_ATC_AP_MSG_NODE;

typedef struct {
    osMutexId_t             msgMutex;
    osSemaphoreId_t         msgSemaPhore;
    unsigned char           ucCnt;
    ST_ATC_AP_MSG_NODE*     head;
    ST_ATC_AP_MSG_NODE*     last;
} ST_ATC_AP_MSG_INFO;

typedef struct atc_ap_link_node
{
    struct atc_ap_link_node* next;
    struct atc_ap_link_node* prev;
}ST_ATC_AP_LINK_NODE;

typedef struct
{
    ST_ATC_AP_LINK_NODE     node;
    unsigned char           ucSeqNum;   //1-255
    unsigned long           ulSemaphore;
#define  D_APP_INTERFACE_RESULT_SUCC    0
#define  D_APP_INTERFACE_RESULT_FAIL    1
    unsigned char           ucResult;
    unsigned short          usDataLen;
    unsigned char*          pucData;
} ST_ATC_AP_APP_INTERFACE_NODE;

typedef struct
{
    ST_ATC_AP_LINK_NODE     node;
    unsigned long           eventId;
    xy_psEventCallback_t    callback;
} ST_ATC_AP_PS_EVENT_REGISTER_INFO;

typedef struct
{
    ST_ATC_AP_LINK_NODE         node;
    ATC_AP_MSG_DATA_REQ_STRU*   pDataReq;
} ST_ATC_AP_ATC_DATA_REQ_NODE;

typedef struct
{
    unsigned char                  ucSeqNum;
    osMutexId_t                    mutex;
    ST_ATC_AP_APP_INTERFACE_NODE*  pHead;
    ST_ATC_AP_APP_INTERFACE_NODE*  pTail;
} ST_ATC_AP_APP_INTERFACE_INFO;

typedef struct
{
    ST_ATC_AP_ATC_DATA_REQ_NODE*   pHead;
    ST_ATC_AP_ATC_DATA_REQ_NODE*   pTail;
} ST_ATC_AP_ATC_DATA_REQ_INFO; 

typedef struct
{
    unsigned short              usCnfLen;
    unsigned char*              pAtCnfBuf;
} ST_ATCNF_INFO;

typedef struct
{
    unsigned int                ucAtTotalLogsize;
    unsigned char               ucAtLogCount;
    unsigned char               ucBufferNum;
#define AT_LOG_CACHE_MAX_NUM     10
    ST_ATCNF_INFO               *pucAtLogCacheList;
} ST_ATCNF_CACHE_INFO;

typedef struct
{
    unsigned char           ucErrMode;
    unsigned char           ucXmlElementLevel;
    unsigned short          usCurrEvent;
    unsigned short          usCurrRspEvent;
    unsigned char           ucUserAtFlg;
    unsigned char           ucWaitOKOrErrorFlg;
    unsigned char           ucAtHeaderSpaceFlg;
    unsigned char           ucWaitSendCpinOKFlg;
    unsigned char           ucTempSeqNum;
    unsigned char           ucSimReadyFlg;
    unsigned char           ucSimURCFlg;
    ST_ATC_CASCADE_AT_INFO  atCascateInfo;
    ST_ATC_AP_MSG_INFO      msgInfo;
    ST_ATC_AP_MSG_INFO      MsgInfo_EventCb;
    ST_ATC_AP_RSP_INFO      stAtRspInfo;
    ST_ATC_CMD_COM_EVENT*   pCurrEvent;
    ST_ATC_AP_APP_INTERFACE_INFO stAppInterfaceInfo;
    ST_ATC_AP_PS_EVENT_REGISTER_INFO* pEventRegList;
    ST_ATC_AP_ATC_DATA_REQ_INFO  stAtcDataReqList;
    ST_ATCNF_CACHE_INFO     stAtLogCacheList;
} ST_ATC_AP_INFO;

typedef struct 
{
    unsigned short                              usErrCode;
    unsigned char                              *pCmeErrorText;
} ST_ATC_GLOBLE_ERROR_CAUSE_TABLE;

#endif
