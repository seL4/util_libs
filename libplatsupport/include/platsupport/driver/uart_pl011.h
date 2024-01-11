/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Driver for a ARM PL011 UART.
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <utils/arith.h>

#define PL011_FR_BUSY       BIT(3) /* UART busy */
#define PL011_FR_RXFE       BIT(4) /* Receive FIFO empty */
#define PL011_FR_TXFF       BIT(5) /* Transmit FIFO full */
#define PL011_FR_RXFF       BIT(6) /* Receive FIFO full */
#define PL011_FR_TXFE       BIT(7) /* Transmit FIFO empty */

#define PL011_IMSC_RXIM     BIT(4)  /* RX interrupt */
#define PL011_IMSC_TXIM     BIT(5)  /* TX interrupt */
#define PL011_IMSC_RTIM     BIT(6)  /* RX timeout interrupt */
#define PL011_IMSC_OEIM     BIT(10) /* Overrun timeout */

#define PL011_CR_UARTEN     BIT(0) /* UART enable */
#define PL011_CR_TXE        BIT(8) /* Transmit enable */
#define PL011_CR_RXE        BIT(9) /* Receive enable */

// 0111 1111 0000
#define PL011_ICR_RXIC      BIT(4)
#define PL011_ICR_TXIC      BIT(5)

typedef volatile struct {
    uint32_t dr;        /* 0x00 */
    uint32_t _rfu_04;   /* 0x04 */
    uint32_t _rfu_08;   /* 0x08 */
    uint32_t _rfu_0c;   /* 0x0c */
    uint32_t _rfu_10;   /* 0x10 */
    uint32_t _rfu_14;   /* 0x14 */
    uint32_t fr;        /* 0x18 */
    uint32_t _rfu_1c;   /* 0x1c */
    uint32_t _rfu_20;   /* 0x20 */
    uint32_t _rfu_24;   /* 0x24 */
    uint32_t _rfu_28;   /* 0x28 */
    uint32_t _rfu_2c;   /* 0x2c */
    uint32_t _rfu_30;   /* 0x30 */
    uint32_t _rfu_34;   /* 0x34 */
    uint32_t imsc;      /* 0x38 */
    uint32_t _rfu_3c;   /* 0x3c */
    uint32_t _rfu_40;   /* 0x40 */
    uint32_t icr;       /* 0x44 */
    uint32_t _rfu_48;   /* 0x48 */
    uint32_t _rfu_4c;   /* 0x4c */
} pl011_regs_t;


/*
 *******************************************************************************
 * UART access primitives
 *******************************************************************************
 */

static bool pl011_is_rx_fifo_empty(pl011_regs_t *regs)
{
    return (0 != (regs->fr & PL011_FR_RXFE));
}

static bool pl011_is_tx_fifo_full(pl011_regs_t *regs)
{
    return (0 != (regs->fr & PL011_FR_TXFF));
}

static void pl011_tx_byte(pl011_regs_t *regs, uint8_t c)
{
    /* Caller has to ensure TX FIFO has space */
    regs->dr = c;
}

static uint8_t pl011_rx_byte(pl011_regs_t *regs)
{
    return (uint8_t)(regs->dr & 0xFF);
}

static void pl011_clear_interrupt(pl011_regs_t *regs)
{
    regs->icr = 0x7f0;
}

/*
 *******************************************************************************
 * UART access helpers
 *******************************************************************************
 */

/*
 * Returns a char from the TX FIFO or EOF if the FIFO is empty.
 */
static int pl011_get_char_or_EOF(pl011_regs_t *regs)
{
    return pl011_is_rx_fifo_empty(regs) ? EOF : pl011_rx_byte(regs);
}

/*
 * Block until there is space in the TX FIFO, then outputs the char.
 */
static void pl011_put_char_blocking(pl011_regs_t *regs, uint8_t c)
{
    while (pl011_is_tx_fifo_full(regs)) {
        /* busy loop */
    }
    pl011_tx_byte(regs, c);
}

/*
 * Block until there is space in the TX FIFO, then outputs the char. Optionally
 * output a CR (\r) first in case of LF (\n) to support the terminal use case.
 */
static void pl011_put_char_blocking_auto_cr(pl011_regs_t *regs, uint8_t c,
                                            bool is_auto_cr)
{
    if ((c == '\n') && is_auto_cr) {
        pl011_put_char_blocking(regs, '\r');
    }
    pl011_put_char_blocking(regs, c);
}
