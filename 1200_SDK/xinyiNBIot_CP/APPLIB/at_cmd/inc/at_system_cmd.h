#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_ASSERT_req(char *at_buf, char **prsp_cmd);
int at_RB_req(char *at_buf, char **prsp_cmd);
int at_CPOF_req(char *at_buf,char **prsp_cmd);
int at_RESET_req(char *at_buf, char **prsp_cmd);
int at_NITZ_req(char *at_buf, char **prsp_cmd);
int at_CCLK_req(char *at_buf, char **prsp_cmd);
int at_STANDBY_req(char *at_buf, char **prsp_cmd);

/*******************************************************************************
 *								BC95系统指令集 							       *
 ******************************************************************************/
int at_NITZ_BC95_req(char *at_buf, char **prsp_cmd);
int at_CCLK_BC95_req(char *at_buf, char **prsp_cmd);
int at_QSCLK_BC95_req(char *at_buf, char **prsp_cmd);

/*******************************************************************************
 *								BC25系统指令集 							       *
 ******************************************************************************/
int at_ANDW_req(char *at_buf, char **prsp_cmd);
int at_CCLK_BC25_req(char *at_buf, char **prsp_cmd);
int at_QCCLK_req(char *at_buf, char **prsp_cmd);
int at_QSCLK_req(char *at_buf, char **prsp_cmd);
int at_QATWAKEUP_req(char *at_buf, char **prsp_cmd);
int at_QPOWD_req(char *at_buf,char **prsp_cmd);

/*******************************************************************************
 *								BC260Y系统指令集 						       *
 ******************************************************************************/
int at_QCFG_260Y_req(char *at_buf, char **prsp_cmd);
int at_QSCLK_260Y_req(char *at_buf, char **prsp_cmd);
int at_CCLK_260Y_req(char *at_buf, char **prsp_cmd);