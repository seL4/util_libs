/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
 * Contains definitions for all character devices on this
 * platform
 */

#include "../../chardev.h"
#include "../../common.h"

#include "uart.h"

#define UART1_PADDR 0x43F90000
#define UART2_PADDR 0x43F94000
#define UART3_PADDR 0x5000C000
#define UART4_PADDR 0x43FB0000
#define UART5_PADDR 0x43FB4000

#define UART1_IRQ   45
#define UART2_IRQ   32
#define UART3_IRQ   18
#define UART4_IRQ   46
#define UART5_IRQ   47

static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};
static const int uart4_irqs[] = {UART4_IRQ, -1};
static const int uart5_irqs[] = {UART5_IRQ, -1};


#define UART_DEFN(devid) {                     \
        .id      = IMX31_UART##devid,          \
        .paddr   = UART##devid##_PADDR,        \
        .size    = (1<<12),                    \
        .irqs    = uart##devid##_irqs,         \
        .init_fn = &uart_init                  \
    }


static const struct dev_defn dev_defn[] = {
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
    UART_DEFN(4),
    UART_DEFN(5)
};


/* It would be nice to reuse this, but it requires knowledge of the variable *
 * sized 'dev_defn'                                                          */
struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* o,
             struct ps_chardevice* d) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return dev_defn[i].init_fn(dev_defn + i, o, d);
        }
    }
    return NULL;
}

