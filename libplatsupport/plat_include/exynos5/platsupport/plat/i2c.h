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

#ifndef _PLATSUPPORT_I2C_H_
#error This file should not be included directly
#endif

enum i2c_id {
    I2C0,
    I2C1,
    I2C2,
    I2C3,
    I2C4,
    I2C5,
    I2C6,
    I2C7,
    I2C8,
    I2C9,
    I2C10,
    I2C11,
    NI2C,
    I2C_HDMI = I2C8,
    I2C0_ISP = I2C9,
    I2C1_ISP = I2C10,
    I2C_SATAPHY = I2C11
};

/**
 * Initalise an exynos I2C bus with memory mapped i2c deviced
 * @param[in]   id      The id of the I2C bus to initalise
 * @param[in]   base    The base address of the i2c device's mapped memory
 * @param[in]   mux     Mux system for exynos
 * @param[out]  i2c     I2C bus struct to be populated
 */
int exynos_i2c_init(enum i2c_id id, void* base, mux_sys_t* mux, i2c_bus_t* i2c);

