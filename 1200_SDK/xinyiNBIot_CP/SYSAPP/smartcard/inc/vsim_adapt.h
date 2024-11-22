/** 
* @file        
* @brief       vsim适配层，内部不得长时间锁中断，以防止影响3GPP的时序。默认硬SIM，通过AT+NV=SET,VSIM,1可以切换为软SIM
* @warning     
*/

#pragma once
#include <ctype.h>
#include <float.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>






bool Is_vsim_type();

int vsim_apdu_process(uint8_t *apdu_req,uint16_t apdu_req_len,uint8_t *apdu_rsp,uint16_t *apdu_rsp_len);








