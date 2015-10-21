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

#define UART0_PADDR  0x44E09000
#define UART1_PADDR  0x48022000
#define UART2_PADDR  0x48024000

#define UART0_IRQ    72
#define UART1_IRQ    73
#define UART2_IRQ    74

enum chardev_id {
    DM_UART0,
    DM_UART1,
    DM_UART2,
    /* Aliases */
    PS_SERIAL0 = DM_UART0,
    PS_SERIAL1 = DM_UART1,
    PS_SERIAL2 = DM_UART2,
    /* defaults */
    PS_SERIAL_DEFAULT = DM_UART0
};


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */


