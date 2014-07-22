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

#include "uart.h"

#define UART1_PADDR 0x02020000
#define UART2_PADDR 0x021E8000
#define UART3_PADDR 0x021EC000
#define UART4_PADDR 0x021F0000
#define UART5_PADDR 0x021F4000

#define UART1_IRQ 58
#define UART2_IRQ 59
#define UART3_IRQ 60
#define UART4_IRQ 61
#define UART5_IRQ 62

static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};
static const int uart4_irqs[] = {UART4_IRQ, -1};
static const int uart5_irqs[] = {UART5_IRQ, -1};


#define UART_DEFN(devid) {          \
    .id      = IMX6_UART##devid,    \
    .paddr   = UART##devid##_PADDR, \
    .size    = (1<<12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}
   

static const struct dev_defn dev_defn[] = {
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
    UART_DEFN(4),
    UART_DEFN(5)
};

struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* o, struct ps_chardevice* d) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return dev_defn[i].init_fn(dev_defn + i, o, d);
        }
    }
    return NULL;
}
