/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _EXYNOS4_GPIO_H
#define _EXYNOS4_GPIO_H

#include <platsupport/gpio.h>

/* Port encodings */
#define _GPIOPORT(bank, port)       (((bank) << 8) | (port))
#define GPIOPORT_GET_BANK(port)     ((port) >> 8)
#define GPIOPORT_GET_PORT(port)     ((port) & 0xff)
#define GPIOPORT(bank, port)        _GPIOPORT(GPIO_##bank##_BANK, port)
#define GPIOPORT_NONE               _GPIOPORT(GPIO_NBANKS, 0)

enum gpio_bank {
    GPIO_LEFT_BANK,
    GPIO_RIGHT_BANK,
    GPIO_C2C_BANK,
    GPIO_AUDIO_BANK,
    GPIO_NBANKS
};

enum gpio_port {
    /* LEFT */
    GPA0 = GPIOPORT(LEFT,   0), /* 0x000 */
    GPA1 = GPIOPORT(LEFT,   1), /* 0x020 */
    GPB  = GPIOPORT(LEFT,   2), /* 0x040 */
    GPC0 = GPIOPORT(LEFT,   3), /* 0x060 */
    GPC1 = GPIOPORT(LEFT,   4), /* 0x080 */
    GPD0 = GPIOPORT(LEFT,   5), /* 0x0A0 */
    GPD1 = GPIOPORT(LEFT,   6), /* 0x0C0 */
};


/**
 * Initialise the exynos GPIO system given an exynos MUX subsystem
 * @param[in] mux_sys    A handle to the mux subsystem. This subsystem
 *                       must contain memory mapped IO for the MUX regions.
 * @param[out] gpio_sys  A handle to a gpio subsystem to populate.
 * @return               0 on success
 */
int exynos_gpio_sys_init(mux_sys_t* mux_sys, gpio_sys_t* gpio_sys);

#endif /* _EXYNOS4_GPIO_H */

