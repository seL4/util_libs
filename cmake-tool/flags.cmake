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

# Make build options a visible choice, default it to Debug
set(force "")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(force "FORCE")
endif()
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Set the user mode build type (kernel build ignores this)" ${force})
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
mark_as_advanced(CLEAR CMAKE_BUILD_TYPE)

# Setup base flags as defined by the kernel before including the rest
include(${KERNEL_FLAGS_PATH})

add_compile_options(-nostdinc -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -ftls-model=local-exec)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++")
set(CMAKE_C_STANDARD 11)
set(LinkPageSize "0x1000" CACHE STRING "Page size to be used for linker")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -nostdlib -z max-page-size=${LinkPageSize}")

if(KernelArchX86)
    add_compile_options(-mtls-direct-seg-refs)
endif()

# find the compilers crtbegin and crtend files
separate_arguments(c_arguments UNIX_COMMAND "${CMAKE_C_FLAGS}")
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} -print-file-name=crtbegin.o
    OUTPUT_VARIABLE CRTBeginFile
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} -print-file-name=crtend.o
    OUTPUT_VARIABLE CRTEndFile
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${c_arguments} --print-file-name libgcc_eh.a
    OUTPUT_VARIABLE libgcc_eh
    ERROR_VARIABLE IgnoreErrorOutput
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(libgcc "-lgcc")
if(NOT "${libgcc_eh}" STREQUAL "libgcc_eh.a")
    set(libgcc "${libgcc} -lgcc_eh")
endif()
set(CRTObjFiles "${CMAKE_CURRENT_BINARY_DIR}/lib/crt1.o ${CMAKE_CURRENT_BINARY_DIR}/lib/crti.o ${CRTBeginFile}")
set(FinObjFiles "${CRTEndFile} ${CMAKE_CURRENT_BINARY_DIR}/lib/crtn.o")

# -lgcc has to be given twice since whilst normally it should be put at the end, we implement
# some libgcc dependencies in our libc
set(common_link_string "<LINK_FLAGS> ${CRTObjFiles} <OBJECTS> ${libgcc} <LINK_LIBRARIES> \
    ${libgcc} <LINK_LIBRARIES> ${FinObjFiles} -o <TARGET>")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER>  <FLAGS> <CMAKE_C_LINK_FLAGS> ${common_link_string}")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER>  <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${common_link_string}")
set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_ASM_COMPILER>  <FLAGS> <CMAKE_ASM_LINK_FLAGS> ${common_link_string}")
