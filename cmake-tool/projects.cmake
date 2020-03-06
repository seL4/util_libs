#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

file(GLOB result RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" projects/*/CMakeLists.txt)

# We sort the results to ensure that builds are deterministic. Whilst build scripts
# should not be written to need a particular order of globbed results here, it is
# better to have reliable builds than random failures
list(SORT result)

foreach(file ${result})
    string(
        REPLACE
            "CMakeLists.txt"
            ""
            file
            "${file}"
    )
    add_subdirectory("${file}")
endforeach()
