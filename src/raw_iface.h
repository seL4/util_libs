/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#ifndef __ETHIFACE_RAW_IFACE_H__
#define __ETHIFACE_RAW_IFACE_H__

#include <stdint.h>
#include <stdlib.h>
#include <platsupport/io.h>
#include <lwip/netif.h>

struct eth_driver {
    void* eth_data;
    struct raw_iface_funcs* r_fn;
    struct raw_desc_funcs* d_fn;
    struct desc* desc;
    ps_io_ops_t io_ops; 
};
    
/* for simplicity we pin DMA memory on allocation, so always store pair
 * of physical / virtual */
typedef struct dma_addr {
    void *virt;
    uintptr_t phys;
}dma_addr_t;

typedef struct ethif_scatter_buf {
    int count;
    struct {
        dma_addr_t buf;
        size_t len;
    } bufs[1];
} ethif_scatter_buf_t;

/*
 * This will be called when a packet arrives
 * @param packet the physical address of the received packet
 * @param len the length of the received packet
 */
typedef int (*rx_cb_t)(uintptr_t buf, int len);

/* Called after a tx packet has been sent */
typedef void (*tx_complete_fn_t)(void *cookie);

/* Sends a packet that is split across multiple buffers. Also give a function
 * to call when the tx has finished. */
//typedef int (*ethif_rawscattertx_t)(struct eth_driver *driver, ethif_scatter_buf_t *buf,
//                       tx_complete_fn_t func, void *cookie);

/* Driver specific handling of IQRs */
//typedef void (*ethif_raw_handleIRQ_t)(struct eth_driver* driver, int irq, rx_cb_t rxcb);
typedef void (*ethif_raw_handleIRQ_t)(struct netif *netif, int irq);

/* Driver specific part of enabling interrupts */
typedef const int* (*ethif_raw_enableIRQ_t)(struct eth_driver *driver, int *nirqs);

/* Driver specific part of ethif_init */
typedef void (*ethif_low_level_init_t)(struct netif* netif);

/* Start TX logic. For when IRQs not enabled. 
 * TODO: check this is a good way to do this
 */
typedef void (*ethif_start_tx_logic_t)(struct netif* netif);
typedef void (*ethif_start_rx_logic_t)(struct netif* netif);

/* Debug methods */
typedef void (*ethif_print_state_t)(struct eth_driver* driver);

/* Struct to hold iface functions to make them easier to pass them around */
struct raw_iface_funcs {
 //   ethif_rawscattertx_t rawscattertx;
    ethif_raw_handleIRQ_t raw_handleIRQ;
    ethif_raw_enableIRQ_t raw_enableIRQ;
    ethif_print_state_t print_state;
    ethif_low_level_init_t low_level_init;
    ethif_start_tx_logic_t start_tx_logic;
    ethif_start_rx_logic_t start_rx_logic;
};
#endif /* __ETHIFACE_RAW_IFACE_H__ */

