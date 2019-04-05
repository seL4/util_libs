#!/bin/sh

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

# Style an input list of files as cmake files.

# Pass the cmake format as args such that cmake-format.py can be used
# to provide definitions for custom function formatting.
CMAKE_FMT="--line-width 100 \
           --tab-size 4 \
           --max-subargs-per-line 3 \
           --separate-ctrl-name-with-space False \
           --separate-fn-name-with-space False \
           --dangle-parens True \
           --command-case unchanged \
           --keyword-case unchanged \
           --enable-markup False"
cmake-format -i $CMAKE_FMT "$@"
