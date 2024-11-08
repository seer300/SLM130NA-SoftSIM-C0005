#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_NV_req(char *at_buf, char **prsp_cmd);
int at_ATI_req(char *at_buf,char **prsp_cmd);
int at_NUESTATS_req(char *at_buf, char **prsp_cmd);
int at_CMVER_req(char *at_buf, char **prsp_cmd);
int at_HVER_req(char *at_buf, char **prsp_cmd);
int at_CGMM_req(char *at_buf, char **prsp_cmd);
int at_CGMI_req(char *at_buf, char **prsp_cmd);
int at_NPSMR_req(char *at_buf, char **prsp_cmd);
int at_QGMR_req(char *at_buf, char **prsp_cmd);
int at_QVERTIME_req(char *at_buf, char **prsp_cmd);
int at_DUMP_req(char *at_buf, char **prsp_cmd);
int at_SGSW_req(char *at_buf, char **prsp_cmd);
int at_CGMR_req(char *at_buf, char **prsp_cmd);
int at_mgsleep_req(char *at_buf, char **prsp_cmd);
int at_NGT3412_req(char *at_buf, char **prsp_cmd);
int at_SOFTSIM_req(char *at_buf, char **prsp_cmd);
int at_SWUICC_req(char *at_buf, char **prsp_cmd);



