#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#
"""
Extract information of interest to the seL4 image build process from ELF files.

THIS IS NOT A STABLE API.  Use as a script, not a module.
"""

import argparse
import elftools.elf.elffile
import sys

from typing import BinaryIO


def get_aligned_size(n: int) -> int:
    """
    Return the smallest multiple of 4KiB not less than the total of the loadable
    segments.  (In other words, the size will be returned as-is if it is an
    exact multiple of 4KiB, otherwise it is rounded up to the next higher
    multiple of 4KiB.)
    """
    return n if n % 4096 == 0 else ((n // 4096) + 1) * 4096


def get_memory_usage(elf_file: BinaryIO, align: bool) -> int:
    """
    Return the size in bytes occuped in memory of the loadable ELF segments from
    the ELF object file `elf_file`.
    """

    elf = elftools.elf.elffile.ELFFile(elf_file)

    # We only care about loadable segments (p_type is "PT_LOAD"), and we
    # want the size in memory of those segments (p_memsz), which can be
    # greater than the size in the file (p_filesz).  This is especially
    # important for the BSS section.  See elf(5).

    # There may be gaps between segments; use the min+max vaddr of
    # the loaded segments to calculate total usage.
    min_vaddr = None
    max_vaddr: int = 0
    for seg in elf.iter_segments():
        if seg['p_type'] == 'PT_LOAD':
            if min_vaddr is None:
                min_vaddr = seg['p_vaddr']
            else:
                min_vaddr = min(seg['p_vaddr'], min_vaddr)
            max_vaddr = max(seg['p_vaddr'] + seg['p_memsz'], max_vaddr)

    total: int = max_vaddr - min_vaddr
    return get_aligned_size(total) if align else total


def get_memory_usage_from_file(filename: str, align: bool) -> int:
    """
    Return the size in bytes occuped in memory of the loadable ELF segments from
    the ELF object file `filename`.
    """

    with open(filename, 'rb') as f:
        return get_memory_size(f)


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
    regions = [get_memory_usage_from_file(elf, args.align)
               for elf in args.elf_file]
    regions.append(args.reserve)
    total = sum(regions)

    if args.align:
        total = get_aligned_size(total)

    print(total)
    return 0


if __name__ == '__main__':
    sys.exit(main())
