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

#include <platsupport/gpio.h>
#include <platsupport/mux.h>

#define GPIOID_GPIO0     GPIOID(GPIO_BANK1,  0)
#define GPIOID_GPIO1     GPIOID(GPIO_BANK1,  1)
#define GPIOID_GPIO2     GPIOID(GPIO_BANK1,  2)
#define GPIOID_GPIO3     GPIOID(GPIO_BANK1,  3)
#define GPIOID_GPIO4     GPIOID(GPIO_BANK1,  4)
#define GPIOID_GPIO5     GPIOID(GPIO_BANK1,  5)
#define GPIOID_GPIO6     GPIOID(GPIO_BANK1,  6)
#define GPIOID_GPIO7     GPIOID(GPIO_BANK1,  7)
#define GPIOID_GPIO8     GPIOID(GPIO_BANK1,  8)
#define GPIOID_GPIO9     GPIOID(GPIO_BANK1,  9)
#define GPIOID_GPIO16    GPIOID(GPIO_BANK7, 11)
#define GPIOID_GPIO17    GPIOID(GPIO_BANK7, 12)
#define GPIOID_GPIO18    GPIOID(GPIO_BANK7, 13)
#define GPIOID_GPIO19    GPIOID(GPIO_BANK4,  5)
#define GPIOID_NAND_D00  GPIOID(GPIO_BANK2,  0)
#define GPIOID_NAND_D01  GPIOID(GPIO_BANK2,  1)
#define GPIOID_NAND_D02  GPIOID(GPIO_BANK2,  2)
#define GPIOID_NAND_D03  GPIOID(GPIO_BANK2,  3)
#define GPIOID_NAND_D04  GPIOID(GPIO_BANK2,  4)
#define GPIOID_NAND_D05  GPIOID(GPIO_BANK2,  5)
#define GPIOID_NAND_D06  GPIOID(GPIO_BANK2,  6)
#define GPIOID_NAND_D07  GPIOID(GPIO_BANK2,  7)

#define KEY_VOL_UP  GPIOID_GPIO18
#define KEY_HOME    GPIOID_NAND_D04
#define KEY_SEARCH  GPIOID_NAND_D03
#define KEY_BACK    GPIOID_NAND_D02
#define KEY_MENU    GPIOID_NAND_D01
#define KEY_VOL_DN  GPIOID_GPIO19

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

