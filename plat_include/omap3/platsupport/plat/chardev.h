/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_CHARDEV_H__
#define __PLATSUPPORT_PLAT_CHARDEV_H__


/* official device names */
enum chardev_id {
    OMAP3_UART1,
    OMAP3_UART2,
    OMAP3_UART3,
    OMAP3_UART4,
    /* Aliases */
    PS_SERIAL0 = OMAP3_UART1,
    PS_SERIAL1 = OMAP3_UART2,
    PS_SERIAL2 = OMAP3_UART3,
    PS_SERIAL3 = OMAP3_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = OMAP3_UART3
};

#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */
