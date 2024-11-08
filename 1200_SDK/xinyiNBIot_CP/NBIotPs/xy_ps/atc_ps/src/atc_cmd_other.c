/*******************************************************************************
@(#)Copyright(C)2016,HuaChang Technology (Dalian) Co,. LTD.
File name   : atc_cmd_basic.c
Description : 
Function List:
History:
1. Dep2_066    2016.12.20  Create
*******************************************************************************/
#include "atc_ps.h"

#ifdef ESM_DEDICATED_EPS_BEARER
/*lint -e438*/
/*******************************************************************************
  MODULE    : ATC_CGDSCONT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGDSCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* "+CGDSCONT" length                    */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pValue;
    unsigned char *pValueFlag;
    unsigned char ucMin;
    unsigned char ucMax;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGDSCONT,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CGDSCONT && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
            case 0:/* <cid> normal value                   */
                pValue     = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucCid);
                pValueFlag = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucCidFlag);
                ucMin      = D_ATC_MIN_CID;
                ucMax      = D_ATC_MAX_CID-1;
                break;
            case 1: /* <p_cid> normal value   */
                pValue     = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucP_cid);
                pValueFlag = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucP_cidFlg);  
                ucMin      = D_ATC_MIN_CID;
                ucMax      = D_ATC_MAX_CID-1;
                break;
            case 2:/* <d_comp> value get */
                pValue     = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucD_comp);
                pValueFlag = NULL;  
                ucMin      = 0;
                ucMax      = 0;
                break;
            default: /* <h_comp> value get */
                pValue     = &(((ST_ATC_CGDSCONT_PARAMETER *)pEventBuffer)->ucH_comp);
                pValueFlag = NULL;  
                ucMin      = 0;
                ucMax      = 0;
                break;
        }        
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, ucMin, ucMax, 1, (D_ATC_PARAM_MAX_P_CGDSCONT-1 == iParamNum) ? 1 : 0);    

        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }     
    }        
    return ucResult;
}
#endif

#ifdef EPS_BEARER_TFT_SUPPORT
/*******************************************************************************
  MODULE    : ATC_CGTFT_LNB_Command
  FUNCTION  : 
  NOTE      : Modifying the code from "case4" to "case10" for TTCN scripts 22.6.2 and 22.6.3,
              modify the return value of every "case D_ATC_PARAM_EMPTY" to make it "D_ATC_COMMAND_OK"
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGTFT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{    
    unsigned short usCmdStrLen = 6;                                                              /* "+CGTFT" length                      */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    //unsigned char ucBinaryData;                                                                 /* binary data                          */
    unsigned long ulBinaryData;
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char *pucRemoteAddressAndSubnetMaskValue;
    unsigned char ausLocalPortRangeValue[D_ATC_P_CGTFT_LOCAL_PORT_RANG_SIZE_MAX+1] = {0};
    unsigned char ausRemotePortRangeValue[D_ATC_P_CGTFT_REMOTE_PORT_RANG_SIZE_MAX+1] = {0};
    unsigned char aucTosAndMask_TracfficClassAndMaskValue[D_ATC_P_CGTFT_TOSANDMASK_TRACFFICCLASSANDMAK_SIZE_MAX+1] = {0};
    unsigned char *pucLocalAddressAndSubnetMaskValue;
    unsigned char ucFieldNum = 1;                                                               /* PDP address field number             */
    unsigned char aucFieldCnt[32] ={0};                                                         /* Digit number of each field           */
    unsigned char ucAnlParLength;
    unsigned char ucOffset = 0;
    unsigned char i;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    signed int iJudgmentResult;                                                                /* judge result                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char *pValue;
    unsigned long *pulValue;
    unsigned char *pValueFlag;
    unsigned long ulMin;
    unsigned long ulMax;
    unsigned int uiFigure;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGTFT,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 
    

    for (; iParamNum < D_ATC_PARAM_MAX_P_CGTFT && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
        case 1:
        case 2:
        case 4:
        case 10:
            switch(iParamNum)
            {
                case 0:/* <cid> normal value                   */
                    pValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucCid);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucCidFlag);
                    ulMin      = D_ATC_MIN_CID;
                    ulMax      = D_ATC_MAX_CID-1;
                    break;
                case 1: /* <PacketFilterIdentifier> normal value   */
                    pValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucPacketFilterIdentifier);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucPacketFilterIdentifierFlag);  
                    ulMin      = 1;
                    ulMax      = 16;
                    break;
                case 2:/* get <evaluation precedence index*/
                    pValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucEvaluationPrecedenceIndex);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucEvaluationPrecedenceIndexFlg);  
                    ulMin      = 0;
                    ulMax      = 255;
                    break;
                case 4: /* get <evaluation precedence index> */
                    pValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucProtocolNumber_NextHeader);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucProtocolNumber_NextHeaderFlag);  
                    ulMin      = 0;
                    ulMax      = 255;
                    break;
                default:/* <direction> normal value                       */
                    pValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucDirection);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucDirectionFlag);  
                    ulMin      = 0;
                    ulMax      = 3;
                    break;  
            }
            
            ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, ulMin, ulMax, 1, 0);
            if(ucResult == D_ATC_COMMAND_OK)
            {
                continue;
            }
            break;

        case 3:
            pucRemoteAddressAndSubnetMaskValue = (unsigned char *)
                AtcAp_Malloc((D_ATC_P_CGTFT_REMOTEADDRESSANDSUBNETMASK_SIZE_MAX+1));

            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CGTFT_REMOTEADDRESSANDSUBNETMASK_SIZE_MAX, &ucParLen, &ucAnlParLength,
                pucRemoteAddressAndSubnetMaskValue, &ucStopStrInf);
            usCmdStrLen += ucParLen;                          /* parameter length set                 */
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:                
                iJudgmentResult = AtcAp_Strncmp(pucRemoteAddressAndSubnetMaskValue, (unsigned char *)"");
                if (0 == iJudgmentResult)
                {
                    ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucRemoteAddressAndSubnetMaskLen = 0;
                    ATC_Strncpy((unsigned char *)(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucRemoteAddressAndSubnetMaskValue),
                        (unsigned char *)"\0", 1);
                }
                else
                {
                    for (i = 0; i <= ucAnlParLength; i++)
                    {
                        aucFieldCnt[ucFieldNum-1]++;
                        if (*(pucRemoteAddressAndSubnetMaskValue + i) == '.')
                        {
                            ucFieldNum++;
                        }
                    }
                    if ((ucFieldNum != 8)&&(ucFieldNum != 32))
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else if(ucFieldNum != 32)
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucRemoteAddressAndSubnetMaskLen = 8;
                        for (i = 0; i < 8; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 4) ||
                                (ATC_OK != ATC_CharToBinary((pucRemoteAddressAndSubnetMaskValue + ucOffset), (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucRemoteAddressAndSubnetMaskValue[i]), 0)))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    else
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucRemoteAddressAndSubnetMaskLen = 32;
                        for (i = 0; i < 32; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 4) ||
                                (ATC_OK != ATC_CharToBinary((pucRemoteAddressAndSubnetMaskValue + ucOffset), (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucRemoteAddressAndSubnetMaskValue[i]), 0)))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    ucFieldNum = 1;
                    AtcAp_MemSet(aucFieldCnt, 0 ,sizeof(aucFieldCnt));
                    ucOffset = 0;
                }
                break;
            case D_ATC_PARAM_ERROR:                                                     /* no define                            */
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:                                              /* parameter error                      */
            case D_ATC_PARAM_EMPTY:       
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            AtcAp_Free(pucRemoteAddressAndSubnetMaskValue);            
            break;
            
        case 5:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CGTFT_LOCAL_PORT_RANG_SIZE_MAX, &ucParLen, &ucAnlParLength,
                ausLocalPortRangeValue, &ucStopStrInf);
            usCmdStrLen += ucParLen;                          /* parameter length set                 */
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                iJudgmentResult = AtcAp_Strncmp(ausLocalPortRangeValue, (unsigned char *)"");
                if (0 == iJudgmentResult)
                {
                    ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucLocalPortRangeLen = 0;
                    ATC_Strncpy((unsigned char *)(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ausLocalPortRangeValue),
                        (unsigned char *)"\0", 1);
                }
                else
                {
                    for (i = 0; i <= ucAnlParLength; i++)
                    {
                        aucFieldCnt[ucFieldNum-1]++;
                        if (ausLocalPortRangeValue[i] == '.')
                        {
                            ucFieldNum++;
                        }
                    }
                    if (ucFieldNum != 2)
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucLocalPortRangeLen = 2;
                        for (i = 0; i < 2; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 6) ||
                                (ATC_OK != ATC_ShortToBinary(&ausLocalPortRangeValue[ucOffset], (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ausLocalPortRangeValue[i]))))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    ucFieldNum = 1;
                    AtcAp_MemSet(aucFieldCnt, 0 ,sizeof(aucFieldCnt));
                    ucOffset = 0;
                }
                break;
            case D_ATC_PARAM_ERROR:                                                     /* no define                            */
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:                                              /* parameter error                      */
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break; 
            
        case 6:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CGTFT_REMOTE_PORT_RANG_SIZE_MAX, &ucParLen, &ucAnlParLength,
                ausRemotePortRangeValue, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                iJudgmentResult = AtcAp_Strncmp(ausRemotePortRangeValue, (unsigned char *)"");
                if (0 == iJudgmentResult)
                {
                    //((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucRemotePortRangeLen = 0;
                    ATC_Strncpy((unsigned char *)(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ausRemotePortRangeValue),
                        (unsigned char *)"\0", 1);
                }
                else
                {
                    for (i = 0; i <= ucAnlParLength; i++)
                    {
                        aucFieldCnt[ucFieldNum-1]++;
                        if (ausRemotePortRangeValue[i] == '.')
                        {
                            ucFieldNum++;
                        }
                    }
                    if (ucFieldNum != 2)
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucRemotePortRangeLen = 2;
                        for (i = 0; i < 2; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 6) 
                                || (ATC_OK != ATC_ShortToBinary(&ausRemotePortRangeValue[ucOffset], (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ausRemotePortRangeValue[i]))))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    ucFieldNum = 1;
                    AtcAp_MemSet(aucFieldCnt, 0 ,sizeof(aucFieldCnt));
                    ucOffset = 0;
                }
                break;
            case D_ATC_PARAM_ERROR:                                                     /* no define                            */
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:                                              /* parameter error                      */
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break; 

        case 7: 
        case 9:            
            switch(iParamNum)
            {
                case 7:/* get <ipsec security parameter index (spi)> */
                    pulValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ulIpsecSpi);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucIpsecSpiFlag);
                    ulMin      = 0;
                    ulMax      = 0xFFFFFFFF;
                    uiFigure   = D_ATC_CGTFT_PARAM_SPI_LEN;
                    break;
                 default:
                    pulValue     = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ulFlowLabel);
                    pValueFlag = &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucFlowLabelFlag);
                    ulMin      = 0;
                    ulMax      = 0xFFFFF;    
                    uiFigure   = D_ATC_CGTFT_PARAM_FLOW_LABEL_LEN;
                    break;
             }
            ucParResult = ATC_CheckHexParameter((pCommandBuffer + usCmdStrLen), uiFigure,
                &ucParLen, &ulBinaryData, &ucStopStrInf);
            usCmdStrLen += ucParLen;  
            if(D_ATC_PARAM_OK == ucParResult)
            {
                if ((ulBinaryData >= ulMin) && (ulBinaryData <= ulMax))                /*lint !e685 !e568*/
                {
                    *pulValue = ulBinaryData;
                    if(pValueFlag != NULL)
                    {
                        *pValueFlag = D_ATC_FLAG_TRUE;
                    } 
                }
                else                                                                    
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;                           /* not normal command set               */
                }                
            }
            else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;  
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR; 
            }
            break;          
     
        case 8: 
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CGTFT_TOSANDMASK_TRACFFICCLASSANDMAK_SIZE_MAX, &ucParLen, &ucAnlParLength,
                aucTosAndMask_TracfficClassAndMaskValue, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                iJudgmentResult = AtcAp_Strncmp(aucTosAndMask_TracfficClassAndMaskValue, (unsigned char *)"");
                if (0 == iJudgmentResult)
                {
                    //((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucTosAndMask_TracfficClassAndMaskLen = 0;
                    ATC_Strncpy((unsigned char *)(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucTosAndMask_TracfficClassAndMaskValue),
                        (unsigned char *)"\0", 1);
                }
                else
                {
                    for (i = 0; i <= ucAnlParLength; i++)
                    {
                        aucFieldCnt[ucFieldNum-1]++;
                        if (aucTosAndMask_TracfficClassAndMaskValue[i] == '.')
                        {
                            ucFieldNum++;
                        }
                    }
                    if (ucFieldNum != 2)
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucTosAndMask_TracfficClassAndMaskLen = 2;
                        for (i = 0; i < 2; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 4) ||
                                (ATC_OK != ATC_CharToBinary(&aucTosAndMask_TracfficClassAndMaskValue[ucOffset], (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucTosAndMask_TracfficClassAndMaskValue[i]),0)))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    ucFieldNum = 1;
                    AtcAp_MemSet(aucFieldCnt, 0 ,sizeof(aucFieldCnt));
                    ucOffset = 0;
                }
                break;
            case D_ATC_PARAM_ERROR:                                                     /* no define                            */
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:                                              /* parameter error                      */
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }            
            break; 
  
        case 11:
            pucLocalAddressAndSubnetMaskValue = (unsigned char *)
                AtcAp_Malloc((D_ATC_P_CGTFT_LOCALADDRESSANDSUBNETMASK_SIZE_MAX+1));

            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CGTFT_LOCALADDRESSANDSUBNETMASK_SIZE_MAX, &ucParLen, &ucAnlParLength,
                pucLocalAddressAndSubnetMaskValue, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                iJudgmentResult = AtcAp_Strncmp(pucLocalAddressAndSubnetMaskValue, (unsigned char *)"");
                if (0 == iJudgmentResult)
                {
                    //((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucLocalAddressAndSubnetMaskLen = 0;
                    ATC_Strncpy((unsigned char *)(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucLocalAddressAndSubnetMaskValue),
                        (unsigned char *)"\0", 1);
                }
                else
                {
                    for (i = 0; i <= ucAnlParLength; i++)
                    {
                        aucFieldCnt[ucFieldNum-1]++;
                        if (*(pucLocalAddressAndSubnetMaskValue +i) == '.')
                        {
                            ucFieldNum++;
                        }
                    }
                    if ((ucFieldNum != 8)&&(ucFieldNum != 32))
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else if(ucFieldNum != 32)
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucLocalAddressAndSubnetMaskLen = 8;
                        for (i = 0; i < 8; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 4) ||
                                (ATC_OK != ATC_CharToBinary((pucLocalAddressAndSubnetMaskValue + ucOffset), (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucLocalAddressAndSubnetMaskValue[i]),0)))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    else
                    {
                        ((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->ucLocalAddressAndSubnetMaskLen = 32;
                        for (i = 0; i < 32; i++)
                        {
                            if ((aucFieldCnt[i] < 2) || (aucFieldCnt[i] > 4) ||
                                (ATC_OK != ATC_CharToBinary((pucLocalAddressAndSubnetMaskValue + ucOffset), (unsigned char)(aucFieldCnt[i]-1), 
                                &(((ST_ATC_CGTFT_PARAMETER *)pEventBuffer)->aucLocalAddressAndSubnetMaskValue[i]),0)))
                            {
                                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                                break;
                            }
                            ucOffset = (unsigned char)(ucOffset + aucFieldCnt[i]);
                        }
                    }
                    ucFieldNum = 1;
                    AtcAp_MemSet(aucFieldCnt, 0 ,sizeof(aucFieldCnt));
                    ucOffset = 0;
                }
                break;
            case D_ATC_PARAM_ERROR:                                                     /* no define                            */
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:                                              /* parameter error                      */
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            AtcAp_Free(pucLocalAddressAndSubnetMaskValue);
            break;
        default:
            break;
        }

        if (D_ATC_STOP_CHAR_CR == ucStopStrInf || D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
        else
        {
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            else if(D_ATC_PARAM_MAX_P_CGTFT-1 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            usCmdStrLen += 1;                                                       /* ',' set                              */
        }
    }
    return ucResult;
}
#endif

/*******************************************************************************
  MODULE    : ATC_CGEQOS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGEQOS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* "+CGEQOS" length                     */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pValue;
    unsigned char *pValueFlag;
    unsigned long ulMin;
    unsigned long ulMax;
    unsigned int  uiFigure;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGEQOS,pEventBuffer,&ucCmdFunc);    
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 

    for (; iParamNum < D_ATC_PARAM_MAX_P_CGEQOS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pValue     = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucCid);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucCidFlag);
            ulMin      = D_ATC_MIN_CID;
            ulMax      = D_ATC_MAX_CID-1;
            uiFigure   = 3;
            break;            
        case 1:
            pValue     = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucQci);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucQciFlag);
            ulMin      = 0;
            ulMax      = 0xFF;
            uiFigure   = 3;            
            break;
        case 2:
            pValue     = (unsigned char*)&(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ulDl_Gbr);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucDl_GbrFlag);
            ulMin      = 0;
            ulMax      = D_ATC_DL_GBR_MAX;
            uiFigure   = 8;            
            break;
        case 3:
            pValue     = (unsigned char*)&(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ulUl_Gbr);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucUl_GbrFlag);
            ulMin      = 0;
            ulMax      = D_ATC_UL_GBR_MAX;
            uiFigure   = 8;            
            break;
        case 4:
            pValue     = (unsigned char*)&(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ulDl_Mbr);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucDl_MbrFlag);
            ulMin      = 0;
            ulMax      = D_ATC_DL_MBR_MAX;
            uiFigure   = 8;            
            break;
        case 5:
            pValue     = (unsigned char*)&(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ulUl_Mbr);
            pValueFlag = &(((ST_ATC_CGEQOS_PARAMETER *)pEventBuffer)->ucUl_MbrFlag);
            ulMin      = 0;
            ulMax      = D_ATC_UL_MBR_MAX;
            uiFigure   = 8;            
            break;
        default:
            break;
        }

        if(uiFigure <= 3)
        {
            ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, uiFigure, pValue, pValueFlag, ulMin, ulMax, 1, (D_ATC_PARAM_MAX_P_CGEQOS-1 == iParamNum) ? 1 : 0);   
        }
        else
        {
            ucResult = ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, uiFigure, (unsigned long*)pValue, pValueFlag, ulMin, ulMax, 1, (D_ATC_PARAM_MAX_P_CGEQOS-1 == iParamNum) ? 1 : 0, 0);
        }

        if (D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
     }
    return ucResult;
}

#ifdef ESM_EPS_BEARER_MODIFY
/*******************************************************************************
  MODULE    : ATC_CGCMOD_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGCMOD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* command length                       */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pValue;
    unsigned char *pValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGCMOD,pEventBuffer,&ucCmdFunc);
       
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /*  '=' set                             */
        pValue     = &(((ST_ATC_CGCMOD_PARAMETER *)pEventBuffer)->ucCid);
        pValueFlag = &(((ST_ATC_CGCMOD_PARAMETER *)pEventBuffer)->ucCidFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1); 
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }
    return ucResult;
}
#endif

/*******************************************************************************
  MODULE    : ATC_COPS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_COPS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* "+COPS" length                       */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucOperData[8];                                                               /* oper data                                */
    unsigned char ucAnlParLength;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucCtr;                                                                        /* count                                */
    unsigned char ucOperStrCtr;                                                                 /* oper count                           */
    
    unsigned char *pValue = NULL;
    unsigned char *pValueFlag = NULL;
    unsigned long ulMin = 0;
    unsigned long ulMax = 0;
    unsigned int  uiFigure;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_COPS,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_COPS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
        case 1:
        case 3:
            uiFigure = 3;
            switch(iParamNum)
            {
            case 0:
                pValue     = &(((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucMode);
                pValueFlag = NULL;
#if Custom_09 
                ulMin      = 0;
                ulMax      = 2;
#else
                ulMin      = 0;
                ulMax      = 4;
#endif
                break; 
            case 1:
                pValue     = &(((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucFormat);
                pValueFlag = &(((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucFormatFlag);
                ulMin      = 2;
                ulMax      = 2;
                break;  
            case 3:
                pValue     = &(((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucAct);
                pValueFlag = &(((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucActFlg);
                ulMin      = 9;
                ulMax      = 9;
                break; 
            default:
                break;
            }
            ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, uiFigure, pValue, pValueFlag, ulMin, ulMax, 1, (D_ATC_PARAM_MAX_P_COPS-1 == iParamNum) ? 1 : 0);
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }

            if(0 == iParamNum && (0 == *pValue || 2 == *pValue))
            {
                if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) ||  D_ATC_N_COMMA == *(pCommandBuffer + usCmdStrLen - 1))
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
            }
            continue;
        case 2:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                                                    D_ATC_P_COPS_OPER_SIZE_MAX,
                                                    &ucParLen, &ucAnlParLength,
                                                    aucOperData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
                ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->ucPerFlag = D_ATC_FLAG_TRUE;
                if ((ucAnlParLength != 5)
                    &&(ucAnlParLength != 6))
                {
                    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                }
                else
                {
                    for (ucOperStrCtr = 0; ucOperStrCtr < 3; ucOperStrCtr++)
                    {
                        ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[ucOperStrCtr] = 0;
                    }
                    for (ucCtr = 0; ucCtr < ucAnlParLength; ucCtr++)
                    {
                        if ((aucOperData[ucCtr] >= '0') && (aucOperData[ucCtr] <= '9'))
                        {
                            aucOperData[ucCtr] = (unsigned char)(aucOperData[ucCtr] - '0');
                        }
                        else if ((aucOperData[ucCtr] >= 'A') && (aucOperData[ucCtr] <= 'F'))
                        {
                            aucOperData[ucCtr] = (unsigned char)((aucOperData[ucCtr] - 'A') + 10);
                        }
                        else if ((aucOperData[ucCtr] >= 'a') && (aucOperData[ucCtr] <= 'f'))
                        {
                            aucOperData[ucCtr] = (unsigned char)((aucOperData[ucCtr] - 'a') + 10);
                        }
                        else
                        {
                            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                            break;
                        }

                    }
                    ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[0] |= (aucOperData[1] & 0x0F) << 4;
                    ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[0] |= (aucOperData[0] & 0x0F);
                    if (5 == ucAnlParLength)
                    {
                        ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[1] |= 0xF0;
                    }
                    else
                    {
                        ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[1] |= (aucOperData[5] & 0x0F) << 4;
                    }
                    ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[1] |= (aucOperData[2] & 0x0F);
                    ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[2] |= (aucOperData[4] & 0x0F) << 4;
                    ((ST_ATC_COPS_PARAMETER *)pEventBuffer)->aucPer[2] |= (aucOperData[3] & 0x0F);
                }
                break;
            case D_ATC_PARAM_EMPTY:                                                     /* no break                             */
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break;         

        default:
            break;
        }
        
        if (D_ATC_STOP_CHAR_CR == ucStopStrInf || D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
        else
        {
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            else if(D_ATC_PARAM_MAX_P_COPS-1 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            usCmdStrLen += 1;                                                       /* ',' set                              */
        }        
    }
    return ucResult;
}

//shao add for USAT
/*******************************************************************************
  MODULE    : ATC_CSIM_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CSIM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned char ucCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                        /* command analysis result              */
    unsigned long ulBinaryData = 0;
    unsigned char ucStopStrInf = 0;  
    unsigned char ucParLen = 0;
    unsigned short usParLen = 0;
    unsigned char ucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;                                       /* command form                         */                                      
    unsigned short usCmdLen = 0;
    unsigned char* pucCommand;
    ST_ATC_CSIM_PARAMETER*   pCsimParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + ucCmdStrLen, EVENT_CSIM, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    ucCmdStrLen += 1;

    pCsimParam = (ST_ATC_CSIM_PARAMETER*)pEventBuffer;
    if(D_ATC_PARAM_OK != ATC_CheckLongNumParameter((pCommandBuffer + ucCmdStrLen), 3, &ucParLen, &ulBinaryData, &ucStopStrInf, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    ucCmdStrLen += (ucParLen + 1);

    if(0 != (ulBinaryData % 2) || ulBinaryData < 8 || ulBinaryData > D_ATC_AP_USIM_MAX_APDU_SIZE * 2)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    pCsimParam->usLength = (unsigned short)(ulBinaryData / 2);
    if(pCsimParam->usLength <= sizeof(pCsimParam->aucCommand))
    {
        pucCommand = pCsimParam->aucCommand;
    }
    else
    {
        pCsimParam->pucCommand = (unsigned char*)AtcAp_Malloc(pCsimParam->usLength);
        pucCommand = pCsimParam->pucCommand;
    }

    if(D_ATC_PARAM_OK != ATC_CheckHexStrParameter((pCommandBuffer + ucCmdStrLen), D_ATC_AP_USIM_MAX_APDU_SIZE * 2, &usParLen, &usCmdLen, pucCommand, &ucStopStrInf))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(pCsimParam->usLength != usCmdLen)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

/*******************************************************************************
  MODULE    : ATC_CCHO_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CCHO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned char ucCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                        /* command analysis result              */
    unsigned char ucStopStrInf = 0;
    unsigned char ucParResult = D_ATC_PARAM_SYNTAX_ERROR;                                       /* parameter result                     */
    unsigned short usParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;                                       /* command form                         */
    signed int   iParamNum;                                           /* PARAMETER NO                         */
    unsigned char aucCmd[16] = {0};
    unsigned short usCmdLen = 0;

    AtcAp_MemSet(((ST_ATC_CCHO_PARAMETER *)pEventBuffer)->aucDfName, 0, 16);
    ucResult = ATC_CmdFuncInf(pCommandBuffer + ucCmdStrLen, EVENT_CCHO, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }

    ucCmdStrLen += 1;
    for (iParamNum = 0; iParamNum < D_ATC_PARAM_MAX_P_CCHO; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            ucParResult = ATC_CheckHexStrParameter((pCommandBuffer + ucCmdStrLen), 32, 
                &usParLen, &usCmdLen, aucCmd, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + usParLen);
                if ((usCmdLen >= 1) && (usCmdLen <= 16))
                {
                    AtcAp_MemCpy(((ST_ATC_CCHO_PARAMETER *)pEventBuffer)->aucDfName, aucCmd, usCmdLen);
                    ((ST_ATC_CCHO_PARAMETER *)pEventBuffer)->ucLength = usCmdLen;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CCHO;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CCHO_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_P_CCHO;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CCHO_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                break;
            }

            if (D_ATC_STOP_CHAR_CR != ucStopStrInf)
            {
                ucCmdStrLen += 1;
                iParamNum = D_ATC_PARAM_MAX_P_CCHO;
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                break;
            }
            break;
        default:
            break;
        }
    }
    return ucResult;
}
/*******************************************************************************
  MODULE    : ATC_CCHC_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CCHC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned char ucCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                        /* command analysis result              */
    unsigned char ucBinaryData = 0;
    unsigned char ucStopStrInf = 0;
    unsigned char ucParResult = D_ATC_PARAM_SYNTAX_ERROR;                                       /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;                                       /* command form                         */
    signed int   iParamNum = 0;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + ucCmdStrLen, EVENT_CCHC, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }

    ucCmdStrLen += 1;
    for (; iParamNum < D_ATC_PARAM_MAX_P_CCHC; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 2, 
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            if (D_ATC_PARAM_OK == ucParResult)
            {
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if ((ucBinaryData > 0) && (ucBinaryData < 20))//UICC_MAX_SELECTED_APP_NUM
                {
                    ((ST_ATC_CCHC_PARAMETER *)pEventBuffer)->ucSesionId = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                    if (D_ATC_STOP_CHAR_CR != ucStopStrInf)
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CCHC;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "\n[LnbAtc]!!!!!!!!!ATC_CCHC_LNB_Command line = %d!!!!!!!!!!!!!\n", __LINE__);
                }
            }
            else
            {
                iParamNum = D_ATC_PARAM_MAX_P_CCHC;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CCHC_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
            }
            break;

        default:
            break;
        }
    }
    return ucResult;
}
/*******************************************************************************
  MODULE    : ATC_CGLA_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CGLA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                        /* command analysis result              */
    unsigned char ucBinaryData = 0;
    unsigned long ulBinaryData = 0;
    unsigned char ucStopStrInf = 0;
    unsigned char ucParResult = D_ATC_PARAM_SYNTAX_ERROR;                                       /* parameter result                     */
    unsigned short usParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;                                       /* command form                         */
    signed int    iParamNum = 0;                                                                /* PARAMETER NO                         */
    unsigned char* pData     = NULL ;
    unsigned short usCmdLen = 0;
    ST_ATC_CGLA_PARAMETER* pCglaParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_CGLR, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }

    pCglaParam = (ST_ATC_CGLA_PARAMETER*)pEventBuffer;
    usCmdStrLen += 1;
    for (; iParamNum < D_ATC_PARAM_MAX_P_CGLA; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
                (unsigned char*)&usParLen, &ucBinaryData, &ucStopStrInf);
            if (D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + usParLen);
                if ((ucBinaryData > 0) && (ucBinaryData < 20))
                {
                    ((ST_ATC_CGLA_PARAMETER *)pEventBuffer)->ucSesionId = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                    if (D_ATC_STOP_CHAR_KANMA != ucStopStrInf)
                    {
                        iParamNum++;
                        ucResult = D_ATC_COMMAND_OK;
                        break;
                    }
                    else
                    {
                        usCmdStrLen += 1;                                                   /* ',' set                              */
                        continue;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "\n[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!\n", __LINE__);
                }
            }
            else
            {
                iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
            }
            break;
        case 1:
            ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 3, (unsigned char*)&usParLen, &ulBinaryData, &ucStopStrInf, 0);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + usParLen);
                if ((ulBinaryData >= 4) && (ulBinaryData <= D_ATC_AP_USIM_MAX_APDU_SIZE * 2) && (ulBinaryData % 2) == 0)
                {
                    ((ST_ATC_CGLA_PARAMETER *)pEventBuffer)->usLength = ulBinaryData / 2;
                    usCmdStrLen += 1;                                                   /* ',' set                              */
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                break;
            }
            break;
        case 2:
            pData = (unsigned char*)AtcAp_Malloc(D_ATC_AP_USIM_MAX_APDU_SIZE);
            ucParResult = ATC_CheckHexStrParameter((pCommandBuffer + usCmdStrLen), D_ATC_AP_USIM_MAX_APDU_SIZE * 2, 
                &usParLen, &usCmdLen, pData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen = (unsigned char)(usCmdStrLen + usParLen);
                if ((usCmdLen >= 4) && (usCmdLen <= D_ATC_AP_USIM_MAX_APDU_SIZE) && (((ST_ATC_CGLA_PARAMETER *)pEventBuffer)->usLength == usCmdLen))
                {
                    if(usCmdLen <= sizeof(pCglaParam->aucCommand))
                    {
                        AtcAp_MemCpy(pCglaParam->aucCommand, pData, usCmdLen);
                    }
                    else
                    {
                        pCglaParam->pucCommand = (unsigned char*)AtcAp_Malloc(usCmdLen);
                        AtcAp_MemCpy(pCglaParam->pucCommand, pData, usCmdLen);
                    }
                    
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                //PS_TRACE_ATCAP(0, NAS_THREAD_ID, FATAL_LOG, "[LnbAtc]!!!!!!!!!ATC_CGLA_LNB_Command line = %d!!!!!!!!!!!!!", __LINE__);
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
                {
                    usCmdStrLen += 1;
                    iParamNum = D_ATC_PARAM_MAX_P_CGLA;
                    ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }
            }
            AtcAp_Free(pData);
            break;
        default:
            break;
        }
    }
    return ucResult;
}


/*******************************************************************************
  MODULE    : ATC_CRSM_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CRSM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned char ucCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                        /* command analysis result              */
    unsigned char ucBinaryData = 0;
    unsigned char ucStopStrInf = 0;
    unsigned char ucParResult = D_ATC_PARAM_SYNTAX_ERROR;                                       /* parameter result                     */
    unsigned char ucParLen = 0;
    unsigned short usParLen = 0;
    unsigned char ucCmdFunc = D_ATC_CMD_FUNC_NOEQUAL_SET;                                       /* command form                         */
    signed int   iParamNum = 0;                                                                /* PARAMETER NO                         */
    unsigned char aucCmd[250] = {0};
    unsigned short usCmdLen = 0;
    unsigned long ulBinaryData = 0;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + ucCmdStrLen, EVENT_CRSM, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    ucCmdStrLen += 1;

    for (; iParamNum < D_ATC_PARAM_MAX_P_CRSM; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3, 
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            if (D_ATC_PARAM_OK == ucParResult)
            {
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucCommand = ucBinaryData;
            }
            else
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            
            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;
            break;
        case 1:
            ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + ucCmdStrLen), 5, 
                &ucParLen, &ulBinaryData, &ucStopStrInf, 0);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (ulBinaryData <= 65535)
                {
                    /*
                    (2FE2)12258 meaning ICCID file 
                    (6F37) 28471 meaning ACMmax file  
                    (6F07) 28423 meaning IMSI file  
                    (6F39) 28473 meaning ACM file  
                    (6F3A) 28474 meaning ADN file(SIM phone book)
                    (6F40) 28480 meaning MSISDN file. 
                    (6F41) 28481 meaning PUKT file  
                    (6F42) 28476 meaning SMS file  
                    (6F46) 28486 meaning SPN file   
                    (6FAD) 28589 meaning EFAD (Administrative data) 
                    (6FC9) 28617 meaning EF
                    */
                    ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ulField = ulBinaryData;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }
            
            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;
            break;
        case 2:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3, 
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucP1 = ucBinaryData;
                break;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }
         
            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;
            break;
        case 3:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3, 
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucP2 = ucBinaryData;
                break;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }
            
            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;
            break;
        case 4:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3, &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);               
                ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucP3 = ucBinaryData;
                break;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }
        
            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;
            break;
        case 5:
            ucParResult = ATC_CheckHexStrParameter((pCommandBuffer + ucCmdStrLen), 500, 
                &usParLen, &usCmdLen, aucCmd, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + usParLen);
                if ((usCmdLen >= 1) && (usCmdLen <= 250 ))
                {
                    ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->pucData = (unsigned char*)AtcAp_Malloc(usCmdLen);
                    AtcAp_MemCpy(((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->pucData, aucCmd, usCmdLen);
                    ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucDataLen = usCmdLen;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }

            if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                return D_ATC_COMMAND_OK;
            }
            ucCmdStrLen++;          
            break;
        case 6:
            ucParResult = ATC_CheckHexStrParameter((pCommandBuffer + ucCmdStrLen), 12, 
                &usParLen, &usCmdLen, aucCmd, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + usParLen);
                AtcAp_MemCpy(((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->aucPathId, aucCmd, usCmdLen);
                ((ST_ATC_CRSM_PARAMETER *)pEventBuffer)->ucPathLen = usCmdLen;
                break;
            case D_ATC_PARAM_ERROR:
                return D_ATC_COMMAND_PARAMETER_ERROR;
            default:
                break;
            }
       
            if (D_ATC_STOP_CHAR_CR != ucStopStrInf)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }      
            break;
        default:
            break;
        }
   }

    return D_ATC_COMMAND_OK;
}

/*******************************************************************************
  MODULE    : ATC_CGEREP_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   yuhao   2018.01.03   create
*******************************************************************************/
unsigned char ATC_CGEREP_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* +CGEREP length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char *pValue;
    unsigned char *pValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGEREP,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                               */
        pValue     = &(((ST_ATC_CGEREP_PARAMETER *)pEventBuffer)->ucMode);
        pValueFlag = &(((ST_ATC_CGEREP_PARAMETER *)pEventBuffer)->ucModeFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, 0, 1, 1, 1);          
    }
    return ucResult;    
}
//
/*******************************************************************************
  MODULE    : ATC_CCIOTOPT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  jiangna   2018.11.07.05   create
*******************************************************************************/
unsigned char ATC_CCIOTOPT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    signed int   iParamNum = 0;
    unsigned char *pValue;
    unsigned char *pValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CCIOTOPT,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                              */
        for (;iParamNum < D_ATC_PARAM_MAX_P_CCIOTOPT && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen);iParamNum++)
        {
            switch(iParamNum)
            {
                case 0:
                    pValue     = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucN);
                    pValueFlag = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucNFlag);
#if VER_BC25
                    ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, pValueFlag, 0, 2, 1, 0);
                    if(ucResult == D_ATC_COMMAND_OK && *pValue == 2)
                    {
                        *pValue = 3;
                    }
#else
                    ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, pValueFlag, 0, 3, 1, 0);
                    if(ucResult == D_ATC_COMMAND_OK && *pValue == 2)/* 0,1,3 */
                    {   
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
#endif
                    break;
                case 1:
                    pValue     = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucSupportUeOpt);
                    pValueFlag = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucSuppUeOptFlag);
                    ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, pValueFlag, 1, 3, 1, 0);/* 1,3 */
                    if(ucResult == D_ATC_COMMAND_OK && *pValue == 2)
                    {   
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    break;
                case 2:
                    pValue     = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucPreferredUeOpt);
                    pValueFlag = &(((ST_ATC_CCIOTOPT_PARAMETER *)pEventBuffer)->ucPreUeOptFlag);
                    ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, pValueFlag, 1, 2, 1, 1);/* 1,2 */
                    break;
                default:
                    break;
            }        
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }
        }       
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CEDRXRDP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  tianchengbin   2018.11.26   create
*******************************************************************************/
unsigned char ATC_CEDRXRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEDRXRDP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }    
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGEQOSRDP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  tianchengbin   2018.11.26   create
*******************************************************************************/
unsigned char ATC_CGEQOSRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;
    unsigned char *pValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGEQOSRDP,pEventBuffer,&ucCmdFunc);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;
        pValue     = &(((ST_ATC_CGEQOSRDP_PARAMETER *)pEventBuffer)->ucCid);
        pValueFlag = &(((ST_ATC_CGEQOSRDP_PARAMETER *)pEventBuffer)->ucCidFlag);;
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);   
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }
    return ucResult;
}
/*******************************************************************************
  MODULE    : ATC_CTZR_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  JN   2018.11.26   create
*******************************************************************************/
unsigned char ATC_CTZR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CTZR,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;
        pValue     = &(((ST_ATC_CTZR_PARAMETER *)pEventBuffer)->ucReporting);
#if VER_BC25
        ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, NULL, 0, 1, 1, 1);
#else
        ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue, NULL, 0, 3, 1, 1); 
#endif
    }
    return ucResult;
}

/*******************************************************************************
MODULE    : ATC_CGCONTRDP_LNB_Command
FUNCTION  : 
NOTE      :
HISTORY   :
1.  JN   2018.11.26   create
*******************************************************************************/
unsigned char ATC_CGCONTRDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pValue;
    unsigned char *pValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGCONTRDP,pEventBuffer,&ucCmdFunc);

    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;
        pValue     = &(((ST_ATC_CGCONTRDP_PARAMETER *)pEventBuffer)->ucCid);
        pValueFlag = &(((ST_ATC_CGCONTRDP_PARAMETER *)pEventBuffer)->ucCidFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, pValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);          
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        }
        break;
    default:
        break;
    }
    return ucResult;
}

#ifdef LCS_MOLR_ENABLE
unsigned char ATC_CMOLR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{                                                                 /* command length                       */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucBinaryData;                                                                 /* binary data                          */
    unsigned short usBinaryData;                                                                /* binary data                          */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    signed int iParamNum;                                                                      /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char aucNmeaReqData[NMEA_STRING_LEN];
    unsigned char aucThirdPartyAddressData[THIRD_PARTY_ADDRESS_LEN];
    unsigned char ucCmdStrLen = 6;                                                                    /* "+CMOLR" length                      */
    unsigned char ucNmeaRepLen;
    unsigned char ucThirdPartyAddLen;
    ST_ATC_CMOLR_PARAMETER* pMoLRReq;
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + ucCmdStrLen, EVENT_CMOLR,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    ucCmdStrLen += 1;

    pMoLRReq = (ST_ATC_CMOLR_PARAMETER*)pEventBuffer;
    for (iParamNum = 0; iParamNum < D_ATC_PARAM_MAX_P_CMOLR; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pMoLRReq->usEvent = D_ATC_EVENT_CMOLR;
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (3 >= ucBinaryData )
                {
                    pMoLRReq->ucEnableReportPos = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucEnableReportPos = 0;                   /*default value == 0                     */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 1:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (6 >= ucBinaryData)
                {
                    pMoLRReq->ucMethod = ucBinaryData;
                    pMoLRReq->ucMethodFlg = PS_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 2:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (1 >= ucBinaryData)
                {
                    pMoLRReq->ucHorAccSet = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucHorAccSet = 0;                   /*default value == 0                     */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 3:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (127 >= ucBinaryData)
                {
                    pMoLRReq->ucHorAcc = ucBinaryData;
                    pMoLRReq->ucHorAccFlg = PS_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                //pMoLRReq->ucHorAcc = D_ATC_OMIT_VAL_P_CMOLR_HOR_ACC;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 4:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (1 >= ucBinaryData)
                {
                    pMoLRReq->ucVerReq = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucVerReq = 0;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 5:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (1 >= ucBinaryData)
                {
                    pMoLRReq->ucVerAccSet = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucVerAccSet = 0;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 6:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (127 >= ucBinaryData)
                {
                    pMoLRReq->ucVerAcc = ucBinaryData;
                    pMoLRReq->ucVerAccFlg = PS_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                //pMoLRReq->ucVerAcc = D_ATC_OMIT_VAL_P_CMOLR_VER_ACC;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 7:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (4 >= ucBinaryData)
                {
                    pMoLRReq->ucVelReq = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucVelReq = 0;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 8:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (1 >= ucBinaryData)
                {
                    pMoLRReq->ucRepMode = ucBinaryData;
//                    pMoLRReq->ucRepModeFlg = PS_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucRepMode = 0;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 9:
            ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + ucCmdStrLen), 5,
                &ucParLen, &usBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if ((1 <= usBinaryData) && (65535 >= usBinaryData))
                {
                    pMoLRReq->usTimeOut = usBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 10:
            ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + ucCmdStrLen), 5,
                &ucParLen, &usBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if ((1 <= usBinaryData) && (65535 >= usBinaryData))
                {
                    pMoLRReq->usInterval = usBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 11:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if(0 < ucBinaryData && ucBinaryData <= 127)
                {
                    pMoLRReq->ucShapeRep = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 12:
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + ucCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (1 >= ucBinaryData)
                {
                    pMoLRReq->ucPlane = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                pMoLRReq->ucPlane = 0;                   /*default value                      */
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;                                                          /* ',' set                              */
                continue;
            }
            break;
        case 13:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + ucCmdStrLen),
                100, &ucParLen, &ucNmeaRepLen, aucNmeaReqData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                /*
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (0 != ((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucNmeaRepLen)
                {
                    AtcAp_MemCpy(((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->aucNmeaRep,aucNmeaReqData,((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucNmeaRepLen);
                }
                */
                break;
            case D_ATC_PARAM_EMPTY:
                //((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucNmeaRepLen = 0;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                    break;
                }
                ucCmdStrLen++;
                continue;
            }
            break;
        case 14:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + ucCmdStrLen),
                100, &ucParLen, &ucThirdPartyAddLen, aucThirdPartyAddressData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                /*
                ucCmdStrLen = (unsigned char)(ucCmdStrLen + ucParLen);
                if (0 != ((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucThirdPartyAddLen)
                {
                    AtcAp_MemCpy(((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->aucThirdPartyAddress,aucThirdPartyAddressData,((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucThirdPartyAddLen);
                }*/
                break;
            case D_ATC_PARAM_EMPTY:
                //((ST_ATC_CMOLR_PARAMETER *)pEventBuffer)->ucThirdPartyAddLen = 0;
                ucResult = D_ATC_COMMAND_OK;
                ucCmdStrLen++;
                continue;
            case D_ATC_PARAM_ERROR:
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMOLR;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_CR)
                {
                    ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                    break;
                }
            }
            break;
        }
    }      
    
    return ucResult;
}
#endif

/*******************************************************************************
  MODULE    : ATC_NFPLMN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_NFPLMN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* command length                       */          
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */     

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NFPLMN,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CPWD_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   tianchengbin   2018.12.18   create
*******************************************************************************/
unsigned char ATC_CPWD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucAnlParLength;                                                               /*  analysis parameter length               */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    unsigned char aucFacData[3] = {0};
    unsigned char aucPassWd[9] = {0};
    unsigned char ucChkRst = ATC_OK;
    unsigned char i = 0;
    unsigned char *PwdLen;
    unsigned char *pPwd;
    signed int iFigure;

    AtcAp_MemSet(((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->aucOldPwd, 0xff, D_ATC_P_CPWD_OLDPWD_SIZE_MAX);
    AtcAp_MemSet(((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->aucNewPwd, 0xff, D_ATC_P_CPWD_NEWPWD_SIZE_MAX);

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CPWD,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CPWD; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0 :
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                2, &ucParLen, &ucAnlParLength, aucFacData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK :
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
                if (aucFacData[0] >= 'a' && aucFacData[0] <= 'z')
                {
                    aucFacData[0] -= 0x20;
                }
                if (aucFacData[1] >= 'a' && aucFacData[1] <= 'z')
                {
                    aucFacData[1] -= 0x20;
                }
                for (i = 0; i < D_ATC_FAC_NUM; i++)
                {
                    if (0 == AtcAp_Strncmp((unsigned char *)aucFacData, (unsigned char *)ATC_Fac_Table[i].aucFac))
                    {
                        ((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->ucFac = ATC_Fac_Table[i].ucFac;
                        ucResult = D_ATC_COMMAND_OK;
                        break;
                    }
                    if (i == (D_ATC_FAC_NUM - 1))
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                break;
            case D_ATC_PARAM_EMPTY :
            case D_ATC_PARAM_SYNTAX_ERROR :
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            case D_ATC_PARAM_ERROR :
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            default:
                break;
            }
            break;
            
        case 1:
        case 2:
            if(iParamNum == 1)
            {
                PwdLen  = &((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->ucOldPwdLen;
                pPwd    = ((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->aucOldPwd;
                iFigure = D_ATC_P_CPWD_OLDPWD_SIZE_MAX;               
            }
            else
            {
                PwdLen  = &((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->ucNewPwdLen;
                pPwd    = ((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->aucNewPwd;
                iFigure = D_ATC_P_CPWD_NEWPWD_SIZE_MAX;                 
            }
            AtcAp_MemSet(aucPassWd, 0, sizeof(aucPassWd));
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                iFigure, 
                &ucParLen, &ucAnlParLength, 
                aucPassWd, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK :
                usCmdStrLen += ucParLen;
                *PwdLen = ucAnlParLength;
                ucChkRst = ATC_PassWordStrCheck(aucPassWd, pPwd);
                if (ATC_OK != ucChkRst)
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }      
                break;
            case D_ATC_PARAM_ERROR :
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_EMPTY :
            case D_ATC_PARAM_SYNTAX_ERROR :
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break;
            
        default:
            break;
        }
        
        if (D_ATC_STOP_CHAR_CR == ucStopStrInf || D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
        else
        {
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            else if(D_ATC_PARAM_MAX_P_CPWD-1 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            usCmdStrLen += 1;                                                       /* ',' set                              */
        }   
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CPIN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   tianchengbin   2018.12.18   create
*******************************************************************************/
unsigned char ATC_CPIN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucAnlParLength;                                                               /*  analysis parameter length               */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    unsigned char aucPinData[9] = {0};
    unsigned char ucChkRst = ATC_OK;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */

    unsigned char *pPin;
    unsigned char *pPinFlag;
    signed int iFigure;

    AtcAp_MemSet(((ST_ATC_CPIN_PARAMETER *)pEventBuffer)->aucPin, 0xff, D_ATC_P_CPIN_PIN_SIZE_MAX);
    AtcAp_MemSet(((ST_ATC_CPIN_PARAMETER *)pEventBuffer)->aucNewPin, 0xff, D_ATC_P_CPIN_NEWPIN_SIZE_MAX);

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CPIN,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 

    for (; iParamNum < D_ATC_PARAM_MAX_P_CPIN; iParamNum++)
    {
        switch(iParamNum)
        {          
        case 0:
        case 1:
            if(iParamNum == 0)
            {
                pPin     = ((ST_ATC_CPIN_PARAMETER *)pEventBuffer)->aucPin;
                pPinFlag = NULL;
                iFigure  = D_ATC_P_CPIN_PIN_SIZE_MAX;               
            }
            else
            {
                pPin     = ((ST_ATC_CPIN_PARAMETER *)pEventBuffer)->aucNewPin;
                pPinFlag = &(((ST_ATC_CPIN_PARAMETER *)pEventBuffer)->ucNewPinFlg);
                iFigure  = D_ATC_P_CPIN_NEWPIN_SIZE_MAX;                     
            }

            AtcAp_MemSet(aucPinData, 0, sizeof(aucPinData));
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                iFigure, 
                &ucParLen, &ucAnlParLength, 
                aucPinData, &ucStopStrInf);            

            switch(ucParResult)
            {
            case D_ATC_PARAM_OK :
                usCmdStrLen += ucParLen;
                if (ucAnlParLength < 4)
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                else
                {
                    ucChkRst = ATC_PassWordStrCheck(aucPinData, pPin);
                    if (ATC_OK != ucChkRst)
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    else if(pPinFlag != NULL)
                    {
                        *pPinFlag = D_ATC_FLAG_TRUE;
                    }   
                }   
                break;
            case D_ATC_PARAM_ERROR :
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_EMPTY :
                if(iParamNum == 0)
                {
                    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                }
                break;
            case D_ATC_PARAM_SYNTAX_ERROR :
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break;
            
        default:
            break;
        }
        
        if (D_ATC_STOP_CHAR_CR == ucStopStrInf || D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
        else
        {
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            else if(D_ATC_PARAM_MAX_P_CPIN-1 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            usCmdStrLen += 1;                                                       /* ',' set                              */
        }   
    }
    return ucResult;    
}

/*******************************************************************************
  MODULE    : ATC_CLCK_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.   tianchengbin   2018.12.18   create
*******************************************************************************/
unsigned char ATC_CLCK_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CLCK length                             */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucAnlParLength;                                                               /*  analysis parameter length               */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    unsigned char aucFacData[3] = {0};
    unsigned char aucPassWd[9] = {0};
    unsigned char ucChkRst;
    unsigned char i = 0;
    unsigned char *pValue;

    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    AtcAp_MemSet(((ST_ATC_CLCK_PARAMETER *)pEventBuffer)->aucPassWd, 0xff, D_ATC_P_CLCK_PASSWD_SIZE_MAX);

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CLCK,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1; 
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CLCK && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0 :
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                2, &ucParLen, &ucAnlParLength, aucFacData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK :
                usCmdStrLen += ucParLen;
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                for (i = 0; i < D_ATC_FAC_NUM; i++)
                {
                    if (0 == AtcAp_Strncmp((unsigned char *)aucFacData, (unsigned char *)ATC_Fac_Table[i].aucFac))
                    {
                        ((ST_ATC_CPWD_PARAMETER *)pEventBuffer)->ucFac = ATC_Fac_Table[i].ucFac;
                        ucResult = D_ATC_COMMAND_OK;
                        break;
                    }           
                }
                break;
            case D_ATC_PARAM_ERROR :
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_EMPTY :
            case D_ATC_PARAM_SYNTAX_ERROR :
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break;
            
        case 1 :
            pValue     = &(((ST_ATC_CLCK_PARAMETER *)pEventBuffer)->ucMode);
            ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 2, 0, 0);  
            if(ucResult == D_ATC_COMMAND_OK)
            {
                continue;
            }
            break;
        case 2 :
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                D_ATC_P_CLCK_PASSWD_SIZE_MAX, 
                &ucParLen, &ucAnlParLength, 
                aucPassWd, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK :
                usCmdStrLen += ucParLen;
                ((ST_ATC_CLCK_PARAMETER *)pEventBuffer)->ucPassWdFlag = D_ATC_FLAG_TRUE;
                ((ST_ATC_CLCK_PARAMETER *)pEventBuffer)->ucPassWdLen = ucAnlParLength;
                ucChkRst = ATC_PassWordStrCheck(aucPassWd, ((ST_ATC_CLCK_PARAMETER *)pEventBuffer)->aucPassWd);
                if (ATC_OK != ucChkRst)
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_ERROR :
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR :
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        
        if (D_ATC_STOP_CHAR_CR == ucStopStrInf || D_ATC_COMMAND_OK != ucResult) 
        {
            return ucResult;
        }
        else
        {
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            else if(D_ATC_PARAM_MAX_P_CLCK-1 == iParamNum)
            {
                return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            usCmdStrLen += 1;                                                       /* ',' set                              */
        }           
    }

    return ucResult;
}

unsigned char ATC_CEER_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen   = 5;//+CEER length
    unsigned char                    ucCmdFunc;

    return ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEER, pEventBuffer,&ucCmdFunc);
}

unsigned char ATC_CIPCA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen   = 6;//+CIPCA length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucParamFlg;
    unsigned char                    ucResult;
    ST_ATC_CIPCA_PARAMETER*  pCipca;

    pCipca = (ST_ATC_CIPCA_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CIPCA, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
  
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCipca->ucN, &ucParamFlg, 3, 3, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCipca->ucAttWithoutPDN, &ucParamFlg, 0, 1, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_CGAUTH_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen   = 7;//+CGAUTH length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucParamFlg;
    unsigned char                    ucResult;
    ST_ATC_CGAUTH_PARAMETER* pCgauth;

    pCgauth = (ST_ATC_CGAUTH_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGAUTH,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    
    //cid
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCgauth->ucCid, &ucParamFlg, D_ATC_MIN_CID, D_ATC_MAX_CID - 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //auth prot
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pCgauth->ucAuthProt), &ucParamFlg, 0, 1, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //username
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pCgauth->ucUsernameLen), pCgauth->aucUsername, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //password
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pCgauth->ucPasswordLen), pCgauth->aucPassword, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_CNMPSD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen   = 7;//+CNMPSD length
    unsigned char                    ucCmdFunc;
    
    return ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_CNMPSD, pEventBuffer, &ucCmdFunc);
}

unsigned char ATC_CPINR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen   = 6;//+CPINR length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucResult;
    ST_ATC_CPINR_PARAMETER*  pPinr;

    pPinr = (ST_ATC_CPINR_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_CPINR, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc && D_ATC_CMD_FUNC_NOEQUAL_SET != ucCmdFunc)
    {
        return ucResult;
    }
    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc)
    {
        pPinr->ucNoParamValue = 1;
        return D_ATC_COMMAND_OK;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_ATC_MAX_SEL_CODE_LEN, &(pPinr->ucLen), pPinr->aucSelCode, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NPOWERCLASS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 12;//+NPOWERCLASS length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucParamFlg;
    unsigned char                           ucResult;
    ST_ATC_NPOWERCLASS_PARAMETER*   pPowerClass;

    pPowerClass = (ST_ATC_NPOWERCLASS_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NPOWERCLASS, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pPowerClass->ucBand), &ucParamFlg, 0, 255, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pPowerClass->ucPowerClass), &ucParamFlg, 0, 255, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

static char ATC_ConvertBinaryStr(unsigned char* pBinaryStr, unsigned char ucStrLen, unsigned char ucDesLen, unsigned char* pbData, unsigned char* pbDataFlg)
{
    unsigned char i;

    if(0 == ucStrLen)
    {
        return D_ATC_PARAM_OK;
    }
    
    if(ucStrLen != ucDesLen)
    {
        return D_ATC_PARAM_ERROR;
    }

    *pbDataFlg = D_ATC_FLAG_TRUE;
    
    for (i = 0; i < ucDesLen; i++)
    {
        if ((pBinaryStr[i] != '0') && (pBinaryStr[i] != '1'))
        {
            return D_ATC_PARAM_ERROR;
        }
        *pbData |= ((pBinaryStr[i] - '0') << (3 - i)); 
    }
    
    return D_ATC_PARAM_OK;
    
}

unsigned char ATC_NPTWEDRXS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 10;//+NPTWEDRXS length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucResult;
    ST_ATC_CEDRXS_PARAMETER*        pCedrxParam;
    unsigned char                           ucParLen;
    unsigned char                           aucBinaryData[4] = { 0 };

    pCedrxParam = (ST_ATC_CEDRXS_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVNET_NPTWEDRXS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pCedrxParam->ucMode), &pCedrxParam->ucModeFlag, 0, 3, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pCedrxParam->ucActType), &pCedrxParam->ucActTypeFlg, 5, 5, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 4, &ucParLen, aucBinaryData, 1, 0)
        || D_ATC_PARAM_OK != ATC_ConvertBinaryStr(aucBinaryData, ucParLen, 4, &(pCedrxParam->ucPtwValue), &(pCedrxParam->ucPtwValueFlag)))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 4, &ucParLen, aucBinaryData, 1, 1)
        || D_ATC_PARAM_OK != ATC_ConvertBinaryStr(aucBinaryData, ucParLen, 4, &(pCedrxParam->ucEDRXValue), &(pCedrxParam->ucEDRXValueFlag)))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NPIN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 5;//+NPIN length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucResult;
    unsigned char                           ucParamFlg;
    ST_ATC_NPIN_PARAMETER*          pNpinParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NPIN, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pNpinParam = (ST_ATC_NPIN_PARAMETER*)pEventBuffer;
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNpinParam->ucCommand, &ucParamFlg, 0, 4, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_ATC_MAX_PIN_LEN, &pNpinParam->ucParam1Len, pNpinParam->aucParam1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    pNpinParam = (ST_ATC_NPIN_PARAMETER*)pEventBuffer;
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_ATC_MAX_PIN_LEN, &pNpinParam->ucParam2Len, pNpinParam->aucParam2, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if((pNpinParam->ucParam1Len > 0 && pNpinParam->ucParam1Len < 4) || (pNpinParam->ucParam2Len > 0 && pNpinParam->ucParam2Len < 4))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NTSETID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 8;//+NTSETID length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucResult;
    unsigned char                           ucParamFlg;
    ST_ATC_NTSETID_PARAMETER*       pNtSetIdParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NTSETID, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pNtSetIdParam = (ST_ATC_NTSETID_PARAMETER*)pEventBuffer;  
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNtSetIdParam->ucSnt, &ucParamFlg, 1, 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 15, &pNtSetIdParam->ucDataLen, pNtSetIdParam->aucData, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NCIDSTATUS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 11;//+NCIDSTATUS length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucResult;
    unsigned char                           ucParamFlg;
    ST_ATC_NCIDSTATUS_PARAMETER*    pCidStatusParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NCIDSTATUS, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pCidStatusParam = (ST_ATC_NCIDSTATUS_PARAMETER*)pEventBuffer;
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCidStatusParam->ucCid, &ucParamFlg, D_ATC_MIN_CID, D_ATC_MAX_CID - 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NGACTR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                          usCmdStrLen   = 7;//+NGACTR length
    unsigned char                           ucCmdFunc;
    unsigned char                           ucResult;
    unsigned char                           ucParamFlg;
    ST_ATC_NGACTR_PARAMETER*        pNgactrParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NGACTR, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pNgactrParam = (ST_ATC_NGACTR_PARAMETER*)pEventBuffer;
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNgactrParam->n, &ucParamFlg, 0, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NPOPB_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 6;//+NPOPB length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucParamFlg;
    unsigned char                    ucParam1Flg = D_ATC_FLAG_FALSE;
    unsigned char                    ucResult;
    unsigned char                    ucIdx;
    unsigned char                    ucPlmnLen = 0;
    unsigned char                    aucplmn[6] = { 0 };
    unsigned long                    ulOffset;
    ST_ATC_NPOPB_PARAMETER   *pNpopb;

    pNpopb = (ST_ATC_NPOPB_PARAMETER*)pEventBuffer;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVNET_NPOPB,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNpopb->ucOperatorIndex, &ucParamFlg, 0, 0xFF, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 6, &ucPlmnLen, aucplmn, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if (ucPlmnLen > 0)
    {
        for (ucIdx = 0; ucIdx < ucPlmnLen; ucIdx++)
        {
            if ((aucplmn[ucIdx] >= '0') && (aucplmn[ucIdx] <= '9'))
            {
                aucplmn[ucIdx] = (unsigned char)(aucplmn[ucIdx] - '0');
            }
            else
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
        }
        pNpopb->ucPlmnFlg = D_ATC_FLAG_TRUE;
        pNpopb->ulPlmn = ((aucplmn[0] << 16) | (aucplmn[1] << 20) | (aucplmn[2] << 8) | aucplmn[3] | (aucplmn[4] << 4));
        if (5 == ucPlmnLen)
        {
            pNpopb->ulPlmn |= (0x0F << 12);
        }
        else
        {
            pNpopb->ulPlmn |= (aucplmn[5] << 12);
        }
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNpopb->ucBandNum, &pNpopb->ucBandNumFlg, 0, 0xFF, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    ucParamFlg = D_ATC_FLAG_FALSE;
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 10, &pNpopb->ulStartFreq, &ucParamFlg, 0, 262143, 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 5, &ulOffset, &ucParam1Flg, 0, 65535, 1, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if (!(((D_ATC_FLAG_TRUE == pNpopb->ucBandNumFlg) && (D_ATC_FLAG_TRUE == ucParamFlg) && (D_ATC_FLAG_TRUE == ucParam1Flg))
        || ((D_ATC_FLAG_FALSE == pNpopb->ucBandNumFlg) && (D_ATC_FLAG_FALSE == ucParamFlg) && (D_ATC_FLAG_FALSE == ucParam1Flg))))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    pNpopb->usOffset = ulOffset;

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NIPINFO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 8;//+NIPINFO length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucParamFlg;
    unsigned char                    ucResult;
    ST_ATC_NIPINFO_PARAMETER        *pIpInfo;

    pIpInfo = (ST_ATC_NIPINFO_PARAMETER*)pEventBuffer;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NIPINFO, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pIpInfo->ucN, &ucParamFlg, 0, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NQPODCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 8;//+NQPODCP length
    unsigned char                    ucCmdFunc;
    unsigned char                    *pValue;
    unsigned char                    ucResult = D_ATC_COMMAND_OK;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NQPODCP, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    pValue     = &(((ST_ATC_NQPODCP_PARAMETER *)pEventBuffer)->ucCid);
    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue,NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);
 
    return ucResult;
}

unsigned char ATC_NSNPD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 6;    //+NSNPD length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucResult = D_ATC_COMMAND_OK;
    unsigned short                   usNonIpDataLen;
    ST_ATC_NSNPD_PARAMETER*          pNsnpdParam;
    unsigned long                    ulDataLen;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NSNPD, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    pNsnpdParam = (ST_ATC_NSNPD_PARAMETER*)pEventBuffer;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pNsnpdParam->ucCid), NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer,&usCmdStrLen, 5, &ulDataLen, NULL, 0, D_ATC_NONIP_DATA_LEN_MAX, 0, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    pNsnpdParam->usNonIpDataLen =(unsigned short)ulDataLen;

    if(0 != pNsnpdParam->usNonIpDataLen)
    {
        pNsnpdParam->pucNonIpData = (unsigned char*)AtcAp_Malloc(pNsnpdParam->usNonIpDataLen);
    
        if(D_ATC_PARAM_OK != ATC_GetHexStrParameter(pCommandBuffer, &usCmdStrLen, pNsnpdParam->usNonIpDataLen * 2, &usNonIpDataLen, pNsnpdParam->pucNonIpData, 1, 0)
            || usNonIpDataLen != pNsnpdParam->usNonIpDataLen)
        {
            if(NULL != pNsnpdParam->pucNonIpData)
            {
                AtcAp_Free(pNsnpdParam->pucNonIpData);
            }
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else
    {
        if(*(pCommandBuffer + usCmdStrLen) != D_ATC_N_QUOTATION && *(pCommandBuffer + usCmdStrLen+1) != D_ATC_N_QUOTATION )
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
         usCmdStrLen += 2;
        if(*(pCommandBuffer + usCmdStrLen) == D_ATC_N_CR)
        {
            return D_ATC_COMMAND_OK;
        }
        usCmdStrLen += 1;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, &(pNsnpdParam->ucRai), NULL, 0, 2, 1, 0))
    {
        if( 0 != pNsnpdParam->usNonIpDataLen)
        {
            AtcAp_Free(pNsnpdParam->pucNonIpData);
        }
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2,&(pNsnpdParam->ucTypeData), NULL, 0, 1, 1, 0))
    {
        if( 0 != pNsnpdParam->usNonIpDataLen)
        {
            AtcAp_Free(pNsnpdParam->pucNonIpData);
        }
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &(pNsnpdParam->ucSquence), NULL, 1, 255, 1, 1))
    {
        if( 0 != pNsnpdParam->usNonIpDataLen)
        {
            AtcAp_Free(pNsnpdParam->pucNonIpData);
        }
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return ucResult;
}
unsigned char ATC_NQPNPD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 7;//+NQPNPD length
    unsigned char                    ucCmdFunc;
    unsigned char                    *pValue;
    unsigned char                    ucResult = D_ATC_COMMAND_OK;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_NQPNPD, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    pValue     = &(((ST_ATC_NQPNPD_PARAMETER *)pEventBuffer)->ucCid);
    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue,NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);

    return ucResult;
}

unsigned char ATC_CNEC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 5;//+CNEC length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucResult    = D_ATC_COMMAND_OK;
    ST_ATC_CNEC_PARAMETER*           pCnecParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVNET_CNEC, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pCnecParam = (ST_ATC_CNEC_PARAMETER*)pEventBuffer;
    
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pCnecParam->ucN, NULL, 0, 255, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NRNPDM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 7;//+NRNPDM length
    unsigned char                    ucCmdFunc;
    unsigned char                    *pValue;
    unsigned char                    ucResult = D_ATC_COMMAND_OK;
    

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_NRNPDM, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    pValue     = &(((ST_ATC_NRNPDM_PARAMETER *)pEventBuffer)->ucReporting);
    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pValue,NULL, 0, 1, 0, 1);

    return ucResult;
}

unsigned char ATC_NCPCDPR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 8;//+NCPCDPR length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucResult    = D_ATC_COMMAND_OK;
    ST_ATC_NCPCDPR_PARAMETER*        pNcpcdprParam;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_NCPCDPR, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    pNcpcdprParam = (ST_ATC_NCPCDPR_PARAMETER*)pEventBuffer; 
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNcpcdprParam->ucParam, NULL, 0, 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pNcpcdprParam->ucState, NULL, 0, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_CEID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CEID length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEID, pEventBuffer,&ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_NOEQUAL_SET)
    {
        return  ucResult;
    }
    
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_MNBIOTEVENT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 12; //+MNBIOTEVENT
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    unsigned char ucEvent;
    ST_ATC_MNBIOTEVENT_PARAMETER* pParam = (ST_ATC_MNBIOTEVENT_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_MNBIOTEVENT, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucEnable, NULL, 0, 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &ucEvent, NULL, 1, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

#if SIMMAX_SUPPORT
unsigned char ATC_CUPREFER_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9; //+CUPREFER
    unsigned char ucResult;                                                              /* command analysis result                  */
    unsigned char ucCmdFunc;                                                             /* command form                             */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CUPREFER,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;

}

unsigned char ATC_CUPREFERTH_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 11; //+CUPREFERTH
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    ST_ATC_CUPREFERTH_PARAMETER* pParam = (ST_ATC_CUPREFERTH_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CUPREFERTH, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 5, &pParam->ucThreshold, NULL, 0, 97, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}
#endif

unsigned char ATC_NPLMNS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7; //+NPLMNS
    unsigned char ucCmdFunc;

    return ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NPLMNS, pEventBuffer, &ucCmdFunc);
}

unsigned char ATC_NLOCKF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7; //+NLOCKF
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    unsigned char ucParResult;
    unsigned char ucParLen;
    unsigned char ucStopStrInf;
    unsigned long ulEarfcn;
    unsigned long ulPci;
    unsigned char i = 0;
    ST_ATC_NLOCKF_PARAMETER* pParam = (ST_ATC_NLOCKF_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NLOCKF, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucMode, NULL, 0, 2, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    switch(pParam->ucMode)
    {
    case 0:
        if(*(pCommandBuffer + 9) != D_ATC_DEFAULT_CR)
        {
            return D_ATC_COMMAND_SYNTAX_ERROR;
        }
        break;
    case 1:
        ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen,&ulEarfcn, &ucStopStrInf, 0);
        usCmdStrLen = usCmdStrLen + ucParLen;
        if(D_ATC_PARAM_OK == ucParResult)
        {
            if(ulEarfcn == 0)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            pParam->aulEarfcnList[0] = ulEarfcn;

            if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                pParam->usPci = 65535;
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
        
        ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ulPci, &ucStopStrInf,0);

        if((D_ATC_PARAM_OK == ucParResult) && (ulPci <= 503))
        {
            if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
               pParam->usPci = ulPci;
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
    case 2:
        for(i = 0;i < D_ATC_EARFCN_MAX_NUM; i++ )
        {
            ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen,&ulEarfcn, &ucStopStrInf, 0);
            usCmdStrLen = usCmdStrLen + ucParLen;
            if(D_ATC_PARAM_OK == ucParResult)
            {
                pParam->aulEarfcnList[i] = ulEarfcn;
                if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                {
                    pParam->ucEarfcnNum = i + 1;
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
        }
        
        if(i > (D_ATC_EARFCN_MAX_NUM - 1))
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_ZICCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* "+ZICCID" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_ZICCID,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

unsigned char ATC_ZCELLINFO_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                              /* "+ZCELLINFO" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_ZCELLINFO,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

unsigned char ATC_QCGDEFCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 11;                                                              /* "+QCGDEFCONT" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParLen;
    unsigned char i = 0;
    unsigned char aucPDPtypeData[7] ={0};

    ST_ATC_QCGDEFCONT_PARAMETER* pQcgdefcontPara =(ST_ATC_QCGDEFCONT_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QCGDEFCONT,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    //pdp
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 6, &ucParLen, aucPDPtypeData, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    for (i = 0; i < 9; i++)
    {
        if (0 == AtcAp_Strncmp(aucPDPtypeData, (unsigned char*)ATC_PdpType_Table[i].aucStr))
        {
            pQcgdefcontPara->ucPdpTypeValue = ATC_PdpType_Table[i].ucStrVal;
            pQcgdefcontPara->ucPdpTypeFlg = D_ATC_FLAG_TRUE; /* IP Existence                 */
            ucResult = D_ATC_COMMAND_OK;
            break;
        }
    } 

    if(i == 9)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#if VER_BC25
    if(pCommandBuffer + usCmdStrLen == D_ATC_N_CR)
    {
        pQcgdefcontPara->ucApnFlg = D_ATC_FLAG_FALSE;
    }
    else
    {
        pQcgdefcontPara->ucApnFlg = D_ATC_FLAG_TRUE;
    }
#endif
    //APN
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, FACTORY_USER_SET_APN_LEN, &(pQcgdefcontPara->ucApnLen), pQcgdefcontPara->aucApnValue, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //username
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pQcgdefcontPara->ucUsernameLen), pQcgdefcontPara->aucUsername, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

#if VER_BC25
    //password
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pQcgdefcontPara->ucPasswordLen), pQcgdefcontPara->aucPassword, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, &(pQcgdefcontPara->ucAuthProt), NULL, 0, 1, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#else
    //password
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pQcgdefcontPara->ucPasswordLen), pQcgdefcontPara->aucPassword, 1, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    
    if(pQcgdefcontPara->ucUsernameLen >= 1 || pQcgdefcontPara->ucPasswordLen >= 1)
    {
        if(pQcgdefcontPara->ucUsernameLen == 0 )
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
        pQcgdefcontPara->ucAuthProt = D_AUTH_PROT_PAP;
    }
#endif
    return ucResult;
}

unsigned char ATC_QBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NBAND" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char i = 0;
    unsigned char *pucValue;
    unsigned char ucBandNum;
    ST_ATC_QBAND_PARAMETER*  stQbandparam =(ST_ATC_QBAND_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QBAND,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, &ucBandNum, NULL, 0, 3, 0, 0);
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
        if(ucBandNum == 0 && *(pCommandBuffer + 8) != D_ATC_DEFAULT_CR)
        {
            return D_ATC_COMMAND_SYNTAX_ERROR;
        }
        
        for(; i < ucBandNum; i++)
        {
            pucValue     = &(stQbandparam->mau8Band[i]);  
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, NULL, 0, 0xFF, 0, i == (ucBandNum-1) ? 1 : 0);    
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }            
        }
        stQbandparam->mu8Num = i;
    }
    return ucResult;
}

unsigned char ATC_QCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+QCCID" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QCCID,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

unsigned char ATC_QENG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5; //+QENG
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    ST_ATC_QENG_PARAMETER* pParam = (ST_ATC_QENG_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QENG, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucVal, NULL, 0, 3, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

#if VER_BC25
    if(pParam->ucVal > 1)
#else
    if(pParam->ucVal != 0 && pParam->ucVal != 3)
#endif
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_QCFG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5; //+QCfg
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStrLen = 0; 
    unsigned char ucParResult;                                                                  /* parameter result                         */

    ST_ATC_QCFG_PARAMETER* pQcfgParam = (ST_ATC_QCFG_PARAMETER*) pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QCFG,pEventBuffer,&ucCmdFunc);
    
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;                                                               /* '=' set                                  */

    ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen, &ucStrLen, pQcfgParam->ucFunc, &ucStopStrInf);
    if(D_ATC_PARAM_OK == ucParResult)
    {
#if VER_BC25
        if(0 != strcmp((char*)pQcfgParam->ucFunc, (char*)"EPCO"))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
#else
        if(0 != strcmp((char*)pQcfgParam->ucFunc, (char*)"autoapn"))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
#endif
        if( D_ATC_STOP_CHAR_CR == ucStopStrInf)
        {
            pQcfgParam->usEvent = D_ATC_EVENT_QCFG_R;
            return ucResult;
        }
        else if(D_ATC_STOP_CHAR_KANMA != ucStopStrInf)
        {
            return D_ATC_COMMAND_SYNTAX_ERROR;
        }
    }
    else
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    usCmdStrLen = (unsigned char)usCmdStrLen + ucParLen + 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pQcfgParam->ucVal, NULL, 0, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return ucResult;
}

unsigned char ATC_NSIMWC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
#if Custom_09
    unsigned short usCmdStrLen = 8;                                                              /* command length     "qusimwc"           */
#else
    unsigned short usCmdStrLen = 7;                                                              /* command length      "nsimwc"          */
#endif
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    ST_ATC_NSIMWC_PARAMETER* pParam = (ST_ATC_NSIMWC_PARAMETER *)pEventBuffer;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NSIMWC,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;                                                               /* '=' set                              */
    ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &pParam->ucMode, NULL, 104, 104, 0, 0);

    if((*(pCommandBuffer + usCmdStrLen - 1) == D_ATC_N_COMMA) && (ucResult == D_ATC_COMMAND_OK))
    {
        pParam->ucSimwcEnableFlag = 1;
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucSimwcEnable, NULL, 0, 1, 0, 1);
    }

    return ucResult;
}

unsigned char ATC_NUICC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NUICC,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                              */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 0, 1);           
    }
    return ucResult;
}

unsigned char ATC_PSTEST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short                   usCmdStrLen = 7;    //+PSTEST length
    unsigned char                    ucCmdFunc;
    unsigned char                    ucResult = D_ATC_COMMAND_OK;
    unsigned short                   usIpDataLen;
    ST_ATC_PSTEST_PARAMETER*         pPstestParam;
    unsigned long                    ulDataLen;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen, EVENT_PSTEST, pEventBuffer, &ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    pPstestParam = (ST_ATC_PSTEST_PARAMETER*)pEventBuffer;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pPstestParam->ucType), NULL, 0, 255, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer,&usCmdStrLen, 5, &ulDataLen, NULL, 0, 1600, 0, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    pPstestParam->usDataLen =(unsigned short)ulDataLen;

    if(0 != pPstestParam->usDataLen)
    {
        pPstestParam->pucData = (unsigned char*)AtcAp_Malloc(pPstestParam->usDataLen);
    
        if(D_ATC_PARAM_OK != ATC_GetHexStrParameter(pCommandBuffer, &usCmdStrLen, pPstestParam->usDataLen * 2, &usIpDataLen, pPstestParam->pucData, 1, 0)
            || usIpDataLen != pPstestParam->usDataLen)
        {
            if(NULL != pPstestParam->pucData)
            {
                AtcAp_Free(pPstestParam->pucData);
            }
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_QNIDD_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;  //+QNIDD
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned long ulDataLen;
    
    ST_ATC_QNIDD_PARAMETER* pParam = (ST_ATC_QNIDD_PARAMETER *)pEventBuffer;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QNIDD,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return  ucResult;
    }
    usCmdStrLen += 1; 

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &pParam->ucOption, NULL, 0, 3, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(D_QNIDD_OPTION_CREATE_ACCOUNT == pParam->ucOption)
    {
        //APN
        if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 64, &(pParam->ucApnLen), pParam->aucApnValue, 1, 0))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }

        //username
        if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pParam->ucUsernameLen), pParam->aucUsername, 1, 0))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }

        //password
        if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pParam->ucPasswordLen), pParam->aucPassword, 1, 1))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else if(D_QNIDD_OPTION_ESTABLISH_CONNECT == pParam->ucOption)
    {
        if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &pParam->ucAccountID, NULL, 0, 3, 0, 1))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else if(D_QNIDD_OPTION_ACTIVE_CONNECT == pParam->ucOption)
    {
        if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &pParam->ucNIDD_ID, NULL, 0, 3, 0, 1))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else
    {
        if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 4, &pParam->ucNIDD_ID, NULL, 0, 3, 0, 0))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    
        pParam->pucData = (unsigned char*)AtcAp_Malloc(512 * 2);
        if(D_ATC_PARAM_OK != ATC_GetHexStrParameter(pCommandBuffer, &usCmdStrLen, 512 * 2, &pParam->usDataLen, pParam->pucData, 1, 1))
        {
            if(NULL != pParam->pucData)
            {
                AtcAp_Free(pParam->pucData);
            }
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_XYCELLS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;  //+XYCELLS
    unsigned char ucCmdFunc;

    return ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_XYCELLS,pEventBuffer,&ucCmdFunc);

}

unsigned char ATC_PRESETFREQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 11; //+PRESETFREQ
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    unsigned char ucParResult;
    unsigned char ucParLen;
    unsigned char ucStopStrInf;
    unsigned long ulEarfcn;
    ST_ATC_PRESETFREQ_PARAMETER* pParam = (ST_ATC_PRESETFREQ_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_PRESETFREQ, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucOperatorIndex, NULL, 0, 2, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    if(*(pCommandBuffer + usCmdStrLen) == D_ATC_DEFAULT_CR)
    {
        pParam->ucFreqFlg = 0;
        pParam->usEvent = D_ATC_EVENT_PRESETFREQ_R;
        return D_ATC_COMMAND_OK;
    }
    ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen,&ulEarfcn, &ucStopStrInf, 0);

    if(D_ATC_PARAM_OK == ucParResult)
    {
        pParam->ucFreqFlg = 1;
        pParam->ulEarfcn = ulEarfcn;
        if(D_ATC_STOP_CHAR_CR != ucStopStrInf)
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    else
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    return D_ATC_COMMAND_OK;
}

unsigned char ATC_NPBPLMNS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;    //+NPBPLMNS
    unsigned char ucCmdFunc;
    unsigned char ucResult;
    unsigned char ucFlag;
    ST_ATC_CMD_PARAMETER  *pParam = (ST_ATC_CMD_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NPBPLMNS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    
    return ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucValue, &ucFlag, 0, 1, 0, 1);      
}

unsigned char ATC_NBACKOFF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;    //+NBACKOFF
    unsigned char ucCmdFunc;
    unsigned char ucResult;
    unsigned char ucFlag;
    ST_ATC_CMD_PARAMETER  *pParam = (ST_ATC_CMD_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NBACKOFF,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;
    
    return ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pParam->ucValue, &ucFlag, 0, 1, 0, 1); 
}

unsigned char ATC_SIMUUICC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;  //+SIMUUICC                                                            /* +NSET length                        */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    ST_ATC_SIMUUICC_PARAMETER * pSimuuiccParam = NULL;
    unsigned char ucFlag;
    unsigned char ucLen;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_SIMUUICC,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;  
   
    pSimuuiccParam = (ST_ATC_SIMUUICC_PARAMETER *)pEventBuffer;

    for (; iParamNum < D_ATC_PARAM_MAX_P_SIMUUICC; iParamNum++)
    {
        switch(iParamNum)
        {
            case 0:
                ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
                                 D_ATC_P_SIMUUICC_INS_SIZE_MAX - 1, &ucParLen, &(pSimuuiccParam->ucInsLen), pSimuuiccParam->aucInsValue, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */    
                    if(pSimuuiccParam->ucInsLen != 0 && D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pSimuuiccParam->usEvent = D_ATC_EVENT_SIMUUICC_R;
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
                break;              
            case 1: 
                if (0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"IMSI"))
                {
                    ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), D_ATC_SIMUUICC_IMSI_SIZE_MAX, &ucParLen, &pSimuuiccParam->u.stImsi.ucImsiLen, pSimuuiccParam->u.stImsi.aucImsi, &ucStopStrInf);
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
                else if (0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"CARDNUM"))
                {
                                   
                    ucParResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 1, &(pSimuuiccParam->u.stCardNum.ucCardNum), &ucFlag, 0, 9, 0, 0);
                    if(D_ATC_PARAM_OK == ucParResult)
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

                    //usCmdStrLen += 1;
                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }     

                }
                else if (0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"FILECONTENT"))
                {
                                   
                    ucParResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 1, &(pSimuuiccParam->u.stFileContent.ucAppType), &ucFlag, 0, 2, 0, 0);
                    if(D_ATC_PARAM_OK == ucParResult)
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
                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }     

                }
                else if (0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"AUTH"))
                {
                    ucParResult     = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 16*2, &ucParLen, &ucLen, pSimuuiccParam->u.stAuth.aucKey, &ucStopStrInf);
                    if(D_ATC_PARAM_OK == ucParResult)
                    {
                        usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */                        
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
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }             
                }
                break;
            case 2:
                if (0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"CARDNUM"))
                {
                    ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 1, &(pSimuuiccParam->u.stCardNum.ucSubCardNum), &ucFlag, 1, 2, 0, 1);
                return ucResult;
                }
                else if(0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"FILECONTENT"))
                {
                    ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 4, &ucParLen, &ucLen, pSimuuiccParam->u.stFileContent.aucFileId, &ucStopStrInf);              
                    if(D_ATC_PARAM_OK == ucParResult)
                    {
                        usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */                        
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
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }  

                }
                else if(0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"AUTH"))
                {
                    ucResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 16*2, &ucParLen, &ucLen, pSimuuiccParam->u.stAuth.aucOp, &ucStopStrInf);              
                    return ucResult;
                }
                break;
            case 3:
                if(0 == AtcAp_Strncmp((unsigned char *)(pSimuuiccParam->aucInsValue), (unsigned char *)"FILECONTENT"))
                {
                    ucResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 32*2, &ucParLen, &ucLen, pSimuuiccParam->u.stFileContent.aucFileContent, &ucStopStrInf);              
                    return ucResult;
                }
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

unsigned char ATC_QLOCKF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7; //+QLOCKF
    unsigned char ucResult;
    unsigned char ucCmdFunc;
    unsigned char ucParResult;
    unsigned char ucParLen;
    unsigned char ucStopStrInf;
    unsigned long ulEarfcn;
    unsigned long ulPci;
    unsigned long ucOffset;
    unsigned char i = 0;
    ST_ATC_QLOCKF_PARAMETER* pParam = (ST_ATC_QLOCKF_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QLOCKF, pEventBuffer, &ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return  ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &(pParam->ucMode), NULL, 0, 1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    switch(pParam->ucMode)
    {
    case 0:
        if(*(pCommandBuffer + 9) != D_ATC_DEFAULT_CR)
        {
            return D_ATC_COMMAND_SYNTAX_ERROR;
        }
        break;
    case 1:
        ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen,&ulEarfcn, &ucStopStrInf, 0);
        usCmdStrLen = usCmdStrLen + ucParLen;
        if(D_ATC_PARAM_OK == ucParResult)
        {
            if(ulEarfcn > 262143)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            pParam->ulEarfcn = ulEarfcn;

            if(D_ATC_STOP_CHAR_KANMA != ucStopStrInf)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
        }
        else
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
        usCmdStrLen += 1;
        
        ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ucOffset, &ucStopStrInf,0);
        usCmdStrLen = usCmdStrLen + ucParLen;
        if(D_ATC_PARAM_OK == ucParResult)
        {
            if((0 <= ucOffset) && (38 >= ucOffset))
            {
                pParam->ucEarfcnOffset = ucOffset;
            }
            else
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            
            if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                pParam->usPci = 65535;
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
        usCmdStrLen += 1;
        
        ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ulPci, &ucStopStrInf,0);
        usCmdStrLen = usCmdStrLen + ucParLen;
        if((D_ATC_PARAM_OK == ucParResult) && (ulPci <= 503))
        {
            if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                pParam->usPci = ulPci;
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
    }
    return D_ATC_COMMAND_OK;
}

unsigned char ATC_QCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                              /* "+QCSEARFCN" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParam;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QCSEARFCN,pEventBuffer,&ucCmdFunc);
    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return ucResult;
    }
    usCmdStrLen++;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &ucParam, NULL, 0, 0, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return ucResult;
}

unsigned char ATC_W_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 2;                                                              /* &w length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_W, pEventBuffer,&ucCmdFunc);
    return  ucResult;
}

unsigned char ATC_W0_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 3;                                                              /* &w0 length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_W, pEventBuffer,&ucCmdFunc);
    return  ucResult;
}

unsigned char ATC_QICSGP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* "+QICSGP" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParLen;
    unsigned char i = 0;

    ST_ATC_QICSGP_PARAMETER* pQicsgpPara =(ST_ATC_QICSGP_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QICSGP,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc != D_ATC_CMD_FUNC_SET
        && ucCmdFunc != D_ATC_CMD_FUNC_READ)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    //CID
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pQicsgpPara->ucATCid, NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#if VER_BC25
    if(pQicsgpPara->ucATCid < 1 || pQicsgpPara->ucATCid > 7)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#endif
    if(*(pCommandBuffer + usCmdStrLen) == D_ATC_DEFAULT_CR)
    {
        pQicsgpPara->usEvent = D_ATC_EVENT_QICSGP_R;
        return ucResult;
    }
    //pdp
    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pQicsgpPara->ucPdpTypeValue, &pQicsgpPara->ucPdpTypeFlg, 1, 3, 0, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //APN
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, FACTORY_USER_SET_APN_LEN, &(pQicsgpPara->ucApnLen), pQicsgpPara->aucApnValue, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //username
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pQicsgpPara->ucUsernameLen), pQicsgpPara->aucUsername, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    //password
    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, D_PCO_AUTH_MAX_LEN, &(pQicsgpPara->ucPasswordLen), pQicsgpPara->aucPassword, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    if(pQicsgpPara->ucUsernameLen >= 1 || pQicsgpPara->ucPasswordLen >= 1)
    {
        if(pQicsgpPara->ucUsernameLen == 0 )
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    //authprot
        if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, &(pQicsgpPara->ucAuthProt), NULL, 0, 1, 0, 1))
        {
            return D_ATC_COMMAND_PARAMETER_ERROR;
        }
    }
    return ucResult;
}

unsigned char ATC_QSPCHSC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+QSPCHSC" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */

    ST_ATC_CMD_PARAMETER* pQspchscPara =(ST_ATC_CMD_PARAMETER*)pEventBuffer;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QSPCHSC,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc != D_ATC_CMD_FUNC_SET)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &pQspchscPara->ucValue, NULL, 0, 1, 0, 1))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return ucResult;
}
