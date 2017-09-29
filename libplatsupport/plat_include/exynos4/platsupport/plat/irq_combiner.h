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
int exynos_irq_combiner_init(void* base, irq_combiner_t* combiner);

