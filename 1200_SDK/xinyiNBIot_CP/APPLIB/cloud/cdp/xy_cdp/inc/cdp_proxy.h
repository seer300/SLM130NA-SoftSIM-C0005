#ifndef _XY_CDP_PROXY_H
#define _XY_CDP_PROXY_H

proxy_recv_callback cdpProxyRecvProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
proxy_send_callback cdpProxySendProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
proxy_config_callback cdpProxyConfigProc(uint8_t req_type, uint8_t* paramList, uint8_t **prsp_cmd);
#endif
