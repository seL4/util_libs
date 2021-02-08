/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <autoconf.h>
#include <ethdrivers/gen_config.h>
#include <lwip/gen_config.h>

#ifdef CONFIG_LIB_LWIP

#include <platsupport/io.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <lwip/netif.h>
#include <stdint.h>

/* Structure describing an LWIP interface to an ethernet driver.
 * This structure is defined publicly for performance reasons
 * but should not be used directly */
typedef struct lwip_iface {
    struct eth_driver driver;
    netif_init_fn ethif_init;
    ps_dma_man_t dma_man;
    struct netif *netif;

    int num_free_bufs;
    dma_addr_t **bufs;
} lwip_iface_t;

/**
 * Initialize an ethernet driver and hook it up to LWIP. This is
 * will create an LWIP compatible interface to the driver, and
 * also fullfills the driver requirements for allocating
 * receive buffers. This interface will either use preallocated
 * dma buffers, and performing copying to and from them, or
 * attempt to dma directly from the lwip pbufs.
 * The returned lwip_iface should be passed to netif_add along
 * with the init function from ethif_get_ethif_init
 *
 * @param[in] io_ops    Structure for OS specific data and functions
 *                      If pbuf_dma is NULL this will be used for
 *                      allocating the DMA buffers
 * @param[in] pbuf_dma  Optional reference to a DMA allocator for
 *                      DMA'ing from pbufs. This interface will only
 *                      be used to pin/upin memory, never allocate or
 *                      free. If specified pbufs will be allocated for
 *                      receive buffers. If pbufs fail to pin there is
 *                      no fallback and receive/transmit will fail.
 *                      This data structure will be copied and no
 *                      no reference to this pointer will be retained
 * @param[in] driver    Function to use to initialize a driver
 *                      see description of ethif_driver_init in raw.h
 * @param[in] driver_config Pointer to driver specific config struct
 *                      that will be passed to the driver init function
 * @return              Pointer to initialized lwip_iface struct, or
 *                      NULL on error
 */
lwip_iface_t *ethif_new_lwip_driver(ps_io_ops_t io_ops, ps_dma_man_t *pbuf_dma, ethif_driver_init driver, void *driver_config);

/**
 * Same as ethif_new_lwip_driver except if an allocated iface
 * and pbuf_dma is passed then malloc will not get called
 */
lwip_iface_t *ethif_new_lwip_driver_no_malloc(ps_io_ops_t io_ops, ps_dma_man_t *pbuf_dma, ethif_driver_init driver, void *driver_config, lwip_iface_t *iface);

/* Wrapper function for an LWIP driver for asking the underlying
 * eth driver to handle an IRQ */
static inline void ethif_lwip_handle_irq(lwip_iface_t *iface, int irq) {
    iface->driver.i_fn.raw_handleIRQ(&iface->driver, irq);
}

/* Wrapper function for an LWIP driver for asking the underlying
 * eth driver to manual check its state in the absence of interrupts */
static inline void ethif_lwip_poll(lwip_iface_t *iface) {
    iface->driver.i_fn.raw_poll(&iface->driver);
}

/* Retrieve the netif_init_fn for this iface for passing to netif_add */
static inline netif_init_fn ethif_get_ethif_init(lwip_iface_t *iface) {
    return iface->ethif_init;
}

#endif
