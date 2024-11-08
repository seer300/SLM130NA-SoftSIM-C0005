#pragma once
#include "ping_api.h"
/**
 * @brief 
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_NPING_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_NPINGSTOP_req(char *at_buf, char **prsp_cmd);

/**
 * @brief 
 * 
 * @param at_buf 
 * @param prsp_cmd 
 * @return int 
 */
int at_QPING_req(char *at_buf, char **prsp_cmd);