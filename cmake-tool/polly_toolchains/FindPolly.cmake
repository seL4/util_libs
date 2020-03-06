#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

# Save path to checkout of https://github.com/ruslo/polly
macro(FindPolly)
    find_path(POLLY_DIR linux-gcc-x64.cmake ${CMAKE_CURRENT_LIST_DIR}/../../../../tools/polly)
    mark_as_advanced(FORCE POLLY_DIR)
    if("${POLLY_DIR}" STREQUAL "POLLY_DIR-NOTFOUND")
        message(FATAL_ERROR "Failed to find polly. Consider cmake -DPOLLY_DIR=/path/to/polly")
    endif()
endmacro()
