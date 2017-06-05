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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/*
 * UART Hardware Constants
 *
 */

/* use the UARTD */
#define TK1_UART1_PADDR   0x70006300
#define TK1_GICD_PADDR    0x50041000
#define TK1_GICC_PADDR    0x50042000

#define UART_PPTR          TK1_UART1_PADDR

#endif /* _PLATFORM_H_ */
