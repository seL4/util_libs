/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_SERIAL_H__
#define __PLATSUPPORT_SERIAL_H__


#include <platsupport/chardev.h>


enum uart_parity {
    PARITY_EVEN,
    PARITY_ODD,
    PARITY_NONE
};


int uart_configure(struct ps_chardevice *d,
                   long bps,
                   int  char_size,
                   enum uart_parity parity,
                   int  stop_bits);

#endif /* __PLATSUPPORT_UART_H__ */
