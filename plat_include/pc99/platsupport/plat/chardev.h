/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PC99_CHARDEV_H
#define __PLATSUPPORT_PC99_CHARDEV_H

enum chardev_id {
    PC99_SERIAL_COM1,
    PC99_SERIAL_COM2,
    PC99_SERIAL_COM3,
    PC99_SERIAL_COM4,
    /* Aliases */
    PS_SERIAL0 = PC99_SERIAL_COM1,
    PS_SERIAL1 = PC99_SERIAL_COM2,
    PS_SERIAL2 = PC99_SERIAL_COM3,
    PS_SERIAL3 = PC99_SERIAL_COM4,
    /* defaults */
    PS_SERIAL_DEFAULT = PC99_SERIAL_COM1
};

#endif /* __PLATSUPPORT_PC99_CHARDEV_H */
