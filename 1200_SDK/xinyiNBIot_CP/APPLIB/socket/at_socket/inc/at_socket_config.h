#pragma once

#define SEQUENCE_MAX                255
#define SOCK_RECV_TIMEOUT           1       //seconds
#define AT_SOCKET_RCV_NODE_MAX		50		/* at socket rcv线程最多缓存的数据节点个数 */
#define SOCK_RCV_BUF_MAX 			4000  	//at socket下行数据缓存上限

#if VER_BC95
#define SOCK_NUM                    7
#define AT_SOCKET_MAX_DATA_LEN      1500
#elif VER_BC25
#define SOCK_NUM                    6
#define AT_SOCKET_MAX_DATA_LEN      1440
#elif VER_260Y
#define SOCK_NUM                    5
#define AT_SOCKET_MAX_DATA_LEN      1400
#else
#define SOCK_NUM                    2
#define AT_SOCKET_MAX_DATA_LEN      1400
#endif //VER_BC95