#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

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
