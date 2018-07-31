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

include(${CMAKE_CURRENT_LIST_DIR}/FindPolly.cmake)

FindPolly()
include("${POLLY_DIR}/utilities/polly_init.cmake")

polly_init(
    "Linux / gcc / PIC / c++11 support / 32 bit"
    "Ninja"
)

include("${POLLY_DIR}/utilities/polly_common.cmake")

include("${POLLY_DIR}/compiler/gcc.cmake")
include("${POLLY_DIR}/flags/cxx11.cmake")
include("${POLLY_DIR}/flags/fpic.cmake")
include("${POLLY_DIR}/flags/32bit.cmake")

