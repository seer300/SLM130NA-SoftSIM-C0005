file(GLOB_RECURSE C_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.S"
)

list(REMOVE_ITEM C_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/croutine.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/event_groups.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/stream_buffer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/dvfs.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/dvfs_measure.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_1.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_2.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_3.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_4.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_5.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/MemMang/heap_6.c"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/newlib/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/portable/GCC/ARM_CM3"
    "${CMAKE_CURRENT_SOURCE_DIR}/OSAL/FreeRTOS/Config"
    "${CMAKE_CURRENT_SOURCE_DIR}/OSAL/FreeRTOS/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/Include"
)

change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}" FALSE .text=.flash.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/OSAL/FreeRTOS/Source/osal_cpu_utilization.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/OSAL/FreeRTOS/Source/osal_statistics.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/OS/FreeRTOS/tasks.c" FALSE .text=.ram.text .rodata=.ram.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/newlib/src/memcpy.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/newlib/src/memset.c" FALSE .text=.ram.text .rodata=.flash.rodata)
change_section_config(${TARGET_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/utils/Source/osAssitantUtils.c" FALSE .text=.ram.text .rodata=.flash.rodata)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)
