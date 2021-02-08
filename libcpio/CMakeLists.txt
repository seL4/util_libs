#
# Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

project(libcpio C)

add_library(cpio EXCLUDE_FROM_ALL src/cpio.c)
if(KernelArchRiscV)
    target_compile_options(cpio PRIVATE "-mcmodel=medany")
endif()

target_compile_options(cpio PRIVATE "-fpic")

target_include_directories(cpio PUBLIC include)
