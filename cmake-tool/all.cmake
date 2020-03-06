#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.7.2)

# import some debug functions
include("${CMAKE_CURRENT_LIST_DIR}/helpers/debug.cmake")

# Include the base part of the build system and setup flags etc
include("${CMAKE_CURRENT_LIST_DIR}/base.cmake")

# Now that flags are setup include all the parts of the project
include("${CMAKE_CURRENT_LIST_DIR}/projects.cmake")
