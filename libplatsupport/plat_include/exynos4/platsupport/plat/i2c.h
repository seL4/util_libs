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

#ifndef _PLATSUPPORT_PLAT_I2C_H_
#define _PLATSUPPORT_PLAT_I2C_H_


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

#endif /* _PLATSUPPORT_PLAT_I2C_H_ */
