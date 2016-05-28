/*
 * Copyright 2016, Data61 CSIRO
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_SERIAL_H__
#define __PLATSUPPORT_PLAT_SERIAL_H__

#define BUS_ADDR_OFFSET             0x7E000000
#define PADDDR_OFFSET               0x3F000000

#define UART_BUSADDR                0x7E215000

#define UART_PADDR                  (UART_BUSADDR-BUS_ADDR_OFFSET+PADDDR_OFFSET)

enum chardev_id {
    BCM2837_UART0,
    /* Aliases */
    PS_SERIAL0 = BCM2837_UART0,
    /* defaults */
    PS_SERIAL_DEFAULT = BCM2837_UART0
};

#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */
