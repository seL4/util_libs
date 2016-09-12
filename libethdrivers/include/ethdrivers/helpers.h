/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef ETHIF_HELPERS_H
#define ETHIF_HELPERS_H

#include <stdint.h>
#include <platsupport/io.h>

typedef struct dma_addr {
    void *virt;
    uintptr_t phys;
}dma_addr_t;

/* Small wrapper that does ps_dma_alloc and then ps_dma_pin */
dma_addr_t dma_alloc_pin(ps_dma_man_t *dma_man, size_t size, int cached, int alignment);

/* Small wrapper than does ps_dma_unpin and then ps_dma_free */
void dma_unpin_free(ps_dma_man_t *dma_man, void *virt, size_t size);

#endif /* ETHIF_DMA_H */
