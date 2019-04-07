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

/*
 * Place a character to the given stream, which we always assume to be
 * 'stdout'.
 */
extern int
__fputc(int c, FILE *stream);

#define URXD  0x00 /* UART Receiver Register */
#define URST  0x18 /* status register */

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))
#define BIT(x) (1U << (x))

int __fputc(int c, FILE *stream)
{
    /* Wait until UART ready for the next character. */

    while ((*UART_REG(URST) & BIT(5)));

    /* Add character to the buffer. */
    *UART_REG(URXD) = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}
