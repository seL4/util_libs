/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <utils/util.h>
#include <platsupport/irq_combiner.h>
#include "../../services.h"


#define NGROUPS 32

struct combiner_gmap {
    uint32_t enable_set;
    uint32_t enable_clr;
    uint32_t status;
    uint32_t masked_status;
};

struct irq_combiner_map {
    struct combiner_gmap g[NGROUPS / 4];
    uint32_t res[32];
    uint32_t pending;
};

volatile struct irq_combiner_map *_combiner_regs = NULL;

#define GROUP_INDEX(cirq) (COMBINER_IRQ_GET_GROUP(cirq) >> 2)
#define GROUP_SHIFT(cirq) ((COMBINER_IRQ_GET_GROUP(cirq) & 0x3) * 8)
#define IRQ_SHIFT(cirq)   (COMBINER_IRQ_GET_INDEX(cirq))
#define GROUP_INDEX_MASK  0xff

static volatile struct irq_combiner_map*
irq_combiner_get_regs(irq_combiner_t* combiner) {
    assert(combiner);
    assert(combiner->priv);
    return (volatile struct irq_combiner_map*)combiner->priv;
}

static int
exynos_irq_combiner_is_pending(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    volatile struct irq_combiner_map* regs;
    int gidx, shift;
    regs = irq_combiner_get_regs(combiner);
    gidx = GROUP_INDEX(cirq);
    shift = GROUP_SHIFT(cirq) + IRQ_SHIFT(cirq);
    return !!(regs->g[gidx].masked_status & BIT(shift));
}

static int
exynos_irq_combiner_is_enabled(irq_combiner_t* combiner, combiner_irq_t cirq)
{
    volatile struct irq_combiner_map* regs;
    int gidx, shift;
    regs = irq_combiner_get_regs(combiner);
    gidx = GROUP_INDEX(cirq);
    shift = GROUP_SHIFT(cirq) + IRQ_SHIFT(cirq);
    return !!(regs->g[gidx].enable_set & BIT(shift));
}

static int
exynos_irq_combiner_set_enabled(irq_combiner_t* combiner, combiner_irq_t cirq, int v)
{
    volatile struct irq_combiner_map* regs;
    int gidx, shift;
    regs = irq_combiner_get_regs(combiner);
    gidx = GROUP_INDEX(cirq);
    shift = GROUP_SHIFT(cirq) + IRQ_SHIFT(cirq);
    if (v) {
        regs->g[gidx].enable_set = BIT(shift);
    } else {
        regs->g[gidx].enable_clr = BIT(shift);
    }
    return 0;
}

static uint32_t
exynos_irq_combiner_grp_pending(irq_combiner_t* combiner, int group)
{
    volatile struct irq_combiner_map* regs;
    combiner_irq_t cirq;
    int gidx, shift;
    uint32_t v;
    regs = irq_combiner_get_regs(combiner);
    cirq = COMBINER_IRQ(group, 0);
    gidx = GROUP_INDEX(cirq);
    shift = GROUP_SHIFT(cirq);
    v = regs->g[gidx].masked_status;
    return (v >> shift) & GROUP_INDEX_MASK;
}

static int
irq_combiner_init_common(irq_combiner_t* combiner)
{
    if (_combiner_regs == NULL) {
        return -1;
    } else {
        /* Initialise the structure */
        combiner->priv = (void*)_combiner_regs;
        combiner->is_pending  = &exynos_irq_combiner_is_pending;
        combiner->is_enabled  = &exynos_irq_combiner_is_enabled;
        combiner->set_enabled = &exynos_irq_combiner_set_enabled;
        combiner->grp_pending = &exynos_irq_combiner_grp_pending;
        return 0;
    }
}

int
exynos_irq_combiner_init(void* base, irq_combiner_t* combiner)
{
    if (base) {
        _combiner_regs = (volatile struct irq_combiner_map *)base;
    }
    return irq_combiner_init_common(combiner);
}

int
irq_combiner_init(enum irq_combiner_id id, ps_io_ops_t* io_ops, irq_combiner_t* combiner)
{
    /* Map memory */
    ZF_LOGD("Mapping device ID %d\n", id);
    switch (id) {
    case IRQ_COMBINER0:
        MAP_IF_NULL(io_ops, EXYNOS_IRQ_COMBINER, _combiner_regs);
        break;
    default:
        return -1;
    }

    return irq_combiner_init_common(combiner);
}

int
irq_combiner_nirqs(enum irq_combiner_id id)
{
    switch (id) {
    case IRQ_COMBINER0:
        return 32;
    default:
        return -1;
    }
}

int irq_combiner_irq(enum irq_combiner_id id, int group)
{
    switch (id) {
    case IRQ_COMBINER0:
        return group + 32;
    default:
        return -1;
    }
}
