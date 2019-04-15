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

project(libplatsupport C)

set(configure_string "")

config_choice(
    LibPlatSupportX86ConsoleDevice
    LIB_PLAT_SUPPORT_X86_CONSOLE_DEVICE
    "X86 console device. The serial port that libplatsupport will use for communication \
        com1 -> 0x3f8 \
        com2 -> 0x2f8 \
        com3 -> 0x3e8 \
        com4 -> 0x2e8 \
        ega -> 80x25 text mode ega screen"
    "com1;LibPlatSupportX86ConsoleDeviceCom1;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM1;KernelPlatPC99"
    "com2;LibPlatSupportX86ConsoleDeviceCom2;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM1;KernelPlatPC99"
    "com3;LibPlatSupportX86ConsoleDeviceCom3;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM1;KernelPlatPC99"
    "com4;LibPlatSupportX86ConsoleDeviceCom4;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM1;KernelPlatPC99"
    "ega;LibPlatSupportX86ConsoleDeviceEGA;LIB_PLAT_SUPPORT_SERIAL_TEXT_EGA;KernelPlatPC99"
)

file(
    GLOB
        deps
        src/mach/${KernelArmMach}/*.c
        src/plat/${KernelPlatform}/*.c
        src/arch/${KernelArch}/*.c
        src/*.c
        src/plat/${KernelPlatform}/acpi/*.c
)

if(KernelPlatformExynos5422)
    list(APPEND deps src/mach/${KernelArmMach}/clock/exynos_5422_clock.c)
elseif(KernelPlatformExynos4 OR KernelPlatformExynos5410 OR KernelPlatformExynos5250)
    list(APPEND deps src/mach/${KernelArmMach}/clock/exynos_common_clock.c)
endif()

list(SORT deps)

add_config_library(platsupport "${configure_string}")

add_library(platsupport EXCLUDE_FROM_ALL ${deps})
target_include_directories(platsupport PRIVATE src/plat_include/${KernelPlatform})
target_include_directories(
    platsupport
    PUBLIC include plat_include/${KernelPlatform} arch_include/${KernelArch}
)
if(NOT "${KernelArmMach}" STREQUAL "")
    target_include_directories(platsupport PUBLIC mach_include/${KernelArmMach})
endif()
target_link_libraries(platsupport Configuration muslc utils)
