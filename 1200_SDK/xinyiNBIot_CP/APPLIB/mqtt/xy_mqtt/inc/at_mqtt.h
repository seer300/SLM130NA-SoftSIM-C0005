#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_QMTCFG_req(char *at_buf, char **prsp_cmd);
int at_QMTOPEN_req(char *at_buf, char **prsp_cmd);
int at_QMTCLOSE_req(char *at_buf, char **prsp_cmd);
int at_QMTCONN_req(char *at_buf, char **prsp_cmd);
int at_QMTDISC_req(char *at_buf, char **prsp_cmd);
int at_QMTSUB_req(char *at_buf, char **prsp_cmd);
int at_QMTUNS_req(char *at_buf, char **prsp_cmd);
int at_QMTPUB_req(char *at_buf, char **prsp_cmd);

int at_MQNEW_req(char *at_buf, char **prsp_cmd);
int at_MQCON_req(char *at_buf, char **prsp_cmd);
int at_MQDISCON_req(char *at_buf, char **prsp_cmd);
int at_MQSUB_req(char *at_buf, char **prsp_cmd);
int at_MQUNSUB_req(char *at_buf, char **prsp_cmd);
int at_MQPUB_req(char *at_buf, char **prsp_cmd);
int at_MQSTATE_req(char *at_buf, char **prsp_cmd);

