#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libpci C)

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
