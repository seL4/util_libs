#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libutils C)

file(GLOB deps src/*.c src/arch/${KernelArch}/*.c)

list(SORT deps)

set(configure_string "")

config_string(
    LibUtilsDefaultZfLogLevel
    LIB_UTILS_DEFAULT_ZF_LOG_LEVEL
    "Default ZF_LOG_LEVEL for the ZF_LOG library. 5 is LOG_LEVEL_ERROR. 0 is \
    the most verbose setting."
    DEFAULT
    5
    UNQUOTE
)
mark_as_advanced(LibUtilsDefaultZfLogLevel)
add_config_library(utils "${configure_string}")

add_library(utils STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(utils PUBLIC include arch_include/${KernelArch})
target_link_libraries(utils muslc utils_Config sel4_autoconf)
