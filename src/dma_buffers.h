/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __DMA_BUFFER_H
#define __DMA_BUFFER_H

#include "descriptors.h"

dma_addr_t dma_alloc_pin(ps_dma_man_t *dma_man, size_t size, int cached, int alignment);
void free_dma_buf(struct desc *desc, dma_addr_t buf);
dma_addr_t alloc_dma_buf(struct desc *desc);
int fill_dma_pool(struct desc *desc, int count, int buf_size, int alignment);

#endif
