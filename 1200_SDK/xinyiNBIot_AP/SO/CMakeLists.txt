file(GLOB_RECURSE C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

get_sub_header_dirs(INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

set(BASE_INCLUDE_DIRS ${BASE_INCLUDE_DIRS} ${INCLUDE_DIRS} PARENT_SCOPE)

if(${CONFIG_DYN_LOAD_SELECT} EQUAL 0)
    set(PRJ_SRC_LIST ${PRJ_SRC_LIST} ${C_SOURCES} PARENT_SCOPE)
else()

    set(LIBRARY_NAME dynload)
    set(SO_LIBRARY_NAME ${LIBRARY_NAME} PARENT_SCOPE)
    set(LIBRARIES ${LIBRARIES} ${LIBRARY_NAME} PARENT_SCOPE)
    add_library(${LIBRARY_NAME} SHARED)
    shorten_src_file_macro(${C_SOURCES})
    target_sources(${LIBRARY_NAME} PRIVATE ${C_SOURCES})
    list(APPEND LIBRARY_COMPILE_OPTIONS_LIST ${PRJ_COMPILE_OPTIONS_LIST})
    list(APPEND PRJ_DEF_LIST_OS "__RAM_FUNC=")
    list(APPEND PRJ_DEF_LIST_OS "__FLASH_FUNC=")
    target_compile_definitions(${LIBRARY_NAME} PRIVATE ${PRJ_DEF_LIST_OS})
    list(REMOVE_ITEM LIBRARY_COMPILE_OPTIONS_LIST "-ffixed-r9")
    list(APPEND LIBRARY_COMPILE_OPTIONS_LIST 
        "-msingle-pic-base"
        "-fpic"
        "-mpic-register=r9"
        "-mno-pic-data-is-text-relative"
    )
    target_compile_options(${LIBRARY_NAME} PRIVATE ${LIBRARY_COMPILE_OPTIONS_LIST})
    # file(MAKE_DIRECTORY ${LIB_DIR})
    # set_property(TARGET ${LIBRARY_NAME} APPEND PROPERTY LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
    set(LINK_FILE -Tso.ld)
    set(LINK_DIR "-L${PROJECT_SOURCE_DIR}/TOOLS/linkscript")
    set(MAP -Wl,-Map=${OUTPUT_ELF_DIR}/${LIBRARY_NAME}.map)
    set(LINK_OPT ${LINK_DIR} ${MAP})

    list(APPEND LIBRARY_LINK_OPTIONS_LIST ${LINK_FILE} ${LINK_OPT} -nostartfiles -mpic-register=r9)
    target_link_options(${LIBRARY_NAME} PRIVATE ${LIBRARY_COMPILE_OPTIONS_LIST} ${LIBRARY_LINK_OPTIONS_LIST})

    set(SO_TEXT_BIN_PATH ${IGNORE_DIR}/so_text.bin PARENT_SCOPE)
    add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary -j .text -j .plt $<TARGET_FILE:${LIBRARY_NAME}> ${IGNORE_DIR}/so_text.bin
        COMMENT "Creating so_text.bin..."
    )

    add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP_UTIL} -S $<TARGET_FILE:${LIBRARY_NAME}> > ${OUTPUT_ELF_DIR}/${LIBRARY_NAME}.asm
        COMMENT "Creating ${LIBRARY_NAME} asm..."
    )
endif()