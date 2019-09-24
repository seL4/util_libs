/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>

#include "image_start_addr.h"

PECOFF_FILE_ALIGNMENT = 0x200;

SECTIONS
{
    .efi_text :
    {
        *(.text.efi.header)
    }
    .interp         : { *(.interp) }
}
INSERT BEFORE .hash;

SECTIONS
{
#ifndef CONFIG_IMAGE_EFI
    . = IMAGE_START_ADDR;
#endif
    .start :
    {
        *(.text.start)
    }
}
INSERT BEFORE .text;

#ifdef CONFIG_IMAGE_EFI
SECTIONS
{
    .pecoff_edata_padding : { BYTE(0); . = ALIGN(PECOFF_FILE_ALIGNMENT); }
}
INSERT AFTER .data;
#endif

SECTIONS
{
    .got.plt : { *(.got.plt) }
    /*
     * Put the GOT in its own section, with symbols around it
     * so that ASM code can easily find it and relocate addresses in it.
     */
    _got_start = .;
    .got : { *(.got) }
    _got_end = .;
}
INSERT AFTER .rodata;

SECTIONS
{
    . = ALIGN(8);
    ._archive_cpio : {
        _archive_start = .;
        *(._archive_cpio)
        _archive_end = .;
    }
}
INSERT AFTER .eh_frame;
