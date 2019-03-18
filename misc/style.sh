#!/bin/sh

#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

TOOLS_DIR=`dirname "$0"`

for i in "$@"
do
case $i in
    -s=*|--source-dir=*)
    SOURCE_DIR="${i#*=}"
    shift
    ;;
    *)
    ;;
esac
done

# Default to current directory if no directory is passed
if [ -z "$SOURCE_DIR" ]; then
    SOURCE_DIR=`pwd`
fi

echo "Styling directory: $SOURCE_DIR"
find "${SOURCE_DIR}" -name '*.[ch]' -type f | xargs astyle --options="$TOOLS_DIR"/astylerc
