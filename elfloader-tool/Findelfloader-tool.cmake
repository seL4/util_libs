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

set(ELFLOADER_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(ELFLOADER_CURRENT_DIR)

function(elfloader_import_project)
    add_subdirectory(${ELFLOADER_CURRENT_DIR} elfloader)
endfunction()

include(${ELFLOADER_CURRENT_DIR}/helpers.cmake)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(elfloader-tool DEFAULT_MSG ELFLOADER_CURRENT_DIR)
