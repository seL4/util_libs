#!/usr/bin/env python3
#
# Copyright 2019, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the GNU General Public License version 2. Note that NO WARRANTY is provided.
# See "LICENSE_GPLv2.txt" for details.
#
# @TAG(DATA61_GPL)
#
"""
Extract information of interest to the seL4 image build process from the
`platform_gen.yaml` file.

THIS IS NOT A STABLE API.  Use as a script, not a module.
"""

import argparse
import sys
import yaml

from typing import Any, Dict, List, Tuple

program_name = sys.argv[0]


# You can run the doctests with `python3 -m doctest $THIS_FILE`.
def is_valid(data: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """
    Verify that the `data` (which should be obtained from a YAML file using
    `load_data()` contains a well-formed List of disjunct memory regions ordered
    by increasing addresses.

    Returns a tuple of a `bool` and a list of strings.  The list is empty if
    there were no problems, and describes one validation issue per element
    otherwise.

    >>> is_valid(None)
    (False, ['no data in file'])
    >>> is_valid({'devices': [{'end': 9699328, 'start': 9437184}]})
    (False, ['no description of memory in file (no "memory" key)'])
    >>> is_valid({'memory': 1})
    (False, ['bad description of memory in file ("memory" is not a list)'])
    >>> is_valid({'memory': []})
    (False, ['memory described as empty in file (list is zero-length)'])
    >>> is_valid({'memory': [{'end': 1342177280, 'start': 268435456}]})
    (True, [])
    >>> is_valid({'memory': [{'end': 1342177280}]})
    (False, ['region 0 is missing its start bound'])
    >>> is_valid({'memory': [{'start': 268435456}]})
    (False, ['region 0 is missing its end bound'])
    >>> is_valid({'memory': [{'junk': 'foo'}]})
    (False, ['region 0 is missing its start bound', 'region 0 is missing its end bound'])
    >>> is_valid({'memory': [{'start': 'foo'}]})
    (False, ['region start "foo" is not an integer', 'region 0 is missing its end bound'])
    >>> is_valid({'memory': [{'start': 'foo', 'end': 'bar'}]})
    (False, ['region start "foo" is not an integer', 'region end "bar" is not an integer'])
    >>> is_valid({'memory': [{'start': 2048, 'end': 1024}]})
    (False, ['region bounds are not in strictly increasing order (1024 not > 2048)'])
    >>> is_valid({'memory': [{'end': 4095, 'start': 0}, {'end': 65535, 'start': 32768}, {'end': 1342177280, 'start': 268435456}]})
    (True, [])
    >>> is_valid({'memory': [{'end': 4095, 'start': 0}, {'end': 65535, 'start': 32768}, {'end': 1342177280, 'start': 268435456}, {'end': 16384, 'start': 32768}]})
    (False, ['region bounds are not in strictly increasing order (32768 not > 1342177280)', 'region bounds are not in strictly increasing order (16384 not > 1342177280)'])
    """
    problems = []

    if data is None:
        problems.append('no data in file')
    elif 'memory' not in data:
        problems.append('no description of memory in file (no "memory" key)')
    elif not isinstance(data['memory'], list):
        problems.append('bad description of memory in file'
                        ' ("memory" is not a list)')
    elif len(data['memory']) == 0:
        problems.append('memory described as empty in file'
                        ' (list is zero-length)')
    else:
        # The initialization of last_seen_bound works with the "increasing
        # bounds" comparison below to require that all addresses be nonnegative.
        last_seen_bound = -1
        region_counter = 0

        for region in data['memory']:
            for bound in ('start', 'end'):
                if bound not in region:
                    problems.append('region {n} is missing its {name} bound'
                                    .format(n=region_counter, name=bound))
                elif not isinstance(region[bound], int):
                    problems.append('region {name} "{value}" is not an integer'
                                    .format(name=bound, value=region[bound]))
                elif not region[bound] > last_seen_bound:
                    problems.append('region bounds are not in strictly'
                                    ' increasing order ({this} not > {last})'
                                    .format(this=region[bound],
                                            last=last_seen_bound))
                else:
                    last_seen_bound = region[bound]

            region_counter += 1

    if problems:
        return (False, problems)

    return (True, [])


def report(data=None, c_symbols: Dict[str, str] = {}, use_c=False) -> str:
    """
    Return a (typically multi-line) string with information about memory regions
    described in `data`.  The string is empty if `is_valid()` rejects the data.

    The default string contents are human-readable; if `use_c` is `True`, C
    syntax is emitted instead.  The `c_symbols` dict describes the C symbol
    names to be emitted.
    """
    if not is_valid(data):
        return ''

    n = len(data['memory'])

    if use_c:
        # Extract C symbol names from the dict for convenience.
        (array, length, tag) = (
            c_symbols['array_symbol'],
            c_symbols['array_length_symbol'],
            c_symbols['structure_tag_symbol']
        )

        # We want to mark generated code with a comment.  For best comprehension
        # (by the reader of the generated code), we want to clearly indicate (1)
        # what generated the code and (2) where the generated section begins and
        # ends.  We also want the comments to otherwise be as similar as
        # possible to facilitate any desired post-processing.  To avoid
        # repeating ourselves here (in Python), we generate a _template_
        # string containing a C comment with the name of the generating program
        # embedded.  The tag ("BEGIN" or "END") is then expanded when written to
        # the appropriate place in the generated code.
        comment_template = '/* generated by {} {{tag}} */'.format(program_name)
        head = '''{comment_begin}
int {length} = {n};

struct {tag} {{
    size_t start;
    size_t end;
}} {array}[{n}] = {{
'''.format(comment_begin=comment_template.format(tag='BEGIN'),
           length=length,
           tag=tag,
           array=array,
           n=n)
        regions = []

        for r in range(n):
            regions.append('''\t{{ .start = {start}, .end = {end} }},\
'''.format(start=data['memory'][r]['start'], end=data['memory'][r]['end']))

        body = '\n'.join(regions)
        tail = '\n}};\n{}'''.format(comment_template.format(tag='END'))
        report = '{head}{body}{tail}'.format(head=head, body=body, tail=tail)
    else:
        head = 'number of memory regions: {}\n'.format(n)
        regions = []

        for r in range(n):
            regions.append('''region {r}:
\tstart: {start}
\tend: {end}'''.format(r=r, start=data['memory'][r]['start'],
                       end=data['memory'][r]['end']))

        report = '{head}{body}'.format(head=head, body='\n'.join(regions))

    return report


def load_data(yaml_filename: str) -> Dict[str, Any]:
    """
    Call `yaml_load()` (from `pyyaml`) on `yaml_filename` and return a Dict
    containing what was found there.
    """
    with open(yaml_filename, 'r') as f:
        data = yaml.load(f)

    return data


def _process_operand(yaml_file: str, c_symbols: Dict[str, str],
                     use_c: bool) -> bool:
    """
    Handle one non-optional command-line argument; called by `main()`.
    """
    data = load_data(yaml_file)
    (is_good_data, problems) = is_valid(data)

    if is_good_data:
        print(report(data, c_symbols, use_c=use_c))
    else:
        # Set up a prefix for diagnostic messages.  Diagnostics should always
        # identify who is talking (`program_name`) and if operating on a file,
        # should name the file in which trouble is encountered.  Both of these
        # make grep more effective.
        prefix = "{pn}: file \"{fn}\":".format(pn=program_name, fn=yaml_file)

        if len(problems) == 1:
            sys.stderr.write("{} {}\n".format(prefix, problems[0]))
        else:
            sys.stderr.write("{} has multiple problems:\n".format(prefix))
            [sys.stderr.write('{}\t{}\n'.format(prefix, p)) for p in problems]

        return False

    return True


def main() -> int:
    """
    Executable entry point.
    """
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Extract information of interest to the seL4 image build process from one or more
files generated by `platform_gen.yaml`.

If a YAML file lacks a description of memory, or fails to parse, a diagnostic is
emitted and an exit status of 1 returned.  Exit status 2 indicates a problem
while attempting to parse arguments.

Note that when `--emit-c-syntax` is specified, C99 designated initialisers are
used in the generated code.  This code can be used directly (e.g., inside a
function body) or in a header file.

An example of usage follows.  Note the symbol names used, including those of the
structure members.  An array of structures is always used, even if there is only
one region and therefore array element.  The length of the array is explicitly
exposed, rather than using values like "NULL, NULL" to mark the end of the list.

```
#include "output_of_this_tool.h"

int main(int argc, char *argv[]) {
    for (int i = 0; i < num_memory_regions; i++) {
        (void) printf("memory region %d: 0x%08lx - 0x%08lx\\n",
                      i, memory_region[i].start, memory_region[i].end);
    }
}
```
""")
    parser.add_argument('platform_file', nargs='+', type=str,
                        help='YAML description of platform parameters')
    parser.add_argument('--emit-c-syntax', action='store_true',
                        help='emit C syntax instead of human-readable output')
    parser.add_argument('--array_symbol', type=str,
                        default='memory_region',
                        help='desired C identifier for struct array')
    parser.add_argument('--array_length_symbol', type=str,
                        default='num_memory_regions',
                        help='desired C identifier for length of struct array')
    parser.add_argument('--structure_tag_symbol', type=str,
                        default='memory_region',
                        help='desired C identifier for structure tag')
    args = parser.parse_args()
    there_was_any_trouble = False

    c_symbols = {
        'array_symbol': args.array_symbol,
        'array_length_symbol': args.array_length_symbol,
        'structure_tag_symbol': args.structure_tag_symbol,
    }

    for yaml_file in args.platform_file:
        if not _process_operand(yaml_file, c_symbols, use_c=args.emit_c_syntax):
            there_was_any_trouble = True

    return 1 if there_was_any_trouble else 0


if __name__ == '__main__':
    sys.exit(main())
