/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <autoconf.h>
#include <platsupport/mach/serial.h>

#define UART1_PADDR  0x02020000
#define UART2_PADDR  0x021E8000
#define UART3_PADDR  0x021EC000
#define UART4_PADDR  0x021F0000
#define UART5_PADDR  0x021F4000

#define UART1_IRQ    58
#define UART2_IRQ    59
#define UART3_IRQ    60
#define UART4_IRQ    61
#define UART5_IRQ    62

#define UART_REF_CLK 40089600

#if defined(CONFIG_PLAT_SABRE)
    #define DEFAULT_SERIAL_PADDR UART2_PADDR
    #define DEFAULT_SERIAL_INTERRUPT UART2_IRQ
#elif defined(CONFIG_PLAT_WANDQ)
    #define DEFAULT_SERIAL_PADDR UART1_PADDR
    #define DEFAULT_SERIAL_INTERRUPT UART1_IRQ
#endif
