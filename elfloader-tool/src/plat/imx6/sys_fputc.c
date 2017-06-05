/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

/*
 * Platform-specific putchar implementation.
 */

#include <printf.h>
#include <types.h>
#include <platform.h>

#define UART_TRANSMIT     0x40
#define UART_CONTROL1     0x80
#define UART_CONTROL2     0x84
#define UART_CONTROL3     0x88
#define UART_CONTROL4     0x8C
#define UART_FIFO_CTRL    0x90
#define UART_STAT1        0x94
#define UART_STAT2        0x98

/* Transmit buffer FIFO empty. */
#define TXFE            (1U << 14)

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))

int
__fputc(int c, FILE *stream)
{
    /* Wait to be able to transmit. */
    while (!(*UART_REG(UART_STAT2) & TXFE));

    /* Transmit. */
    *UART_REG(UART_TRANSMIT) = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}
