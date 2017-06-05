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


#define USR                   0x08
#define UTF                   0x70
#define UNTX                  0x40

#define USR_TXRDY             (1U << 2)
#define USR_TXEMP             (1U << 3)

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))

int
__fputc(int c, FILE *stream)
{
    (void)stream;
    /* Wait for TX fifo to be empty */
    while ( !(*UART_REG(USR) & USR_TXEMP) );
    /* Tell the peripheral how many characters to send */
    *UART_REG(UNTX) = 1;
    /* Write the character into the FIFO */
    *UART_REG(UTF) = c & 0xff;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}

