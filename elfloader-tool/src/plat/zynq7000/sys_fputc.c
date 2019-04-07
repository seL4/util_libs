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
 * UART Hardware Constants
 */

#define UART_CONTROL                 0x00
#define UART_MODE                    0x04
#define UART_INTRPT_EN               0x08
#define UART_INTRPT_DIS              0x0C
#define UART_INTRPT_MASK             0x10
#define UART_CHNL_INT_STS            0x14
#define UART_BAUD_RATE_GEN           0x18
#define UART_RCVR_TIMEOUT            0x1C
#define UART_RCVR_FIFO_TRIGGER_LEVEL 0x20
#define UART_MODEM_CTRL              0x24
#define UART_MODEM_STS               0x28
#define UART_CHANNEL_STS             0x2C
#define UART_TX_RX_FIFO              0x30
#define UART_BAUD_RATE_DIVIDER       0x34
#define UART_FLOW_DELAY              0x38
#define UART_TX_FIFO_TRIGGER_LEVEL   0x44

#define UART_INTRPT_MASK_TXEMPTY     (1U << 3)
#define UART_CHANNEL_STS_TXEMPTY     (1U << 3)

#define UART_CONTROL_TX_EN           (1U << 4)
#define UART_CONTROL_TX_DIS          (1U << 5)

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))

int __fputc(int c, FILE *stream)
{
    /* Wait to be able to transmit. */
    while (!(*UART_REG(UART_CHANNEL_STS) & UART_CHANNEL_STS_TXEMPTY));

    /* Transmit. */
    *UART_REG(UART_TX_RX_FIFO) = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}

void enable_uart()
{
    uint32_t v = *UART_REG(UART_CONTROL);
    v |= UART_CONTROL_TX_EN;
    v &= ~UART_CONTROL_TX_DIS;
    *UART_REG(UART_CONTROL) = v;
}
