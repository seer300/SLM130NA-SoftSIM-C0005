cmake_minimum_required(VERSION 3.20.6)
#规避编译器检查错误
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(XY1200 C ASM)

include(${PROJECT_SOURCE_DIR}/TOOLS/cmake/utils.cmake)

set(IGNORE_DIR ${PROJECT_SOURCE_DIR}/.ignore)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/LIB)
set(OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(OUTPUT_ELF_DIR ${CMAKE_CURRENT_BINARY_DIR}/elf)

set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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


include(${PROJECT_SOURCE_DIR}/../define.cmake)
set(TARGET_NAME xinyiNBSoC)

add_executable(${TARGET_NAME})

include(${PROJECT_SOURCE_DIR}/TOOLS/cmake/config.cmake)

clear_section_config(${TARGET_NAME})

add_subdirectory_if_exist(DRIVERS)
add_subdirectory_if_exist(EXT)
add_subdirectory_if_exist(MIDDLEWARE)
add_subdirectory_if_exist(PROJECT)
add_subdirectory_if_exist(SYSAPP)
if(NOT ${CONFIG_DYN_LOAD_SELECT} EQUAL 0)
add_subdirectory(SO)
endif()

###########################################################
#代码段放置FLASH还是SRAM的配置
###########################################################
foreach(def ${PRJ_DEF_LIST})
    if(${def} MATCHES "BAN_WRITE_FLASH")
    string(REGEX MATCH "[0-9]+" BAN_WRITE_FLASH ${def})
    endif()
endforeach()

if(${BAN_WRITE_FLASH} EQUAL 1)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/EXT/" TRUE .text=.flash.text .rodata=.flash.rodata)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/" TRUE .text=.flash.text .rodata=.flash.rodata)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/HAL/src/hal_lpuart.c" FALSE)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/HAL/src/hal_i2c.c" FALSE)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/PROJECT/" TRUE .text=.flash.text .rodata=.flash.rodata)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/SYSAPP/" TRUE  .rodata=.flash.rodata)
else()
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/EXT/" TRUE .text=.flash.text)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/" TRUE .text=.flash.text)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/HAL/src/hal_lpuart.c" FALSE)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/MIDDLEWARE/HAL/src/hal_i2c.c" FALSE)
    change_section_config(${TARGET_NAME} "${PROJECT_SOURCE_DIR}/PROJECT/" TRUE .text=.flash.text)
endif()

handle_section_config(${TARGET_NAME})

list(JOIN PRJ_DEF_LIST "\nDEFINES += -D" menuconfig_define)
file(GENERATE OUTPUT ${IGNORE_DIR}/meunconfig_define.mk CONTENT "DEFINES += -D${menuconfig_define}")

# 设置target属性
target_include_directories(${TARGET_NAME} PUBLIC ${BASE_INCLUDE_DIRS})
target_compile_definitions(${TARGET_NAME} PRIVATE ${PRJ_DEF_LIST})
target_compile_options(${TARGET_NAME}
    PRIVATE ${PRJ_COMPILE_OPTIONS_LIST})

cmake_path(GET PROJECT_SOURCE_DIR FILENAME project_base_name)
if (DEFINED CONFIG_${project_base_name}_SOURCES)
    list(APPEND PRJ_SRC_LIST ${CONFIG_${project_base_name}_SOURCES})
endif ()
shorten_src_file_macro(${PRJ_SRC_LIST})

target_sources(${TARGET_NAME} PRIVATE ${PRJ_SRC_LIST})

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LIB_DIR})


file(GLOB_RECURSE addtional_libs "${LIB_DIR}/*.a")

foreach(lib ${LIBRARIES})
    target_include_directories(${lib} PUBLIC ${BASE_INCLUDE_DIRS})
    target_compile_definitions(${lib} PRIVATE ${PRJ_DEF_LIST})
    get_target_property(lib_output_directory ${lib} ARCHIVE_OUTPUT_DIRECTORY)
    list(REMOVE_ITEM addtional_libs "${lib_output_directory}/lib${lib}.a")
endforeach()

target_link_libraries(${TARGET_NAME}
    -Wl,--start-group
    ${LIBRARIES}
    ${addtional_libs}
    -Wl,--end-group)

add_custom_target(
    generate_timestamp
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/TOOLS/cmake/timestamp.cmake
)

target_link_options(${TARGET_NAME} PRIVATE ${PRJ_COMPILE_OPTIONS_LIST} ${PRJ_LINK_OPTIONS_LIST})

file(GENERATE OUTPUT ${IGNORE_DIR}/${TARGET_NAME}.objectfiles CONTENT "$<JOIN:$<TARGET_OBJECTS:${TARGET_NAME}>,\n>")

if (${SAVE_OTHER_INFO})
    get_target_property(c_defines_list ${TARGET_NAME} COMPILE_DEFINITIONS)
    list(JOIN c_defines_list "\n" c_defines)
    file(GENERATE OUTPUT ${IGNORE_DIR}/c_defines.txt CONTENT "${c_defines}")

    get_target_property(include_directories_list ${TARGET_NAME} INCLUDE_DIRECTORIES)
    list(JOIN include_directories_list "\n" include_directories)
    file(GENERATE OUTPUT ${IGNORE_DIR}/include_directories.txt CONTENT "${include_directories}")

    get_target_property(sources_list ${TARGET_NAME} SOURCES)
    list(JOIN sources_list "\n" sources)
    file(GENERATE OUTPUT ${IGNORE_DIR}/sources.txt CONTENT "${sources}")

    get_target_property(link_options_list ${TARGET_NAME} LINK_OPTIONS)
    list(JOIN link_options_list "\n" link_options)
    file(GENERATE OUTPUT ${IGNORE_DIR}/link_options.txt CONTENT "${link_options}")
endif ()

#从elf中获取部分段形成bin文件
set(flash_bin_sections .flash.text .boot.text .data )
generate_object(${TARGET_NAME} ${OUTPUT_BIN_DIR} flash.bin binary ${flash_bin_sections})

set(ram_bin_sections .isr_vector .text .plt .got.plt)
generate_object(${TARGET_NAME} ${OUTPUT_BIN_DIR} ram.bin binary ${ram_bin_sections})

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

if(NOT ${CONFIG_DYN_LOAD_SELECT} EQUAL 0)
    if(${CONFIG_FLASH_GIGA_2M})
        set(FLASH_BIN_EXEC_ADDR 0x30174000)
    else()
        set(FLASH_BIN_EXEC_ADDR 0x30217000)
    endif()

    # so running on cpram
    if (DEFINED PYTHON)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/so.py $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${SO_LIBRARY_NAME}> 0x0101EF80 0x60000000 0x10000 ${BAN_WRITE_FLASH} 5 ${SO_TEXT_BIN_PATH} ${OUTPUT_BIN_DIR}/flash.bin ${FLASH_BIN_EXEC_ADDR} ${OUTPUT_BIN_DIR}/ram.bin 0x01000000 > ${IGNORE_DIR}/so_log.txt
            COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/find_so_unused.py $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${SO_LIBRARY_NAME}> > ${IGNORE_DIR}/so_unused.txt
        )
    else ()
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/so.exe $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${SO_LIBRARY_NAME}> 0x0101EF80 0x60000000 0x10000 ${BAN_WRITE_FLASH} 5 ${SO_TEXT_BIN_PATH} ${OUTPUT_BIN_DIR}/flash.bin ${FLASH_BIN_EXEC_ADDR} ${OUTPUT_BIN_DIR}/ram.bin 0x01000000 > ${IGNORE_DIR}/so_log.txt
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/find_so_unused.exe $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${SO_LIBRARY_NAME}> > ${IGNORE_DIR}/so_unused.txt
        )
    endif ()
endif()
foreach(def ${PRJ_DEF_LIST})
    if(${def} MATCHES "CP_USED_APRAM")
        string(REGEX MATCH "[0-9]+" CP_USED_APRAM ${def})
    endif()
endforeach()

if(NOT ${CONFIG_DYN_LOAD_SELECT} EQUAL 0)
    # so running on cpram
    if (DEFINED PYTHON)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
	    COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/heapsize.py $<TARGET_FILE:${TARGET_NAME}> ${CP_USED_APRAM} $<TARGET_FILE:${SO_LIBRARY_NAME}>  ${IGNORE_DIR}/so_log.txt
        )
    else ()
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/heapsize.exe $<TARGET_FILE:${TARGET_NAME}> ${CP_USED_APRAM} $<TARGET_FILE:${SO_LIBRARY_NAME}> ${IGNORE_DIR}/so_log.txt
        )
    endif ()
else()
    # so running on cpram
    if (DEFINED PYTHON)
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/heapsize.py $<TARGET_FILE:${TARGET_NAME}> ${CP_USED_APRAM}
        )
    else ()
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/heapsize.exe $<TARGET_FILE:${TARGET_NAME}> ${CP_USED_APRAM}
        )
    endif ()
endif()
#生成img文件
if(${CONFIG_FLASH_GIGA_2M})
    set(IMG_GEN_CONFIG_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/m3ld_2m.ini)
else()
    set(IMG_GEN_CONFIG_FILE ${PROJECT_SOURCE_DIR}/TOOLS/build/m3ld_4m.ini)
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
