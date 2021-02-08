/*
 * Copyright 2020, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <platsupport/plat/icicle_mss.h>
#include <string.h>

#include "../../chardev.h"

static inline uart_regs_t *uart_get_priv(ps_chardevice_t *d)
{
    return (uart_regs_t *)d->vaddr;
}

int uart_getchar(ps_chardevice_t *d)
{
    uart_regs_t *regs = uart_get_priv(d);
    if (regs->line_status & LSR_DATA_READY_MASK) {
        return regs->rx_buffer;
    }
    return -1;
}

static void busy_wait_fifo_empty_and_tx_char(uart_regs_t *regs, int c)
{
    // wait until FIFO empty
    while ((regs->line_status & LSR_TX_HOLD_REG_EMPTY_MASK) == 0) {
        // busy loop
    }

    regs->tx_buffer = c;
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    uart_regs_t *regs = uart_get_priv(d);

    // turn LF into CR+LF if SERIAL_AUTO_CR is active
    if ((c == '\n') && (d->flags & SERIAL_AUTO_CR)) {
        busy_wait_fifo_empty_and_tx_char(regs, '\r');
    }

    busy_wait_fifo_empty_and_tx_char(regs, c);
    return c;
}

static void uart_handle_irq(ps_chardevice_t *d UNUSED)
{
    // IRQs are cleared when the TX/RX watermark conditions are no longer met
    // so there is nothing to do here.
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    uart_regs_t *regs;
    uint32_t divisor;
    uint32_t divisor_by_128;
    uint32_t divisor_by_64;
    uint32_t fractional_divisor;
    /* Attempt to map the virtual address, assure this works */
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    regs = uart_get_priv(dev);

    regs->line_control = LCR_WORD_LEN_8;
    regs->modem_control = 0;
    regs->multi_mode_control_0 = 0;
    regs->multi_mode_control_1 = 0;
    regs->multi_mode_control_2 = 0;
    regs->glitch_filter = 0;
    regs->transmitter_time_guard = 0;
    regs->receiver_time_out = 0;

    /*
     * Configure baud rate divisors. This uses the fractional baud rate divisor
     * where possible to provide the most accurate baud rate possible.
     * This algorithm is taken from the Microchip MSS UART Driver (LIC:MIT)
     */
    divisor_by_128 = (uint32_t)((8UL * POLARFIRE_PCLK) / POLARFIRE_BAUD);
    divisor_by_64 = divisor_by_128 / 2u;
    divisor = divisor_by_64 / 64u;
    fractional_divisor = divisor_by_64 - (divisor * 64u);
    fractional_divisor += (divisor_by_128 - (divisor * 128u))
                          - (fractional_divisor * 2u);

    // div: 81, frac: 24 => 115207

    regs->line_control |= LCR_DIV_LATCH_MASK;
    regs->divisor_latch_msb = (uint8_t)(divisor >> 8);
    regs->divisor_latch_lsb = (uint8_t)divisor;
    regs->line_control &= ~LCR_DIV_LATCH_MASK;

    regs->multi_mode_control_0 |= MM0_ENABLE_FRAC_MASK;
    regs->fractional_divisor = (uint8_t)fractional_divisor;
    regs->line_control = LCR_WORD_LEN_8;

    return 0;
}
