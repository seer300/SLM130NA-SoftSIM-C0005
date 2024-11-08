add_subdirectory(at_cmd)
add_subdirectory(cJSON)
add_subdirectory(cloud)
add_subdirectory(dm)
add_subdirectory(Dtls)
add_subdirectory(fs)
add_subdirectory(http)
add_subdirectory(libcoap)
add_subdirectory(lwip)
# add_subdirectory(mqtt) 		# Disabled during SoftSIM deevelopment
add_subdirectory(netled)
add_subdirectory(perf)
add_subdirectory(ping) 		# Disabled during SoftSIM deevelopment
add_subdirectory(socket)
# add_subdirectory(wakaama) 	# Disabled during SoftSIM deevelopment
# add_subdirectory(wireshark) 	# Disabled during SoftSIM deevelopment
add_subdirectory(fota)
add_subdirectory(onomondo)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)
set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)