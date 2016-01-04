#
# Copyright 2016, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

cmake_minimum_required (VERSION 2.6)
project (test-json)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -W -Wall -Wextra")

find_library (CUNIT NAMES cunit libcunit cunitlib)

# Make json.h findable.
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable (test-json test-json.c)
target_link_libraries (test-json ${CUNIT})
