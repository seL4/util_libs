/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define TMU_IRQ0 COMBINER_IRQ(-1, -1)
#define TMU_IRQ1 COMBINER_IRQ(-1, -1)

#define EXYNOS_TMU_PADDR  0x0
#define EXYNOS_TMU_SIZE   0x0

enum tmu_id {
    TMU_CORE0_CLUSTER0,
    TMU_CORE1_CLUSTER0,
    TMU_CORE2_CLUSTER0,
    TMU_CORE3_CLUSTER0,
    TMU_CORE4_CLUSTER0,
    TMU_CORE0_CLUSTER1,
    TMU_CORE1_CLUSTER1,
    TMU_CORE2_CLUSTER1,
    TMU_CORE3_CLUSTER1,
    TMU_CORE4_CLUSTER1,
    NTMU,
    /* Aliases */
    TMU_CORE0 = TMU_CORE0_CLUSTER0,
    TMU_CORE1 = TMU_CORE1_CLUSTER0,
    TMU_CORE2 = TMU_CORE2_CLUSTER0,
    TMU_CORE3 = TMU_CORE3_CLUSTER0,
    TMU_CORE4 = TMU_CORE4_CLUSTER0,
    TMU_CORE = TMU_CORE0
};

/**
 * Initialise a Thermal Management Unit
 * @param[in]  id    The ID of the TMU that is to be initialised
 * @param[in]  vaddr The virtual address of the TMU
 * @param[out] tmu   A TMU structure to populate
 * @return           0 on success
 */
int exynos4_tmu_init(enum tmu_id id, void *vaddr, tmu_t *tmu);

