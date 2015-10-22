/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/*
 * UART Hardware Constants
 *
 * (from IMX31 SoC Manual).
 */

#define IMX6_UART1_PADDR   0x02020000
#define IMX6_UART2_PADDR   0x021e8000
#define IMX6_UART3_PADDR   0x021ec000
#define IMX6_UART4_PADDR   0x021f0000
#define IMX6_UART5_PADDR   0x021F4000

#define UART_PPTR          IMX6_UART2_PADDR

#endif /* _PLATFORM_H_ */
