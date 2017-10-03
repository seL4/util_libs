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

#define EXYNOS_GPIOLEFT_PADDR    0x11400000
#define EXYNOS_GPIORIGHT_PADDR   0x11000000
#define EXYNOS_GPIOC2C_PADDR     0x106E0000
#define EXYNOS_GPIOAUDIO_PADDR   0x03860000

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
    MUX_SPI0,
    MUX_SPI1,
    MUX_SPI2,
    MUX_UART0,
    MUX_UART0_FLOW,
    MUX_UART1,
    MUX_UART1_FLOW,
    MUX_UART2,
    MUX_UART2_FLOW,
    MUX_UART3,
    MUX_UART3_FLOW,
    MUX_SROM,
    MUX_EBI,
    NMUX_FEATURES
};

/**
 * Initialise the mux subsystem with pre-mapped regions.
 * @param[in]  bankX  A virtual mapping for bank X of the MUX subsystem.
 *                    bank3 and bank4 are present for compatibility reasons
 *                    only and can be passed as NULL.
 * @param[out] mux    On success, this will be filled with the appropriate
 *                    subsystem data.
 * @return            0 on success
 */
int exynos_mux_init(void* bank1,
                    void* bank2,
                    void* bank3,
                    void* bank4,
                    mux_sys_t* mux);

