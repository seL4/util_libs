#
# Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

project(libfdt C)

file(GLOB deps *.c)
list(SORT deps)

add_library(fdt STATIC EXCLUDE_FROM_ALL ${deps})
target_include_directories(fdt PUBLIC include ".")
target_link_libraries(fdt muslc)
