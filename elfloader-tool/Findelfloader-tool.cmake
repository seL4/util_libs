#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

set(ELFLOADER_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(ELFLOADER_CURRENT_DIR)

function(elfloader_import_project)
    add_subdirectory(${ELFLOADER_CURRENT_DIR} elfloader)
endfunction()

include(${ELFLOADER_CURRENT_DIR}/helpers.cmake)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(elfloader-tool DEFAULT_MSG ELFLOADER_CURRENT_DIR)
