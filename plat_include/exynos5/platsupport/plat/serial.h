/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_SERIAL_H__
#define __PLATSUPPORT_PLAT_SERIAL_H__

#define EXYNOS_UART0_PADDR  0x12C00000
#define EXYNOS_UART1_PADDR  0x12C10000
#define EXYNOS_UART2_PADDR  0x12C20000
#define EXYNOS_UART3_PADDR  0x12C30000

#define EXYNOS_UART0_IRQ    83
#define EXYNOS_UART1_IRQ    84
#define EXYNOS_UART2_IRQ    85
#define EXYNOS_UART3_IRQ    86

#if defined(PLAT_EXYNOS5250)
#define UART_DEFAULT_FIN    100000000
#elif defined(PLAT_EXYNOS5410)
#define UART_DEFAULT_FIN     64000000
#elif defined(PLAT_EXYNOS5422)
#define UART_DEFAULT_FIN  53200000
#else
#error Unknown platform
#endif

/* official device names */
enum chardev_id {
    PS_SERIAL0,
    PS_SERIAL1,
    PS_SERIAL2,
    PS_SERIAL3,
    PS_NSERIAL,
    /* defaults */
    PS_SERIAL_DEFAULT = PS_SERIAL2
};

#include <platsupport/mach/serial.h>

#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */


