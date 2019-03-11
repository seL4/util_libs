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

cmake_minimum_required(VERSION 3.8.2)

include("${CMAKE_CURRENT_LIST_DIR}/helpers/application_settings.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/cakeml.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/cross_compiling.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/external-project-helpers.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/rust.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/dts.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/simulation.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/cpio.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/helpers/rootserver.cmake")
