/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

static const int uart0_irqs[] = {UART0_IRQ, -1};

/*
 * Despite each UART being 0x100 in size (according to the device tree) we
 * only need to map in the first page for the driver to function.
 */
#define UART_DEFN(devid) {          \
    .id      = UART##devid,    \
    .paddr   = UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
};

struct ps_chardevice *
ps_cdev_init(enum chardev_id id, const ps_io_ops_t *o, struct ps_chardevice *d)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, o, d)) ? NULL : d;
        }
    }
    return NULL;
}
