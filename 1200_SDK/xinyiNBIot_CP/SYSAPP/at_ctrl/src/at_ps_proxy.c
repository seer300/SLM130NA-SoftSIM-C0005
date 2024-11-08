/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ps_proxy.h"
#include "at_ctl.h"
#include "oss_nv.h"
#include "nbiot_ps_export_interface.h"
#include "xy_atc_interface.h"
#include "xy_utils.h"
#include "at_worklock.h"

extern void xy_atc_data_req(unsigned short usDataLen, unsigned char*pucData);


/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
//平台实现，PS调用，发送PS相关的URC及结果码，参考  at_rcv_from_nearps/send_rsp_at_to_ext
void SendAtInd2User(char *pAt, unsigned int ulAtLen)
{
    xy_printf(0,PLATFORM, WARN_LOG, "[SendAtInd2User] %s",(const char *)pAt);
	send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, pAt,ulAtLen, &nearps_ctx);
}

//PS实现，平台调用，发送从串口接收来的PS相关AT命令给ATC
void SendAt2AtcAp(char *pAt, unsigned int ulAtLen)
{
	//SendAtInd2User("\r\nOK\r\n",strlen("\r\nOK\r\n"));
    xy_printf(0,PLATFORM, WARN_LOG, "SendAt2AtcAp:%s",pAt);
    xy_atc_data_req(ulAtLen, (unsigned char*)pAt);
}

bool is_urc_drop(void)
{
    if(IsMtNetTest() && IS_WAKEUP_BY_RTC() && g_softap_var_nv->ps_deepsleep_state == 2)
        return true;
    return false;
}

int g_ps_lock = 0;
/*AT框架中，收到AT命令进行定制拦截，识别PS的一些特殊AT命令处理*/
void special_ps_at_proc(char *at_str)
{
	UNUSED_ARG(at_str);
	/*仅针对模组形态进行PS的AT命令定制处理，OPENCPU的不考虑*/
	if (Is_OpenCpu_Ver())
		return;
}