/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <platsupport/src.h>
#include "../../services.h"

/** PWRREG **/
#define PWRREG_PHY_CONTROL_OFFSET  0x708
#define PWRREG_PHY_CONTROL_PHY_EN  BIT(0)
#define PWRREG_SWRST_OFFSET        0x400
#define PWRREG_SWRST               BIT(0)

/** SYSREG **/
#define SYSREG_PHY_CONFIG_OFFSET   0x230
#define SYSREG_PHY_CONFIG_PHY_EN   BIT(0)

struct src_priv {
    void *sysreg_vaddr[1];
    void *pwrreg_vaddr[5];
};

struct src_priv _src_priv;

static inline struct src_priv *src_get_priv(src_dev_t *d)
{
    return (struct src_priv *)d->priv;
}

static inline volatile uint32_t *sreg(struct src_priv *src, int offset)
{
    void *reg = src->sysreg_vaddr[offset >> 12] + (offset & MASK(12));
    return (volatile uint32_t *)reg;
}

static inline volatile uint32_t *preg(struct src_priv *src, int offset)
{
    void *reg_base;
    reg_base = src->pwrreg_vaddr[offset >> 12];
    if (reg_base) {
        return (volatile uint32_t *)(reg_base + (offset & MASK(12)));
    } else {
        return NULL;
    }
}

int sysreg_usbphy_enable(src_dev_t *dev)
{
    volatile uint32_t *a;
    struct src_priv *priv = src_get_priv(dev);
    a = sreg(priv, SYSREG_PHY_CONFIG_OFFSET);
    if (a) {
        *a |= SYSREG_PHY_CONFIG_PHY_EN;
    }
    a = preg(priv, PWRREG_PHY_CONTROL_OFFSET);
    if (a) {
        *a |= PWRREG_PHY_CONTROL_PHY_EN;
    }
    return 0;
}

int sysreg_swrst_enable(src_dev_t *dev)
{
    struct src_priv *priv = src_get_priv(dev);
    volatile uint32_t *a;
    a = preg(priv, PWRREG_SWRST_OFFSET);
    if (a) {
        LOG_INFO("Software reset triggered");
        fflush(stdout);
        *a = PWRREG_SWRST;
        while (1);
    }
    return 0;
}

void reset_controller_assert_reset(src_dev_t *dev, enum src_rst_id id)
{
    switch (id) {
    case SRCRST_SW_RST:
        sysreg_swrst_enable(dev);
        break;
    case SRCRST_USBPHY_EN:
        sysreg_usbphy_enable(dev);
        break;
    default:
        LOG_ERROR("Invalid option: %d", id);
    }
}

int reset_controller_init(enum src_id id, ps_io_ops_t *ops, src_dev_t *dev)
{
    struct src_priv *src_priv = &_src_priv;
    int i;
    /* Sanity check the provided ID */
    if (id < 0 || id >= NSRC) {
        return -1;
    }
    /* Map sysreg memory */
    for (i = 0; i < EXYNOS_SYSREG_SIZE >> 12; i++) {
        if (src_priv->sysreg_vaddr[i] == NULL) {
            void *vaddr;
            vaddr = ps_io_map(&ops->io_mapper,
                              EXYNOS_SYSREG_PADDR + i * BIT(12),
                              0x1000, 0, PS_MEM_NORMAL);
            src_priv->sysreg_vaddr[i] = vaddr;
        }
    }
    /* Map pmu memory */
    for (i = 0; i < EXYNOS_PMU_SIZE >> 12; i++) {
        if (src_priv->pwrreg_vaddr[i] == NULL) {
            void *vaddr;
            vaddr = ps_io_map(&ops->io_mapper,
                              EXYNOS_PMU_PADDR + i * BIT(12),
                              0x1000, 0, PS_MEM_NORMAL);

            src_priv->pwrreg_vaddr[i] = vaddr;
        }
    }
    /* Assign private data */
    dev->priv = src_priv;
    return 0;
}
