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

include(${KERNEL_FLAGS_PATH})
include(cpio)

set(configure_string "")

config_choice(
    ElfloaderImage
    ELFLOADER_IMAGE
    "Boot image type"
    "elf;ElfloaderImageELF;IMAGE_ELF;KernelArchARM OR KernelArchRiscV"
    "binary;ElfloaderImageBinary;IMAGE_BINARY;KernelArchARM"
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
    "hash_sha;ElfloaderHashSHA;HASH_SHA;ElfloaderImageELF"
    "hash_md5;ElfloaderHashMD5;HASH_MD5;ElfloaderImageELF"
)

config_option(
    ElfloaderIncludeDtb ELFLOADER_INCLUDE_DTB
    "Include DTB in the CPIO in case bootloader doesn't provide one"
    DEFAULT ON
    DEPENDS "KernelArchARM"
    DEFAULT_DISABLED OFF
)

config_option(
    ElfloaderRootserversLast ELFLOADER_ROOTSERVERS_LAST
    "Place the rootserver images at the end of memory"
    DEFAULT OFF
    # This setting does not work on EFI as paging structures are not
    # enabled to write to high memory
    DEPENDS "NOT ElfloaderImageEFI;"
    DEFAULT_DISABLED OFF
)

add_config_library(elfloader "${configure_string}")

add_compile_options(-D_XOPEN_SOURCE=700 -ffreestanding -Wall -Werror -W -Wextra)
set(linkerScript "${CMAKE_CURRENT_LIST_DIR}/src/arch-${KernelArch}/linker.lds")
if(KernelArchRiscV)
    add_compile_options(-mcmodel=medany)
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

MakeCPIO(archive.o "${cpio_files}" CPIO_SYMBOL _archive_start)

# If our platform has a YAML description, create a C header file to include
# information about the platform that is of interest to the ELF-loader, such as
# a physical memory map.
if(DEFINED platform_yaml)
    set(PLATFORM_SIFT "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-tool/helpers/platform_sift.py")
    set(PLATFORM_INFO_H "${CMAKE_CURRENT_BINARY_DIR}/gen_headers/platform_info.h")
    add_custom_command(
        OUTPUT ${PLATFORM_INFO_H}
        COMMAND ${PLATFORM_SIFT} --emit-c-syntax ${platform_yaml} > ${PLATFORM_INFO_H}
        VERBATIM
        DEPENDS ${platform_yaml}
    )
endif()

# Generate linker script
separate_arguments(c_arguments NATIVE_COMMAND "${CMAKE_C_FLAGS}")
add_custom_command(
    OUTPUT "linker.lds_pp"
    COMMAND
        ${CMAKE_C_COMPILER} "${c_arguments}"
        "-I$<JOIN:$<TARGET_PROPERTY:Configuration,INTERFACE_INCLUDE_DIRECTORIES>,;-I>" -P -E -o
        linker.lds_pp -x c ${linkerScript}
    DEPENDS Configuration ${linkerScript} ${PLATFORM_INFO_H}
    VERBATIM COMMAND_EXPAND_LISTS
)
add_custom_target(elfloader_linker DEPENDS linker.lds_pp)

add_executable(elfloader EXCLUDE_FROM_ALL ${files} archive.o)
target_include_directories(
    elfloader
    PRIVATE
        "include"
        "include/plat/${KernelPlatform}"
        "include/arch-${KernelArch}"
        "include/arch-${KernelArch}/${KernelWordSize}"
        "${CMAKE_CURRENT_BINARY_DIR}/gen_headers"
)
if(KernelArchARM)
    target_include_directories(
        elfloader
        PRIVATE
            "include/arch-${KernelArch}/armv/${KernelArmArmV}"
            "include/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}"
    )
endif()

target_link_libraries(elfloader PRIVATE Configuration cpio gcc)
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
else()
    if(KernelPlatImx6 OR KernelPlatformRpi3 OR KernelPlatformOdroidc2 OR KernelPlatformRockpro64)
        set(PlatformEntryAddr 0x20000000)
    elseif(
        KernelPlatformKZM
        OR KernelPlatformOMAP3
        OR KernelPlatformAM335X
        OR KernelPlatformImx7Sabre
        OR KernelPlatformTx2
        OR KernelPlatformFVP
    )
        set(PlatformEntryAddr 0x82000000)
    elseif(KernelPlatExynos5 OR KernelPlatformExynos4 OR KernelPlatformImx8mq-evk)
        set(PlatformEntryAddr 0x41000000)
    elseif(KernelPlatformHikey OR KernelPlatformTx1)
        if(KernelSel4ArchAarch64)
            set(PlatformEntryAddr 0)
        else()
            set(PlatformEntryAddr 0x1000)
        endif()
    elseif(KernelPlatformAPQ8064)
        set(PlatformEntryAddr 0x82008000)
    elseif(KernelPlatformTK1)
        set(PlatformEntryAddr 0x90000000)
    elseif(KernelPlatformZynq7000 OR KernelPlatformZynqmp)
        set(PlatformEntryAddr 0x10000000)
    elseif(
        KernelPlatformSpike
        OR KernelPlatformHifive
        OR KernelPlatformAriane
        OR KernelPlatformRocketChip
    )
        set(PlatformEntryAddr 0x80400000)
    else()
        if(KernelArchARM OR KernelArchRiscV)
            message(FATAL_ERROR "Unknown platform when generating image")
        endif()
    endif()
    set_property(TARGET elfloader PROPERTY PlatformEntryAddr ${PlatformEntryAddr})
    set_property(
        TARGET elfloader
        APPEND_STRING
        PROPERTY LINK_FLAGS " -Xlinker --section-start -Xlinker .start=${PlatformEntryAddr} "
    )
endif()
