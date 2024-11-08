/** 
* @file         xy_ps_api.h
* @brief        3GPP相关AT命令处理接口，包括两个部分，一个是主动上报类消息的处理机制xy_atc_registerPSEventCallback；\n
***  另一个是用户触发的AT请求类消息的处理xy_atc_interface_call。对于客户常用的AT命令，芯翼提供了相应的API接口，如xy_get_IMEI。
* @attention    3GPP细节较多，用户二次开发时请务必确认清楚后再执行相关AT的改造
*
*/

#pragma once
#include <stdint.h>

/*******************************************************************************
 *                             Macro definitions                               *
 ******************************************************************************/
/**
 * @brief           Corresponding "AT+CFUN" operation
 * @warning 
 */
#define NET_CFUN0       0 ///< will  AT+CFUN=0
#define NET_CFUN1       1 ///< will  AT+CFUN=1
#define NET_CFUN5       5 ///< will  AT+CFUN=5

#define IMEI_LEN        (15 + 1)
#define IMSI_LEN        (15 + 1)
#define APN_LEN         100
#define UCCID_LEN       (20 + 1)
#define SN_LEN          (64 + 1)
#define PDPTYPE_LEN     7
#define EDRX_BASE_TIME  10.24
#define PTW_BASE_TIME   2.56

/* User registrable PS eventId */
#ifndef PS_REG_EVENT
#define PS_REG_EVENT
#define D_XY_PS_REG_EVENT_SIMST              0x00000001 //ATC_MSG_SIMST_IND_STRU
#define D_XY_PS_REG_EVENT_XYIPDNS            0x00000002 //ATC_MSG_XYIPDNS_IND_STRU
#define D_XY_PS_REG_EVENT_CRTDCP             0x00000004 //ATC_MSG_CRTDCP_IND_STRU
#define D_XY_PS_REG_EVENT_CGAPNRC            0x00000008 //ATC_MSG_CGAPNRC_IND_STRU
#define D_XY_PS_REG_EVENT_CGEV               0x00000010 //ATC_MSG_CGEV_IND_STRU
#define D_XY_PS_REG_EVENT_CEREG              0x00000020 //ATC_MSG_CEREG_IND_STRU
#define D_XY_PS_REG_EVENT_CSCON              0x00000040 //ATC_MSG_CSCON_IND_STRU
#define D_XY_PS_REG_EVENT_NPTWEDRXP          0x00000080 //ATC_MSG_NPTWEDRXP_IND_STRU
#define D_XY_PS_REG_EVENT_CEDRXP             0x00000100 //ATC_MSG_CEDRXP_IND_STRU
#define D_XY_PS_REG_EVENT_CCIOTOPTI          0x00000200 //ATC_MSG_CCIOTOPTI_IND_STRU
#define D_XY_PS_REG_EVENT_CESQ_Ind           0x00000400 //ATC_MSG_CESQ_IND_STRU, Report RSRP/SINR every 10 seconds
#define D_XY_PS_REG_EVENT_LOCALTIMEINFO      0x00000800 //ATC_MSG_LOCALTIMEINFO_IND_STRU
#define D_XY_PS_REG_EVENT_PDNIPADDR          0x00001000 //ATC_MSG_PdnIPAddr_IND_STRU
#define D_XY_PS_REG_EVENT_NGACTR             0x00002000 //ATC_MSG_NGACTR_IND_STRU
#define D_XY_PS_REG_EVENT_CMT                0x00004000 //ATC_MSG_CMT_IND_STRU
#define D_XY_PS_REG_EVENT_CMOLRE             0x00008000 //ATC_MSG_CMOLRE_IND_STRU
#define D_XY_PS_REG_EVENT_CMOLRG             0x00010000 //ATC_MSG_CMOLRG_IND_STRU
#endif

/*******************************************************************************
 *                             Type definitions                                *
 ******************************************************************************/
typedef struct ril_serving_cell_info
{
    int Signalpower;
    int Totalpower;
    int TXpower;
    int CellID;
    int ECL;
    int SNR;
    int EARFCN;
    int PCI;
    int RSRQ;
    char tac[5];
    int sband;
    unsigned long plmn;
} ril_serving_cell_info_t;

typedef struct neighbor_cell_info
{
    int nc_earfcn;
    int nc_pci;
    int nc_rsrp;
} neighbor_cell_info_t;

typedef struct ril_phy_info
{
    //BLER
    int RLC_UL_BLER;
    int RLC_DL_BLER;
    int MAC_UL_BLER;
    int MAC_DL_BLER;
    int MAC_UL_total_bytes;
    int MAC_DL_total_bytes;
    int MAC_UL_total_HARQ_TX;
    int MAC_DL_total_HARQ_TX;
    int MAC_UL_HARQ_re_TX;
    int MAC_DL_HARQ_re_TX;
    //THP
    int RLC_UL_tput;
    int RLC_DL_tput;
    int MAC_UL_tput;
    int MAC_DL_tput;
} ril_phy_info_t;

typedef enum
{
    UICC_UNKNOWN = 0,
    UICC_TELECOM, //telecom
    UICC_MOBILE,  //mobile
    UICC_UNICOM,  //unicom
} SIM_card_type_t;

typedef struct ril_neighbor_cell_info
{
    int nc_num;
    neighbor_cell_info_t neighbor_cell_info[5];
} ril_neighbor_cell_info_t;

typedef struct ril_radio_info
{
    ril_serving_cell_info_t serving_cell_info;
    ril_neighbor_cell_info_t neighbor_cell_info;
    ril_phy_info_t phy_info;
} ril_radio_info_t;

typedef void (*xy_psEventCallback_t)(unsigned long eventId, void *param, int paramLen);

/**
 * @brief           回调解析函数
 * @param1 void*    EventId对应的CNF结构体
 * @param2 itn      结构体大小
 * @param3 void*    pResult返回回调函数解析结果
 * @return          Y_ERR失败, XY_OK成功
 */
typedef int (*func_AppInterfaceCallback)(void*, int, void*);

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/**
 * @brief           PS主动上报消息的回调注册函数，当PS状态发生变化后，会触发eventId的上报，并执行callback回调函数
 * @param eventId   用户注册事件ID，见PS_REG_EVENT
 * @param callback  回调函数
 * @return          void
 * @attention       回调函数内，不能调用xy_atc_interface_call。
 */
void xy_atc_registerPSEventCallback(unsigned long eventId, xy_psEventCallback_t callback);

/**
 * @brief           供用户发送3GPP相关的AT请求，并同步接收处理返回的AT应答
 * @param pCmd      AT命令字符串，e.g."AT+CGSN=1\r\n"
 * @param callback  AT命令回调函数, 可为NULL。
 *                      为NULL时，pResult保存EventId对应的CNF结构体，AT命令与返回结果结构体对应关系示例如下:
 *                                1) 设置/执行命令：AT+XXXX[=<value>] -> ATC_MSG_XXXX_CNF_STRU
 *                                2) 查询命令         : AT+XXXX?          -> ATC_MSG_XXXX_R_CNF_STRU
 *                                3) 测试命令         : AT+XXXX=?         -> ATC_MSG_XXXX_T_CNF_STRU
 *                                (详细参见xy_atc_interface.h)
 *                      不为NULL时，pResult返回回调函数解析结果。
 * @param pResult   执行返回结果, 可为NULL；仅返回OK的命令，如设置命令，该参数为NULL
 * @return          XY_ERR失败, XY_OK成功
 * @attention       如果客户需要解析对应的中间结果AT命令，需要针对每条AT请求使用正确的中间结构结构体，并将地址强制为pResult入参；具体对应关系请咨询协议栈开发人员
 */
int xy_atc_interface_call(char* pCmd, func_AppInterfaceCallback callback, void* pResult);

/**
 * @brief           供用户发送3GPP相关的AT请求，并同步接收处理返回的AT应答
 * @param pCmd      SMS PDU e.g. 0x08,0x91...
 * @param callback  AT命令回调函数, 可为NULL。
 *                      为NULL时，pResult保存EventId对应的CNF结构体，AT命令与返回结果结构体对应关系示例如下:
 *                                1) 设置/执行命令：AT+XXXX[=<value>] -> ATC_MSG_XXXX_CNF_STRU
 *                                2) 查询命令         : AT+XXXX?          -> ATC_MSG_XXXX_R_CNF_STRU
 *                                3) 测试命令         : AT+XXXX=?         -> ATC_MSG_XXXX_T_CNF_STRU
 *                                (详细参见xy_atc_interface.h)
 *                      不为NULL时，pResult返回回调函数解析结果。
 * @param pResult   执行返回结果, 可为NULL；仅返回OK的命令，如设置命令，该参数为NULL
 * @return          ATC_AP_FALSE失败, ATC_AP_TRUE成功
 * @attention       如果客户需要解析对应的中间结果AT命令，需要针对每条AT请求使用正确的中间结构结构体，并将地址强制为pResult入参；具体对应关系请咨询协议栈开发人员
 */
int xy_atc_interface_call_pdu(unsigned char* pPduData, unsigned short usPduLen, func_AppInterfaceCallback callback, void* pResult);

/**
 * @brief           Corresponding "AT+CFUN" operation
 * @param status    see  @ref NET_CFUN0  NET_CFUN1
 * @return          ATC_AP_TRUE is success,other value is fail
 * @warning         AT+CFUN=1成功返回OK后，并不代表此时PDP已激活，需要调用xy_wait_tcpip_ok接口等待PDP激活后，才能进行网络通信业务
 */
int xy_cfun_excute(int status);

/**
* @brief            This is funtion user Read the value of CFUN
* @param            存储cfun的int指针
* @return           XY_ERR获取失败, XY_OK获取成功
*/
int xy_cfun_read(int *cfun);

/**
* @brief            This is funtion user Read check whether the network is registered
* @param            存储cgact的int指针
* @return           XY_ERR获取失败, XY_OK获取成功
*/
int xy_get_CGACT(int *cgact);

/**
* @brief            查询CEREG配置的n值
* @param            存储cereg的int指针，获取成功时，该值的具体含义请查阅《芯翼XY1100 AT命令手册》中的CEREG中n参数值
* @return           XY_ERR获取失败, XY_OK获取成功
* @warning          如果需要查询当前是否已小区驻留，可以使用AT+NUESTATS=CELL命令查询当前的服务小区
*/
int xy_cereg_read(int *cereg);

/**
 * @brief           This is funtion user set the SN
 * @param sn        SN字符串
 * @param len       SN字符串长度
 * @return          XY_ERR获取失败, XY_OK获取成功
 */
int xy_set_SN(char *sn, int len);

/**
 * @brief           This is funtion user Read the SN
 * @param sn        由调用者申请内存，接口内部赋值
 * @param len       入参sn指针的长度，由调用者赋值，但不能小于@ref SN_LEN 
 * @return          XY_ERR获取失败, XY_OK获取成功
 * @warning         注意入参内存申请长度，否则会造成内存越界
 */
int xy_get_SN(char *sn, int len);

/**
 * @brief           This is funtion user set the SVN
 * @param svn       SVN版本号,长度为2。
 * @return          XY_ERR获取失败, XY_OK获取成功
 */
int xy_set_SVN(char *svn);

/**
 * @brief           This is funtion user Read the SVN
 * @param svn       由调用者申请内存，接口内部赋值
 * @param len       入参svn指针的空间大小，由调用者赋值，但不能小于3
 * @return          XY_ERR获取失败, XY_OK获取成功
 * @warning         注意入参内存申请长度，否则会造成内存越界
 */
int xy_get_SVN(char *svn, int len);

/**
* @brief            获取信号强度
* @param            存储rssi的int指针
* @return           XY_ERR获取失败, XY_OK获取成功
*/
int xy_get_RSSI(int *rssi);

/**
 * @brief           This is funtion user set the IMEI
 * @param sn        IMEI字符串
 * @return          XY_ERR获取失败, XY_OK获取成功
 * @warning
 */
int xy_set_IMEI(char *imei);

/**
 * @brief           This is funtion user get IMEI
 * @param imei      由调用者申请内存，接口内部赋值
 * @param len       入参imei指针的长度，由调用者赋值，但不能小于16
 * @return          XY_ERR获取失败, XY_OK获取成功
 * @warning         注意入参内存申请长度，否则会造成内存越界
 */
int xy_get_IMEI(char *imei, int len);

/**
 * @brief           This is funtion user get IMSI
 * @param imsi      由调用者申请内存，接口内部赋值
 * @param len       入参imsi指针的长度，由调用者赋值，但不能小于16
 * @return          ATC_AP_FALSE is fail, ATC_AP_TRUE is success
 * @warning         注意入参内存申请长度，否则会造成内存越界
 *                  若尚未检测到SIM卡，imsi指针未被赋值，需要客户自行识别是否已经识别到SIM卡
 */
int xy_get_IMSI(char *imsi, int len);

/**
 * @brief           This is funtion user get cellid
 * @param           存储cellid的int指针
 * @return          XY_ERR获取失败, XY_OK获取成功
 * @warning         小区未驻留之前，无法获取小区ID
 */
int xy_get_CELLID(int *cell_id);

/**
 * @brief           Corresponding get NCCID operation
 * @param ccid      由调用者申请内存，接口内部赋值
 * @param len       入参ccid指针的长度，由调用者赋值，但不能小于23 
 * @return          ATC_AP_FALSE is fail, ATC_AP_TRUE is success
 * @warning         注意入参内存申请长度，否则会造成内存越界
 *                  若尚未检测到SIM卡，imsi指针未被赋值，需要客户自行识别是否已经识别到SIM卡
 */
int xy_get_NCCID(char *ccid, int len);

/**
 * @brief           This is funtion user get APN
 * @param apn_buf   由调用者申请内存，接口内部赋值
 * @param len       入参apn_buf指针的长度，由调用者赋值，但不能小于100 
 * @param query_cid PDP激活对应的哪一路cid，调用者若不关心则填-1，接口内部会自动赋值，即当前网络激活承载的cid路数，一般为0
 * @return          ATC_AP_FALSE is fail,ATC_AP_TRUE is success.
 * @warning         注意入参内存申请长度，否则会造成内存越界
 *                  若尚未配置APN，apn_buf不会被赋值
                    若query_cid要在驻网之后才能填-1，否则返回空
 */
int xy_get_PDP_APN(char *apn_buf, int len, int query_cid);

/**
 * @brief           获取当前网络激活承载的为哪路cid，PDP未激活时默认-1
 * @return          当前网络激活承载的为哪路cid，默认值为0
 */
int xy_get_working_cid();

/**
 * @brief           This is funtion user get 3412 tau_time
 * @param           存储tau的int指针
 * @return          XY_ERR获取失败, XY_OK获取成功
 */
int xy_get_T_TAU(int *tau);

/**
 * @brief           This is funtion user get 3324 act_time
 * @param           存储t3324的int指针
 * @return          XY_ERR获取失败, XY_OK获取成功
 */
int xy_get_T_ACT(int *t3324);

/**
 * @brief       用来通知PS发送空的UDP报文，并携带RAI指示给基站，以快速释放链接
 * @return      ATC_AP_TRUE is success,ATC_AP_FALSE is fail
 * @note        当用户认为后续没有报文交互需求时，调用该接口来伴随触发RAI，以降低功耗。必须设置出厂NV参数close_rai为1，否则在用户释放最后一把锁后会由芯翼平台自触发RAI
 * @warning     该接口慎用！！！建议用户使用如cis_ext_send_with_rai、cdp_send_syn_with_rai等接口\n
 ***    PS会释放当前待发送的所有上行数据，进行需要用户确保调用该接口时已经没有待发送的上行数据。
 */
int xy_send_rai();

/**
 * @brief   This is funtion user get uicc type
 * @param   uicc_type  存储uicc_type的int 指针
 * @return  ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
*/
int xy_get_UICC_TYPE(int *uicc_type);


/**
 * @brief   设置eDRX的模式,寻呼周期大小
 * @param   modeVal     eDRX模式
 * @param   actType     接入技术类型
 * @param   ulDrxValue  eDRX的寻呼周期，单位毫秒
 * @return  ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
*/
int xy_set_eDRX_value(unsigned char modeVal, unsigned char actType, unsigned long ulDrxValue);

/**
 * @brief   用以分别获取eDRX的模式,寻呼周期和寻呼窗口周期大小
 * @param   pucActType      接入技术类型
 * @param   pulEDRXValue    eDRX的寻呼周期，单位毫秒
 * @param   pulPtwValue     寻呼窗口周期，单位毫秒
 * @return  ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
*/
int xy_get_eDRX_value_MS(unsigned char* pucActType, unsigned long* pulEDRXValue, unsigned long* pulPtwValue);

/**
 * @brief   用以分别获取eDRX的寻呼周期和寻呼窗口周期大小
 * @param   eDRX_value  eDRX的寻呼周期，单位秒
 * @param   ptw_value   寻呼窗口周期，单位秒
 * @return  ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
*/
int xy_get_eDRX_value(float *eDRX_value, float *ptw_value);

/**
 * @brief   获取服务小区信息(对应AT命令为AT+NUESTATS=RADIO)
 * @param rcv_servingcell_info 用户主动malloc，并将地址传递进来，不可为NULL
 * @return  ATC_AP_FALSE is fail,return ATC_AP_TRUE is success. 
 */
int xy_get_servingcell_info(ril_serving_cell_info_t *rcv_servingcell_info);

/**
 * @brief   获取邻小区信息(对应AT命令为AT+NUESTATS=CELL)
 * @param neighbor_cell_info 用户主动malloc，并将地址传递进来，不可为NULL
 * @return ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
 * @note 获取邻小区的earfcn，pci，rsrp与邻小区的数量，最多只返回5个邻小区的信息。
 */
int xy_get_neighborcell_info(ril_neighbor_cell_info_t *neighbor_cell_info);

/**
 * @brief   获取PHY信息 (对应AT命令为AT+NUESTATS=BLER和AT+NUESTATS=THP)
 * @param  rcv_phy_info 用户主动malloc，并将地址传递进来，不可为NULL
 * @return ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
 * @note        
 */
int xy_get_phy_info(ril_phy_info_t *rcv_phy_info);

/**
 * @brief   获取NUESTATS中3GPP相关信息的API接口
 * @param rcv_radio_info 用户主动malloc，并将地址传递进来，不可为NULL
 * @return ATC_AP_FALSE is fail,return ATC_AP_TRUE is success.
 * @note  该接口内部分为几次发送不同的AT命令给PS，由于这条AT命令非标准，存在变化的可能，客户自行参考该DEMO实现即可      
 */
int xy_get_radio_info(ril_radio_info_t *rcv_radio_info);

int xy_ps_api_test(char *at_buf, char **prsp_cmd);

/**
* @brief            通过输入AT+CSCON?，查询当前rrc的链路状态
* @param            存储cscon的int指针，获取成功时，该值的具体含义请查阅《芯翼AT命令手册》中的CSCON中mode参数值
* @return           ATC_AP_FALSE获取失败, ATC_AP_TRUE获取成功
* @warning          不可在PS相关AT处理过程中的调用
*/
int xy_get_Cscon(int *Cscon);

/**
* @brief            通过输入AT+CGCONTRDP=<CID>，查询当前CID的IPV4_mtu
* @param            cid：需要查询的链路的cid序号。取值范围(0-10)
                    IPV4_MTU:保存当前cid的mtu信息的地址
* @return           ATC_AP_FALSE获取失败, ATC_AP_TRUE获取成功
* @warning          不可在PS相关AT处理过程中的调用
*/
int xy_get_ipv4_mtu(unsigned char cid, unsigned short* IPV4_MTU);