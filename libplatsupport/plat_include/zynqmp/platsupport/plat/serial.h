/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_BSD)
 */
/*
 * This data was produced by DornerWorks, Ltd. of Grand Rapids, MI, USA under
 * a DARPA SBIR, Contract Number D16PC00107.
 *
 * Approved for Public Release, Distribution Unlimited.
 */

#ifndef __PLATSUPPORT_PLAT_SERIAL_H__
#define __PLATSUPPORT_PLAT_SERIAL_H__

#define UART0_PADDR  0xFF000000
#define UART1_PADDR  0xFF010000

#define UART0_IRQ    53
#define UART1_IRQ    54

enum chardev_id {
    ZYNQ_UART0,
    ZYNQ_UART1,
    /* Aliases */
    PS_SERIAL0 = ZYNQ_UART0,
    PS_SERIAL1 = ZYNQ_UART1,
    /* defaults */
    PS_SERIAL_DEFAULT = ZYNQ_UART0
};


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */
