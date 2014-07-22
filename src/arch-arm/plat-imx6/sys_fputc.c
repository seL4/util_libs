/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

/*
 * Platform-specific putchar implementation.
 */

#include "../stdint.h"
#include "../stdio.h"

/*
 * UART Hardware Constants
 *
 * (from IMX31 SoC Manual).
 */

#define IMX6_UART1_BASE   0x02020000
#define IMX6_UART2_BASE   0x021e8000
#define IMX6_UART3_BASE   0x021ec000
#define IMX6_UART4_BASE   0x021f0000
#define IMX6_UART5_BASE   0x021F4000

#define IMX6_UART_BASE    IMX6_UART2_BASE

#define UART_TRANSMIT     0x40
#define UART_CONTROL1     0x80
#define UART_CONTROL2     0x84
#define UART_CONTROL3     0x88
#define UART_CONTROL4     0x8C
#define UART_FIFO_CTRL    0x90
#define UART_STAT1        0x94
#define UART_STAT2        0x98

/* Transmit buffer FIFO empty. */
#define TXFE            (1 << 14)

int
__fputc(int c, FILE *stream)
{
    volatile uint32_t *stat_reg
        = (volatile uint32_t *)(IMX6_UART_BASE + UART_STAT2);
    volatile uint32_t *trans_reg
        = (volatile uint32_t *)(IMX6_UART_BASE + UART_TRANSMIT);

    /* Wait to be able to transmit. */
    while ((*stat_reg & TXFE) == 0) {
        /* spin. */
    }

    /* Transmit. */
    *trans_reg = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}
