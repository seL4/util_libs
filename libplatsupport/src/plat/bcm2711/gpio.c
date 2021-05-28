/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <utils/util.h>
#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>
#include "../../services.h"

#define BCM2711_GPIO_PADDR  0xfe200000
#define BCM2711_GPIO_SIZE   0x1000

// Registers according to section 5.2 of BCM2711 TRM
typedef volatile struct bcm2711_gpio_regs {
    uint32_t gpfsel0;        /* +0x00 */
    uint32_t gpfsel1;        /* +0x04 */
    uint32_t gpfsel2;        /* +0x08 */
    uint32_t gpfsel3;        /* +0x0c */
    uint32_t gpfsel4;        /* +0x10 */
    uint32_t gpfsel5;        /* +0x14 */
    uint32_t unused0;
    uint32_t gpset0;         /* +0x1c */
    uint32_t gpset1;         /* +0x20 */
    uint32_t unused1;
    uint32_t gpclr0;         /* +0x28 */
    uint32_t gpclr1;         /* +0x2c */
    uint32_t unused2;
    uint32_t gplev0;         /* +0x34 */
    uint32_t gplev1;         /* +0x38 */
    uint32_t unused3;
    uint32_t gpeds0;         /* +0x40 */
    uint32_t gpeds1;         /* +0x44 */
    uint32_t unused4;
    uint32_t gpren0;         /* +0x4c */
    uint32_t gpren1;         /* +0x50 */
    uint32_t unused5;
    uint32_t gpfen0;         /* +0x58 */
    uint32_t gpfen1;         /* +0x5c */
    uint32_t unused6;
    uint32_t gphen0;         /* +0x64 */
    uint32_t gphen1;         /* +0x68 */
    uint32_t unused7;
    uint32_t gplen0;         /* +0x70 */
    uint32_t gplen1;         /* +0x74 */
    uint32_t unused8;
    uint32_t gparen0;        /* +0x7c */
    uint32_t gparen1;        /* +0x80 */
    uint32_t unused9;
    uint32_t gpafen0;        /* +0x88 */
    uint32_t gpafen1;        /* +0x8c */
    uint32_t unused10;
    uint32_t gppud;          /* +0x94 */
    uint32_t gppudclk0;      /* +0x98 */
    uint32_t gppudclk1;      /* +0x9c */
} bcm2711_gpio_regs_t;

static struct bcm2711_gpio {
    bcm2711_gpio_regs_t *bank[GPIO_NBANKS];
} gpio_ctx;

static bcm2711_gpio_regs_t *bcm2711_gpio_get_bank(gpio_t *gpio)
{
    assert(gpio);
    assert(gpio->gpio_sys);
    assert(gpio->gpio_sys->priv);
    struct bcm2711_gpio *gpio_priv = (struct bcm2711_gpio *)gpio->gpio_sys->priv;
    int port = 0;
    return gpio_priv->bank[port];
}

static int bcm2711_gpio_init(gpio_sys_t *gpio_sys, int id, enum gpio_dir dir, gpio_t *gpio)
{
    assert(gpio);
    assert(gpio_sys);
    struct bcm2711_gpio *gpio_priv = (struct bcm2711_gpio *)gpio_sys->priv;
    assert(gpio_priv);
    if (id < 0 || id > MAX_GPIO_ID) {
        ZF_LOGE("GPIO Pin ID is not in valid range!");
        return -1;
    }

    gpio->id = id;
    gpio->gpio_sys = gpio_sys;

    return 0;
}

/*
 * GPIO set level function
 *
 * See BCM2711 TRM, section 5 General Purpose I/O (GPIO):
 * The bcm2711 SoC features 58 GPIO pins. The level of each of these pins can
 * be set with the registers gpset0 (0x1c), gpset1 (0x20) and can be cleared
 * with the registers gpclr0 (0x28), gpclr1 (0x2c). Each bit in one of these
 * GPIO registers is responsible for setting/clearing the level of a GPIO pin.
 * Separating the set and clear functions removes the need for read-modify-write
 * operations.
 *
 * A prerequisite for using this function is to configure the respective GPIO
 * pin to OUT (with the bcm2711_gpio_fsel function).
 */
static int bcm2711_gpio_set_level(gpio_t *gpio, enum gpio_level level)
{
    bcm2711_gpio_regs_t *bank = bcm2711_gpio_get_bank(gpio);
    assert(bank);
    int pin = gpio->id;
    if (pin < 0 || pin > MAX_GPIO_ID) {
        ZF_LOGE("GPIO Pin ID is not in valid range!");
        return -1;
    }

    volatile uint32_t *paddr = (level == GPIO_LEVEL_HIGH) ? &(bank->gpset0) + (pin / 32)
                               : &(bank->gpclr0) + (pin / 32);
    uint8_t shift = (pin % 32);
    *paddr = (1u << shift);

    return 0;
}

/*
 * GPIO read level function
 *
 * See BCM2711 TRM, section 5 General Purpose I/O (GPIO):
 * The bcm2711 SoC features 58 GPIO pins. The level of each of these pins can
 * be read with the registers gplev0 (0x34) and gplev1 (0x38). Each bit in one
 * of these GPIO registers is responsible for reading the level of a GPIO pin.
 * Separating the set and clear functions removes the need for read-modify-write
 * operations.
 *
 * A prerequisite for using this function is to configure the respective GPIO
 * pin to OUT (with the bcm2711_gpio_fsel function).
 */
static int bcm2711_gpio_read_level(gpio_t *gpio)
{
    bcm2711_gpio_regs_t *bank = bcm2711_gpio_get_bank(gpio);
    assert(bank);
    int pin = gpio->id;
    if (pin < 0 || pin > MAX_GPIO_ID) {
        ZF_LOGE("GPIO Pin ID is not in valid range!");
        return -1;
    }

    volatile uint32_t *paddr = &(bank->gplev0) + (pin / 32);
    uint8_t shift = pin % 32;
    uint32_t value = *paddr;

    return (value & (1u << shift)) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

/*
 * GPIO function select
 *
 * See BCM2711 TRM, section 5.2 Register View:
 * The bcm2711 has 6 GPIO function select registers. Each of these control
 * registers controls 10 GPIO pins of a total of 58 GPIO pins. As a result each
 * of these control registers has 10 sets of 3 bits per GPIO pin. Depending on
 * the mode given, these 3 bits are set accordingly for a specific pin i:
 *
 * 000 = GPIO Pin i is an input
 * 001 = GPIO Pin i is an output
 * 100 = GPIO Pin i takes alternate function 0
 * 101 = GPIO Pin i takes alternate function 1
 * 110 = GPIO Pin i takes alternate function 2
 * 111 = GPIO Pin i takes alternate function 3
 * 011 = GPIO Pin i takes alternate function 4
 * 010 = GPIO Pin i takes alternate function 5
 *
 * To calculate the 3 bits for Pin i use the following formula:
 *      i / 10 + ((i % 10) * 3)
 */
int bcm2711_gpio_fsel(gpio_t *gpio, uint8_t mode)
{
    bcm2711_gpio_regs_t *bank = bcm2711_gpio_get_bank(gpio);
    assert(bank);
    int pin = gpio->id;
    if (pin < 0 || pin > MAX_GPIO_ID) {
        ZF_LOGE("GPIO Pin ID is not in valid range!");
        return -1;
    }

    volatile uint32_t *paddr = &(bank->gpfsel0) + (pin / 10);
    uint8_t  shift = (pin % 10) * 3;
    uint32_t mask  = BCM2711_GPIO_FSEL_MASK << shift;
    uint32_t value = mode << shift;

    uint32_t v = *paddr;
    v = (v & ~mask) | (value & mask);
    *paddr = v;

    return 0;
}

int bcm2711_gpio_init_common(gpio_sys_t *gpio_sys)
{
    gpio_sys->priv = (void *)&gpio_ctx;
    gpio_sys->set_level = &bcm2711_gpio_set_level;
    gpio_sys->read_level = &bcm2711_gpio_read_level;
    gpio_sys->init = &bcm2711_gpio_init;
    return 0;
}

int bcm2711_gpio_sys_init(void *bank1, gpio_sys_t *gpio_sys)
{
    if (bank1 != NULL) {
        gpio_ctx.bank[GPIO_BANK1] = bank1;
    }
    return bcm2711_gpio_init_common(gpio_sys);
}

int gpio_sys_init(ps_io_ops_t *io_ops, gpio_sys_t *gpio_sys)
{
    MAP_IF_NULL(io_ops, BCM2711_GPIO, gpio_ctx.bank[0]);
    return bcm2711_gpio_init_common(gpio_sys);
}
