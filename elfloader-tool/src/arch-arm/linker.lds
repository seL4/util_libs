/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>

#include "image_start_addr.h"

SECTIONS
{
    .interp         : { *(.interp) }
}
INSERT BEFORE .hash;

SECTIONS
{
    . = IMAGE_START_ADDR;
    _text = .;
    .start :
    {
        *(.text.start)
    }
}
INSERT BEFORE .text;

SECTIONS
{
    .bss : {
        . = ALIGN(0x1000);
        core_stack_alloc = .;
        . = . + CONFIG_MAX_NUM_NODES * 1 << 12;
        core_stack_alloc_end = .;
        _bss = .;
        *(.bss)
        _bss_end = .;
    }
}

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
