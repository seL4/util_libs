#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
#

# Format (in place) a list of files as Python code.
autopep8 -i --max-line-length 100 "$@"
