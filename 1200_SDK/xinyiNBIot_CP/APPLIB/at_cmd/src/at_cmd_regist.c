/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "at_com.h"
#include "at_hardware_cmd.h"
#include "at_ps_cmd.h"
#include "at_system_cmd.h"
#include "at_tcpip_cmd.h"
#include "at_test_cmd.h"
#include "at_worklock.h"
#include "at_xy_cmd.h"
#include "cloud_proxy.h"


#if MOBILE_VER
#include "at_onenet.h"
#include "at_lwm2m.h"
#if XY_DM
#include "cmcc_dm.h"
#endif
#endif

#if TELECOM_VER
#include "at_cdp.h"
#endif

#if CTWING_VER
#include "at_ctwing.h"
#endif

#if XY_PING
#include "at_ping.h"
#endif

#if XY_PERF
#include "at_perf.h"
#endif

#if XY_WIRESHARK
#include "at_wireshark.h"
#endif

#if	AT_SOCKET
#include "at_socket_context.h"
#include "at_socket.h"
#endif

#if MQTT
#include "at_mqtt.h"	
#endif

#if LIBCOAP
#include "at_coap.h"
#endif

#include "xy_fota.h"

#if XY_HTTP
#include "at_http.h"	
#endif

#if WEBLOG
#include "web_log.h"
#endif

#if XY_DM
#include "dm_ctl.h"
#endif

const struct at_serv_proc_e at_basic_req[] = {
/*软复位功能放在AP核实现*/
	/*System command*/
	{"NV", at_NV_req},
	{"WORKLOCK", at_WORKLOCK_req},
	{"CPOF", at_CPOF_req},
	{"FASTOFF", at_CPOF_req},
	{"ASSERTCP", at_ASSERT_req},
	{"DUMP", at_DUMP_req},
	{"NUESTATS", at_NUESTATS_req},
	{"MEMSTATS", at_NUESTATS_req},
	{"TEST", at_TEST_req},
	{"CPREGTEST", at_REGTEST_req},
	{"PHY", at_PHY_req},
	{"RF", at_rf_mt},
	{"QRF", at_qrf_mt},
	{"RDNV", at_READNV_req},   // Read RF Calibration NV
	{"RFNV", at_READRFNV_req}, // Read RF Calibration NV
	{"CTZU", at_NITZ_req},
	{"ZCONTLED", at_LED_req},
	{"SWVER", at_CMVER_req},
	{"HVER", at_HVER_req},
	{"QVERTIME", at_QVERTIME_req},
	{"CGATT", at_CGATT_req},
	{"NSET", at_NSET_req},
	{"ZADC", at_ADC_req},

	// 基础平台(主流模组和芯翼都有的指令)
	{"ATI", at_ATI_req},	
	{"ATE", at_ECHOMODE_req},
	{"CGMI", at_CGMI_req},
	{"CGMM", at_CGMM_req},
	{"CGMR", at_CGMR_req},
	{"NITZ", at_NITZ_req},
	{"NPSMR", at_NPSMR_req},
	{"QGMR", at_QGMR_req},
	{"QDNS", at_QDNS_req},
	{"XYRAI", at_XYRAI_req},
	{"CPINFO", at_CPINFO_req},
	{"CMEE", at_CMEE_req},
	{"SGSW", at_SGSW_req},
	{"NGT3412",at_NGT3412_req},
	// SoftSIM
	{"SOFTSIM",at_SOFTSIM_req},
	// switch uicc
	{"SWUICC",at_SWUICC_req},

#if VER_BC25
	{"AT&W", at_ANDW_req},
	{"QPOWD", at_QPOWD_req},
	{"QATWAKEUP", at_QATWAKEUP_req},
	{"CCLK", at_CCLK_BC25_req},
	{"QCCLK", at_QCCLK_req},
	{"QSCLK", at_QSCLK_req},
	{"QNTP", at_QNTP_req},
	{"QIDNSCFG", at_QIDNSCFG_req2},
	{"QIDNSGIP", at_QIDNSGIP_req},
	{"QLEDMODE", at_LED_BC25_req},
	{"QADC", at_ADC_BC25_req},
	{"CBC", at_CBC_BC25_req},
#elif VER_260Y
	{"CCLK", at_CCLK_260Y_req},
	{"QNTP", at_QNTP_req},
	{"QIDNSCFG", at_QIDNSCFG_req2},
	{"QIDNSGIP", at_QIDNSGIP_req},
	{"QCFG", at_QCFG_260Y_req},
	{"QSCLK", at_QSCLK_260Y_req},
	{"QADC", at_ADC_260Y_req},
	{"CBC", at_CBC_260Y_req},
#elif VER_BC95
	{"QSCLK", at_QSCLK_BC95_req},
	{"CCLK", at_CCLK_BC95_req},
	{"QADC", at_ADC_BC95_req},
	{"QCHIPINFO", at_CHIPINFO_BC95_req},
	{"QLEDMODE", at_LED_BC95_req},
	{"QIDNSCFG", at_QIDNSCFG_req},
	{"QRFTESTMODE", at_QRFTESTMODE_req},
#else
	{"CCLK", at_CCLK_req},
	{"STANDBY", at_STANDBY_req},
	{"XDNSCFG", at_XDNSCFG_req},
	{"XDNS", at_XDNS_req},
	{"CMDNS", at_CMDNS_req},
	{"XNTP", at_XNTP_req},
	{"CMNTP", at_CMNTP_req},
	{"VBAT", at_VBAT_req},
#endif

#if XY_FOTA
	{"NFWUPD", at_NFWUPD_req},
	{"FOTACTR", at_FOTACTR_req},
#endif

#if XY_WIRESHARK
	{"WIRESHARK", at_WIRESHARK_req},
#endif
#if XY_PING
	{"NPING", at_NPING_req},
	{"NPINGSTOP", at_NPINGSTOP_req},
#if VER_BC25 || VER_260Y
	{"QPING", at_QPING_req},
#endif 	
#endif
#if XY_PERF
	{"XYPERF", at_XYPERF_req},
#endif
#if WEBLOG
	{"LOGENABLE", at_LOGENABLE_req},
#endif

#if	AT_SOCKET
#if VER_BC25
	{"QICFG", at_QICFG_BC25_req},
	{"QIOPEN", at_QIOPEN_BC25_req},
	{"QICLOSE", at_QICLOSE_BC25_req},
	{"QISTATE", at_QISTATE_BC25_req},
	{"QISEND", at_QISEND_BC25_req},
	{"QIRD", at_QIRD_req},
	{"QISENDEX", at_QISENDEX_req},
	{"QISWTMD", at_QISWTMD_req},
	{"QIGETERROR", at_QIGETERROR_req},
#elif VER_260Y
	{"QICFG", at_QICFG_260Y_req},
	{"QIOPEN", at_QIOPEN_260Y_req},
	{"QICLOSE", at_QICLOSE_260Y_req},
	{"QISTATE", at_QISTATE_260Y_req},
	{"QISEND", at_QISEND_260Y_req},
#elif VER_BC95
	{"SEQUENCE", at_SOCKSEQ_req},
	{"NQSOS", at_SOCKQSOS_BC95_req},
	{"NSOCR", at_SOCKCFG_BC95_req},
	{"NSOST", at_SOCKSENT_BC95_req},
	{"NSOSTF", at_SOCKSENTF_BC95_req},
	{"NSORF", at_SOCKRF_BC95_req},
	{"NSOCL", at_SOCKCLZ_BC95_req},
	{"NSONMI", at_SOCKNMI_req},
	{"NSOCO", at_SOCKCONN_BC95_req},
	{"NSOSD", at_SOCKSEND_BC95_req},
	{"NSOSDEX", at_SOCKSENDEX_BC95_req},
	{"NSOSTEX", at_SOCKSENTEX_BC95_req},
	{"NSOSTFEX", at_SOCKSENTFEX_BC95_req},
	{"NSOSTATUS", at_SOCKSTAT_BC95_req},
#else
	{"SEQUENCE", at_SOCKSEQ_req},
	{"NQSOS", at_SOCKQSOS_req},
	{"NSOCR", at_SOCKCFG_req},
	{"NSOST", at_SOCKSENT_req},
	{"NSOSTF", at_SOCKSENTF_req},
	{"NSORF", at_SOCKRF_req},
	{"NSOCL", at_SOCKCLZ_req},
	{"NSONMI", at_SOCKNMI_req},
	{"NSOCO", at_SOCKCONN_req},
	{"NSOSD", at_SOCKSEND_req},
	{"NSOSDEX", at_SOCKSENDEX_req},
	{"NSOSTEX", at_SOCKSENTEX_req},
	{"NSOSTFEX", at_SOCKSENTFEX_req},
	{"NSOSTATUS", at_SOCKSTAT_req},
#endif 
/* VER_BC25 */	
#endif /* AT_SOCKET */

#if MQTT
#if VER_BC95
	{"QMTCFG", at_QMTCFG_req},
	{"QMTOPEN", at_QMTOPEN_req},
	{"QMTCLOSE", at_QMTCLOSE_req},
	{"QMTCONN", at_QMTCONN_req},
	{"QMTDISC", at_QMTDISC_req},
	{"QMTSUB", at_QMTSUB_req},
	{"QMTUNS", at_QMTUNS_req},
	{"QMTPUB", at_QMTPUB_req},
#else
	{"MQNEW", at_MQNEW_req},
	{"MQCON", at_MQCON_req},
	{"MQDISCON", at_MQDISCON_req},
	{"MQSUB", at_MQSUB_req},
	{"MQUNSUB", at_MQUNSUB_req},
	{"MQPUB", at_MQPUB_req},
	{"MQSTATE", at_MQSTATE_req},
#endif
#endif
#if LIBCOAP
#if VER_BC95
	{"QCOAPSEND", at_QCOAPSEND_req},
	{"QCOAPDATASTATUS", at_QCOAPDATASTATUS_req},
	{"QCOAPADDRES", at_QCOAPADDRES_req},
	{"QCOAPCFG", at_QCOAPCFG_req},
	{"QCOAPCREATE", at_QCOAPCREATE_req},
	{"QCOAPHEAD", at_QCOAPHEAD_req},
	{"QCOAPOPTION", at_COAPOPTION_req},
	{"QCOAPDEL", at_COAPDEL_req},
#else
	{"COAPCREATE", at_COAPCREATE_req},
	{"COAPDEL", at_COAPDEL_req},
	{"COAPHEAD", at_COAPHEAD_req},
	{"COAPOPTION", at_COAPOPTION_req},
	{"COAPSEND", at_COAPSEND_req},
#endif
#endif
#if MOBILE_VER
	//onenet
#if XY_DM
	{"XYDMP", at_XYDMP_req},
#endif
	{"MIPLCONFIG", at_proc_miplconfig_req},
	{"MIPLCREATE", at_onenet_create_req},
	{"MIPLDELETE", at_proc_mipldel_req},
	{"MIPLOPEN", at_proc_miplopen_req},
	{"MIPLADDOBJ", at_proc_mipladdobj_req},
	{"MIPLDELOBJ", at_proc_mipldelobj_req},
	{"MIPLCLOSE", at_proc_miplclose_req},
	{"MIPLNOTIFY", at_proc_miplnotify_req},
	{"MIPLREADRSP", at_proc_miplread_req},
	{"MIPLWRITERSP", at_proc_miplwrite_req},
	{"MIPLEXECUTERSP", at_proc_miplexecute_req},
	{"MIPLOBSERVERSP", at_proc_miplobserve_req},
	{"MIPLVER", at_proc_miplver_req},
	{"MIPLDISCOVERRSP", at_proc_discover_req},
	{"MIPLPARAMETERRSP", at_proc_setparam_req},
	{"MIPLUPDATE", at_proc_update_req},
	{"ONETRMLFT", at_proc_rmlft_req},
#if LWM2M_COMMON_VER
	{"QLACONFIG", at_proc_qlaconfig_req},
	{"QLACFG", at_proc_qlacfg_req},
	{"QLAREG", at_proc_qlareg_req},
	{"QLAUPDATE", at_proc_qlaupdate_req},
	{"QLADEREG", at_proc_qladereg_req},
	{"QLAADDOBJ", at_proc_qlaaddobj_req},
	{"QLADELOBJ", at_proc_qladelobj_req},
	{"QLARDRSP", at_proc_qlardrsp_req},
	{"QLAWRRSP", at_proc_qlawrrsp_req},
	{"QLAEXERSP", at_proc_qlaexersp_req},
	{"QLAOBSRSP", at_proc_qlaobsrsp_req},
	{"QLANOTIFY", at_proc_qlanotify_req},
	{"QLARD", at_proc_qlard_req},
	{"QLASTATUS", at_proc_qlstatus_req},
	{"QLARECOVER", at_proc_qlarecover_req},
#endif
#endif
#if TELECOM_VER
#if XY_DM
#if VER_BC25
	{"QSELFREGISTER", at_QSELFREGISTER_req},
#endif
	{"QSREGENABLE", at_QSREGENABLE_req},
	{"DMCFG",at_XYDMCFG_req},
#endif
	{"NCDP", at_NCDP_req},
	{"NMGS", at_QLWULDATA_req},
	{"NMGSEXT", at_NMGS_EXT_req},
	//	{"NMGSEXT2", at_NMGS_EXT2_req},
	{"NMGR", at_NMGR_req},
	{"NNMI", at_NNMI_req},
	{"NSMI", at_NSMI_req},
	{"NQMGS", at_NQMGS_req},
	{"NQMGR", at_NQMGR_req},
	{"QLWSREGIND", at_QLWSREGIND_req},
	{"CDPUPDATE", at_CDPUPDATE_req},
	{"NMSTATUS", at_NMSTATUS_req},
	{"QLWULDATA", at_QLWULDATA_req},
	{"QLWULDATAEX", at_QLWULDATAEX_req},
	{"QLWULDATASTATUS", at_QLWULDATASTATUS_req},
	{"CDPRMLFT", at_CDPRMLFT_req},
	{"QSECSWT", at_QSECSWT_req},
	{"QSETPSK", at_QSETPSK_req},
	{"QRESETDTLS", at_QRESETDTLS_req},
	{"QDTLSSTAT", at_QDTLSSTAT_req},
	{"QLWFOTAIND", at_QLWFOTAIND_req},
	{"QLWEVTIND", at_QLWEVTIND_req},
	{"QCRITICALDATA", at_QCRITICALDATA_req},
#if !VER_BC25 && !VER_260Y
	{"QREGSWT", at_QREGSWT_req},
	{"QCFG", at_QCFG_req},
#endif

#if VER_BC25 || VER_260Y
	{"NCFG", at_NCFG_req},
	{"NCDPOPEN", at_NCDPOPEN_req},
	{"NCDPCLOSE", at_NCDPCLOSE_req},
#endif

#if VER_BC25
	{"NMUPDATE", at_NMUPDATE_req},
#endif 
#endif
	{"XYCONFIG", at_XYCONFIG_req},
	{"XYSEND", at_XYSEND_req},
	{"XYRECV", at_XYRECV_req},
#if CTWING_VER
	{"CTLWVER", at_CTLWVER_req},
	{"CTLWGETSRVFRMDNS", at_CTLWGETSRVFRMDNS_req},
	{"CTLWSETSERVER", at_CTLWSETSERVER_req},
	{"CTLWSETLT", at_CTLWSETLT_req},
	{"CTLWSETPSK", at_CTLWSETPSK_req},
	{"CTLWSETAUTH", at_CTLWSETAUTH_req},
	{"CTLWSETPCRYPT", at_CTLWSETPCRYPT_req},
	{"CTLWSETMOD", at_CTLWSETMOD_req},
	{"CTLWREG", at_CTLWREG_req},
	{"CTLWUPDATE", at_CTLWUPDATE_req},
	{"CTLWDTLSHS", at_CTLWDTLSHS_req},
	{"CTLWDEREG", at_CTLWDEREG_req},
	{"CTLWGETSTATUS", at_CTLWGETSTATUS_req},
	{"CTLWCFGRST", at_CTLWCFGRST_req},
	{"CTLWSESDATA", at_CTLWSESDATA_req},
	{"CTLWSEND", at_CTLWSEND_req},
	{"CTLWRECV", at_CTLWRECV_req},
	{"CTLWGETRECVDATA", at_CTLWGETRECVDATA_req},
	{"CTLWSETREGMOD", at_CTLWSETREGMOD_req},
#endif
#if XY_HTTP
	{"HTTPCREATE", at_http_create},
	{"HTTPCFG", at_http_cfg},
	{"HTTPHEADER", at_http_header},
	{"HTTPCONTENT", at_http_content},
	{"HTTPSEND", at_http_send},
	{"HTTPCLOSE", at_http_close},
	{"HTTPFOTA", at_http_fota},
#endif
	{"NGSEARFCN", at_ngsearfcn_req},
	{"MGSLEEP", at_mgsleep_req},
	{0, 0} // can not delete!!!
};
struct at_serv_proc_e *g_at_basic_req = at_basic_req;

