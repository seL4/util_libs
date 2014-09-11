#if 0
/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
/*
 * Provide serial UART glue to hardware. This file is not used on debug
 * kernels, as we use a seL4 debug syscall to print to the serial console.
 */

#include <sel4/sel4.h>
#include "../../plat_internal.h"
#include <stdio.h>

#define UART_PADDR     0x48020000
#define UART_REG(x)    ((volatile seL4_Word *)(uart_page + (x)))
#define UART_SR1_TRDY  5
#define UTXD           0x00
#define USR1           0x14

static void* uart_page;

void
__plat_serial_input_init_IRQ(void)
{
}

void
__plat_serial_init(void)
{
    uart_page = __map_device_page(UART_PADDR, 12);
}

void
__plat_putchar(int c)
{
    /* Wait for serial to become ready. */
    while (!(*UART_REG(USR1) & BIT(UART_SR1_TRDY)));

    /* Write out the next character. */
    *UART_REG(UTXD) = c;
    if (c == '\n') {
        __plat_putchar('\r');
    }
}

int
__plat_getchar(void)
{
    return EOF;
}
#endif
