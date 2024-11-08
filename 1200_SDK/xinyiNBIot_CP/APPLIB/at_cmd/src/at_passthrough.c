/**
 * @file at_passthrough.c
 * @brief 
 * @version 1.0
 * @date 2021-04-29
 * @copyright Copyright (c) 2021  芯翼信息科技有限公司
 * 
 */

#include "at_passthrough.h"
#include "at_ctl.h"
#include "xy_at_api.h"


/*******************************************************************************
 *						Local function implementations						   *
 ******************************************************************************/
bool find_ppp_quit_symbol(char *buf, uint32_t data_len)
{
	if ((data_len == 3) && (memcmp(buf, PASSTHR_PPP_QUIT_SYMBOL, 3) == 0))
	{
		return true;
	}
	return false;
}

/************* SMS BEGIN *********************/
/**
 * @brief 处理短信sms透传数据
 * @note 如果接收到短信结束标识符Ctrl+Z或者ESC，退出透传模式
 */
extern void xy_atc_data_req(unsigned short usDataLen, unsigned char*pucData);
void at_sms_passthr_proc(char *buf, uint32_t data_len)
{
    char *ret_rsp = NULL;
    if (passthr_rcv_buff == NULL)
	{
		passthr_rcv_buff = xy_malloc(PASSTHR_BLOCK_LEN);
		passthr_dynmic_buff_len = PASSTHR_BLOCK_LEN;
	}

    if (passthr_dynmic_buff_len < passthr_rcvd_len + data_len)
    {
        char *new_mem = xy_malloc(passthr_dynmic_buff_len + PASSTHR_BLOCK_LEN);
        if (passthr_rcvd_len != 0)
            memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = new_mem;
        passthr_dynmic_buff_len += PASSTHR_BLOCK_LEN;
    }

    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, data_len);
    passthr_rcvd_len += data_len;

	if (buf[data_len - 1] == PASSTHR_CTRLZ || buf[data_len - 1] == PASSTHR_ESC)
	{
		void *sms_data = NULL;
		sms_data = (void *)xy_malloc(passthr_rcvd_len);
		memcpy((char *)sms_data, passthr_rcv_buff, passthr_rcvd_len);
        xy_atc_data_req(passthr_rcvd_len, sms_data);
        xy_free(sms_data);
		goto END;
	}
	else
    {
        //未收到ctrl+z or esc
        return;
    }

END:
    xy_exitPassthroughMode();
    return;
}

void at_sms_passthr_exit()
{
    if (passthr_rcv_buff != NULL)
    {
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = NULL;
    }
    passthr_rcvd_len = 0;
    passthr_fixed_buff_len = 0;
    passthr_dynmic_buff_len = 0;
}
/************* SMS END *********************/

/************* ATD BEGIN *********************/
void atd_passthrough_exit(void)
{
    if (passthr_rcv_buff != NULL)
    {
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = NULL;
    }
    passthr_rcvd_len = 0;
    passthr_fixed_buff_len = 0;
    passthr_dynmic_buff_len = 0;
	send_rsp_at_to_ext("\r\nNO CARRIER\r\n\r\nOK\r\n");
}

void atd_passthrough_proc(char *buf, uint32_t len)
{
    xy_printf(0,XYAPP, WARN_LOG, "atd recv len:%d", len);
	if (find_ppp_quit_symbol(buf, len))
    {
		xy_exitPassthroughMode();
        return;
    }

    //用户记录并处理数据
    if (passthr_rcv_buff == NULL)
	{
		passthr_rcv_buff = xy_malloc(PASSTHR_BLOCK_LEN);
		passthr_dynmic_buff_len = PASSTHR_BLOCK_LEN;
	}

    if (passthr_dynmic_buff_len < passthr_rcvd_len + len)
    {
        char *new_mem = xy_malloc(passthr_dynmic_buff_len + PASSTHR_BLOCK_LEN);
        if (passthr_rcvd_len != 0)
            memcpy(new_mem, passthr_rcv_buff, passthr_rcvd_len);
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = new_mem;/*  */
        passthr_dynmic_buff_len += PASSTHR_BLOCK_LEN;
    }

    memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, len);
    passthr_rcvd_len += len;
}

/**
 * @brief  PPP协议中的ATD切换透传模式的处理函数
 * @note   
 */
int at_ATD_req(char *at_buf,char **prsp_cmd)
{
    if (g_req_type == AT_CMD_ACTIVE)
    {
		if (at_prefix_strstr(at_buf, "*98") != NULL || at_prefix_strstr(at_buf, "*99") != NULL)
		{
			xy_enterPassthroughMode((app_passthrough_proc)atd_passthrough_proc, (app_passthrough_exit)atd_passthrough_exit);
            *prsp_cmd = xy_malloc(32);
            snprintf(*prsp_cmd, 32, "\r\nCONNECTING\r\n");
        }
		else
		{
			return ATERR_PARAM_INVALID;
		}
	}	
	return AT_END;
}
/************* ATD END *********************/

/************* FIXED LENGTH DEMO START *********************/
void fixed_length_passthr_proc(char *buf, uint32_t len)
{
	xy_printf(0,XYAPP, WARN_LOG, "fixed recv len:%d", len);
    if (passthr_fixed_buff_len == 0)
        return;

    //申请内存
    if (passthr_rcv_buff == NULL)
    {
        passthr_rcv_buff = xy_malloc(passthr_fixed_buff_len);
    }

    if ((passthr_rcvd_len + len) < passthr_fixed_buff_len)
    {
		memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, len);
        passthr_rcvd_len += len;
    }
	else
	{
		if (passthr_rcvd_len < passthr_fixed_buff_len)
		{ 
			memcpy(passthr_rcv_buff + passthr_rcvd_len, buf, passthr_fixed_buff_len - passthr_rcvd_len);
			passthr_rcvd_len = passthr_fixed_buff_len;
		}

        //数据处理      
        
        //退出透传
		xy_exitPassthroughMode();
	}
}

void fixed_length_passthr_exit(void)
{
    //释放内存
    if (passthr_rcv_buff != NULL)
    {
        xy_free(passthr_rcv_buff);
        passthr_rcv_buff = NULL;
    }
    passthr_rcvd_len = 0;
    passthr_fixed_buff_len = 0;
    send_rsp_at_to_ext("\r\nNO CARRIER\r\n\r\nOK\r\n");
}

int at_TRANSPARENTDEMO_req(char *at_buf,char **prsp_cmd)
{
    if (g_req_type == AT_CMD_REQ)
    {
		uint32_t data_len = 0;

		if (at_parse_param("%d", at_buf, &data_len) != AT_OK) 
		{
			return ATERR_PARAM_INVALID;
		}
		else
		{
            passthr_fixed_buff_len = data_len;
			xy_enterPassthroughMode((app_passthrough_proc)fixed_length_passthr_proc, (app_passthrough_exit)fixed_length_passthr_exit);
        }
		*prsp_cmd = xy_malloc(32);
		snprintf(*prsp_cmd, 32, "\r\nCONNECT\r\n");
	}
	else
	{
		return ATERR_PARAM_INVALID;
	}
	return AT_END;
}
/************* FIXED LENGTH DEMO END *********************/