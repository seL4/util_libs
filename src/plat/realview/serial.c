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

#include "../../plat_internal.h"
#include <stdio.h>


#define UART_PADDR     0x10009000
#define UART_REG(x)    ((volatile seL4_Word *)(uart_page + (x)))
#define UTXD           0x00
#define USRT           0x18

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
    while ((*UART_REG(USRT) & BIT(5)));

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
