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
        "-Wall"
        "-Wextra"
        "-g"
        "-nostartfiles")
if(${CONFIG_DYN_LOAD_SELECT} EQUAL 1)
    list(APPEND PRJ_COMPILE_OPTIONS_LIST
        "-ffixed-r9"
    )
endif()

#链接配置
if(${XY_SOC_VER} EQUAL 0)      #1200
  set(LINK_FILE   -Tmem_2m.ld -Tsections.ld)
elseif(${XY_SOC_VER} EQUAL 1)  #1200S
  set(LINK_FILE   -Tmem_2m.ld -Tsections.ld)
elseif(${XY_SOC_VER} EQUAL 2)  #1200S
  set(LINK_FILE   -Tmem_2m.ld -Tsections.ld)
elseif(${XY_SOC_VER} EQUAL 3)  #2100S
  set(LINK_FILE   -Tmem_4m.ld -Tsections.ld)
elseif(${XY_SOC_VER} EQUAL 4)  #2100S
  set(LINK_FILE   -Tmem_4m.ld -Tsections.ld)
elseif(${XY_SOC_VER} EQUAL 5)  #1200S
  set(LINK_FILE   -Tmem_2m.ld -Tsections.ld)
endif()

set(LINK_DIR "-L${PROJECT_SOURCE_DIR}/TOOLS/linkscript")
set(MAP -Wl,-Map=${OUTPUT_ELF_DIR}/${TARGET_NAME}.map)
set(LINK_OPT -Xlinker --gc-sections ${LINK_DIR} ${MAP})
set(GNULIB_NANO --specs=nano.specs)
set(GNULIB_SEMIHOST --specs=rdimon.specs)
set(GNULIB_NOHOST --specs=nosys.specs)
set(USE_LIB ${GNULIB_NANO} ${GNULIB_NOHOST})

list(APPEND PRJ_LINK_OPTIONS_LIST ${LINK_FILE} ${LINK_OPT} ${USE_LIB})