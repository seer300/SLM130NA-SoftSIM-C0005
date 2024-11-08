#ifndef _AT_COAP_H_
#define _AT_COAP_H_

int at_COAPCREATE_req(char *at_buf, char **prsp_cmd);
int at_COAPDEL_req(char *at_buf, char **prsp_cmd);
int at_COAPHEAD_req(char *at_buf, char **prsp_cmd);
int at_COAPOPTION_req(char *at_buf, char **prsp_cmd);
int at_COAPSEND_req(char *at_buf, char **prsp_cmd);
int at_QCOAPDATASTATUS_req(char *at_buf, char **prsp_cmd);
int at_QCOAPADDRES_req(char *at_buf, char **prsp_cmd);
int at_QCOAPCFG_req(char *at_buf, char **prsp_cmd);
int at_QCOAPSEND_req(char *at_buf, char **prsp_cmd);
int at_QCOAPHEAD_req(char *at_buf, char **prsp_cmd);
int at_QCOAPCREATE_req(char *at_buf, char **prsp_cmd);

#endif
