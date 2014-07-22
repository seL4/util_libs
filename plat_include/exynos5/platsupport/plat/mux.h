/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _PLATSUPPORT_PLAT_MUX_H
#define _PLATSUPPORT_PLAT_MUX_H

enum mux_feature {
    MUX_I2C0,
    MUX_I2C1,
    MUX_I2C2,
    MUX_I2C3,
    MUX_I2C4,
    MUX_I2C5,
    MUX_I2C6,
    MUX_I2C7,
    MUX_I2C8,
    MUX_I2C9,
    MUX_I2C10,
    MUX_I2C11,
    MUX_UART0,
    MUX_UART0_FLOW,
    MUX_UART1,
    MUX_UART1_FLOW,
    MUX_UART2,
    MUX_UART2_FLOW,
    MUX_UART3,
    MUX_UART3_FLOW,
    MUX_SPI0,
    MUX_SPI1,
    MUX_SPI2,
    NMUX_FEATURES,
    MUX_I2C_HDMI    = MUX_I2C8,
    MUX_I2C_0_ISP   = MUX_I2C9,
    MUX_I2C_1_ISP   = MUX_I2C10,
    MUX_I2C_SATAPHY = MUX_I2C11
};

/**
 * Initialise the mux subsystem with pre-mapped regions.
 * @param[in]  gpioleft  A virtual mapping for the left part of the MUX subsystem.
 * @param[in]  gpioright A virtual mapping for the right part of the MUX subsystem.
 * @param[in]  gpioc2c   A virtual mapping for c2c part of the MUX subsystem.
 * @param[in]  gpioaudio A virtual mapping for audio part of the MUX subsystem.
 * @param[out] mux    On success, this will be filled with the appropriate
 *                    subsystem data.
 * @return            0 on success
 */
int exynos5_mux_init(void* gpioleft,
                     void* gpioright,
                     void* gpioc2c,
                     void* gpioaudio,
                     mux_sys_t* mux);

#endif /* _PLATSUPPORT_PLAT_MUX_H */
