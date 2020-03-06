#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)
# Include our common helpers
list(
    APPEND
        CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/helpers/ ${CMAKE_SOURCE_DIR}/projects/musllibc
)

include(check_arch_compiler)

enable_language(C)
enable_language(CXX)
enable_language(ASM)

# Hide cmake variables that will just confuse the user
mark_as_advanced(
    FORCE
    CMAKE_INSTALL_PREFIX
    CROSS_COMPILER_PREFIX
    EXECUTABLE_OUTPUT_PATH
    CMAKE_BACKWARDS_COMPATIBILITY
    LIBRARY_OUTPUT_PATH
    CMAKE_ASM_COMPILER
    CMAKE_C_COMPILER
)

find_file(KERNEL_PATH kernel PATHS ${CMAKE_SOURCE_DIR} NO_CMAKE_FIND_ROOT_PATH)
mark_as_advanced(FORCE KERNEL_PATH)
if("${KERNEL_PATH}" STREQUAL "KERNEL_PATH-NOTFOUND")
    message(FATAL_ERROR "Failed to find kernel. Consider cmake -DKERNEL_PATH=/path/to/kernel")
endif()

# Give an explicit build directory as there is no guarantee this is actually
# subdirectory from the root source hierarchy
add_subdirectory("${KERNEL_PATH}" kernel)
# Include helpers from the kernel
include(${KERNEL_HELPERS_PATH})

include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")

# Due to some symlink madness in some project manifests we first get the realpath
# for the current list directory and use that to find the elfloader
# Both of these might not be subdirectories from our source root, so also
# give them both explicit build directories
get_filename_component(real_list "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
# Include the elfloader before setting up user mode flags, as the elfloader does
# not run as a user under the kernel
add_subdirectory("${real_list}/../elfloader-tool" elfloader-tool)

find_package(musllibc REQUIRED)
# Make build options a visible choice, default it to Debug
set(
    CMAKE_BUILD_TYPE "Debug"
    CACHE STRING "Set the user mode build type (kernel build ignores this)"
)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)

# Declare build flags for using musllibc in targets
musllibc_set_environment_flags()

# Now all platform compilation flags have been set, we can check the compiler against flags
check_arch_compiler()

add_subdirectory("${KERNEL_PATH}/libsel4" libsel4)
