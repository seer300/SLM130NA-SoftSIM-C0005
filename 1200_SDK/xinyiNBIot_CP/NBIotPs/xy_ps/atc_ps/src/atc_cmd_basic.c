/*******************************************************************************
@(#)Copyright(C)2016,HuaChang Technology (Dalian) Co,. LTD.
File name   : atc_cmd_basic.c
Description : 
Function List:
History:
1. Dep2_066    2016.12.20  Create
*******************************************************************************/
#include "atc_ps.h"

#define HWREG(x)    (*((volatile unsigned long *)(x)))

/*******************************************************************************
  MODULE    : ATC_CGSN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGSN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CGSN length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
                                                               
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGSN,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc == D_ATC_CMD_FUNC_SET)
    {
        usCmdStrLen += 1;        
        pucValue     = &(((ST_ATC_CGSN_PARAMETER *)pEventBuffer)->ucSnt);
#if VER_BC25
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 0, 1);    
#else
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 3, 0, 1);    
#endif
    }
    else if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc)
    {
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
    }     
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CEREG_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CEREG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CEREG length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
                                                                
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEREG,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /*  '=' set                             */
        pucValue     = &(((ST_ATC_CEREG_PARAMETER *)pEventBuffer)->ucN);
        pucValueFlag = &(((ST_ATC_CEREG_PARAMETER *)pEventBuffer)->ucNFlag);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 5, 1, 1);     
    }
    return ucResult;  
}

/*******************************************************************************
  MODULE    : ATC_CGATT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGATT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CGATT length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGATT,pEventBuffer,&ucCmdFunc);
    
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 1, 0, 1);        
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CIMI_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CIMI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CIMI length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CIMI,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc == D_ATC_CMD_FUNC_NOEQUAL_SET && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        return  D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGDCONT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGDCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+CGDCONT" length                    */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucPDPtypeData[7] ={0};                                                       /* PDP type                             */
    unsigned char aucApnValue[101] ={0};
    unsigned char ucAnlParLength;
    unsigned char i;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    unsigned char ucMinValue;
    unsigned char ucMaxValue;
    ST_ATC_CGDCONT_PARAMETER* pCgdcontParam = (ST_ATC_CGDCONT_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGDCONT,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1;                                                               /* '=' set                              */
   
    for (; iParamNum < D_ATC_PARAM_MAX_P_CGDCONT && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucCid);
            pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucCidFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0); 
#if VER_BC25
            if(*pucValue < 1 || *pucValue > 7)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
#endif
            break; 
        case 1:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
                 6, &ucParLen, &ucAnlParLength, aucPDPtypeData, &ucStopStrInf);

            if(D_ATC_PARAM_OK == ucParResult)     
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                
                for (i = 0; i < 9; i++)
                {
                    if (0 == AtcAp_Strncmp(aucPDPtypeData, (unsigned char*)ATC_PdpType_Table[i].aucStr))
                    {
                        ((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucPdpTypeValue = ATC_PdpType_Table[i].ucStrVal;
                         ((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucPdpTypeFlg = D_ATC_FLAG_TRUE; /* IP Existence                 */
                        ucResult = D_ATC_COMMAND_OK;
                        break;
                    }
                } 
                if(i == 9)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;                               /* parameter error set                  */
            }
            else
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            
            if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                usCmdStrLen += 1;                                                       /* ',' set                              */
            }

            break;
            
        case 2:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                       D_ATC_P_CGDCONT_APN_SIZE_MAX - 1, &ucParLen, 
                       &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucApnLen),
                       aucApnValue, &ucStopStrInf);

            if(D_ATC_PARAM_OK == ucParResult)     
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                if (0 != pCgdcontParam->ucApnLen)
                {
                    if(pCgdcontParam->ucApnLen <= sizeof(pCgdcontParam->aucApnValue))
                    {
                        AtcAp_MemCpy(pCgdcontParam->aucApnValue, aucApnValue, pCgdcontParam->ucApnLen);
                    }
                    else
                    {
                        pCgdcontParam->pucApnValue = (unsigned char*)AtcAp_Malloc(pCgdcontParam->ucApnLen);
                        AtcAp_MemCpy(pCgdcontParam->pucApnValue, aucApnValue, pCgdcontParam->ucApnLen);
                    }
                }
            }
            else if(D_ATC_PARAM_EMPTY == ucParResult)
            {
                if ((D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    ||(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    ||(D_ATC_STOP_CHAR_SEMICOLON == ucStopStrInf))
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                      /* parameter length set                 */
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }                    
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }

            if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                usCmdStrLen += 1;                                                       /* ',' set                              */
            }
            break;
            
        case 3:
            for (; D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen); usCmdStrLen++)
            {
                if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
                {
                    return D_ATC_COMMAND_OK;
                }
            }
            ucStopStrInf = D_ATC_STOP_CHAR_KANMA;
            usCmdStrLen += 1;                                                       /* ',' set                              */
            break;
#if (!Custom_09 || VER_CTCC)
        case 4:            
#endif
        case 5:
        case 10:
#if VER_CTCC
            switch(iParamNum)
            {
                case 4:            /* <d_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucD_comp);
                    pucValueFlag = NULL;
                    ucMinValue   = 0;
                    ucMaxValue   = 0;
                    break;
                case 5:            /* <h_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_comp);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_compFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 0;
                    break;
                default:           /* <NSLPI> value get                    */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPI);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPIFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;                    
                    break;
            } 
            if(iParamNum == 10)
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 1); 
            }
            else
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 0); 
            }
            
            if(pucValueFlag != NULL) /* default value get                    */
            {
                *pucValueFlag = D_ATC_FLAG_TRUE;
            }          
            break;
#else
        case 11:
            switch(iParamNum)
            {
                case 4:            /* <d_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucD_comp);
                    pucValueFlag = NULL;
                    ucMinValue   = 0;
                    ucMaxValue   = 0;
                    break;
                case 5:            /* <h_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_comp);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_compFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;
                    break;
                case 10:           /* <NSLPI> value get                    */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPI);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPIFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;                    
                    break;
                default:           /* <SecurePco> value get                    */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucSecurePco);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucSecurePcoFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;
                    break;
            } 
#if VER_BC25
            if(iParamNum == 5)
#else
            if(iParamNum == 11)
#endif
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 1); 
            }
            else
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 0); 
            }
            
            if(pucValueFlag != NULL) /* default value get                    */
            {
                *pucValueFlag = D_ATC_FLAG_TRUE;
            }          
            break;
#endif
#if (Custom_09 && !VER_CTCC)
        case 4:            /* <d_comp> value get */
#endif
        case 6:            /* <IPv4AddrAlloc> value get            */
        case 7:            /* <request_type> value get *//*lint !e685 !e568*/                   
        case 8:            /* <P-CSCF_discovery> value get*/
        case 9:            /* <IM_CN_Signalling_Flag_Ind> value get */
            if(D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen) && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            usCmdStrLen += 1;
            break;
        default:
            break;
        }

        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CFUN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CFUN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
    unsigned char iParamNum=0;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CFUN,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    for (; iParamNum < D_ATC_PARAM_MAX_P_CFUN && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:  /* <Fun> value get */
            pucValue  = &(((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun);
#if VER_CTCC
            ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 5, pucValue, NULL, 0, 101, 0, 0);  
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }

            if(0 != *pucValue && 1 != *pucValue && 101 != *pucValue && 4 != *pucValue)
            {
                if(g_AtcApInfo.stAppInterfaceInfo.ucSeqNum != 0 && *pucValue == 5)
                {
                    break;
                }
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
#else
            ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 5, 0, 0);  
    #if VER_BC25
            if(0 != *pucValue && 1 != *pucValue)
            {
                if(g_AtcApInfo.stAppInterfaceInfo.ucSeqNum != 0 && *pucValue == 5)
                {
                    break;
                }
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
    #else
            if(0 != *pucValue && 1 != *pucValue && 5 != *pucValue)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
    #endif
            break;
#endif
        case 1: /* <Rst> value get */
            pucValue  = &(((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucRst);
            ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 1, 1);    
            break;
        default:
            break;
        }          
    }

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CMEE_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CMEE_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CMEE,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;  
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 2, 1, 1);  
    }      
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CLAC_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CLAC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                       /* +CLAC length set                         */
    unsigned char ucResult;                                                              /* command analysis result                  */
    unsigned char ucCmdFunc;                                                             /* command form                             */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CLAC,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CESQ_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CESQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CESQ length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned long ulBinaryData;
    unsigned char ucValueFlag;
    ST_ATC_CESQ_PARAMETER* pCesqParam = (ST_ATC_CESQ_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CESQ,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
#if VER_BC25
    if(ATC_AP_FALSE == g_AtcApInfo.ucUserAtFlg)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#endif
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 5, (unsigned long*)&ulBinaryData, &ucValueFlag, 0, 65535, 1, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    pCesqParam->usTimerVal = (unsigned short)ulBinaryData;
    return ucResult;
}

unsigned char ATC_CSQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 4;                                                              /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSQ,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGPADDR_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGPADDR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* +CGPADDR length                       */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucCidNum = 0;
    unsigned char ucValue;
                                                               
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGPADDR,pEventBuffer,&ucCmdFunc);

    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;

        while (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &ucValue, NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, ucCidNum == D_MAX_CNT_CID - 1 ? 1 : 0);            
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }

            ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->aucCid[ucCidNum] = ucValue;
            ucCidNum++;
        }
        
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_FALSE;
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucCidNum = ucCidNum;
        break;     
#if VER_BC25
    case D_ATC_CMD_FUNC_READ:
        usCmdStrLen += 1;
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            break;
        }
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CGPADDR;
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_TRUE;
        ucResult = D_ATC_COMMAND_OK;
        break; 
#endif
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
        {
            ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_TRUE;
        }
        else
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGACT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGACT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CGACT length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGACT,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;                                                               /*  '=' set                             */

    pucValue     = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucState);
    pucValueFlag = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucStateFlag);
    ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 0);   
    if(ucResult != D_ATC_COMMAND_OK)
    {
        return ucResult;
    } 

    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        pucValue     = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucCid );
        pucValueFlag = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucCidFlag);;
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);   
    }  
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CSODCP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CSODCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                             /* "+CSODCP" length                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned short usBinaryData;

    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    unsigned char  ucParamFlg = 0;
    unsigned char  ucValueFlag;
    ST_ATC_CSODCP_PARAMETER*   pCsodcpParam;
    unsigned short usCpdataLength;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSODCP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */

    pCsodcpParam = (ST_ATC_CSODCP_PARAMETER*)pEventBuffer;
    for (; iParamNum < D_ATC_PARAM_MAX_P_CSODCP && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pCsodcpParam->pucCpdata = NULL;

            pucValue     = &(pCsodcpParam->ucCid);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0);
#if VER_BC25
            if(*pucValue < 1 || *pucValue > 7)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
#endif
            break;
        case 1:
            ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 5,
                &ucParLen, &usBinaryData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen += ucParLen;                                                /* parameter length set                 */
#if VER_BC25
                if (1500 >= usBinaryData)
#else
                if (EPS_CSODCP_CPDATA_LENGTH_MAX >= usBinaryData)
#endif
                {
                    pCsodcpParam->usCpdataLength = usBinaryData;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;                           /* not normal command set               */
                }
            }
            else
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;                               /* not normal command set               */
            }
  
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            usCmdStrLen++;                                                          /* ',' set                              */
            break;

        case 2:
            if(0 != pCsodcpParam->usCpdataLength)
            {
                pCsodcpParam->pucCpdata = (unsigned char*)AtcAp_Malloc(pCsodcpParam->usCpdataLength);
            }
            if(D_ATC_PARAM_OK != ATC_GetHexStrParameter(pCommandBuffer, &usCmdStrLen, pCsodcpParam->usCpdataLength * 2, &usCpdataLength, pCsodcpParam->pucCpdata, 1, 0)
                || usCpdataLength != pCsodcpParam->usCpdataLength)
            {
                if(NULL != pCsodcpParam->pucCpdata)
                {
                    AtcAp_Free(pCsodcpParam->pucCpdata);
                }
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;      
        case 3:
            pucValue     = &(pCsodcpParam->ucRAI);
            pucValueFlag = &(pCsodcpParam->ucRAIFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 2, 1, 0);  

            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }        
            break;
            
        case 4:
            pucValue     = &(pCsodcpParam->ucTUserData);
            pucValueFlag = &(pCsodcpParam->ucTUserDataFlag);
#if VER_BC25
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 1);  
#else
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 0);  
#endif
            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }        
            break;        
        case 5:
            pucValue     = &(pCsodcpParam->ucSequence);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, &ucValueFlag, 1, 255, 1, 1);  

            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }
            break;
        default:
            break;
        }

        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
        ucParamFlg |= (1 << iParamNum);
    }

    if((ucParamFlg & 0x07) != 0x07)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CRTDCP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CRTDCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* +CRTDCP length                       */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CRTDCP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /*  '=' set                             */
        pucValue     = &(((ST_ATC_CRTDCP_PARAMETER *)pEventBuffer)->ucReporting);
        pucValueFlag = &(((ST_ATC_CRTDCP_PARAMETER *)pEventBuffer)->ucReportingFlag);        
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 1);  
    } 
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGAPNRC_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGAPNRC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGAPNRC,pEventBuffer,&ucCmdFunc);
    if(ucCmdFunc == D_ATC_CMD_FUNC_SET)
    {
        usCmdStrLen += 1;        
        pucValue     = &(((ST_ATC_CGAPNRC_PARAMETER *)pEventBuffer)->ucCid);
        pucValueFlag = &(((ST_ATC_CGAPNRC_PARAMETER *)pEventBuffer)->ucCidFlag);        
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);          
    }
    else if(ucCmdFunc == D_ATC_CMD_FUNC_NOEQUAL_SET)
    {
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
    }     
    return ucResult;
}
/*******************************************************************************
  MODULE    : ATC_SIMST_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_SIMST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* ^SIMST command length set            */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_SIMST,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CEDRXS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CEDRXS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* "+CEDRXS" length                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char auceDRXData[5];                                                               /* eDRX data                            */
    unsigned char ucAnlParLength;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucCtr;                                                                        /* count                                */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEDRXS,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CEDRXS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucMode);
            pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucModeFlag);
            *pucValue     = 0;/*default value == 0  */
            *pucValueFlag = D_ATC_FLAG_TRUE;
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 3, 1, 0); 
#if Custom_09
            if(D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
#endif
            break;
        case 1:
            pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucActType);
            pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucActTypeFlg);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 5, 5, 0, 0);             
            break;
            
        case 2:
        case 3:
#if VER_BC25
            if( 3 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
#endif
            if(2 == iParamNum)
            {
                pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucEDRXValue);
                pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucEDRXValueFlag);
            }
            else 
            {
                pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucPtwValue);
                pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucPtwValueFlag);
            }
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                                                    4,
                                                    &ucParLen, &ucAnlParLength,
                                                    auceDRXData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
                if (ucAnlParLength != 4)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                else
                {
                    *pucValueFlag = D_ATC_FLAG_TRUE;
                    for (ucCtr = 0; ucCtr < 4; ucCtr++)
                    {
                        if ((auceDRXData[ucCtr] == '0') || (auceDRXData[ucCtr] == '1'))
                        {
                            auceDRXData[ucCtr] = (unsigned char)(auceDRXData[ucCtr] - '0');
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                        (*pucValue) |= auceDRXData[ucCtr] << (3 - ucCtr);
                    }                    
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }       
       
            if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
#if Custom_09 
                if(2 == iParamNum)
                {
                    return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }
#else
                if(3 == iParamNum)
                {
                    return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }                
#endif
            }
            else
            {
                return ucResult;
            }
            usCmdStrLen++;
            break;         

        default:
            break;
        }
        
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }        
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CPSMS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CPSMS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+CPSMS" length                      */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucReqPeriTAUData[9];                                                         /* Requested_Periodic-TAU data          */
    unsigned char ucAnlParLength;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucCtr;                                                                        /* count                                */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
#if VER_CTCC
    unsigned char ucPsmUnit;
#endif
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CPSMS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CPSMS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucMode);
            pucValueFlag = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucModeFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 2, 1, 0);              
#if VER_BC25
            if((D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen)) && ((*pucValue == 0) || (*pucValue == 2)))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
#endif
            break;
            
        case 1:
        case 2:
            if(D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen) && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            usCmdStrLen++;
            break;
        case 3:
        case 4:
            if(3 == iParamNum)
            {
                pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqPeriTAU);
                pucValueFlag = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqPeriTAUFlg);
            }
            else 
            {
                pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqActTime);
                pucValueFlag = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqActTimeFlag);
            }            
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                8,
                &ucParLen, &ucAnlParLength,
                aucReqPeriTAUData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                
                if (ucAnlParLength != 8)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                else
                {
                    *pucValueFlag = D_ATC_FLAG_TRUE;
                    for (ucCtr = 0; ucCtr < 8; ucCtr++)
                    {
                        if ((aucReqPeriTAUData[ucCtr] == '0') || (aucReqPeriTAUData[ucCtr] == '1'))
                        {
                            aucReqPeriTAUData[ucCtr] = (unsigned char)(aucReqPeriTAUData[ucCtr] - '0');
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                        (*pucValue) |= aucReqPeriTAUData[ucCtr] << (7-ucCtr);
                    }
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }           
#if VER_CTCC
            if(*pucValueFlag == D_ATC_FLAG_TRUE)
            {
                ucPsmUnit = (*pucValue & 0xE0) >> 5;
                if((3 == iParamNum) && (0x07 == ucPsmUnit))
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                if((4 == iParamNum) && (0x03 <= ucPsmUnit) && (0x06 >= ucPsmUnit) )
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
            }
#endif
            if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                if(4 == iParamNum)
                {
                    return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }                
            }
            else
            {
                return ucResult;
            }
            usCmdStrLen++;
            break;          
        default:
            break;
        }
        
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
    }
    return ucResult;
}
/*lint +e438*/

/*******************************************************************************
  MODULE    : ATC_CSCON_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  jiangna   2018.03.07   create
*******************************************************************************/
unsigned char ATC_CSCON_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSCON,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                              */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
#if Custom_09 || VER_BC25
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 1, 1);           
#else
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 3, 1, 1);           
#endif
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_NL2THP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_NL2THP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* command length                       */
    unsigned char ucCmdFunc;                                                                   /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NL2THP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 0, 1);          
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_NSET_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
unsigned char ATC_NSET_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +NSET length                        */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    ST_ATC_NSET_PARAMETER * pNsetParam = NULL;
    unsigned char  ucLastParamFlg;
    unsigned char  ucAllowZeroStartFlg = ATC_FALSE;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NSET,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;  
   
    pNsetParam = (ST_ATC_NSET_PARAMETER *)pEventBuffer;

    for (; iParamNum < D_ATC_PARAM_MAX_P_NSET; iParamNum++)
    {
        switch(iParamNum)
        {
            case 0:
                ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
                                 D_ATC_P_NSET_INS_SIZE_MAX - 1, &ucParLen, &(pNsetParam->ucInsLen), pNsetParam->aucInsValue, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */    
                    if(pNsetParam->ucInsLen != 0 && D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pNsetParam->usEvent = D_ATC_EVENT_NSET_R;
                    }                    
                }
                else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
                {
                    return D_ATC_COMMAND_SYNTAX_ERROR;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }         

                usCmdStrLen += 1;
                if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                {
                    return ucParResult;
                }
                                                                      /* ',' set                              */                
                if(0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETIMEI"))
                {
                    ucAllowZeroStartFlg = ATC_TRUE;
                }
                break;
                
            case 1: 
                if (0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETSN")
                    || 0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETSVN"))
                {
                    ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), NVM_MAX_SN_LEN, &ucParLen, &pNsetParam->ucLen, pNsetParam->aucData, &ucStopStrInf);
                    if(D_ATC_PARAM_OK == ucParResult && D_ATC_DEFAULT_CR == *(pCommandBuffer + usCmdStrLen + ucParLen))
                    {
                                         
                    }
                    else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
                    {
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    return ucResult;                  
                }
                else
                {
                    if(0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETIMEI")
                        || 0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"ULGB"))
                    {
                        ucLastParamFlg = ATC_FALSE;
                    }
                    else
                    {
                        ucLastParamFlg = ATC_TRUE;
                    }
                    ucResult     = ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 8, &(pNsetParam->ulParam1), &(pNsetParam->ucParam1Flg), 0, 0xFFFFFFFF, 0, ucLastParamFlg, ucAllowZeroStartFlg);
                    if(D_ATC_PARAM_OK != ucResult || ATC_TRUE == ucLastParamFlg)
                    {
                        return ucResult;
                    }
                }
                break;
            case 2:
                ucResult     = ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 8, &(pNsetParam->ulParam2), &(pNsetParam->ucParam2Flg), 0, 0xFFFFFFFF, 0, 1, ucAllowZeroStartFlg);          
                if(ucResult != D_ATC_COMMAND_OK)
                {
                    return ucResult;
                }
                break;
            default:
                break;
        }
    }    
    return ucResult;
}



unsigned char ATC_NUESTATS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned short usParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    unsigned char aucType[13] = {0};
    unsigned char i;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NUESTATS,pEventBuffer,&ucCmdFunc);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_NUESTATS_PARAMETER*)pEventBuffer)->mu8Type = ATC_NUESTATS_TYPE_NOPARAMETER;
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        ucParResult = ATC_ChkStrPara((pCommandBuffer + usCmdStrLen), 12, &usParLen, aucType, &ucStopStrInf);
        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
        if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_CR == ucStopStrInf))
        {
            for(i = 0; i < usParLen; i++)
            {
                aucType[i] = toupper(aucType[i]);
            }
            for (i = 0; i < ATC_NUESTATS_MAX; i++)
            {
                if (0 == AtcAp_Strncmp(aucType, (unsigned char*)ATC_NUESTATS_Table[i].aucStr))
                {
                    ((ST_ATC_NUESTATS_PARAMETER*)pEventBuffer)->mu8Type = ATC_NUESTATS_Table[i].ucStrVal;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
            }          
        }  
        break;
    default :
        break;
    }
    return ucResult;
}

unsigned char ATC_NEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+NEARFCN" length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */

    unsigned char u8Mode;
    unsigned long u32Earfcn;
    unsigned short u16Pci;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NEARFCN,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */        
        for (; iParamNum < D_ATC_MAX_PARAM_NUM_3; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 1, &ucParLen, &u8Mode, &ucStopStrInf);
                usCmdStrLen = usCmdStrLen + ucParLen;
                if((D_ATC_PARAM_OK == ucParResult) && (0 == u8Mode) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu8Mode = u8Mode;                    
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                usCmdStrLen+=1;
                break;
                
            case 1:
                ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen, &u32Earfcn, &ucStopStrInf, 0);
                usCmdStrLen = usCmdStrLen + ucParLen;
#if Custom_09
                if((D_ATC_PARAM_OK == ucParResult) && (u32Earfcn <= 262143))
#else
                if(D_ATC_PARAM_OK == ucParResult)
#endif
                {
                    ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu32Earfcn = u32Earfcn;

                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu16Pci = 0xFFFF;
                        return ucResult;
                    }
                    else if(D_ATC_STOP_CHAR_KANMA != ucStopStrInf)
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                usCmdStrLen+=1;
                break;
                
            case 2:
                ucParResult = ATC_CheckHexNumParameter((pCommandBuffer + usCmdStrLen), 3, &ucParLen, &u16Pci, &ucStopStrInf);

                if((D_ATC_PARAM_OK == ucParResult) && (u16Pci <= 0x1F7))
                {
                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                       ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu16Pci = u16Pci;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            default:
                return D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }
    }
    return ucResult;
}

unsigned char ATC_NBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NBAND" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char i = 0;
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NBAND,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        for(; i < 14 && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); i++)
        {
            pucValue     = &(((ST_ATC_NBAND_PARAMETER*)pEventBuffer)->mau8Band[i]);  
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, NULL, 0, 0xFF, 0, i == 13 ? 1 : 0);    
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }            
        }    
        ((ST_ATC_NBAND_PARAMETER*)pEventBuffer)->mu8Num = i;    
    }
    return ucResult;
}

unsigned char ATC_NCONFIG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+NCONFIG" length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStrLen = 0; 
    unsigned char ucParResult;                                                                  /* parameter result                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char aucFunc[40]  = { 0 };
    unsigned long ulVal;
    unsigned char index;
    unsigned char aucValue[6] = { 0 };
    unsigned char i;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCONFIG,pEventBuffer,&ucCmdFunc);
    
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                  */
        for (; iParamNum < D_ATC_MAX_PARAM_NUM_2; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 39, &ucParLen, &ucStrLen, aucFunc, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    for(i = 0; i < ucParLen; i++)
                    {
                        aucFunc[i] = toupper(aucFunc[i]);
                    }
                    for(index = 0; index < D_ATC_NCONFIG_MAX; index++)
                    {
                        if(0 == strcmp((char*)aucFunc, (char*)ATC_NConfig_Table[index].aucStr))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu8Func = ATC_NConfig_Table[index].ucStrVal;
                            break;
                        }
                    }
                
                    if(D_ATC_NCONFIG_MAX == index)
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;               
            case 1:
                if(ATC_AP_TRUE == ATC_NCONFIG_SET_IsStrChk(ATC_NConfig_Table[index].ucStrVal))
                {
                    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 5, &ucParLen, aucValue, 0, 1))
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }

                    for(i = 0; i < ucParLen; i++)
                    {
                        aucValue[i] = toupper(aucValue[i]);
                    }

                    if(ATC_NConfig_Table[index].ucStrVal == D_ATC_NCONFIG_PCO_IE_TYPE)
                    {
                        if(0 == strcmp((char*)aucValue, "EPCO"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 1;
                        }
                        else if(0 == strcmp((char*)aucValue, "PCO"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 0;
                        }
                         else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        if(0 == strcmp((char*)aucValue, "TRUE"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 1;
                        }
                        else if(0 == strcmp((char*)aucValue, "FALSE"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 0;
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                    }
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ulVal, &ucStopStrInf, 0);
                    if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_CR== ucStopStrInf) && ulVal <= 65535)
                    {
                        ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = (unsigned short)ulVal;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                break;
            default:
                return D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }            
    }
    return ucResult;
}

unsigned char ATC_NCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NCCID" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCCID,pEventBuffer,&ucCmdFunc);

    return ucResult;
}


unsigned char ATC_NCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                             /* "+NCSEARFCN" length                      */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCSEARFCN,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_RAI_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
unsigned char ATC_RAI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 4;                                                              /* +RAI length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_RAI,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 0xFF, 0, 1);         
    }   
    return ucResult;
}



unsigned char ATC_CGPIAF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short   usCmdStrLen = 7;                                                               /* command length                       */
    unsigned char   ucResult;                                                                     /* command analysis result              */
    unsigned char   ucCmdFunc;                                                                    /* command form                         */
    unsigned char*   pucValue;
    unsigned char*   pucValueFlag;
    ST_ATC_CGPIAF_PARAMETER*   pCgpiafParam = (ST_ATC_CGPIAF_PARAMETER*) pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGPIAF,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        pucValue = &(pCgpiafParam->ucIpv6AddressFormat);
        pucValueFlag = &(pCgpiafParam->ucIpv6AddressFormatFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6SubnetNotation);
        pucValueFlag = &(pCgpiafParam->ucIpv6SubnetNotationFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6LeadingZeros);
        pucValueFlag = &(pCgpiafParam->ucIpv6LeadingZerosFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6CompressZeros);
        pucValueFlag = &(pCgpiafParam->ucIpv6CompressZerosFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 1);
    }
    
    return ucResult;
}

