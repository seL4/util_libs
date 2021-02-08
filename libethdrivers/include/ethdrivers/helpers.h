/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

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

