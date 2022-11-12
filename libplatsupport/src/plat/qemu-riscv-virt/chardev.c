/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Contains the definition for all character devices on this platform. */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

static const int uart0_irqs[] = {UART0_IRQ, -1};

#define UART_DEFN(devid) {          \
    .id      = UART##devid,         \
    .paddr   = UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
};

struct ps_chardevice *ps_cdev_init(enum chardev_id id, const ps_io_ops_t *o, struct ps_chardevice *d)
{
    for (int i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, o, d)) ? NULL : d;
        }
    }
    return NULL;
}
