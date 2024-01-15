/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <autoconf.h>
#include <elfloader/gen_config.h>
#include "image_start_addr.h"

ENTRY( _start )

SECTIONS
{
    . = IMAGE_START_ADDR;
    _text = .;
    .start :
    {
        *(.text.start)
    }
    .text :
    {
        *(.text)
    }
    . = ALIGN(16);
    .rodata :
    {
        *(.srodata*)
        . = ALIGN(16);
        *(.rodata)
        *(.rodata.*)
        /*
         * ld crashes when we add this here: *(_driver_list)
         */
        . = ALIGN(16);
        _archive_start = .;
        *(._archive_cpio)
        _archive_end = .;
    }
    . = ALIGN(16);
    .data :
    {
        __global_pointer$ = . + 0x800;
        *(.sdata*)
        *(.data)
        *(.data.*)
    }
    . = ALIGN(16);
    .bss (NOLOAD) :
    {
        . = ALIGN(0x1000);
        core_stack_alloc = .;
        . = . + CONFIG_MAX_NUM_NODES * 1 << 12;
        core_stack_alloc_end = .;
        _bss = .;
        *(.sbss*)
        *(.bss)
        *(.bss.*)
        _bss_end = .;
    }
     _end = .;
}
