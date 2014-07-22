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
    IMX31_UART1,
    IMX31_UART2,
    IMX31_UART3,
    IMX31_UART4,
    IMX31_UART5,
    /* Aliases */
    PS_SERIAL0 = IMX31_UART1,
    PS_SERIAL1 = IMX31_UART2,
    PS_SERIAL2 = IMX31_UART3,
    PS_SERIAL3 = IMX31_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = IMX31_UART1
};




#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */


