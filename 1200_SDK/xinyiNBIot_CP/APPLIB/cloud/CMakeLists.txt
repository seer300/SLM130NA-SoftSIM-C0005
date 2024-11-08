if(${VER_BC25})
	file(GLOB_RECURSE C_SOURCES 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/*.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/*.s"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/*.S"
	)

	set(INCLUDE_DIRS 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/atiny_lwm2m"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/log"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/lwm2m_core"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/lwm2m_core/er-coap-13"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/net"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/osdepends"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/AgentTiny/ota/fota"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc25/xy_cdp/inc"
	)
elseif(${VER_260Y})
	file(GLOB_RECURSE C_SOURCES 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/*.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/*.s"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/*.S"
	)

	set(INCLUDE_DIRS 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/atiny_lwm2m"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/log"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/lwm2m_core"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/lwm2m_core/er-coap-13"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/net"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/osdepends"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/AgentTiny/ota/fota"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp_bc260/xy_cdp/inc"
	)

else()
	file(GLOB_RECURSE C_SOURCES 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/*.c"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/*.s"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/*.S"
	)

	set(INCLUDE_DIRS 
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/atiny_lwm2m"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/log"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/lwm2m_core"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/lwm2m_core/er-coap-13"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/net"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/osdepends"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/AgentTiny/ota/fota"
		"${CMAKE_CURRENT_SOURCE_DIR}/cdp/xy_cdp/inc"
	)


endif()

file(GLOB_RECURSE GENERAL_C_SOURCES
	// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/*.c"
	// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/*.s"
	// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/*.S"
	// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/*.c"
	// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/*.s"
	// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/*.S"
	// "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c"
	// "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.s"
	// "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.S"
)

set(C_SOURCES ${C_SOURCES} ${GENERAL_C_SOURCES})

set(INCLUDE_DIRS ${INCLUDE_DIRS}
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/apps"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/core"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/core/er-coap-13"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client/common"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client/ctlw_mbedtls_interface"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client/fota"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client/object19"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/ctlw_client/objects"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/ctiot_aep_coap/src/port"
		// "${CMAKE_CURRENT_SOURCE_DIR}/ctwing/xy_ctwing/inc"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/ciscore"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/ciscore/cis_er-coap-13"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/ciscore/dm_utils"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/ciscore/dtls"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/ciscore/std_object"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/DM/inc"
		// "${CMAKE_CURRENT_SOURCE_DIR}/onenet/xy_onenet/inc"
		// "${CMAKE_CURRENT_SOURCE_DIR}/utils/inc"
	)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)
set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)
