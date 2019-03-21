#!/usr/bin/env python3
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

# never run the tools on anything git
DEFAULT_FILTERS = ["*.git/*"]
# dict of style-tools to regex. The regex should match any full file path.
# if the regex matches, that tool will be applied to that file. Multiple
# tools can be defined for the same regex.
STYLE_MAP = {
    'style-c.sh':r'((\.c)|(\.h))$',
    'style-cmake.sh':r'((\.cmake)|(CMakeLists.txt))$'
}

# support comments alone and trailing (comment with #)
COMMENT = re.compile(r'^\s*#')
TRAILING_COMMENT = re.compile(r'\s+#.*$')

def parse_filter(line: str):
    """Parse a filter from a line of a filter file"""
    line = line.strip()
    if line and not COMMENT.search(line):
        return TRAILING_COMMENT.sub('', line)
    return None

def parse_filters(filters_file: str):
    """Parse the filter file, returning a list of filters"""
    filters = DEFAULT_FILTERS
    if filters_file:
        try:
            with open(filters_file, 'r') as outfile:
                lines = outfile.readlines()
                filters += [parse_filter(l) for l in lines if parse_filter(l)]
        except IOError as exception:
            logging.warning("Failed to open filter file %s: %s", filters_file, exception)
    return filters

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

    # construct a list of files to pass to each tool
    for fname in args.files:
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
    for k, files in filemap.items():
        if files:
            script = os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), k)
            completed = subprocess.run([script] + files, stderr=subprocess.PIPE)
            if completed.returncode:
                logging.fatal("%s failed with error code %d\n%s", files,
                              completed.returncode, completed.stderr)

if __name__ == '__main__':
    sys.exit(main())
