#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libpci C)

set(configure_string "")

config_option(LibPCIDisplayFoundDevices PCI_DISPLAY_FOUND_DEVICES
    "Display found devices during a libpci_scan"
    DEFAULT OFF
)

add_config_library(pci "${configure_string}")

add_library(
    pci
    STATIC
    EXCLUDE_FROM_ALL
    src/helper.c
    src/ioreg.c
    src/pci.c
    src/virtual_device.c
    src/virtual_pci.c
)

target_include_directories(pci PUBLIC include)
target_link_libraries(pci muslc platsupport)
