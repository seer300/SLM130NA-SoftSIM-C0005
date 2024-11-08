/*
 * @file	ap_at_cmd.h
 * @brief	处理注册在AP核的AT命令；
*/
#pragma once

typedef int (*at_cmd_fun)(char *buf, char **rsp_cmd);	//AT命令的注册回调函数声明

typedef enum
{
	AT_CMD_INVALID = 0, // 当AT命令类型不属于以下任何一类时，直接转发至CP处理
	AT_CMD_REQ,			// AT+XXX=param
	AT_CMD_ACTIVE,		// AT+XXX
	AT_CMD_QUERY,		// AT+XXX?
	AT_CMD_TEST,		// AT+XXX=?
} at_cmd_type_t;


typedef struct
{
	char *at_prefix;
	at_cmd_fun proc;
}at_cmd_t;

extern uint8_t g_cmd_type;
extern const at_cmd_t g_AT_cmd_list[];

int Match_AT_Cmd(char *buf);
int at_DEMOCFG_req(char *param,char **rsp_cmd);

int at_ASSERT_req(char *param,char **res_cmd);
int at_SOCID_req(char *param,char **rsp_cmd);
int at_REGTEST_req(char *param,char **res_cmd);
int at_APTEST_req(char *at_buf, char **prsp_cmd);
int at_LOCK_req(char *param,char **rsp_cmd);
int at_BOOTCP_req(char *param,char **rsp_cmd);
int at_RB_req(char *at_buf, char **prsp_cmd);
int at_RESET_req(char *at_buf, char **prsp_cmd);
int at_APMEMSTATUS_req(char *param, char **rsp_cmd);
int at_FLASHTEST_req(char *param,char **rsp_cmd);
int at_QRST_req(char *param,char **rsp_cmd);
int at_QSCLK_req(char *param,char **rsp_cmd);
int at_QPOWD_req(char *param,char **rsp_cmd);
int at_QRST_req2(char *param,char **rsp_cmd);
int at_IPR_req2(char  *param, char **rsp_cmd);
