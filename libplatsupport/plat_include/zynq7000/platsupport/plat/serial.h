/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_SERIAL_H__
#define __PLATSUPPORT_PLAT_SERIAL_H__

#define UART0_PADDR  0xE0000000
#define UART1_PADDR  0xE0001000

#define UART0_IRQ    59
#define UART1_IRQ    82

enum chardev_id {
    ZYNQ_UART0,
    ZYNQ_UART1,
    /* Aliases */
    PS_SERIAL0 = ZYNQ_UART0,
    PS_SERIAL1 = ZYNQ_UART1,
    /* defaults */
    PS_SERIAL_DEFAULT = ZYNQ_UART1
};


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */
