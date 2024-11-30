/** 
* @file     softsim_adapt.h
* @date     2024-06-25
* @author   Onomondo
* @brief    Header file for the integration of the Onomondo SoftSIM - also known as onomondo-uicc
*/

#pragma once

#include <stdint.h>

bool Is_softsim_type();

void softsim_apdu_process(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len);