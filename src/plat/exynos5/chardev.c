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

//#include "uart.h"

#define UART0_PADDR 0x12C00000
#define UART1_PADDR 0x12C10000
#define UART2_PADDR 0x12C20000
#define UART3_PADDR 0x12C30000

#define UART0_IRQ 83
#define UART1_IRQ 84
#define UART2_IRQ 85
#define UART3_IRQ 86

static const int uart0_irqs[] = {UART0_IRQ, -1};
static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};

struct ps_chardevice* uart_init(const struct dev_defn* defn, const ps_io_ops_t* ops, struct ps_chardevice* dev);

#define UART_DEFN(devid) {                     \
        .id      = EXYNOS5_UART##devid,          \
        .paddr   = UART##devid##_PADDR,        \
        .size    = (1<<12),                    \
        .irqs    = uart##devid##_irqs,         \
        .init_fn = &uart_init                  \
    }


static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
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

