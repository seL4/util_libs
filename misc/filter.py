#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#
import argparse
import fnmatch
import re
import sys

# never run the tools on anything git
DEFAULT_FILTERS = ["*.git/*"]

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
    parser = argparse.ArgumentParser("Filter files.")
    parser.add_argument('-f', '--filters', type=str,
                        help='File with glob filters of files')
    parser.add_argument('files', nargs='*', type=str,
                        help='List of files to be filtered')
    args = parser.parse_args()

    filters = parse_filters(args.filters)
    for fname in args.files:
        def matches(pattern, fname=fname):
            return fnmatch.fnmatch(fname, pattern)
        if not any(map(matches, filters)):
            print(fname)


if __name__ == '__main__':
    sys.exit(main())
