/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_MUX_H__
#define __PLATSUPPORT_PLAT_MUX_H__

enum mux_feature {
    MUX_I2C0,
    MUX_I2C1,
    MUX_I2C2,
    MUX_I2C3,
    MUX_I2C4,
    MUX_I2C5,
    MUX_I2C6,
    MUX_I2C7,
#if 0
    MUX_UART0,
    MUX_UART1,
    MUX_UART2,
    MUX_UART3,
    MUX_UART4,
#endif
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
int exynos4_mux_init(void* bank1,
                     void* bank2,
                     void* bank3,
                     void* bank4,
                     mux_sys_t* mux);


#endif /* __PLATSUPPORT_PLAT_MUX_H__ */
