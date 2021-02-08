#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

function(get_device_list var device_type platform)
    get_filename_component(platsupport_tools ${PLATSUPPORT_HELPERS} DIRECTORY)
    find_file(
        ${platform}_${device_type}_LIST "${platform}.yaml"
        PATHS "${platsupport_tools}/device_lists/${device_type}"
        CMAKE_FIND_ROOT_PATH_BOTH
    )
    set(${var} ${${platform}_${device_type}_LIST} PARENT_SCOPE)
endfunction()

# List of helper CMake functions for libplatsupport
function(gen_device_header device_type platform)
    get_filename_component(platsupport_tools ${PLATSUPPORT_HELPERS} DIRECTORY)
    get_device_list(dev_list_file ${device_type} ${platform})
    if("${dev_list_file}" STREQUAL "${platform}_${device_type}_LIST-NOTFOUND")
        # Exit silently...
        return()
    endif()
    set(header_dir "${CMAKE_CURRENT_BINARY_DIR}/${device_type}/platsupport")
    set(header_file "${header_dir}/${device_type}_list.h")
    set(DEVICE_GEN_PATH "${platsupport_tools}/device_header_gen.py")
    set(dev_header_deps ${DEVICE_GEN_PATH} ${dev_list_file})
    check_outfile_stale(
        regen
        ${header_file}
        dev_header_deps
        "${CMAKE_CURRENT_BINARY_DIR}/${device_type}.cmd"
    )
    if(regen)
        file(MAKE_DIRECTORY ${header_dir})
        execute_process(
            COMMAND
                ${PYTHON3} "${DEVICE_GEN_PATH}" --device-list "${dev_list_file}" --device-type
                "${device_type}" --header-out "${header_file}"
            INPUT_FILE /dev/stdin
            OUTPUT_FILE /dev/stdout
            ERROR_FILE /dev/stderr
            RESULT_VARIABLE error
        )
        if(error)
            message(FATAL_ERROR "Failed to gen GPIO header: ${header_file}")
        endif()
    endif()
    add_custom_target(${device_type}_list_gen DEPENDS "${dev_list_file}")
    add_library(${device_type}_list INTERFACE)
    target_include_directories(
        ${device_type}_list
        INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/${device_type}"
    )
    add_dependencies(${device_type}_list ${device_type}_list_gen ${header_file})
    # NOTE: Add this header to the CONFIG_LIBRARIES and GENERATED_FILES properties?
endfunction(gen_device_header)
