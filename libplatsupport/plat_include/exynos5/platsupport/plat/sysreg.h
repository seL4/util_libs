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

#include <platsupport/io.h>

#define EXYNOS5_POWER_PADDR    0x10040000
#define EXYNOS5_POWER_SIZE     0x5000
#define EXYNOS5_SYSREG_PADDR   0x10050000
#define EXYNOS5_SYSREG_SIZE    0x1000

struct sysreg {
    void* sysreg_vaddr[1];
    void* pwrreg_vaddr[5];
};
typedef struct sysreg sysreg_t;

/**
 * Initialise the system register subsystem
 * @param[in]  ops     a structure containing OS specific operations for memory access
 * @param[out] sysreg  A sysreg structure to initialise
 * @return             0 on success
 */
int exynos5_sysreg_init(ps_io_ops_t* ops, sysreg_t* sysreg);

enum usb_phy_id {
    USBPHY_USB2,
    USBPHY_NUSBPHY
};

/**
 * Enable power for USB PHYs
 * @param[in] phy_id  The ID of the phy to power up
 * @param[in] sysreg  A handle to the sysreg subsystem
 * @return            0 on success
 */
int exynos5_sysreg_usbphy_enable(enum usb_phy_id phy_id, sysreg_t* sysreg);

