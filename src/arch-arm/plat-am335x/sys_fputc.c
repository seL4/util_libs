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
 * Place a character to the given stream, which we always assume to be
 * 'stdout'.
 */
extern int
__fputc(int c, FILE *stream);

/* AM335X UART 0 physical address. */
#define UART0_PADDR 0x44E09000
#define UART_PPTR UART0_PADDR

#define UTHR 0x00 /* UART Transmit Holding Register */
#define ULSR 0x14 /* UART Line Status Register */
#define ULSR_THRE 0x20 /* Transmit Holding Register Empty */

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR+(x)))

int
__fputc(int c, FILE *stream)
{
    /* Wait until UART ready for the next character. */
    while ((*UART_REG(ULSR) & ULSR_THRE) == 0);

    /* Add character to the buffer. */
    *UART_REG(UTHR) = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}
