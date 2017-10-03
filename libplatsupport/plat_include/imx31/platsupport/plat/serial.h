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

#define UART1_PADDR  0x43F90000
#define UART2_PADDR  0x43F94000
#define UART3_PADDR  0x5000C000
#define UART4_PADDR  0x43FB0000
#define UART5_PADDR  0x43FB4000

#define UART1_IRQ    45
#define UART2_IRQ    32
#define UART3_IRQ    18
#define UART4_IRQ    46
#define UART5_IRQ    47

/* official device names */
enum chardev_id {
    IMX31_UART1,
    IMX31_UART2,
    IMX31_UART3,
    IMX31_UART4,
    IMX31_UART5,
    /* Aliases */
    PS_SERIAL0 = IMX31_UART1,
    PS_SERIAL1 = IMX31_UART2,
    PS_SERIAL2 = IMX31_UART3,
    PS_SERIAL3 = IMX31_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = IMX31_UART1
};

