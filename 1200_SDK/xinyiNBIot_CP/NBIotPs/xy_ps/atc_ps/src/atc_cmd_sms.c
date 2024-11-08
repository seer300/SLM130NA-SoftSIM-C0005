/*******************************************************************************
@(#)Copyright(C)2016,HuaChang Technology (Dalian) Co,. LTD.
File name   : atc_cmd_sms.c
Description : 
Function List:
History:
1. Dep2_066    2016.12.20  Create
*******************************************************************************/
#include "atc_ps.h"

#ifdef NBIOT_SMS_FEATURE

/*lint -e438*/
/*******************************************************************************
  MODULE    : ATC_CSMS_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CSMS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSMS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;
        pValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 1, 0, 1);
    }
    return ucResult; 
}

/*******************************************************************************
  MODULE    : ATC_CMGF_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CMGF_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CMGF,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;
        pValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 1, 0, 1);      
     }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CNMA_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CNMA_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen = 5; 
    unsigned char                           ucResult;
    unsigned char                           ucCmdFunc;
    unsigned char                           ucValFlg;
    ST_ATC_PDU_TPDU_PARAMETER*              pCnmaParam;
   
    pCnmaParam = (ST_ATC_PDU_TPDU_PARAMETER*)pEventBuffer;
    AtcAp_MemSet(pCnmaParam, 0, sizeof(ST_ATC_PDU_TPDU_PARAMETER));

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CNMA,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_PARAM_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCnmaParam->ucNum, &ucValFlg, 0, 2, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    if(D_ATC_PARAM_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCnmaParam->ucPduLength, &ucValFlg, 0, 255, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_CSCA_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                      usCmdStrLen = 5;             //+CSCA
    unsigned char                       ucCmdFunc;
    unsigned char                       ucResult;
    ST_ATC_CSCA_PARAMETER*      pCscaParam;
   
    pCscaParam = (ST_ATC_CSCA_PARAMETER*)pEventBuffer;
    AtcAp_MemSet(pCscaParam, 0, sizeof(ST_ATC_CSCA_PARAMETER));
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_CSCA, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_PARAM_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 21, &pCscaParam->ucScaLen, pCscaParam->aucScaData, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    if(D_ATC_PARAM_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCscaParam->ucTosca, &pCscaParam->ucToscaFlag, 0, 255, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

/*******************************************************************************
  MODULE    : ATC_CMGS_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CMGS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;
    ST_ATC_PDU_TPDU_PARAMETER*  pCmgsParam;

    pCmgsParam = (ST_ATC_PDU_TPDU_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CMGS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                              */
        pValue     = &pCmgsParam->ucPduLength;
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 255, 0, 1);  
        if(ucResult == D_ATC_COMMAND_OK)
        {
            ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            if (((*pValue >= 7) && (*pValue <= 164))  || (0 == *pValue))
            {
                ucResult = D_ATC_COMMAND_OK;
            }            
        }
    }
    return ucResult;
}

unsigned char ATC_CMGC_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                      usCmdStrLen   = 5;//+CMGC length
    unsigned char                       ucCmdFunc;
    unsigned char                       ucResult;
    ST_ATC_PDU_TPDU_PARAMETER*      pCmgcParam;

    pCmgcParam = (ST_ATC_PDU_TPDU_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_CMGC, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCmgcParam->ucPduLength, NULL, 0, 255, 0, 1);
    if(D_ATC_COMMAND_OK != ucResult || pCmgcParam->ucPduLength < 7 ||  pCmgcParam->ucPduLength > 164)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_CMMS_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                      usCmdStrLen   = 5;//+CMMS length
    unsigned char                       ucCmdFunc;
    unsigned char                       ucResult;
    ST_ATC_CMMS_PARAMETER*      pCmmsParam;

    pCmmsParam = (ST_ATC_CMMS_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_CMMS, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCmmsParam->ucVal, NULL, 0, 2, 0, 1);
    if(D_ATC_COMMAND_OK != ucResult)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    return D_ATC_COMMAND_OK;
}

/*lint +e438*/
#endif
