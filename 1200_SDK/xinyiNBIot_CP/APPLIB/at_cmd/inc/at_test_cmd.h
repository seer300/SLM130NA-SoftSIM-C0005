#pragma once

/*******************************************************************************
 *						Global function declarations 					       *
 ******************************************************************************/
int at_TEST_req(char *at_buf, char **prsp_cmd);
int at_REGTEST_req(char *param,char **rsp_cmd);
int at_PHY_req(char *at_buf, char **prsp_cmd);
int at_RTC_req(char *at_buf, char **prsp_cmd);
