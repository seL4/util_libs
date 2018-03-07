#
# Copyright 2018, Data61
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

project(libpci C)

add_library(pci STATIC EXCLUDE_FROM_ALL src/helper.c src/ioreg.c src/pci.c src/virtual_device.c src/virtual_pci.c)
target_include_directories(pci PUBLIC include)
target_link_libraries(pci Configuration muslc platsupport)
