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

set(configure_string "")

config_choice(ElfloaderImage ELFLOADER_IMAGE "Boot image type"
    "elf;ElfloaderImageELF;IMAGE_ELF;KernelArchARM"
    "binary;ElfloaderImageBinary;IMAGE_BINARY;KernelArchARM"
    "efi;ElfloaderImageEFI;IMAGE_EFI;KernelArchARM"
)

config_choice(ElfloaderMode ELFLOADER_MODE "seL4 mode"
    "secure supervisor;ElfloaderModeSSup;ARM_S_SUPERVISOR_MODE;KernelPlatImx6"
    "monitor;ElfloaderModeMonitor;ARM_MONITOR_MODE;KernelPlatformTK1 OR KernelPlatImx6"
    "hypervisor;ElfloaderModeHyp;ARM_HYPERVISOR_MODE;KernelPlatformTK1"
    "non-secure supervisor;ElfloaderModeNSSup;ARM_NS_SUPERVISOR_MODE;KernelPlatformTK1 OR KernelPlatImx6"
)

config_option(ElfloaderMonitorHook ARM_MONITOR_HOOK
    "Install SMC call handlers in monitor mode."
    DEFAULT OFF
    DEPENDS "KernelPlatformTK1"
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

set(linkerScript "${CMAKE_CURRENT_LIST_DIR}/src/arch-arm/linker.lds")

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
    # We cannot control where EFI loads the image and so we must make it relocatable
    add_compile_options(-fpic)
else ()
    add_compile_options(-fno-pic)
endif()
add_compile_options(-fno-pie)

# Sort files to make build reproducible
list(SORT files)

MakeCPIO(archive.o "$<TARGET_FILE:kernel.elf>;$<TARGET_PROPERTY:rootserver_image,ROOTSERVER_IMAGE>" CPIO_SYMBOL _archive_start)

# Generate linker script
separate_arguments(c_arguments NATIVE_COMMAND "${CMAKE_C_FLAGS}")
add_custom_command(OUTPUT "linker.lds_pp"
    COMMAND ${CMAKE_C_COMPILER} "${c_arguments}" "-I$<JOIN:$<TARGET_PROPERTY:Configuration,INTERFACE_INCLUDE_DIRECTORIES>,;-I>"
        -P -E -o linker.lds_pp -x c ${linkerScript}
    DEPENDS Configuration ${linkerScript}
    VERBATIM
    COMMAND_EXPAND_LISTS
)
add_custom_target(elfloader_linker DEPENDS linker.lds_pp)

add_executable(elfloader EXCLUDE_FROM_ALL ${files} archive.o)
target_include_directories(elfloader PRIVATE
    "include"
    "include/plat/${KernelPlatform}"
    "include/arch-${KernelArch}"
    "include/arch-${KernelArch}/${KernelWordSize}"
    "include/arch-${KernelArch}/armv/${KernelArmArmV}"
    "include/arch-${KernelArch}/armv/${KernelArmArmV}/${KernelWordSize}"
)
target_link_libraries(elfloader PRIVATE Configuration cpio)
add_dependencies(elfloader elfloader_linker)
set_property(TARGET elfloader APPEND_STRING PROPERTY LINK_FLAGS " -T ${CMAKE_CURRENT_BINARY_DIR}/linker.lds_pp -nostartfiles -static -Wl,--build-id=none")

if (ElfloaderImageEFI)
    set_property(TARGET elfloader APPEND_STRING PROPERTY LINK_FLAGS " -pie ")
else()
    if(KernelPlatImx6 OR KernelPlatformRpi3)
        set(PlatformEntryAddr 0x20000000)
    elseif(KernelPlatformKZM OR KernelPlatformOMAP3 OR KernelPlatformAM335X OR KernelPlatformImx7Sabre)
        set(PlatformEntryAddr 0x82000000)
    elseif(KernelPlatExynos5 OR KernelPlatformExynos4)
        set(PlatformEntryAddr 0x41000000)
    elseif(KernelPlatformHikey OR KernelPlatformTx1)
        if (KernelSel4ArchAarch64)
            set(PlatformEntryAddr 0)
        else()
            set(PlatformEntryAddr 0x1000)
        endif()
    elseif(KernelPlatformAPQ8064)
        set(PlatformEntryAddr 0x82008000)
    elseif(KernelPlatformTK1)
        set(PlatformEntryAddr 0x90000000)
    elseif(KernelPlatformZynq7000)
        set(PlatformEntryAddr 0x10000000)
    else()
        if (KernelArchARM)
            message(FATAL_ERROR "Unknown platform when generating image")
        endif()
    endif()
    set_property(TARGET elfloader APPEND_STRING PROPERTY LINK_FLAGS " -Ttext=${PlatformEntryAddr} ")
endif()
