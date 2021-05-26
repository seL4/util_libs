#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2020, HENSOLDT Cyber GmbH
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)
include_guard(GLOBAL)

function(ApplyData61ElfLoaderSettings kernel_platform kernel_sel4_arch)
    set(binary_list "tx1;hikey;odroidc2;imx8mq-evk;zynqmp;imx8mm-evk;hifive")
    set(efi_list "tk1;rockpro64")
    set(uimage_list "tx2;am335x")
    if(
        ${kernel_platform} IN_LIST efi_list
        OR (${kernel_platform} STREQUAL "hikey" AND ${kernel_sel4_arch} STREQUAL "aarch64")
    )
        set(ElfloaderImage "efi" CACHE STRING "" FORCE)
    elseif(${kernel_platform} IN_LIST uimage_list)
        set(ElfloaderImage "uimage" CACHE STRING "" FORCE)
        #rpi3
    elseif(${kernel_platform} STREQUAL "bcm2837" AND ${kernel_sel4_arch} STREQUAL "aarch64")
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
        #rpi4
    elseif(${kernel_platform} STREQUAL "bcm2711" AND ${kernel_sel4_arch} STREQUAL "aarch64")
        set(ElfloaderImage "efi" CACHE STRING "" FORCE)
    elseif(${kernel_platform} IN_LIST binary_list)
        set(ElfloaderImage "binary" CACHE STRING "" FORCE)
    else()
        set(ElfloaderImage "elf" CACHE STRING "" FORCE)
    endif()

    if(${kernel_platform} STREQUAL "tk1" AND ${kernel_sel4_arch} STREQUAL "arm_hyp")
        set(ElfloaderMode "hypervisor" CACHE STRING "" FORCE)
        set(ElfloaderMonitorHook ON CACHE BOOL "" FORCE)
    endif()
    if((KernelPlatformImx8mm-evk OR KernelPlatformImx8mq-evk) AND KernelSel4ArchAarch32)
        set(ElfloaderArmV8LeaveAarch64 ON CACHE BOOL "" FORCE)
        # This applies to imx8mm and imx8mq when in aarch32 configuration
        # It should be possible to use a uimage format but when tried nothing
        # runs after uboot.
        set(IMAGE_START_ADDR 0x41000000 CACHE INTERNAL "" FORCE)
    endif()
    if(KernelPlatformHikey AND KernelSel4ArchAarch32)
        # This is preserving what the Hikey's bootloader requires.
        set(IMAGE_START_ADDR 0x1000 CACHE INTERNAL "" FORCE)
    endif()
    if(KernelPlatformZynqmp AND KernelSel4ArchAarch32)
        set(IMAGE_START_ADDR 0x8000000 CACHE INTERNAL "" FORCE)
    endif()
    if(KernelPlatformSpike AND KernelSel4ArchRiscV32)
        set(IMAGE_START_ADDR 0x80400000 CACHE INTERNAL "" FORCE)
    endif()
endfunction()

function(ApplyCommonSimulationSettings kernel_sel4_arch)
    if("${kernel_sel4_arch}" STREQUAL "x86_64" OR "${kernel_sel4_arch}" STREQUAL "ia32")
        # Generally we cannot simulate some more recent features
        set(KernelSupportPCID OFF CACHE BOOL "" FORCE)
        if("${kernel_sel4_arch}" STREQUAL "ia32")
            set(KernelFSGSBase gdt CACHE STRING "" FORCE)
        else()
            set(KernelFSGSBase msr CACHE STRING "" FORCE)
        endif()
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
    mark_as_advanced(CMAKE_BUILD_TYPE)
endfunction()

# Try and map a PLATFORM value to a valid kernel platform and architecture
# variables. Additionally, the following boolean configs can be set to indicate
# which seL4 arch to select:
#  - aarch32: ARM, AARCH32, AARCH32HF
#  - arm_hyp: ARM_HYP
#  - aarch64: AARCH64
#  - riscv64: RISCV64
#  - riscv32: RISCV32
#
# Calling this function will result in forced updates to the cache.
function(correct_platform_strings)

    if(KernelX86Sel4Arch)
        # this used to be a common mechanism how x86 architectures were
        # selected. It's deprecated now and PLATFORM should be used instead. As
        # of Nov/2020, we don't make this an error to give everybody a chance
        # to update their scripts.
        if("${PLATFORM}" STREQUAL "")
            if(NOT correct_platform_strings_no_print)
                message(
                    DEPRECATION
                        "setting PLATFORM from deprecated KernelX86Sel4Arch: ${KernelX86Sel4Arch}"
                )
            endif()
            set(PLATFORM "${KernelX86Sel4Arch}")
        elseif("${PLATFORM}" STREQUAL "${KernelX86Sel4Arch}")
            if(NOT correct_platform_strings_no_print)
                message(DEPRECATION "KernelX86Sel4Arch is deprecated, use PLATFORM only")
            endif()
        else()
            message(
                FATAL_ERROR
                    "PLATFORM=${PLATFORM} does not match KernelX86Sel4Arch=${KernelX86Sel4Arch}"
            )
        endif()
    elseif(KernelArmSel4Arch)
        # this has never been widely in use, so we stop supporting it
        message(FATAL_ERROR "KernelArmSel4Arch is no longer supported, use PLATFROM")
    elseif(KernelRiscVSel4Arch)
        # this should not have been in use at all
        message(FATAL_ERROR "KernelRiscVSel4Arch is no longer supported, use PLATFROM")
    endif()

    set(
        platform_aliases
        # The elements of this list are:
        #      "-"+<name of architecture #1 specific platform variable>
        #      <chip family>:<board_1>,<board_2>...
        #      <chip family>:<board_1>,<board_2>...
        #      ...
        #      "-"+<name of architecture #2 specific platform variable>
        #      <chip family>:<board_1>,<board_2>...
        #      ...
        #  where this function will try to match PLATFORM against <board_n> and
        #  then set:
        #      KernelPlatform=<chip family>
        #      <platform variable>=<board_n>
        # Note that <board_n> must be a unique name, as the first match will be
        # used. If there was no match for any board the function will set:
        #     KernelPlatform=${PLATFORM}
        # and leave all further setup to the architecture/platform specific
        # configuration to <sel4 kernel>/src/plat/*/config.cmake
        #
        "-KernelARMPlatform"
        "imx6:sabre,wandq,nitrogen6sx"
        "imx31:kzm"
        "bcm2837:rpi3"
        "bcm2711:rpi4"
        "exynos5:exynos5250,exynos5410,exynos5422"
        "am335x:am335x-boneblack,am335x-boneblue"
        "-KernelSel4Arch"
        "pc99:x86_64,ia32"
    )

    set(all_boards "")
    set(block_kernel_var "")
    set(kernel_var "")

    foreach(item IN LISTS platform_aliases)

        if(item MATCHES "^-(.*)$")
            set(block_kernel_var "${CMAKE_MATCH_1}")
            continue()
        endif()

        if(NOT block_kernel_var)
            message(
                FATAL_ERROR
                    "platform_aliases must set architecture specific kernel platform variable first"
            )
        endif()

        if(NOT item MATCHES "^(.*):(.*)$")
            message(FATAL_ERROR "invalid line in platform_aliases: ${item}")
        endif()

        set(plat "${CMAKE_MATCH_1}")

        string(
            REPLACE
                ","
                ";"
                item_board_list
                "${CMAKE_MATCH_2}"
        )

        # remember board alias names, we need to build a complete list
        list(APPEND all_boards "${item_board_list}")

        if(kernel_var OR ("${PLATFORM}" STREQUAL "") OR (NOT "${PLATFORM}" IN_LIST item_board_list))
            continue()
        endif()

        if(KernelPlatform AND (NOT "${KernelPlatform}" STREQUAL "${plat}"))
            message(
                FATAL_ERROR
                    "config mismatch, wont overwrite KernelPlatform=${KernelPlatform} with ${plat}"
            )
        endif()
        set(KernelPlatform "${plat}" CACHE STRING "" FORCE)

        if(${block_kernel_var} AND (NOT "${${block_kernel_var}}" STREQUAL "${PLATFORM}"))
            message(
                FATAL_ERROR
                    "config mismatch, wont overwrite ${block_kernel_var}=${${block_kernel_var}} with ${PLATFORM}"
            )
        endif()
        set(kernel_var "${block_kernel_var}")
        set(${kernel_var} "${PLATFORM}" CACHE STRING "" FORCE)

    endforeach()

    if(NOT kernel_var)
        set(KernelPlatform "${PLATFORM}" CACHE STRING "" FORCE)
    endif()

    # declare a special variable that some CMake files expect to hold a list of
    # all board alias names from the "database" above
    set(correct_platform_strings_platform_aliases "${all_boards}" CACHE INTERNAL "")

    # printing a message about the adaption is optional. When CMake is
    # invoked multiple times to get a stable configuration, we print this
    # for the first run only.
    if(NOT correct_platform_strings_no_print)
        message(STATUS "Set platform details from PLATFORM=${PLATFORM}")
        message(STATUS "  KernelPlatform: ${KernelPlatform}")
        if(kernel_var)
            message(STATUS "  ${kernel_var}: ${${kernel_var}}")
        endif()
    endif()

    set(_REWRITE ON)

    if(ARM OR AARCH32 OR AARCH32HF)
        # "arm_hyp" was needed as a new kernel architecture long time ago when
        # the scripts that produced a kernel that was given to L4V could only
        # handle changing the kernel architecture. It was used to mean
        # "aarch32 + hyp extensions". Now it would be possible to completely
        # remove it and follow the same pattern as aarch64.
        if(ARM_HYP OR ("${KernelSel4Arch}" STREQUAL "arm_hyp"))
            set(KernelSel4Arch "arm_hyp" CACHE STRING "" FORCE)
        else()
            set(KernelSel4Arch "aarch32" CACHE STRING "" FORCE)
        endif()
    elseif(AARCH64)
        set(KernelSel4Arch "aarch64" CACHE STRING "" FORCE)
    elseif(RISCV64)
        set(KernelSel4Arch "riscv64" CACHE STRING "" FORCE)
    elseif(RISCV32)
        set(KernelSel4Arch "riscv32" CACHE STRING "" FORCE)
    else()
        set(_REWRITE OFF)
    endif()
    if(_REWRITE AND (NOT correct_platform_strings_no_print))
        message(STATUS "Setting from flags KernelSel4Arch: ${KernelSel4Arch}")
    endif()

    # Only print out these info messages on first initialisation
    # otherwise the ccache gets interrupted with output everytime it is used.
    # The ccache also has a mechanism for showing what config options get
    # changed after a configuration anyway so the user will still be informed.
    set(correct_platform_strings_no_print ON CACHE INTERNAL "")

endfunction()
