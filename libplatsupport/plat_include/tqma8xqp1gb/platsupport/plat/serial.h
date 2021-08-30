/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

/* Note: The TQMa8XQP board only has LPUART_1 available by default.
 * This is due to that UART being enabled by u-boot. Additional,
 * UARTs require an implementation of the SCFW client API to
 * turn on additional UARTs.
 *
 * This is beyond the scope of the simple platform support library.
 * As such only a single UART is supported.
 */
enum chardev_id {
    PS_SERIAL_DEFAULT,
};