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

# This script is intended to be symlinked into the same location as your root
# CMakeLists.txt file and then invoked from a clean build directory.

set -eu

# Determine path to this script (fast, cheap "dirname").
SCRIPT_PATH=${0%/*}
# Save script name for diagnostic messages (fast, cheap "basename").
SCRIPT_NAME=${0##*/}

# Ensure script path and current working directory are not the same.
if [ "$PWD" = "$SCRIPT_PATH" ]
then
    echo "\"$SCRIPT_NAME\" should not be invoked from top-level directory" >&2
    exit 1
fi

# Try and make sure we weren't invoked from a source directory by checking for a
# CMakeLists.txt file.
if [ -e CMakeLists.txt ]
then
    echo "\"$SCRIPT_NAME\" should be invoked from a build directory and not" \
        "source directories containing a CMakeLists.txt file" >&2
    exit 1
fi

# Initialize CMake.
#
# We run cmake multiple times because project-specific mandatory settings are
# sometimes buried deep in the tree of cmake files, and might get dereferenced
# before they are used.  CMake's caching stabilizes the situation, at the cost
# of requiring multiple runs.  Fixing this infelicity is JIRA SELFOUR-1648.
cmake -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_PATH"/kernel/gcc.cmake -G Ninja "$@" \
    "$SCRIPT_PATH" && cmake "$SCRIPT_PATH" && cmake "$SCRIPT_PATH"
