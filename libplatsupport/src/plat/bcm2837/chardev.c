/*
 * Copyright 2016, Data61 CSIRO
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

/**
 * Contains the definition for all character devices on this platform.
 * Currently this is just a simple patch.
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

#include "serial.h"

static const int uart_irqs_0[] = {-1};

#define UART_DEFN(devid) {          \
    .id      = BCM2837_UART##devid,       \
    .paddr   = UART_PADDR_##devid, \
    .size    = BIT(12),                  \
    .irqs    = uart_irqs_##devid,  \
    .init_fn = &uart_init                \
}

static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
};

struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* o, struct ps_chardevice* d) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, o, d)) ? NULL : d;
        }
    }
    return NULL;
}
