/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *
 * QEMU riscv-virt emulates a 16550 compatible UART, where the register width
 * sticks to the classic size of 8 bits. This is fine, because it's just a
 * simulation, but contradicts many actual hardware implementations. There the
 * peripheral registers are usually 32-bit wide, because this fits much better
 * to the natural bus transfer sizes and alignments.
 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>

#define NS16550_WITH_REG8
#include <platsupport/driver/uart_ns16550.h>

#include "../../chardev.h"

static ns16550_regs_t *uart_get_regs(ps_chardevice_t *dev)
{
    return (ns16550_regs_t *)(dev->vaddr);
}

/*
 *******************************************************************************
 * UART access API
 *******************************************************************************
 */

int uart_putchar(ps_chardevice_t *dev, int c)
{
    ns16550_regs_t *regs = uart_get_regs(dev);

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
    while (!ns16550_is_tx_empty(regs)) {
        /* busy waiting loop */
    }

    /* Extract the byte to send, drop any flags. */
    uint8_t byte = (uint8_t)c;

    /* If SERIAL_AUTO_CR is enabled, a CR is sent before any LF. */
    if ((byte == '\n') && (dev->flags & SERIAL_AUTO_CR)) {
        ns16550_tx_byte(regs, '\r');
        /* Since we have blocked until the FIFO is empty, we don't have to wait
         * here. And QEMU does not emulate a FIFOs anyway.
         */
    }

    ns16550_tx_byte(regs, byte);

    return byte;
}

int uart_getchar(ps_chardevice_t *dev)
{
    ns16550_regs_t *regs = uart_get_regs(dev);
    return ns16550_get_char_or_EOF(regs);
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    /* No IRQ handling required here. */
}

static void ns16550_init(ns16550_regs_t *regs)
{
    /* disable interrupts */
    regs->dlm_ier = 0;

    /* Baudrates and serial line parameters are not emulated by QEMU, so the
     * divisor is just a dummy.
     */
    uint16_t clk_divisor = 1; /* dummy, would be for 115200 baud */
    regs->lcr = NS16550_LCR_DLAB; /* baud rate divisor setup */
    regs->dlm_ier = (clk_divisor >> 8) & 0xFF;
    regs->rbr_dll_thr = clk_divisor & 0xFF;
    regs->lcr = 0x03; /* set 8N1, clear DLAB to end baud rate divisor setup */

    /* enable and reset FIFOs, interrupt for each byte */
    regs->iir_fcr = NS16550_FCR_ENABLE_FIFOS
                    | NS16550_FCR_RESET_RX_FIFO
                    | NS16550_FCR_RESET_TX_FIFO
                    | NS16550_FCR_TRIGGER_1;

    /* enable RX interrupts */
    regs->dlm_ier = NS16550_IER_ERBFI;
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
    dev->read       = &uart_read; /* calls uart_putchar() */
    dev->write      = &uart_write; /* calls uart_getchar() */
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    /* Initialize the device. */
    ns16550_regs_t *regs = uart_get_regs(dev);
    ns16550_init(regs);

    return 0;
}
