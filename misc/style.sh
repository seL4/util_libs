#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

set -efu

# A repo can keep its local style filter in its top-level directory.
LOCAL_FILTER=.stylefilter

if [ -f $LOCAL_FILTER ]
then
    FILTER_ARG="-f $LOCAL_FILTER"
else
    FILTER_ARG=
fi

# Run style tools over list of files passed as input.
"${0%/*}"/style.py $FILTER_ARG "$@"
