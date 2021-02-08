#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

function(AddLWIPConfiguration include_dir)
    if(TARGET liblwip_config)
        message(FATAL_ERROR "liblwip configuration already declared")
    endif()
    add_library(liblwip_config INTERFACE)
    target_include_directories(liblwip_config INTERFACE ${include_dir})
endfunction()
