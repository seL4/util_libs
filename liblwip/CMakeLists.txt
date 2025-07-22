#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)
project(liblwip C)

set(configure_string "")

config_option(LibLwip LIB_LWIP "Build LwIP" DEFAULT OFF)
mark_as_advanced(LibLwip)
add_config_library(lwip "${configure_string}")
if(LibLwip)
    set(projects_dir "${CMAKE_CURRENT_LIST_DIR}/../..")
    find_file(LWIP_PATH lwip PATHS ${projects_dir} CMAKE_FIND_ROOT_PATH_BOTH)
    mark_as_advanced(FORCE LWIP_PATH)
    if("${LWIP_PATH}" STREQUAL "LWIP_PATH-NOTFOUND")
        message(FATAL_ERROR "Failed to find lwIP. Consider cmake -DLWIP_PATH=/path/to/lwip")
    endif()

    get_property(compile_options DIRECTORY PROPERTY COMPILE_OPTIONS)
    separate_arguments(cmake_c_flags_sep NATIVE_COMMAND "${CMAKE_C_FLAGS}")
    list(APPEND compile_options "${cmake_c_flags_sep}")

    add_custom_target(get_muslc)
    add_dependencies(get_muslc muslc)

    file(
        GLOB
            sources
            ${LWIP_PATH}/src/*/*.c
            ${LWIP_PATH}/src/core/ipv4/*.c
            ${LWIP_PATH}/src/apps/snmp/*.c
    )

    add_library(lwip STATIC EXCLUDE_FROM_ALL ${sources})

    target_include_directories(
        lwip
        PUBLIC
            ${LWIP_PATH}/src/include
            # Include the header files in this directory which contain header configs
            include include/lwip
    )

    target_link_libraries(lwip muslc liblwip_config)

    target_compile_options(lwip PRIVATE ${compile_options})
endif()
