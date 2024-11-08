file(GLOB_RECURSE C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/at_ctrl/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/at_uart/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/api"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/core/base"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/core/format"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/core/recv"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/port"
    "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/web_log"
    "${CMAKE_CURRENT_SOURCE_DIR}/flash/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/lowpower/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/rtc_tmr/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/shm_msg/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/smartcard/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/inc"
)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/api" FALSE .text=.ram.text .rodata=.ram.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/core" FALSE .text=.ram.text .rodata=.ram.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/core/recv" FALSE .text=.flash.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/diag_ctrl/port" FALSE .text=.ram.text .rodata=.ram.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/flash/src/flash_adapt.c" FALSE .text=.ram.text .rodata=.ram.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/rtc_tmr/src/rtc_utils.c" FALSE .text=.ram.text .rodata=.flash.rodata)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)