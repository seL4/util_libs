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

#include <elfloader_common.h>
#include <platform.h>


#define UTHR 0x00 /* UART Transmit Holding Register */
#define ULSR 0x14 /* UART Line Status Register */
#define ULSR_THRE 0x20 /* Transmit Holding Register Empty */

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))

int plat_console_putchar(unsigned int c)
{
    /* Wait until UART ready for the next character. */
    while ((*UART_REG(ULSR) & ULSR_THRE) == 0);

    /* Add character to the buffer. */
    *UART_REG(UTHR) = c;

    return 0;
}
