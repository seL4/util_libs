
/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
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

#endif /* _PLATSUPPORT_PLAT_I2C_H_ */
