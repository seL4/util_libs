#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

set(UTIL_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
set(LWIP_HELPERS "${CMAKE_CURRENT_LIST_DIR}/liblwip/lwip_helpers.cmake" CACHE STRING "")
mark_as_advanced(UTIL_LIBS_DIR LWIP_HELPERS)

macro(util_libs_import_libraries)
    add_subdirectory(${UTIL_LIBS_DIR} util_libs)
endmacro()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(util_libs DEFAULT_MSG UTIL_LIBS_DIR LWIP_HELPERS)
