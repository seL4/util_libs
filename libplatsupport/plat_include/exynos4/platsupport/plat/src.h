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


#ifndef PLATSUPPORT_PLAT_SRC_H
#define PLATSUPPORT_PLAT_SRC_H

#ifndef SRC_H
#error This file should not be included directly
#endif

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

#endif /* PLATSUPPORT_PLAT_SRC_H */
