#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_CGATT_req(char *at_buf, char **prsp_cmd);
int at_ECHOMODE_req(char *at_buf, char **prsp_cmd);
int at_NSET_req(char *at_buf, char **prsp_cmd);
int at_XYRAI_req(char *at_buf,char **prsp_cmd);//AT+XYRAI=<remote IP>,<remote port>,send null packet and rai=1
int at_CPINFO_req(char *at_buf,char **prsp_cmd);//依次输出：设备版本号，软件版本号，imei，imsi，usim，apn信息
int at_CMEE_req(char *at_buf, char **prsp_cmd);
int at_ngsearfcn_req(char *at_buf,char **prsp_cmd);
