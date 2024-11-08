# Copyright (c) 2024 Onomondo ApS. All rights reserved.
# SPDX-License-Identifier: GPL-3.0-only

add_subdirectory(softsim)

file(GLOB_RECURSE C_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "./*.c"
)

set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
	"../inc"
    "../include"
)

set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)
set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)
