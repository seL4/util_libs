#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.16.0)

# Hook for CAmkES build system. This allows CAmkES projects to
# force a particular rootserver location.
function(SetElfloaderRootserversLast)
    set(ElfloaderRootserversLast ON CACHE BOOL "" FORCE)
endfunction()
