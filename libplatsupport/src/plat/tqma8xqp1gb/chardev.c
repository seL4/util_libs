/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * TQMa8XQP support a single UART (UART1 on the iMX8).
 *
 * Although there are four UARTs in the SoC only UART1
 * is configured by the bootloader software to be powered-on
 * and available in a root-server.
 *
 * Powering on additional UARTs requires an implementation
 * of the SCFW API to make calls on the SCU chipset, which
 * is beyond the scope of this platform port.
 */
#include "../../chardev.h"

static const struct dev_defn defn = {
    .id = PS_SERIAL_DEFAULT,
    .paddr = 0x5a070000,
    /* Note: The actual register block is 64k in size, but
     * chardev_map only supports mapping a single page-size
     * register block, and in practise there aren't very
     * many registers anyway! */
    .size = BIT(12),
    .init_fn = uart_init,
};

struct ps_chardevice *
ps_cdev_init(enum chardev_id id, const ps_io_ops_t *o, struct ps_chardevice *d)
{
    if (id != PS_SERIAL_DEFAULT) {
        /*
         * Only the PS_SERIAL_DEFAULT is supported, so return error
         * on any other value
         */
        return NULL;
    }

    return (defn.init_fn(&defn, o, d)) ? NULL : d;
}
