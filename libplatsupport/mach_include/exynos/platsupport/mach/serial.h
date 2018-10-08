/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <platsupport/chardev.h>
#include <platsupport/mux.h>
#include <platsupport/clock.h>

/* INTP, INTSP, INTM */
#define INT_MODEM BIT(3)
#define INT_TX    BIT(2)
#define INT_ERR   BIT(1)
#define INT_RX    BIT(0)
#define EXYNOS_UART_RX_IRQ INT_RX
#define EXYNOS_UART_TX_IRQ INT_TX

typedef struct {
    void *vaddr;
    enum clk_id clock_id;
    enum mux_feature uart_mux_feature;
} static_serial_params_t;

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
                       clk_t* clk_src, ps_chardevice_t* dev);

/*
 * Handles only the receive interrupt. Allows RX and TX work to be seperated
 */
void exynos_handle_rx_irq(ps_chardevice_t *d);

/*
 * Handles only the transmit interrupt. Allows RX and TX work to be seperated.
 */
void exynos_handle_tx_irq(ps_chardevice_t *d);

/*
 * Checks which interrupts are set and returns the results as a bit field.
 * This does NOT handle the interrupts, this must be done seperately.
 */
int exynos_check_irq(ps_chardevice_t *d);

