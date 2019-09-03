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

include(environment_flags)
# Make build options a visible choice, default it to Debug
set(force "")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(force "FORCE")
endif()
set(
    CMAKE_BUILD_TYPE "Debug"
    CACHE STRING "Set the user mode build type (kernel build ignores this)" ${force}
)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)

add_default_compilation_options()

find_libgcc_files()

set(
    CRTObjFiles
    "${CMAKE_CURRENT_BINARY_DIR}/lib/crt0.o ${CMAKE_CURRENT_BINARY_DIR}/lib/crti.o ${CRTBeginFile}"
)
set(FinObjFiles "${CRTEndFile} ${CMAKE_CURRENT_BINARY_DIR}/lib/crtn.o")

# -lgcc has to be given twice since whilst normally it should be put at the end, we implement
# some libgcc dependencies in our libc
set(common_link_string "<LINK_FLAGS> ${CRTObjFiles} <OBJECTS> ${libgcc} <LINK_LIBRARIES> \
    ${libgcc} <LINK_LIBRARIES> ${FinObjFiles} -o <TARGET>")
set(
    CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> ${common_link_string}"
)
set(
    CMAKE_CXX_LINK_EXECUTABLE
    "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${common_link_string}"
)
set(
    CMAKE_ASM_LINK_EXECUTABLE
    "<CMAKE_ASM_COMPILER>  <FLAGS> <CMAKE_ASM_LINK_FLAGS> ${common_link_string}"
)

# We want to check what we can set the -mfloat-abi to on arm and if that matches what is requested
add_fpu_compilation_options()
