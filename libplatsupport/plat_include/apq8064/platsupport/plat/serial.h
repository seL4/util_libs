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

#define GSBI3_UART_PADDR  0x16240000
#define GSBI4_UART_PADDR  0x16340000
#define GSBI5_UART_PADDR  0x1A240000
#define GSBI6_UART_PADDR  0x16540000
#define GSBI7_UART_PADDR  0x16640000

#define GSBI3_UART_IRQ    -1
#define GSBI4_UART_IRQ    -1
#define GSBI5_UART_IRQ    -1
#define GSBI6_UART_IRQ    -1
#define GSBI7_UART_IRQ    -1

/* official device names */
enum chardev_id {
    GSBI3_UART,
    GSBI4_UART,
    GSBI5_UART,
    GSBI6_UART,
    GSBI7_UART,
    /* Aliases */
    PS_SERIAL0 = GSBI3_UART,
    PS_SERIAL1 = GSBI4_UART,
    PS_SERIAL2 = GSBI5_UART,
    PS_SERIAL3 = GSBI6_UART,
    PS_SERIAL4 = GSBI7_UART,
    /* defaults */
    PS_SERIAL_DEFAULT = GSBI7_UART
};

