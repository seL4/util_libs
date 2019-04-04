#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# This module declares a macro for debugging purposes.
# Simply place a call to set_break() in your build files and you will get a prompt

set(debug_list_dir "${CMAKE_CURRENT_LIST_DIR}")

macro(set_break)
    include(${debug_list_dir}/cmakerepl)
endmacro()
