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

link_libraries(kernel_autoconf)
add_subdirectory(libutils)
add_subdirectory(libplatsupport)
add_subdirectory(libelf)
add_subdirectory(libcpio)
add_subdirectory(libpci)
add_subdirectory(libethdrivers)
add_subdirectory(liblwip)
add_subdirectory(libpicotcp)
add_subdirectory(libfdt)
