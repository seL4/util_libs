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

enable_language(C)
enable_language(CXX)
enable_language(ASM)

# Hide cmake variables that will just confuse the user
mark_as_advanced(FORCE
    CMAKE_INSTALL_PREFIX
    CROSS_COMPILER_PREFIX
    EXECUTABLE_OUTPUT_PATH
    CMAKE_BACKWARDS_COMPATIBILITY
    LIBRARY_OUTPUT_PATH
    CCACHE_FOUND
    CMAKE_ASM_COMPILER
    CMAKE_C_COMPILER
)

find_file(KERNEL_PATH kernel PATHS ${CMAKE_SOURCE_DIR} CMAKE_FIND_ROOT_PATH_BOTH)
mark_as_advanced(FORCE KERNEL_PATH)
if("${KERNEL_PATH}" STREQUAL "KERNEL_PATH-NOTFOUND")
    message(FATAL_ERROR "Failed to find kernel. Consider cmake -DKERNEL_PATH=/path/to/kernel")
endif()

# Use ccache if possible
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# Give an explicit build directory as there is no guarantee this is actually
# subdirectory from the root source hierarchy
add_subdirectory("${KERNEL_PATH}" kernel)
# Include helpers from the kernel
include(${KERNEL_HELPERS_PATH})

# Include our common helpers
include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

# Include the elfloader before setting up user mode flags, as the elfloader does
# not run as a user under the kernel
add_subdirectory("${real_list}/../elfloader-tool" elfloader-tool)

# Setup the build flags
include("${CMAKE_CURRENT_LIST_DIR}/flags.cmake")

# Include the elfloader and libsel4 (we already added the kernel)
# Due to some symlink madness in some project manifests we first get the realpath
# for the current list directory and use that to find the elfloader
# Both of these might not be subdirectories from our source root, so also
# give them both explicit build directories
get_filename_component(real_list "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
add_subdirectory("${KERNEL_PATH}/libsel4" libsel4)
