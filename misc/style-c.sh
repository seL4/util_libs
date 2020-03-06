#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Format (in place) a list of files as C code.
astyle --options="${0%/*}/astylerc" "$@"
REPO=$(git rev-parse --show-toplevel)
REPO=${REPO##*/}

if [ "$REPO" != "kernel" ] && [ "$REPO" != "seL4" ]
# we cannot use #pragma once in the kernel
then
    for f
    do
        python -m guardonce.guard2once -s "$f"
    done
fi
