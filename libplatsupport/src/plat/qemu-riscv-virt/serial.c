/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* QEMU RISC-V virt emulates a 16550 compatible UART. */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

static uart_regs_t *uart_get_regs(ps_chardevice_t *dev)
{
    return (uart_regs_t *)(dev->vaddr);
}

/*
 *******************************************************************************
 * UART access primitives
 *******************************************************************************
 */

static bool internal_uart_is_tx_empty(uart_regs_t *regs)
{
    /* The THRE bit is set when the FIFO is fully empty. On real hardware, there
     * seems no way to detect if the FIFO is partially empty only, so we can't
     * implement a "tx_ready" check. Since QEMU does not emulate a FIFO, this
     * does not really matter.
     */
    return (0 != (regs->lsr & UART_LSR_THRE));
}

static void internal_uart_tx_byte(uart_regs_t *regs, uint8_t byte)
{
    /* Caller has to ensure TX FIFO is ready */
    regs->rbr_dll_thr = byte;
}

static bool internal_uart_is_rx_empty(uart_regs_t *regs)
{
    return (0 == (regs->lsr & UART_LSR_DR));
}


static int internal_uart_rx_byte(uart_regs_t *regs)
{
    /* Caller has to ensure RX FIFO has data */
    return regs->rbr_dll_thr;
}


/*
 *******************************************************************************
 * UART access API
 *******************************************************************************
 */

int uart_putchar(ps_chardevice_t *dev, int c)
{
    uart_regs_t *regs = uart_get_regs(dev);

    /* There is no way to check for "TX ready", the only thing we have is a
     * check for "TX FIFO empty". This is not optimal, as we might wait here
     * even if there is space in the FIFO. Seems the 16550 was built based on
     * the idea that software keeps track of the FIFO usage. A driver would
     * know how much space is left in the FIFO, so it can write new data
     * either immediately or buffer it. If the FIFO empty interrupt arrives,
     * data can be written from the buffer to fill the FIFO.
     * However, since QEMU does not emulate a FIFO, we can just implement a
     * simple model here and block - expecting to never block practically.
     */
    while (!internal_uart_is_tx_empty(regs)) {
        /* busy waiting loop */
    }

    /* Extract the byte to send, drop any flags. */
    uint8_t byte = (uint8_t)c;

    /* If SERIAL_AUTO_CR is enabled, a CR is sent before any LF. */
    if ((byte == '\n') && (dev->flags & SERIAL_AUTO_CR)) {
        internal_uart_tx_byte(regs, '\r');
        /* Since we have blocked until the FIFO is empty, we don't have to wait
         * here. And QEMU does not emulate a FIFOs anyway.
         */
    }

    internal_uart_tx_byte(regs, byte);

    return byte;
}

int uart_getchar(ps_chardevice_t *dev)
{
    uart_regs_t *regs = uart_get_regs(dev);

    /* if UART is empty return an error */
    if (internal_uart_is_rx_empty(regs)) {
        return EOF;
    }

    return internal_uart_rx_byte(regs) & 0xFF;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    /* No IRQ handling required here. */
}

static void uart_setup(ps_chardevice_t *dev)
{
    uart_regs_t *regs = uart_get_regs(dev);

    regs->dlm_ier = 0; // disable interrupts

    /* Baudrates and serial line parameters are not emulated by QEMU, so the
     * divisor is just a dummy.
     */
    uint16_t clk_divisor = 1; /* dummy, would be for 115200 baud */
    regs->lcr = UART_LCR_DLAB; /* baud rate divisor setup */
    regs->dlm_ier = (clk_divisor >> 8) & 0xFF;
    regs->rbr_dll_thr = clk_divisor & 0xFF;
    regs->lcr = 0x03; /* set 8N1, clear DLAB to end baud rate divisor setup */

    /* enable and reset FIFOs, interrupt for each byte */
    regs->iir_fcr = UART_FCR_ENABLE_FIFOS
                    | UART_FCR_RESET_RX_FIFO
                    | UART_FCR_RESET_TX_FIFO
                    | UART_FCR_TRIGGER_1;

    /* enable RX interrupts */
    regs->dlm_ier = UART_IER_ERBFI;
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));

    /* Map device. */
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    /* Set up the device. */
    uart_setup(dev);

    return 0;
}
