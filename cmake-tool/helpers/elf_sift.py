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
Extract information of interest to the seL4 image build process from ELF files.
"""

import argparse
import elftools.elf.elffile
import sys


def get_aligned_size(n: int) -> int:
    """
    Return the smallest multiple of 4KiB not less than the total of the loadable
    segments.  (In other words, the size will be returned as-is if it is an
    exact multiple of 4KiB, otherwise it is rounded up to the next higher
    multiple of 4KiB.)
    """
    return n if n % 4096 == 0 else ((n // 4096) + 1) * 4096


def get_memory_size(filename: str, align: bool) -> int:
    """
    Return the size in bytes occuped in memory of the loadable ELF segments from
    the ELF object file `filename`.
    """

    total: int = 0

    with open(filename, 'rb') as f:
        elf = elftools.elf.elffile.ELFFile(f)

        # We only care about loadable segments (p_type is "PT_LOAD"), and we
        # want the size in memory of those segments (p_memsz), which can be
        # greater than the size in the file (p_filesz).  This is especially
        # important for the BSS section.  See elf(5).
        total = sum([seg['p_memsz'] for seg in elf.iter_segments()
                     if seg['p_type'] == 'PT_LOAD'])

        return get_aligned_size(total) if align else total


def main() -> int:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Extract information of interest to the seL4 image build process from ELF files.

We extract the sizes of loadable ELF segments from the ELF files given as
operands and print their sum.

If the "--align" flag is specified, the space "after" each ELF file is aligned
to the next 4KiB boundary, increasing the total.
""")
    parser.add_argument('elf_file', nargs='+', type=str,
                        help='ELF object file to examine')
    parser.add_argument('--align', action='store_true',
                        help='align to 4KiB between files')
    parser.add_argument('--reserve', metavar='BYTES', type=int, action='store',
                        default=0, help='number of additional bytes to reserve')
    args = parser.parse_args()
    regions = [get_memory_size(elf, args.align) for elf in args.elf_file]
    regions.append(args.reserve)
    total = sum(regions)

    if args.align:
        total = get_aligned_size(total)

    print(total)
    return 0


if __name__ == '__main__':
    sys.exit(main())
