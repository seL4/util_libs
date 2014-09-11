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

#define EXYNOS_UART0_PADDR  0x13800000
#define EXYNOS_UART1_PADDR  0x13810000
#define EXYNOS_UART2_PADDR  0x13820000
#define EXYNOS_UART3_PADDR  0x13830000
#define EXYNOS_UART4_PADDR  0x13840000

#define EXYNOS_UART0_IRQ    84
#define EXYNOS_UART1_IRQ    85
#define EXYNOS_UART2_IRQ    86
#define EXYNOS_UART3_IRQ    87
#define EXYNOS_UART4_IRQ    88

#define UART_DEFAULT_FIN    90000000

/* official device names */
enum chardev_id {
    PS_SERIAL0,
    PS_SERIAL1,
    PS_SERIAL2,
    PS_SERIAL3,
    PS_NSERIAL,
    /* defaults */
    PS_SERIAL_DEFAULT = PS_SERIAL1
};

#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */


