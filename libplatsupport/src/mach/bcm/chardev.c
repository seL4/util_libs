/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <platsupport/gen_config.h>

#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>

#include "../../chardev.h"
#include "../../common.h"

#include "serial.h"


#define UART_DEFN(devid) {          \
    .id      = BCM2xxx_UART##devid, \
    .paddr   = UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart_irqs_##devid,   \
    .init_fn = &uart_init           \
}

#if defined(CONFIG_PLAT_BCM2711)

static const int uart_irqs_0[] = { UART0_IRQ, -1 };
static const int uart_irqs_1[] = { UART1_IRQ, -1 };
static const int uart_irqs_2[] = { UART2_IRQ, -1 };
static const int uart_irqs_3[] = { UART3_IRQ, -1 };
static const int uart_irqs_4[] = { UART4_IRQ, -1 };
static const int uart_irqs_5[] = { UART5_IRQ, -1 };

static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
    UART_DEFN(4),
    UART_DEFN(5)
};

#elif defined(CONFIG_PLAT_BCM2837)

static const int uart_irqs_0[] = { UART0_IRQ, -1 };
static const int uart_irqs_1[] = { UART1_IRQ, -1 };

static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
    UART_DEFN(1)
};

#else
#error "Unknown BCM2xxx platform!"
#endif


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
