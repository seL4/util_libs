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

# This file is guaranteed to exist
find_file(PKG_CONFIG_TEMPLATE pkg-config.in PATHS ${CMAKE_CURRENT_LIST_DIR} CMAKE_FIND_ROOT_PATH_BOTH)
function(CreatePkgConfigExecForDir exec_name sysroot)
	configure_file(${PKG_CONFIG_TEMPLATE} ${exec_name} @ONLY)
endfunction()
