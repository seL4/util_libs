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
    . = ALIGN(8);
    ._archive_cpio : {
        _archive_start = .;
        *(._archive_cpio)
        _archive_end = .;
    }
}
INSERT AFTER .eh_frame;
