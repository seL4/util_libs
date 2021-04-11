/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2020, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ethdrivers/helpers.h>

dma_addr_t dma_alloc_pin(ps_dma_man_t *dma_man, size_t size, int cached, int alignment)
{
    void *virt = ps_dma_alloc(dma_man, size, alignment, cached, PS_MEM_NORMAL);
    if (!virt) {
        ZF_LOGE("ps_dma_alloc() failed for size=%zu, alignment=%d",
                size, alignment);
        return (dma_addr_t) {
            .virt = NULL, .phys = 0
        };
    }
    uintptr_t phys = ps_dma_pin(dma_man, virt, size);
    if (!phys) {
        ZF_LOGE("ps_dma_pin() failed for virt=%p, size=%zu", virt, size);
        /* hmm this shouldn't really happen */
        ps_dma_free(dma_man, virt, size);
        return (dma_addr_t) {
            .virt = NULL, .phys = 0
        };
    }
    if (!cached) {
        /* Prevent any cache bombs */
        ps_dma_cache_clean_invalidate(dma_man, virt, size);
    }
    return (dma_addr_t) {
        .virt = virt, .phys = phys
    };
}

void dma_unpin_free(ps_dma_man_t *dma_man, void *virt, size_t size)
{
    ps_dma_unpin(dma_man, virt, size);
    ps_dma_free(dma_man, virt, size);
}
