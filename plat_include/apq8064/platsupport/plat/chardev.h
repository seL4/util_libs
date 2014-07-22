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
    APQ8064_UART0,
    APQ8064_UART1,
    APQ8064_UART2,
    APQ8064_UART3,
    /* Aliases */
    PS_SERIAL0 = APQ8064_UART0,
    PS_SERIAL1 = APQ8064_UART1,
    PS_SERIAL2 = APQ8064_UART2,
    PS_SERIAL3 = APQ8064_UART3,
    /* defaults */
    PS_SERIAL_DEFAULT = APQ8064_UART0
};




#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */


