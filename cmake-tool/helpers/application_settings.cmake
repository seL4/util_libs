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

function(ApplyData61ElfLoaderSettings kernel_arm_platform kernel_arm_sel4_arch)
    set(binary_list "tx1;hikey;am335x;odroidc2")
    set(uimage_list "tx2")
    if(${kernel_arm_platform} STREQUAL "hikey" AND ${kernel_arm_sel4_arch} STREQUAL "aarch64")
        set(ElfloaderImage "efi" CACHE STRING "" FORCE)
    elseif(${kernel_arm_platform} IN_LIST uimage_list)
        set(ElfloaderImage "uimage" CACHE STRING "" FORCE)
    elseif(${kernel_arm_platform} STREQUAL "rpi3" AND ${kernel_arm_sel4_arch} STREQUAL "aarch64")
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
    elseif(${kernel_arm_platform} IN_LIST binary_list)
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
    elseif(${kernel_arm_platform} STREQUAL "tk1" AND ${kernel_arm_sel4_arch} STREQUAL "arm_hyp")
        set(ElfloaderMode "hypervisor" CACHE STRING "" FORCE)
        set(ElfloaderMonitorHook ON CACHE BOOL "" FORCE)
    endif()
endfunction()

function(ApplyCommonSimulationSettings kernel_arch)
    if("${kernel_arch}" STREQUAL "x86")
        # Generally we cannot simulate some more recent features
        set(KernelSupportPCID OFF CACHE BOOL "" FORCE)
        set(KernelFSGSBase msr CACHE STRING "" FORCE)
        set(KernelIOMMU OFF CACHE BOOL "" FORCE)
        set(KernelFPU FXSAVE CACHE STRING "" FORCE)
    endif()
endfunction()

function(ApplyCommonReleaseVerificationSettings release verification)
    # Setup flags for different combinations of 'release' (performance optimized builds) and
    # 'verification' (verification friendly features) builds
    if(release)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
        set(KernelPrinting OFF CACHE BOOL "" FORCE)
    else()
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
    endif()
    if(verification)
        set(KernelVerificationBuild ON CACHE BOOL "" FORCE)
    else()
        set(KernelVerificationBuild OFF CACHE BOOL "" FORCE)
    endif()
    # If neither release nor verification then enable debug facilities, otherwise turn them off
    if((NOT release) AND (NOT verification))
        set(KernelDebugBuild ON CACHE BOOL "" FORCE)
        set(KernelPrinting ON CACHE BOOL "" FORCE)
    else()
        set(KernelDebugBuild OFF CACHE BOOL "" FORCE)
    endif()
endfunction()
