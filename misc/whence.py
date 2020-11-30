#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#

'''
A tool for determining provenance.

Occasionally one encounters a directory of source code that was derived from an upstream repository
with history either squashed or discarded. To pull in upstream changes, it is desirable to know what
commit the source code originated from. This script helps you determine that by looking for the
upstream commit with the smallest diff to the downstream files. Sample usage:

    whence.py -u https://github.com/torvalds/linux --upstream-subdir scripts/kconfig \
              -d https://github.com/seL4/seL4_tools --downstream-subdir kbuild-tool/kconfig
'''

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


class GitRepo(object):
    def __init__(self, url):
        self.tmp = tempfile.mkdtemp()
        subprocess.check_call(['git', 'clone', url, self.tmp])

    def checkout(self, commit):
        subprocess.check_call(['git', 'checkout', commit], cwd=self.tmp)

    def log(self, subdir):
        # Reverse the commit list to test them chronologically, just for consistency.
        return reversed(subprocess.check_output(['git', 'log', '--pretty=tformat:%H', '.'],
                                                cwd=os.path.join(self.tmp, subdir)).split())

    def __del__(self):
        shutil.rmtree(self.tmp)


def main(argv):
    parser = argparse.ArgumentParser(description='locate a Git commit in an upstream project from '
                                     'which downstream source was derived')
    parser.add_argument('--upstream', '-u', required=True, help='URL of upstream repository to '
                        'search')
    parser.add_argument('--upstream-subdir', default='', help='subdirectory within upstream '
                        'repository to consider (root by default)')
    parser.add_argument('--downstream', '-d', required=True, help='URL of downstream repository '
                        'to analyse')
    parser.add_argument('--downstream-subdir', default='', help='subdirectory within downstream '
                        'repository to analyse (root by default)')
    parser.add_argument('--downstream-commit', help='commit in downstream repository to consider '
                        '(HEAD of master by default)')
    opts = parser.parse_args(argv[1:])

    sys.stderr.write('Cloning %s into a temporary directory...\n' % opts.upstream)
    try:
        upstream = GitRepo(opts.upstream)
    except subprocess.CalledProcessError:
        return -1

    sys.stderr.write('Cloning %s into a temporary directory...\n' % opts.downstream)
    try:
        downstream = GitRepo(opts.downstream)
    except subprocess.CalledProcessError:
        return -1

    if opts.downstream_commit is not None:
        sys.stderr.write('Updating downstream to %s...\n' % opts.downstream_commit)
        try:
            downstream.checkout(opts.downstream_commit)
        except subprocess.CalledProcessError:
            return -1

    sys.stderr.write('Retrieving candidate commit list...\n')
    try:
        commits = list(upstream.log(opts.upstream_subdir))
    except subprocess.CalledProcessError:
        return -1
    sys.stderr.write('%d commits to consider\n' % len(commits))

    # We now have everything we need. Examine each commit, tracking the smallest diff we've seen.

    min_diff = None
    min_commit = None

    for index, commit in enumerate(commits):
        sys.stderr.write('Considering %s (%d of %d)...\n' % (commit, index, len(commits)))
        try:
            upstream.checkout(commit)
        except subprocess.CalledProcessError:
            return -1
        src = os.path.join(upstream.tmp, opts.upstream_subdir)
        dst = os.path.join(downstream.tmp, opts.downstream_subdir)

        p = subprocess.Popen(['diff', src, dst], stdout=subprocess.PIPE)
        stdout, _ = p.communicate()
        diff = len(stdout.split('\n'))
        sys.stderr.write('This commit has a difference metric of %d\n' % diff)

        if min_diff is None or min_diff > diff:
            min_diff = diff
            min_commit = commit

    sys.stderr.write('The most likely commit is %s\n' % min_commit)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
