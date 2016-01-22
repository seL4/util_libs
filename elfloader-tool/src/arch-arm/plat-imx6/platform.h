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

#include <autoconf.h>

/*
 * UART Hardware Constants
 *
 * (from IMX6 SoC Manual).
 */

#define IMX6_UART1_PADDR   0x02020000
#define IMX6_UART2_PADDR   0x021e8000
#define IMX6_UART3_PADDR   0x021ec000
#define IMX6_UART4_PADDR   0x021f0000
#define IMX6_UART5_PADDR   0x021F4000

#ifdef CONFIG_PLAT_SABRE
#define UART_PPTR          IMX6_UART2_PADDR
#elif CONFIG_PLAT_WANDQ
#define UART_PPTR          IMX6_UART1_PADDR
#else
#error "unknown imx6 platform selected!"
#endif

#endif /* _PLATFORM_H_ */
