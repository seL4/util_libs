/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>

#include "serial.h"
#include "mini_serial.h"
#include "pl011_serial.h"

typedef struct {
    int (*uart_init)(const struct dev_defn *defn,
                     const ps_io_ops_t *ops,
                     ps_chardevice_t *dev);
    int (*uart_getchar)(ps_chardevice_t *d);
    int (*uart_putchar)(ps_chardevice_t *d, int c);
} uart_funcs_t;

uart_funcs_t uart_funcs;

int uart_getchar(ps_chardevice_t *d)
{
    uart_funcs.uart_getchar(d);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    uart_funcs.uart_putchar(d, c);
}

/*
 * Configure UART GPIO pins.
 *
 * On the RPi4, there are 6 UARTs: One miniUART (UART1) and 5 PL011 UARTs (UART0
 * + UART2-5). There is also a possibility to configure these pins via the
 * config.txt file statically. Since these pins might be used for other
 * functionality than UART, it might be best to configure these pins here
 * dynamically.
 *
 * BCM2711 TRM, Section 11.3. Primary UART Inputs and Outputs.
 */
int uart_gpio_configure(enum chardev_id id, const ps_io_ops_t *o)
{
    gpio_sys_t gpio_sys;
    gpio_t gpio;

    // We only configure the TX/RX pins for each UART peripheral and ignore flow
    // control (CTS/RTS pins) for now.
    int tx_pin = -1;
    int rx_pin = -1;
    int alt_function = -1;

    /*
     * GPIO pin configuration
     *
     * GPIO pins 14/15 are mapped accordingly: https://pinout.xyz/pinout/pin8_gpio14#
     *
     * This means:
     *  UART0: these pins are RX/TX lines; pins must be configured for ALT0
     *  UART1: these pins are RX/TX lines; pins must be configured for ALT5
     *  UART5: these pins are CTS/RTS lines; we ignore cts/rts for now
    */
    switch (id) {
    case 0:
        // UART 0 uses GPIO pins 14-15
        tx_pin = 14;
        rx_pin = 15;
        alt_function = BCM2711_GPIO_FSEL_ALT0;
        break;
    case 1:
        // UART 1 uses GPIO pins 14-15
        tx_pin = 14;
        rx_pin = 15;
        alt_function = BCM2711_GPIO_FSEL_ALT5;
        break;
    case 2:
        // UART 2 uses GPIO pins 0-3
        tx_pin = 0;
        rx_pin = 1;
        alt_function = BCM2711_GPIO_FSEL_ALT4;
        break;
    case 3:
        // UART 3 uses GPIO pins 4-7
        tx_pin = 4;
        rx_pin = 5;
        alt_function = BCM2711_GPIO_FSEL_ALT4;
        break;
    case 4:
        // UART 4 uses GPIO pins 8-11
        tx_pin = 8;
        rx_pin = 9;
        alt_function = BCM2711_GPIO_FSEL_ALT4;
        break;
    case 5:
        // UART 5 uses GPIO pins 12-13
        tx_pin = 12;
        rx_pin = 13;
        alt_function = BCM2711_GPIO_FSEL_ALT4;
        break;
    default:
        ZF_LOGD("No pin configuration required!");
        return 0;
    }

    if (tx_pin < 0 || rx_pin < 0) {
        ZF_LOGE("TX/RX pins wrongly configured!");
        return -1;
    }

    if (alt_function < 0) {
        ZF_LOGE("ALT function wrongly configured!");
        return -1;
    }

    // GPIO initialization
    int ret = gpio_sys_init((ps_io_ops_t *)o, &gpio_sys);
    if (ret) {
        ZF_LOGE("gpio_sys_init() failed: ret = %i", ret);
        return -EIO;
    }

    // configure tx pin
    gpio_sys.init(&gpio_sys, tx_pin, 0, &gpio);
    bcm2711_gpio_fsel(&gpio, alt_function);

    // configure rx pin
    gpio_sys.init(&gpio_sys, rx_pin, 0, &gpio);
    bcm2711_gpio_fsel(&gpio, alt_function);

    return 0;
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    switch (defn->id) {
    case 1:
        uart_funcs.uart_init    = &mini_uart_init;
        uart_funcs.uart_getchar = &mini_uart_getchar;
        uart_funcs.uart_putchar = &mini_uart_putchar;
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
    default:
        ZF_LOGE("UART with id %d does not exist!", defn->id);
        return -EINVAL;
    }

    int ret = uart_gpio_configure(defn->id, ops);
    if(0 != ret) {
        ZF_LOGF("UART GPIO configuration failed. %i", ret);
        return -EIO;
    }

    uart_funcs.uart_init(defn, ops, dev);

    return 0;
}
