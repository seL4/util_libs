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
#include <platsupport/plat/gpio.h>
#include <stdlib.h>
#include <string.h>

#include "serial.h"

typedef struct uart_gpio_defn {
    int uart_id;
    int tx_pin;
    int rx_pin;
    int alt_function;
} uart_gpio_defn_t;

#define UART_GPIO_DEFN(id, tx, rx, alt) {   \
    .uart_id      = id,                     \
    .tx_pin       = tx,                     \
    .rx_pin       = rx,                     \
    .alt_function = alt                     \
}

/*
 * GPIO pin definitions from TRMs
 */

#if defined(CONFIG_PLAT_BCM2711)

static const uart_gpio_defn_t gpio_defs[NUM_CHARDEV] = {
    UART_GPIO_DEFN(BCM2xxx_UART0, 14, 15, BCM2711_GPIO_FSEL_ALT0), // UART 0 uses GPIO pins 14-15
    UART_GPIO_DEFN(BCM2xxx_UART1, 14, 15, BCM2711_GPIO_FSEL_ALT5), // UART 1 uses GPIO pins 14-15
    UART_GPIO_DEFN(BCM2xxx_UART2, 0,  1,  BCM2711_GPIO_FSEL_ALT4), // UART 2 uses GPIO pins 0-3
    UART_GPIO_DEFN(BCM2xxx_UART3, 4,  5,  BCM2711_GPIO_FSEL_ALT4), // UART 3 uses GPIO pins 4-7
    UART_GPIO_DEFN(BCM2xxx_UART4, 8,  9,  BCM2711_GPIO_FSEL_ALT4), // UART 4 uses GPIO pins 8-11
    UART_GPIO_DEFN(BCM2xxx_UART5, 12, 13, BCM2711_GPIO_FSEL_ALT4)  // UART 5 uses GPIO pins 12-13
};

#elif defined(CONFIG_PLAT_BCM2837)

static const uart_gpio_defn_t gpio_defs[NUM_CHARDEV] = {
    UART_GPIO_DEFN(BCM2xxx_UART0, 14, 15, BCM2837_GPIO_FSEL_ALT0), // UART 0 uses GPIO pins 14-15
    UART_GPIO_DEFN(BCM2xxx_UART1, 14, 15, BCM2837_GPIO_FSEL_ALT5)  // UART 1 uses GPIO pins 14-15
};

#else
#error "Unknown BCM2xxx platform!"
#endif


typedef struct {
    int (*uart_init)(const struct dev_defn *defn,
                     const ps_io_ops_t *ops,
                     ps_chardevice_t *dev);
    int (*uart_getchar)(ps_chardevice_t *d);
    int (*uart_putchar)(ps_chardevice_t *d, int c);
} uart_funcs_t;

static uart_funcs_t uart_funcs;



int uart_getchar(ps_chardevice_t *d)
{
    return uart_funcs.uart_getchar(d);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    return uart_funcs.uart_putchar(d, c);
}

/*
 * Configure UART GPIO pins.
 *
 * There is a possibility to configure these pins via the
 * config.txt file statically. Since these pins might be 
 * used for other functionality than UART, it might be 
 * best to configure these pins here dynamically.
 */
int uart_gpio_configure(enum chardev_id id, const ps_io_ops_t *o)
{
    gpio_sys_t gpio_sys;
    gpio_t gpio;

    // We only configure the TX/RX pins for each UART peripheral and ignore flow
    // control (CTS/RTS pins) for now.
    uart_gpio_defn_t pindef = { -1, -1, -1, -1 };

    for (int i = 0; i < NUM_CHARDEV; i++)
    {
        if (id == gpio_defs[i].uart_id)
        {
            pindef = gpio_defs[i];
            break;
        }
    }

    if (pindef.uart_id < 0) {
        ZF_LOGE("No valid pin configuration found for chardev: %i", id);
        return -1;
    }

    // GPIO initialization
    int ret = gpio_sys_init((ps_io_ops_t *)o, &gpio_sys);
    if (ret) {
        ZF_LOGE("GPIO sys init failed: %i", ret);
        return -1;
    }

#if defined(CONFIG_PLAT_BCM2711)

    // Configure TX pin
    gpio_sys.init(&gpio_sys, pindef.tx_pin, 0, &gpio);
    bcm2711_gpio_fsel(&gpio, pindef.alt_function);

    // Configure RX pin
    gpio_sys.init(&gpio_sys, pindef.rx_pin, 0, &gpio);
    bcm2711_gpio_fsel(&gpio, pindef.alt_function);

#elif defined(CONFIG_PLAT_BCM2837)

    // Configure TX pin
    gpio_sys.init(&gpio_sys, pindef.tx_pin, 0, &gpio);
    bcm2837_gpio_fsel(&gpio, pindef.alt_function);

    // Configure RX pin
    gpio_sys.init(&gpio_sys, pindef.rx_pin, 0, &gpio);
    bcm2837_gpio_fsel(&gpio, pindef.alt_function);

#else
#error "Unknown BCM2xxx platform!"
#endif

    return 0;
}

int uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev)
{
    switch (defn->id) {

#if defined(CONFIG_PLAT_BCM2711)

    case 1:
        uart_funcs.uart_init    = &bcm_uart_init;
        uart_funcs.uart_getchar = &bcm_uart_getchar;
        uart_funcs.uart_putchar = &bcm_uart_putchar;
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        uart_funcs.uart_init    = &pl011_uart_init;
        uart_funcs.uart_getchar = &pl011_uart_getchar;
        uart_funcs.uart_putchar = &pl011_uart_putchar;
        break;

#elif defined(CONFIG_PLAT_BCM2837)

    case 0:
        uart_funcs.uart_init    = &pl011_uart_init;
        uart_funcs.uart_getchar = &pl011_uart_getchar;
        uart_funcs.uart_putchar = &pl011_uart_putchar;
        break;
    case 1:
        uart_funcs.uart_init    = &bcm_uart_init;
        uart_funcs.uart_getchar = &bcm_uart_getchar;
        uart_funcs.uart_putchar = &bcm_uart_putchar;
        break;

#else
#error "Unknown BCM2xxx platform!"
#endif
    default:
        ZF_LOGE("UART with id %d does not exist!", defn->id);
        return -1;

    }

    uart_gpio_configure(defn->id, ops);

    uart_funcs.uart_init(defn, ops, dev);

    return 0;
}
