#!/bin/sh
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

# Run the style tools on all changed files in the current repository.
git ls-files -mo | xargs "${0%/*}"/style.sh
