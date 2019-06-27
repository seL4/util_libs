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

# import some debug functions
include("${CMAKE_CURRENT_LIST_DIR}/helpers/debug.cmake")

# Include the base part of the build system and setup flags etc
include("${CMAKE_CURRENT_LIST_DIR}/base.cmake")

# Now that flags are setup include all the parts of the project
include("${CMAKE_CURRENT_LIST_DIR}/projects.cmake")

# Should be done adding targets, can now generate the global configuration
include("${CMAKE_CURRENT_LIST_DIR}/configuration.cmake")
