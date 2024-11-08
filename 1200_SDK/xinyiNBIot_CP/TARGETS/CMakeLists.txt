file(GLOB_RECURSE C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/xinyiNBSoC/USERAPP/inc"
)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/xinyiNBSoC/USERAPP/src/main.c" FALSE .text=.ram.text .rodata=.flash.rodata)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)