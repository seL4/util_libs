/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once


/* official device names */
enum chardev_id {
    IMX_UART1,
    IMX_UART2,
    IMX_UART3,
    IMX_UART4,
    IMX_UART5,
    /* Aliases */
    PS_SERIAL0 = IMX_UART1,
    PS_SERIAL1 = IMX_UART2,
    PS_SERIAL2 = IMX_UART3,
    PS_SERIAL3 = IMX_UART4,
    PS_SERIAL4 = IMX_UART5,
#if defined(CONFIG_PLAT_SABRE) || defined(CONFIG_PLAT_IMX7)
    PS_SERIAL_DEFAULT = IMX_UART2
#elif defined(CONFIG_PLAT_WANDQ)
    PS_SERIAL_DEFAULT = IMX_UART1
#else
#error "unknown imx platform selected!"
#endif
};

