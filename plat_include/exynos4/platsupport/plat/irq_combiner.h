/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _PLATSUPPORT_PLAT_IRQ_COMBINER_H_
#define _PLATSUPPORT_PLAT_IRQ_COMBINER_H_

enum irq_combiner_id {
    IRQ_COMBINER0,
    NIRQ_COMBINERS
};


#define EXYNOS4_IRQ_COMBINER_PADDR 0x10440000
#define EXYNOS4_IRQ_COMBINER_SIZE  0x1000

#define EXYNOS_IRQ_COMBINER_PADDR EXYNOS4_IRQ_COMBINER_PADDR
#define EXYNOS_IRQ_COMBINER_SIZE  EXYNOS4_IRQ_COMBINER_SIZE

#endif /* _PLATSUPPORT_PLAT_IRQ_COMBINER_H_ */
