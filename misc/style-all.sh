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

# Run style tools on all dirs passed in, or the current dir.
if [ -z "$@" ]
then
    DIRS=$(pwd)
else
    DIRS="$@"
fi

for d in "$DIRS"
do
    find "$d" -type f | xargs "${0%/*}"/style.sh
done
