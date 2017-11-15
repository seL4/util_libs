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

set(configure_string "")

config_choice(ElfloaderImage ELFLOADER_IMAGE "Boot image type"
    "elf;ElfloaderImageELF;IMAGE_ELF;KernelArchARM"
    "binary;ElfloaderImageBinary;IMAGE_BINARY;KernelArchARM"
    "efi;ElfloaderImageEFI;IMAGE_EFI;KernelArchARM"
)

config_choice(ElfloaderMode ELFLOADER_MODE "seL4 mode"
    "secure supervisor;ElfloaderModeSSup;ARM_S_SUPERVISOR_MODE;KernelPlatImx6"
    "monitor;ElfloaderModeMonitor;ARM_MONITOR_MODE;KernelPlatformJetson OR KernelPlatImx6"
    "hypervisor;ElfloaderModeHyp;ARM_HYPERVISOR_MODE;KernelPlatformJetson"
    "non-secure supervisor;ElfloaderModeNSSup;ARM_NS_SUPERVISOR_MODE;KernelPlatformJetson OR KernelPlatImx6"
)

config_option(ElfloaderMonitorHook ARM_MONITOR_HOOK
    "Install SMC call handlers in monitor mode."
    DEFAULT OFF
    DEPENDS "KernelPlatformJetson"
)

config_option(ElfloaderGPTPtimersNSPL1Access GPT_PTIMER_NS_PL1_ACCESS
    "Enalbe the GPT physical timer access for NS PL1"
    DEFAULT ON
    DEPENDS "ElfloaderModeNSSup" DEFAULT_DISABLED OFF
)

config_option(ElfloaderErrata764369 ARM_ERRATA_764369
    "Work around for a Cortex-A9 errata. Derived from Linux kernel."
    DEFAULT ON
    DEPENDS "KernelArmCortexA9" DEFAULT_DISABLED OFF
)

add_config_library(elfloader "${configure_string}")

add_compile_options(-D_XOPEN_SOURCE=700 -ffreestanding -Wall -Werror -W -Wextra)

RequireFile(ElfloaderArchiveBinLds "archive.bin.lds" PATHS "${CMAKE_CURRENT_LIST_DIR}/src")
RequireFile(ElfloaderLinkerScript "linker.lds" PATHS "${CMAKE_CURRENT_LIST_DIR}/src/arch-arm")

file(GLOB files
    src/*.c
    src/arch-${KernelArch}/*.c
    src/plat/${KernelPlatform}/*.c
    src/binaries/elf/*.c
    src/arch-${KernelArch}/${KernelWordSize}/*.c
    src/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}/*.c
    src/plat/${KernelPlatform}/*.S
    src/arch-${KernelArch}/${KernelWordSize}/*.S
    src/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}/*.S
)

if(ElfloaderImageEFI)
    file(GLOB efi_files
        src/binaries/efi/*.c
    )
    list(APPEND files
        "src/binaries/efi/efi_head.S"
        ${efi_files}
    )
endif()

# Sort files to make build reproducible
list(SORT files)

# Define a custom way of linking that just does a partial link into an archive ready
# to be relinked with the kernel and user image to create a bootable elf
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> -r <OBJECTS> <LINK_LIBRARIES> <LINK_FLAGS> -lgcc \
     -o <TARGET>")
set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_ASM_COMPILER> <CMAKE_ASM_LINK_FLAGS> -r <OBJECTS> <LINK_LIBRARIES> <LINK_FLAGS> \
     -o <TARGET> -lgcc")

add_executable(elfloader EXCLUDE_FROM_ALL ${files})
target_include_directories(elfloader PRIVATE
    "include"
    "include/plat/${KernelPlatform}"
    "include/arch-${KernelArch}"
    "include/arch-${KernelArch}/${KernelWordSize}"
    "include/arch-${KernelArch}/armv/${KernelArmArmV}"
    "include/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}"
)
target_link_libraries(elfloader PRIVATE Configuration cpio)
