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
include_guard(GLOBAL)

function(ApplyData61ElfLoaderSettings kernel_platform kernel_sel4_arch)
    set(
        binary_list
        "tx1;hikey;am335x;odroidc2;imx8mq-evk;rockpro64;zynqmp;imx8mm-evk;hifive"
    )
    set(efi_list "tk1")
    set(uimage_list "tx2")
    if(
        ${kernel_platform} IN_LIST efi_list
        OR (${kernel_platform} STREQUAL "hikey" AND ${kernel_sel4_arch} STREQUAL "aarch64")
    )
        set(ElfloaderImage "efi" CACHE STRING "" FORCE)
    elseif(${kernel_platform} IN_LIST uimage_list)
        set(ElfloaderImage "uimage" CACHE STRING "" FORCE)
    elseif(${kernel_platform} STREQUAL "rpi3" AND ${kernel_sel4_arch} STREQUAL "aarch64")
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
    elseif(${kernel_platform} IN_LIST binary_list)
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
    endif()

    if(${kernel_platform} STREQUAL "tk1" AND ${kernel_sel4_arch} STREQUAL "arm_hyp")
        set(ElfloaderMode "hypervisor" CACHE STRING "" FORCE)
        set(ElfloaderMonitorHook ON CACHE BOOL "" FORCE)
    endif()
    if((KernelPlatformImx8mm-evk OR KernelPlatformImx8mq-evk) AND KernelSel4ArchAarch32)
        set(ElfloaderArmV8LeaveAarch64 ON CACHE BOOL "" FORCE)
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

# Try and map a PLATFORM value to a valid kernel platform and architecture
# setting based on some established conventions:
#  - Any previous value taken by KernelARMPlatform
#  - Any value accepted by KernelPlatform
#  - x86_64 and ia32 map to pc99
#
# Additionally use the following boolean configs to indicate which seL4 arch
# to select:
#  - aarch32: ARM, AARCH32, AARCH32HF
#  - arm_hyp: ARM_HYP
#  - aarch64: AARCH64
#  - riscv64: RISCV64
#  - riscv32: RISCV32
#
# Calling this function will result in forced updates to the cache.
function(correct_platform_strings)
    set(_REWRITE ON)
    if("${PLATFORM}" STREQUAL "sabre")
        set(KernelPlatform imx6 CACHE STRING "" FORCE)
        set(KernelARMPlatform sabre CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "wandq")
        set(KernelPlatform imx6 CACHE STRING "" FORCE)
        set(KernelARMPlatform wandq CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "kzm")
        set(KernelPlatform imx31 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "rpi3")
        set(KernelPlatform bcm2837 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "exynos5250")
        set(KernelPlatform exynos5 CACHE STRING "" FORCE)
        set(KernelARMPlatform exynos5250 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "exynos5410")
        set(KernelPlatform exynos5 CACHE STRING "" FORCE)
        set(KernelARMPlatform exynos5410 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "exynos5422")
        set(KernelPlatform exynos5 CACHE STRING "" FORCE)
        set(KernelARMPlatform exynos5422 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "am335x-boneblack")
        set(KernelPlatform am335x CACHE STRING "" FORCE)
        set(KernelARMPlatform am335x-boneblack CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "am335x-boneblue")
        set(KernelPlatform am335x CACHE STRING "" FORCE)
        set(KernelARMPlatform am335x-boneblue CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "x86_64")
        set(KernelPlatform pc99 CACHE STRING "" FORCE)
        set(KernelSel4Arch x86_64 CACHE STRING "" FORCE)
    elseif("${PLATFORM}" STREQUAL "ia32")
        set(KernelPlatform pc99 CACHE STRING "" FORCE)
        set(KernelSel4Arch ia32 CACHE STRING "" FORCE)
    elseif(NOT "${PLATFORM}" STREQUAL "")
        set(KernelPlatform ${PLATFORM} CACHE STRING "" FORCE)
        set(_REWRITE OFF)
    else()
        set(_REWRITE OFF)
    endif()
    if(_REWRITE AND (NOT correct_platform_strings_no_print))
        message("correct_platform_strings: Attempting to correct PLATFORM: ${PLATFORM}
            to new valid KernelPlatform: ${KernelPlatform}")
        if("${KernelPlatform}" STREQUAL pc99)
            message("                         KernelSel4Arch: ${KernelSel4Arch}")
        else()
            message("                      KernelARMPlatform: ${KernelARMPlatform}")
        endif()
    endif()
    set(_REWRITE ON)

    if(ARM OR AARCH32 OR AARCH32HF)
        if(
            ARM_HYP
            OR ("${KernelSel4Arch}" STREQUAL arm_hyp)
            OR ("${KernelArmSel4Arch}" STREQUAL arm_hyp)
        )
            set(KernelSel4Arch arm_hyp CACHE STRING "" FORCE)
        else()
            set(KernelSel4Arch aarch32 CACHE STRING "" FORCE)
        endif()
    elseif(AARCH64)
        set(KernelSel4Arch aarch64 CACHE STRING "" FORCE)
    elseif(RISCV64)
        set(KernelSel4Arch riscv64 CACHE STRING "" FORCE)
    elseif(RISCV32)
        set(KernelSel4Arch riscv32 CACHE STRING "" FORCE)
    else()
        set(_REWRITE OFF)
    endif()
    if(_REWRITE AND (NOT correct_platform_strings_no_print))
        message(
            "correct_platform_strings: Based on toolchain, setting KernelSel4Arch: ${KernelSel4Arch}"
        )
    endif()

    # This is a common mechanism for how x86 architectures were selected
    if("${KernelX86Sel4Arch}" STREQUAL ia32)
        set(KernelSel4Arch ia32 CACHE STRING "" FORCE)
    elseif("${KernelX86Sel4Arch}" STREQUAL x86_64)
        set(KernelSel4Arch x86_64 CACHE STRING "" FORCE)
    endif()

    # Only print out these info messages on first initialisation
    # otherwise the ccache gets interrupted with output everytime it is used.
    # The ccache also has a mechanism for showing what config options get chagned
    # after a configuration anyway so the user will still be informed.
    set(correct_platform_strings_no_print ON CACHE INTERNAL "")
endfunction()
