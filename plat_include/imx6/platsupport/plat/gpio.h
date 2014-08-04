/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _IMX6_GPIO_H
#define _IMX6_GPIO_H

#include <platsupport/gpio.h>
#include <platsupport/mux.h>

enum gpio_port {
    GPIO_BANK1,
    GPIO_BANK2,
    GPIO_BANK3,
    GPIO_BANK4,
    GPIO_BANK5,
    GPIO_BANK6,
    GPIO_BANK7,
    GPIO_NBANKS
};

/**
 * Initialise the exynos GPIO system given an exynos MUX subsystem
 * @param[in] bankX      A virtual mapping for gpio bank X.
 * @param[in] mux_sys    A handle to the mux subsystem. This subsystem
 *                       must contain memory mapped IO for the MUX regions.
 * @param[out] gpio_sys  A handle to a gpio subsystem to populate.
 * @return               0 on success
 */
int imx6_gpio_sys_init(void* bank1, void* bank2, void* bank3,
                       void* bank4, void* bank5, void* bank6,
                       void* bank7, 
                       mux_sys_t* mux_sys, gpio_sys_t* gpio_sys);

#endif /* _IMX6_GPIO_H */

