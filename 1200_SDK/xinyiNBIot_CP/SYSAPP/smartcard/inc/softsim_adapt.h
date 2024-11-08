#pragma once

#include <stdint.h>

bool Is_softsim_type();

void softsim_init();

int softsim_apdu_process(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len);

int softsim_apdu_handler(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len);

uint16_t softsim_reset_handler(uint8_t *atr_data, uint8_t *atr_size);


int get_uicc_mode();