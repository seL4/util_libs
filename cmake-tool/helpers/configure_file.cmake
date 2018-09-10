# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)

cmake_minimum_required(VERSION 3.8.2)

if(NOT CONFIGURE_INPUT_FILE)
    message(FATAL_ERROR "CONFIGURE_INPUT_FILE not set.")
endif()

if(NOT CONFIGURE_OUTPUT_FILE)
    message(FATAL_ERROR "CONFIGURE_OUTPUT_FILE not set.")
endif()

if(NOT EXISTS "${CONFIGURE_INPUT_FILE}")
    message(FATAL_ERROR "Configure file: ${CONFIGURE_INPUT_FILE} does not exist")
endif()

configure_file(${CONFIGURE_INPUT_FILE} ${CONFIGURE_OUTPUT_FILE} @ONLY)
