/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_ctl.h"
#include "xy_utils.h"

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
void at_rcv_from_nearps(void *buf, unsigned int len)
{

	xy_assert(strlen(buf) != 0 && len == strlen(buf));

	xy_printf(0,PLATFORM, WARN_LOG, "recv from NAS: %s\n", buf);

	send_msg_2_atctl(AT_MSG_RCV_STR_FROM_NEARPS, buf, len, &nearps_ctx);

}


