#!/usr/bin/env python3
#
# Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: GPL-2.0-only
#
"""
Generate a C header file to standard output with a preprocessor symbol
definition of the physical load address in hexadecimal of the ELF-loader in
`payload_filename`.  The address is calculated using the description of memory
in `platform_filename` and the CPIO archive members embedded in the payload
file, including the loadable segments of the ELF objects and a possible DTB
(device tree binary) file.  The ELF-loader is placed in the first (lowest)
sufficiently-large memory region.

THIS IS NOT A STABLE API.  Use as a script, not a module.
"""

import argparse
import io
import os.path
import re
import sys

import libarchive

import elf_sift
import platform_sift

do_debug = False
program_name = 'shoehorn'


def write(message: str):
    """
    Write diagnostic `message` to standard error.
    """
    sys.stderr.write('{}: {}\n'.format(program_name, message))


def debug(message: str):
    """
    Emit debugging diagnostic `message`.
    """
    if do_debug:
        write('debug: {}'.format(message))


def debug_marker_set(mark: int, obj: str):
    debug('setting marker to 0x{:x} ({})'.format(mark, obj))


def die(message: str, status: int = 3):
    """
    Emit fatal diagnostic `message` and exit with `status` (3 if not specified).
    """
    write('fatal error: {}'.format(message))
    sys.exit(status)


def notice(message: str):
    """
    Emit notification diagnostic `message`.
    """
    write('notice: {}'.format(message))


def warn(message: str):
    """
    Emit warning diagnostic `message`.
    """
    write('warning: {}'.format(message))


def get_bytes(entry: libarchive.entry.ArchiveEntry) -> io.BytesIO:
    """
    Return an io.BytesIO object with the contents of the given archive entry.
    """
    bytes = bytearray()
    bytes.extend([byte for block in entry.get_blocks() for byte in block])
    return io.BytesIO(bytes)


def get_cpio(payload_filename: str) -> io.BytesIO:
    """
    Return an io.BytesIO object containing the CPIO part of `payload_filename`.
    The payload file is a CPIO archive with an object file header (e.g., an ELF
    prologue) prepended.  The embedded CPIO archive file is expected to be of
    the format the `file` command calls an "ASCII cpio archive (SVR4 with no
    CRC)".

    We assume that the CPIO "magic number" is not a valid sequence inside the
    object file header of `payload_filename`.
    """
    cpio_magic = b'070701'

    with open(payload_filename, 'rb') as payload:
        match = re.search(cpio_magic, payload.read())

        if match:
            debug('found CPIO identifying sequence {} at offset 0x{:x} in {}'
                  .format(cpio_magic, match.start(), payload_filename))
            payload.seek(match.start())
            cpio_bytes = payload.read()
        else:
            warn('did not find the CPIO identifying sequence {} expected in {}'
                 .format(cpio_magic, header_size, payload_filename))
            cpio_bytes = None

    return cpio_bytes


def main() -> int:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Generate a C header file to standard output with a preprocessor symbol
definition of the physical load address in hexadecimal of the ELF-loader in
`payload_filename`.  The address is calculated using the description of memory
in `platform_filename` and the CPIO archive members embedded in the payload
file, including the loadable segments of the ELF objects and a possible DTB
(device tree binary) file.  The ELF-loader is placed in the first (lowest)
sufficiently-large memory region.
""")
    parser.add_argument('--load-rootservers-high', dest='load_rootservers_high',
                        default=False, action='store_true',
                        help='assume ELF-loader will put rootservers at top of'
                             ' memory')
    parser.add_argument('platform_filename', nargs=1, type=str,
                        help='YAML description of platform parameters (e.g.,'
                             ' platform_gen.yaml)')
    parser.add_argument('payload_filename', nargs=1, type=str,
                        help='ELF-loader image file (e.g., archive.o)')

    # Set up some simpler names for argument data and derived information.
    args = parser.parse_args()
    image = args.payload_filename[0]
    image_size = os.path.getsize(image)
    do_load_rootservers_high = args.load_rootservers_high
    platform = platform_sift.load_data(args.platform_filename[0])

    rootservers = []
    is_dtb_present = False
    is_good_fit = False
    kernel_elf = None

    with libarchive.memory_reader(get_cpio(image)) as archive:
        for entry in archive:
            name = entry.name
            debug('encountered CPIO entry name: {}'.format(name))

            if name == 'kernel.elf':
                kernel_elf = get_bytes(entry)
            elif name == 'kernel.dtb':
                # The ELF-loader loads the entire DTB into memory.
                is_dtb_present = True
                dtb_size = entry.size
            elif name.endswith('.bin'):
                # Skip checksum entries.
                notice('skipping checkum entry "{}"'.format(name))
            else:
                rootservers.append(get_bytes(entry))

    if not kernel_elf:
        die('missing kernel.elf')

    # Enumerate the regions as we encounter them for diagnostic purposes.
    region_counter = -1
    last_region = len(platform['memory'])

    for region in platform['memory']:
        region_counter += 1
        marker = region['start']
        debug_marker_set(marker, 'region {} start'.format(region_counter))
        # Note: We assume that the kernel is loaded at the start of memory
        # because that is what elfloader-tool/src/arch-arm/linker.lds ensures
        # will happen.  This assumption may change in the future!
        kernel_start = region['start']
        kernel_size = elf_sift.get_memory_usage(kernel_elf, align=True)
        kernel_end = elf_sift.get_aligned_size(kernel_start + kernel_size)
        marker = kernel_end
        debug_marker_set(marker, 'kernel_end')

        if is_dtb_present:
            dtb_start = marker
            dtb_end = elf_sift.get_aligned_size(dtb_start + dtb_size)
            marker = dtb_end
            debug_marker_set(marker, 'dtb_end')

        if do_load_rootservers_high and (region_counter == last_region):
            warn('"--load-rootservers-high" specified but placing'
                 ' ELF-loader in last (or only) region ({} of {}); overlap'
                 ' may not be detected by this tool'
                 .format(region_counter, last_region))

        # Deal with the 1..(# of CPUs - 1) possible user payloads, if we're not
        # loading them in high memory, discontiguously with the kernel.
        #
        # TODO: Handle this differently (skipping, or checking to see if we had
        # to push the kernel so high that it whacks the user payloads--but the
        # ELF-loader itself should detect that case).  At present the case of
        # multiple rootservers is difficult to debug because it is not
        # implemented on the archive-construction side; see JIRA SELFOUR-2368.
        if not do_load_rootservers_high:
            for elf in rootservers:
                marker += elf_sift.get_memory_usage(elf, align=True)
                debug_marker_set(marker, 'end of rootserver')

            # Note: sel4test_driver eats (almost) 4 more MiB than it claims to.
            # Fixing this is JIRA SELFOUR-2335.
            fudge_factor = 4 * 1024 * 1024
            marker += elf_sift.get_aligned_size(fudge_factor)
            debug_marker_set(marker, 'end of (aligned) fudge factor')

        image_start_address = marker

        if (image_start_address + image_size) <= region['end']:
            is_good_fit = True
            break

    if not is_good_fit:
        die('ELF-loader image "{image}" of size 0x{size:x} does not fit within'
            ' any memory region described in "{yaml}"'
            .format(image=image, size=image_size, yaml=platform), status=1)

    sys.stdout.write('#define IMAGE_START_ADDR 0x{load:x}\n'
                     .format(load=image_start_address))
    return 0


if __name__ == '__main__':
    sys.exit(main())
