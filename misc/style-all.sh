#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Run style tools on all dirs passed in, or the current dir.
if [ $# -eq 0 ]
then
    set -- "$(pwd)"
fi

for DIR
do
    find "$DIR" -type f | xargs "${0%/*}"/style.sh
done
