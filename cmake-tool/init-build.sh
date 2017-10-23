#!/bin/bash
#
# Copyright 2017, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#

# This script is intended to be symlinked into the same location as your root CMakeLists.txt file
# and then invoked from a clean build directory

set -e

# Determine script path (do not resolve symbolic links, we want it as given by the user)
pushd . > /dev/null
SCRIPT_PATH="${BASH_SOURCE[0]}"
cd `dirname ${SCRIPT_PATH}`
SCRIPT_PATH=`pwd`;
popd > /dev/null

# Ensure script path and current working directory are not the same
if [ "`pwd`" = "${SCRIPT_PATH}" ]
then
    echo "${BASH_SOURCE[0]} should not be invoked from build directory":
    exit -1
fi

# Try and make sure we weren't invoked from a source directory by checking for a CMakeLists.txt file
if [ -e "`pwd`/CMakeLists.txt" ]
then
    echo "${BASH_SOURCE[0]} should be invoked from build directory and not source directories containing a CMakeLists.txt file"
    exit -1
fi

# Initialize cmake
cmake -DCMAKE_TOOLCHAIN_FILE=${SCRIPT_PATH}/kernel/gcc.cmake -G Ninja $@ ${SCRIPT_PATH} && cmake ${SCRIPT_PATH} && cmake ${SCRIPT_PATH}
