/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/**
 * Contains the definition for all character devices on this platform.
 * Currently this is just a simple patch.
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>
#include "serial.h"

static const int gsbi3_uart_irqs[] = {GSBI3_UART_IRQ, -1};
static const int gsbi4_uart_irqs[] = {GSBI4_UART_IRQ, -1};
static const int gsbi5_uart_irqs[] = {GSBI5_UART_IRQ, -1};
static const int gsbi6_uart_irqs[] = {GSBI6_UART_IRQ, -1};
static const int gsbi7_uart_irqs[] = {GSBI7_UART_IRQ, -1};


#define GSBI_UART_DEFN(devid) {          \
    .id      = GSBI##devid##_UART,       \
    .paddr   = GSBI##devid##_UART_PADDR, \
    .size    = BIT(12),                  \
    .irqs    = gsbi##devid##_uart_irqs,  \
    .init_fn = &uart_init                \
}



static const struct dev_defn dev_defn[] = {
    GSBI_UART_DEFN(3),
    GSBI_UART_DEFN(4),
    GSBI_UART_DEFN(5),
    GSBI_UART_DEFN(6),
    GSBI_UART_DEFN(7)
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
