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

cmake_minimum_required(VERSION 3.8.2)

project(liblwip C)

set(configure_string "")

config_option(LibLwip LIB_LWIP "Build LwIP" DEFAULT OFF)
mark_as_advanced(LibLwip)
add_config_library(lwip "${configure_string}")
if(LibLwip)
    add_compile_options(-std=gnu99)

    file(
        GLOB
            sources
            lwip-1.4.1/src/*/*.c
            lwip-1.4.1/src/core/snmp/*.c
            lwip-1.4.1/src/core/ipv4/*.c
    )

    add_library(lwip STATIC EXCLUDE_FROM_ALL ${sources})

    target_include_directories(
        lwip
        PUBLIC
            lwip-1.4.1/src/include
            lwip-1.4.1/src/include/ipv4
            lwip-1.4.1/src/include/netif
            include
            include/lwip
    )

    target_link_libraries(lwip muslc liblwip_config lwip_Config)
endif()
