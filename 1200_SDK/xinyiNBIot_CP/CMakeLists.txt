cmake_minimum_required(VERSION 3.20.6)
#规避编译器检查错误
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(XY1200 C ASM CXX)

include(${PROJECT_SOURCE_DIR}/TOOLS/cmake/utils.cmake)

set(IGNORE_DIR ${PROJECT_SOURCE_DIR}/.ignore)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/LIB)
set(OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(OUTPUT_ELF_DIR ${CMAKE_CURRENT_BINARY_DIR}/elf)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(TARGET_NAME xinyiNBSoC)
set(CORE cortex-m3)

file(MAKE_DIRECTORY ${IGNORE_DIR})
file(MAKE_DIRECTORY ${OUTPUT_BIN_DIR} ${OUTPUT_ELF_DIR})

if (DEFINED PYTHON)
    if (CMAKE_HOST_WIN32)
        if (${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
            execute_process(COMMAND cygpath.exe ${PYTHON} OUTPUT_VARIABLE PYTHON_EXE OUTPUT_STRIP_TRAILING_WHITESPACE)
        else ()
            set(PYTHON_EXE ${PYTHON})
        endif ()
    else ()
        set(PYTHON_EXE ${PYTHON})
    endif ()
endif ()

add_executable(${TARGET_NAME})

include(${PROJECT_SOURCE_DIR}/../define.cmake)
include(${PROJECT_SOURCE_DIR}/TOOLS/cmake/config.cmake)

clear_section_config(${TARGET_NAME})

add_subdirectory(ARCH)
add_subdirectory(DRIVERS)
add_subdirectory(KERNEL)
add_subdirectory(NBIotPs)
add_subdirectory(SYSAPP)
add_subdirectory(APPLIB)
add_subdirectory(TARGETS)

handle_section_config(${TARGET_NAME})

set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINKER_LANGUAGE "CXX")

# 设置target属性
target_include_directories(${TARGET_NAME} PUBLIC ${BASE_INCLUDE_DIRS})
target_compile_definitions(${TARGET_NAME} PRIVATE ${PRJ_DEF_LIST})

target_compile_options(${TARGET_NAME}
    PRIVATE ${PRJ_COMPILE_OPTIONS_LIST})

shorten_src_file_macro(${PRJ_SRC_LIST})
target_sources(${TARGET_NAME} PRIVATE ${PRJ_SRC_LIST})

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LIB_DIR})

target_link_options(${TARGET_NAME} PRIVATE ${PRJ_COMPILE_OPTIONS_LIST} ${PRJ_LINK_OPTIONS_LIST})

file(GLOB addtional_libs "${LIB_DIR}/*.a")

foreach(lib ${LIBRARIES})
    target_include_directories(${lib} PUBLIC ${BASE_INCLUDE_DIRS})
    target_compile_definitions(${lib} PRIVATE ${PRJ_DEF_LIST})
    get_target_property(lib_output_directory ${lib} ARCHIVE_OUTPUT_DIRECTORY)
    list(REMOVE_ITEM addtional_libs "${lib_output_directory}/lib${lib}.a")
endforeach()

file(GLOB addtional_xy_soc_ver_libs "${LIB_DIR}/XY_SOC_VER=${XY_SOC_VER}/*.a")

target_link_libraries(${TARGET_NAME}
    -Wl,--start-group
    ${LIBRARIES}
    ${addtional_libs}
    ${addtional_xy_soc_ver_libs}
    -Wl,--end-group)

set(base_loginfo ${PROJECT_SOURCE_DIR}/loginfo/XY_SOC_VER=${XY_SOC_VER}/loginfo.info)

set(log_match_func PrintLog PhyPrintf0 PhyPrintf1 PhyPrintf2 PhyPrintf3 PhyPrintf4 PhyPrintf5 PhyPrintf6 PhyPrintf7 PhyPrintf8 PhyPrintf9 PhyPrintf10 PhyPrintf11 PhyPrintf12 PhyPrintf13 PhyPrintf14 PhyPrintf15 PhyPrintf16 xy_printf)

if (DEFINED PYTHON)
    add_custom_target(
        pre_build_target
        COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.py -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m ${log_match_func} -rp -v ${XY_SOFTWARE_VERSION}
        COMMENT "Repalce log string and refresh log id..."
    )
else ()
    add_custom_target(
        pre_build_target
        COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.exe -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m ${log_match_func} -rp  -v ${XY_SOFTWARE_VERSION}
        COMMENT "Repalce log string and refresh log id..."
    )
endif ()

add_dependencies(${TARGET_NAME} pre_build_target)

if (DEFINED PYTHON)
    add_custom_target(
        restore_log_string
        COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.py -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m ${log_match_func} -rs -z
        COMMENT "Replace log string and refresh log id..."
    )
else ()
    add_custom_target(
        restore_log_string
        COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.exe -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m ${log_match_func} -rs -z
        COMMENT "Replace log string and refresh log id..."
    )
endif ()

# if (DEFINED PYTHON)
#     add_custom_target(
#         reset_log_id
#         COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.py -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m  ${log_match_func} -z
#         COMMENT "Reset log id..."    
#     )
# else ()
#     add_custom_target(
#         reset_log_id
#         COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/loginfo.exe -p ${PROJECT_SOURCE_DIR} -i ${CMAKE_BINARY_DIR}/loginfo.info -b ${base_loginfo} -m  ${log_match_func} -z
#         COMMENT "Reset log id..."    
#     )
# endif ()

file(GENERATE OUTPUT ${IGNORE_DIR}/${TARGET_NAME}.objectfiles CONTENT "$<JOIN:$<TARGET_OBJECTS:${TARGET_NAME}>,\n>")

#从elf中获取部分段形成bin文件
set(flash_bin_sections .flash.text .mt.text .mt.data)

generate_object(${TARGET_NAME} ${OUTPUT_BIN_DIR} cp_flash.bin binary ${flash_bin_sections})

set(ram_bin_sections .isr_vector .text .data)

generate_object(${TARGET_NAME} ${OUTPUT_BIN_DIR} cp_ram.bin binary ${ram_bin_sections})

#生成elf相关信息文件
add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_SIZE_UTIL} @${IGNORE_DIR}/${TARGET_NAME}.objectfiles > ${OUTPUT_ELF_DIR}/${TARGET_NAME}.siz
    COMMENT "Creating siz..."
)
add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJDUMP_UTIL} -S $<TARGET_FILE:${TARGET_NAME}> > ${OUTPUT_ELF_DIR}/${TARGET_NAME}.asm
    COMMENT "Creating asm..."
)

add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_READELF_UTIL} -a $<TARGET_FILE:${TARGET_NAME}> > ${OUTPUT_ELF_DIR}/${TARGET_NAME}.dumpelf
    COMMENT "Creating dumpelf..."
)

add_custom_command(
    TARGET ${TARGET_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_SIZE_UTIL} -B $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Calculating size..."
)

add_custom_command(
    TARGET ${TARGET_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_NAME}> ${OUTPUT_ELF_DIR}
)

#生成img文件
if(${CONFIG_FLASH_GIGA_2M})
    set(IMG_GEN_CONFIG_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/dspld_2m.ini)
else()
    set(IMG_GEN_CONFIG_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/dspld_4m.ini)
endif()
if (DEFINED PYTHON)
    set(IMG_GEN_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/xy_packer.py)
    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${PYTHON_EXE} ${IMG_GEN_FILE} ${OUTPUT_BIN_DIR} ${IMG_GEN_CONFIG_FILE}
    )
else ()
    set(IMG_GEN_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/xy_packer.exe)
    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${IMG_GEN_FILE} ${OUTPUT_BIN_DIR} ${IMG_GEN_CONFIG_FILE}
    )
endif ()