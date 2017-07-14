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

#ifndef __PLATSUPPORT_PLAT_MUX_H__
#define __PLATSUPPORT_PLAT_MUX_H__

enum mux_feature {
    MUX_I2C1,
    MUX_I2C2,
    MUX_I2C3,
    MUX_GPIO0_CLKO1,
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

#endif /* __PLATSUPPORT_PLAT_MUX_H__ */
