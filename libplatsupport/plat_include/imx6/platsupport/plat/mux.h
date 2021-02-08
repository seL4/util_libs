/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum mux_feature {
    MUX_I2C1,
    MUX_I2C2,
    MUX_I2C3,
    MUX_GPIO0_CLKO1,
    MUX_UART1,
    NMUX_FEATURES
};

/**
 * Initialise the mux subsystem with pre-mapped regions.
 * @param[in]  iomuxc A virtual mapping for IOMUXC of the MUX subsystem.
 * @param[out] mux    On success, this will be filled with the appropriate
 *                    subsystem data.
 * @return            0 on success
 */
int imx6_mux_init(void* iomuxc, mux_sys_t* mux);

