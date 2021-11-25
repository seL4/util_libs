/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * Contains the definition for all character devices on this platform.
 * Currently this is just a simple patch.
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

#include "serial.h"

static const int uart0_irqs[] = {UART0_IRQ, -1};
static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};
static const int uart4_irqs[] = {UART4_IRQ, -1};
static const int uart5_irqs[] = {UART5_IRQ, -1};

#define PL011_UART_DEFN(devid) {    \
    .id      = BCM2711_UART##devid, \
    .paddr   = PL011_UART_BASE,     \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

#define MINI_UART_DEFN(devid) {     \
    .id      = BCM2711_UART##devid, \
    .paddr   = MINI_UART_BASE,      \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

static const struct dev_defn dev_defn[] = {
    PL011_UART_DEFN(0),
    MINI_UART_DEFN(1),
    PL011_UART_DEFN(2),
    PL011_UART_DEFN(3),
    PL011_UART_DEFN(4),
    PL011_UART_DEFN(5)
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
