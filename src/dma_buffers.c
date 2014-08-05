/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "raw_iface.h"
#include "descriptors.h"
#include <stdlib.h>

#define DMA_SIZE_ALIGN 16

dma_addr_t 
dma_alloc_pin(ps_dma_man_t *dma_man, size_t size, int cached) 
{
    void *virt = ps_dma_alloc(dma_man, size, DMA_SIZE_ALIGN, cached, PS_MEM_NORMAL);
    if (!virt) {
        return (dma_addr_t) {0, 0};
    }
    uintptr_t phys = ps_dma_pin(dma_man, virt, size);
    if (!phys) {
        /* hmm this shouldn't really happen */
        ps_dma_free(dma_man, virt, size);
        return (dma_addr_t) {0, 0};
    }
    if (!cached) {
        /* Prevent any cache bombs */
        ps_dma_cache_clean_invalidate(dma_man, virt, size);
    }
    return (dma_addr_t) {.virt = virt, .phys = phys};
}

void 
free_dma_buf(struct desc *desc, dma_addr_t buf) 
{
    assert(desc);
    assert(desc->queue_index > 0);
    assert(buf.virt);
    desc->queue_index--;
    desc->pool_queue[desc->queue_index] = buf;
}

dma_addr_t 
alloc_dma_buf(struct desc *desc)
{
    dma_addr_t ret;
    if (desc->queue_index == desc->pool_size) {
        assert(0); // die for now, we don't handle out of buffers...
        return (dma_addr_t){0, 0};
    }
    ret = desc->pool_queue[desc->queue_index];
    desc->queue_index++;
    return ret;
}

int 
fill_dma_pool(struct desc *desc, int count, int buf_size) 
{
    printf("Making dma pool, %d buffer of size %d\n", count, buf_size);
    int i;
    desc->pool_queue = malloc(sizeof(*(desc->pool_queue)) * count);
    if (!desc->pool_queue) {
        return 1;
    }
    for (i = 0; i < count; i++) {
        desc->pool_queue[i] = dma_alloc_pin(&desc->dma_man, buf_size, 1);
        if (!desc->pool_queue[i].virt) {
            return 1;
        }
    }
    desc->queue_index = 0;
    desc->pool_size = count;
    desc->buf_size = buf_size;
    return 0;
}
