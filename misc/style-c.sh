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
# Format (in-place) a list of files as c-code.
astyle --options="${0%/*}/astylerc" "$@"
REPO=$(git rev-parse --show-toplevel)
REPO=${REPO##*/}
if [ "$REPO" != "kernel" -a "$REPO" != "seL4" ]
# we cannot use #pragma once in the kernel
then
    for f
    do
        python -m guardonce.guard2once -s "$f"
    done
fi
