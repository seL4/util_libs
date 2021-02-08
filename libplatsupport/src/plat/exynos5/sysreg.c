/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/plat/sysreg.h>
#include "../../services.h"

/** PWRREG **/
#define PWRREG_PHY_CONTROL_OFFSET  0x708
#define PWRREG_PHY_CONTROL_PHY_EN  BIT(0)

/** SYSREG **/
#define SYSREG_PHY_CONFIG_OFFSET   0x230
#define SYSREG_PHY_CONFIG_PHY_EN   BIT(0)

static inline volatile uint32_t*
sreg(sysreg_t* sysreg, int offset)
{
    void* reg = sysreg->sysreg_vaddr[offset >> 12] + (offset & MASK(12));
    return (volatile uint32_t*)reg;
}

static inline volatile uint32_t*
preg(sysreg_t* sysreg, int offset)
{
    void* reg_base;
    reg_base = sysreg->pwrreg_vaddr[offset >> 12];
    if (reg_base) {
        return (volatile uint32_t*)(reg_base + (offset & MASK(12)));
    } else {
        return NULL;
    }
}

int
exynos5_sysreg_init(ps_io_ops_t* ops, sysreg_t* sysreg)
{
    int i;
    for (i = 0; i < EXYNOS5_SYSREG_SIZE >> 12; i++) {
        if (sysreg->sysreg_vaddr[i] == NULL) {
            void *vaddr;
            vaddr = ps_io_map(&ops->io_mapper,
                              EXYNOS5_SYSREG_PADDR + i * BIT(12),
                              0x1000, 0, PS_MEM_NORMAL);
            sysreg->sysreg_vaddr[i] = vaddr;
        }
    }
    for (i = 0; i < EXYNOS5_POWER_SIZE >> 12; i++) {
        if (sysreg->pwrreg_vaddr[i] == NULL) {
            void *vaddr;
            vaddr = ps_io_map(&ops->io_mapper,
                              EXYNOS5_POWER_PADDR + i * BIT(12),
                              0x1000 , 0, PS_MEM_NORMAL);

            sysreg->pwrreg_vaddr[i] = vaddr;
        }
    }
    return 0;
}

int
exynos5_sysreg_usbphy_enable(enum usb_phy_id phy_id, sysreg_t* sysreg)
{
    volatile uint32_t* a;
    a = sreg(sysreg, SYSREG_PHY_CONFIG_OFFSET);
    if (a) {
        *a |= SYSREG_PHY_CONFIG_PHY_EN;
    }
    a = preg(sysreg, PWRREG_PHY_CONTROL_OFFSET);
    if (a) {
        *a |= PWRREG_PHY_CONTROL_PHY_EN;
    }
    return 0;
}
