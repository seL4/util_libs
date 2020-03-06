#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
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

if [ -d "$HOME/.sel4_cache" ]
then
    CACHE_DIR="$HOME/.sel4_cache"
else
    CACHE_DIR="$SCRIPT_PATH/.sel4_cache"
fi

if [ -e "$SCRIPT_PATH/CMakeLists.txt" ]
then
    # If we have a CMakeLists.txt in the top level project directory,
    # initialize CMake.
    cmake -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_PATH"/kernel/gcc.cmake -G Ninja "$@" \
        -DSEL4_CACHE_DIR="$CACHE_DIR" -C "$SCRIPT_PATH/settings.cmake" "$SCRIPT_PATH"
else
    # If we don't have a CMakeLists.txt in the top level project directory then
    # assume we use the project's directory tied to easy-settings.cmake and resolve
    # that to use as the CMake source directory.
    real_easy_settings="$(realpath $SCRIPT_PATH/easy-settings.cmake)"
    project_dir="$(dirname $real_easy_settings)"
    # Initialize CMake.
    cmake -G Ninja "$@" -DSEL4_CACHE_DIR="$CACHE_DIR" -C "$project_dir/settings.cmake" "$project_dir"
fi
