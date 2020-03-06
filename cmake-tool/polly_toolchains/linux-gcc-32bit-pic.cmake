#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

include(${CMAKE_CURRENT_LIST_DIR}/FindPolly.cmake)

FindPolly()
include("${POLLY_DIR}/utilities/polly_init.cmake")

polly_init("Linux / gcc / PIC / c++11 support / 32 bit" "Ninja")

include("${POLLY_DIR}/utilities/polly_common.cmake")

include("${POLLY_DIR}/compiler/gcc.cmake")
include("${POLLY_DIR}/flags/cxx11.cmake")
include("${POLLY_DIR}/flags/fpic.cmake")
include("${POLLY_DIR}/flags/32bit.cmake")
