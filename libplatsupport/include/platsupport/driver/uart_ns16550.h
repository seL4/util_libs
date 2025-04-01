/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Driver for a 16550 compatible UART.
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <utils/arith.h>

#define NS16550_IER_ERBFI   BIT(0)   /* Enable Received Data Available Interrupt */
#define NS16550_IER_ETBEI   BIT(1)   /* Enable Transmitter Holding Register Empty Interrupt */
#define NS16550_IER_ELSI    BIT(2)   /* Enable Receiver Line Status Interrupt */
#define NS16550_IER_EDSSI   BIT(3)   /* Enable MODEM Status Interrupt */

#define NS16550_FCR_ENABLE_FIFOS   BIT(0)
#define NS16550_FCR_RESET_RX_FIFO  BIT(1)
#define NS16550_FCR_RESET_TX_FIFO  BIT(2)
#define NS16550_FCR_TRIGGER_1      (0u << 6)
#define NS16550_FCR_TRIGGER_4      (1u << 6)
#define NS16550_FCR_TRIGGER_8      (2u << 6)
#define NS16550_FCR_TRIGGER_14     (3u << 6)

#define NS16550_LCR_DLAB    BIT(7)   /* Divisor Latch Access */

#define NS16550_LSR_DR      BIT(0)   /* Data Ready */
#define NS16550_LSR_THRE    BIT(5)   /* Transmitter Holding Register Empty */

/* There are different NS16550 hardware implementations. The classic size of
 * each register is just one byte, but some implementations started to use
 * 32-bit registers, as this fits better with the natural alignment.
 */
#if defined(NS16550_WITH_REG32)
typedef volatile uint32_t ns16550_reg_t;
#elif defined(NS16550_WITH_REG8)
typedef volatile uint8_t ns16550_reg_t;
#else
#error "define NS16550_WITH_REG[8|32]"
#endif

typedef struct {
    /* 0x00 */
    ns16550_reg_t rbr_dll_thr; /* Receiver Buffer Register (Read Only)
                                * Divisor Latch (LSB)
                                * Transmitter Holding Register (Write Only)
                                */
    /* 0x01 or 0x04 */
    ns16550_reg_t dlm_ier;     /* Divisor Latch (MSB)
                                * Interrupt Enable Register
                                */
    /* 0x02 or 0x08 */
    ns16550_reg_t iir_fcr;     /* Interrupt Identification Register (Read Only)
                                * FIFO Control Register (Write Only)
                                */
    /* 0x03 or 0x0c */
    ns16550_reg_t lcr;         /* Line Control Register */
    /* 0x04 or 0x10 */
    ns16550_reg_t mcr;         /* MODEM Control Register */
    /* 0x05 or 0x14 */
    ns16550_reg_t lsr;         /* Line Status Register */
    /* 0x06 or 0x18 */
    ns16550_reg_t msr;         /* MODEM Status Register */
    /* 0x07 or 0x1c */
} ns16550_regs_t;


/*
 *******************************************************************************
 * UART access primitives
 *******************************************************************************
 */

static bool ns16550_is_tx_empty(ns16550_regs_t *regs)
{
    /* The THRE bit is set when the FIFO is fully empty. There seems no way to
     * detect if the FIFO is partially empty only, so we can't implement a
     * "tx_ready" check.
     */
    return (0 != (regs->lsr & NS16550_LSR_THRE));
}

static void ns16550_tx_byte(ns16550_regs_t *regs, uint8_t byte)
{
    /* Caller has to ensure TX FIFO is ready */
    regs->rbr_dll_thr = byte;
}

static bool ns16550_is_rx_empty(ns16550_regs_t *regs)
{
    return (0 == (regs->lsr & NS16550_LSR_DR));
}

static int ns16550_rx_byte(ns16550_regs_t *regs)
{
    /* Caller has to ensure RX FIFO has data */
    return regs->rbr_dll_thr;
}


/*
 *******************************************************************************
 * UART access helpers
 *******************************************************************************
 */

/*
 * Returns a char from the TX FIFO or EOF if the FIFO is empty.
 */
static int ns16550_get_char_or_EOF(ns16550_regs_t *regs)
{
    return ns16550_is_rx_empty(regs) ? EOF : ns16550_rx_byte(regs);
}
