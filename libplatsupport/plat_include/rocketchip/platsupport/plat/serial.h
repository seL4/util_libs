/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once
#include <autoconf.h>

enum chardev_id {
    PS_SERIAL0,
    /* defaults */
    PS_SERIAL_DEFAULT = PS_SERIAL0
};

#define PS_SERIAL_DEFAULT 0

#define DEFAULT_SERIAL_PADDR NULL
#define DEFAULT_SERIAL_INTERRUPT 0
