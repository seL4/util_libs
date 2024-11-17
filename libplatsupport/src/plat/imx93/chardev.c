/*
 * Copyright 2024, Indan Zupancic
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "../../chardev.h"

static const struct dev_defn lpuart[NUM_CHARDEV] = {
    {
        .id = PS_SERIAL0,
        .paddr = 0x44380000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL1,
        .paddr = 0x44390000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL2,
        .paddr = 0x42570000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL3,
        .paddr = 0x42580000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL4,
        .paddr = 0x42590000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL5,
        .paddr = 0x425a0000,
        .size = 4096,
        .init_fn = uart_init
    },
    {
        .id = PS_SERIAL6,
        .paddr = 0x42690000,
        .size = 4096,
        .init_fn = uart_init,
    },
    {
        .id = PS_SERIAL7,
        .paddr = 0x426a0000,
        .size = 4096,
        .init_fn = uart_init,
    }
};

struct ps_chardevice *
ps_cdev_init(enum chardev_id id, const ps_io_ops_t *ops, struct ps_chardevice *dev)
{
    if (id < PS_SERIAL0 || id >= NUM_CHARDEV) {
        return NULL;
    }
    if (lpuart[id].init_fn(&lpuart[id], ops, dev)) {
        return NULL;
    }
    return dev;
}
