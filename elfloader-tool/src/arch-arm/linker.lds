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

#ifdef CONFIG_IMAGE_EFI
SECTIONS
{
    .pecoff_edata_padding : { BYTE(0); . = ALIGN(PECOFF_FILE_ALIGNMENT); }
}
INSERT AFTER .data;
#endif

SECTIONS
{
    .abort_stack :
    {
        _abortstack_bottom = .;
        . = . + 4K;
        . = ALIGN(16K);
        _abortstack_top = .;
    }
}
INSERT AFTER .bss;
