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

#include <platsupport/mux.h>
#include <platsupport/clock.h>

#define EXYNOS_UART0_PADDR  0x12C00000
#define EXYNOS_UART1_PADDR  0x12C10000
#define EXYNOS_UART2_PADDR  0x12C20000
#define EXYNOS_UART3_PADDR  0x12C30000

#define EXYNOS_UART0_IRQ    83
#define EXYNOS_UART1_IRQ    84
#define EXYNOS_UART2_IRQ    85
#define EXYNOS_UART3_IRQ    86

#if defined(PLAT_EXYNOS5250)
#define UART_DEFAULT_FIN    100000000
#elif defined(PLAT_EXYNOS5410)
#define UART_DEFAULT_FIN     64000000
#else
#error Unknown platform
#endif


/* official device names */
enum chardev_id {
    PS_SERIAL0,
    PS_SERIAL1,
    PS_SERIAL2,
    PS_SERIAL3,
    PS_NSERIAL,
    /* defaults */
    PS_SERIAL_DEFAULT = PS_SERIAL2
};

/*
 * Initialiase an exynos serial device
 * @param[in] id      the id of the character device
 * @param[in] vaddr   The address at which the device can be accessed
 * @param[in] mux_sys A mux subsystem for pin control. If NULL is passed here,
 *                    the initialisation process will assume that the mux has
 *                    already been configured.
 * @param[in] clk_sys A clock subsystem for controlling the UART input clock.
 *                    If NULL is passed, the default clock frequency is assumed,
 *                    however, this may vary depending on the bootloader.
 * @param[out] dev    A character device structure to initialise
 * @return            0 on success
 */
int exynos_serial_init(enum chardev_id id, void* vaddr, mux_sys_t* mux_sys,
                       clock_sys_t* clock_sys, ps_chardevice_t* dev);


#endif /* __PLATSUPPORT_PLAT_SERIAL_H__ */


