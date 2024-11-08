file(GLOB_RECURSE C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.s"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/assert/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/core_cm3/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/nvic/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/startup/inc"
)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/assert/src/dump.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/core_cm3/src/HardFault.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/core_cm3/src/MPU.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/startup/src/startup_xinyi_2100.s" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/startup/src/system_cm3.c" FALSE .text=.ram.text .rodata=.flash.rodata)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)