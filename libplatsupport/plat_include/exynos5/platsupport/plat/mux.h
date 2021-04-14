/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <autoconf.h>
#include <platsupport/gen_config.h>

#if defined(CONFIG_PLAT_EXYNOS5250)
/* These are for the arndale */
#define EXYNOS_GPIOLEFT_PADDR    0x11400000
#define EXYNOS_GPIORIGHT_PADDR   0x13400000
#define EXYNOS_GPIOC2C_PADDR     0x10D10000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000
#elif defined(CONFIG_PLAT_EXYNOS54XX)
/* These are for Odroid-XU and XU3 */
#define EXYNOS_GPIOLEFT_PADDR    0x13400000
#define EXYNOS_GPIORIGHT_PADDR   0x14000000
#define EXYNOS_GPIOC2C_PADDR     0x10D10000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000
#else
#error Unidentified Exynos5 SoC
#endif

#define EXYNOS_GPIOX_SIZE        0x1000
#define EXYNOS_GPIOLEFT_SIZE     EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIORIGHT_SIZE    EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIOC2C_SIZE      EXYNOS_GPIOX_SIZE
#define EXYNOS_GPIOAUDIO_SIZE    EXYNOS_GPIOX_SIZE

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
    MUX_SPI0_ISP,
    MUX_SPI1_ISP,
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
int exynos_mux_init(void *gpioleft,
                    void *gpioright,
                    void *gpioc2c,
                    void *gpioaudio,
                    mux_sys_t *mux);

