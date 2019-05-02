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
Generate a C header file to standard output with a preprocessor symbol defining
the physical memory address in hexadecimal of the load address of ELF-loader
image given in `payload_file` using the information describing the loadable
segements of the `kernel_file` argument into the first sufficiently-large region
described in `platform_file`.
"""

import argparse
import os.path
import sys

import elf_sift
import platform_sift

do_debug = False


def debug_marker_set(mark: int, obj: str):
    if do_debug:
        sys.stderr.write('shoehorn: setting marker to 0x{:x} ({})\n'
                         .format(mark, obj))


def main() -> int:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Generate a C header file to standard output with a preprocessor symbol defining
the physical memory address in hexadecimal of the load address of ELF-loader
image given in `payload_file` using the information describing the loadable
segements of the `kernel_file` argument into the first sufficiently-large region
described in `platform_file`.
""")
    parser.add_argument('--load-rootservers-high', dest='load_rootservers_high',
                        default=False, action='store_true',
                        help='assume ELF-loader will put rootservers at top of'
                             ' memory')
    # Note: If we used a CPIO-reading module, we might be able to get by with
    # just 2 arguments: the platform description and the payload file.
    parser.add_argument('platform_file', nargs=1, type=str,
                        help='YAML description of platform parameters (e.g.,'
                             ' platform_gen.yaml)')
    parser.add_argument('payload_file', nargs=1, type=str,
                        help='ELF-loader payload object (e.g., archive.o)')
    parser.add_argument('kernel_file', nargs=1, type=str,
                        help='kernel ELF object (e.g., kernel.elf)')
    parser.add_argument('dtb_file', nargs=1, type=str,
                        help='device tree description file (e.g., kernel.dtb)')
    parser.add_argument('rootserver_file', nargs='+', type=str,
                        help='rootserver ELF object (e.g., sel4test-driver)')
    args = parser.parse_args()
    image_size = os.path.getsize(args.payload_file[0])
    platform = platform_sift.load_data(args.platform_file[0])
    do_load_rootservers_high = True if args.load_rootservers_high else False
    is_good_fit = False
    memory_iterator = iter(platform['memory'])
    counter = -1

    for region in memory_iterator:
        counter += 1
        marker = region['start']
        debug_marker_set(marker, 'region {} start'.format(counter))
        # Note: We assume that the kernel is loaded at the start of memory
        # because that is what elfloader-tool/src/arch-arm/linker.lds ensures
        # will happen.  This assumption may change in the future!
        kernel_start = region['start']
        kernel_size = elf_sift.get_memory_size(args.kernel_file[0], align=True)
        kernel_end = elf_sift.get_aligned_size(kernel_start + kernel_size)
        marker = kernel_end
        debug_marker_set(marker, 'kernel_end')
        # TODO: Handle the case where the DTB isn't in the payload but handed to
        # us by U-Boot (see elfloader-tool/src/arch-arm/sys_boot.c).
        dtb_start = marker
        dtb_size = os.path.getsize(args.dtb_file[0])
        dtb_end = elf_sift.get_aligned_size(dtb_start + dtb_size)
        marker = dtb_end
        debug_marker_set(marker, 'dtb_end')

        if do_load_rootservers_high \
            and ((counter + 1) == len(platform['memory'])):
                sys.stderr.write('shoehorn: note: "--load-rootservers-high"'
                                 ' specified but placing ELF-loader in last (or'
                                 ' only) region ({} of {}); overlap may not be'
                                 ' detected by this tool\n'
                                 .format((counter + 1),
                                         len(platform['memory'])))

        # Deal with the 1..(# of CPUs - 1) possible user payloads, if we're not
        # loading them in high memory, discontiguously with the kernel.
        #
        # TODO: Handle this differently (skipping, or checking to see if we had
        # to push the kernel so high that it whacks the user payloads--but the
        # ELF-loader itself should detect that case).
        if not do_load_rootservers_high:
            for elf in args.rootserver_file:
                marker += elf_sift.get_memory_size(elf, align=True)
                debug_marker_set(marker, 'end of {}'.format(elf))

            # FIXME: sel4test_driver eats (almost) 4 more MiB than it claims to.
            fudge_factor = 4 * 1024 * 1024
            marker += elf_sift.get_aligned_size(fudge_factor)
            debug_marker_set(marker, 'end of (aligned) fudge factor')

        image_start_address = marker

        if (image_start_address + image_size) <= region['end']:
            is_good_fit = True
            break

    if not is_good_fit:
        sys.stderr.write("""\
shoehorn: ELF-loader image of size 0x{size:x} does not fit within any available
memory region\n""".format(size=image_size))
        return 1

    sys.stdout.write('#define IMAGE_START_ADDR 0x{load:x}\n'
                     .format(load=image_start_address))
    return 0


if __name__ == '__main__':
    sys.exit(main())
