/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/**
 * Contains the definition for all character devices on this platform.
 * Currently this is just a simple patch.
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

static const int uartA_irqs[] = {UARTA_IRQ, -1};
static const int uartB_irqs[] = {UARTB_IRQ, -1};
static const int uartC_irqs[] = {UARTC_IRQ, -1};
static const int uartD_irqs[] = {UARTD_IRQ, -1};
static const int uartA_ASYNC_irqs[] = {UARTA_IRQ, -1};
static const int uartB_ASYNC_irqs[] = {UARTB_IRQ, -1};
static const int uartC_ASYNC_irqs[] = {UARTC_IRQ, -1};
static const int uartD_ASYNC_irqs[] = {UARTD_IRQ, -1};


#define UART_DEFN(devid) {          \
    .id      = NV_UART##devid,    \
    .paddr   = UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

#define UART_ASYNC_DEFN(devid) {          \
    .id      = NV_UART##devid##_ASYNC,    \
    .paddr   = UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_ASYNC_irqs,  \
    .init_fn = &uart_init           \
}


static const struct dev_defn dev_defn[] = {
    UART_DEFN(A),
    UART_DEFN(B),
    UART_DEFN(C),
    UART_DEFN(D),
    UART_ASYNC_DEFN(A),
    UART_ASYNC_DEFN(B),
    UART_ASYNC_DEFN(C),
    UART_ASYNC_DEFN(D)
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
