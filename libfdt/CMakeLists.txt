#
# Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libfdt C)

add_library(
    fdt
    STATIC
    EXCLUDE_FROM_ALL
    "fdt.c"
    "fdt_addresses.c"
    "fdt_empty_tree.c"
    "fdt_overlay.c"
    "fdt_ro.c"
    "fdt_rw.c"
    "fdt_strerror.c"
    "fdt_sw.c"
    "fdt_wip.c"
    "fdtget.c"
)

target_include_directories(fdt PUBLIC ".")

target_link_libraries(fdt muslc)
