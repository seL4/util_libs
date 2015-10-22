/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

ENTRY(_start)

SECTIONS
{
    .text :
    {
        *(.text.start)
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(16);
        _archive_start = .;
        *(.archive_ar)
        _archive_end = .;
    }

    .data :
    {
        *(.data)
        . = ALIGN(8);
        _bootstack_bottom = .;
        . = . + 4K;
        . = ALIGN(16K);
        _bootstack_top = .;
        _boot_pd = .;
        . = . + 16K;
        _lpae_boot_pmd = .;
        . = . + 16K;
        _lpae_boot_pgd = .;
        . = . + 32;
    }
    _end = .;
}
