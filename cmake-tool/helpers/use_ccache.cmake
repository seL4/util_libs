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

macro(use_ccache)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(DIRECTORY PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(DIRECTORY PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
    mark_as_advanced(CCACHE_FOUND)
endmacro()
