/** 
* @file        
* @brief       Integration of the Onomondo SoftSIM - also known as onomondo-uicc
* @warning     
*/

#pragma once

#include "xy_log.h"
#include "xy_system.h"
#include "xy_fs.h"

#include "softsim_adapt.h"

#include "onomondo/softsim/softsim.h"
#include "onomondo/utils/ss_provisioning.h"

#define APDU_MAX_LEN (256)

struct ss_apdu_response
{
    uint8_t data[APDU_MAX_LEN];
    size_t len;
};

struct ss_apdu_response apdu_rsp_buf;
struct ss_context *apductx = NULL;

osMutexId_t	g_softsim_mutex = NULL;

bool Is_softsim_type()
{
	return get_uicc_mode();
}

uint16_t softsim_reset_handler(uint8_t *atr_data, uint8_t *atr_size)
{
    *atr_size = 25;
    size_t atr_len = ss_atr(apductx, atr_data, *atr_size);

    if (atr_len == 0)
    {
		// ATR Error. Could use a log output here.
        return -1;
    }
    *atr_size = atr_len;
    return 0;
}


int softsim_apdu_handler(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len)
{
	size_t request_len = apdu_req_len;
    apdu_rsp_buf.len = ss_application_apdu_transact(apductx, apdu_rsp_buf.data, APDU_MAX_LEN + 2, apdu_req, &request_len);

    if (apdu_rsp_buf.len < 2)
        return 0x6301; // XinYi expected error code?

    int rsp = apdu_rsp_buf.data[apdu_rsp_buf.len - 2] << 8 | apdu_rsp_buf.data[apdu_rsp_buf.len - 1];

    memcpy(apdu_rsp, apdu_rsp_buf.data, apdu_rsp_buf.len);
    *apdu_rsp_len = apdu_rsp_buf.len;

    return rsp;
}


int get_uicc_mode()
{
    int uiccmode = 1;
    xy_file fp = xy_fopen("uiccmode.txt", "r", FS_DEFAULT);
    if ( fp == NULL )
    {
        return uiccmode;
    }
    int readcount = xy_fread(&uiccmode, sizeof(int), fp);
    xy_fclose(fp);
    return uiccmode;
}


// SoftSIM Application Initialization
void softsim_init()
{
    xy_printf(0, PLATFORM, DEBUG_LOG, "");

    if (onomondo_profile_provisioning() == 0)
        xy_printf(0, PLATFORM, DEBUG_LOG, "");
    else
        xy_printf(0, PLATFORM, DEBUG_LOG, "");

	if (!apductx) 
    {   
        xy_printf(0, PLATFORM, DEBUG_LOG, "");
        apductx = ss_new_ctx();

        if (!apductx)
            return -1;
        
        xy_printf(0, PLATFORM, DEBUG_LOG, "");
        ss_reset(apductx);
    }

    xy_printf(0, PLATFORM, DEBUG_LOG, "");
}


int softsim_apdu_process(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len)
{
    if(NULL == g_softsim_mutex)
    	g_softsim_mutex = osMutexNew(NULL);
	
    osMutexAcquire(g_softsim_mutex, osWaitForever);

    switch(apdu_req[0])
    {
    	case 0x62: // Pw_on
            softsim_init();
            apdu_rsp[0] = 0x90;
            apdu_rsp[1] = 0x00;
            *apdu_rsp_len = 2;
            break;

        case 0x63: // Pw_off
            // Nothing to do for SoftSIM
            apdu_rsp[0] = 0x90;
            apdu_rsp[1] = 0x00;
            *apdu_rsp_len = 2;
            break;
        
        default:
            if(0x6c32 == softsim_apdu_handler(apdu_req, apdu_req_len, apdu_rsp, apdu_rsp_len)) {
                apdu_rsp[0] = 0x90;
                apdu_rsp[1] = 0x00;
                *apdu_rsp_len = 2;
            }
            break;
    }

    osMutexRelease(g_softsim_mutex);
}
