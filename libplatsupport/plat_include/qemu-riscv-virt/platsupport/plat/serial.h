/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>

/* This information is taken from the device tree. */
#define UART0_PADDR     0x10000000
#define UART0_IRQ       10

enum chardev_id {
    UART0,
    /* Aliases */
    PS_SERIAL0 = UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ

/* QEMU RISC-V virt emulates a 16550 compatible UART. */

#define UART_IER_ERBFI   BIT(0)   /* Enable Received Data Available Interrupt */
#define UART_IER_ETBEI   BIT(1)   /* Enable Transmitter Holding Register Empty Interrupt */
#define UART_IER_ELSI    BIT(2)   /* Enable Receiver Line Status Interrupt */
#define UART_IER_EDSSI   BIT(3)   /* Enable MODEM Status Interrupt */

#define UART_FCR_ENABLE_FIFOS   BIT(0)
#define UART_FCR_RESET_RX_FIFO  BIT(1)
#define UART_FCR_RESET_TX_FIFO  BIT(2)
#define UART_FCR_TRIGGER_1      (0u << 6)
#define UART_FCR_TRIGGER_4      (1u << 6)
#define UART_FCR_TRIGGER_8      (2u << 6)
#define UART_FCR_TRIGGER_14     (3u << 6)

#define UART_LCR_DLAB    BIT(7)   /* Divisor Latch Access */

#define UART_LSR_DR      BIT(0)   /* Data Ready */
#define UART_LSR_THRE    BIT(5)   /* Transmitter Holding Register Empty */

typedef volatile struct {
    uint8_t rbr_dll_thr; /* 0x00 Receiver Buffer Register (Read Only)
                           *   Divisor Latch (LSB)
                           *   Transmitter Holding Register (Write Only)
                           */
    uint8_t dlm_ier;     /* 0x04 Divisor Latch (MSB)
                           *   Interrupt Enable Register
                           */
    uint8_t iir_fcr;     /* 0x08 Interrupt Identification Register (Read Only)
                           *    FIFO Control Register (Write Only)
                           */
    uint8_t lcr;         /* 0xC Line Control Register */
    uint8_t mcr;         /* 0x10 MODEM Control Register */
    uint8_t lsr;         /* 0x14 Line Status Register */
    uint8_t msr;         /* 0x18 MODEM Status Register */
} uart_regs_t;
