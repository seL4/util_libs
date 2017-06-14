/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

 #include <platsupport/io.h>
 #include <ethdrivers/raw.h>
 #include <ethdrivers/helpers.h>
 #include <pico_device.h>
 #include <stdint.h>

typedef struct pico_device_eth {
    // Pico device, wrapped inside this eth device
    struct pico_device pico_dev;
    
    // Underlying ethernet driver struct
    struct eth_driver driver;
    
    // Buffer management
    ps_dma_man_t dma_man;
    int num_free_bufs;
    dma_addr_t **bufs;

    // Pico buffer
    int num_free_rx_bufs;
    dma_addr_t **rx_bufs;
    int *rx_lens;
                                                      
} pico_device_eth;

 /**
  * Creates a pico device and initialises the ethernet driver.
  */
struct pico_device *pico_eth_create(char *name, ethif_driver_init driver_init, void *driver_config, ps_io_ops_t io_ops);

/* Wrapper function for an LWIP driver for asking the underlying
 * eth driver to handle an IRQ */
static inline void ethif_pico_handle_irq(pico_device_eth *iface, int irq) {
    iface->driver.i_fn.raw_handleIRQ(&iface->driver, irq);
}

