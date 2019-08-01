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

project(elfloader C ASM)

if(KernelArchX86)
    # This project is only used on Arm or RISC-V
    return()
endif()

include(${KERNEL_FLAGS_PATH})
include(cpio)

set(configure_string "")

config_choice(
    ElfloaderImage
    ELFLOADER_IMAGE
    "Boot image type"
    "elf;ElfloaderImageELF;IMAGE_ELF;KernelArchARM OR KernelArchRiscV"
    "binary;ElfloaderImageBinary;IMAGE_BINARY;KernelArchARM OR KernelArchRiscV"
    "efi;ElfloaderImageEFI;IMAGE_EFI;KernelArchARM"
    "uimage;ElfloaderImageUimage;IMAGE_UIMAGE;KernelArchARM"
)

config_choice(
    ElfloaderMode
    ELFLOADER_MODE
    "seL4 mode"
    "secure supervisor;ElfloaderModeSSup;ARM_S_SUPERVISOR_MODE;KernelPlatImx6"
    "monitor;ElfloaderModeMonitor;ARM_MONITOR_MODE;KernelPlatformTK1 OR KernelPlatImx6"
    "hypervisor;ElfloaderModeHyp;ARM_HYPERVISOR_MODE;KernelPlatformTK1"
    "non-secure supervisor;ElfloaderModeNSSup;ARM_NS_SUPERVISOR_MODE;KernelPlatformTK1 OR KernelPlatImx6"
)

config_option(
    ElfloaderMonitorHook ARM_MONITOR_HOOK "Install SMC call handlers in monitor mode."
    DEFAULT OFF
    DEPENDS "KernelPlatformTK1 OR KernelPlatImx6"
)

config_option(
    ElfloaderGPTPtimersNSPL1Access GPT_PTIMER_NS_PL1_ACCESS
    "Enable the GPT physical timer access for NS PL1"
    DEFAULT ON
    DEPENDS "ElfloaderModeNSSup"
    DEFAULT_DISABLED OFF
)

config_option(
    ElfloaderErrata764369 ARM_ERRATA_764369
    "Work around for a Cortex-A9 errata. Derived from Linux kernel."
    DEFAULT ON
    DEPENDS "KernelArmCortexA9"
    DEFAULT_DISABLED OFF
)

config_choice(
    ElfloaderHashInstructions
    HASH_INSTRUCTIONS
    "Perform a SHA256/MD5 Hash of the of each elf file that the elfloader checks on load"
    "hash_none;ElfloaderHashNone;HASH_NONE"
    "hash_sha;ElfloaderHashSHA;HASH_SHA"
    "hash_md5;ElfloaderHashMD5;HASH_MD5"
)

config_option(
    ElfloaderIncludeDtb ELFLOADER_INCLUDE_DTB
    "Include DTB in the CPIO in case bootloader doesn't provide one"
    DEFAULT ON
    DEPENDS "KernelArchARM OR KernelArchRiscV"
    DEFAULT_DISABLED OFF
)

config_option(
    ElfloaderRootserversLast ELFLOADER_ROOTSERVERS_LAST
    "Place the rootserver images at the end of memory"
    DEFAULT
        OFF
        # This setting does not work on EFI as paging structures are not
        # enabled to write to high memory
    DEPENDS "NOT ElfloaderImageEFI;"
    DEFAULT_DISABLED OFF
)

config_option(
    ElfloaderArmV8LeaveAarch64 ELFLOADER_ARMV8_LEAVE_AARCH64
    "Insert aarch64 code to switch to aarch32. Requires the elfloader to be in EL2"
    DEFAULT OFF
    DEPENDS KernelArchArmV8a
)

add_config_library(elfloader "${configure_string}")

add_compile_options(-D_XOPEN_SOURCE=700 -ffreestanding -Wall -Werror -W -Wextra)
set(linkerScript "${CMAKE_CURRENT_LIST_DIR}/src/arch-${KernelArch}/linker.lds")
if(KernelArchRiscV)
    add_compile_options(-mcmodel=medany)
endif()

if(ElfloaderArmV8LeaveAarch64)
    # We need to build a aarch64 assembly file during an aarch32 build. We have
    # to write custom rules to do this as CMake doesn't support multiple compilers
    # within a single build config.
    find_program(AARCH64_COMPILER aarch64-linux-gnu-gcc)
    if("${AARCH64_COMPILER}" STREQUAL "AARCH64_COMPILER-NOTFOUND")
        message(
            FATAL_ERROR
                "Cannot find 'aarch64-linux-gnu-gcc' program. Use -DAARCH64_COMPILER=compiler"
        )
    endif()
    find_program(AARCH64_OBJCOPY aarch64-linux-gnu-objcopy)
    if("${AARCH64_OBJCOPY}" STREQUAL "AARCH64_OBJCOPY-NOTFOUND")
        message(
            FATAL_ERROR
                "Cannot find 'aarch64-linux-gnu-objcopy' program. Use -DAARCH64_OBJCOPY=objcopy"
        )
    endif()
    # Compile crt0_64.S and convert to a binary. This way the actual crt0.S can use
    # the .incbin directive and insert the aarch64 instructions before its own.
    add_custom_command(
        OUTPUT crt0_64.bin crt0_64.o
        COMMAND
            ${AARCH64_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/include/
            -I${CMAKE_CURRENT_SOURCE_DIR}/include/arch-arm/64/
            -I${CMAKE_CURRENT_SOURCE_DIR}/include/arch-arm/armv/armv8-a/64/ -c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/arch-arm/32/crt0_64.S -o
            ${CMAKE_CURRENT_BINARY_DIR}/crt0_64.o
        COMMAND
            ${AARCH64_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/crt0_64.o crt0_64.bin
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/arch-arm/32/crt0_64.S
    )

    # We set the OBJECT_DEPENDS property on the crt0.S source file that tells CMake
    # any object files created from crt0.S also depend on crt0_64.bin. This causes
    # our builds to be rerun correctly.
    set(armv8_leave_arch_bin ${CMAKE_CURRENT_BINARY_DIR}/crt0_64.bin)
    set_property(SOURCE src/arch-arm/32/crt0.S PROPERTY OBJECT_DEPENDS ${armv8_leave_arch_bin})
endif()

if(KernelSel4ArchAarch64)
    # NEON registers aren't necessarily initialized for use before elfloader starts
    add_compile_options(-mgeneral-regs-only)
endif()

file(
    GLOB
        files
        src/*.c
        src/utils/*.c
        src/arch-${KernelArch}/*.c
        src/arch-${KernelArch}/*.S
        src/plat/${KernelPlatform}/*.c
        src/binaries/elf/*.c
        src/arch-${KernelArch}/${KernelWordSize}/*.c
        src/plat/${KernelPlatform}/*.S
        src/arch-${KernelArch}/${KernelWordSize}/*.S
)

# We never want to give crt0_64.S to add_executable
list(FILTER files EXCLUDE REGEX src/arch-arm/32/crt0_64.S)

if(KernelArchARM)
    file(
        GLOB
            arm_files src/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}/*.c
            src/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}/*.S
    )
    list(APPEND files ${arm_files})
endif()

if(ElfloaderImageEFI)
    file(GLOB efi_files src/binaries/efi/*.c)
    list(APPEND files "src/binaries/efi/efi_head.S" ${efi_files})
    # We cannot control where EFI loads the image and so we must make it relocatable
    add_compile_options(-fpic)
    if(KernelSel4ArchAarch32)
        # on aarch32 building with -fno-pie results in the compiler generating
        # movt/movw pairs that we can't easily relocate.
        add_compile_options(-fpie -mno-single-pic-base)
    else()
        # on aarch64 building with -fno-pie will just use pc-relative addressing.
        add_compile_options(-fno-pie)
    endif()
else()
    add_compile_options(-fno-pic)
    add_compile_options(-fno-pie)
endif()

# Sort files to make build reproducible
list(SORT files)

set(cpio_files "")
list(APPEND cpio_files "$<TARGET_FILE:kernel.elf>")
if(ElfloaderIncludeDtb)
    list(APPEND cpio_files "${KernelDTBPath}")
endif()
list(APPEND cpio_files "$<TARGET_PROPERTY:rootserver_image,ROOTSERVER_IMAGE>")
if(NOT ${ElfloaderHashInstructions} STREQUAL "hash_none")
    set(hash_command "")
    if(ElfloaderHashSHA)
        set(hash_command "sha256sum")
    else()
        set(hash_command "md5sum")
    endif()
    add_custom_command(
        OUTPUT "kernel.bin"
        COMMAND
            bash -c
            "${hash_command} $<TARGET_FILE:kernel.elf> | cut -d ' ' -f 1 | xxd -r -p > ${CMAKE_CURRENT_BINARY_DIR}/kernel.bin"
        VERBATIM
        DEPENDS "$<TARGET_FILE:kernel.elf>"
    )
    add_custom_command(
        OUTPUT "app.bin"
        COMMAND
            bash -c
            "${hash_command} $<TARGET_PROPERTY:rootserver_image,ROOTSERVER_IMAGE> | cut -d ' ' -f 1 | xxd -r -p > ${CMAKE_CURRENT_BINARY_DIR}/app.bin"
        VERBATIM
        DEPENDS "$<TARGET_PROPERTY:rootserver_image,ROOTSERVER_IMAGE>"
    )
    list(APPEND cpio_files "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin")
    list(APPEND cpio_files "${CMAKE_CURRENT_BINARY_DIR}/app.bin")
endif()

# Construct the ELF loader's payload.
MakeCPIO(archive.o "${cpio_files}" CPIO_SYMBOL _archive_start)

# If our platform has a YAML description, create a C header file to include
# information about the platform that is of interest to the ELF-loader, such as
# a physical memory map.
#
# We also need to put the ELF-loader's payload in memory at a place that will be
# out of the way of the kernel and user images that the elfloader extracts.  In
# other words, we don't want the ELF-loader (with its payload) to clobber
# itself.
#
# Formerly there was a complex set of conditionals driving a table of
# hard-coded addresses.
#
# Now, instead, we compute a reasonable image start address, using a tool called
# `shoehorn`, based on knowledge of how where and how big the extracted payloads
# will be, obtained by a tool called `elf_sift`.
if(DEFINED platform_yaml)
    set(PLATFORM_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen_headers/")
    set(PLATFORM_SIFT "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-tool/helpers/platform_sift.py")
    set(PLATFORM_INFO_H "${PLATFORM_HEADER_DIR}/platform_info.h")
    add_custom_command(
        OUTPUT ${PLATFORM_INFO_H}
        COMMAND ${PLATFORM_SIFT} --emit-c-syntax ${platform_yaml} > ${PLATFORM_INFO_H}
        VERBATIM
        DEPENDS ${platform_yaml} ${PLATFORM_SIFT}
    )
    set_property(SOURCE src/common.c PROPERTY OBJECT_DEPENDS ${PLATFORM_INFO_H})

    # Construct the `shoehorn` command line.  Convert the semicolon-delimited
    # CMake list of CPIO archive members to a whitespace-separated list so we
    # can pass it via the shell to `shoehorn`.
    string(
        REPLACE
            ";"
            " "
            cpio_files_ws
            "${cpio_files}"
    )
    set(ARCHIVE_O "${CMAKE_CURRENT_BINARY_DIR}/archive.o ")
    # `shoehorn` calls `elf_sift`.
    set(ELF_SIFT "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-tool/helpers/elf_sift.py")
    set(SHOEHORN "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-tool/helpers/shoehorn.py")
    set(IMAGE_START_ADDR_H "${PLATFORM_HEADER_DIR}/image_start_addr.h")
    set(
        SHOEHORN_COMMAND
        "${SHOEHORN} ${platform_yaml} ${ARCHIVE_O} ${cpio_files_ws} > ${IMAGE_START_ADDR_H}"
    )

    if(NOT "${IMAGE_START_ADDR}" STREQUAL "")
        file(WRITE ${IMAGE_START_ADDR_H} "#define IMAGE_START_ADDR ${IMAGE_START_ADDR}")
    else()
        add_custom_command(
            OUTPUT ${IMAGE_START_ADDR_H}
            COMMAND sh -c "${SHOEHORN_COMMAND}"
            VERBATIM
            DEPENDS archive.o
            DEPENDS ${cpio_files} ${ELF_SIFT} ${SHOEHORN}
        )
    endif()
else()
    message(
        FATAL_ERROR
            "no image start address computed for platform; use platform YAML file ${platform_yaml} or replace this diagnostic with a CMake custom command to generate a file \"${IMAGE_START_ADDR_H}\" that hard-codes it; the same goes for \"${PLATFORM_INFO_H}\""
    )
endif()

# Generate linker script
separate_arguments(c_arguments NATIVE_COMMAND "${CMAKE_C_FLAGS}")
add_custom_command(
    OUTPUT "linker.lds_pp"
    COMMAND
        ${CMAKE_C_COMPILER} "${c_arguments}" "-I${PLATFORM_HEADER_DIR}"
        "-I$<JOIN:$<TARGET_PROPERTY:sel4_autoconf,INTERFACE_INCLUDE_DIRECTORIES>,;-I>"
        "-I$<JOIN:$<TARGET_PROPERTY:elfloader_Config,INTERFACE_INCLUDE_DIRECTORIES>,;-I>" -P -E -o
        linker.lds_pp -x c ${linkerScript}
    DEPENDS
        sel4_autoconf
        ${linkerScript}
        elfloader_Config
        ${IMAGE_START_ADDR_H}
    VERBATIM COMMAND_EXPAND_LISTS
)
add_custom_target(elfloader_linker DEPENDS linker.lds_pp)

add_executable(elfloader EXCLUDE_FROM_ALL ${files} archive.o)
set_target_properties(elfloader PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/linker.lds_pp)
target_include_directories(
    elfloader
    PRIVATE
        "include"
        "include/plat/${KernelPlatform}"
        "include/arch-${KernelArch}"
        "include/arch-${KernelArch}/${KernelWordSize}"
        "${CMAKE_CURRENT_BINARY_DIR}/gen_headers"
        "${CMAKE_CURRENT_BINARY_DIR}"
)
if(KernelArchARM)
    target_include_directories(
        elfloader
        PRIVATE
            "include/arch-${KernelArch}/armv/${KernelArmArmV}"
            "include/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}"
    )
endif()

target_link_libraries(
    elfloader
    PRIVATE
        cpio
        gcc
        elfloader_Config
        sel4_autoconf
)
add_dependencies(elfloader elfloader_linker)
set_property(
    TARGET elfloader
    APPEND_STRING
    PROPERTY
        LINK_FLAGS
        " -T ${CMAKE_CURRENT_BINARY_DIR}/linker.lds_pp -nostdlib -static -Wl,--build-id=none"
)

if(ElfloaderImageEFI)
    set_property(TARGET elfloader APPEND_STRING PROPERTY LINK_FLAGS " -pie ")
endif()
