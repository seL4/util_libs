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
#include "dma_buffers.h"

#define FALSE 0
#define TRUE 1

static int create_ring(struct desc_ring *r, ps_dma_man_t *dma_man, dma_addr_t descs,
                       int count) {
    /* create the buffer list */
    r->buf = (struct dma_addr*)malloc(sizeof(struct dma_addr) * count);
    if(r->buf == NULL){
        return 1;
    }
    r->buf_cookies = (void**)malloc(sizeof(void*) * count);
    if (!r->buf_cookies) {
        free(r->buf);
        return 1;
    }
    r->complete_funcs = (tx_complete_fn_t*)malloc(sizeof(tx_complete_fn_t) * count);
    if (!r->complete_funcs) {
        free(r->buf_cookies);
        free(r->buf);
        return 1;
    }
    r->ring = descs;
    r->count = count;
    return 0;
}

struct desc*
desc_init(ps_dma_man_t *dma_man, int rx_count, int tx_count, int buf_count,
          int buf_size, struct eth_driver *driver)
{
    struct desc *d;
    struct raw_desc_funcs *d_fn = driver->d_fn;
    int error;
    
    assert(buf_count > tx_count);
    d = malloc(sizeof(struct desc));
    if (!d) {
        return NULL;
    }
    d->dma_man = *dma_man;

    dma_addr_t tx_descs = d_fn->create_tx_descs(dma_man, tx_count);
    if (!tx_descs.virt) {
        LOG_ERROR("Failed to create tx descriptors. Panic");
        goto panic;
    }
    error = create_ring(&d->tx, dma_man, tx_descs, tx_count);
    if (error) {
        LOG_ERROR("Failed to create tx ring. Panic!");
        goto panic;
    }

    dma_addr_t rx_descs = d_fn->create_rx_descs(dma_man, rx_count);
    if (!rx_descs.virt) {
        LOG_ERROR("Failed to create rx descriptors. Panic");
        goto panic;
    }
    error = create_ring(&d->rx, dma_man, rx_descs, rx_count);
    if (error) {
        LOG_ERROR("Failed to create rx ring. Panic!");
        goto panic;
    }

    error = fill_dma_pool(d, buf_count, buf_size);
    if (error) {
        LOG_ERROR("Failed to fill dma pool. Panic!");
        goto panic;
    }

    driver->desc = d;
    desc_reset(driver);
    return d;
panic:
    assert(0);
    return NULL;
}

void
desc_reset(struct eth_driver *driver)   
{
    struct desc *desc = driver->desc;
    struct raw_desc_funcs *d_fn = driver->d_fn;
    /* reset head and tail */
    desc->rx.head = 0;
    desc->rx.tail = 0;
    desc->rx.unused = 0;
    desc->tx.head = 0;
    desc->tx.tail = 0;
    desc->tx.unused = desc->tx.count;
      
    /* Clear RX & TX desciptor data */
    d_fn->reset_rx_descs(driver);
    d_fn->reset_tx_descs(driver);
}

void
desc_free(struct desc *desc)
{
    // Not implemented
    assert(0);
}

int 
desc_txget(struct desc *desc, dma_addr_t *dma, struct raw_desc_funcs *d_fn)
{
    int i;
    assert(desc);
    assert(dma);

    i = desc->tx.tail;
    /* Clean stale buffers so we can try to reuse them */
    desc_txcomplete(desc, d_fn);
    if (desc->tx.unused == 0) {
        assert(0);
        return 0;
    }

    assert(!(d_fn->is_tx_desc_ready(i, desc)));   
 
    dma_addr_t dma_buf = alloc_dma_buf(desc);
    assert(dma_buf.virt);
    *dma = dma_buf;
    desc->tx.buf[i] = dma_buf;
    return desc->buf_size;
}

int
desc_txput(struct eth_driver *driver, dma_addr_t buf, int len)
{
    int i;
    assert(driver);
    struct desc *desc = driver->desc;
    assert(desc);
    struct raw_desc_funcs *d_fn = driver->d_fn;
    assert(desc->tx.unused > 0);

    i = desc->tx.tail;
    assert(!(d_fn->is_tx_desc_ready(i, desc)));
    assert(desc->tx.buf[i].phys == buf.phys);
    assert(desc->tx.buf[i].virt == buf.virt);

    /* Clean the buffer out to RAM */
    assert((uintptr_t)buf.virt % 16 == 0);
    ps_dma_cache_clean(&desc->dma_man, buf.virt, ROUND_UP(len, 32));


     /* Adjust for next index */
    desc->tx.tail++;
    desc->tx.unused--;
    int tx_desc_wrap = FALSE;
    if (desc->tx.tail == desc->tx.count) {
        desc->tx.tail = 0;
        tx_desc_wrap = TRUE;
    }
    d_fn->set_tx_desc_buf(i, buf, len, tx_desc_wrap, TRUE, desc);
    d_fn->ready_tx_desc(i, 1, driver);
    desc->tx.buf_cookies[i] = NULL;
    return 0;
}

int
desc_txhasspace(struct desc *desc, int count)
{
    assert(desc);
    return desc->tx.unused >= count + 1;
}

int
desc_txputmany(struct eth_driver *driver, ethif_scatter_buf_t *buf, tx_complete_fn_t func, 
               void *cookie)
{
    int i, j;
    struct desc *desc = driver->desc;
    struct raw_desc_funcs *d_fn = driver->d_fn;
    assert(desc);
    assert(desc_txhasspace(desc, buf->count));

    int start = desc->tx.tail;
    for (j = 0; j < buf->count; j++) {
        i = desc->tx.tail;
        assert(desc->tx.unused > 0);
        assert(!(d_fn->is_tx_desc_ready(i, desc)));
        /* Clean the buffer out to RAM */
        assert((uintptr_t)buf->bufs[j].buf.virt % 16 == 0);
        ps_dma_cache_clean(&desc->dma_man, buf->bufs[j].buf.virt, ROUND_UP(buf->bufs[j].len, 32));
        /* Adjust for next index */
        desc->tx.tail++;
        desc->tx.unused--;
        int tx_desc_wrap = FALSE;
        int tx_last_section = FALSE;
        if(desc->tx.tail == desc->tx.count){
            tx_desc_wrap = TRUE;
            desc->tx.tail = 0;
        }
        if (j + 1 == buf->count) {
            tx_last_section = TRUE;
            desc->tx.buf_cookies[i] = cookie;
            desc->tx.complete_funcs[i] = func;
        } else {
            desc->tx.buf_cookies[i] = NULL;
        }
        /* Set the descriptor */
        d_fn->set_tx_desc_buf(i, buf->bufs[j].buf, buf->bufs[j].len, tx_desc_wrap, tx_last_section, desc);
    }
    /* Ready all the descriptors */
    d_fn->ready_tx_desc(start, buf->count, driver);
    return 0;
}

static void
desc_rxrefill(struct eth_driver *driver) {
    struct desc *desc = driver->desc;
    struct raw_desc_funcs *d_fn = driver->d_fn;
    int i;
    while (desc->rx.unused > 0) {
        i = desc->rx.head;
        assert(!(d_fn->is_rx_desc_empty(i, desc)));
        dma_addr_t dma_buf = alloc_dma_buf(desc);
        if (!dma_buf.phys) {
            /* filled all we can */
            return;
        }
        desc->rx.buf[i] = dma_buf;
        /* Update descriptor */
        d_fn->set_rx_desc_buf(i, dma_buf, desc->buf_size, desc);
        /* Invalidate the buffer */
        assert((uintptr_t)dma_buf.virt % 16 == 0);
        ps_dma_cache_invalidate(&desc->dma_man, dma_buf.virt, ROUND_UP(desc->buf_size, 32));
        /* Ensure descriptor changes are observable before giving it to DMA */
        //dmb();
        __sync_synchronize();
        /* Adjust our index */
        desc->rx.head++;
        int rx_desc_wrap = FALSE;
        if(desc->rx.head == desc->rx.count){
            rx_desc_wrap = TRUE;
            desc->rx.head = 0;
        }
        d_fn->ready_rx_desc(i, rx_desc_wrap, driver);
        desc->rx.unused--;
        /* Must ensure changes are observable before signalling the MAC */
        __sync_synchronize(); //dmb();
    }
}

int
desc_rxget(struct eth_driver *driver, dma_addr_t *buf, int *len)
{
    struct desc *desc = driver->desc;
    struct raw_desc_funcs *d_fn = driver->d_fn;
    int i, error;
    i = desc->rx.tail;
    if (desc->rx.count == desc->rx.unused || d_fn->is_rx_desc_empty(i, desc)) {
        return 0;
    }

    *buf = desc->rx.buf[i];
    *len = d_fn->get_rx_buf_len(i, desc);

    /* It is possible that between the invalidate in rxput and now some cache lines
     * were speculatively loaded as a result of nearby reads. Therefore we need to
     * invalidate this range again to prevent reading stale data from the cache. round
     * up to the l2 line size (of which the buf size is guaranteed to be a multiple of)
     * to prevent the OS from needing to do partial line clean/invalidates */
    ps_dma_cache_invalidate(&desc->dma_man, buf->virt, ROUND_UP(*len, 32));

    error = d_fn->get_rx_desc_error(i, desc);

    /* move reader to the next buffer and increase unused count */
    desc->rx.tail = (desc->rx.tail + 1) % desc->rx.count;
    desc->rx.unused++;
        
    /* attempt to refill the RX buffer */
    desc_rxrefill(driver);
        
    if (error) {
        LOG_ERROR("Received error %d in receiver", error);
        return -1;
    } else {
        return 1;
    }
}

void
desc_rxfree(struct eth_driver *driver, dma_addr_t dma) 
{
    free_dma_buf(driver->desc, dma);
    /* we may have failed to install an RX buffer in the past */
    desc_rxrefill(driver);
}

int 
desc_txcomplete(struct desc *desc, struct raw_desc_funcs *d_fn) 
{
    int i;
    for (i = desc->tx.head
         ; desc->tx.unused < desc->tx.count && !(d_fn->is_tx_desc_ready(i, desc))
         ; i = (i + 1) % desc->tx.count) {
        /* TODO check for errors */
        if (desc->tx.buf_cookies[i]) {
            desc->tx.complete_funcs[i](desc->tx.buf_cookies[i]);
        } else {
            free_dma_buf(desc, desc->tx.buf[i]);
        }
        desc->tx.unused++;
    }
    desc->tx.head = i;
    return desc->tx.unused == desc->tx.count;
}
