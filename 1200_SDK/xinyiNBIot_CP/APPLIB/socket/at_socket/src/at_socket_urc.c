
#include "xy_system.h"
#include "at_socket.h"

typedef void (*SocketStr_URC_Cb)(int id, uint16_t seqno, int8_t state);
SocketStr_URC_Cb p_SOCKSTR_URC_Hook = NULL;
void SOCKSTR_URC_Regist(SocketStr_URC_Cb pfun)
{
	p_SOCKSTR_URC_Hook = pfun;
}

typedef void (*SocketNmi_URC_Cb)(int id, uint32_t read_len, char *buf, void *remoteinfo);
SocketNmi_URC_Cb p_SOCKNMI_URC_Hook = NULL;
void SOCKNMI_URC_Regist(SocketNmi_URC_Cb pfun)
{
	p_SOCKNMI_URC_Hook = pfun;
}

typedef void (*SocketClz_URC_Cb)(int id, bool isquit);
SocketClz_URC_Cb p_SOCKCLZ_URC_Hook = NULL;
void SOCKCLZ_URC_Regist(SocketClz_URC_Cb pfun)
{
	p_SOCKCLZ_URC_Hook = pfun;
}

typedef void (*SocketRfNmi_URC_Cb)(void *arg);
SocketRfNmi_URC_Cb p_SOCKRFNMI_URC_Hook = NULL;
void SOCKRFNMI_URC_Regist(SocketRfNmi_URC_Cb pfun)
{
	p_SOCKRFNMI_URC_Hook = pfun;
}

typedef void (*SocketDataModeExit_URC_Cb)(void *arg);
SocketDataModeExit_URC_Cb p_SOCKDATAMODE_EXIT_URC_Hook = NULL;
void SOCKDATAMODE_EXIT_URC_Regist(SocketDataModeExit_URC_Cb pfun)
{
	p_SOCKDATAMODE_EXIT_URC_Hook = pfun;
}

typedef void (*SocketConnect_URC_Cb)(int id, int err);
SocketConnect_URC_Cb p_SOCKCONN_URC_Hook = NULL;
void SOCKCONN_URC_Regist(SocketConnect_URC_Cb pfun)
{
	p_SOCKCONN_URC_Hook = pfun;
}

void regist_socket_callback(void)
{
#if VER_BC95
	SOCKSTR_URC_Regist(at_SOCKSTR_Default_URC);
	SOCKNMI_URC_Regist(at_SOCKNMI_Default_URC);
	SOCKCLZ_URC_Regist(at_SOCKCLZ_BC95_URC);
	SOCKRFNMI_URC_Regist(at_SOCKRFNMI_Default_URC);
	SOCKDATAMODE_EXIT_URC_Regist(at_SOCKDATAMODE_EXIT_Default_URC);
	SOCKCONN_URC_Regist(at_SOCKCONN_BC95_URC);
#elif VER_BC25
	SOCKNMI_URC_Regist(at_SOCKNMI_BC25_URC);
	SOCKCLZ_URC_Regist(at_SOCKCLZ_BC25_URC);
	SOCKDATAMODE_EXIT_URC_Regist(at_SOCKDATAMODE_EXIT_BC25_URC);
#elif VER_260Y
	SOCKNMI_URC_Regist(at_SOCKNMI_260Y_URC);
	SOCKCLZ_URC_Regist(at_SOCKCLZ_260Y_URC);
	SOCKDATAMODE_EXIT_URC_Regist(at_SOCKDATAMODE_EXIT_260Y_URC);
#else
	SOCKSTR_URC_Regist(at_SOCKSTR_Default_URC);
	SOCKNMI_URC_Regist(at_SOCKNMI_Default_URC);
	SOCKCLZ_URC_Regist(at_SOCKCLZ_Default_URC);
	SOCKRFNMI_URC_Regist(at_SOCKRFNMI_Default_URC);
	SOCKDATAMODE_EXIT_URC_Regist(at_SOCKDATAMODE_EXIT_Default_URC);
#endif
}

int at_SOCKSTR_URC(int id, uint16_t seqno, int8_t state)
{
	if (p_SOCKSTR_URC_Hook != NULL)
		p_SOCKSTR_URC_Hook(id, seqno, state);
	return XY_OK;
}

int at_SOCKNMI_URC(int id, uint32_t read_len, char *buf, void *remoteinfo)
{
	if (p_SOCKNMI_URC_Hook != NULL)
		p_SOCKNMI_URC_Hook(id, read_len, buf, remoteinfo);
	return XY_OK;
}

int at_SOCKCLZ_URC(int id, bool isquit)
{
	if (p_SOCKCLZ_URC_Hook != NULL)
		p_SOCKCLZ_URC_Hook(id, isquit);
	return XY_OK;
}

void at_SOCKRFNMI_URC(void *arg)
{
	if (p_SOCKRFNMI_URC_Hook != NULL)
		p_SOCKRFNMI_URC_Hook(arg);
}

int at_SOCKDATAMODE_EXIT_URC(void *arg)
{
	if (p_SOCKDATAMODE_EXIT_URC_Hook != NULL)
		p_SOCKDATAMODE_EXIT_URC_Hook(arg);
	return XY_OK;
}

int at_SOCKCONN_URC(int id, int err)
{
	if (p_SOCKCONN_URC_Hook != NULL)
		p_SOCKCONN_URC_Hook(id, err);
	return XY_OK;
}