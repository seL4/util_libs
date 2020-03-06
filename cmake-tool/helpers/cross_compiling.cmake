#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
cmake_minimum_required(VERSION 3.8.2)
include_guard(GLOBAL)

# This path is guaranteed to exist
find_path(
    CUSTOM_POLLY_TOOLCHAINS FindPolly.cmake
    PATHS ${CMAKE_CURRENT_LIST_DIR}/../polly_toolchains
    CMAKE_FIND_ROOT_PATH_BOTH
)

function(FindCustomPollyToolchain location toolchain)
    find_file(
        ${location} ${toolchain}.cmake
        PATHS ${CUSTOM_POLLY_TOOLCHAINS}
        CMAKE_FIND_ROOT_PATH_BOTH
    )
endfunction()

# This file is guaranteed to exist
find_file(
    PKG_CONFIG_TEMPLATE pkg-config.in
    PATHS ${CMAKE_CURRENT_LIST_DIR}
    CMAKE_FIND_ROOT_PATH_BOTH
)
function(CreatePkgConfigExecForDir exec_name sysroot)
    configure_file(${PKG_CONFIG_TEMPLATE} ${exec_name} @ONLY)
endfunction()
