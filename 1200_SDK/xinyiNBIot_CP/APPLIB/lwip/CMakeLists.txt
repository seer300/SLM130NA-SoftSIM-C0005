
add_subdirectory(xy_adapter)

file(GLOB_RECURSE C_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.s"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.S"
)

list(REMOVE_ITEM C_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/bridgeif.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/bridgeif_fdb.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/lowpan6.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/lowpan6_ble.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/lowpan6_common.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/slipif.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/netif/zepif.c"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip_config"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip_port"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip_port/arch"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip_port/os"
    "${CMAKE_CURRENT_SOURCE_DIR}/lwip-2.1.3/include"
)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)
set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)

