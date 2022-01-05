/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 * Copyright 2022, Capgemini Engineering
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

#if defined(CONFIG_PLAT_SABRE) || defined(CONFIG_PLAT_IMX8MM_EVK)

    PS_SERIAL_DEFAULT = IMX_UART2

#elif defined(CONFIG_PLAT_WANDQ) || defined(CONFIG_PLAT_NITROGEN6SX) \
      || defined(CONFIG_PLAT_IMX7_SABRE) || defined(CONFIG_PLAT_IMX8MQ_EVK) \
      || defined(CONFIG_PLAT_MAAXBOARD)

    PS_SERIAL_DEFAULT = IMX_UART1

#else
#error "unknown imx platform selected!"
#endif

};
