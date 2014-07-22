/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __DESC_H
#define __DESC_H

#include "raw_iface.h"
#include "raw_descriptors.h"
#include <platsupport/io.h>

/**
 * Handles descriptors
 * Currently only allows get/put operations on the next buffer such that
 * "get" will always return the same buffer until a successful call to "put"
 * is made. It should be a trivial extension to correct this if needed.
 *
 * @Author Alexander Kroh
 */

struct desc_ring {
    dma_addr_t ring;
    dma_addr_t* buf;
    /* Every entry in the ring may have an associated cookie and
     * completion func. Currently this is only used for the TX ring.
     * A NULL cookie means the buf was allocated from the internal
     * buffer pool and should be free'd back to it. Non null means
     * the associated complete_func should be called and passed
     * the cookie */
    void **buf_cookies;
    tx_complete_fn_t *complete_funcs;
    int count;
    int head;
    int tail;
    int unused;
};

struct desc {
    struct desc_ring rx;
    struct desc_ring tx;
    /* buffer pool. acts as a queue to try and optimize cache usage */
    dma_addr_t *pool_queue;
    int queue_index;
    int pool_size;
    int buf_size;
    ps_dma_man_t dma_man;
};

/**
 * Create and initialise descriptors
 * @param[in] dma_man    DMA manager for allocating dma buffers
 * @param[in] rx_count   Number of RX descriptors and buffers to create
 * @param[in] tx_count   Number of TX descriptors and buffers to create
 * @param[in] buf_count  Number of DMA buffers to allocate for both RX and TX purposes
 * @param[in] bufsize    Size of DMA buffers to allocate
 * @param[in] driver     Ethernet driver to use.
 * @return               A reference to the descriptor structure created.
 */
struct desc* desc_init(ps_dma_man_t *dma_man, int rx_count, int tx_count, int buf_count, 
                       int bufsize, struct eth_driver *driver);

/**
 * Free any memory alloctated for the descriptors
 * @param[in] desc   Descriptor previously returned by desc_init
 */
void desc_free(struct desc *desc);

/**
 * Resets ring buffers. All indexes will begin again from 0.
 * @param[in] driver   A reference to ethernet driver structure.
 */
void desc_reset(struct eth_driver *driver);

/**
 * Retrieve an available TX buffer. NOTE: subsequent calls will receive the
 * same buffer until @ref desc_txput is successfully called.
 * @param[in]  ldesc  A reference to descriptor structure
 * @param[out] dma    If successful, returns a representation of the next 
 *                    available buffer.
 * @return            The size of the buffer, or 0 if no buffers are available.
 */
int desc_txget(struct desc* desc, dma_addr_t* dma, struct raw_desc_funcs *d_fn);

/**
 * Return a TX buffer back into the ring 
 * @param[in]  driver A reference to ethernet driver structure.
 * @param[in]  buf    The buffer to insert. The buffer will not
 *                    be added to the internal pool and can therefore have an
 *                    arbitrary size.
 *                    the descriptor structure was created.
 * @param[in]  len    The length of the provided frame.
 * @return            0 indicates success
 */
int desc_txput(struct eth_driver *driver, dma_addr_t buf, int len);

/**
 * Returns whether there is space for 'count' many buffers in the TX ring
 * If this function returns true then txput or txputmany for <=count is
 * guaranteed to succeed
 * @param[in]  desc   A reference to ethernet driver structure
 * @param[in]  count  Number of buffer spaces wanted
 * @return            0 if no buffers available
 */
int desc_txhasspace(struct desc *desc, int count);

/**
 * Place multiple buffers in the TX ring at once, all describing a single
 * packet
 * @param[in]  driver A reference to ethernet driver structure
 * @param[in]  buf    The buffers to insert.
 * @param[in]  func   Function to callback when transmit is complete
 * @param[in]  cookie Cookie to pass to the callback function
 * @return            0 indicates success
 */
int desc_txputmany(struct eth_driver *driver, ethif_scatter_buf_t *buf,
                    tx_complete_fn_t func, void *cookie);

/**
 * Retrieve a filled RX buffer. A new buffer will be allocated and placed in the
 * ring, if no buffer can be allocated then it will try and allocate again next
 * time rxfree is called. You should call rxfree when you are finished using this buffer.
 * @param[in]  driver A reference to ethernet driver structure
 * @param[out] buf    If successful, returns a representation of a received frame.
 * @param[out] len    The length of the received frame.
 * @return            0 if there the queue is empty
 *                    1 if buf contains a valid frame.
 *                   -1 if buf contains an error frame
 */
int desc_rxget(struct eth_driver *driver, dma_addr_t* buf, int* len);

/**
 * Return an RX buffer that is not longer needed
 * @param[in]  driver A reference to ethernet driver structure
 * @param[in]  dma    The buffer to free.
 */
void desc_rxfree(struct eth_driver *driver, dma_addr_t dma);

/**
 * Cycles through and cleans up empty TX descriptors  At the least, this
 * function must report on whether or not there are descriptors waiting
 * to be sent.
 * @param[in]  ldesc  A reference to legacy descriptor structure
 * @return            1 if there are no more descriptors pending
 */
int desc_txcomplete(struct desc* desc, struct raw_desc_funcs *d_fn);

/* Print descriptors */
void desc_print(struct desc* desc);

#endif /* __DESC_H */

