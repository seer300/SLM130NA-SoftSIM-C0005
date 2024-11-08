/****************************************************************************

            (c) Copyright 2019 by 天翼物联科技有限公司. All rights reserved.

****************************************************************************/

#ifndef _CTLW_AT_CONFIG_H
#define _CTLW_AT_CONFIG_H

#define AT_CTLWSETSERVER_TEST_STR "+CTLWSETSERVER: (0-1),(0-1),,(0-65535)"

#define AT_CTLWSETLT_TEST_STR "+CTLWSETLT: (300-2592000)"

#define AT_CTLWSETPSK_TEST_STR "+CTLWSETPSK: (0-1)"

#define AT_CTLWSETAUTH_TEST_STR "+CTLWAUTH: (0-2)"

#define AT_CTLWSETPCRYPT_TEST_STR "+CTLWSETPCRYPT: (0-1)"

#define AT_CTLWSETMOD_TEST_STR "+CTLWSETMOD: 1,(0-2)\r\n+CTLWSETMOD: 2,(0-1)\r\n+CTLWSETMOD: 3,(0-4)\r\n+CTLWSETMOD: 4,(0-2,9)\r\n+CTLWSETMOD: 5,(0-1)"

#define AT_CTLWREG_TEST_STR ""

#define AT_CTLWUPDATE_TEST_STR "+CTLWUPDATE: (0-1)"

#define AT_CTLWDEREG_TEST_STR "+CTLWDEREG: (0-1)"

#define AT_CTLWGETSTATUS_TEST_STR "+CTLWGETSTATUS: (0-7)"

#define AT_CTLWCFGRST_TEST_STR "+CTLWCFGRST: (0)"

#define AT_CTLWSESDATA_TEST_STR "+CTLWSESDATA: (0)"

#define AT_CTLWSEND_TEST_STR "+CTLWSEND: (0-4)"

#define AT_CTLWRECV_TEST_STR "+CTLWRECV: (0-2),(0-8)"

#define AT_CTLWGETRECVDATA_TEST_STR ""

#define AT_CTLWDTLSHS_TEST_STR ""

#endif
