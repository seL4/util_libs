/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

OUTPUT_ARCH( "riscv" )
ENTRY( _start )

SECTIONS
{
  . = 0x0000000080400000;
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
    .bss :
    {
        _bss = .;
        *(.sbss*)
        *(.bss)
        *(.bss.*)
        _bss_end = .;
    }
     _end = .;
}
