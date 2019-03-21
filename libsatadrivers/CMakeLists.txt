#
# Copyright 2019, DornerWorks
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
cmake_minimum_required(VERSION 3.7.2)

project(libsatadrivers C)

file(
    GLOB
        sources
        src/ahci.c
        src/ide.c
        src/common.c
)

add_library(satadrivers STATIC EXCLUDE_FROM_ALL ${sources})
target_include_directories(satadrivers PUBLIC include)
target_link_libraries(satadrivers muslc platsupport utils sel4)
