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
