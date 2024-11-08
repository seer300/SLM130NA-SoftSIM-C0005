#编译配置
list(APPEND PRJ_COMPILE_OPTIONS_LIST
        "-mthumb"
        "-mcpu=${CORE}"
        "-O2"
        "-Os"
        "-fmessage-length=0"
        "-fsigned-char"
        "-ffunction-sections"
        "-fdata-sections"
        "-ffreestanding"
        "-fcommon"
        "-w"
        "-Wall"
        "-Wextra"
        "-g3"
        "-Werror=implicit-function-declaration"
        "-Werror=implicit-int"
        "-Werror=int-conversion"
        "-Werror=sign-compare"
        "-Werror=uninitialized"
        "-Werror=return-type"
        )

#链接配置
if(${CONFIG_FLASH_GIGA_2M})
    set(LINK_FILE -Tmem_2m.ld -Tsections.ld)
else()
    set(LINK_FILE -Tmem_4m.ld -Tsections.ld)
endif()

set(LINK_DIR "-L${PROJECT_SOURCE_DIR}/TOOLS/linkscript")
set(MAP -Wl,-Map=${OUTPUT_ELF_DIR}/${TARGET_NAME}.map)
set(LINK_OPT -Xlinker --gc-sections ${LINK_DIR} ${MAP} -u _printf_float -nostartfiles)
set(GNULIB_NANO --specs=nano.specs)
set(GNULIB_SEMIHOST --specs=rdimon.specs)
set(GNULIB_NOHOST --specs=nosys.specs)
set(USE_LIB ${GNULIB_NANO} ${GNULIB_NOHOST})

list(APPEND PRJ_LINK_OPTIONS_LIST ${LINK_FILE} ${LINK_OPT} ${USE_LIB})