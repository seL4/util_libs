/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_IMX_UART_H
#define _PLATSUPPORT_IMX_UART_H

#define IMXUART_DLL             0x000
#define IMXUART_RHR             0x000
#define IMXUART_THR             0x040

#define IMXUART_LSR             0x094
#define IMXUART_LSR_RXFIFIOE    (1<<9)
#define IMXUART_LSR_RXOE        (1<<1)
#define IMXUART_LSR_RXPE        (1<<2)
#define IMXUART_LSR_RXFE        (1<<3)
#define IMXUART_LSR_RXBI        (1<<4)
#define IMXUART_LSR_TXFIFOE     (1<<13)
#define IMXUART_LSR_TXSRE       (1<<6)
#define IMXUART_LSR_RXFIFOSTS   (1<<7)

#define UART_RHR_READY_MASK    (1 << 15)
#define UART_BYTE_MASK           0xFF

#endif /*_PLATSUPPORT_IMX_UART_H */
