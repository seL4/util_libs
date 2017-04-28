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

#include <autoconf.h>

#define UART1_PADDR  0x02020000
#define UART2_PADDR  0x021E8000
#define UART3_PADDR  0x021EC000
#define UART4_PADDR  0x021F0000
#define UART5_PADDR  0x021F4000

#define UART1_IRQ    58
#define UART2_IRQ    59
#define UART3_IRQ    60
#define UART4_IRQ    61
#define UART5_IRQ    62

/* official device names */
enum chardev_id {
    IMX6_UART1,
    IMX6_UART2,
    IMX6_UART3,
    IMX6_UART4,
    IMX6_UART5,
    /* Aliases */
    PS_SERIAL0 = IMX6_UART1,
    PS_SERIAL1 = IMX6_UART2,
    PS_SERIAL2 = IMX6_UART3,
    PS_SERIAL3 = IMX6_UART4,
    PS_SERIAL4 = IMX6_UART5,
    /* defaults */
#if defined(CONFIG_PLAT_SABRE)
    PS_SERIAL_DEFAULT = IMX6_UART2
#elif defined(CONFIG_PLAT_WANDQ)
    PS_SERIAL_DEFAULT = IMX6_UART1
#else
#error "unknown imx6 platform selected!"
#endif
};


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */
