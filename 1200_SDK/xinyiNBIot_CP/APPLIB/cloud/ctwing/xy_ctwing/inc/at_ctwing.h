#pragma once

#include "at_ctwing.h"

/**
 * @brief 查询模组接入物联网开放平台软件版本
 * @note AT+CTLWVER?
 */
int at_CTLWVER_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 从 DNS 获取物联网开放平台 IP 地址
 * @note AT+CTLWGETSRVFRMDNS=<dns_server_ip>,<lw_server_dn> [,<ip_type>]
 */
int at_CTLWGETSRVFRMDNS_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 初始化接入物联网开放平台IP地址,设置、删除模组接入平台IP地址参数
 * 
 * @note AT+CTLWSETSERVER=<action_type>,<ip_type>[,<sever_ip>[,<port>]]
 */
int at_CTLWSETSERVER_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 初始化接入物联网开放平台 LIFETIME
 * 
 * @note AT+CTLWSETLT=<lifetime>
 */
int at_CTLWSETLT_req(char *at_buf, char **prsp_cmd);

/**
 * @brief DTLS 参数设置
 * 
 * @note AT+CTLWSETPSK=<mode>,<PSK> [,<PSKID>]
 */
int at_CTLWSETPSK_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 终端扩展认证参数设置（模组提供认证串）
 * 
 * @note AT+CTLWSETAUTH=<auth type>
 */
int at_CTLWSETAUTH_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 终端报文加密参数设置（模组加密收发报文PAYLOAD）
 * 
 * @note AT+CTLWSETPCRYPT=<type>，<value> 
 */
int at_CTLWSETPCRYPT_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 初始化模组会话模式
 * 
 * @note AT+CTLWSETMOD=<MOD_ID>,<MOD_DATA>
 */
int at_CTLWSETMOD_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 登录物联网开放平台
 * 
 * @note AT+CTLWREG[=<auth mode str>,<auth token str>]
 */
int at_CTLWREG_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 通知模组向平台发送心跳或延长已有的LIFETIME
 * 
 * @note AT+CTLWUPDATE[=<RAI Indication>]
 * 
 * <RAI Indication>：整型(缺省为0)，0 –常规模式； 1- 发出报文后收到下行的报文后释放空口（RAI）
 */
int at_CTLWUPDATE_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 模组主动发送DTLS HS操作,用于定期刷新DTLS会话的上下文数据，增强安全性
 * 或者实现DTLS Server要求的某些心跳机制
 * 
 * @note AT+CTLWDTLSHS
 */
int at_CTLWDTLSHS_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 取消登录物联网开放平台(登出平台)
 * 
 * @note AT+CTLWDEREG[=mode]
 * 
 * <mode>：整型，（缺省为0-常规DEREG）
 * 常规DEREG, 适用于MCU登出平台后需要过一段时间再登录平台的场景
 * 本地DEREG，适用于MCU需要DEREG后马上登录平台的场景
 */
int at_CTLWDEREG_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 状态查询
 * 
 * @note AT+CTLWGETSTATUS=<query_type> [,<data1>]
 */
int at_CTLWGETSTATUS_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 重置模组接入平台软件配置数据,将持久化存储中的接入平台的配置数据恢复到缺省状态
 * 
 * @note AT+CTLWCFGRST [=<mode>]
 */
int at_CTLWCFGRST_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 清除持久化存储中的当前会话数据
 * （只能在“引擎无法工作”状态下使用，使得重启模组后会话处于“未登录”状态）
 * 
 * @note AT+CTLWSESDATA=<action>
 */
int at_CTLWSESDATA_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 通知模组向平台发送业务数据，支持选择使用CON模式或NON模式
 * 
 * @note AT+CTLWSEND=<data>[,<mode>]<CR>
 */
int at_CTLWSEND_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 模组使用该指令设置模组接收平台下发数据的模式
 * 
 * @note AT+CTLWRECV=<mode>[,<data1>]
 * <data1>下行报文保存在缓存中的时间
 */
int at_CTLWRECV_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 读取物联网开放平台下发的数据
 * 
 * @note AT+CTLWGETRECVDATA
 */
int at_CTLWGETRECVDATA_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 设置注册模式,0:手动注册连接平台 1:上电自动连接云平台
 * 
 * @note AT+CTLWSETREGMOD=<regmode>
 */
int at_CTLWSETREGMOD_req(char *at_buf, char **prsp_cmd);






