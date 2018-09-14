/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_GPL)
 */
/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <autoconf.h>


#define ZYNQMP_UART0_BASE        0xFF000000
#define ZYNQMP_UART1_BASE        0xFF010000


#ifdef CONFIG_PLAT_ZYNQMP_ULTRA96
#define UART_PPTR              ZYNQMP_UART1_BASE
#else
#define UART_PPTR              ZYNQMP_UART0_BASE
#endif

#endif /* _PLATFORM_H_ */
