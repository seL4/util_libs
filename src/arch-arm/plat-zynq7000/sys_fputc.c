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
 * UART Hardware Constants
 */

#define ZYNQ_UART0_BASE        0xE0000000
#define ZYNQ_UART1_BASE        0xE0001000

#define ZYNQ_UART_BASE         ZYNQ_UART1_BASE

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
int
__fputc(int c, FILE *stream)
{
    volatile uint32_t *stat_reg
        = (volatile uint32_t *)(ZYNQ_UART_BASE + UART_CHANNEL_STS);
    volatile uint32_t *trans_reg
        = (volatile uint32_t *)(ZYNQ_UART_BASE + UART_TX_RX_FIFO);

    /* Wait to be able to transmit. */
    while ((*stat_reg & UART_CHANNEL_STS_TXEMPTY) == 0) {
        /* spin. */
    }

    /* Transmit. */
    *trans_reg = c;

    /* Send '\r' after every '\n'. */
    if (c == '\n') {
        (void)__fputc('\r', stream);
    }

    return 0;
}
