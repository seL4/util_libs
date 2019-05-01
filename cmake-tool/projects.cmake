#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
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
