/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum i2c_id {
    I2C0,
    I2C1,
    I2C2,
    I2C3,
    I2C4,
    I2C5,
    I2C6,
    I2C7,
    NI2C
};

/**
 * Initalise an exynos I2C bus with memory mapped i2c deviced
 * @param[in]   id      The id of the I2C bus to initalise
 * @param[in]   base    The base address of the i2c device's mapped memory
 * @param[in]   mux     Mux system for exynos
 * @param[out]  i2c     I2C bus struct to be populated
 */
int exynos_i2c_init(enum i2c_id id, void* base, mux_sys_t* mux, i2c_bus_t* i2c);

