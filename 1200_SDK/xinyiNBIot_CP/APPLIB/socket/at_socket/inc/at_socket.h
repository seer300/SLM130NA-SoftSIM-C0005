#pragma once

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
/* 1100命令集 */
int at_SOCKCFG_req(char *at_buf, char **prsp_cmd);
int at_SOCKCONN_req(char *at_buf, char **prsp_cmd);
int at_SOCKSEND_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENDEX_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENT_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTEX_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTF_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTFEX_req(char *at_buf, char **prsp_cmd);
int at_SOCKCLZ_req(char *at_buf, char **prsp_cmd);
int at_SOCKSTAT_req(char *at_buf, char **prsp_cmd);
int at_SOCKNMI_req(char *at_buf, char **prsp_cmd);
int at_SOCKSEQ_req(char *at_buf, char **prsp_cmd);
int at_SOCKRF_req(char *at_buf, char **prsp_cmd);
int at_SOCKQSOS_req(char *at_buf, char **prsp_cmd);
int at_SOCKDTMD_req(char *at_buf, char **prsp_cmd);

int at_SOCKSTR_Default_URC(int id, uint16_t seqno, int8_t state);
void at_SOCKRFNMI_Default_URC(void *arg);
int at_SOCKCLZ_Default_URC(int id, bool isquit);
int at_SOCKDATAMODE_EXIT_Default_URC(void *arg);
int at_SOCKNMI_Default_URC(int id, uint32_t read_len, char *buf, void *remoteinfo);

/* BC95命令集及回调 */
int at_SOCKQSOS_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKCFG_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENT_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTF_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKRF_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKCLZ_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKCONN_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSEND_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENDEX_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTEX_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSENTFEX_BC95_req(char *at_buf, char **prsp_cmd);
int at_SOCKSTAT_BC95_req(char *at_buf, char **prsp_cmd);

int at_SOCKCLZ_BC95_URC(int id, bool isquit);
int at_SOCKCONN_BC95_URC(int id, int err);

/* BC25命令集及回调 */
int at_QICFG_BC25_req(char *at_buf, char **prsp_cmd);
int at_QIOPEN_BC25_req(char *at_buf, char **prsp_cmd);
int at_QICLOSE_BC25_req(char *at_buf, char **prsp_cmd);
int at_QISTATE_BC25_req(char *at_buf, char **prsp_cmd);
int at_QISEND_BC25_req(char *at_buf, char **prsp_cmd);
int at_QIRD_req(char *at_buf, char **prsp_cmd);
int at_QISENDEX_req(char *at_buf, char **prsp_cmd);
int at_QISWTMD_req(char *at_buf, char **prsp_cmd);

int at_SOCKCLZ_BC25_URC(int id, bool isquit);
int at_SOCKNMI_BC25_URC(int sock_id, uint32_t len, char *data, void *remoteinfo);
int at_SOCKDATAMODE_EXIT_BC25_URC(void *arg);

/* BC25、BC26命令集及回调 */
int at_QICFG_260Y_req(char *at_buf, char **prsp_cmd);
int at_QIOPEN_260Y_req(char *at_buf, char **prsp_cmd);
int at_QICLOSE_260Y_req(char *at_buf, char **prsp_cmd);
int at_QISTATE_260Y_req(char *at_buf, char **prsp_cmd);
int at_QISEND_260Y_req(char *at_buf, char **prsp_cmd);

int at_SOCKCLZ_260Y_URC(int id, bool isquit);
int at_SOCKNMI_260Y_URC(int sock_id, uint32_t len, char *data, void *remoteinfo);
int at_SOCKDATAMODE_EXIT_260Y_URC(void *arg);

/* URC回调抽象 */
void regist_socket_callback(void);
int at_SOCKSTR_URC(int id, uint16_t seqno, int8_t state);
int at_SOCKNMI_URC(int id, uint32_t recv_len, char* data, void* remote_info);
int at_SOCKCLZ_URC(int id, bool isquit);
void at_SOCKRFNMI_URC(void *arg);
