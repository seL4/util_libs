/*
 *  Copyright 2017, Data61
 *  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 *  ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#ifdef CONFIG_LIB_PICOTCP

#include <platsupport/io.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <pico_device.h>
#include <stdint.h>

#include <pico_stack.h>
#include <pico_device.h>


typedef struct pico_device_eth {
    // Pico device, wrapped inside this eth device
    struct pico_device pico_dev;
    
    // Underlying ethernet driver struct
    struct eth_driver driver;
    
    // Buffer management
    ps_dma_man_t dma_man;
    int num_free_bufs;
    dma_addr_t **bufs;
    dma_addr_t * dma_bufs; 

    int next_free_buf;
    int *buf_pool;
    int *rx_queue;
    int *rx_lens;
    int rx_count;    
                                                 
} pico_device_eth;

/*
 * Creates a pico device and initialises the ethernet driver.
 */
struct pico_device *pico_eth_create(char *name, ethif_driver_init driver_init, void *driver_config, ps_io_ops_t io_ops);

struct pico_device *pico_eth_create_no_malloc(char *name, ethif_driver_init driver_init, void *driver_config, ps_io_ops_t io_ops, pico_device_eth *pico_dev);

/* Wrapper function for a picotcp driver for asking the underlying
 * eth driver to handle an IRQ */
static inline void ethif_pico_handle_irq(pico_device_eth *iface, int irq) {
    iface->driver.i_fn.raw_handleIRQ(&iface->driver, irq);
}

#endif // CONFIG_LIB_PICOTCP

