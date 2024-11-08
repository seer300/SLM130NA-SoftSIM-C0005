/*******************************************************************************
 *                           Include header files                              *
 ******************************************************************************/
#include "xy_ps_api.h"
#include "xy_net_api.h"
#include "at_com.h"
#include "at_ctl.h"
#include "ipc_msg.h"
#include "ipc_msg.h"
#include "ps_netif_api.h"
#ifndef SINGLE_CORE
#include "shm_msg_api.h"
#endif
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_atc_interface.h"
#include "atc_ps_def.h"
#include "atc_ps.h"
#include "xy_at_api.h"

typedef struct
{
    unsigned char                       ucSetVal;
    unsigned char                       aucBitStr[5];
    unsigned long                       ulMS;
} ST_XY_ATC_EDRX_VALUE_TABLE;

const ST_XY_ATC_EDRX_VALUE_TABLE xy_atc_eDrxValue_Tab[10] = 
{
    { 2,  "0010", 20480    },
    { 3,  "0011", 40960    },
    { 5,  "0101", 81920    },
    { 9,  "1001", 163840   },
    { 10, "1010", 327680   },
    { 11, "1011", 655360   },
    { 12, "1100", 1310720  },
    { 13, "1101", 2621440  },
    { 14, "1110", 5242880  },
    { 15, "1111", 10485760 },
};

const ST_XY_ATC_EDRX_VALUE_TABLE xy_atc_eDrxPtw_Tab[15] = 
{
    { 0,  "0000", 2560  },
    { 1,  "0001", 5120  },
    { 2,  "0010", 7680  },
    { 3,  "0011", 10240 },
    { 4,  "0100", 12800 },
    { 5,  "0101", 15360 },
    { 6,  "0110", 17920 },
    { 7,  "0111", 20480 },
    { 8,  "1000", 23040 },
    { 9,  "1001", 25600 },
    { 10, "1010", 28160 },
    { 11, "1011", 30720 },
    { 12, "1100", 33280 },
    { 13, "1101", 35840 },
    { 14, "1110", 38400 },
};

static char* xy_atc_GeteDrxValueBitStr(unsigned long ulMs)
{
    unsigned int i;

    for(i = 0; i < sizeof(xy_atc_eDrxValue_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(ulMs == xy_atc_eDrxValue_Tab[i].ulMS)
        {
            return (char*)xy_atc_eDrxValue_Tab[i].aucBitStr;
        }
    }

    return NULL;
}

static unsigned long xy_atc_GeteDrxValueMS(unsigned char uceDrxValue)
{
    unsigned int i;

    for(i = 0; i < sizeof(xy_atc_eDrxValue_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(uceDrxValue == xy_atc_eDrxValue_Tab[i].ucSetVal)
        {
            return xy_atc_eDrxValue_Tab[i].ulMS;
        }
    }

    return 0;
}

static unsigned long xy_atc_GetPtwValueMS(unsigned char ucPtwValue)
{
    unsigned int i;

    for(i = 0; i < sizeof(xy_atc_eDrxPtw_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(ucPtwValue == xy_atc_eDrxPtw_Tab[i].ucSetVal)
        {
            return xy_atc_eDrxPtw_Tab[i].ulMS;
        }
    }

    return 0;
}

/*******************************************************************************
 *                      Global function implementations                        *
 ******************************************************************************/
int xy_cfun_excute(int status)
{
    char aucCmd[20] = { 0 };

    if(status != NET_CFUN0 && status != NET_CFUN1 && status != NET_CFUN5)
    {
        return ATC_AP_FALSE;
    }

    sprintf(aucCmd, "AT+CFUN=%d\r\n", status);
    return xy_atc_interface_call(aucCmd, NULL, (void*)NULL);
}

int xy_cfun_read(int *cfun)
{
    ATC_MSG_CFUN_R_CNF_STRU tCfunRCnf = { 0 };

    if(cfun == NULL)
    {
        return ATC_AP_FALSE;
    }

    if(xy_atc_interface_call("AT+CFUN?\r\n", NULL, (void*)&tCfunRCnf) == ATC_AP_FALSE)
    {
        return ATC_AP_FALSE;
    }

    *cfun = tCfunRCnf.ucFunMode;
    return ATC_AP_TRUE;
}

int xy_get_CGACT(int *cgact)
{
    ATC_MSG_CGACT_R_CNF_STRU tCgactRCnf = { 0 };

    if(cgact == NULL)
    {
        return ATC_AP_FALSE;
    }
    
    if(xy_atc_interface_call("AT+CGACT?\r\n", NULL, (void*)&tCgactRCnf) == ATC_AP_FALSE)
        return ATC_AP_FALSE;

    *cgact = tCgactRCnf.stState.aCidSta[0].ucState;
    return ATC_AP_TRUE;
}

int xy_cereg_read(int *cereg)
{
    ATC_MSG_CEREG_R_CNF_STRU tCeregRCnf = { 0 };
    
    if(xy_atc_interface_call("AT+CEREG?\r\n", NULL, (void*)&tCeregRCnf) == ATC_AP_FALSE)
        return ATC_AP_FALSE;

    *cereg = tCeregRCnf.tRegisterState.ucEPSRegStatus;
    return ATC_AP_TRUE;
}

int xy_set_SN(char* sn, int len)
{
    char *pucCmd;
    int   ret;
    UNUSED_ARG(len);

    if(NULL == sn || strlen(sn) > 64)
    {
        return ATC_AP_FALSE;
    }

    pucCmd = (char*)AtcAp_Malloc(80);
    sprintf(pucCmd, "AT+NSET=\"SETSN\",\"%s\"\r\n", sn);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
   
    return ret;
}

int xy_get_SN(char *sn, int len)
{
    ATC_MSG_CGSN_CNF_STRU* pCgsnCnf;

    if (len < SN_LEN || sn == NULL)
    {
        return ATC_AP_FALSE;
    }

    pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_CGSN_CNF_STRU) + NVM_MAX_SN_LEN + 1- 4);
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+CGSN=0\r\n", (func_AppInterfaceCallback)NULL, (void*)pCgsnCnf))
    {
        xy_printf(0,PLATFORM, WARN_LOG, "");
        xy_free(pCgsnCnf);
        return ATC_AP_FALSE;
    }

    if(0 != pCgsnCnf->ucLen)
    {
        strncpy(sn, (char*)pCgsnCnf->aucData, pCgsnCnf->ucLen);
        xy_assert(strlen(sn) <= NVM_MAX_SN_LEN);
    }
    xy_free(pCgsnCnf);

    return ATC_AP_TRUE;
}

int xy_set_SVN(char* svn)
{
    char *pucCmd;
    int   ret;

    if(NULL == svn || strlen(svn) != 2)
    {
        return ATC_AP_FALSE;
    }

    pucCmd = (char*)AtcAp_Malloc(30);
    sprintf(pucCmd, "AT+NSET=\"SETSVN\",\"%s\"\r\n", svn);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
   
    return ret;
}

int xy_get_SVN(char *svn, int len)
{
    ATC_MSG_CGSN_CNF_STRU* pCgsnCnf;

    if (len < 3 || svn == NULL)
    {
        return ATC_AP_FALSE;
    }

    pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_CGSN_CNF_STRU) + 2 + 1);
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+CGSN=3\r\n", (func_AppInterfaceCallback)NULL, (void*)pCgsnCnf))
    {
        xy_printf(0,PLATFORM, WARN_LOG, "xy_get_SVN err");
        xy_free(pCgsnCnf);
        return ATC_AP_FALSE;
    }

    if(0 != pCgsnCnf->ucLen)
    {
        strncpy(svn, (char*)pCgsnCnf->aucData, 2);
    }
    xy_free(pCgsnCnf);

    return ATC_AP_TRUE;
}

int xy_get_RSSI(int *rssi)
{
    ATC_MSG_CSQ_CNF_STRU tCsqCnf = { 0 };

    if (rssi == NULL)
    {
        return ATC_AP_FALSE;
    }
    
    if(xy_atc_interface_call("AT+CSQ\r\n", NULL, (void*)&tCsqCnf) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    *rssi = tCsqCnf.ucRxlev;
    return ATC_AP_TRUE;
}

int xy_set_IMEI(char *imei)
{
    char *pucCmd;
    int   ret;
    char  part1[8] = { 0 };
    char  part2[9] = { 0 };

    if(NULL == imei || strlen(imei) > 15)
    {
        return ATC_AP_FALSE;
    }

    strncpy(part1, imei, 7);
    strncpy(part2, imei+7, 8);

    AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[xy_ps_api_test]: part1=%s, part2=%s", part1, part2);

    pucCmd = (char*)AtcAp_Malloc(80);
    sprintf(pucCmd, "AT+NSET=\"SETIMEI\",%s,%s\r\n", part1, part2);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
   
    return ret;
}

int xy_get_IMEI(char *imei, int len)
{
     ATC_MSG_CGSN_CNF_STRU*      pCgsnCnf;
     static char                 s_Imei[IMEI_LEN]  = { 0 };
     
    if (len < IMEI_LEN || imei == NULL)
    {
        return ATC_AP_FALSE;    
    }

    if(strlen(s_Imei) == 0)
    {
        pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_CGSN_CNF_STRU) + IMEI_LEN + 1 - 4);
        if(xy_atc_interface_call("AT+CGSN=1\r\n", (func_AppInterfaceCallback)NULL, (void*)pCgsnCnf) != ATC_AP_TRUE)
        {
            xy_free(pCgsnCnf);
            return ATC_AP_FALSE;
        }
        strcpy(s_Imei, (char*)pCgsnCnf->aucData);
        xy_assert(strlen(s_Imei) < IMEI_LEN);
        xy_free(pCgsnCnf);
    }
    
    strcpy(imei, s_Imei);    
    xy_assert(strlen(imei) < IMEI_LEN);
    
    return ATC_AP_TRUE;
}

int xy_get_IMSI(char *imsi, int len)
{
    ATC_MSG_CIMI_CNF_STRU tImsiCnf          = { 0 };
    static char           s_Imsi[IMSI_LEN]  = { 0 };
    
    if (len < IMSI_LEN || imsi == NULL)
    {
        return ATC_AP_FALSE;
    }   

    if(strlen(s_Imsi) == 0)
    {       
        if(xy_atc_interface_call("AT+CIMI\r\n", (func_AppInterfaceCallback)NULL, (void*)&tImsiCnf) == ATC_AP_FALSE)
        {
            return ATC_AP_FALSE;
        }
        strcpy(s_Imsi, (char*)tImsiCnf.stImsi.aucImsi);
        xy_assert(strlen(s_Imsi) < IMSI_LEN);
    }
    
    strcpy(imsi, s_Imsi);
    xy_assert(strlen(imsi) < IMSI_LEN);

    return ATC_AP_TRUE;
}

int xy_get_CELLID(int *cell_id)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(cell_id == NULL)
    {
        return ATC_AP_FALSE;
    }
    
    if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    *cell_id = tNgactrRCnf.stRegContext.ulCellId;
    return ATC_AP_TRUE;
}

int xy_get_NCCID(char *ccid, int len)
{
    static char             s_Iccid[UCCID_LEN]  = { 0 };
    ATC_MSG_UICCID_CNF_STRU tUccidCnf           = { 0 };
    
    if (len < UCCID_LEN)
    {
        return ATC_AP_FALSE;    
    }   

    if(0 == strlen(s_Iccid))
    {
        if(ATC_AP_TRUE != xy_atc_interface_call("AT+NCCID\r\n", NULL, (void*)&tUccidCnf))
        {
            return ATC_AP_FALSE;
        }
        strcpy(s_Iccid, (char*)tUccidCnf.aucICCIDstring);
        xy_assert(strlen(s_Iccid) < UCCID_LEN);
    }

    strcpy(ccid, s_Iccid);
    xy_assert(strlen(ccid) < UCCID_LEN);
    
    return ATC_AP_TRUE;
}

int xy_get_PDP_APN(char *apn_buf, int len, int cid)
{
    ATC_MSG_CGCONTRDP_CNF_STRU  *pCgcontrdpCnf;
    char                         aucAtCmd[20]  = { 0 };
    int                          query_cid     = 0;

    if (len < APN_LEN || apn_buf == NULL)
    {   
        return ATC_AP_FALSE;
    }

    query_cid = (cid == -1 ? g_working_cid : cid);
    pCgcontrdpCnf = (ATC_MSG_CGCONTRDP_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_CGCONTRDP_CNF_STRU));
    sprintf(aucAtCmd, "AT+CGCONTRDP=%d\r\n", query_cid);   
    if(xy_atc_interface_call(aucAtCmd, NULL, (void*)pCgcontrdpCnf) != ATC_AP_TRUE)
    {
        xy_free(pCgcontrdpCnf);
        return ATC_AP_FALSE;
    }

    if(0 != pCgcontrdpCnf->stPara.ucValidNum && 0 != pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen)
    {
        strncpy(apn_buf, (char*)pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].aucApn, pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen);
        xy_assert(strlen(apn_buf) <= pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen);
    }

    xy_free(pCgcontrdpCnf);
    return ATC_AP_TRUE;
}

int xy_get_working_cid()
{
    return g_working_cid;
}

int xy_get_T_ACT(int *t3324)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(t3324 == NULL)
    {
        return  ATC_AP_FALSE;
    }
    
    if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }
    
    *t3324 = tNgactrRCnf.stRegContext.ulActTime;
    return ATC_AP_TRUE;
}

int xy_get_T_TAU(int *tau)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(tau == NULL)
    {
        return  ATC_AP_FALSE;
    }

    if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    *tau = tNgactrRCnf.stRegContext.ulTauTime;
    return ATC_AP_TRUE;
}

// #if (_CURR_VERSION != _XY2100_) //multiple definition
int xy_send_rai()
{
    xy_printf(0,XYAPP, INFO_LOG, "xy_send_rai,AT+RAI=1\r\n");
    return xy_atc_interface_call("AT+RAI=1\r\n", NULL, (void*)NULL);
}
// #endif

int xy_get_UICC_TYPE(int *uicc_type)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };
#if PS_INTEGRATION_TEST
    int                s_uiccType  = 0;
#else
    static int                s_uiccType  = 0;
#endif
    if(uicc_type == NULL)
    {
        return  ATC_AP_FALSE;
    }

    if(s_uiccType == 0)
    {
        if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != ATC_AP_TRUE)
        {
            return ATC_AP_FALSE;
        }
        s_uiccType = tNgactrRCnf.usOperType + 1;
    }

    *uicc_type = s_uiccType;
    return ATC_AP_TRUE;
}

int xy_get_eDRX_value_MS(unsigned char* pucActType, unsigned long* pulEDRXValue, unsigned long* pulPtwValue)
{
    ATC_MSG_CEDRXRDP_CNF_STRU tCderxrdpCnf = { 0 };
    
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+CEDRXRDP\r\n", (func_AppInterfaceCallback)NULL, (void*)&tCderxrdpCnf))
    {
        return ATC_AP_FALSE;
    }

    if(pucActType != NULL)
    {
        *pucActType = tCderxrdpCnf.stPara.ucActType;
    }

    if(pulEDRXValue != NULL)
    {
        *pulEDRXValue = xy_atc_GeteDrxValueMS(tCderxrdpCnf.stPara.ucNWeDRXValue);
    }

    if(pulPtwValue != NULL)
    {
        *pulPtwValue = xy_atc_GetPtwValueMS(tCderxrdpCnf.stPara.ucPagingTimeWin);
    }
    
    return ATC_AP_TRUE;
}

int xy_set_eDRX_value(unsigned char modeVal, unsigned char actType, unsigned long ulDrxValue)
{
    char*             pDrxValueBitStr;
    char*             pucCmd;
    int               ret;

    pDrxValueBitStr = xy_atc_GeteDrxValueBitStr(ulDrxValue);
    if(pDrxValueBitStr == NULL)
    {
        return ATC_AP_FALSE;
    }
   
    pucCmd = (char*)AtcAp_Malloc(30);
    sprintf(pucCmd, "AT+CEDRXS=%d,%d,\"%s\"\r\n", modeVal, actType, pDrxValueBitStr);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
    
    return ret;
}

int xy_get_eDRX_value(float *eDRX_value, float *ptw_value)
{
    unsigned long ulEDRXValue;
    unsigned long ulPtwValue;

    if(eDRX_value == NULL || ptw_value == NULL)
    {
        return ATC_AP_FALSE;
    }

    if(xy_get_eDRX_value_MS(NULL, &ulEDRXValue, &ulPtwValue) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    *eDRX_value = ulEDRXValue / 1000.00;
    *ptw_value = ulPtwValue / 1000.00;
    
    return ATC_AP_TRUE;
}

int xy_get_servingcell_info(ril_serving_cell_info_t *rcv_serving_cell_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;

    xy_assert(rcv_serving_cell_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+NUESTATS=RADIO\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }

    memset(rcv_serving_cell_info, 0, sizeof(ril_serving_cell_info_t));
    
    rcv_serving_cell_info->Signalpower = pNueStatsCnf->stRadio.rsrp;
    rcv_serving_cell_info->Totalpower = pNueStatsCnf->stRadio.rssi;
    rcv_serving_cell_info->TXpower = pNueStatsCnf->stRadio.current_tx_power_level;
    rcv_serving_cell_info->CellID  = pNueStatsCnf->stRadio.last_cell_ID;
    rcv_serving_cell_info->ECL = pNueStatsCnf->stRadio.last_ECL_value;
    rcv_serving_cell_info->SNR = pNueStatsCnf->stRadio.last_snr_value;
    rcv_serving_cell_info->EARFCN = pNueStatsCnf->stRadio.last_earfcn_value;
    rcv_serving_cell_info->PCI = pNueStatsCnf->stRadio.last_pci_value;
    rcv_serving_cell_info->RSRQ = pNueStatsCnf->stRadio.rsrq;
    sprintf(rcv_serving_cell_info->tac, "%d", pNueStatsCnf->stRadio.current_tac);
    rcv_serving_cell_info->sband = pNueStatsCnf->stRadio.band;
    rcv_serving_cell_info->plmn = pNueStatsCnf->stRadio.current_plmn;

    xy_free(pNueStatsCnf);
    return ATC_AP_TRUE;
}

int xy_get_neighborcell_info(ril_neighbor_cell_info_t *ril_neighbor_cell_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;
    int                        i;

    xy_assert(ril_neighbor_cell_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+NUESTATS=CELL\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }
        
    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_CELL)
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }

    memset(ril_neighbor_cell_info, 0, sizeof(ril_neighbor_cell_info_t)); 
    for(i = 0; i < pNueStatsCnf->stCell.stCellList.ucCellNum && i < 5; i++)
    {
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_earfcn = pNueStatsCnf->stCell.stCellList.aNCell[i].ulDlEarfcn;
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_pci = pNueStatsCnf->stCell.stCellList.aNCell[i].usPhyCellId;
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_rsrp = pNueStatsCnf->stCell.stCellList.aNCell[i].sRsrp;
        ril_neighbor_cell_info->nc_num++;
    }

    xy_free(pNueStatsCnf);
    return ATC_AP_TRUE;
}

int xy_get_phy_info(ril_phy_info_t *rcv_phy_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;

    xy_assert(rcv_phy_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+NUESTATS=BLER\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }
#ifndef PS_INTEGRATION_TEST
    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_BLER)
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }
#endif

    rcv_phy_info->RLC_UL_BLER = pNueStatsCnf->stBler.rlc_ul_bler;
    rcv_phy_info->RLC_DL_BLER = pNueStatsCnf->stBler.rlc_dl_bler;
    rcv_phy_info->MAC_UL_BLER = pNueStatsCnf->stBler.mac_ul_bler;
    rcv_phy_info->MAC_DL_BLER = pNueStatsCnf->stBler.mac_dl_bler;
    rcv_phy_info->MAC_UL_total_bytes = pNueStatsCnf->stBler.total_bytes_transmit;
    rcv_phy_info->MAC_DL_total_bytes = pNueStatsCnf->stBler.total_bytes_receive;
    rcv_phy_info->MAC_UL_total_HARQ_TX = pNueStatsCnf->stBler.transport_blocks_send;
    rcv_phy_info->MAC_DL_total_HARQ_TX = pNueStatsCnf->stBler.transport_blocks_receive;
    rcv_phy_info->MAC_UL_HARQ_re_TX = pNueStatsCnf->stBler.transport_blocks_retrans;
    rcv_phy_info->MAC_DL_HARQ_re_TX = pNueStatsCnf->stBler.total_ackOrNack_msg_receive;

    //AT+NUESTATS=THP
    memset(pNueStatsCnf, 0, sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(ATC_AP_TRUE != xy_atc_interface_call("AT+NUESTATS=THP\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }
#ifndef PS_INTEGRATION_TEST
    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_THP)
    {
        xy_free(pNueStatsCnf);
        return ATC_AP_FALSE;
    }
#endif

    rcv_phy_info->RLC_UL_tput = pNueStatsCnf->stThp.rlc_ul;
    rcv_phy_info->RLC_DL_tput = pNueStatsCnf->stThp.rlc_dl;
    rcv_phy_info->MAC_UL_tput = pNueStatsCnf->stThp.mac_ul;
    rcv_phy_info->MAC_DL_tput = pNueStatsCnf->stThp.mac_dl;

    xy_free(pNueStatsCnf);
    return ATC_AP_TRUE;
}

int xy_get_radio_info(ril_radio_info_t *rcv_radio_info)
{
    xy_assert(rcv_radio_info != NULL);
    memset(rcv_radio_info, 0, sizeof(ril_radio_info_t));

    if (xy_get_servingcell_info(&rcv_radio_info->serving_cell_info) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    if (xy_get_neighborcell_info(&rcv_radio_info->neighbor_cell_info) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }

    if (xy_get_phy_info(&rcv_radio_info->phy_info) != ATC_AP_TRUE)
    {
        return ATC_AP_FALSE;
    }
    return ATC_AP_TRUE;
}

int xy_ps_api_test(char *at_buf, char **prsp_cmd)
{
    char sub_cmd[20] = { 0 };

    *prsp_cmd = NULL;

    AtcAp_PrintLog(0,NAS_THREAD_ID, DEBUG_LOG, "[xy_ps_api_test]: at_buf=%s", at_buf);
    
    if (at_parse_param(",%19s", at_buf, sub_cmd) != AT_OK)
    {
        return ATERR_PARAM_INVALID;
    }
    
    if (0 == strcmp(sub_cmd, "CFUN0"))
    {
        if(xy_cfun_excute(0) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }           
    }
    else if (0 == strcmp(sub_cmd, "CFUN1"))
    {
        if(xy_cfun_excute(1) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }
    }
    else if (0 == strcmp(sub_cmd, "GETCFUN"))
    {
        int cfun = -1;

        if(xy_cfun_read(&cfun) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(32);
        sprintf(*prsp_cmd, "\r\n+CFUN:%d\r\n\r\nOK\r\n", cfun);
    }
    else if (0 == strcmp(sub_cmd, "GETCGACT"))
    {
        int cgact = -1;
        
        if(xy_get_CGACT(&cgact) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(32);
        sprintf(*prsp_cmd, "\r\n+CGACT:%d\r\n\r\nOK\r\n", cgact);

    }
    else if (0 == strcmp(sub_cmd, "GETCEREG"))
    {
        int cereg = -1;
        if(xy_cereg_read(&cereg) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }      

        *prsp_cmd = (char*)xy_malloc(32);
        sprintf(*prsp_cmd, "\r\n+CEREG:%d\r\n\r\nOK\r\n", cereg);
    }
    else if (0 == strcmp(sub_cmd, "SETSN"))
    {
        if(xy_set_SN("00010203", SN_LEN) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }
    }
    else if (0 == strcmp(sub_cmd, "GETSN"))
    {
        char *sn = (char*)xy_malloc(SN_LEN + 1); 
        memset(sn, 0, SN_LEN + 1);
        if(xy_get_SN(sn, SN_LEN) != ATC_AP_TRUE)
        {
            xy_free(sn);
            return ATERR_XY_ERR;
        }
        
        *prsp_cmd = (char*)xy_malloc(100);
        sprintf(*prsp_cmd, "\r\n+SN:%s\r\n\r\nOK\r\n", sn);
        xy_free(sn);
    }
    else if (0 == strcmp(sub_cmd, "SETSVN"))
    {
        if(xy_set_SVN("03") != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }
    }
    else if (0 == strcmp(sub_cmd, "GETSVN"))
    {
        char *svn = (char*)xy_malloc(3); 
        memset(svn, 0, 3);
        if(xy_get_SVN(svn, 3) != ATC_AP_TRUE)
        {
            xy_free(svn);
            return ATERR_XY_ERR;
        }
        
        *prsp_cmd = (char*)xy_malloc(100);
        sprintf(*prsp_cmd, "\r\n+SVN:%s\r\n\r\nOK\r\n", svn);
        xy_free(svn);
    }
    else if (0 == strcmp(sub_cmd, "GETRSSI"))
    {
        int rssi = 0;
        if(xy_get_RSSI(&rssi) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(32);
        sprintf(*prsp_cmd, "\r\n+RSSI:%u\r\n\r\nOK\r\n", rssi);
    }
    else if (0 == strcmp(sub_cmd, "SETIMEI"))
    {
        if(xy_set_IMEI("000000010000016") != ATC_AP_TRUE)
        {      
            return ATERR_XY_ERR;
        }
    }
    else if (0 == strcmp(sub_cmd, "GETIMEI"))
    {
        char *imei=(char*)xy_malloc(IMEI_LEN);
        if(xy_get_IMEI(imei, IMEI_LEN) != ATC_AP_TRUE)
        {      
            xy_free(imei);
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(64);
        sprintf(*prsp_cmd, "\r\n+IMEI:%s\r\n\r\nOK\r\n", imei);
        xy_free(imei);
    }
    else if (0 == strcmp(sub_cmd, "GETIMSI"))
    {
        char *imsi=(char*)xy_malloc(IMSI_LEN);
        if(xy_get_IMSI(imsi, IMSI_LEN) != ATC_AP_TRUE)
        {
            xy_free(imsi);
            return ATERR_XY_ERR;
        }

        *prsp_cmd = xy_malloc(64);
        sprintf(*prsp_cmd, "\r\n+IMSI:%s\r\n\r\nOK\r\n", imsi);
        xy_free(imsi);
    }
    else if (0 == strcmp(sub_cmd, "GETCELLID"))
    {
        int cellid = -1;
        if(xy_get_CELLID(&cellid) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(32);
        sprintf(*prsp_cmd, "\r\n+CELLID:%d\r\n\r\nOK\r\n", cellid);
    }
    else if (0 == strcmp(sub_cmd, "GETCCID"))
    {
        char *ccid=xy_malloc(UCCID_LEN);
        if(xy_get_NCCID(ccid, UCCID_LEN) != ATC_AP_TRUE)
        {
            xy_free(ccid);
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(64);
        sprintf(*prsp_cmd, "\r\n+CCID:%s\r\n\r\nOK\r\n", ccid);
        xy_free(ccid);
    }
    else if (0 == strcmp(sub_cmd, "GETAPN"))
    {
        char *apn = (char*)xy_malloc(APN_LEN);
        if(xy_get_PDP_APN(apn, APN_LEN, -1) != ATC_AP_TRUE)
        {
            xy_free(apn);
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+APN:%s\r\n\r\nOK\r\n", apn);
        xy_free(apn);
    } 
    else if (0 == strcmp(sub_cmd, "GETACT"))
    {
        int t3324;
        if(xy_get_T_ACT(&t3324) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+T3324:%d\r\n\r\nOK\r\n", t3324);
    }
    else if (0 == strcmp(sub_cmd, "GETTAU"))
    {
        int tau;
        if(xy_get_T_TAU(&tau) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+T3412:%d\r\n\r\nOK\r\n", tau);
    }
    else if (0 == strcmp(sub_cmd, "GETUICCTYPE"))
    {
        int uicc_type;
        if(xy_get_UICC_TYPE(&uicc_type) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+UICCTYPE:%d\r\n\r\nOK\r\n", uicc_type);
    }
    else if (0 == strcmp(sub_cmd, "SETDERX"))
    {
        if(xy_set_eDRX_value(1,5,3) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }
    }
    else if (0 == strcmp(sub_cmd, "GETEDRX"))
    {
        float eDRX_value;
        float ptw_value;
        if(xy_get_eDRX_value(&eDRX_value, &ptw_value) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+GETDERX:%.2f,%.2f\r\n\r\nOK\r\n", eDRX_value, ptw_value);
    }
    else if (0 == strcmp(sub_cmd, "GETSERVCLLINFO"))
    {
        ril_serving_cell_info_t rcv_serving_cell_info;
        
        if(xy_get_servingcell_info(&rcv_serving_cell_info) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+GETSERVCLLINFO:%d\r\n\r\nOK\r\n", rcv_serving_cell_info.CellID);
    }
    else if (0 == strcmp(sub_cmd, "GETNEIBCLLINFO"))
    {
        ril_neighbor_cell_info_t ril_neighbor_cell_info = { 0 };
        
        if(xy_get_neighborcell_info(&ril_neighbor_cell_info) != ATC_AP_TRUE)
        {
            return ATERR_XY_ERR;
        }

        *prsp_cmd = (char*)xy_malloc(150);
        sprintf(*prsp_cmd, "\r\n+GETNEIBCLLINFO:cell_num=%d\r\n\r\nOK\r\n", ril_neighbor_cell_info.nc_num);
    }
    else
    {
        return ATERR_PARAM_INVALID;
    }

    return AT_OK;
}

int xy_get_Cscon(int *Cscon)
{
    ATC_MSG_CSCON_R_CNF_STRU tCsconRCnf = { 0 };

    if(Cscon == NULL)
    {
        return ATC_AP_FALSE;
    }
    
    if(xy_atc_interface_call("AT+CSCON?\r\n", NULL, (void*)&tCsconRCnf) != ATC_AP_TRUE)
        return ATC_AP_FALSE;

    *Cscon = tCsconRCnf.stPara.ucMode;
    return ATC_AP_TRUE;
}

int xy_get_ipv4_mtu(unsigned char cid, unsigned short* IPV4_MTU)
{
    char                         aucAtCmd[20]  = { 0 };
    ATC_MSG_CGCONTRDP_CNF_STRU  *pCgcontrdpCnf;

    pCgcontrdpCnf = (ATC_MSG_CGCONTRDP_CNF_STRU*)AtcAp_Malloc(sizeof(ATC_MSG_CGCONTRDP_CNF_STRU));
    sprintf(aucAtCmd, "AT+CGCONTRDP=%d\r\n", cid);   
    if(xy_atc_interface_call(aucAtCmd, NULL, (void*)pCgcontrdpCnf) != ATC_AP_TRUE)
    {
        AtcAp_Free(pCgcontrdpCnf);
        return ATC_AP_FALSE;
    }

    if(0 != pCgcontrdpCnf->stPara.ucValidNum && 0 != pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucIPv4MTUFlag)
    {
        *IPV4_MTU = pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].usIPv4MTU;
        AtcAp_Free(pCgcontrdpCnf);
        return ATC_AP_TRUE;
    }

    AtcAp_Free(pCgcontrdpCnf);
    return ATC_AP_FALSE;
}