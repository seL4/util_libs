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

#define XEINT0   GPIOID(GPX0, 0)
#define XEINT1   GPIOID(GPX0, 1)
#define XEINT2   GPIOID(GPX0, 2)
#define XEINT3   GPIOID(GPX0, 3)
#define XEINT4   GPIOID(GPX0, 4)
#define XEINT5   GPIOID(GPX0, 5)
#define XEINT6   GPIOID(GPX0, 6)
#define XEINT7   GPIOID(GPX0, 7)
#define XEINT8   GPIOID(GPX1, 0)
#define XEINT9   GPIOID(GPX1, 1)
#define XEINT10  GPIOID(GPX1, 2)
#define XEINT11  GPIOID(GPX1, 3)
#define XEINT12  GPIOID(GPX1, 4)
#define XEINT13  GPIOID(GPX1, 5)
#define XEINT14  GPIOID(GPX1, 6)
#define XEINT15  GPIOID(GPX1, 7)
#define XEINT16  GPIOID(GPX2, 0)
#define XEINT17  GPIOID(GPX2, 1)
#define XEINT18  GPIOID(GPX2, 2)
#define XEINT19  GPIOID(GPX2, 3)
#define XEINT20  GPIOID(GPX2, 4)
#define XEINT21  GPIOID(GPX2, 5)
#define XEINT22  GPIOID(GPX2, 6)
#define XEINT23  GPIOID(GPX2, 7)
#define XEINT24  GPIOID(GPX3, 0)
#define XEINT25  GPIOID(GPX3, 1)
#define XEINT26  GPIOID(GPX3, 2)
#define XEINT27  GPIOID(GPX3, 3)
#define XEINT28  GPIOID(GPX3, 4)
#define XEINT29  GPIOID(GPX3, 5)
#define XEINT30  GPIOID(GPX3, 6)
#define XEINT31  GPIOID(GPX3, 7)





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
    GPF0 = GPIOPORT(LEFT,  12), /* 0x180 */
    GPF1 = GPIOPORT(LEFT,  13), /* 0x1A0 */
    GPF2 = GPIOPORT(LEFT,  14), /* 0x1C0 */
    GPF3 = GPIOPORT(LEFT,  15), /* 0x1E0 */
    ETC1 = GPIOPORT(LEFT,  17), /* 0x220 */
    GPJ0 = GPIOPORT(LEFT,  18), /* 0x240 */
    GPJ1 = GPIOPORT(LEFT,  19), /* 0x260 */

    /* RIGHT */
    GPK0 = GPIOPORT(RIGHT,  2), /* 0x040 */
    GPK1 = GPIOPORT(RIGHT,  3), /* 0x060 */
    GPK2 = GPIOPORT(RIGHT,  4), /* 0x080 */
    GPK3 = GPIOPORT(RIGHT,  5), /* 0x0A0 */
    GPL0 = GPIOPORT(RIGHT,  6), /* 0x0C0 */
    GPL1 = GPIOPORT(RIGHT,  7), /* 0x0E0 */
    GPL2 = GPIOPORT(RIGHT,  8), /* 0x100 */
    GPY0 = GPIOPORT(RIGHT,  9), /* 0x120 */
    GPY1 = GPIOPORT(RIGHT, 10), /* 0x140 */
    GPY2 = GPIOPORT(RIGHT, 11), /* 0x160 */
    GPY3 = GPIOPORT(RIGHT, 12), /* 0x180 */
    GPY4 = GPIOPORT(RIGHT, 13), /* 0x1A0 */
    GPY5 = GPIOPORT(RIGHT, 14), /* 0x1C0 */
    GPY6 = GPIOPORT(RIGHT, 15), /* 0x1E0 */
    ETC0 = GPIOPORT(RIGHT, 16), /* 0x200 */
    GPM0 = GPIOPORT(RIGHT, 19), /* 0x260 */
    GPM1 = GPIOPORT(RIGHT, 20), /* 0x280 */
    GPM2 = GPIOPORT(RIGHT, 21), /* 0x2A0 */
    GPM3 = GPIOPORT(RIGHT, 22), /* 0x2C0 */
    GPM4 = GPIOPORT(RIGHT, 23), /* 0x2E0 */
    GPX0 = GPIOPORT(RIGHT, 96), /* 0xC00 */
    GPX1 = GPIOPORT(RIGHT, 97), /* 0xC20 */
    GPX2 = GPIOPORT(RIGHT, 98), /* 0xC40 */
    GPX3 = GPIOPORT(RIGHT, 99), /* 0xC60 */
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

