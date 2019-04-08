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
