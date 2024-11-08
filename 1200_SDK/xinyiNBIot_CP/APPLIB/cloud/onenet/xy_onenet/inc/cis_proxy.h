#ifndef _XY_CIS_PROXY_H
#define _XY_CIS_PROXY_H

proxy_recv_callback cisProxyRecvProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
proxy_send_callback cisProxySendProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
proxy_config_callback cisProxyConfigProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
#endif
