set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/driverlib/inc"
)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)