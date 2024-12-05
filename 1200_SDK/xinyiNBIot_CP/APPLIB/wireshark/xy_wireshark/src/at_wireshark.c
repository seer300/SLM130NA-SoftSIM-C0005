#include "at_wireshark.h"
#include "xy_at_api.h"
#include "xy_utils.h"
#include "xy_wireshark.h"

#if XY_WIRESHARK

//AT+WIRESHARK=<capture_type>
//capture_type:1-将M3接收到的数据以wireshark形式输出，0-取消M3输出数据
//用于M3判断是否接收到协议栈上报的数据
int at_WIRESHARK_req(char *at_buf, char **prsp_cmd)
{
	int capture_type = -1;
	struct common_msg *msg = NULL;
	osThreadAttr_t task_attr = {0};

	if (at_parse_param("%d", at_buf, &capture_type) != AT_OK || (capture_type != 1 && capture_type != 0))
	{
		return  (ATERR_PARAM_INVALID);
	}

	xy_printf(0,XYAPP, WARN_LOG, "wireshark origin: %d, new: %d", g_wireshark_set, capture_type);
	g_wireshark_set = capture_type;

	if (capture_type == 1)
	{
		if (g_wireshark_TskHandle == NULL)
		{
			task_attr.name = "wireshark_task";
			task_attr.priority = osPriorityNormal1;
			task_attr.stack_size = osStackShared;
			g_wireshark_TskHandle = osThreadNew((osThreadFunc_t)(wireshark_task), NULL, &task_attr);
		}
	}
	else
	{
		if (g_wireshark_TskHandle != NULL)
		{
			msg = xy_malloc(sizeof(struct common_msg));
			msg->msg_id = WIRESHARK_END;
			osMessageQueuePut(wireshark_msg_q, &msg, 0, osWaitForever);
		}
	}
	return AT_END;
}
#endif //XY_WIRESHARK
