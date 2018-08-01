#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#
cmake_minimum_required(VERSION 3.8.2)

# This path is guaranteed to exist
find_path(DTS_PATH "sabre.dts" PATHS ${CMAKE_CURRENT_LIST_DIR}/../../dts CMAKE_FIND_ROOT_PATH_BOTH)
# find a dts file matching platform.dts and put into a cache variable named
# <platform>_FOUND_DTS
function(FindDTS var platform)
    find_file(${platform}_FOUND_DTS ${platform}.dts PATHS ${DTS_PATH} CMAKE_FIND_ROOT_PATH_BOTH)
    if ("${${platform}_FOUND_DTS}}" STREQUAL "${platform}_FOUND_DTS-NOTFOUND")
        message(WARNING "Could not find default dts file ${PLATFORM.dts}")
    endif()
    set(${var} ${${platform}_FOUND_DTS} PARENT_SCOPE)
endfunction()
