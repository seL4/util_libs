#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
# Copyright 2022, Capgemini Engineering
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libplatsupport C)

# HACK: This is a stop-gap measure for now to allow projects that don't
# use the CMake modules feature of our build system to build properly.
# Some projects in particular that need to be fixed:
#   - rumprun-sel4-demoapps
if(NOT ${PLATSUPPORT_HELPERS} STREQUAL "")
    include(${PLATSUPPORT_HELPERS})
endif()

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
    "com2;LibPlatSupportX86ConsoleDeviceCom2;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM2;KernelPlatPC99"
    "com3;LibPlatSupportX86ConsoleDeviceCom3;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM3;KernelPlatPC99"
    "com4;LibPlatSupportX86ConsoleDeviceCom4;LIB_PLAT_SUPPORT_SERIAL_PORT_X86_COM4;KernelPlatPC99"
    "ega;LibPlatSupportX86ConsoleDeviceEGA;LIB_PLAT_SUPPORT_SERIAL_TEXT_EGA;KernelPlatPC99"
)

config_choice(
    LibPlatSupportLPTMRclock
    LIB_PLAT_SUPPORT_LPTMR_CLOCK
    "Which input clock to use for the Low Power Timer\
    32.768 kHz -> High accuracy, but low precision.\
    24 MHz -> Higher precision, but lower accuracy. Synchronous with the scheduling clock."
    "32kHz;LibPlatSupportLPTMRclock24MHz;LIB_PLAT_SUPPORT_LPTMR_CLOCK_32kHZ;KernelPlatformIMX93"
    "24MHz;LibPlatSupportLPTMRclock32kHz;LIB_PLAT_SUPPORT_LPTMR_CLOCK_24MHz;KernelPlatformIMX93"
)

mark_as_advanced(CLEAR LibPlatSupportX86ConsoleDevice LibPlatSupportLPTMRclock)

# Some platforms don't have a platform timer.
if(
    (KernelPlatformQEMUArmVirt AND NOT (KernelArmExportPCNTUser AND KernelArmExportPTMRUser))
    OR KernelPlatformRocketchip
    OR KernelPlatformRocketchipZCU102
    OR KernelPlatformCheshire
    OR (SIMULATION AND (KernelArchRiscV OR KernelArchARM))
)
    set(LibPlatSupportHaveTimer OFF)
else()
    set(LibPlatSupportHaveTimer ON)
endif()

config_option(
    LibPlatSupportHaveTimer
    LIB_PLAT_SUPPORT_HAVE_TIMER
    "Indicates that this platform has support for platform ltimers"
    DEFAULT
    ON
)

mark_as_advanced(LibPlatSupportHaveTimer)

set(LibPlatSupportMach "")
if(KernelPlatformRpi3 OR KernelPlatformRpi4)
    set(LibPlatSupportMach "bcm")
elseif(KernelPlatformCheshire OR KernelPlatformAriane)
    set(LibPlatSupportMach "cva6")
elseif(KernelPlatformHifiveP550)
    set(LibPlatSupportMach "eswin")
elseif(NOT ${KernelArmMach} STREQUAL "")
    # falling back to kernel settings is done to keep legacy compatibility
    set(LibPlatSupportMach "${KernelArmMach}")
endif()

file(
    GLOB
        deps
        src/plat/${KernelPlatform}/*.c
        src/*.c
        src/plat/${KernelPlatform}/acpi/*.c
)

if(NOT ${LibPlatSupportMach} STREQUAL "")
    file(GLOB lib_deps "src/mach/${LibPlatSupportMach}/*.c")
    list(APPEND deps ${lib_deps})
endif()

if(KernelArchARM)
    list(APPEND deps src/arch/arm/clock.c)
    list(APPEND deps src/arch/arm/delay.c)
    list(APPEND deps src/arch/arm/dma330.c)
    list(APPEND deps src/arch/arm/i2c.c)
    list(APPEND deps src/arch/arm/i2c_bitbang.c)
    list(APPEND deps src/arch/arm/generic_timer.c)
    list(APPEND deps src/arch/arm/irqchip/gic.c)
    list(APPEND deps src/arch/arm/irqchip/tegra.c)
    list(APPEND deps src/arch/arm/irqchip/gicv3.c)
    list(APPEND deps src/arch/arm/irqchip/omap3.c)
    # Link the IRQ chip parser modules
    list(
        APPEND
            irqchip_modules
            "-Wl,--undefined=arm_gic_ptr,--undefined=tegra_ictlr_ptr,--undefined=arm_gicv3_ptr,\
--undefined=fsl_avic_ptr,--undefined=ti_omap3_ptr"
    )
elseif(KernelArchX86)
    list(APPEND deps src/arch/x86/delay.c)
    list(APPEND deps src/arch/x86/tsc.c)
elseif(KernelArchRiscV)
    list(APPEND deps src/arch/riscv/irqchip/plic.c)
    list(APPEND irqchip_modules "-Wl,--undefined=riscv_plic_ptr")
else()
    message(FATAL_ERROR "Unsupported KernelArch '${KernelArch}'")
endif()

if(KernelPlatformQEMUArmVirt OR KernelPlatformQuartz64 OR KernelPlatformIMX93)
    if(KernelArmExportPCNTUser AND KernelArmExportPTMRUser)
        list(APPEND deps src/arch/arm/generic_ltimer.c)
    endif()
endif()

if(KernelPlatformExynos5422)
    list(APPEND deps src/mach/${LibPlatSupportMach}/clock/exynos_5422_clock.c)
elseif(KernelPlatformExynos4 OR KernelPlatformExynos5410 OR KernelPlatformExynos5250)
    list(APPEND deps src/mach/${LibPlatSupportMach}/clock/exynos_common_clock.c)
endif()

if(KernelPlatImx6)
    list(APPEND deps src/mach/${LibPlatSupportMach}/epit/epit.c)
endif()

if(
    KernelPlatImx6
    OR KernelPlatImx7
    OR KernelPlatformImx8mq-evk
    OR KernelPlatformImx8mm-evk
    OR KernelPlatformImx8mp-evk
    OR KernelPlatformMaaxboard
)
    list(APPEND deps src/mach/${LibPlatSupportMach}/serial/serial.c)
endif()

if(KernelPlatImx8mq OR KernelPlatformImx8mm-evk OR KernelPlatformImx8mp-evk)
    list(APPEND deps src/plat/imx8m/chardev.c)
    # There's no clock driver at the moment, but this is to allow the
    # libethdrivers to build for imx8mq
    list(APPEND deps src/plat/imx8m/clock.c)
endif()

if(KernelPlatPC99)
    set_source_files_properties(
        src/plat/pc99/keyboard_vkey.c
        PROPERTIES
        COMPILE_FLAGS
        -Wno-initializer-overrides
    )
endif()

list(SORT deps)

add_config_library(platsupport "${configure_string}")

add_library(platsupport EXCLUDE_FROM_ALL ${deps})

if(KernelPlatImx8mq OR KernelPlatformImx8mm-evk OR KernelPlatformImx8mp-evk)
    if(KernelPlatformImx8mp-evk)
        # For the imx8mp, the DTS is in a different format compared to those of
        # the imx8mm and imx8mq, there are some constants in the headers that
        # point to the proper path
        target_compile_definitions(platsupport PRIVATE USE_IMX8MP_GPT_PATH)
    endif()
    target_include_directories(platsupport PUBLIC plat_include/imx8m)
endif()

if(KernelPlatformZynqmp)
    # This still needs to be cleaned up, so there is a mach folder "zynqmp" and
    # the board specific folders "zcu102" and "ultra96". Then we don't need to
    # include this folder here explicitly
    target_include_directories(platsupport PUBLIC plat_include/zynqmp)
endif()

if(NOT "${LibPlatSupportMach}" STREQUAL "")
    target_include_directories(platsupport PUBLIC mach_include/${LibPlatSupportMach})
endif()

# special handling for "arm_hyp", it's an add-on-hack over "aarch32"
set(_inc_folder_KernelSel4Arch "${KernelSel4Arch}")
if("${KernelSel4Arch}" STREQUAL "arm_hyp")
    set(_inc_folder_KernelSel4Arch "aarch32")
endif()

target_include_directories(
    platsupport
    PUBLIC
        include
        plat_include/${KernelPlatform}
        sel4_arch_include/${_inc_folder_KernelSel4Arch}
        arch_include/${KernelArch}
)

target_link_libraries(
    platsupport
    muslc
    utils
    fdt
    sel4_autoconf
    platsupport_Config
    ${irqchip_modules}
)
