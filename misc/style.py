#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
"""
run style tools over files
"""

import argparse
import fnmatch
import logging
import os
import re
import subprocess
import sys

from filter import parse_filters

# dict of style-tools to regex. The regex should match any full file path.
# if the regex matches, that tool will be applied to that file. Multiple
# tools can be defined for the same regex.
STYLE_MAP = {
    'style-c.sh': r'((\.c)|(\.h))$',
    'style-cmake.sh': r'((\.cmake)|(CMakeLists.txt))$',
    'style-py.sh': r'\.py$'
}


def main():
    parser = argparse.ArgumentParser("Run style updates on files.")
    parser.add_argument('-f', '--filters', type=str,
                        help='File with glob filters of files to filter')
    parser.add_argument('files', nargs='*', type=str,
                        help='List of files to run style updates on.')
    args = parser.parse_args()

    filters = parse_filters(args.filters)
    regexmap = {k: re.compile(v) for k, v in STYLE_MAP.items()}
    filemap = {k: [] for k, v in STYLE_MAP.items()}

    args.files = filter(os.path.isfile, args.files)
    # construct a list of files to pass to each tool
    for fname in args.files:
        # strip leading `./` which `find` produces, but stylefilters don't expect
        if fname[0:2] == './':
            fname = fname[2:]

        def matches(pattern, fname=fname):
            """filter any files that match the filter filters"""
            return fnmatch.fnmatch(fname, pattern)
        if not any(map(matches, filters)):
            for k, regex in regexmap.items():
                if regex.search(fname):
                    filemap.get(k, list()).append(fname)

    # now spawn processes to style all the files
    # this is currently done sequentially to collect output.
    # if this becomes a bottle neck we can consider spawning more processes
    return_code = 0
    for k, files in filemap.items():
        if files:
            script = os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), k)
            completed = subprocess.run([script] + files, stderr=subprocess.PIPE)
            if completed.returncode:
                logging.fatal("%s failed with error code %d\n%s", files,
                              completed.returncode, completed.stderr)
                return_code = completed.returncode
    return return_code


if __name__ == '__main__':
    sys.exit(main())
