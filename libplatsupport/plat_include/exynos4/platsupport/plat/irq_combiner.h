/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

enum irq_combiner_id {
    IRQ_COMBINER0,
    NIRQ_COMBINERS
};

#define EXYNOS4_IRQ_COMBINER_PADDR 0x10440000
#define EXYNOS4_IRQ_COMBINER_SIZE  0x1000

#define EXYNOS_IRQ_COMBINER_PADDR EXYNOS4_IRQ_COMBINER_PADDR
#define EXYNOS_IRQ_COMBINER_SIZE  EXYNOS4_IRQ_COMBINER_SIZE

/**
 * Initialise the IRQ combiner with a provided address for IO access
 * @param[in]  base     The memory address of the combiner registers
 * @param[out] combiner An IRQ combiner structure to populate
 * @return              0 on success.
 */
int exynos_irq_combiner_init(void *base, irq_combiner_t *combiner);

