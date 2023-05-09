/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/page.h>

static const int uart0_irqs[] = {UART0_IRQ, -1};
static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};
static const int uart4_irqs[] = {UART4_IRQ, -1};
static const int uart5_irqs[] = {UART5_IRQ, -1};

/*
 * Despite each UART being 0x10000 in size (according to the device tree) we
 * only need to map in the first page for the driver to functon.
 */
#define UART_DEFN(devid) {          \
    .id      = UART##devid,         \
    .paddr   = UART##devid##_PADDR, \
    .size    = PAGE_SIZE_4K,        \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
    UART_DEFN(4),
    UART_DEFN(5),
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
