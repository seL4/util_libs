/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "../../mach/exynos/mux.h"
#include <platsupport/plat/gpio.h>

static struct mux_feature_data i2c0_data[] = {
    { .port = GPD1, .pin = 0, .value = 2},
    { .port = GPD1, .pin = 1, .value = 2},
    { .port = GPIOPORT_NONE }
};
static struct mux_feature_data i2c1_data[] = {
    { .port = GPD1, .pin = 2, .value = 2},
    { .port = GPD1, .pin = 3, .value = 2},
    { .port = GPIOPORT_NONE }
};

struct mux_feature_data* feature_data[] = {
    [MUX_I2C0] = i2c0_data,
    [MUX_I2C1] = i2c1_data,
};


