#pragma once

/*******************************************************************************
 *						    Global function declarations 			           *
 ******************************************************************************/
int at_XDNSCFG_req(char *at_buf, char **prsp_cmd);
int at_QIDNSCFG_req(char *at_buf, char **prsp_cmd);
int at_XDNS_req(char *at_buf, char **prsp_cmd);
int at_QDNS_req(char *at_buf, char **prsp_cmd);
int at_CMDNS_req(char *at_buf, char **prsp_cmd);
int at_CMNTP_req(char *at_buf, char **prsp_cmd);
int at_XNTP_req(char *at_buf, char **prsp_cmd);
int at_QNTP_req(char *at_buf, char **prsp_cmd);
int at_QIDNSGIP_req(char *at_buf, char **prsp_cmd);
int at_QIDNSCFG_req2(char *at_buf, char **prsp_cmd);
int at_QIGETERROR_req(char *at_buf, char **prsp_cmd);