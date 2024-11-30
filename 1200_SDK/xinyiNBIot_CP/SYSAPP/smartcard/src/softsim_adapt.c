/** 
* @file     softsim_adapt.c
* @date     2024-06-25
* @author   Onomondo
* @brief    Integration of the Onomondo SoftSIM - also known as onomondo-uicc
*/

#include "xy_log.h"
#include "xy_system.h"
#include "xy_fs.h"
#include "softsim_adapt.h"
#include "onomondo/softsim/softsim.h"
#include "onomondo/utils/ss_provisioning.h"

#define APDU_MAX_LEN    256

static struct ss_apdu_response
{
    uint8_t data[APDU_MAX_LEN];
    size_t len;
};

static struct ss_apdu_response apdu_rsp_buf;
static struct ss_context *apductx = NULL;

static osMutexId_t	g_softsim_mutex = NULL;

/**
 * @brief Check if SoftSIM has been selected as active UICC
 * 
 * @return true if SoftSIM has actively been selected
 */
bool Is_softsim_type()
{
    int uicc_mode_readout = get_uicc_mode();

    if (uicc_mode_readout == 1)
        return true;
    else
        return false;    
}

/**
 * @brief Get the UICC mode from the filesystem
 * 
 * @return  the value written to uiccmode.txt
 */
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

/**
 * @brief Get an ATR (without resetting the UICC state)
 * 
 * @param atr_data memory to store the resulting ATR
 * @param atr_size length of the resulting ATR
 * @return 0 on success
 */
static int softsim_reset_handler(uint8_t *atr_data, uint8_t *atr_size)
{
    *atr_size = 25;
    size_t atr_len = ss_atr(apductx, atr_data, *atr_size);

    if (atr_len == 0)
    {
		xy_printf(0, PLATFORM, DEBUG_LOG, "atr len = 0, forcing an error");
        return -1;
    }

    *atr_size = atr_len;
    return 0;
}

/**
 * @brief onomondo-uicc APDU processor
 * 
 * @param apdu_req      memory with request APDU
 * @param apdu_req_len  length of the request APDU
 * @param apdu_rsp      memory to store the resulting response APDU (at least 2+256 bytes)
 * @param apdu_rsp_len  memory to store the resulting response len
 * @return APDU response data
 */
static int softsim_apdu_handler(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len)
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

/**
 * @brief SoftSIM initialization process
 * 
 *  This is the initial initialization of the SoftSIM profile and context.
 *  It is expected for the SoftSIM profile to be provisioned once and for it to remain 
 *  persistent in the duration of the NVM FLASH located filesystem.
 */
static void softsim_init()
{
    xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Application Initialization");

    if (onomondo_profile_provisioning() == 0)
        xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Provisioning Succcesfully Written");
    else
        xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Provisioning inclomplete... error occured");

	if (!apductx) 
    {   
        xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Context Generation");
        apductx = ss_new_ctx();

        if (!apductx)
            return;
        
        xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Context Reset");
        ss_reset(apductx);
    }

    xy_printf(0, PLATFORM, DEBUG_LOG, "SoftSIM Application Initialization Success");
}

/**
 * @brief onomondo-uicc (SoftSIM) entrypoint
 *  The main entrypoint of the Onomondo SoftSIM with
 *  additional power on/off options to initialize the
 *  SoftSIM filesystem and context.
 * 
 * @param apdu_req      memory with request APDU
 * @param apdu_req_len  length of the request APDU
 * @param apdu_rsp      memory to store the resulting response APDU (at least 2+256 bytes)
 * @param apdu_rsp_len  memory to store the resulting response len 
 */
void softsim_apdu_process(uint8_t *apdu_req, uint16_t apdu_req_len, uint8_t *apdu_rsp, uint16_t *apdu_rsp_len)
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
            /* TODO: validate if it is required for us to validate the existence of the ctx 
            before this function call. the apdu ctx reference are not checked from here on. */ 
            if(0x6c32 == softsim_apdu_handler(apdu_req, apdu_req_len, apdu_rsp, apdu_rsp_len)) {
                apdu_rsp[0] = 0x90;
                apdu_rsp[1] = 0x00;
                *apdu_rsp_len = 2;
            }
            break;
    }

    osMutexRelease(g_softsim_mutex);
}
