/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/util.h>
#include <platsupport/gpio.h>

enum gpio_port {
    GPIO_BANK1,
    GPIO_NBANKS
};

// GPIO IDs: https://elinux.org/RPi_BCM2835_GPIOs
#define GPIOID_GPIO_00      GPIOID(GPIO_BANK1,  0)
#define GPIOID_GPIO_01      GPIOID(GPIO_BANK1,  1)
#define GPIOID_GPIO_02      GPIOID(GPIO_BANK1,  2)
#define GPIOID_GPIO_03      GPIOID(GPIO_BANK1,  3)
#define GPIOID_GPIO_04      GPIOID(GPIO_BANK1,  4)
#define GPIOID_GPIO_05      GPIOID(GPIO_BANK1,  5)
#define GPIOID_GPIO_06      GPIOID(GPIO_BANK1,  6)
#define GPIOID_GPIO_07      GPIOID(GPIO_BANK1,  7)
#define GPIOID_GPIO_08      GPIOID(GPIO_BANK1,  8)
#define GPIOID_GPIO_09      GPIOID(GPIO_BANK1,  9)
#define GPIOID_GPIO_10      GPIOID(GPIO_BANK1,  10)
#define GPIOID_GPIO_11      GPIOID(GPIO_BANK1,  11)
#define GPIOID_GPIO_12      GPIOID(GPIO_BANK1,  12)
#define GPIOID_GPIO_13      GPIOID(GPIO_BANK1,  13)
#define GPIOID_GPIO_14      GPIOID(GPIO_BANK1,  14)
#define GPIOID_GPIO_15      GPIOID(GPIO_BANK1,  15)
#define GPIOID_GPIO_16      GPIOID(GPIO_BANK1,  16)
#define GPIOID_GPIO_17      GPIOID(GPIO_BANK1,  17)
#define GPIOID_GPIO_18      GPIOID(GPIO_BANK1,  18)
#define GPIOID_GPIO_19      GPIOID(GPIO_BANK1,  19)
#define GPIOID_GPIO_20      GPIOID(GPIO_BANK1,  20)
#define GPIOID_GPIO_21      GPIOID(GPIO_BANK1,  21)
#define GPIOID_GPIO_22      GPIOID(GPIO_BANK1,  22)
#define GPIOID_GPIO_23      GPIOID(GPIO_BANK1,  23)
#define GPIOID_GPIO_24      GPIOID(GPIO_BANK1,  24)
#define GPIOID_GPIO_25      GPIOID(GPIO_BANK1,  25)
#define GPIOID_GPIO_26      GPIOID(GPIO_BANK1,  26)
#define GPIOID_GPIO_27      GPIOID(GPIO_BANK1,  27)
#define GPIOID_GPIO_28      GPIOID(GPIO_BANK1,  28)
#define GPIOID_GPIO_29      GPIOID(GPIO_BANK1,  29)
#define GPIOID_GPIO_30      GPIOID(GPIO_BANK1,  30)
#define GPIOID_GPIO_31      GPIOID(GPIO_BANK1,  31)
#define GPIOID_GPIO_32      GPIOID(GPIO_BANK1,  32)
#define GPIOID_GPIO_33      GPIOID(GPIO_BANK1,  33)
#define GPIOID_GPIO_34      GPIOID(GPIO_BANK1,  34)
#define GPIOID_GPIO_35      GPIOID(GPIO_BANK1,  35)
#define GPIOID_GPIO_36      GPIOID(GPIO_BANK1,  36)
#define GPIOID_GPIO_37      GPIOID(GPIO_BANK1,  37)
#define GPIOID_GPIO_38      GPIOID(GPIO_BANK1,  38)
#define GPIOID_GPIO_39      GPIOID(GPIO_BANK1,  39)
#define GPIOID_GPIO_40      GPIOID(GPIO_BANK1,  40)
#define GPIOID_GPIO_41      GPIOID(GPIO_BANK1,  41)
#define GPIOID_GPIO_42      GPIOID(GPIO_BANK1,  42)
#define GPIOID_GPIO_43      GPIOID(GPIO_BANK1,  43)
#define GPIOID_GPIO_44      GPIOID(GPIO_BANK1,  44)
#define GPIOID_GPIO_45      GPIOID(GPIO_BANK1,  45)
#define GPIOID_GPIO_46      GPIOID(GPIO_BANK1,  46)
#define GPIOID_GPIO_47      GPIOID(GPIO_BANK1,  47)
#define GPIOID_GPIO_48      GPIOID(GPIO_BANK1,  48)
#define GPIOID_GPIO_49      GPIOID(GPIO_BANK1,  49)
#define GPIOID_GPIO_50      GPIOID(GPIO_BANK1,  50)
#define GPIOID_GPIO_51      GPIOID(GPIO_BANK1,  51)
#define GPIOID_GPIO_52      GPIOID(GPIO_BANK1,  52)
#define GPIOID_GPIO_53      GPIOID(GPIO_BANK1,  53)

#define MAX_GPIO_ID         GPIOID_GPIO_53

/*
 * According to BCM2835 TRM, section 6.1 Register View
 * GPIO function select (bit 27-29):
 *
 * 000 = GPIO Pin is an input
 * 001 = GPIO Pin is an output
 * 100 = GPIO Pin takes alternate function 0
 * 101 = GPIO Pin takes alternate function 1
 * 110 = GPIO Pin takes alternate function 2
 * 111 = GPIO Pin takes alternate function 3
 * 011 = GPIO Pin takes alternate function 4
 * 010 = GPIO Pin takes alternate function 5
 */
enum {
    BCM2837_GPIO_FSEL_INPT = 0,
    BCM2837_GPIO_FSEL_OUTP = 1,
    BCM2837_GPIO_FSEL_ALT0 = 4,
    BCM2837_GPIO_FSEL_ALT1 = 5,
    BCM2837_GPIO_FSEL_ALT2 = 6,
    BCM2837_GPIO_FSEL_ALT3 = 7,
    BCM2837_GPIO_FSEL_ALT4 = 3,
    BCM2837_GPIO_FSEL_ALT5 = 2,
    BCM2837_GPIO_FSEL_MASK = 7
};

/**
 * Initialise the bcm2837 GPIO system
 * @param[in] bankX      A virtual mapping for gpio bank X.
 * @param[out] gpio_sys  A handle to a gpio subsystem to populate.
 * @return               0 on success
 */
int bcm2837_gpio_sys_init(void *bank1, gpio_sys_t *gpio_sys);

/**
 * Sets the Function Select register for the given pin, which configures the pin
 * as Input, Output or one of the 6 alternate functions.
 * @param[in] pin   GPIO pin number
 * @param[in] mode  Mode to set the pin to, one of BCM2837_GPIO_FSEL_*
 * @return          0 on success
 */
int bcm2837_gpio_fsel(gpio_t *gpio, uint8_t mode);
