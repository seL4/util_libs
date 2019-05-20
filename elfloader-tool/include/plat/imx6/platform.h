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

#pragma once

#include <autoconf.h>
#include <elfloader/gen_config.h>

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

