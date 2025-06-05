#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
#

# Style an input list of files as cmake files.

PROGNAME=${0##*/}

# Pass the cmake format as args such that cmake-format.py can be used
# to provide definitions for custom function formatting.
CMAKE_FMT="--line-width 100 \
           --tab-size 4 \
           --separate-ctrl-name-with-space False \
           --separate-fn-name-with-space False \
           --dangle-parens True \
           --command-case unchanged \
           --keyword-case unchanged \
           --enable-markup False"

# cmake-format sends its version info to standard error.  :-/
CF_VERSION=$(cmake-format --version 2>&1)
DESIRED_VERSION=0.6.13

case "$CF_VERSION" in
    ("")
        echo "$PROGNAME: fatal error: no output from \"cmake-format --version\""
        exit 2
        ;;
    ($DESIRED_VERSION)
        # Good version; proceed.
        ;;
    (*)
        echo "$PROGNAME: fatal error: need version $DESIRED_VERSION of" \
            "cmake-format; $CF_VERSION is installed"
        exit 2
        ;;
esac

cmake-format -i $CMAKE_FMT "$@"
