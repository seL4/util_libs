/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
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
