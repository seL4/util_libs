/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/chardev.h>
#include <platsupport/plat/serial.h>

/*****************************
 **** Serial device flags ****
 *****************************/

/* Auto-send CR(Carriage Return) after each "\n".
 * NOTE: This flag should be set by default. */
#define SERIAL_AUTO_CR BIT(0)

/*****************************/

enum serial_parity {
    PARITY_NONE,
    PARITY_EVEN,
    PARITY_ODD
};

/*
 * Initialiase a device
 * @param  id: the id of the character device
 * @param ops: a structure containing OS specific operations for memory access
 * @param dev: a character device structure to populate
 * @return   : 0 on success
 */
int serial_init(enum chardev_id id,
                ps_io_ops_t *ops,
                ps_chardevice_t *dev);

/*
 * Performs line configuration of a serial port
 * @param       dev: an initialised character device structure
 * @param       bps: The desired boad rate
 * @param char_size: The desired character size
 * @param stop_bits: The number of stop bits
 */
int serial_configure(ps_chardevice_t *dev,
                     long bps,
                     int  char_size,
                     enum serial_parity parity,
                     int  stop_bits);

