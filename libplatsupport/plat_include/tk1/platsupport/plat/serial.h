/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_SERIAL_H__
#define __PLATSUPPORT_PLAT_SERIAL_H__

/* the UARTs are in one page, so we only map the first one */
#define UARTA_PADDR  0x70006000
#define UARTB_PADDR  0x70006000
#define UARTC_PADDR  0x70006000
#define UARTD_PADDR  0x70006000

#define UARTB_OFFSET 0x40
#define UARTC_OFFSET 0x200
#define UARTD_OFFSET 0x300


#define UARTA_IRQ    68 
#define UARTB_IRQ    69 
#define UARTC_IRQ    78 
#define UARTD_IRQ    122

/* official device names */
enum chardev_id {
    TK1_UARTA,
    TK1_UARTB,
    TK1_UARTC,
    TK1_UARTD,
    /* Aliases */
    PS_SERIAL0 = TK1_UARTA,
    PS_SERIAL1 = TK1_UARTB,
    PS_SERIAL2 = TK1_UARTC,
    PS_SERIAL3 = TK1_UARTD,
    /* defaults */
    PS_SERIAL_DEFAULT = TK1_UARTD
};


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */


