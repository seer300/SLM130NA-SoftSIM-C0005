#pragma once

#include "xy_passthrough.h"

/*******************************************************************************
 *                       Global function declarations                          *
 ******************************************************************************/
int at_ATD_req(char *at_buf, char **prsp_cmd);
int at_TRANSPARENTDEMO_req(char *at_buf, char **prsp_cmd);
void at_sms_passthr_proc(char *buf, uint32_t data_len);
void at_sms_passthr_exit();
