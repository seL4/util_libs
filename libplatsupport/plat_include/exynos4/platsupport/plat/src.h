/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* IRQS */

/* Physical addresses */
#define EXYNOS_SYSREG_PADDR    0x10010000
#define EXYNOS_PMU_PADDR       0x10020000

/* Sizes */
#define EXYNOS_SYSREG_SIZE     0x1000
#define EXYNOS_PMU_SIZE        0x5000

enum src_id {
    SRC1,
    /* ---- */
    NSRC,
    SRC_DEFAULT = SRC1
};

enum src_rst_id {
    SRCRST_SW_RST,
    SRCRST_USBPHY_EN,
    /* ---- */
    NSRCRST
};

