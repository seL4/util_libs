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
    PS_SERIAL_DEFAULT = IMX6_UART2
};


#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */


