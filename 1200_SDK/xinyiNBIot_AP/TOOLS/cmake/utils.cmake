# Get all subdirectories under ${current_dir} and store them
# in ${result} variable
macro(subdirlist result current_dir)
    file(GLOB children ${current_dir}/*)
    set(dirlist "")

    foreach(child ${children})
        if (IS_DIRECTORY ${child})
            list(APPEND dirlist ${child})
        endif()
    endforeach()

    set(${result} ${dirlist})
endmacro()

macro(get_current_project_path result CMAKE_CURRENT_SOURCE_DIR CONFIG_COMPILE_PROJECT_SELECT)
	file(GLOB_RECURSE ALL_ITEMS LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
	foreach(ITEM ${ALL_ITEMS})
		if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ITEM})
			get_filename_component(FOLDER_NAME ${CMAKE_CURRENT_SOURCE_DIR}/${ITEM} NAME)
			if (${FOLDER_NAME} STREQUAL ${CONFIG_COMPILE_PROJECT_SELECT})
	            set(${result} ${CMAKE_CURRENT_SOURCE_DIR}/${ITEM})
	            break()
	        endif()
		endif()
    endforeach()
endmacro()

macro(get_sub_header_dirs result)
    set(dirlist "")
    foreach(current_dir ${ARGN})
	    file(GLOB_RECURSE children ${current_dir}/*.h)

	    foreach(child ${children})
	        get_filename_component(PARENT_DIR ${child} DIRECTORY)
	        list(FIND dirlist ${PARENT_DIR} found)
	        if (${found} STREQUAL -1)
	            list(APPEND dirlist ${PARENT_DIR})
	        endif()
    	endforeach()
    endforeach()

    set(${result} ${dirlist})
endmacro()

# Prepend ${CMAKE_CURRENT_SOURCE_DIR} to a ${directory} name
# and save it in PARENT_SCOPE ${variable}
macro(prepend_cur_dir variable directory)
    set(${variable} ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
endmacro()

macro(add_subdirectory_if_exist dir)
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
            add_subdirectory(${dir})
        endif()
    endif()
endmacro()

function(clear_section_config target)
    set(orginal_section_config_file ${IGNORE_DIR}/.original_compile_options.${target})
    file(REMOVE ${orginal_section_config_file})
    file(TOUCH ${orginal_section_config_file})
endfunction()

function(change_section_config target)
    set(orginal_section_config_file ${IGNORE_DIR}/.original_compile_options.${target})
    list(JOIN ARGN "," single_section_config_string)
    set(single_section_config_string ${single_section_config_string} "\n")
    file(APPEND ${orginal_section_config_file} ${single_section_config_string})
endfunction()

function(handle_section_config target)
    set(orginal_section_config_file ${IGNORE_DIR}/.original_compile_options.${target})
    set(dst_section_config_file ${IGNORE_DIR}/.compile_options.${target})
    get_target_property(current_target_source_dir ${target} SOURCE_DIR)
    get_target_property(current_target_binary_dir ${target} BINARY_DIR)
    if (DEFINED PYTHON)
        add_custom_command(
            TARGET ${target}
            PRE_LINK
            COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/convert_section_config.py ${orginal_section_config_file} ${dst_section_config_file}
            COMMENT "convert section config for ${target}..."
        )
        add_custom_command(
            TARGET ${target}
            PRE_LINK
            COMMAND ${PYTHON_EXE} ${PROJECT_SOURCE_DIR}/TOOLS/build/rename_sections.py ${current_target_source_dir} ${current_target_binary_dir}/CMakeFiles/${target}.dir ${dst_section_config_file} # CMakeFiles/${target}.dir, ugly, haven't found a better way
            COMMENT "rename sections for ${target}..."
        )
    else ()
        add_custom_command(
            TARGET ${target}
            PRE_LINK
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/convert_section_config.exe ${orginal_section_config_file} ${dst_section_config_file}
            COMMENT "convert section config for ${target}..."
        )
        add_custom_command(
            TARGET ${target}
            PRE_LINK
            COMMAND ${PROJECT_SOURCE_DIR}/TOOLS/build/rename_sections.exe ${current_target_source_dir} ${current_target_binary_dir}/CMakeFiles/${target}.dir ${dst_section_config_file} # CMakeFiles/${target}.dir, ugly, haven't found a better way
            COMMENT "rename sections for ${target}..."
        )
    endif ()
endfunction()

function(shorten_src_file_macro)
    foreach(file ${ARGN})
        get_filename_component(PARENT_DIR ${file} DIRECTORY)
        set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "-fmacro-prefix-map=${PARENT_DIR}/=")
    endforeach()
endfunction()

# import_kconfig(<prefix> <kconfig_fragment> [<keys>])
#
# Parse a KConfig fragment (typically with extension .config) and
# introduce all the symbols that are prefixed with 'prefix' into the
# CMake namespace. List all created variable names in the 'keys'
# output variable if present.
function(import_kconfig prefix kconfig_fragment)
  # Parse the lines prefixed with 'prefix' in ${kconfig_fragment}
  file(
    STRINGS
    ${kconfig_fragment}
    DOT_CONFIG_LIST
    REGEX "^${prefix}"
    ENCODING "UTF-8"
  )

  foreach (CONFIG ${DOT_CONFIG_LIST})
    # CONFIG could look like: CONFIG_NET_BUF=y

    # Match the first part, the variable name
    string(REGEX MATCH "[^=]+" CONF_VARIABLE_NAME ${CONFIG})

    # Match the second part, variable value
    string(REGEX MATCH "=(.+$)" CONF_VARIABLE_VALUE ${CONFIG})
    # The variable name match we just did included the '=' symbol. To just get the
    # part on the RHS we use match group 1

    set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

    if("${CONF_VARIABLE_VALUE}" MATCHES "^\"(.*)\"$") # Is surrounded by quotes

        set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

    endif()

    set("${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)

    list(APPEND keys "${CONF_VARIABLE_NAME}")

  endforeach()

  foreach(outvar ${ARGN})
    set(${outvar} "${keys}" PARENT_SCOPE)
  endforeach()
endfunction()

# Add custom command to print firmware size in Berkley format
function(firmware_size target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_SIZE_UTIL} -B
        "${CMAKE_CURRENT_BINARY_DIR}/${target}${CMAKE_EXECUTABLE_SUFFIX}"
        COMMENT "Calculating size..."
    )
endfunction()

# Add a command to generate firmare in a provided format
function(generate_object target output_dir output type sections)
    set(output_sections -j ${sections})
    foreach(section ${ARGN})
        set(output_sections ${output_sections} -j ${section})
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ${type} ${output_sections}
        "${CMAKE_CURRENT_BINARY_DIR}/${target}${CMAKE_EXECUTABLE_SUFFIX}" "${output_dir}/${output}"
        COMMENT "Creating ${output}..."
    )
endfunction()

# Add custom linker script to the linker flags
function(linker_script_add path_to_script)
    string(APPEND CMAKE_EXE_PRJ_LINK_OPTIONS_LIST " -T ${path_to_script}")
endfunction()

# Update a target LINK_DEPENDS property with a custom linker script.
# That allows to rebuild that target if the linker script gets changed
function(linker_script_target_dependency target path_to_script)
    get_target_property(_cur_link_deps ${target} LINK_DEPENDS)
    string(APPEND _cur_link_deps " ${path_to_script}")
    set_target_properties(${target} PROPERTIES LINK_DEPENDS ${_cur_link_deps})
endfunction()
